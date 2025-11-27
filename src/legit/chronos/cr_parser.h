#ifndef CR_PARSER_H
#define CR_PARSER_H

#include "cr_vm.h"

CR_API int cr_eval(cr_engine *engine, const char *script, int reset);
CR_API int cr_get_variable_node(cr_context *ctx, const char *name);
CR_API void cr_set_node_value(cr_context *ctx, int node_idx, double value);
CR_API int cr_find_adsr_node_for_gate(cr_context *ctx, int gate_node_idx);

#ifdef CR_PARSER_IMPLEMENTATION

static void cr_log(cr_engine *eng, int level, const char *fmt, ...) {
  va_list args;
  if (level <= eng->log_level) {
    printf((level==CR_LOG_ERROR)?"[ERROR] ":"[INFO] ");
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
    printf("\n");
  }
}

static void cr_error(cr_context *ctx, const char *msg) {
  snprintf(ctx->error_msg, 128, "Line %d: %s", ctx->current_line, msg);
  longjmp(ctx->err_jmp, 1);
}

static void* cr_arena_alloc(cr_context* ctx, size_t size) {
    size_t aligned = (size + 7) & ~7;
    if (ctx->arena_top + aligned > ctx->arena_size) { cr_error(ctx, "Memory Arena Exceeded"); return NULL; }
    {
        void* ptr = ctx->arena_base + ctx->arena_top;
        ctx->arena_top += aligned;
        return ptr;
    }
}

static int alloc_node(cr_context *ctx, const cr_op_desc *op_desc) {
  if (ctx->node_idx >= CR_MAX_NODES) cr_error(ctx, "Max nodes reached");
  {
      int idx = ctx->node_idx++;
      cr_node *n = &ctx->node_pool[idx];
      memset(n, 0, sizeof(cr_node));
      n->id = idx;
      n->op_desc = op_desc;
      
      if (ctx->active_block_ptr > 0) {
          n->block_id = ctx->active_block_stack[ctx->active_block_ptr - 1];
          n->control_node = ctx->block_dependency_stack[ctx->active_block_ptr - 1];
      } else {
          n->block_id = 0;
          n->control_node = -1;
      }
      return idx;
  }
}

CR_API int cr_get_variable_node(cr_context *ctx, const char *name) {
    int i;
    for (i = 0; i < ctx->var_count; i++) {
        if (strcmp(ctx->variables[i].name, name) == 0) {
            return ctx->variables[i].node_index;
        }
    }
    return -1;
}

CR_API void cr_set_node_value(cr_context *ctx, int node_idx, double value) {
    if (node_idx >= 0 && node_idx < ctx->node_idx) {
        cr_node *n = &ctx->node_pool[node_idx];
        if (n->op_desc && n->op_desc->opcode == OP_CONST) {
            n->value = make_float(value);
        }
    }
}

CR_API int cr_find_adsr_node_for_gate(cr_context *ctx, int gate_node_idx) {
    int i;
    for (i = 0; i < ctx->node_idx; i++) {
        cr_node *n = &ctx->node_pool[i];
        if (n->op_desc && n->op_desc->opcode == OP_ADSR) {
            if (n->input_count > 0 && n->inputs[0] == gate_node_idx) {
                return i;
            }
        }
    }
    return -1;
}

static void get_tok(cr_context *ctx) {
    int i = 0;
    while (1) {
        while (isspace((unsigned char)*ctx->src_ptr)) {
            if (*ctx->src_ptr == '\n') ctx->current_line++;
            ctx->src_ptr++;
        }
        if (*ctx->src_ptr == 0) { ctx->token_type = 0; return; }
        if (*ctx->src_ptr == '#') {
            while (*ctx->src_ptr != '\n' && *ctx->src_ptr != 0) ctx->src_ptr++;
            continue;
        }
        break;
    }
    {
        char c = *ctx->src_ptr;
        if (isalpha(c) || c == '_') {
            while (isalnum((unsigned char)*ctx->src_ptr) || *ctx->src_ptr == '_' || *ctx->src_ptr == '.') {
                if (i < 127) ctx->token[i++] = *ctx->src_ptr++; else ctx->src_ptr++;
            }
            ctx->token[i] = 0; ctx->token_type = 1;
        } else if (isdigit(c) || (c == '-' && isdigit(ctx->src_ptr[1]))) {
            ctx->token[i++] = *ctx->src_ptr++;
            while (isdigit((unsigned char)*ctx->src_ptr) || *ctx->src_ptr == '.' || *ctx->src_ptr == 'x') {
                if (i < 127) ctx->token[i++] = *ctx->src_ptr++; else ctx->src_ptr++;
            }
            ctx->token[i] = 0; ctx->token_type = (strchr(ctx->token, '.') ? 4 : 2);
        } else if (c == '"') {
            ctx->src_ptr++;
            while (*ctx->src_ptr != '"' && *ctx->src_ptr) {
                 if (i < 127) ctx->token[i++] = *ctx->src_ptr++; else ctx->src_ptr++;
            }
            if (*ctx->src_ptr == '"') ctx->src_ptr++;
            ctx->token[i] = 0; ctx->token_type = 5;
        } else {
            ctx->token[0] = *ctx->src_ptr++;
            {
                char n = *ctx->src_ptr;
                if ((c=='='||c=='!'||c=='<'||c=='>') && n=='=') { ctx->token[1]=n; ctx->src_ptr++; ctx->token[2]=0; }
                else if ((c=='&'&&n=='&')||(c=='|'&&n=='|')) { ctx->token[1]=n; ctx->src_ptr++; ctx->token[2]=0; }
                else ctx->token[1]=0;
                ctx->token_type = 3;
            }
        }
    }
}

static void match(cr_context *ctx, const char *str) {
    if (strcmp(ctx->token, str) != 0) {
        char err[160]; snprintf(err, 160, "Expected '%s' found '%s'", str, ctx->token);
        cr_error(ctx, err);
    }
    get_tok(ctx);
}

static int expr(cr_context *ctx);
static void statement(cr_context *ctx);

static int factor(cr_context *ctx) {
    int idx = -1;
    const cr_op_desc *op_desc = NULL;
    if (ctx->token_type == 2) { 
        op_desc = cr_lookup_op_by_name("const");
        idx = alloc_node(ctx, op_desc);
        ctx->node_pool[idx].value = make_int(atoi(ctx->token));
        ctx->node_pool[idx].is_constant = 1;
        get_tok(ctx);
    } else if (ctx->token_type == 4) { 
        op_desc = cr_lookup_op_by_name("const");
        idx = alloc_node(ctx, op_desc);
        ctx->node_pool[idx].value = make_float(atof(ctx->token));
        ctx->node_pool[idx].is_constant = 1;
        get_tok(ctx);
    } else if (ctx->token_type == 1) { 
        char name[128]; snprintf(name, 128, "%s", ctx->token);
        get_tok(ctx);
        if (strcmp(ctx->token, "(") == 0) {
            if (!strcmp(name, "rhythm")) {
                get_tok(ctx); 
                if (ctx->token_type != 5) cr_error(ctx, "rhythm() expects string");
                op_desc = cr_lookup_op_by_name("rhythm");
                idx = alloc_node(ctx, op_desc);
                {
                    int len = strlen(ctx->token);
                    double* buf = (double*)cr_arena_alloc(ctx, len * sizeof(double));
                    if (buf) {
                        int i;
                        for(i=0; i<len; i++) buf[i] = (ctx->token[i] == 'x' || ctx->token[i] == 'X') ? 1.0 : 0.0;
                    }
                    ctx->node_pool[idx].dsp.buffer = buf;
                    ctx->node_pool[idx].dsp.buf_len = len;
                    ctx->node_pool[idx].is_constant = 1;
                }
                get_tok(ctx); match(ctx, ")");
                if (ctx->active_block_ptr > 0) {
                    int ctrl = ctx->block_dependency_stack[ctx->active_block_ptr-1];
                    if (ctrl != -1) ctx->node_pool[idx].inputs[ctx->node_pool[idx].input_count++] = ctrl;
                }
                return idx;
            }
            
            op_desc = cr_lookup_op_by_name(name);
            if (op_desc) {
                get_tok(ctx); 
                idx = alloc_node(ctx, op_desc);
                
                if (op_desc->opcode == OP_DELAY) {
                    int len = (int)(ctx->sample_rate * 4.0); 
                    int p2 = 1; while(p2 < len) p2 <<= 1; len = p2;
                    ctx->node_pool[idx].dsp.buffer = (double*)cr_arena_alloc(ctx, len * sizeof(double));
                    ctx->node_pool[idx].dsp.buf_len = len;
                    if(ctx->node_pool[idx].dsp.buffer) memset(ctx->node_pool[idx].dsp.buffer, 0, len * sizeof(double));
                } else if (op_desc->opcode == OP_REVERB) {
                    int len = 65536; 
                    ctx->node_pool[idx].dsp.buffer = (double*)cr_arena_alloc(ctx, len * sizeof(double));
                    ctx->node_pool[idx].dsp.buf_len = len;
                    if(ctx->node_pool[idx].dsp.buffer) memset(ctx->node_pool[idx].dsp.buffer, 0, len * sizeof(double));
                }
                
                {
                    int arg_c = 0;
                    if (strcmp(ctx->token, ")") != 0) {
                        while(1) {
                            int arg = expr(ctx);
                            if (arg_c < CR_MAX_ARGS) ctx->node_pool[idx].inputs[arg_c++] = arg;
                            if (strcmp(ctx->token, ",") == 0) get_tok(ctx); else break;
                        }
                    }
                    if (ctx->active_block_ptr > 0) {
                         int ctrl = ctx->block_dependency_stack[ctx->active_block_ptr-1];
                         if (ctrl != -1 && arg_c < CR_MAX_ARGS) ctx->node_pool[idx].inputs[arg_c++] = ctrl;
                    }
                    ctx->node_pool[idx].input_count = arg_c;
                }
                match(ctx, ")");
                return idx;
            } else {
                int m_idx = -1;
                int m;
                for(m=0; m<ctx->macro_count; m++) {
                    if(!strcmp(ctx->macros[m].name, name)) { m_idx = m; break; }
                }
                if (m_idx != -1) {
                    int args[CR_MAX_ARGS]; int argc = 0;
                    if (strcmp(ctx->token, ")") != 0) {
                        while(1) {
                            if (argc < CR_MAX_ARGS) args[argc++] = expr(ctx);
                            if (strcmp(ctx->token, ",") == 0) get_tok(ctx); else break;
                        }
                    }
                    match(ctx, ")");
                    {
                        char old_scope[128]; strcpy(old_scope, ctx->scope);
                        const char* old_src = ctx->src_ptr;
                        char old_tok[128]; strcpy(old_tok, ctx->token);
                        int old_type = ctx->token_type;
                        int old_ret = ctx->returning;
                        int i;
                        snprintf(ctx->scope, 128, "%s%s%d_", old_scope, name, ctx->scope_id_ctr++);
                        for(i=0; i<argc && i<ctx->macros[m_idx].arg_count; i++) {
                             char scoped_name[64];
                             snprintf(scoped_name, 64, "%s%s", ctx->scope, ctx->macros[m_idx].args[i]);
                             if (ctx->var_count < CR_MAX_SYMBOLS) {
                                 strcpy(ctx->variables[ctx->var_count].name, scoped_name);
                                 ctx->variables[ctx->var_count].node_index = args[i];
                                 ctx->var_count++;
                             }
                        }
                        ctx->src_ptr = ctx->macros[m_idx].body;
                        ctx->returning = 0;
                        get_tok(ctx);
                        while(ctx->token_type != 0 && !ctx->returning) statement(ctx);
                        idx = ctx->return_reg;
                        strcpy(ctx->scope, old_scope);
                        ctx->src_ptr = old_src;
                        strcpy(ctx->token, old_tok);
                        ctx->token_type = old_type;
                        ctx->returning = old_ret;
                    }
                } else {
                    char err[160]; snprintf(err, 160, "Unknown func '%s'", name); cr_error(ctx, err);
                }
            }
        } else {
            char scoped[128]; snprintf(scoped, 128, "%s%s", ctx->scope, name);
            int found = -1;
            int i;
            for(i=0; i<ctx->var_count; i++) { if (!strcmp(ctx->variables[i].name, scoped)) { found = ctx->variables[i].node_index; break; } }
            if (found == -1 && strlen(ctx->scope) > 0) {
                for(i=0; i<ctx->var_count; i++) { if (!strcmp(ctx->variables[i].name, name)) { found = ctx->variables[i].node_index; break; } }
            }
            if (found == -1) {
                if (!strcmp(name, "time")) {
                    op_desc = cr_lookup_op_by_name("time");
                    idx = alloc_node(ctx, op_desc);
                } else if (!strcmp(name, "bars")) {
                    op_desc = cr_lookup_op_by_name("bars");
                    idx = alloc_node(ctx, op_desc);
                } else { char err[160]; snprintf(err, 160, "Unknown var '%s'", name); cr_error(ctx, err); }
            } else idx = found;
        }
    } else if (!strcmp(ctx->token, "(")) {
        get_tok(ctx); idx = expr(ctx); match(ctx, ")");
    } else {
        cr_error(ctx, "Syntax Error: Unexpected token");
    }
    return idx;
}

static int term(cr_context *ctx) { 
    int left = factor(ctx);
    while (!strcmp(ctx->token, "*") || !strcmp(ctx->token, "/") || !strcmp(ctx->token, "%")) {
        const cr_op_desc *op_desc = NULL;
        if (!strcmp(ctx->token, "*")) op_desc = cr_lookup_op_by_name("mul");
        else if (!strcmp(ctx->token, "/")) op_desc = cr_lookup_op_by_name("div");
        else if (!strcmp(ctx->token, "%")) op_desc = cr_lookup_op_by_name("mod");
        get_tok(ctx); int right = factor(ctx);
        if (!op_desc) cr_error(ctx, "Internal Error: Unknown operator");
        {
            int n = alloc_node(ctx, op_desc);
            ctx->node_pool[n].inputs[0] = left; ctx->node_pool[n].inputs[1] = right;
            ctx->node_pool[n].input_count = 2; left = n;
        }
    }
    return left;
}

static int sum(cr_context *ctx) {
    int left = term(ctx);
    while (!strcmp(ctx->token, "+") || !strcmp(ctx->token, "-")) {
        const cr_op_desc *op_desc = NULL;
        if (!strcmp(ctx->token, "+")) op_desc = cr_lookup_op_by_name("add");
        else if (!strcmp(ctx->token, "-")) op_desc = cr_lookup_op_by_name("sub");
        get_tok(ctx); int right = term(ctx);
        if (!op_desc) cr_error(ctx, "Internal Error: Unknown operator");
        {
            int n = alloc_node(ctx, op_desc);
            ctx->node_pool[n].inputs[0] = left; ctx->node_pool[n].inputs[1] = right;
            ctx->node_pool[n].input_count = 2; left = n;
        }
    }
    return left;
}

static int relation(cr_context *ctx) {
    int left = sum(ctx);
    if (!strcmp(ctx->token, "<") || !strcmp(ctx->token, ">") || !strcmp(ctx->token, ">=") || !strcmp(ctx->token, "<=")) {
        const cr_op_desc *op_desc = NULL; 
        if (!strcmp(ctx->token, "<")) op_desc = cr_lookup_op_by_name("lt");
        else if (!strcmp(ctx->token, ">")) op_desc = cr_lookup_op_by_name("gt");
        else if (!strcmp(ctx->token, ">=")) op_desc = cr_lookup_op_by_name("ge");
        else if (!strcmp(ctx->token, "<=")) op_desc = cr_lookup_op_by_name("le");
        get_tok(ctx); int right = sum(ctx);
        if (!op_desc) cr_error(ctx, "Internal Error: Unknown operator");
        {
            int n = alloc_node(ctx, op_desc);
            ctx->node_pool[n].inputs[0] = left; ctx->node_pool[n].inputs[1] = right;
            ctx->node_pool[n].input_count = 2; left = n;
        }
    }
    return left;
}

static int equality(cr_context *ctx) {
    int left = relation(ctx);
    if (!strcmp(ctx->token, "==") || !strcmp(ctx->token, "!=")) {
        const cr_op_desc *op_desc = NULL;
        if (!strcmp(ctx->token, "==")) op_desc = cr_lookup_op_by_name("eq");
        else if (!strcmp(ctx->token, "!=")) op_desc = cr_lookup_op_by_name("ne");
        get_tok(ctx); int right = relation(ctx);
        if (!op_desc) cr_error(ctx, "Internal Error: Unknown operator");
        {
            int n = alloc_node(ctx, op_desc);
            ctx->node_pool[n].inputs[0] = left; ctx->node_pool[n].inputs[1] = right;
            ctx->node_pool[n].input_count = 2; left = n;
        }
    }
    return left;
}

static int logic_and(cr_context *ctx) {
    int left = equality(ctx);
    while (!strcmp(ctx->token, "&&")) {
        const cr_op_desc *op_desc = cr_lookup_op_by_name("and");
        get_tok(ctx); int right = equality(ctx);
        if (!op_desc) cr_error(ctx, "Internal Error: Unknown operator");
        {
            int n = alloc_node(ctx, op_desc);
            ctx->node_pool[n].inputs[0] = left; ctx->node_pool[n].inputs[1] = right;
            ctx->node_pool[n].input_count = 2; left = n;
        }
    }
    return left;
}

static int logic_or(cr_context *ctx) {
    int left = logic_and(ctx);
    while (!strcmp(ctx->token, "||")) {
        const cr_op_desc *op_desc = cr_lookup_op_by_name("or");
        get_tok(ctx); int right = logic_and(ctx);
        if (!op_desc) cr_error(ctx, "Internal Error: Unknown operator");
        {
            int n = alloc_node(ctx, op_desc);
            ctx->node_pool[n].inputs[0] = left; ctx->node_pool[n].inputs[1] = right;
            ctx->node_pool[n].input_count = 2; left = n;
        }
    }
    return left;
}

static int expr(cr_context *ctx) { return logic_or(ctx); }

static void statement(cr_context *ctx) {
    if (strcmp(ctx->token, "def") == 0) {
        if (ctx->macro_count >= CR_MAX_MACROS) cr_error(ctx, "Max macros");
        {
            cr_macro *m = &ctx->macros[ctx->macro_count++];
            const char *scan;
            long len;
            int depth;
            int i;
            get_tok(ctx); strcpy(m->name, ctx->token);
            get_tok(ctx); match(ctx, "(");
            m->arg_count = 0;
            if (strcmp(ctx->token, ")") != 0) {
                while(1) {
                    if(m->arg_count < CR_MAX_ARGS) strcpy(m->args[m->arg_count++], ctx->token);
                    get_tok(ctx);
                    if (strcmp(ctx->token, ",") == 0) get_tok(ctx); else break;
                }
            }
            match(ctx, ")");
            if (strcmp(ctx->token, "{") != 0) cr_error(ctx, "Expected '{'");
            {
                const char *body_start = ctx->src_ptr;
                depth = 1;
                scan = body_start;
                while (*scan && depth > 0) {
                    if (*scan == '#') { while (*scan && *scan != '\n') scan++; }
                    else if (*scan == '{') { depth++; scan++; }
                    else if (*scan == '}') { depth--; if(depth==0) break; scan++; }
                    else scan++;
                }
                if (depth != 0) cr_error(ctx, "Unbalanced macro braces");
                len = scan - body_start;
                if (len >= CR_MAX_MACRO_SIZE) cr_error(ctx, "Macro too big");
                strncpy(m->body, body_start, len);
                m->body[len] = 0;
                ctx->src_ptr = scan + 1; 
                get_tok(ctx);
            }
        }
        return;
    }

    if (strcmp(ctx->token, "return") == 0) {
        get_tok(ctx);
        ctx->return_reg = expr(ctx);
        ctx->returning = 1;
        return;
    }

    if (strcmp(ctx->token, "if") == 0) {
        const cr_op_desc *ctrl_desc = cr_lookup_op_by_name("branch_ctrl");
        const cr_op_desc *not_desc = cr_lookup_op_by_name("not");
        const cr_op_desc *select_desc = cr_lookup_op_by_name("select");
        const cr_op_desc *const_desc = cr_lookup_op_by_name("const");
        
        get_tok(ctx); match(ctx, "(");
        {
            int cond = expr(ctx); match(ctx, ")"); match(ctx, "{");
            
            int snap_nodes[CR_MAX_SYMBOLS];
            int snap_count = ctx->var_count;
            int i;
            for(i=0; i<snap_count; i++) snap_nodes[i] = ctx->variables[i].node_index;

            {
                int blk = ++ctx->block_id_counter;
                int ctrl = alloc_node(ctx, ctrl_desc);
                ctx->node_pool[ctrl].inputs[0] = cond;
                {
                    int bconst = alloc_node(ctx, const_desc);
                    ctx->node_pool[bconst].value = make_float((double)blk);
                    ctx->node_pool[bconst].is_constant = 1;
                    ctx->node_pool[ctrl].inputs[1] = bconst; 
                    ctx->node_pool[ctrl].input_count = 2;
                }
                ctx->active_block_stack[ctx->active_block_ptr] = blk;
                ctx->block_dependency_stack[ctx->active_block_ptr] = ctrl;
                ctx->active_block_ptr++;
                
                while (strcmp(ctx->token, "}") != 0 && ctx->token_type != 0 && !ctx->returning) statement(ctx);
                match(ctx, "}");
                ctx->active_block_ptr--;
            }
            
            {
                int if_nodes[CR_MAX_SYMBOLS]; 
                for(i=0; i<snap_count; i++) {
                    if_nodes[i] = (ctx->variables[i].node_index != snap_nodes[i]) ? ctx->variables[i].node_index : -1;
                    ctx->variables[i].node_index = snap_nodes[i];
                }

                if (strcmp(ctx->token, "else") == 0) {
                    get_tok(ctx); match(ctx, "{");
                    {
                        int blk2 = ++ctx->block_id_counter;
                        int not_n = alloc_node(ctx, not_desc);
                        ctx->node_pool[not_n].inputs[0] = cond; ctx->node_pool[not_n].input_count = 1;
                        int ctrl2 = alloc_node(ctx, ctrl_desc);
                        ctx->node_pool[ctrl2].inputs[0] = not_n;
                        {
                            int bconst2 = alloc_node(ctx, const_desc);
                            ctx->node_pool[bconst2].value = make_float((double)blk2);
                            ctx->node_pool[bconst2].is_constant = 1;
                            ctx->node_pool[ctrl2].inputs[1] = bconst2; 
                            ctx->node_pool[ctrl2].input_count = 2;
                        }
                        ctx->active_block_stack[ctx->active_block_ptr] = blk2;
                        ctx->block_dependency_stack[ctx->active_block_ptr] = ctrl2;
                        ctx->active_block_ptr++;
                        while (strcmp(ctx->token, "}") != 0 && ctx->token_type != 0 && !ctx->returning) statement(ctx);
                        match(ctx, "}");
                        ctx->active_block_ptr--;
                    }
                }
                
                for(i=0; i<snap_count; i++) {
                    int node_if = if_nodes[i];
                    int node_else = ctx->variables[i].node_index;
                    int node_orig = snap_nodes[i];
                    
                    if (node_if != -1 || node_else != node_orig) {
                        int true_node = (node_if != -1) ? node_if : node_orig;
                        int false_node = (node_else != node_orig) ? node_else : node_orig;
                        int sel = alloc_node(ctx, select_desc);
                        ctx->node_pool[sel].inputs[0] = cond;
                        ctx->node_pool[sel].inputs[1] = true_node;
                        ctx->node_pool[sel].inputs[2] = false_node;
                        ctx->node_pool[sel].input_count = 3;
                        ctx->variables[i].node_index = sel;
                    }
                }
            }
        }
        return;
    }

    if (ctx->token_type == 1) {
        char name[128]; snprintf(name, 128, "%s", ctx->token);
        get_tok(ctx); match(ctx, "=");
        {
            int val = expr(ctx);
            int found = -1;
            int i;
            if (!strcmp(name, "out")) {
                ctx->output_nodes[0] = val; ctx->output_nodes[1] = val;
            } else {
                if (!strcmp(name, "bpm")) ctx->bpm_node_idx = val;
                {
                    char scoped[128]; snprintf(scoped, 128, "%s%s", ctx->scope, name);
                    for(i=0; i<ctx->var_count; i++) { if (!strcmp(ctx->variables[i].name, scoped)) { ctx->variables[i].node_index = val; found = 1; break; } }
                    if (found == -1) {
                        if(ctx->var_count < CR_MAX_SYMBOLS) {
                            strncpy(ctx->variables[ctx->var_count].name, scoped, 64);
                            ctx->variables[ctx->var_count].node_index = val;
                            ctx->var_count++;
                        }
                    }
                }
            }
        }
    }
}

static void topo_visit(cr_context *ctx, int u) {
    int i;
    if (ctx->visit_state[u] == 1) return;
    if (ctx->visit_state[u] == 2) return;
    ctx->visit_state[u] = 1;
    
    {
        cr_node* n = &ctx->node_pool[u];
        
        if (n->control_node != -1) topo_visit(ctx, n->control_node);
        
        for (i = 0; i < n->input_count; i++) topo_visit(ctx, n->inputs[i]);
        ctx->visit_state[u] = 2;
        ctx->exec_order[ctx->exec_count++] = u;
    }
}

static void build_exec_list(cr_context *ctx) {
    int c;
    memset(ctx->visit_state, 0, sizeof(ctx->visit_state));
    ctx->exec_count = 0;
    for(c=0; c<CR_MAX_CHANNELS; c++) { if (ctx->output_nodes[c] != -1) topo_visit(ctx, ctx->output_nodes[c]); }
}

CR_API int cr_eval(cr_engine *engine, const char *script, int reset) {
    size_t new_len = strlen(script);
    if (reset) {
        engine->source_len = 0;
        engine->source_history[0] = 0;
    }

    if (engine->source_len + new_len + 2 >= engine->source_capacity) {
        engine->source_capacity = (engine->source_len + new_len + 4096) * 2;
        engine->source_history = (char*)realloc(engine->source_history, engine->source_capacity);
    }
    strcat(engine->source_history, script);
    strcat(engine->source_history, "\n");
    engine->source_len += new_len + 1;

    {
        cr_context *back = engine->back;
        size_t new_script_len = new_len;
        
        back->node_idx = 0; 
        back->arena_top = 0; 
        back->var_count = 0; 
        back->exec_count = 0;
        back->output_nodes[0] = -1; back->output_nodes[1] = -1; 
        back->block_id_counter = 0; 
        back->macro_count = 0;
        back->bpm_node_idx = -1;
        memset(back->node_pool, 0, sizeof(cr_node) * CR_MAX_NODES);
        memset(back->block_skip_flags, 0, sizeof(back->block_skip_flags));
        
        back->global_time = engine->active->global_time;
        back->src_ptr = engine->source_history;
        back->current_line = 1; 
        back->scope[0] = 0; back->scope_id_ctr = 0;
        
        if (setjmp(back->err_jmp) != 0) { 
            cr_log(engine, CR_LOG_ERROR, "Compile Error: %s", back->error_msg); 
            if (!reset) {
                 engine->source_len -= (new_script_len + 1);
                 engine->source_history[engine->source_len] = 0;
            }
            return 0; 
        }

        get_tok(back);
        while (back->token_type != 0) statement(back);
        build_exec_list(back);
        
        #if defined(_WIN32)
        EnterCriticalSection(&engine->swap_lock);
        #else
        pthread_mutex_lock(&engine->swap_lock);
        #endif
        
        migrate_dsp_state(back, engine->active);
        
        {
            cr_context *temp = engine->active; 
            engine->active = back; 
            engine->back = temp;
        }
        
        #if defined(_WIN32)
        LeaveCriticalSection(&engine->swap_lock);
        #else
        pthread_mutex_unlock(&engine->swap_lock);
        #endif
        return 1;
    }
}
#endif
#endif

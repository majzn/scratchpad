/**
 * @file cr_parser.h
 * @brief Script parsing, DSP graph construction, and compile-time utility functions.
 *
 * This file implements the recursive descent parser, the tokenizer/lexer, the
 * code generation logic (node allocation and linking), and the necessary functions
 * to prepare the DSP graph for execution, including topological sorting.
 */
#ifndef CR_PARSER_H
#define CR_PARSER_H

#include "cr_vm.h"

/**
 * @brief Parses and evaluates a music script.
 *
 * Appends the new script to the source history, compiles the complete script
 * into the back context, performs topological sorting, and hot-swaps the
 * active context with the newly compiled one.
 *
 * @param engine The engine instance.
 * @param script The null-terminated string containing the new script code.
 * @param reset If true, the source history is cleared before processing the new script.
 * @return 1 on successful compilation and swap, 0 on compile error.
 */
CR_API int cr_eval(cr_engine *engine, const char *script, int reset);

/**
 * @brief Looks up the node index associated with a script variable name.
 *
 * Used primarily by the application layer (e.g., in main.c) to find controllable nodes.
 *
 * @param ctx The context to search (usually engine->active).
 * @param name The variable name (e.g., "NOTE_PITCH").
 * @return The index of the corresponding cr_node, or -1 if not found.
 */
CR_API int cr_get_variable_node(cr_context *ctx, const char *name);

/**
 * @brief Sets the constant value of a node, provided it is an OP_CONST node.
 *
 * This function allows runtime modification of constant values (e.g., changing BPM or PITCH).
 *
 * @param ctx The context containing the node.
 * @param node_idx The index of the node to modify.
 * @param value The new floating-point value to set.
 */
CR_API void cr_set_node_value(cr_context *ctx, int node_idx, double value);

/**
 * @brief Finds the ADSR node that takes a specific gate signal as its first input.
 *
 * Used by the application layer to find the ADSR envelope to trigger on key presses.
 *
 * @param ctx The context to search.
 * @param gate_node_idx The node index of the gate signal (e.g., "NOTE_GATE").
 * @return The index of the ADSR node, or -1 if not found.
 */
CR_API int cr_find_adsr_node_for_gate(cr_context *ctx, int gate_node_idx);

#ifdef CR_PARSER_IMPLEMENTATION

/**
 * @brief Internal function for logging compiler messages.
 *
 * Supports different log levels (ERROR, INFO, etc.).
 *
 * @param eng The engine instance (provides log level).
 * @param level The log level of the message.
 * @param fmt The format string.
 * @param ... Variable arguments for the format string.
 */
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

/**
 * @brief Handles a compilation error by storing the message and performing a long jump.
 *
 * Uses `longjmp` to abort parsing and return control to `cr_eval`.
 *
 * @param ctx The context containing the parser state and error message buffer.
 * @param msg The error message string.
 */
static void cr_error(cr_context *ctx, const char *msg) {
  snprintf(ctx->error_msg, 128, "Line %d: %s", ctx->current_line, msg);
  longjmp(ctx->err_jmp, 1);
}

/**
 * @brief Allocates memory from the context's internal arena.
 *
 * Used for dynamically sized data required by nodes, such as delay/reverb buffers
 * or the rhythm pattern array. Ensures 8-byte alignment.
 *
 * @param ctx The context containing the memory arena.
 * @param size The size of memory block to allocate (in bytes).
 * @return Pointer to the allocated memory, or NULL if arena is full.
 */
static void* cr_arena_alloc(cr_context* ctx, size_t size) {
    size_t aligned = (size + 7) & ~7;
    if (ctx->arena_top + aligned > ctx->arena_size) { cr_error(ctx, "Memory Arena Exceeded"); return NULL; }
    {
        void* ptr = ctx->arena_base + ctx->arena_top;
        ctx->arena_top += aligned;
        return ptr;
    }
}

/**
 * @brief Allocates and initializes a new node in the DSP graph.
 *
 * Increments `ctx->node_idx` and sets up the node's base properties.
 * It also handles linking the new node to the active conditional block if one exists.
 *
 * @param ctx The context where the node pool resides.
 * @param op_desc The operation descriptor for the new node.
 * @return The index of the newly allocated node.
 */
static int alloc_node(cr_context *ctx, const cr_op_desc *op_desc) {
  if (ctx->node_idx >= CR_MAX_NODES) cr_error(ctx, "Max nodes reached");
  {
      int idx = ctx->node_idx++;
      cr_node *n = &ctx->node_pool[idx];
      memset(n, 0, sizeof(cr_node));
      n->id = idx;
      n->op_desc = op_desc;
      
      /** Link the node to the current conditional block, if inside one */
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

/**
 * @brief Looks up a variable name and returns its associated node index.
 *
 * Searches for the fully scoped name first, then attempts an unscoped search
 * if within a macro (scope > 0).
 *
 * @param ctx The context containing the symbol table.
 * @param name The variable name to search for (unscoped).
 * @return The node index associated with the variable, or -1 if not found.
 */
CR_API int cr_get_variable_node(cr_context *ctx, const char *name) {
    int i;
    for (i = 0; i < ctx->var_count; i++) {
        if (strcmp(ctx->variables[i].name, name) == 0) {
            return ctx->variables[i].node_index;
        }
    }
    return -1;
}

/**
 * @brief Sets the constant value of an existing node in the graph.
 *
 * Only modifies nodes that are explicitly OP_CONST. Used by the application
 * to control script parameters at runtime.
 *
 * @param ctx The context containing the node pool.
 * @param node_idx The index of the node to modify.
 * @param value The new floating-point value.
 */
CR_API void cr_set_node_value(cr_context *ctx, int node_idx, double value) {
    if (node_idx >= 0 && node_idx < ctx->node_idx) {
        cr_node *n = &ctx->node_pool[node_idx];
        if (n->op_desc && n->op_desc->opcode == OP_CONST) {
            n->value = make_float(value);
        }
    }
}

/**
 * @brief Searches for an OP_ADSR node that uses the specified node as its gate input.
 *
 * @param ctx The context containing the node pool.
 * @param gate_node_idx The node index representing the gate signal.
 * @return The index of the ADSR node, or -1 if not found.
 */
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

/**
 * @brief Lexical analyzer (tokenizer). Reads the next token from the script source.
 *
 * Skips whitespace and comments (`#`). Determines the token type (identifier,
 * integer, float, string literal, or operator) and populates `ctx->token` and `ctx->token_type`.
 *
 * @param ctx The context containing the source pointer and token buffers.
 */
static void get_tok(cr_context *ctx) {
    int i = 0;
    while (1) {
        /** Skip whitespace and advance line counter on newline */
        while (isspace((unsigned char)*ctx->src_ptr)) {
            if (*ctx->src_ptr == '\n') ctx->current_line++;
            ctx->src_ptr++;
        }
        if (*ctx->src_ptr == 0) { ctx->token_type = 0; return; }
        
        /** Skip single-line comments */
        if (*ctx->src_ptr == '#') {
            while (*ctx->src_ptr != '\n' && *ctx->src_ptr != 0) ctx->src_ptr++;
            continue;
        }
        break;
    }
    {
        char c = *ctx->src_ptr;
        /** Parse identifiers (variables, function/macro names) */
        if (isalpha(c) || c == '_') {
            while (isalnum((unsigned char)*ctx->src_ptr) || *ctx->src_ptr == '_' || *ctx->src_ptr == '.') {
                if (i < 127) ctx->token[i++] = *ctx->src_ptr++; else ctx->src_ptr++;
            }
            ctx->token[i] = 0; 
            ctx->token_type = 1; /** Identifier */
        } 
        /** Parse numbers (integers and floats) */
        else if (isdigit(c) || (c == '-' && isdigit(ctx->src_ptr[1]))) {
            ctx->token[i++] = *ctx->src_ptr++;
            while (isdigit((unsigned char)*ctx->src_ptr) || *ctx->src_ptr == '.' || *ctx->src_ptr == 'x') {
                if (i < 127) ctx->token[i++] = *ctx->src_ptr++; else ctx->src_ptr++;
            }
            ctx->token[i] = 0; 
            /** Determine if it's an INT (2) or FLOAT (4) */
            ctx->token_type = (strchr(ctx->token, '.') ? 4 : 2);
        } 
        /** Parse string literals (used by rhythm function) */
        else if (c == '"') {
            ctx->src_ptr++;
            while (*ctx->src_ptr != '"' && *ctx->src_ptr) {
                 if (i < 127) ctx->token[i++] = *ctx->src_ptr++; else ctx->src_ptr++;
            }
            if (*ctx->src_ptr == '"') ctx->src_ptr++;
            ctx->token[i] = 0; 
            ctx->token_type = 5; /** String */
        } 
        /** Parse operators and single-character tokens */
        else {
            ctx->token[0] = *ctx->src_ptr++;
            {
                char n = *ctx->src_ptr;
                /** Check for two-character operators (==, !=, >=, <=, &&, ||) */
                if ((c=='='||c=='!'||c=='<'||c=='>') && n=='=') { ctx->token[1]=n; ctx->src_ptr++; ctx->token[2]=0; }
                else if ((c=='&'&&n=='&')||(c=='|'&&n=='|')) { ctx->token[1]=n; ctx->src_ptr++; ctx->token[2]=0; }
                else ctx->token[1]=0;
                ctx->token_type = 3; /** Operator/Symbol */
            }
        }
    }
}

/**
 * @brief Expects a specific token, advancing the lexer if matched, or throwing an error.
 *
 * @param ctx The parser context.
 * @param str The expected token string.
 */
static void match(cr_context *ctx, const char *str) {
    if (strcmp(ctx->token, str) != 0) {
        char err[160]; snprintf(err, 160, "Expected '%s' found '%s'", str, ctx->token);
        cr_error(ctx, err);
    }
    get_tok(ctx);
}

/** Forward declarations for the recursive descent parser functions */
static int expr(cr_context *ctx);
static void statement(cr_context *ctx);

/**
 * @brief Parses the highest precedence expression components (constants, variables, function calls, parentheses).
 *
 * This function also handles special syntax like the `rhythm` function.
 *
 * @param ctx The parser context.
 * @return The index of the resulting cr_node.
 */
static int factor(cr_context *ctx) {
    int idx = -1;
    const cr_op_desc *op_desc = NULL;
    
    /** Handle Integer Constant */
    if (ctx->token_type == 2) { 
        op_desc = cr_lookup_op_by_name("const");
        idx = alloc_node(ctx, op_desc);
        ctx->node_pool[idx].value = make_int(atoi(ctx->token));
        ctx->node_pool[idx].is_constant = 1;
        get_tok(ctx);
    } 
    /** Handle Float Constant */
    else if (ctx->token_type == 4) { 
        op_desc = cr_lookup_op_by_name("const");
        idx = alloc_node(ctx, op_desc);
        ctx->node_pool[idx].value = make_float(atof(ctx->token));
        ctx->node_pool[idx].is_constant = 1;
        get_tok(ctx);
    } 
    /** Handle Identifiers (Variables, Functions, Macros, Built-ins) */
    else if (ctx->token_type == 1) { 
        char name[128]; 
        snprintf(name, 128, "%s", ctx->token);
        get_tok(ctx);
        
        /** Check for function/macro call: ID + "(" */
        if (strcmp(ctx->token, "(") == 0) {
            
            /** Special case: rhythm() function, which embeds string data as a pattern buffer */
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
                        /** Map 'x'/'X' to 1.0 (trigger) and anything else to 0.0 (off) */
                        for(i=0; i<len; i++) buf[i] = (ctx->token[i] == 'x' || ctx->token[i] == 'X') ? 1.0 : 0.0;
                    }
                    ctx->node_pool[idx].dsp.buffer = buf;
                    ctx->node_pool[idx].dsp.buf_len = len;
                    ctx->node_pool[idx].is_constant = 1;
                }
                get_tok(ctx); 
                match(ctx, ")");
                
                /** If inside a conditional block, link the rhythm node to the control node */
                if (ctx->active_block_ptr > 0) {
                    int ctrl = ctx->block_dependency_stack[ctx->active_block_ptr-1];
                    if (ctrl != -1) ctx->node_pool[idx].inputs[ctx->node_pool[idx].input_count++] = ctrl;
                }
                return idx;
            }
            
            /** Look up built-in function/operator */
            op_desc = cr_lookup_op_by_name(name);
            if (op_desc) {
                get_tok(ctx); 
                idx = alloc_node(ctx, op_desc);
                
                /** Handle special setup for stateful nodes (Delay/Reverb: buffer allocation) */
                if (op_desc->opcode == OP_DELAY) {
                    int len = (int)(ctx->sample_rate * 4.0); /** Default to 4 seconds delay buffer */
                    int p2 = 1; while(p2 < len) p2 <<= 1; len = p2; /** Round up to next power of 2 */
                    ctx->node_pool[idx].dsp.buffer = (double*)cr_arena_alloc(ctx, len * sizeof(double));
                    ctx->node_pool[idx].dsp.buf_len = len;
                    if(ctx->node_pool[idx].dsp.buffer) memset(ctx->node_pool[idx].dsp.buffer, 0, len * sizeof(double));
                } else if (op_desc->opcode == OP_REVERB) {
                    int len = 65536; /** Fixed size for reverb FDN buffer */
                    ctx->node_pool[idx].dsp.buffer = (double*)cr_arena_alloc(ctx, len * sizeof(double));
                    ctx->node_pool[idx].dsp.buf_len = len;
                    if(ctx->node_pool[idx].dsp.buffer) memset(ctx->node_pool[idx].dsp.buffer, 0, len * sizeof(double));
                }
                
                /** Parse arguments by recursively calling expr() */
                {
                    int arg_c = 0;
                    if (strcmp(ctx->token, ")") != 0) {
                        while(1) {
                            int arg = expr(ctx);
                            if (arg_c < CR_MAX_ARGS) ctx->node_pool[idx].inputs[arg_c++] = arg;
                            if (strcmp(ctx->token, ",") == 0) get_tok(ctx); else break;
                        }
                    }
                    /** Add control dependency as the last input argument if inside a block */
                    if (ctx->active_block_ptr > 0) {
                         int ctrl = ctx->block_dependency_stack[ctx->active_block_ptr-1];
                         if (ctrl != -1 && arg_c < CR_MAX_ARGS) ctx->node_pool[idx].inputs[arg_c++] = ctrl;
                    }
                    ctx->node_pool[idx].input_count = arg_c;
                }
                match(ctx, ")");
                return idx;
            } 
            /** Look up user-defined Macro */
            else {
                int m_idx = -1;
                int m;
                for(m=0; m<ctx->macro_count; m++) {
                    if(!strcmp(ctx->macros[m].name, name)) { m_idx = m; break; }
                }
                
                /** Macro Execution */
                if (m_idx != -1) {
                    int args[CR_MAX_ARGS]; 
                    int argc = 0;
                    
                    /** Parse macro call arguments */
                    if (strcmp(ctx->token, ")") != 0) {
                        while(1) {
                            if (argc < CR_MAX_ARGS) args[argc++] = expr(ctx);
                            if (strcmp(ctx->token, ",") == 0) get_tok(ctx); else break;
                        }
                    }
                    match(ctx, ")");
                    {
                        /** Save current parser state for restoration after macro execution */
                        char old_scope[128]; strcpy(old_scope, ctx->scope);
                        const char* old_src = ctx->src_ptr;
                        char old_tok[128]; strcpy(old_tok, ctx->token);
                        int old_type = ctx->token_type;
                        int old_ret = ctx->returning;
                        int i;
                        
                        /** Set up new scope for macro instance */
                        snprintf(ctx->scope, 128, "%s%s%d_", old_scope, name, ctx->scope_id_ctr++);
                        
                        /** Bind macro parameters to arguments (create local variables pointing to argument nodes) */
                        for(i=0; i<argc && i<ctx->macros[m_idx].arg_count; i++) {
                             char scoped_name[64];
                             snprintf(scoped_name, 64, "%s%s", ctx->scope, ctx->macros[m_idx].args[i]);
                             if (ctx->var_count < CR_MAX_SYMBOLS) {
                                 strcpy(ctx->variables[ctx->var_count].name, scoped_name);
                                 ctx->variables[ctx->var_count].node_index = args[i];
                                 ctx->var_count++;
                             }
                        }
                        
                        /** Start parsing the macro body */
                        ctx->src_ptr = ctx->macros[m_idx].body;
                        ctx->returning = 0;
                        get_tok(ctx);
                        while(ctx->token_type != 0 && !ctx->returning) statement(ctx);
                        idx = ctx->return_reg; /** Capture the result node index */
                        
                        /** Restore previous parser state */
                        strcpy(ctx->scope, old_scope);
                        ctx->src_ptr = old_src;
                        strcpy(ctx->token, old_tok);
                        ctx->token_type = old_type;
                        ctx->returning = old_ret;
                    }
                } else {
                    char err[160]; 
                    snprintf(err, 160, "Unknown func '%s'", name); 
                    cr_error(ctx, err);
                }
            }
        } 
        /** Handle Variable Access or Built-in time constants (ID only) */
        else {
            char scoped[128]; 
            snprintf(scoped, 128, "%s%s", ctx->scope, name);
            int found = -1;
            int i;
            
            /** Search for scoped variable */
            for(i=0; i<ctx->var_count; i++) { 
                if (!strcmp(ctx->variables[i].name, scoped)) { 
                    found = ctx->variables[i].node_index; 
                    break; 
                } 
            }
            
            /** If not found, search for unscoped global/parent variable (only if currently inside a scope) */
            if (found == -1 && strlen(ctx->scope) > 0) {
                for(i=0; i<ctx->var_count; i++) { 
                    if (!strcmp(ctx->variables[i].name, name)) { 
                        found = ctx->variables[i].node_index; 
                        break; 
                    } 
                }
            }
            
            /** If still not found, check for built-in read-only nodes */
            if (found == -1) {
                if (!strcmp(name, "time")) {
                    op_desc = cr_lookup_op_by_name("time");
                    idx = alloc_node(ctx, op_desc);
                } else if (!strcmp(name, "bars")) {
                    op_desc = cr_lookup_op_by_name("bars");
                    idx = alloc_node(ctx, op_desc);
                } else { 
                    char err[160]; snprintf(err, 160, "Unknown var '%s'", name); 
                    cr_error(ctx, err); 
                }
            } else idx = found;
        }
    } 
    /** Handle Parenthesized Expression */
    else if (!strcmp(ctx->token, "(")) {
        get_tok(ctx); 
        idx = expr(ctx); 
        match(ctx, ")");
    } 
    /** Syntax error */
    else {
        cr_error(ctx, "Syntax Error: Unexpected token");
    }
    return idx;
}

/**
 * @brief Parses multiplication, division, and modulo terms.
 *
 * Implements left-to-right precedence over sum.
 *
 * @param ctx The parser context.
 * @return The index of the resulting cr_node.
 */
static int term(cr_context *ctx) { 
    int left = factor(ctx);
    while (!strcmp(ctx->token, "*") || !strcmp(ctx->token, "/") || !strcmp(ctx->token, "%")) {
        const cr_op_desc *op_desc = NULL;
        if (!strcmp(ctx->token, "*")) op_desc = cr_lookup_op_by_name("mul");
        else if (!strcmp(ctx->token, "/")) op_desc = cr_lookup_op_by_name("div");
        else if (!strcmp(ctx->token, "%")) op_desc = cr_lookup_op_by_name("mod");
        
        get_tok(ctx); 
        int right = factor(ctx);
        
        if (!op_desc) cr_error(ctx, "Internal Error: Unknown operator");
        {
            int n = alloc_node(ctx, op_desc);
            ctx->node_pool[n].inputs[0] = left; 
            ctx->node_pool[n].inputs[1] = right;
            ctx->node_pool[n].input_count = 2; 
            left = n;
        }
    }
    return left;
}

/**
 * @brief Parses addition and subtraction terms.
 *
 * Implements left-to-right precedence over relations.
 *
 * @param ctx The parser context.
 * @return The index of the resulting cr_node.
 */
static int sum(cr_context *ctx) {
    int left = term(ctx);
    while (!strcmp(ctx->token, "+") || !strcmp(ctx->token, "-")) {
        const cr_op_desc *op_desc = NULL;
        if (!strcmp(ctx->token, "+")) op_desc = cr_lookup_op_by_name("add");
        else if (!strcmp(ctx->token, "-")) op_desc = cr_lookup_op_by_name("sub");
        
        get_tok(ctx); 
        int right = term(ctx);
        
        if (!op_desc) cr_error(ctx, "Internal Error: Unknown operator");
        {
            int n = alloc_node(ctx, op_desc);
            ctx->node_pool[n].inputs[0] = left; 
            ctx->node_pool[n].inputs[1] = right;
            ctx->node_pool[n].input_count = 2; 
            left = n;
        }
    }
    return left;
}

/**
 * @brief Parses relational comparisons (<, >, <=, >=).
 * @param ctx The parser context.
 * @return The index of the resulting cr_node (a logical 0 or 1).
 */
static int relation(cr_context *ctx) {
    int left = sum(ctx);
    if (!strcmp(ctx->token, "<") || !strcmp(ctx->token, ">") || !strcmp(ctx->token, ">=") || !strcmp(ctx->token, "<=")) {
        const cr_op_desc *op_desc = NULL; 
        if (!strcmp(ctx->token, "<")) op_desc = cr_lookup_op_by_name("lt");
        else if (!strcmp(ctx->token, ">")) op_desc = cr_lookup_op_by_name("gt");
        else if (!strcmp(ctx->token, ">=")) op_desc = cr_lookup_op_by_name("ge");
        else if (!strcmp(ctx->token, "<=")) op_desc = cr_lookup_op_by_name("le");
        
        get_tok(ctx); 
        int right = sum(ctx);
        
        if (!op_desc) cr_error(ctx, "Internal Error: Unknown operator");
        {
            int n = alloc_node(ctx, op_desc);
            ctx->node_pool[n].inputs[0] = left; 
            ctx->node_pool[n].inputs[1] = right;
            ctx->node_pool[n].input_count = 2; 
            left = n;
        }
    }
    return left;
}

/**
 * @brief Parses equality and inequality comparisons (==, !=).
 * @param ctx The parser context.
 * @return The index of the resulting cr_node (a logical 0 or 1).
 */
static int equality(cr_context *ctx) {
    int left = relation(ctx);
    if (!strcmp(ctx->token, "==") || !strcmp(ctx->token, "!=")) {
        const cr_op_desc *op_desc = NULL;
        if (!strcmp(ctx->token, "==")) op_desc = cr_lookup_op_by_name("eq");
        else if (!strcmp(ctx->token, "!=")) op_desc = cr_lookup_op_by_name("ne");
        
        get_tok(ctx); 
        int right = relation(ctx);
        
        if (!op_desc) cr_error(ctx, "Internal Error: Unknown operator");
        {
            int n = alloc_node(ctx, op_desc);
            ctx->node_pool[n].inputs[0] = left; 
            ctx->node_pool[n].inputs[1] = right;
            ctx->node_pool[n].input_count = 2; 
            left = n;
        }
    }
    return left;
}

/**
 * @brief Parses logical AND (&&).
 * @param ctx The parser context.
 * @return The index of the resulting cr_node (a logical 0 or 1).
 */
static int logic_and(cr_context *ctx) {
    int left = equality(ctx);
    while (!strcmp(ctx->token, "&&")) {
        const cr_op_desc *op_desc = cr_lookup_op_by_name("and");
        
        get_tok(ctx); 
        int right = equality(ctx);
        
        if (!op_desc) cr_error(ctx, "Internal Error: Unknown operator");
        {
            int n = alloc_node(ctx, op_desc);
            ctx->node_pool[n].inputs[0] = left; 
            ctx->node_pool[n].inputs[1] = right;
            ctx->node_pool[n].input_count = 2; 
            left = n;
        }
    }
    return left;
}

/**
 * @brief Parses logical OR (||).
 * @param ctx The parser context.
 * @return The index of the resulting cr_node (a logical 0 or 1).
 */
static int logic_or(cr_context *ctx) {
    int left = logic_and(ctx);
    while (!strcmp(ctx->token, "||")) {
        const cr_op_desc *op_desc = cr_lookup_op_by_name("or");
        
        get_tok(ctx); 
        int right = logic_and(ctx);
        
        if (!op_desc) cr_error(ctx, "Internal Error: Unknown operator");
        {
            int n = alloc_node(ctx, op_desc);
            ctx->node_pool[n].inputs[0] = left; 
            ctx->node_pool[n].inputs[1] = right;
            ctx->node_pool[n].input_count = 2; 
            left = n;
        }
    }
    return left;
}

/**
 * @brief The top-level function for parsing expressions (lowest precedence).
 * @param ctx The parser context.
 * @return The index of the resulting cr_node.
 */
static int expr(cr_context *ctx) { return logic_or(ctx); }

/**
 * @brief Parses a single top-level statement (macro definition, return, if, or assignment).
 * @param ctx The parser context.
 */
static void statement(cr_context *ctx) {
    /** Macro Definition (def name(...) {...}) */
    if (strcmp(ctx->token, "def") == 0) {
        if (ctx->macro_count >= CR_MAX_MACROS) cr_error(ctx, "Max macros");
        {
            cr_macro *m = &ctx->macros[ctx->macro_count++];
            const char *scan;
            long len;
            int depth;
            int i;
            
            get_tok(ctx); strcpy(m->name, ctx->token); /** Capture macro name */
            get_tok(ctx); match(ctx, "(");
            
            /** Parse argument list */
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
            
            /** Capture macro body content (raw script text) */
            {
                const char *body_start = ctx->src_ptr;
                depth = 1;
                scan = body_start;
                /** Scan until the closing brace '}' is found, respecting nested braces and comments */
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
                ctx->src_ptr = scan + 1; /** Advance source pointer past the closing brace */
                get_tok(ctx);
            }
        }
        return;
    }

    /** Return Statement (return expr) */
    if (strcmp(ctx->token, "return") == 0) {
        get_tok(ctx);
        ctx->return_reg = expr(ctx); /** Capture the node ID of the expression result */
        ctx->returning = 1;          /** Set flag to stop macro execution */
        return;
    }

    /** Conditional Statement (if (expr) {...} else {...}) */
    if (strcmp(ctx->token, "if") == 0) {
        const cr_op_desc *ctrl_desc = cr_lookup_op_by_name("branch_ctrl");
        const cr_op_desc *not_desc = cr_lookup_op_by_name("not");
        const cr_op_desc *select_desc = cr_lookup_op_by_name("select");
        const cr_op_desc *const_desc = cr_lookup_op_by_name("const");
        
        get_tok(ctx); match(ctx, "(");
        {
            int cond = expr(ctx); /** Node ID of the condition expression */
            match(ctx, ")"); match(ctx, "{");
            
            /** Snapshot variable state BEFORE the 'if' block */
            int snap_nodes[CR_MAX_SYMBOLS];
            int snap_count = ctx->var_count;
            int i;
            for(i=0; i<snap_count; i++) snap_nodes[i] = ctx->variables[i].node_index;

            /** Parse the 'if' (True) block */
            {
                int blk = ++ctx->block_id_counter;
                int ctrl = alloc_node(ctx, ctrl_desc); /** Allocate the branch control node */
                ctx->node_pool[ctrl].inputs[0] = cond;
                {
                    /** Add the block ID as a constant input to the control node */
                    int bconst = alloc_node(ctx, const_desc);
                    ctx->node_pool[bconst].value = make_float((double)blk);
                    ctx->node_pool[bconst].is_constant = 1;
                    ctx->node_pool[ctrl].inputs[1] = bconst; 
                    ctx->node_pool[ctrl].input_count = 2;
                }
                
                /** Push block context onto stacks */
                ctx->active_block_stack[ctx->active_block_ptr] = blk;
                ctx->block_dependency_stack[ctx->active_block_ptr] = ctrl;
                ctx->active_block_ptr++;
                
                /** Process statements inside the 'if' block */
                while (strcmp(ctx->token, "}") != 0 && ctx->token_type != 0 && !ctx->returning) statement(ctx);
                match(ctx, "}");
                ctx->active_block_ptr--;
            }
            
            /** Prepare for merging: Store the resulting nodes from the 'if' branch */
            {
                int if_nodes[CR_MAX_SYMBOLS]; 
                for(i=0; i<snap_count; i++) {
                    /** Store the node index if it changed inside the 'if' block, or -1 if unchanged */
                    if_nodes[i] = (ctx->variables[i].node_index != snap_nodes[i]) ? ctx->variables[i].node_index : -1;
                    /** Restore variable indices to the value before the 'if' block */
                    ctx->variables[i].node_index = snap_nodes[i];
                }

                /** Parse the 'else' (False) block */
                if (strcmp(ctx->token, "else") == 0) {
                    get_tok(ctx); match(ctx, "{");
                    {
                        int blk2 = ++ctx->block_id_counter;
                        int not_n = alloc_node(ctx, not_desc); /** Condition for 'else' is NOT(cond) */
                        ctx->node_pool[not_n].inputs[0] = cond; 
                        ctx->node_pool[not_n].input_count = 1;
                        
                        int ctrl2 = alloc_node(ctx, ctrl_desc); /** Allocate the 'else' branch control node */
                        ctx->node_pool[ctrl2].inputs[0] = not_n;
                        {
                            /** Add the block ID for the 'else' block */
                            int bconst2 = alloc_node(ctx, const_desc);
                            ctx->node_pool[bconst2].value = make_float((double)blk2);
                            ctx->node_pool[bconst2].is_constant = 1;
                            ctx->node_pool[ctrl2].inputs[1] = bconst2; 
                            ctx->node_pool[ctrl2].input_count = 2;
                        }
                        
                        /** Push 'else' block context */
                        ctx->active_block_stack[ctx->active_block_ptr] = blk2;
                        ctx->block_dependency_stack[ctx->active_block_ptr] = ctrl2;
                        ctx->active_block_ptr++;
                        
                        /** Process statements inside the 'else' block */
                        while (strcmp(ctx->token, "}") != 0 && ctx->token_type != 0 && !ctx->returning) statement(ctx);
                        match(ctx, "}");
                        ctx->active_block_ptr--;
                    }
                }
                
                /** Merge Variables using OP_SELECT nodes (Phi function equivalent) */
                for(i=0; i<snap_count; i++) {
                    int node_if = if_nodes[i];
                    /** node_else is the current value of the variable (if 'else' block ran) or snap_nodes[i] (if no 'else') */
                    int node_else = ctx->variables[i].node_index; 
                    int node_orig = snap_nodes[i];
                    
                    /** Check if the variable was modified in EITHER the 'if' or the 'else' block */
                    if (node_if != -1 || node_else != node_orig) {
                        int true_node = (node_if != -1) ? node_if : node_orig;
                        int false_node = (node_else != node_orig) ? node_else : node_orig;
                        
                        /** Create a SELECT node: SELECT(Condition, TrueNode, FalseNode) */
                        int sel = alloc_node(ctx, select_desc);
                        ctx->node_pool[sel].inputs[0] = cond;
                        ctx->node_pool[sel].inputs[1] = true_node;
                        ctx->node_pool[sel].inputs[2] = false_node;
                        ctx->node_pool[sel].input_count = 3;
                        
                        /** The variable now points to the result of the SELECT node */
                        ctx->variables[i].node_index = sel;
                    }
                }
            }
        }
        return;
    }

    /** Variable Assignment (var = expr) */
    if (ctx->token_type == 1) {
        char name[128]; 
        snprintf(name, 128, "%s", ctx->token);
        get_tok(ctx); 
        match(ctx, "=");
        {
            int val = expr(ctx); /** Node ID of the expression result */
            int found = -1;
            int i;
            
            /** Special assignment: output variable */
            if (!strcmp(name, "out")) {
                ctx->output_nodes[0] = val; 
                ctx->output_nodes[1] = val; /** Assign to both stereo channels */
            } 
            /** Special assignment: BPM variable (tracked separately) */
            else {
                if (!strcmp(name, "bpm")) ctx->bpm_node_idx = val;
                
                /** Standard variable assignment (handles scoping) */
                {
                    char scoped[128]; 
                    snprintf(scoped, 128, "%s%s", ctx->scope, name);
                    
                    /** Check if variable already exists (update existing entry) */
                    for(i=0; i<ctx->var_count; i++) { 
                        if (!strcmp(ctx->variables[i].name, scoped)) { 
                            ctx->variables[i].node_index = val; 
                            found = 1; 
                            break; 
                        } 
                    }
                    
                    /** If not found, create a new variable entry */
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

/**
 * @brief Traverses the dependency graph depth-first to build the execution order.
 *
 * This is a recursive function used by `build_exec_list`.
 *
 * @param ctx The context.
 * @param u The index of the current node being visited.
 */
static void topo_visit(cr_context *ctx, int u) {
    int i;
    
    /** State 1: Visiting (detects cycles, though not explicitly handled as error) */
    if (ctx->visit_state[u] == 1) return;
    /** State 2: Visited and added to execution list */
    if (ctx->visit_state[u] == 2) return;
    
    ctx->visit_state[u] = 1;
    
    {
        cr_node* n = &ctx->node_pool[u];
        
        /** First, visit the control node if one exists (ensures block skip flags are set) */
        if (n->control_node != -1) topo_visit(ctx, n->control_node);
        
        /** Then, visit all input nodes (ensures inputs are computed before 'u') */
        for (i = 0; i < n->input_count; i++) topo_visit(ctx, n->inputs[i]);
        
        ctx->visit_state[u] = 2;
        /** Add the current node to the end of the execution list */
        ctx->exec_order[ctx->exec_count++] = u;
    }
}

/**
 * @brief Initiates the topological sort starting from the output nodes.
 *
 * Clears visit state and recursively calls `topo_visit` for each audio output.
 * The resulting `ctx->exec_order` array dictates runtime VM execution flow.
 *
 * @param ctx The context containing the final graph.
 */
static void build_exec_list(cr_context *ctx) {
    int c;
    memset(ctx->visit_state, 0, sizeof(ctx->visit_state));
    ctx->exec_count = 0;
    
    /** Start traversal from the output nodes (channels 0 to CR_MAX_CHANNELS-1) */
    for(c=0; c<CR_MAX_CHANNELS; c++) { 
        if (ctx->output_nodes[c] != -1) topo_visit(ctx, ctx->output_nodes[c]); 
    }
}

/**
 * @brief Parses and compiles the script, manages hot-swapping contexts.
 *
 * @param engine The engine instance.
 * @param script The null-terminated script string.
 * @param reset If true, clears source history first.
 * @return 1 on success, 0 on failure.
 */
CR_API int cr_eval(cr_engine *engine, const char *script, int reset) {
    size_t new_len = strlen(script);
    
    /** Reset history if requested */
    if (reset) {
        engine->source_len = 0;
        engine->source_history[0] = 0;
    }

    /** Append new script to the source history buffer (handles resizing) */
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
        
        /** Reset back context to a clean state for compilation */
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
        
        /** Initialize parser state */
        back->global_time = engine->active->global_time; /** Preserve current time */
        back->src_ptr = engine->source_history;
        back->current_line = 1; 
        back->scope[0] = 0; back->scope_id_ctr = 0;
        
        /** Setup error handling jump point */
        if (setjmp(back->err_jmp) != 0) { 
            cr_log(engine, CR_LOG_ERROR, "Compile Error: %s", back->error_msg); 
            /** Roll back source history if compilation failed on an update (not a full reset) */
            if (!reset) {
                 engine->source_len -= (new_script_len + 1);
                 engine->source_history[engine->source_len] = 0;
            }
            return 0; 
        }

        /** Start parsing and code generation */
        get_tok(back);
        while (back->token_type != 0) statement(back);
        
        /** Build execution order */
        build_exec_list(back);
        
        /** Hot-Swap procedure: Acquire lock, migrate DSP state, swap pointers */
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

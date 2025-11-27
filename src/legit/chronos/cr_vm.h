#ifndef CR_VM_H
#define CR_VM_H
#include "cr_utils.h"
#include "cr_op_registry.h"
#include "cr_dsp.h"

CR_API cr_engine *cr_create_engine(int sample_rate);
CR_API void cr_destroy_engine(cr_engine *engine);
CR_API void cr_set_log_level(cr_engine *engine, int level);
CR_API double cr_process(cr_engine *engine, int channel);
CR_API void cr_tick(cr_engine *engine);

#ifdef CR_VM_IMPLEMENTATION
static void migrate_dsp_state(cr_context *dest, cr_context *src) {
    int i;
    for (i = 0; i < dest->var_count; i++) {
        char *name = dest->variables[i].name;
        int j;
        for (j = 0; j < src->var_count; j++) {
            if (strcmp(src->variables[j].name, name) == 0) {
                int old_id = src->variables[j].node_index;
                int new_id = dest->variables[i].node_index;
                
                double* new_ptr = dest->node_pool[new_id].dsp.buffer;
                dest->node_pool[new_id].dsp = src->node_pool[old_id].dsp;
                dest->node_pool[new_id].dsp.buffer = new_ptr;
                
                if (new_ptr && src->node_pool[old_id].dsp.buffer) {
                    size_t bytes = dest->node_pool[new_id].dsp.buf_len * sizeof(double);
                    memcpy(new_ptr, src->node_pool[old_id].dsp.buffer, bytes);
                }
                break;
            }
        }
    }
}

// FIX 2: Remove 'static' keyword from all op_handler functions
cr_val op_handler_default(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(0.0);
}

cr_val op_handler_const(cr_context *ctx, cr_node *n, cr_val *v) {
    return n->value;
}

cr_val op_handler_branch_ctrl(cr_context *ctx, cr_node *n, cr_val *v) {
    int cond = as_int(v[0]);
    int blk = (int)as_float(v[1]);
    if (blk > 0 && blk < CR_MAX_BLOCKS) ctx->block_skip_flags[blk] = (cond == 0) ? 1 : 0;
    return v[0];
}

cr_val op_handler_select(cr_context *ctx, cr_node *n, cr_val *v) {
    return (as_int(v[0]) != 0) ? v[1] : v[2];
}

cr_val op_handler_add(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(as_float(v[0]) + as_float(v[1]));
}

cr_val op_handler_sub(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(as_float(v[0]) - as_float(v[1]));
}

cr_val op_handler_mul(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(as_float(v[0]) * as_float(v[1]));
}

cr_val op_handler_div(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float((fabs(as_float(v[1])) < 1e-9) ? 0.0 : as_float(v[0]) / as_float(v[1]));
}

cr_val op_handler_mod(cr_context *ctx, cr_node *n, cr_val *v) {
    double d = as_float(v[1]); 
    return make_float((fabs(d) < 1e-9) ? 0.0 : fmod(as_float(v[0]), d));
}

cr_val op_handler_pow(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(pow(as_float(v[0]), as_float(v[1])));
}

cr_val op_handler_gt(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(as_float(v[0]) > as_float(v[1]));
}

cr_val op_handler_lt(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(as_float(v[0]) < as_float(v[1]));
}

cr_val op_handler_ge(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(as_float(v[0]) >= as_float(v[1]));
}

cr_val op_handler_le(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(as_float(v[0]) <= as_float(v[1]));
}

cr_val op_handler_eq(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(fabs(as_float(v[0]) - as_float(v[1])) < 1e-6);
}

cr_val op_handler_ne(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(fabs(as_float(v[0]) - as_float(v[1])) > 1e-6);
}

cr_val op_handler_and(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(as_int(v[0]) && as_int(v[1]));
}

cr_val op_handler_or(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(as_int(v[0]) || as_int(v[1]));
}

cr_val op_handler_not(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(!as_int(v[0]));
}

cr_val op_handler_bit_and(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(as_int(v[0]) & as_int(v[1]));
}

cr_val op_handler_bit_or(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(as_int(v[0]) | as_int(v[1]));
}

cr_val op_handler_bit_xor(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(as_int(v[0]) ^ as_int(v[1]));
}

cr_val op_handler_bit_not(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(~as_int(v[0]));
}

cr_val op_handler_bit_lshift(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(as_int(v[0]) << as_int(v[1]));
}

cr_val op_handler_bit_rshift(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(as_int(v[0]) >> as_int(v[1]));
}

cr_val op_handler_sine(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(sin(as_float(v[0]) * 2.0 * CR_PI));
}

cr_val op_handler_phasor(cr_context *ctx, cr_node *n, cr_val *v) {
    double dt = as_float(v[0]) / ctx->sample_rate;
    double p = n->dsp.s[0] + dt; p -= floor(p);
    n->dsp.s[0] = p; 
    return make_float(p);
}

cr_val op_handler_noise(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(((double)rand() / (double)RAND_MAX) * 2.0 - 1.0);
}

cr_val op_handler_seq(cr_context *ctx, cr_node *n, cr_val *v) {
    double step = as_float(v[0]);
    int use_pattern = 0;
    cr_val out = make_float(0.0);
    
    if (n->input_count >= 2) {
        cr_node* pat = &ctx->node_pool[n->inputs[1]];
        if (pat->op_desc && pat->op_desc->opcode == OP_PATTERN) {
            if (pat->dsp.buf_len > 0 && step > 0.0001) {
                double t = (double)ctx->global_time / ctx->sample_rate;
                int idx = (int)(fmod(t, step * pat->dsp.buf_len) / step);
                if (idx >= 0 && idx < pat->dsp.buf_len) out = make_float(pat->dsp.buffer[idx]);
            }
            use_pattern = 1;
        }
    }
    if (!use_pattern) {
        int count = n->input_count - 1;
        if (count > 0 && step > 0.0001) {
            double t = (double)ctx->global_time / ctx->sample_rate;
            int seq_idx = (int)(fmod(t, step * count) / step);
            if (seq_idx >= 0 && seq_idx < count) out = ctx->node_pool[n->inputs[seq_idx + 1]].value;
        }
    }
    return out;
}

cr_val op_handler_time(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float((double)ctx->global_time / ctx->sample_rate);
}

cr_val op_handler_bars(cr_context *ctx, cr_node *n, cr_val *v) {
    double bpm = 120.0;
    double sec_per_beat;
    double t;
    if (ctx->bpm_node_idx != -1) bpm = as_float(ctx->node_pool[ctx->bpm_node_idx].value);
    sec_per_beat = 60.0 / bpm;
    t = (double)ctx->global_time / ctx->sample_rate;
    return make_float(t / (sec_per_beat * 4.0));
}

cr_val op_handler_floor(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(floor(as_float(v[0])));
}

cr_val op_handler_ceil(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(ceil(as_float(v[0])));
}

cr_val op_handler_abs(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(fabs(as_float(v[0])));
}

cr_val op_handler_sign(cr_context *ctx, cr_node *n, cr_val *v) {
    double val = as_float(v[0]);
    return make_float((val > 0) ? 1.0 : ((val < 0) ? -1.0 : 0.0));
}

cr_val op_handler_mtof(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(440.0 * pow(2.0, (as_float(v[0]) - 69.0) / 12.0));
}

cr_val op_handler_lerp(cr_context *ctx, cr_node *n, cr_val *v) {
     double a = as_float(v[0]); double b = as_float(v[1]); double t = as_float(v[2]);
     return make_float(a + (b - a) * t);
}

cr_val op_handler_clamp(cr_context *ctx, cr_node *n, cr_val *v) {
     double val = as_float(v[0]); double min = as_float(v[1]); double max = as_float(v[2]);
     if (val < min) val = min; if (val > max) val = max;
     return make_float(val);
}

cr_val op_handler_map(cr_context *ctx, cr_node *n, cr_val *v) {
     double val = as_float(v[0]); double in_min = as_float(v[1]); double in_max = as_float(v[2]);
     double out_min = as_float(v[3]);
     double out_max = as_float(v[4]);
     double t = (val - in_min) / (in_max - in_min);
     return make_float(out_min + t * (out_max - out_min));
}

cr_val op_handler_square(cr_context *ctx, cr_node *n, cr_val *v) {
    double val = as_float(v[0]);
    return make_float(val * val);
}

cr_val op_handler_sqrt(cr_context *ctx, cr_node *n, cr_val *v) {
    double val = as_float(v[0]);
    return make_float(sqrt(val));
}

cr_val op_handler_exp(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(exp(as_float(v[0])));
}

cr_val op_handler_log(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(log(as_float(v[0])));
}

cr_val op_handler_atan2(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(atan2(as_float(v[0]), as_float(v[1])));
}

cr_val op_handler_hpf(cr_context *ctx, cr_node *n, cr_val *v) {
    double in = as_float(v[0]);
    double cutoff = as_float(v[1]);
    double f, out_val;
    
    f = 2.0 * sin(CR_PI * cutoff / ctx->sample_rate);
    n->dsp.s[0] += f * (in - n->dsp.s[0]);
    out_val = in - n->dsp.s[0];
    return make_float(out_val);
}

cr_val op_handler_diff(cr_context *ctx, cr_node *n, cr_val *v) {
    double in = as_float(v[0]);
    double last_in = n->dsp.s[0];
    double diff = in - last_in;
    n->dsp.s[0] = in;
    return make_float(diff);
}

cr_val op_handler_integrate(cr_context *ctx, cr_node *n, cr_val *v) {
    double in = as_float(v[0]);
    double current = n->dsp.s[0];
    double rate = as_float(v[1]);
    current += in * (rate / ctx->sample_rate);
    n->dsp.s[0] = current;
    return make_float(current);
}

static void run_node(cr_context *ctx, int idx) {
    cr_node *n = &ctx->node_pool[idx];
    
    if (n->op_desc->opcode != OP_CONST && n->block_id > 0 && ctx->block_skip_flags[n->block_id]) {
        n->value = make_float(0.0); return;
    }

    if (n->op_desc && n->op_desc->handler) {
        cr_val v[CR_MAX_ARGS];
        int input_max = n->input_count;
        int i;
        cr_val out = make_float(0.0);

        for(i=0; i<input_max; i++) v[i] = ctx->node_pool[n->inputs[i]].value;
        
        out = n->op_desc->handler(ctx, n, v);

        n->value = out;
    }
}

CR_API cr_engine *cr_create_engine(int sample_rate) {
    cr_engine *eng = (cr_engine*)calloc(1, sizeof(cr_engine));
    size_t pool_size = CR_ARENA_SIZE;
    eng->memory_block = calloc(1, pool_size * 2); 
    eng->contexts[0].sample_rate = (double)sample_rate;
    eng->contexts[0].arena_base = (unsigned char*)eng->memory_block;
    eng->contexts[0].arena_size = pool_size;
    eng->contexts[0].output_nodes[0] = -1; eng->contexts[0].output_nodes[1] = -1;
    eng->contexts[1].sample_rate = (double)sample_rate;
    eng->contexts[1].arena_base = (unsigned char*)eng->memory_block + pool_size;
    eng->contexts[1].arena_size = pool_size;
    eng->contexts[1].output_nodes[0] = -1; eng->contexts[1].output_nodes[1] = -1;
    eng->active = &eng->contexts[0]; eng->back = &eng->contexts[1];
    
    eng->source_capacity = 4096;
    eng->source_history = (char*)malloc(eng->source_capacity);
    eng->source_history[0] = 0;
    eng->source_len = 0;

    #if defined(_WIN32)
    InitializeCriticalSection(&eng->swap_lock);
    #else
    pthread_mutex_init(&eng->swap_lock, NULL);
    #endif
    return eng;
}

CR_API void cr_destroy_engine(cr_engine *eng) { 
    free(eng->source_history);
    free(eng->memory_block); 
    free(eng); 
}

CR_API void cr_set_log_level(cr_engine *eng, int level) { eng->log_level = level; }

CR_API void cr_tick(cr_engine *engine) { engine->active->global_time++; }

CR_API double cr_process(cr_engine *engine, int channel) {
    cr_context *ctx = engine->active;
    int i;
    if (channel == 0) { for(i=0; i<ctx->exec_count; i++) run_node(ctx, ctx->exec_order[i]); }
    if (channel < 0 || channel >= CR_MAX_CHANNELS) return 0.0;
    {
        int out_node = ctx->output_nodes[channel];
        if (out_node == -1) return 0.0;
        return as_float(ctx->node_pool[out_node].value);
    }
}

#endif
#endif

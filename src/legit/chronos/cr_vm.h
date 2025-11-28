/**
 * @file cr_vm.h
 * @brief Chronos Virtual Machine core logic, DSP handlers, and engine management API.
 *
 * This file contains the implementation of the main engine functions, the logic
 * for copying DSP state during hot-swapping, and the operation handlers for
 * general math, comparison, logical, time-based, and simple signal generation nodes.
 */
#ifndef CR_VM_H
#define CR_VM_H
#include "cr_utils.h"
#include "cr_op_registry.h"
#include "cr_dsp.h"

/**
 * @brief Creates and initializes the Chronos engine instance.
 *
 * Sets up the double-buffered contexts (active/back), allocates the memory arena,
 * sets the sample rate, and initializes synchronization primitives.
 *
 * @param sample_rate The audio sample rate (e.g., 48000).
 * @return A pointer to the newly created cr_engine structure.
 */
CR_API cr_engine *cr_create_engine(int sample_rate);

/**
 * @brief Destroys the Chronos engine and frees all allocated memory.
 *
 * @param engine The engine instance to destroy.
 */
CR_API void cr_destroy_engine(cr_engine *engine);

/**
 * @brief Sets the logging verbosity level for the engine.
 *
 * @param engine The engine instance.
 * @param level The desired log level (CR_LOG_NONE to CR_LOG_DEBUG).
 */
CR_API void cr_set_log_level(cr_engine *engine, int level);

/**
 * @brief Processes one sample of audio for a given channel.
 *
 * For channel 0, it executes the entire topologically sorted DSP graph.
 * For subsequent channels, it only reads the output node value.
 *
 * @param engine The active engine instance.
 * @param channel The output channel index (0 or 1).
 * @return The resulting audio sample value as a double.
 */
CR_API double cr_process(cr_engine *engine, int channel);

/**
 * @brief Increments the global sample time counter.
 *
 * This must be called exactly once per sample by the audio callback.
 *
 * @param engine The active engine instance.
 */
CR_API void cr_tick(cr_engine *engine);

#ifdef CR_VM_IMPLEMENTATION
/**
 * @brief Migrates DSP state (buffers, internal state) from the old context to the new context during hot-swap.
 *
 * This function iterates over all variables defined in the new context. If a variable
 * name matches one in the old context, the non-pointer DSP state fields and the
 * contents of the dynamically allocated buffer are copied. This maintains continuity
 * for stateful nodes like filters, delays, and phasors.
 *
 * @param dest The new cr_context being compiled to.
 * @param src The old, currently active cr_context.
 */
static void migrate_dsp_state(cr_context *dest, cr_context *src) {
    int i;
    for (i = 0; i < dest->var_count; i++) {
        char *name = dest->variables[i].name;
        int j;
        for (j = 0; j < src->var_count; j++) {
            if (strcmp(src->variables[j].name, name) == 0) {
                int old_id = src->variables[j].node_index;
                int new_id = dest->variables[i].node_index;
                
                /** Save new pointer location */
                double* new_ptr = dest->node_pool[new_id].dsp.buffer;
                
                /** Copy DSP state struct, overwriting the new pointer location */
                dest->node_pool[new_id].dsp = src->node_pool[old_id].dsp;
                
                /** Restore the new pointer location */
                dest->node_pool[new_id].dsp.buffer = new_ptr;
                
                /** Copy the contents of the buffer if both exist */
                if (new_ptr && src->node_pool[old_id].dsp.buffer) {
                    size_t bytes = dest->node_pool[new_id].dsp.buf_len * sizeof(double);
                    memcpy(new_ptr, src->node_pool[old_id].dsp.buffer, bytes);
                }
                break;
            }
        }
    }
}

/**
 * @brief Default operation handler (used for OP_HALT, OP_LOAD, OP_STORE, OP_PATTERN placeholders).
 * @param ctx The current context.
 * @param n The node being executed.
 * @param v Array of input values.
 * @return A floating-point value of 0.0.
 */
cr_val op_handler_default(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(0.0);
}

/**
 * @brief Handler for constant nodes. Returns the node's stored value.
 * @param ctx The current context.
 * @param n The node being executed.
 * @param v Array of input values (ignored).
 * @return The constant value stored in the node.
 */
cr_val op_handler_const(cr_context *ctx, cr_node *n, cr_val *v) {
    return n->value;
}

/**
 * @brief Internal handler for conditional branch control.
 *
 * Sets the block skip flag in the context based on the condition input,
 * enabling or disabling nodes in the associated block.
 *
 * @param ctx The current context.
 * @param n The node being executed.
 * @param v Array of input values: v[0] = condition, v[1] = block ID (float-encoded int).
 * @return The condition value.
 */
cr_val op_handler_branch_ctrl(cr_context *ctx, cr_node *n, cr_val *v) {
    int cond = as_int(v[0]);
    int blk = (int)as_float(v[1]);
    if (blk > 0 && blk < CR_MAX_BLOCKS) ctx->block_skip_flags[blk] = (cond == 0) ? 1 : 0;
    return v[0];
}

/**
 * @brief Handler for selecting one of two inputs based on a condition (used for merging 'if/else' branches).
 * @param ctx The current context.
 * @param n The node being executed.
 * @param v Array of input values: v[0] = condition (int), v[1] = true value, v[2] = false value.
 * @return v[1] if condition is true (non-zero), otherwise v[2].
 */
cr_val op_handler_select(cr_context *ctx, cr_node *n, cr_val *v) {
    return (as_int(v[0]) != 0) ? v[1] : v[2];
}

/** @brief Handles addition (v[0] + v[1]). */
cr_val op_handler_add(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(as_float(v[0]) + as_float(v[1]));
}

/** @brief Handles subtraction (v[0] - v[1]). */
cr_val op_handler_sub(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(as_float(v[0]) - as_float(v[1]));
}

/** @brief Handles multiplication (v[0] * v[1]). */
cr_val op_handler_mul(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(as_float(v[0]) * as_float(v[1]));
}

/** @brief Handles division (v[0] / v[1]), guarding against division by zero. */
cr_val op_handler_div(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float((fabs(as_float(v[1])) < 1e-9) ? 0.0 : as_float(v[0]) / as_float(v[1]));
}

/** @brief Handles floating-point modulo (fmod(v[0], v[1])), guarding against division by zero. */
cr_val op_handler_mod(cr_context *ctx, cr_node *n, cr_val *v) {
    double d = as_float(v[1]); 
    return make_float((fabs(d) < 1e-9) ? 0.0 : fmod(as_float(v[0]), d));
}

/** @brief Handles exponentiation (pow(v[0], v[1])). */
cr_val op_handler_pow(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(pow(as_float(v[0]), as_float(v[1])));
}

/** @brief Handles Greater Than comparison (v[0] > v[1]), returns 1 or 0 (int). */
cr_val op_handler_gt(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(as_float(v[0]) > as_float(v[1]));
}

/** @brief Handles Less Than comparison (v[0] < v[1]), returns 1 or 0 (int). */
cr_val op_handler_lt(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(as_float(v[0]) < as_float(v[1]));
}

/** @brief Handles Greater than or Equal To comparison (v[0] >= v[1]), returns 1 or 0 (int). */
cr_val op_handler_ge(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(as_float(v[0]) >= as_float(v[1]));
}

/** @brief Handles Less than or Equal To comparison (v[0] <= v[1]), returns 1 or 0 (int). */
cr_val op_handler_le(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(as_float(v[0]) <= as_float(v[1]));
}

/** @brief Handles floating-point Equality (v[0] == v[1]), using a small epsilon (1e-6). */
cr_val op_handler_eq(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(fabs(as_float(v[0]) - as_float(v[1])) < 1e-6);
}

/** @brief Handles floating-point Not Equal (v[0] != v[1]), using a small epsilon (1e-6). */
cr_val op_handler_ne(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(fabs(as_float(v[0]) - as_float(v[1])) > 1e-6);
}

/** @brief Handles logical AND (v[0] && v[1]). */
cr_val op_handler_and(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(as_int(v[0]) && as_int(v[1]));
}

/** @brief Handles logical OR (v[0] || v[1]). */
cr_val op_handler_or(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(as_int(v[0]) || as_int(v[1]));
}

/** @brief Handles logical NOT (!v[0]). */
cr_val op_handler_not(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(!as_int(v[0]));
}

/** @brief Handles bitwise AND (v[0] & v[1]). */
cr_val op_handler_bit_and(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(as_int(v[0]) & as_int(v[1]));
}

/** @brief Handles bitwise OR (v[0] | v[1]). */
cr_val op_handler_bit_or(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(as_int(v[0]) | as_int(v[1]));
}

/** @brief Handles bitwise XOR (v[0] ^ v[1]). */
cr_val op_handler_bit_xor(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(as_int(v[0]) ^ as_int(v[1]));
}

/** @brief Handles bitwise NOT (~v[0]). */
cr_val op_handler_bit_not(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(~as_int(v[0]));
}

/** @brief Handles bitwise Left Shift (v[0] << v[1]). */
cr_val op_handler_bit_lshift(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(as_int(v[0]) << as_int(v[1]));
}

/** @brief Handles bitwise Right Shift (v[0] >> v[1]). */
cr_val op_handler_bit_rshift(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_int(as_int(v[0]) >> as_int(v[1]));
}

/**
 * @brief Generates a sine wave output from a phase input (v[0]).
 * @param ctx The current context.
 * @param n The node being executed.
 * @param v Array of input values: v[0] = normalized phase (0 to 1).
 * @return sin(phase * 2 * PI).
 */
cr_val op_handler_sine(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(sin(as_float(v[0]) * 2.0 * CR_PI));
}

/**
 * @brief Generates a continuous, ramp-up phase signal (0 to 1) from a frequency input.
 *
 * The internal state `n->dsp.s[0]` holds the current phase.
 *
 * @param ctx The current context.
 * @param n The node being executed (holds phase state).
 * @param v Array of input values: v[0] = frequency (Hz).
 * @return The current phase (0.0 to < 1.0).
 */
cr_val op_handler_phasor(cr_context *ctx, cr_node *n, cr_val *v) {
    double dt = as_float(v[0]) / ctx->sample_rate;
    double p = n->dsp.s[0] + dt; 
    p -= floor(p);
    n->dsp.s[0] = p; 
    return make_float(p);
}

/**
 * @brief Generates white noise.
 * @return A random floating-point value between -1.0 and 1.0.
 */
cr_val op_handler_noise(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(((double)rand() / (double)RAND_MAX) * 2.0 - 1.0);
}

/**
 * @brief Implements a step-sequencer or rhythmic pattern reader.
 *
 * Reads values based on the global time, the step length (v[0]), and either
 * subsequent non-pattern inputs or the values inside a dedicated rhythm/pattern node.
 *
 * @param ctx The current context.
 * @param n The node being executed.
 * @param v Array of input values: v[0] = step time (seconds), v[1..N] = sequence values/pattern node.
 * @return The value of the current step.
 */
cr_val op_handler_seq(cr_context *ctx, cr_node *n, cr_val *v) {
    double step = as_float(v[0]);
    int use_pattern = 0;
    cr_val out = make_float(0.0);
    
    if (n->input_count >= 2) {
        cr_node* pat = &ctx->node_pool[n->inputs[1]];
        if (pat->op_desc && pat->op_desc->opcode == OP_PATTERN) {
            /** Use the rhythm pattern buffer if input 1 is an OP_PATTERN node */
            if (pat->dsp.buf_len > 0 && step > 0.0001) {
                double t = (double)ctx->global_time / ctx->sample_rate;
                int idx = (int)(fmod(t, step * pat->dsp.buf_len) / step);
                if (idx >= 0 && idx < pat->dsp.buf_len) out = make_float(pat->dsp.buffer[idx]);
            }
            use_pattern = 1;
        }
    }
    if (!use_pattern) {
        /** Use subsequent node inputs as sequence steps */
        int count = n->input_count - 1;
        if (count > 0 && step > 0.0001) {
            double t = (double)ctx->global_time / ctx->sample_rate;
            int seq_idx = (int)(fmod(t, step * count) / step);
            if (seq_idx >= 0 && seq_idx < count) out = ctx->node_pool[n->inputs[seq_idx + 1]].value;
        }
    }
    return out;
}

/**
 * @brief Returns the current playback time in seconds.
 * @return The time (double).
 */
cr_val op_handler_time(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float((double)ctx->global_time / ctx->sample_rate);
}

/**
 * @brief Returns the current playback position in musical bars.
 *
 * Calculation uses the global time, sample rate, and the current BPM value
 * (fetched from the node pointed to by `ctx->bpm_node_idx`).
 *
 * @return The number of bars elapsed (double).
 */
cr_val op_handler_bars(cr_context *ctx, cr_node *n, cr_val *v) {
    double bpm = 120.0;
    double sec_per_beat;
    double t;
    if (ctx->bpm_node_idx != -1) bpm = as_float(ctx->node_pool[ctx->bpm_node_idx].value);
    sec_per_beat = 60.0 / bpm;
    t = (double)ctx->global_time / ctx->sample_rate;
    /** Divide by seconds per bar (4 * sec_per_beat) */
    return make_float(t / (sec_per_beat * 4.0));
}

/** @brief Handles the floor function. */
cr_val op_handler_floor(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(floor(as_float(v[0])));
}

/** @brief Handles the ceiling function. */
cr_val op_handler_ceil(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(ceil(as_float(v[0])));
}

/** @brief Handles the absolute value function (fabs). */
cr_val op_handler_abs(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(fabs(as_float(v[0])));
}

/** @brief Handles the sign function (+1.0, -1.0, or 0.0). */
cr_val op_handler_sign(cr_context *ctx, cr_node *n, cr_val *v) {
    double val = as_float(v[0]);
    return make_float((val > 0) ? 1.0 : ((val < 0) ? -1.0 : 0.0));
}

/**
 * @brief Converts MIDI note number (v[0]) to frequency (Hz).
 * @return The frequency (Hz).
 */
cr_val op_handler_mtof(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(440.0 * pow(2.0, (as_float(v[0]) - 69.0) / 12.0));
}

/**
 * @brief Linear interpolation (A + (B - A) * T).
 * @param v Array of inputs: v[0] = A (start), v[1] = B (end), v[2] = T (factor 0-1).
 */
cr_val op_handler_lerp(cr_context *ctx, cr_node *n, cr_val *v) {
     double a = as_float(v[0]); double b = as_float(v[1]); double t = as_float(v[2]);
     return make_float(a + (b - a) * t);
}

/**
 * @brief Clamps a value between a minimum and maximum limit.
 * @param v Array of inputs: v[0] = value, v[1] = min, v[2] = max.
 */
cr_val op_handler_clamp(cr_context *ctx, cr_node *n, cr_val *v) {
     double val = as_float(v[0]); double min = as_float(v[1]); double max = as_float(v[2]);
     if (val < min) val = min; if (val > max) val = max;
     return make_float(val);
}

/**
 * @brief Maps a value from an input range to an output range (linear scaling).
 * @param v Array of inputs: v[0] = value, v[1] = in_min, v[2] = in_max, v[3] = out_min, v[4] = out_max.
 */
cr_val op_handler_map(cr_context *ctx, cr_node *n, cr_val *v) {
     double val = as_float(v[0]); double in_min = as_float(v[1]); double in_max = as_float(v[2]);
     double out_min = as_float(v[3]);
     double out_max = as_float(v[4]);
     double t = (val - in_min) / (in_max - in_min);
     return make_float(out_min + t * (out_max - out_min));
}

/** @brief Handles squaring the input value (v[0] * v[0]). */
cr_val op_handler_square(cr_context *ctx, cr_node *n, cr_val *v) {
    double val = as_float(v[0]);
    return make_float(val * val);
}

/** @brief Handles the square root function (sqrt(v[0])). */
cr_val op_handler_sqrt(cr_context *ctx, cr_node *n, cr_val *v) {
    double val = as_float(v[0]);
    return make_float(sqrt(val));
}

/** @brief Handles the exponential function (exp(v[0])). */
cr_val op_handler_exp(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(exp(as_float(v[0])));
}

/** @brief Handles the natural logarithm function (log(v[0])). */
cr_val op_handler_log(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(log(as_float(v[0])));
}

/** @brief Handles the arctangent of two variables (atan2(v[0], v[1])). */
cr_val op_handler_atan2(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(atan2(as_float(v[0]), as_float(v[1])));
}

/**
 * @brief Implements a simple High Pass Filter (HPF).
 *
 * Uses a single-pole high-pass design, storing the filter state in `n->dsp.s[0]`.
 *
 * @param v Array of inputs: v[0] = signal in, v[1] = cutoff frequency.
 * @return The filtered signal out.
 */
cr_val op_handler_hpf(cr_context *ctx, cr_node *n, cr_val *v) {
    double in = as_float(v[0]);
    double cutoff = as_float(v[1]);
    double f, out_val;
    
    /** Calculate coefficient */
    f = 2.0 * sin(CR_PI * cutoff / ctx->sample_rate);
    
    /** Low-pass filtering of the input to get the low-frequency content (state) */
    n->dsp.s[0] += f * (in - n->dsp.s[0]);
    
    /** Output is the input minus the low-frequency content (High Pass) */
    out_val = in - n->dsp.s[0];
    return make_float(out_val);
}

/**
 * @brief Calculates the difference between the current input and the previous input.
 *
 * Implements a simple difference filter (1st-order differentiation).
 * Stores the last input sample in `n->dsp.s[0]`.
 *
 * @param v Array of inputs: v[0] = signal in.
 * @return Current input minus last input.
 */
cr_val op_handler_diff(cr_context *ctx, cr_node *n, cr_val *v) {
    double in = as_float(v[0]);
    double last_in = n->dsp.s[0];
    double diff = in - last_in;
    n->dsp.s[0] = in;
    return make_float(diff);
}

/**
 * @brief Accumulates the input signal over time (integration).
 *
 * Stores the current accumulated value in `n->dsp.s[0]`.
 *
 * @param v Array of inputs: v[0] = signal in, v[1] = integration rate scalar.
 * @return The current accumulated value.
 */
cr_val op_handler_integrate(cr_context *ctx, cr_node *n, cr_val *v) {
    double in = as_float(v[0]);
    double current = n->dsp.s[0];
    double rate = as_float(v[1]);
    /** Accumulate the input scaled by the rate and inverse sample rate */
    current += in * (rate / ctx->sample_rate);
    n->dsp.s[0] = current;
    return make_float(current);
}

/**
 * @brief Executes a single DSP node based on its opcode and inputs.
 *
 * Checks if the node belongs to a skipped conditional block. If not, it retrieves
 * the input values and calls the appropriate operation handler.
 *
 * @param ctx The current context.
 * @param idx The index of the node to run in the node_pool.
 */
static void run_node(cr_context *ctx, int idx) {
    cr_node *n = &ctx->node_pool[idx];
    
    /** Skip execution if this node is part of a block marked for skipping, unless it's a constant. */
    if (n->op_desc->opcode != OP_CONST && n->block_id > 0 && ctx->block_skip_flags[n->block_id]) {
        n->value = make_float(0.0); 
        return;
    }

    if (n->op_desc && n->op_desc->handler) {
        cr_val v[CR_MAX_ARGS];
        int input_max = n->input_count;
        int i;
        cr_val out = make_float(0.0);

        /** Fetch input values from upstream nodes */
        for(i=0; i<input_max; i++) v[i] = ctx->node_pool[n->inputs[i]].value;
        
        /** Execute the operation handler */
        out = n->op_desc->handler(ctx, n, v);

        /** Store the result */
        n->value = out;
    }
}

CR_API cr_engine *cr_create_engine(int sample_rate) {
    cr_engine *eng = (cr_engine*)calloc(1, sizeof(cr_engine));
    size_t pool_size = CR_ARENA_SIZE;
    
    /** Allocate contiguous memory for both contexts' arenas */
    eng->memory_block = calloc(1, pool_size * 2); 
    
    /** Initialize Context 0 (Active) */
    eng->contexts[0].sample_rate = (double)sample_rate;
    eng->contexts[0].arena_base = (unsigned char*)eng->memory_block;
    eng->contexts[0].arena_size = pool_size;
    eng->contexts[0].output_nodes[0] = -1; 
    eng->contexts[0].output_nodes[1] = -1;
    
    /** Initialize Context 1 (Back/Compilation) */
    eng->contexts[1].sample_rate = (double)sample_rate;
    eng->contexts[1].arena_base = (unsigned char*)eng->memory_block + pool_size;
    eng->contexts[1].arena_size = pool_size;
    eng->contexts[1].output_nodes[0] = -1; 
    eng->contexts[1].output_nodes[1] = -1;
    
    eng->active = &eng->contexts[0]; 
    eng->back = &eng->contexts[1];
    
    /** Initialize script source history buffer */
    eng->source_capacity = 4096;
    eng->source_history = (char*)malloc(eng->source_capacity);
    eng->source_history[0] = 0;
    eng->source_len = 0;

    /** Initialize mutex for thread-safe hot-swapping */
    #if defined(_WIN32)
    InitializeCriticalSection(&eng->swap_lock);
    #else
    pthread_mutex_init(&eng->swap_lock, NULL);
    #endif
    return eng;
}

CR_API void cr_destroy_engine(cr_engine *eng) { 
    if (eng) {
        free(eng->source_history);
        free(eng->memory_block); 
        free(eng); 
    }
}

CR_API void cr_set_log_level(cr_engine *eng, int level) { eng->log_level = level; }

CR_API void cr_tick(cr_engine *engine) { engine->active->global_time++; }

CR_API double cr_process(cr_engine *engine, int channel) {
    cr_context *ctx = engine->active;
    int i;
    
    /** Only execute the graph fully once per sample (for channel 0) */
    if (channel == 0) { 
        for(i=0; i<ctx->exec_count; i++) run_node(ctx, ctx->exec_order[i]); 
    }
    
    if (channel < 0 || channel >= CR_MAX_CHANNELS) return 0.0;
    
    /** Return the value of the output node for the requested channel */
    {
        int out_node = ctx->output_nodes[channel];
        if (out_node == -1) return 0.0;
        return as_float(ctx->node_pool[out_node].value);
    }
}

#endif
#endif

#ifndef CHRONOS_H
#define CHRONOS_H

#include <setjmp.h>
#include <stddef.h>

#ifndef CR_API
#define CR_API
#endif

#define CR_MAX_CHANNELS 2

typedef enum {
  CR_LOG_NONE = 0,
  CR_LOG_ERROR,
  CR_LOG_WARN,
  CR_LOG_INFO,
  CR_LOG_DEBUG
} cr_log_level;

typedef struct cr_context cr_context;

CR_API cr_context *cr_create_context(void);
CR_API void cr_destroy_context(cr_context *ctx);
CR_API void cr_set_log_level(cr_context *ctx, int level);

CR_API double cr_process(cr_context *ctx, int channel);
CR_API int cr_run(cr_context *ctx, const char *script);
CR_API void cr_lock(cr_context *ctx);
CR_API void cr_unlock(cr_context *ctx);

#ifdef CHRONOS_IMPLEMENTATION

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

#ifndef WAVL_H
#include "wavl.h"
#endif

#define CR_MAX_SYMBOLS 1024
#define CR_MAX_NODES 16384
#define CR_MAX_BUFFERS 128
#define CR_MAX_ARGS 64
#define CR_MAX_FUNCS 256
#define CR_MAX_MACROS 256
#define CR_MAX_MACRO_SIZE 8192
#define CR_PI 3.14159265359
#define CR_SR 44100.0
#define CR_VM_STACK_SIZE 1024
#define CR_ARENA_SIZE (64 * 1024 * 1024) 

enum {
  OP_HALT = 0,
  OP_CONST, OP_LOAD, OP_STORE,
  OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, OP_POW,
  OP_GT, OP_LT, OP_GE, OP_LE, OP_EQ, OP_NE, OP_AND, OP_OR, OP_NOT,
  OP_SINE, OP_PHASOR, OP_NOISE,
  OP_SEQ, OP_LPF, OP_FILTER, OP_PEAK_EQ,
  OP_DELAY, OP_REVERB, OP_COMPRESS, OP_LIMIT, OP_ADSR, OP_CLIP,
  OP_TANH, OP_FOLD, OP_SLEW, OP_SAH,
  OP_PROBE, OP_TIME, OP_READ, 
  OP_GET, OP_GET_WRAP, OP_SET, OP_COUNT, OP_LEN, OP_SELECT,
  OP_FLOOR, OP_CEIL, OP_ABS, OP_SIGN,
  OP_MTOF, OP_LERP,
  OP_LOGISTIC, OP_HENON, OP_WOLFRAM,
  OP_WAVE
};

typedef struct {
    double s[8];
    double* buffer; /* High Fidelity: Double precision delay lines */
    int buf_len;
    int write_head;
    int type;
} cr_dsp_state;

typedef struct cr_node {
  int id;
  int op;
  int inputs[CR_MAX_ARGS];
  int input_count;
  double value;
  int is_constant;
  cr_dsp_state dsp;
  int i_data;
} cr_node;

typedef struct {
  char name[32];
  int node_index;
} cr_variable;

typedef struct {
  char name[32];
  char args[CR_MAX_ARGS][32];
  int arg_count;
  char body[CR_MAX_MACRO_SIZE];
} cr_macro;

typedef struct {
  double *data; /* High Fidelity: Double precision sample data */
  long length;
  int channels;
  char name[64];
} cr_buffer;

struct cr_context {
  long global_time;
  int output_nodes[CR_MAX_CHANNELS];
  
  cr_variable variables[CR_MAX_SYMBOLS];
  int var_count;
  
  cr_macro macros[CR_MAX_MACROS];
  int macro_count;
  
  cr_node node_pool[CR_MAX_NODES];
  int node_idx;
  
  int exec_order[CR_MAX_NODES];
  int exec_count;

  cr_buffer buffers[CR_MAX_BUFFERS];
  int buffer_count;
  
  unsigned char *arena_base;
  size_t arena_size;
  size_t arena_top;

  char scope[128];
  int scope_id_ctr;
  int return_reg;
  int returning;
  
  jmp_buf err_jmp;
  char error_msg[128];
  int log_level;
  int current_line;
  
#if defined(_WIN32)
  CRITICAL_SECTION lock;
#else
  pthread_mutex_t lock;
#endif
};

static void cr_log(cr_context *ctx, int level, const char *fmt, ...) {
  if (level <= ctx->log_level) {
    va_list args;
    const char *tag = "[INFO]";
    if (level == CR_LOG_ERROR) tag = "[ERROR]";
    else if (level == CR_LOG_WARN) tag = "[WARN ]";
    else if (level == CR_LOG_DEBUG) tag = "[DEBUG]";
    fprintf(stdout, "%s ", tag);
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
    fprintf(stdout, "\n");
  }
}

static void cr_error(cr_context *ctx, const char *msg) {
  char buf[128];
  sprintf(buf, "Line %d: %s", ctx->current_line, msg);
  strncpy(ctx->error_msg, buf, 127);
  cr_log(ctx, CR_LOG_ERROR, "%s", buf);
  longjmp(ctx->err_jmp, 1);
}

static void* cr_arena_alloc(cr_context* ctx, size_t size) {
    size_t aligned = (size + 7) & ~7;
    if (ctx->arena_top + aligned > ctx->arena_size) {
         cr_error(ctx, "Memory Arena Exceeded");
         return NULL;
    }
    void* ptr = ctx->arena_base + ctx->arena_top;
    ctx->arena_top += aligned;
    return ptr;
}

static int alloc_node(cr_context *ctx, int op) {
  if (ctx->node_idx >= CR_MAX_NODES)
    cr_error(ctx, "Max nodes reached");
  int idx = ctx->node_idx++;
  cr_node *n = &ctx->node_pool[idx];
  memset(n, 0, sizeof(cr_node));
  n->id = idx;
  n->op = op;
  n->input_count = 0;
  n->is_constant = 0;
  
  if (op == OP_CONST) n->is_constant = 1;
  return idx;
}

static double hermite(double y0, double y1, double y2, double y3, double mu) {
   double m0, m1, mu2, mu3;
   double a0, a1, a2, a3;
   mu2 = mu * mu;
   mu3 = mu2 * mu;
   m0  = (y1-y0)/2.0 + (y2-y1)/2.0;
   m1  = (y2-y1)/2.0 + (y3-y2)/2.0;
   a0 =  2*mu3 - 3*mu2 + 1;
   a1 =    mu3 - 2*mu2 + mu;
   a2 =    mu3 -   mu2;
   a3 = -2*mu3 + 3*mu2;
   return(a0*y1 + a1*m0 + a2*m1 + a3*y2);
}

static double run_node(cr_context *ctx, int idx) {
    cr_node *n = &ctx->node_pool[idx];
    double v[4] = {0};
    int i;
    double out = 0.0;
    
    if (n->op == OP_CONST) return n->value;

    for(i=0; i<n->input_count; i++) {
        v[i] = ctx->node_pool[n->inputs[i]].value;
    }

    switch (n->op) {
        case OP_ADD: out = v[0] + v[1]; break;
        case OP_SUB: out = v[0] - v[1]; break;
        case OP_MUL: out = v[0] * v[1]; break;
        case OP_DIV: out = (v[1] == 0) ? 0 : v[0] / v[1]; break;
        
        case OP_POW: {
            double base = v[0];
            double exp = v[1];
            double res = pow(fabs(base), exp);
            if (base < 0) res = -res;
            out = res;
            break;
        }

        case OP_MOD: out = fmod(v[0], v[1]); break;
        
        case OP_GT: out = (v[0] > v[1]) ? 1.0 : 0.0; break;
        case OP_LT: out = (v[0] < v[1]) ? 1.0 : 0.0; break;
        case OP_GE: out = (v[0] >= v[1]) ? 1.0 : 0.0; break;
        case OP_LE: out = (v[0] <= v[1]) ? 1.0 : 0.0; break;
        case OP_EQ: out = (fabs(v[0] - v[1]) < 0.000001) ? 1.0 : 0.0; break;
        case OP_NE: out = (fabs(v[0] - v[1]) > 0.000001) ? 1.0 : 0.0; break;
        case OP_AND: out = (v[0] > 0.5 && v[1] > 0.5) ? 1.0 : 0.0; break;
        case OP_OR:  out = (v[0] > 0.5 || v[1] > 0.5) ? 1.0 : 0.0; break;
        case OP_NOT: out = (v[0] <= 0.5) ? 1.0 : 0.0; break;
        
        case OP_SELECT: out = (v[0] > 0.5) ? v[1] : v[2]; break;
        
        case OP_FLOOR: out = floor(v[0]); break;
        case OP_CEIL:  out = ceil(v[0]); break;
        case OP_ABS:   out = fabs(v[0]); break;
        case OP_SIGN:  out = (v[0] > 0) ? 1.0 : ((v[0] < 0) ? -1.0 : 0.0); break;

        case OP_MTOF:  out = 440.0 * pow(2.0, (v[0] - 69.0) / 12.0); break;
        case OP_LERP:  out = v[0] + (v[1] - v[0]) * v[2]; break;
        
        case OP_SLEW: {
            double target = v[0];
            double time = v[1];
            double current = n->dsp.s[0];
            double dt = 1.0 / CR_SR;
            double step = (time > 0.0001) ? (1.0 / time) * dt : 1.0;
            if (target > current) {
                current += step; if (current > target) current = target;
            } else if (target < current) {
                current -= step; if (current < target) current = target;
            }
            n->dsp.s[0] = current; out = current;
            break;
        }

        case OP_SAH: {
            double signal = v[0];
            double trig = v[1];
            double last_trig = n->dsp.s[1];
            double val = n->dsp.s[0];
            if (trig > 0.5 && last_trig <= 0.5) val = signal;
            n->dsp.s[0] = val; n->dsp.s[1] = trig; out = val;
            break;
        }

        case OP_TANH: out = tanh(v[0]); break;
        
        case OP_FOLD: {
            double in = v[0];
            double thresh = v[1];
            if (thresh < 0.001) thresh = 0.001;
            in /= thresh;
            out = sin(in); 
            break;
        }
        
        case OP_LOGISTIC: {
            double r = v[0];
            double trig = v[1];
            double prev_trig = n->dsp.s[1];
            double x = n->dsp.s[0];
            if (x == 0.0) x = 0.5; 
            if (trig > 0.5 && prev_trig <= 0.5) {
                x = r * x * (1.0 - x);
            }
            n->dsp.s[0] = x; n->dsp.s[1] = trig; out = x;
            break;
        }

        case OP_HENON: {
            double trig = v[0];
            double prev_trig = n->dsp.s[2];
            double x = n->dsp.s[0];
            double y = n->dsp.s[1];
            if (trig > 0.5 && prev_trig <= 0.5) {
                double nx = 1.0 - 1.4 * x * x + y;
                double ny = 0.3 * x;
                x = nx; y = ny;
            }
            n->dsp.s[0] = x; n->dsp.s[1] = y; n->dsp.s[2] = trig; out = x;
            break;
        }

        case OP_WOLFRAM: {
            int rule = (int)v[0];
            double trig = v[1];
            double prev_trig = n->dsp.s[1];
            unsigned int state = (unsigned int)n->dsp.s[0];
            if (state == 0) state = 0xACE1; 
            
            if (trig > 0.5 && prev_trig <= 0.5) {
                unsigned int next_state = 0;
                int k;
                for (k = 0; k < 32; k++) {
                    int l = (state >> ((k + 1) % 32)) & 1;
                    int c = (state >> k) & 1;
                    int r = (state >> ((k - 1 + 32) % 32)) & 1;
                    int p = (l << 2) | (c << 1) | r;
                    if ((rule >> p) & 1) next_state |= (1 << k);
                }
                state = next_state;
            }
            n->dsp.s[0] = (double)state; n->dsp.s[1] = trig;
            out = (double)state / 4294967295.0 * 2.0 - 1.0;
            break;
        }

        case OP_WAVE: {
            double excite = v[0];
            double damp = v[1];
            double speed = v[2];
            double pos = v[3];
            int N = 64;
            if (!n->dsp.buffer) {
                n->dsp.buf_len = N * 2; 
                n->dsp.buffer = (double*)cr_arena_alloc(ctx, n->dsp.buf_len * sizeof(double));
                if (n->dsp.buffer) memset(n->dsp.buffer, 0, n->dsp.buf_len * sizeof(double));
            }
            if (n->dsp.buffer) {
                double* u = n->dsp.buffer;       
                double* u_prev = n->dsp.buffer + N;
                int p_idx = (int)(pos * (N - 1));
                if (p_idx < 1) p_idx = 1; if (p_idx > N - 2) p_idx = N - 2;
                if (damp > 0.9999) damp = 0.9999; if (damp < 0.8) damp = 0.8;
                
                u[p_idx] += excite * 0.1; 

                double c2 = speed * 0.5;
                double u_next[64];
                u_next[0] = 0.0; u_next[N-1] = 0.0;
                int i;
                for(i=1; i<N-1; i++) {
                    double laplacian = u[i+1] - 2.0*u[i] + u[i-1];
                    double val = 2.0*u[i] - u_prev[i] + c2 * laplacian;
                    u_next[i] = val * damp;
                }
                for(i=0; i<N; i++) { u_prev[i] = u[i]; u[i] = u_next[i]; }
                out = u[p_idx];
            }
            break;
        }

        case OP_SINE: out = sin(v[0] * 2.0 * CR_PI); break;
            
        case OP_PHASOR: {
            double freq = v[0];
            double dt = freq / CR_SR;
            double p = n->dsp.s[0];
            p += dt;
            p -= floor(p);
            n->dsp.s[0] = p;
            out = p;
            break;
        }
        
        case OP_NOISE:
            out = ((double)rand() / (double)RAND_MAX) * 2.0 - 1.0;
            break;

        case OP_CLIP:
            out = v[0];
            if (out > 1.0) out = 1.0;
            if (out < -1.0) out = -1.0;
            break;

        case OP_SEQ: {
            double step = v[0];
            int count = n->input_count - 1;
            if (count <= 0 || step <= 0.0001) out = 0.0;
            else {
                double t = (double)ctx->global_time / CR_SR;
                double dur = step * count;
                double lt = fmod(t, dur);
                int seq_idx = (int)(lt / step);
                if (seq_idx >= count) seq_idx = count - 1;
                if (seq_idx < 0) seq_idx = 0;
                out = ctx->node_pool[n->inputs[seq_idx + 1]].value;
            }
            break;
        }
        
        case OP_GET: {
            int buf_idx = (int)v[0];
            if (buf_idx >= 0 && buf_idx < ctx->buffer_count) {
                cr_buffer *b = &ctx->buffers[buf_idx];
                int idx = (int)v[1];
                if (idx < 0) idx = 0; 
                if (idx >= b->length) idx = b->length - 1;
                out = b->data[idx];
            }
            break;
        }

        case OP_GET_WRAP: {
            int buf_idx = (int)v[0];
            if (buf_idx >= 0 && buf_idx < ctx->buffer_count) {
                cr_buffer *b = &ctx->buffers[buf_idx];
                if (b->length > 0) {
                    int idx = (int)v[1];
                    int wrapped = idx % b->length;
                    if (wrapped < 0) wrapped += b->length;
                    out = b->data[wrapped];
                }
            }
            break;
        }

        case OP_SET: {
            int buf_idx = (int)v[0];
            double val = v[2];
            if (buf_idx >= 0 && buf_idx < ctx->buffer_count) {
                cr_buffer *b = &ctx->buffers[buf_idx];
                int idx = (int)v[1];
                if (idx >= 0 && idx < b->length) {
                    b->data[idx] = val;
                }
            }
            out = val;
            break;
        }
        
        case OP_COUNT: {
            double trig = v[0];
            double reset = v[1];
            double prev = n->dsp.s[1];
            double count = n->dsp.s[0];
            
            if (reset > 0.5) count = 0;
            else if (trig > 0.1 && prev <= 0.1) {
                count = count + 1.0;
            }
            
            n->dsp.s[0] = count;
            n->dsp.s[1] = trig;
            out = count;
            break;
        }
        
        case OP_LEN: {
            int buf_idx = (int)v[0];
            if (buf_idx >= 0 && buf_idx < ctx->buffer_count) {
                out = (double)ctx->buffers[buf_idx].length;
            }
            break;
        }

        case OP_LPF: {
            double in = v[0];
            double cut = v[1];
            double dt = 1.0 / CR_SR;
            double rc = 1.0 / (2.0 * CR_PI * cut);
            double alpha = dt / (rc + dt);
            double prev = n->dsp.s[0];
            double out_val = prev + alpha * (in - prev);
            n->dsp.s[0] = out_val;
            out = out_val;
            break;
        }

        case OP_DELAY: {
            double in = v[0];
            double time = v[1];
            double fb = v[2];
            
            if (!n->dsp.buffer) {
                n->dsp.buf_len = (int)(2.0 * CR_SR); 
                n->dsp.buffer = (double*)cr_arena_alloc(ctx, n->dsp.buf_len * sizeof(double));
                if (n->dsp.buffer) memset(n->dsp.buffer, 0, n->dsp.buf_len * sizeof(double));
            }
            
            if (n->dsp.buffer) {
                double delay_samps = time * CR_SR;
                if (delay_samps < 1.0) delay_samps = 1.0;
                if (delay_samps > n->dsp.buf_len - 4) delay_samps = n->dsp.buf_len - 4;
                
                double read_pos = (double)n->dsp.write_head - delay_samps;
                while(read_pos < 0) read_pos += n->dsp.buf_len;
                
                int r0 = (int)read_pos;
                double frac = read_pos - r0;
                int r1 = (r0 + 1) % n->dsp.buf_len;
                int r2 = (r0 + 2) % n->dsp.buf_len;
                int r_1 = (r0 - 1 + n->dsp.buf_len) % n->dsp.buf_len;
                
                double delayed = hermite(n->dsp.buffer[r_1], n->dsp.buffer[r0], n->dsp.buffer[r1], n->dsp.buffer[r2], frac);
                
                double next_val = in + delayed * fb;
                next_val = tanh(next_val);

                n->dsp.buffer[n->dsp.write_head] = next_val;
                n->dsp.write_head++;
                if (n->dsp.write_head >= n->dsp.buf_len) n->dsp.write_head = 0;
                out = delayed;
            }
            break;
        }

        case OP_REVERB: {
            double in = v[0];
            double size = v[1];
            double fb = v[2];
            
            if (!n->dsp.buffer) {
                n->dsp.buf_len = (int)(0.5 * CR_SR); 
                n->dsp.buffer = (double*)cr_arena_alloc(ctx, n->dsp.buf_len * sizeof(double));
                if (n->dsp.buffer) memset(n->dsp.buffer, 0, n->dsp.buf_len * sizeof(double));
            }
            
            if (n->dsp.buffer) {
                int tap1 = (int)(0.0297 * CR_SR * size);
                int tap2 = (int)(0.0371 * CR_SR * size);
                int tap3 = (int)(0.0411 * CR_SR * size);
                int tap4 = (int)(0.0437 * CR_SR * size);
                
                int p = n->dsp.write_head;
                int l = n->dsp.buf_len;
                
                double val1 = n->dsp.buffer[(p - tap1 + l) % l];
                double val2 = n->dsp.buffer[(p - tap2 + l) % l];
                double val3 = n->dsp.buffer[(p - tap3 + l) % l];
                double val4 = n->dsp.buffer[(p - tap4 + l) % l];
                
                double sum = val1 + val2 + val3 + val4;
                
                double next = in * 0.5 + sum * fb * 0.2;
                if(next > 1.0) next = 1.0; if(next < -1.0) next = -1.0;
                
                n->dsp.buffer[p] = next;
                n->dsp.write_head = (p + 1) % l;
                out = sum * 0.25;
            }
            break;
        }

        case OP_FILTER: {
            double in = v[0];
            int type = (int)v[1];
            double freq = v[2];
            double q = v[3];
            
            if (q < 0.1) q = 0.1;
            if (freq < 10) freq = 10;
            if (freq > CR_SR/2 - 100) freq = CR_SR/2 - 100;

            double omega = 2.0 * CR_PI * freq / CR_SR;
            double alpha = sin(omega) / (2.0 * q);
            double cos_w = cos(omega);
            
            double a0, a1, a2, b0, b1, b2;
            
            if (type == 1) {
                b0 =  (1 - cos_w) / 2;
                b1 =   1 - cos_w;
                b2 =  (1 - cos_w) / 2;
                a0 =   1 + alpha;
                a1 =  -2 * cos_w;
                a2 =   1 - alpha;
            } else if (type == 2) {
                b0 =  (1 + cos_w) / 2;
                b1 = -(1 + cos_w);
                b2 =  (1 + cos_w) / 2;
                a0 =   1 + alpha;
                a1 =  -2 * cos_w;
                a2 =   1 - alpha;
            } else {
                 b0 =   alpha;
                 b1 =   0;
                 b2 =  -alpha;
                 a0 =   1 + alpha;
                 a1 =  -2 * cos_w;
                 a2 =   1 - alpha;
            }
            
            b0 /= a0; b1 /= a0; b2 /= a0; a1 /= a0; a2 /= a0;
            double res = b0 * in + b1 * n->dsp.s[0] + b2 * n->dsp.s[1] - a1 * n->dsp.s[2] - a2 * n->dsp.s[3];
            
            res += 1.0e-18; res -= 1.0e-18;
            if(!isfinite(res)) res = 0.0;
            
            n->dsp.s[1] = n->dsp.s[0]; n->dsp.s[0] = in;
            n->dsp.s[3] = n->dsp.s[2]; n->dsp.s[2] = res;
            out = res;
            break;
        }

        case OP_PEAK_EQ: {
            double in = v[0];
            double freq = v[1];
            double gain_db = v[2];
            double q = v[3];
            
            if (q < 0.1) q = 0.1;
            if (freq < 10) freq = 10;
            
            double A = pow(10.0, gain_db / 40.0);
            double omega = 2.0 * CR_PI * freq / CR_SR;
            double sn = sin(omega);
            double cs = cos(omega);
            double alpha = sn / (2.0 * q);
            
            double b0 = 1.0 + alpha * A;
            double b1 = -2.0 * cs;
            double b2 = 1.0 - alpha * A;
            double a0 = 1.0 + alpha / A;
            double a1 = -2.0 * cs;
            double a2 = 1.0 - alpha / A;
            
            b0 /= a0; b1 /= a0; b2 /= a0; a1 /= a0; a2 /= a0;
            
            double res = b0 * in + b1 * n->dsp.s[0] + b2 * n->dsp.s[1] 
                                 - a1 * n->dsp.s[2] - a2 * n->dsp.s[3];
                                 
            res += 1.0e-18; res -= 1.0e-18;
            if(!isfinite(res)) res = 0.0;

            n->dsp.s[1] = n->dsp.s[0]; n->dsp.s[0] = in;
            n->dsp.s[3] = n->dsp.s[2]; n->dsp.s[2] = res;
            out = res;
            break;
        }
        
        case OP_COMPRESS: {
            double in = v[0];
            double thresh_db = v[1];
            double ratio = v[2];
            double ar = v[3];
            
            double env = fabs(in);
            double curr_env = n->dsp.s[0];
            if (env > curr_env) curr_env += ar * (env - curr_env);
            else curr_env += (ar * 0.1) * (env - curr_env);
            n->dsp.s[0] = curr_env;
            
            double gain = 1.0;
            double env_db = (curr_env > 0.000001) ? 20.0 * log10(curr_env) : -120.0;
            
            if (env_db > thresh_db) {
                double over = env_db - thresh_db;
                double out_db = thresh_db + over / ratio;
                double gain_db = out_db - env_db;
                gain = pow(10.0, gain_db / 20.0);
            }
            out = in * gain;
            break;
        }

        case OP_LIMIT: {
            double in = v[0];
            if (in > 1.0) in = 1.0;
            if (in < -1.0) in = -1.0;
            out = in;
            break;
        }
        
        case OP_ADSR: {
            double gate = v[0];
            double a = v[1];
            double d = v[2];
            int state = (int)n->dsp.s[0];
            double val = n->dsp.s[1];
            
            if (gate > 0.5 && state == 0) state = 1;
            if (gate < 0.5) state = 0;
            
            double dt = 1.0/CR_SR;
            if (state == 1) {
                double rate = 1.0 / (a + 0.001);
                val += rate * dt;
                if (val >= 1.0) { val = 1.0; state = 2; }
            } else if (state == 2) {
                double rate = 1.0 / (d + 0.001);
                val -= rate * dt;
                if (val < 0.0) { val = 0.0; state = 0; }
            } else {
                 val -= 20.0 * dt;
                 if (val < 0) val = 0;
            }
            n->dsp.s[0] = (double)state;
            n->dsp.s[1] = val;
            out = val;
            break;
        }

        case OP_PROBE:
            n->dsp.s[0] += 1.0 / CR_SR;
            if (n->dsp.s[0] > 0.5) {
                n->dsp.s[0] = 0.0;
                printf("[PROBE %d] %.4f\n", n->id, v[0]);
            }
            out = v[0];
            break;
            
        case OP_TIME:
            out = (double)ctx->global_time / CR_SR;
            break;

        case OP_READ: {
            cr_buffer *b = &ctx->buffers[n->i_data];
            double phase = v[0];
            double idx_f = phase * b->length;
            long idx = (long)idx_f;
            if (b->length > 0) {
                idx = idx % b->length;
                if (idx < 0) idx += b->length;
                out = b->data[idx * b->channels];
            } else {
                out = 0.0;
            }
            break;
        }
    }
    
    if(!isfinite(out)) out = 0.0;
    n->value = out;
    return out;
}

static char *token_ptr;
static char token[128];
static int token_type;

static void get_tok(cr_context *ctx) {
  while (1) {
    while (isspace(*token_ptr)) {
      if (*token_ptr == '\n') ctx->current_line++;
      token_ptr++;
    }
    if (*token_ptr == 0) {
      token_type = 0;
      return;
    }
    if (*token_ptr == '#') {
      while (*token_ptr != '\n' && *token_ptr != 0)
        token_ptr++;
      continue;
    }
    break;
  }

  if (isalpha(*token_ptr) || *token_ptr == '_') {
    int i = 0;
    while (isalnum(*token_ptr) || *token_ptr == '_' || *token_ptr == '.')
      token[i++] = *token_ptr++;
    token[i] = 0;
    token_type = 1;
  } else if (isdigit(*token_ptr) || (*token_ptr == '-' && isdigit(token_ptr[1]))) {
    int i = 0;
    token[i++] = *token_ptr++;
    while (isdigit(*token_ptr) || *token_ptr == '.')
      token[i++] = *token_ptr++;
    token[i] = 0;
    token_type = 2;
  } else if (*token_ptr == '"') {
    int i = 0;
    token_ptr++;
    while (*token_ptr != '"' && *token_ptr)
      token[i++] = *token_ptr++;
    if (*token_ptr == '"')
      token_ptr++;
    token[i] = 0;
    token_type = 4;
  } else if (strchr("><=!&|", *token_ptr)) {
    token[0] = *token_ptr++;
    if (*token_ptr == '=' || (*token_ptr == token[0] && (*token_ptr == '&' || *token_ptr == '|'))) {
         token[1] = *token_ptr++;
         token[2] = 0;
    } else {
         token[1] = 0;
    }
    token_type = 3; 
  } else {
    token[0] = *token_ptr++;
    token[1] = 0;
    token_type = 3;
  }
}

static void match(cr_context *ctx, const char *str) {
  if (strcmp(token, str) != 0) {
    char msg[64];
    sprintf(msg, "Expected '%s' found '%s'", str, token);
    cr_error(ctx, msg);
  }
  get_tok(ctx);
}

static int try_fold_const(cr_context *ctx, int op, int in1, int in2) {
    cr_node *n1 = &ctx->node_pool[in1];
    cr_node *n2 = (in2 >= 0) ? &ctx->node_pool[in2] : NULL;
    
    if (n1->is_constant && (!n2 || n2->is_constant)) {
        double v1 = n1->value;
        double v2 = n2 ? n2->value : 0;
        double res = 0;
        
        switch(op) {
            case OP_ADD: res = v1 + v2; break;
            case OP_SUB: res = v1 - v2; break;
            case OP_MUL: res = v1 * v2; break;
            case OP_DIV: res = (v2 == 0) ? 0 : v1 / v2; break;
            case OP_SINE: res = sin(v1 * 2 * CR_PI); break;
            case OP_POW: res = pow(v1, v2); break;
            case OP_FLOOR: res = floor(v1); break;
            case OP_CEIL: res = ceil(v1); break;
            case OP_ABS: res = fabs(v1); break;
            case OP_MTOF: res = 440.0 * pow(2.0, (v1 - 69.0) / 12.0); break;
            default: return -1;
        }
        
        int nidx = alloc_node(ctx, OP_CONST);
        ctx->node_pool[nidx].value = res;
        ctx->node_pool[nidx].is_constant = 1;
        return nidx;
    }
    return -1;
}

static int find_var_node(cr_context *ctx, const char *name) {
  int i;
  char scoped_name[160];
  if (ctx->scope[0] != 0) {
    sprintf(scoped_name, "%s%s", ctx->scope, name);
    for (i = 0; i < ctx->var_count; i++) {
      if (strcmp(ctx->variables[i].name, scoped_name) == 0)
        return ctx->variables[i].node_index;
    }
  }
  for (i = 0; i < ctx->var_count; i++) {
    if (strcmp(ctx->variables[i].name, name) == 0)
      return ctx->variables[i].node_index;
  }
  return -1;
}

static void set_var_node(cr_context *ctx, const char *name, int idx) {
    int i;
    char target[160];
    if (ctx->scope[0] != 0)
      sprintf(target, "%s%s", ctx->scope, name);
    else
      strcpy(target, name);

    for(i=0; i<ctx->var_count; i++) {
        if (!strcmp(ctx->variables[i].name, target)) {
            ctx->variables[i].node_index = idx;
            return;
        }
    }
    if (ctx->var_count < CR_MAX_SYMBOLS) {
        strcpy(ctx->variables[ctx->var_count].name, target);
        ctx->variables[ctx->var_count++].node_index = idx;
    } else {
        cr_error(ctx, "Symbol table full");
    }
}

static int load_buffer(cr_context *ctx, const char *filename) {
  int i;
  for (i = 0; i < ctx->buffer_count; i++) {
    if (strcmp(ctx->buffers[i].name, filename) == 0) return i;
  }
  if (ctx->buffer_count >= CR_MAX_BUFFERS) cr_error(ctx, "Max buffers");
  
  wav_file_t *w = wav_load(filename);
  if (!w) cr_error(ctx, "File not found");
  
  long frames = w->data_size / (w->num_channels * (w->bits_per_sample / 8));
  cr_buffer *b = &ctx->buffers[ctx->buffer_count];
  b->length = frames;
  b->channels = w->num_channels;
  strcpy(b->name, filename);
  
  b->data = (double *)cr_arena_alloc(ctx, frames * w->num_channels * sizeof(double));
  if (!b->data) { free(w); return -1; }
  
  short *src = (short *)w->data;
  for (long k = 0; k < frames * w->num_channels; k++)
    b->data[k] = (double)src[k] / 32768.0;
  free(w);
  return ctx->buffer_count++;
}

static int make_array(cr_context *ctx, int count, double *vals) {
    if (ctx->buffer_count >= CR_MAX_BUFFERS) cr_error(ctx, "Max buffers");
    cr_buffer *b = &ctx->buffers[ctx->buffer_count];
    b->length = count;
    b->channels = 1;
    sprintf(b->name, "arr_%d", ctx->buffer_count);
    
    b->data = (double *)cr_arena_alloc(ctx, count * sizeof(double));
    if (!b->data) return -1;
    
    int i;
    for(i=0; i<count; i++) b->data[i] = vals[i];
    return ctx->buffer_count++;
}

static int cr_find_macro(cr_context *ctx, const char *name) {
  int i;
  for (i = 0; i < ctx->macro_count; i++) {
    if (strcmp(ctx->macros[i].name, name) == 0)
      return i;
  }
  return -1;
}

static void statement(cr_context *ctx);
static int expr(cr_context *ctx);

static int factor(cr_context *ctx) {
    int idx = -1;
    if (token_type == 2) {
        idx = alloc_node(ctx, OP_CONST);
        ctx->node_pool[idx].value = atof(token);
        ctx->node_pool[idx].is_constant = 1;
        get_tok(ctx);
    } else if (token_type == 1) {
        char name[32];
        strcpy(name, token);
        get_tok(ctx);
        if (strcmp(token, "(") == 0) {
            get_tok(ctx);
            
            if (strcmp(name, "load") == 0) {
                if (token_type != 4) cr_error(ctx, "String expected");
                int bi = load_buffer(ctx, token);
                get_tok(ctx);
                
                int ph_idx;
                if (strcmp(token, ",") == 0) {
                    get_tok(ctx);
                    ph_idx = expr(ctx);
                } else {
                    int t = alloc_node(ctx, OP_TIME);
                    int len = alloc_node(ctx, OP_CONST);
                    ctx->node_pool[len].value = (double)ctx->buffers[bi].length / CR_SR;
                    ctx->node_pool[len].is_constant = 1;
                    int d = alloc_node(ctx, OP_DIV);
                    ctx->node_pool[d].inputs[0] = t;
                    ctx->node_pool[d].inputs[1] = len;
                    ctx->node_pool[d].input_count = 2;
                    ph_idx = d;
                }
                match(ctx, ")");
                idx = alloc_node(ctx, OP_READ);
                ctx->node_pool[idx].inputs[0] = ph_idx;
                ctx->node_pool[idx].input_count = 1;
                ctx->node_pool[idx].i_data = bi;
                return idx;
            }
            
            if (strcmp(name, "array") == 0) {
                double vals[CR_MAX_ARGS];
                int c = 0;
                if (strcmp(token, ")") != 0) {
                    while(1) {
                        if (c >= CR_MAX_ARGS) cr_error(ctx, "Max array size");
                        int e = expr(ctx);
                        int folded = try_fold_const(ctx, OP_ADD, e, -1);
                        if (folded != -1) vals[c++] = ctx->node_pool[folded].value;
                        else cr_error(ctx, "Array contents must be constant");
                        
                        if (strcmp(token, ",") == 0) get_tok(ctx);
                        else break;
                    }
                }
                match(ctx, ")");
                int bid = make_array(ctx, c, vals);
                idx = alloc_node(ctx, OP_CONST);
                ctx->node_pool[idx].value = (double)bid;
                ctx->node_pool[idx].is_constant = 1;
                return idx;
            }
            
            if (strcmp(name, "select") == 0) {
                int cond = expr(ctx);
                match(ctx, ",");
                int a = expr(ctx);
                match(ctx, ",");
                int b = expr(ctx);
                match(ctx, ")");
                idx = alloc_node(ctx, OP_SELECT);
                ctx->node_pool[idx].inputs[0] = cond;
                ctx->node_pool[idx].inputs[1] = a;
                ctx->node_pool[idx].inputs[2] = b;
                ctx->node_pool[idx].input_count = 3;
                return idx;
            }

            int mi = cr_find_macro(ctx, name);
            if (mi >= 0) {
                cr_macro *m = &ctx->macros[mi];
                int args[CR_MAX_ARGS];
                int ac = 0;
                
                if (strcmp(token, ")") != 0) {
                    while(1) {
                        if (ac >= CR_MAX_ARGS) cr_error(ctx, "Too many args");
                        args[ac++] = expr(ctx);
                        if (strcmp(token, ",") == 0) get_tok(ctx);
                        else break;
                    }
                }
                match(ctx, ")");
                if (ac != m->arg_count) cr_error(ctx, "Arg count mismatch");

                char old_scope[128];
                char *old_ptr = token_ptr;
                char old_tok[128];
                int old_type = token_type;
                int old_line = ctx->current_line;
                
                strcpy(old_scope, ctx->scope);
                strcpy(old_tok, token);
                sprintf(ctx->scope, "%s%d_", m->name, ctx->scope_id_ctr++);
                
                int i;
                for(i=0; i<ac; i++) set_var_node(ctx, m->args[i], args[i]);

                token_ptr = m->body;
                ctx->current_line = 1;
                get_tok(ctx);
                while(token_type != 0) {
                    statement(ctx);
                    if(ctx->returning) break;
                }
                idx = ctx->return_reg;
                ctx->returning = 0;
                
                strcpy(ctx->scope, old_scope);
                token_ptr = old_ptr;
                strcpy(token, old_tok);
                token_type = old_type;
                ctx->current_line = old_line;
                
                return idx;
            }

            int op = -1;
            if (!strcmp(name, "sine")) op = OP_SINE;
            else if (!strcmp(name, "phasor")) op = OP_PHASOR;
            else if (!strcmp(name, "noise")) op = OP_NOISE;
            else if (!strcmp(name, "seq")) op = OP_SEQ;
            else if (!strcmp(name, "lpf")) op = OP_LPF;
            else if (!strcmp(name, "delay")) op = OP_DELAY;
            else if (!strcmp(name, "reverb")) op = OP_REVERB;
            else if (!strcmp(name, "filter")) op = OP_FILTER;
            else if (!strcmp(name, "eq")) op = OP_PEAK_EQ;
            else if (!strcmp(name, "compress")) op = OP_COMPRESS;
            else if (!strcmp(name, "limit")) op = OP_LIMIT;
            else if (!strcmp(name, "adsr")) op = OP_ADSR;
            else if (!strcmp(name, "clip")) op = OP_CLIP;
            else if (!strcmp(name, "tanh")) op = OP_TANH;
            else if (!strcmp(name, "fold")) op = OP_FOLD;
            else if (!strcmp(name, "slew")) op = OP_SLEW;
            else if (!strcmp(name, "sah")) op = OP_SAH;
            else if (!strcmp(name, "probe")) op = OP_PROBE;
            else if (!strcmp(name, "get")) op = OP_GET;
            else if (!strcmp(name, "get_wrap")) op = OP_GET_WRAP;
            else if (!strcmp(name, "set")) op = OP_SET;
            else if (!strcmp(name, "count")) op = OP_COUNT;
            else if (!strcmp(name, "len")) op = OP_LEN;
            else if (!strcmp(name, "floor")) op = OP_FLOOR;
            else if (!strcmp(name, "ceil")) op = OP_CEIL;
            else if (!strcmp(name, "abs")) op = OP_ABS;
            else if (!strcmp(name, "sign")) op = OP_SIGN;
            else if (!strcmp(name, "mtof")) op = OP_MTOF;
            else if (!strcmp(name, "lerp")) op = OP_LERP;
            else if (!strcmp(name, "logistic")) op = OP_LOGISTIC;
            else if (!strcmp(name, "henon")) op = OP_HENON;
            else if (!strcmp(name, "wolfram")) op = OP_WOLFRAM;
            else if (!strcmp(name, "wave")) op = OP_WAVE;

            if (op == -1) cr_error(ctx, "Unknown function");
            
            idx = alloc_node(ctx, op);
            int ac = 0;
            if (strcmp(token, ")") != 0) {
                while(1) {
                    if (ac >= CR_MAX_ARGS) cr_error(ctx, "Too many args");
                    ctx->node_pool[idx].inputs[ac++] = expr(ctx);
                    if (strcmp(token, ",") == 0) get_tok(ctx);
                    else break;
                }
            }
            ctx->node_pool[idx].input_count = ac;
            match(ctx, ")");
            
            if (op == OP_SINE) {
                int f = try_fold_const(ctx, op, ctx->node_pool[idx].inputs[0], -1);
                if (f != -1) idx = f;
            }
            
        } else {
            if (!strcmp(name, "time")) idx = alloc_node(ctx, OP_TIME);
            else {
                idx = find_var_node(ctx, name);
                if (idx == -1) {
                    char err[64]; sprintf(err, "Unknown var '%s'", name);
                    cr_error(ctx, err);
                }
            }
        }
    } else if (!strcmp(token, "(")) {
        get_tok(ctx);
        idx = expr(ctx);
        match(ctx, ")");
    } else if (!strcmp(token, "!")) {
        get_tok(ctx);
        int r = factor(ctx);
        int node = alloc_node(ctx, OP_NOT);
        ctx->node_pool[node].inputs[0] = r;
        ctx->node_pool[node].input_count = 1;
        idx = node;
    } else if (!strcmp(token, "-")) {
         get_tok(ctx);
         int r = factor(ctx);
         int node = alloc_node(ctx, OP_SUB);
         int zero = alloc_node(ctx, OP_CONST);
         ctx->node_pool[zero].value = 0.0;
         ctx->node_pool[zero].is_constant = 1;
         ctx->node_pool[node].inputs[0] = zero;
         ctx->node_pool[node].inputs[1] = r;
         ctx->node_pool[node].input_count = 2;
         idx = node;
    } else {
        cr_error(ctx, "Syntax Error");
    }
    return idx;
}

static int power(cr_context *ctx) {
    int n = factor(ctx);
    while (!strcmp(token, "^")) {
        get_tok(ctx);
        int r = factor(ctx);
        int node = alloc_node(ctx, OP_POW);
        ctx->node_pool[node].inputs[0] = n;
        ctx->node_pool[node].inputs[1] = r;
        ctx->node_pool[node].input_count = 2;
        n = node;
    }
    return n;
}

static int term(cr_context *ctx) {
    int n = power(ctx);
    while (!strcmp(token, "*") || !strcmp(token, "/") || !strcmp(token, "%")) {
        char op_char = token[0];
        get_tok(ctx);
        int r = power(ctx);
        
        int op = OP_MUL;
        if(op_char == '/') op = OP_DIV;
        if(op_char == '%') op = OP_MOD;

        int folded = try_fold_const(ctx, op, n, r);
        if (folded != -1) {
            n = folded;
        } else {
            int node = alloc_node(ctx, op);
            ctx->node_pool[node].inputs[0] = n;
            ctx->node_pool[node].inputs[1] = r;
            ctx->node_pool[node].input_count = 2;
            n = node;
        }
    }
    return n;
}

static int sum(cr_context *ctx) {
    int n = term(ctx);
    while (!strcmp(token, "+") || !strcmp(token, "-")) {
        int is_add = !strcmp(token, "+");
        get_tok(ctx);
        int r = term(ctx);

        int folded = try_fold_const(ctx, is_add ? OP_ADD : OP_SUB, n, r);
        if (folded != -1) {
            n = folded;
        } else {
            int op = alloc_node(ctx, is_add ? OP_ADD : OP_SUB);
            ctx->node_pool[op].inputs[0] = n;
            ctx->node_pool[op].inputs[1] = r;
            ctx->node_pool[op].input_count = 2;
            n = op;
        }
    }
    return n;
}

static int relation(cr_context *ctx) {
    int n = sum(ctx);
    while (!strcmp(token, ">") || !strcmp(token, "<") || !strcmp(token, ">=") || 
           !strcmp(token, "<=") || !strcmp(token, "==") || !strcmp(token, "!=")) {
        char op_str[4]; strcpy(op_str, token);
        get_tok(ctx);
        int r = sum(ctx);
        int op = 0;
        if(!strcmp(op_str, ">")) op = OP_GT;
        if(!strcmp(op_str, "<")) op = OP_LT;
        if(!strcmp(op_str, ">=")) op = OP_GE;
        if(!strcmp(op_str, "<=")) op = OP_LE;
        if(!strcmp(op_str, "==")) op = OP_EQ;
        if(!strcmp(op_str, "!=")) op = OP_NE;
        
        int node = alloc_node(ctx, op);
        ctx->node_pool[node].inputs[0] = n;
        ctx->node_pool[node].inputs[1] = r;
        ctx->node_pool[node].input_count = 2;
        n = node;
    }
    return n;
}

static int logic_and(cr_context *ctx) {
    int n = relation(ctx);
    while (!strcmp(token, "&&")) {
        get_tok(ctx);
        int r = relation(ctx);
        int node = alloc_node(ctx, OP_AND);
        ctx->node_pool[node].inputs[0] = n;
        ctx->node_pool[node].inputs[1] = r;
        ctx->node_pool[node].input_count = 2;
        n = node;
    }
    return n;
}

static int logic_or(cr_context *ctx) {
    int n = logic_and(ctx);
    while (!strcmp(token, "||")) {
        get_tok(ctx);
        int r = logic_and(ctx);
        int node = alloc_node(ctx, OP_OR);
        ctx->node_pool[node].inputs[0] = n;
        ctx->node_pool[node].inputs[1] = r;
        ctx->node_pool[node].input_count = 2;
        n = node;
    }
    return n;
}

static int expr(cr_context *ctx) {
    return logic_or(ctx);
}

static void statement(cr_context *ctx) {
    if (strcmp(token, "def") == 0) {
        if (ctx->scope[0] != 0) cr_error(ctx, "Nested defs not allowed");
        get_tok(ctx); 
        if (token_type != 1) cr_error(ctx, "Expected macro name");
        if (ctx->macro_count >= CR_MAX_MACROS) cr_error(ctx, "Max macros limit reached");
        cr_macro *m = &ctx->macros[ctx->macro_count++];
        strcpy(m->name, token);
        get_tok(ctx);
        match(ctx, "(");
        m->arg_count = 0;
        if (strcmp(token, ")") != 0) {
            while(1) {
                if (token_type != 1) cr_error(ctx, "Expected arg name");
                strcpy(m->args[m->arg_count++], token);
                get_tok(ctx);
                if (strcmp(token, ",") == 0) get_tok(ctx);
                else break;
            }
        }
        match(ctx, ")");
        
        if (strcmp(token, "{") != 0) cr_error(ctx, "Expected '{'");
        
        int brace = 1;
        int pos = 0;
        while(brace > 0 && *token_ptr) {
            char c = *token_ptr++;
            if (c == '{') brace++;
            if (c == '}') brace--;
            if (c == '\n') ctx->current_line++;
            if (brace > 0) {
                if (pos < CR_MAX_MACRO_SIZE-1) m->body[pos++] = c;
            }
        }
        m->body[pos] = 0;
        get_tok(ctx); 
        return;
    }

    if (strcmp(token, "return") == 0) {
        get_tok(ctx);
        ctx->return_reg = expr(ctx);
        ctx->returning = 1;
        return;
    }
    
    if (strcmp(token, "inspect") == 0) {
        get_tok(ctx); match(ctx, "("); expr(ctx); match(ctx, ")"); 
        return;
    }
  
    if (token_type == 1) {
        char name[32];
        strcpy(name, token);
        get_tok(ctx);
        match(ctx, "=");
        int val = expr(ctx);
        
        if (!strcmp(name, "out") || !strcmp(name, "output.0")) {
            ctx->output_nodes[0] = val;
        } else if (!strcmp(name, "output.1")) {
            ctx->output_nodes[1] = val;
        } else {
            set_var_node(ctx, name, val);
        }
    } else {
        cr_error(ctx, "Statement expected");
    }
}

static int visited[CR_MAX_NODES];
static void topo_visit(cr_context *ctx, int u) {
    visited[u] = 1; 
    int i;
    for (i = 0; i < ctx->node_pool[u].input_count; i++) {
        int v = ctx->node_pool[u].inputs[i];
        if (!visited[v]) topo_visit(ctx, v);
    }
    ctx->exec_order[ctx->exec_count++] = u;
}

static void build_exec_list(cr_context *ctx) {
    memset(visited, 0, sizeof(visited));
    ctx->exec_count = 0;
    int i;
    for(i=0; i<CR_MAX_CHANNELS; i++) {
        if(ctx->output_nodes[i] != -1)
            topo_visit(ctx, ctx->output_nodes[i]);
    }
}

CR_API cr_context *cr_create_context(void) {
  size_t full_size = sizeof(cr_context) + CR_ARENA_SIZE;
  void *block = calloc(1, full_size);
  if (!block) return NULL;
  
  cr_context *ctx = (cr_context *)block;
  ctx->arena_base = (unsigned char*)block + sizeof(cr_context);
  ctx->arena_size = CR_ARENA_SIZE;
  ctx->arena_top = 0;

#if defined(_WIN32)
  InitializeCriticalSection(&ctx->lock);
#else
  pthread_mutex_init(&ctx->lock, NULL);
#endif
  ctx->log_level = CR_LOG_WARN; 
  ctx->current_line = 1;
  ctx->output_nodes[0] = -1;
  ctx->output_nodes[1] = -1;
  return ctx;
}

CR_API void cr_destroy_context(cr_context *ctx) {
  free(ctx); // Frees the entire block (Context + Arena) at once
}

CR_API void cr_set_log_level(cr_context *ctx, int level) {
    ctx->log_level = level;
}

CR_API void cr_lock(cr_context *ctx) {
#if defined(_WIN32)
  EnterCriticalSection(&ctx->lock);
#else
  pthread_mutex_lock(&ctx->lock);
#endif
}

CR_API void cr_unlock(cr_context *ctx) {
#if defined(_WIN32)
  LeaveCriticalSection(&ctx->lock);
#else
  pthread_mutex_unlock(&ctx->lock);
#endif
}

CR_API double cr_process(cr_context *ctx, int channel) {
    if (channel == 0) { 
        int i;
        for(i=0; i<ctx->exec_count; i++) {
            run_node(ctx, ctx->exec_order[i]);
        }
    }
    if (ctx->output_nodes[channel] == -1) return 0.0;
    return ctx->node_pool[ctx->output_nodes[channel]].value;
}

CR_API int cr_run(cr_context *ctx, const char *script) {
  if (setjmp(ctx->err_jmp) != 0) return 0;
  
  ctx->node_idx = 0;
  ctx->var_count = 0;
  ctx->output_nodes[0] = -1;
  ctx->output_nodes[1] = -1;
  ctx->arena_top = 0; // Reset Arena on reload
  
  token_ptr = (char *)script;
  ctx->current_line = 1; 
  get_tok(ctx);
  
  while (token_type != 0) {
    statement(ctx);
  }
  
  build_exec_list(ctx);
  cr_log(ctx, CR_LOG_INFO, "Compiled: %d nodes active in VM", ctx->exec_count);
  
  return 1;
}

#endif
#endif

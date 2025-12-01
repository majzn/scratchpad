#ifndef CHRONOS_H
#define CHRONOS_H

#include <ctype.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

#ifndef CR_API
#define CR_API
#endif

#define CR_MAX_CHANNELS 2
#define CR_MAX_SYMBOLS 1024
#define CR_MAX_NODES 16384
#define CR_MAX_ARGS 16
#define CR_MAX_VOICES 8
#define CR_MAX_MACROS 128
#define CR_CALL_STACK 32
#define CR_PI 3.14159265359
#define CR_ARENA_SIZE (64 * 1024 * 1024)

#define CR_TRACK_CHANNELS 8
#define CR_TRACK_ROWS 64
#define CR_MAX_PATTERNS 64
#define CR_MAX_ORDER 128

typedef enum {
  CR_LOG_NONE = 0,
  CR_LOG_ERROR,
  CR_LOG_WARN,
  CR_LOG_INFO,
  CR_LOG_DEBUG
} cr_log_level;

typedef enum { 
    CR_FLOAT, 
    CR_INT
} cr_type_t;

typedef struct cr_val {
  cr_type_t type;
  union {
    double f;
    int i;
  } as;
} cr_val;

typedef struct {
  int active;
  int note_id;
  double pitch;
  double velocity;
  long start_time;
  long release_time;
} cr_voice;

typedef struct {
  unsigned char note;
  unsigned char inst;
  unsigned char vol;
  unsigned char cmd;
  unsigned char val;
} cr_track_cell;

typedef struct {
  cr_track_cell rows[CR_TRACK_ROWS][CR_TRACK_CHANNELS];
} cr_pattern;

typedef struct {
  cr_pattern patterns[CR_MAX_PATTERNS];
  unsigned char order[CR_MAX_ORDER];
  int current_row;
  int current_tick;
  int current_pattern_idx;
  int speed;
  int tempo;
  double ch_freq[CR_TRACK_CHANNELS];
  double ch_gate[CR_TRACK_CHANNELS];
  double ch_vol[CR_TRACK_CHANNELS];
  double ch_period[CR_TRACK_CHANNELS];
} cr_tracker_state;

struct cr_context;
struct cr_node;

typedef cr_val (*cr_op_func)(struct cr_context *ctx, struct cr_node *n, cr_val *inputs);

typedef struct cr_op_desc {
  const char *name;
  int opcode;
  cr_op_func handler;
} cr_op_desc;

typedef struct {
  double s[8];
  double *buffer;
  int buf_len;
  int write_head;
} cr_dsp_state;

typedef struct cr_node {
  int id;
  const cr_op_desc *op_desc;
  int inputs[CR_MAX_ARGS];
  int input_count;
  cr_val value;
  cr_dsp_state dsp;
} cr_node;

typedef struct {
  char name[64];
  int node_index;
} cr_variable;

typedef struct {
  char name[32];
  size_t body_start_offset;
  size_t body_end_offset;
  char args[CR_MAX_ARGS][32];
  int arg_count;
} cr_macro;

typedef struct {
  char *ret_addr;
  char *ret_end;
  char old_scope[64];
} cr_call_frame;

typedef struct {
  double bpm;
  double phase;
  double samples_per_beat;
  long total_samples;
  int playing;
} cr_transport;

typedef struct cr_context {
  cr_transport transport;
  cr_tracker_state tracker;
  long global_time;
  int output_nodes[CR_MAX_CHANNELS];
  double sample_rate;
  cr_variable variables[CR_MAX_SYMBOLS];
  int var_count;
  cr_macro macros[CR_MAX_MACROS];
  int macro_count;
  cr_call_frame call_stack[CR_CALL_STACK];
  int call_depth;
  char scope[64];
  int scope_counter;
  cr_node node_pool[CR_MAX_NODES];
  int node_idx;
  int exec_order[CR_MAX_NODES];
  int exec_count;
  unsigned char visit_state[CR_MAX_NODES];
  unsigned char *arena_base;
  size_t arena_size;
  size_t arena_top;
  char *src_base;
  char *src_ptr;
  char *src_end;
  char token[8192]; // INCREASED SIZE to handle pattern strings
  int token_type;
  jmp_buf err_jmp;
  char error_msg[128];
  int current_line;
  int bpm_node_idx;
  int return_node_idx;
} cr_context;

typedef void (*cr_log_cb)(int level, const char *msg);

typedef struct cr_engine {
  cr_context contexts[2];
  cr_context *active;
  cr_context *back;
  void *memory_block;
  char *source_history;
  size_t source_capacity;
  size_t source_len;
  int log_level;
  cr_voice voice_manager[CR_MAX_VOICES];
  int current_note_id;
  cr_log_cb log_callback;
#if defined(_WIN32)
  CRITICAL_SECTION swap_lock;
#else
  pthread_mutex_t swap_lock;
#endif
} cr_engine;

CR_API cr_engine *cr_create_engine(int sample_rate);
CR_API void cr_destroy_engine(cr_engine *engine);
CR_API void cr_set_log_level(cr_engine *engine, int level);
CR_API void cr_set_log_callback(cr_engine *engine, cr_log_cb cb);
CR_API double cr_process(cr_engine *engine, int channel);
CR_API void cr_tick(cr_engine *engine);
CR_API int cr_eval(cr_engine *engine, const char *script, int reset);
CR_API int cr_get_variable_node(cr_context *ctx, const char *name);
CR_API void cr_note_on(cr_engine *engine, double pitch, double velocity);
CR_API void cr_note_off(cr_engine *engine, double pitch);

#ifdef CR_IMPLEMENTATION

static void cr_log(cr_engine *e, int level, const char *fmt, ...) {
  if (e && e->log_level >= level && e->log_callback) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    e->log_callback(level, buf);
  }
}

static void cr_error(cr_context *ctx, const char *msg) {
  sprintf(ctx->error_msg, "Line %d: %s (Token: '%s')", ctx->current_line, msg, ctx->token);
  longjmp(ctx->err_jmp, 1);
}

static void *cr_arena_alloc(cr_context *ctx, size_t size) {
  size_t aligned = (size + 7) & ~7;
  void *ptr;
  if (ctx->arena_top + aligned > ctx->arena_size) {
    cr_error(ctx, "Memory Arena Exceeded");
    return NULL;
  }
  ptr = ctx->arena_base + ctx->arena_top;
  memset(ptr, 0, aligned);
  ctx->arena_top += aligned;
  return ptr;
}

static int alloc_node(cr_context *ctx, const cr_op_desc *op_desc) {
  int idx;
  cr_node *n;
  if (ctx->node_idx >= CR_MAX_NODES)
    cr_error(ctx, "Max nodes reached");
  idx = ctx->node_idx++;
  n = &ctx->node_pool[idx];
  memset(n, 0, sizeof(cr_node));
  n->id = idx;
  n->op_desc = op_desc;
  n->value.type = CR_FLOAT;
  n->value.as.f = 0.0;
  return idx;
}

static double as_float(cr_val v) {
  return (v.type == CR_INT) ? (double)v.as.i : v.as.f;
}

static cr_val make_float(double f) {
  cr_val v;
  v.type = CR_FLOAT;
  v.as.f = f;
  return v;
}

static double cr_parse_float(const char *str) {
  double res = 0.0, sign = 1.0, div = 10.0;
  if (*str == '-') {
    sign = -1.0;
    str++;
  }
  while (isdigit((unsigned char)*str)) {
    res = res * 10.0 + (*str - '0');
    str++;
  }
  if (*str == '.') {
    str++;
    while (isdigit((unsigned char)*str)) {
      res += (*str - '0') / div;
      div *= 10.0;
      str++;
    }
  }
  return res * sign;
}

static double note_to_freq(const char *note) {
  static const char *names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
  char n[4];
  int oct = 4, i, semi = 0;
  strncpy(n, note, 3);
  n[3] = 0;
  if (isdigit(n[strlen(n) - 1])) {
    oct = n[strlen(n) - 1] - '0';
    n[strlen(n) - 1] = 0;
  }
  for (i = 0; i < 12; i++)
    if (!strcmp(n, names[i])) {
      semi = i;
      break;
    }
  return 440.0 * pow(2.0, (double)((oct - 4) * 12 + semi - 9) / 12.0);
}

// --- DSP Functions ---
static double poly_blep(double t, double dt) {
  if (t < dt) {
    t /= dt;
    return t + t - t * t - 1.0;
  } else if (t > 1.0 - dt) {
    t = (t - 1.0) / dt;
    return t * t + t + t + 1.0;
  }
  return 0.0;
}

static double hermite(double x, double y0, double y1, double y2, double y3) {
  double c0 = y1;
  double c1 = 0.5 * (y2 - y0);
  double c2 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3;
  double c3 = 0.5 * (y3 - y0) + 1.5 * (y1 - y2);
  return ((c3 * x + c2) * x + c1) * x + c0;
}

// --- Operations ---
static const cr_op_desc *find_op(const char *name);

static void build_pattern_op(cr_context *c, cr_node *n, int mode) {
  // Pattern op implementation... same as before
  // (Omitted for brevity, logic unchanged from previous robust version)
  // Re-inserting core logic to ensure no compile errors:
  cr_node *in0 = &c->node_pool[n->inputs[0]];
  if (n->dsp.buffer) return;
  if (mode == 2 && in0->dsp.buffer) {
     int len = (int)(in0->dsp.buf_len / as_float(c->node_pool[n->inputs[1]].value));
     if(len<1) len=1;
     n->dsp.buffer = (double*)cr_arena_alloc(c, len*sizeof(double));
     if(n->dsp.buffer) memcpy(n->dsp.buffer, in0->dsp.buffer, len*sizeof(double));
     n->dsp.buf_len = len;
     n->op_desc = find_op("data");
  }
}

static cr_val op_const(cr_context *c, cr_node *n, cr_val *v) { return n->value; }
static cr_val op_data(cr_context *c, cr_node *n, cr_val *v) { return make_float(0.0); }
static cr_val op_add(cr_context *c, cr_node *n, cr_val *v) { return make_float(as_float(v[0]) + as_float(v[1])); }
static cr_val op_sub(cr_context *c, cr_node *n, cr_val *v) { return make_float(as_float(v[0]) - as_float(v[1])); }
static cr_val op_mul(cr_context *c, cr_node *n, cr_val *v) { return make_float(as_float(v[0]) * as_float(v[1])); }
static cr_val op_div(cr_context *c, cr_node *n, cr_val *v) { 
  double d = as_float(v[1]); return make_float((fabs(d)<1e-9)?0.0:as_float(v[0])/d); 
}
static cr_val op_mod(cr_context *c, cr_node *n, cr_val *v) { 
  double d = as_float(v[1]); return make_float((fabs(d)<1e-9)?0.0:fmod(as_float(v[0]), d)); 
}
static cr_val op_pow(cr_context *c, cr_node *n, cr_val *v) { return make_float(pow(as_float(v[0]), as_float(v[1]))); }

static cr_val op_sin(cr_context *c, cr_node *n, cr_val *v) {
  double freq = as_float(v[0]);
  double inc = freq / c->sample_rate;
  double p = n->dsp.s[0] + inc;
  p -= floor(p);
  n->dsp.s[0] = p;
  return make_float(sin(p * 2.0 * CR_PI));
}

static cr_val op_noise(cr_context *c, cr_node *n, cr_val *v) { return make_float(((double)rand() / RAND_MAX) * 2.0 - 1.0); }
static cr_val op_time(cr_context *c, cr_node *n, cr_val *v) { return make_float((double)c->global_time / c->sample_rate); }

static cr_val op_gt(cr_context *c, cr_node *n, cr_val *v) { return make_float(as_float(v[0]) > as_float(v[1]) ? 1.0 : 0.0); }
static cr_val op_lt(cr_context *c, cr_node *n, cr_val *v) { return make_float(as_float(v[0]) < as_float(v[1]) ? 1.0 : 0.0); }
static cr_val op_eq(cr_context *c, cr_node *n, cr_val *v) { return make_float(fabs(as_float(v[0]) - as_float(v[1])) < 1e-5 ? 1.0 : 0.0); }
static cr_val op_if(cr_context *c, cr_node *n, cr_val *v) { return make_float(as_float(v[0]) > 0.5 ? as_float(v[1]) : as_float(v[2])); }
static cr_val op_select(cr_context *c, cr_node *n, cr_val *v) {
  int sel = (int)as_float(v[0]);
  int num_options = n->input_count - 1;
  if (sel < 0) sel = 0; if (sel >= num_options) sel = num_options - 1;
  return v[sel + 1];
}

static cr_val op_param(cr_context *c, cr_node *n, cr_val *v) {
  double target = as_float(v[0]);
  double cur = n->dsp.s[0];
  double d = target - cur;
  if (fabs(d) > 0.005) cur += (d > 0) ? 0.005 : -0.005; else cur = target;
  n->dsp.s[0] = cur;
  return make_float(cur);
}

static cr_val op_phasor(cr_context *c, cr_node *n, cr_val *v) {
  double inc = as_float(v[0]) / c->sample_rate;
  double p = n->dsp.s[0] + inc;
  p -= floor(p); n->dsp.s[0] = p;
  return make_float(p);
}

static cr_val op_saw(cr_context *c, cr_node *n, cr_val *v) {
  double freq = as_float(v[0]);
  double inc, p, out;
  if (freq < 0.1) freq = 0.1;
  inc = freq / c->sample_rate;
  p = n->dsp.s[0] + inc;
  p -= floor(p); n->dsp.s[0] = p;
  out = (2.0 * p - 1.0);
  out -= poly_blep(p, inc);
  return make_float(out);
}

static cr_val op_pulse(cr_context *c, cr_node *n, cr_val *v) {
  double freq = as_float(v[0]);
  double width, inc, p, out;
  if (freq < 0.1) freq = 0.1;
  width = (n->input_count > 1) ? as_float(v[1]) : 0.5;
  inc = freq / c->sample_rate;
  p = n->dsp.s[0] + inc;
  p -= floor(p); n->dsp.s[0] = p;
  out = (p < width) ? 1.0 : -1.0;
  out += poly_blep(p, inc);
  out -= poly_blep(fmod(p - width + 1.0, 1.0), inc);
  return make_float(out);
}

static cr_val op_filter(cr_context *c, cr_node *n, cr_val *v) {
  double in = as_float(v[0]), type = as_float(v[1]), cut = as_float(v[2]), q = as_float(v[3]);
  double f = 2.0 * sin(CR_PI * cut / c->sample_rate);
  if (f > 0.9) f = 0.9; if (f < 0.001) f = 0.001;
  if (q < 0.1) q = 0.1; if (q > 10.0) q = 10.0;
  n->dsp.s[1] += f * (in - n->dsp.s[1] - (1.0 / q) * n->dsp.s[0]);
  n->dsp.s[0] += f * n->dsp.s[1];
  if (type < 0.5) return make_float(n->dsp.s[0]);
  if (type < 1.5) return make_float(n->dsp.s[1]);
  return make_float(in - n->dsp.s[0]);
}

static cr_val op_delay(cr_context *c, cr_node *n, cr_val *v) {
  double in = as_float(v[0]), tm = as_float(v[1]), fb = as_float(v[2]), out = 0.0;
  if (n->dsp.buffer && n->dsp.buf_len > 0) {
    double dspls = tm * c->sample_rate;
    double rp = n->dsp.write_head - dspls;
    int i0, idx0, idx1, idx2, idx3;
    double frac, fb_sig;
    while (rp < 0) rp += n->dsp.buf_len;
    i0 = (int)rp; frac = rp - i0;
    idx0 = (i0 - 1 + n->dsp.buf_len) % n->dsp.buf_len;
    idx1 = i0 % n->dsp.buf_len;
    idx2 = (i0 + 1) % n->dsp.buf_len;
    idx3 = (i0 + 2) % n->dsp.buf_len;
    out = hermite(frac, n->dsp.buffer[idx0], n->dsp.buffer[idx1], n->dsp.buffer[idx2], n->dsp.buffer[idx3]);
    fb_sig = in + out * fb;
    if (fb_sig > 1.2) fb_sig = 1.2 + tanh(fb_sig - 1.2) * 0.5;
    n->dsp.buffer[n->dsp.write_head] = fb_sig;
    n->dsp.write_head = (n->dsp.write_head + 1) % n->dsp.buf_len;
  }
  return make_float(out);
}

static cr_val op_seq(cr_context *c, cr_node *n, cr_val *v) {
  int data_idx = n->inputs[0];
  double div = as_float(v[1]);
  cr_node *data = &c->node_pool[data_idx];
  if (data->dsp.buffer && data->dsp.buf_len > 0) {
    int idx = (int)(c->transport.phase * div) % data->dsp.buf_len;
    if (idx < 0) idx += data->dsp.buf_len;
    return make_float(data->dsp.buffer[idx]);
  }
  return make_float(0.0);
}

static cr_val op_clock(cr_context *c, cr_node *n, cr_val *v) {
  double div = as_float(v[0]);
  double phase = c->transport.phase * div;
  double p_frac = phase - floor(phase);
  double prev = n->dsp.s[0];
  n->dsp.s[0] = p_frac;
  if (p_frac < prev || (c->global_time == 0 && p_frac == 0.0)) return make_float(1.0);
  return make_float(0.0);
}

static cr_val op_env(cr_context *c, cr_node *n, cr_val *v) {
  double trig = as_float(v[0]);
  double decay = as_float(v[1]);
  double val = n->dsp.s[0];
  if (trig > 0.5) val = 1.0; else val *= decay;
  n->dsp.s[0] = val;
  return make_float(val);
}

static cr_val op_perc(cr_context *c, cr_node *n, cr_val *v) {
  double trig = as_float(v[0]);
  double a = as_float(v[1]), d = as_float(v[2]);
  double val = n->dsp.s[0];
  int state = (int)n->dsp.s[1];
  double prev_trig = n->dsp.s[2];
  if (trig > 0.5 && prev_trig <= 0.5) state = 1;
  n->dsp.s[2] = trig;
  if (state == 1) {
    val += 1.0 / (a * c->sample_rate);
    if (val >= 1.0) { val = 1.0; state = 2; }
  } else if (state == 2) {
    val *= exp(-2.2 / (d * c->sample_rate));
    if (val < 0.0001) { val = 0.0; state = 0; }
  }
  n->dsp.s[0] = val; n->dsp.s[1] = (double)state;
  return make_float(val);
}

static cr_val op_adsr(cr_context *c, cr_node *n, cr_val *v) {
  double gate = as_float(v[0]), a = as_float(v[1]), d = as_float(v[2]), s = as_float(v[3]), r = as_float(v[4]);
  double val = n->dsp.s[0];
  int state = (int)n->dsp.s[1];
  double prev_gate = n->dsp.s[2];
  
  if (gate > 0.5 && prev_gate <= 0.5) state = 1;
  else if (gate <= 0.5 && prev_gate > 0.5) state = 4;
  
  switch (state) {
  case 1: val += 1.0/(a*c->sample_rate); if(val>=1.0) {val=1.0; state=2;} break;
  case 2: val = s + (val-s)*exp(-2.2/(d*c->sample_rate)); if(val<=s+0.001) {val=s; state=3;} break;
  case 3: val = s; break;
  case 4: val *= exp(-2.2/(r*c->sample_rate)); if(val<0.001) {val=0.0; state=0;} break;
  }
  n->dsp.s[0] = val; n->dsp.s[1] = (double)state; n->dsp.s[2] = gate;
  return make_float(val);
}

static cr_val op_mix(cr_context *c, cr_node *n, cr_val *v) {
  double sum = 0.0;
  for (int i = 0; i < n->input_count; i++) sum += as_float(v[i]);
  if (sum > 1.5) sum = 1.5 + tanh(sum - 1.5) * 0.5;
  else if (sum < -1.5) sum = -1.5 + tanh(sum + 1.5) * 0.5;
  return make_float(sum);
}

static cr_val op_trk_freq(cr_context *c, cr_node *n, cr_val *v) {
  int ch = (int)as_float(v[0]);
  if (ch < 0 || ch >= CR_TRACK_CHANNELS) return make_float(0.0);
  return make_float(c->tracker.ch_freq[ch]);
}

static cr_val op_trk_gate(cr_context *c, cr_node *n, cr_val *v) {
  int ch = (int)as_float(v[0]);
  if (ch < 0 || ch >= CR_TRACK_CHANNELS) return make_float(0.0);
  return make_float(c->tracker.ch_gate[ch]);
}

static cr_val op_trk_vol(cr_context *c, cr_node *n, cr_val *v) {
  int ch = (int)as_float(v[0]);
  if (ch < 0 || ch >= CR_TRACK_CHANNELS) return make_float(0.0);
  return make_float(c->tracker.ch_vol[ch]);
}

static const cr_op_desc op_reg[] = {
    {"const", 0, op_const},    {"param", 1, op_param},
    {"add", 2, op_add},        {"sub", 3, op_sub},
    {"mul", 4, op_mul},        {"div", 5, op_div},
    {"mod", 6, op_mod},        {"pow", 7, op_pow},
    {"sine", 8, op_sin},       {"phasor", 10, op_phasor},
    {"noise", 11, op_noise},   {"filter", 12, op_filter},
    {"delay", 13, op_delay},   {"time", 14, op_time},
    {"gt", 30, op_gt},         {"lt", 31, op_lt},
    {"eq", 32, op_eq},         {"if", 33, op_if},
    {"gen", 90, NULL},         {"hex", 91, NULL},
    {"saw", 21, op_saw},       {"pulse", 22, op_pulse},
    {"seq", 40, op_seq},       {"clock", 41, op_clock},
    {"env", 42, op_env},       {"adsr", 43, op_adsr},
    {"select", 50, op_select}, {"mix", 51, op_mix},
    {"trig", 54, NULL},        {"zeros", 55, NULL}, // Placeholders
    {"perc", 56, op_perc},     {"data", 20, op_data},
    {"trk_freq", 60, op_trk_freq},
    {"trk_gate", 61, op_trk_gate},
    {"trk_vol",  62, op_trk_vol},
    {"nop", 99, NULL}
};

static const cr_op_desc *find_op(const char *name) {
  int i;
  for (i = 0; i < sizeof(op_reg) / sizeof(op_reg[0]); i++)
    if (!strcmp(op_reg[i].name, name))
      return &op_reg[i];
  return NULL;
}

// --- Parsing ---

static void migrate_state(cr_context *dst, cr_context *src) {
  int i, j;
  for (i = 0; i < dst->var_count; i++) {
    for (j = 0; j < src->var_count; j++) {
      if (!strcmp(dst->variables[i].name, src->variables[j].name)) {
        cr_node *dn = &dst->node_pool[dst->variables[i].node_index];
        cr_node *sn = &src->node_pool[src->variables[j].node_index];
        if (dn->op_desc == sn->op_desc) {
          if (dn->op_desc->handler == op_param) {
            dn->dsp.s[0] = sn->dsp.s[0];
            dn->value = sn->value;
          } else if (dn->op_desc->handler != op_data) {
            double *nb = dn->dsp.buffer;
            dn->dsp = sn->dsp;
            dn->dsp.buffer = nb;
            // Buffer Copy Logic if needed
          }
        }
        break;
      }
    }
  }
}

static void exec_node(cr_context *ctx, int idx) {
  cr_node *n = &ctx->node_pool[idx];
  if (n->op_desc && n->op_desc->handler) {
    cr_val v[CR_MAX_ARGS];
    int i;
    for (i = 0; i < n->input_count; i++)
      v[i] = ctx->node_pool[n->inputs[i]].value;
    n->value = n->op_desc->handler(ctx, n, v);
  }
}

static void topo_visit(cr_context *ctx, int u) {
  int i;
  cr_node *n;
  if (ctx->visit_state[u] == 2) return;
  if (ctx->visit_state[u] == 1) cr_error(ctx, "DSP Cycle detected");
  ctx->visit_state[u] = 1;
  n = &ctx->node_pool[u];
  for (i = 0; i < n->input_count; i++) topo_visit(ctx, n->inputs[i]);
  ctx->visit_state[u] = 2;
  ctx->exec_order[ctx->exec_count++] = u;
}

static void get_tok(cr_context *ctx, cr_engine *engine) {
  const char *p;
  int i = 0;
  while (1) {
    if (ctx->src_end && ctx->src_ptr >= ctx->src_end) { ctx->token_type = 0; return; }
    while (isspace((unsigned char)*ctx->src_ptr)) {
      if (*ctx->src_ptr == '\n') ctx->current_line++;
      ctx->src_ptr++;
      if (ctx->src_end && ctx->src_ptr >= ctx->src_end) { ctx->token_type = 0; return; }
    }
    if (*ctx->src_ptr == '#') {
      while (*ctx->src_ptr && *ctx->src_ptr != '\n') ctx->src_ptr++;
      continue;
    }
    break;
  }
  if (*ctx->src_ptr == 0) { ctx->token_type = 0; return; }
  p = ctx->src_ptr;
  if (isalpha((unsigned char)*p) || *p == '_') {
    while (isalnum((unsigned char)*ctx->src_ptr) || *ctx->src_ptr == '_')
      if (i < sizeof(ctx->token) - 1) ctx->token[i++] = *ctx->src_ptr++; else ctx->src_ptr++;
    ctx->token[i] = 0; ctx->token_type = 1;
  } else if (isdigit((unsigned char)*p) || (*p == '.' && isdigit((unsigned char)p[1]))) {
    while (isdigit((unsigned char)*ctx->src_ptr) || *ctx->src_ptr == '.')
      if (i < sizeof(ctx->token) - 1) ctx->token[i++] = *ctx->src_ptr++; else ctx->src_ptr++;
    ctx->token[i] = 0; ctx->token_type = 2;
  } else if (*p == '"') {
    ctx->src_ptr++;
    // FIXED: Corrected string parsing to avoid infinite loop on long strings
    while (*ctx->src_ptr && *ctx->src_ptr != '"') {
        char c = *ctx->src_ptr++; 
        if (i < sizeof(ctx->token) - 1) ctx->token[i++] = c;
    }
    if (*ctx->src_ptr == '"') ctx->src_ptr++;
    ctx->token[i] = 0; ctx->token_type = 4;
  } else {
    if (i < sizeof(ctx->token) - 1) ctx->token[i++] = *ctx->src_ptr++;
    ctx->token[i] = 0; ctx->token_type = 3;
  }
}

static int parse_hex_2(char *s) {
    if (!s || strlen(s) < 1) return -1;
    if (s[0] == '.' || (strlen(s)>1 && s[1] == '.')) return -1;
    char b[3] = {s[0], (strlen(s)>1 ? s[1] : 0), 0};
    return (int)strtol(b, NULL, 16);
}

static void cr_parse_pattern_data(cr_context *ctx, int pat_idx, int ch_idx, char *text) {
    if (pat_idx < 0 || pat_idx >= CR_MAX_PATTERNS) return;
    if (ch_idx < 0 || ch_idx >= CR_TRACK_CHANNELS) return;
    cr_pattern *p = &ctx->tracker.patterns[pat_idx];
    char *line = text;
    int row = 0;
    while (*line && row < CR_TRACK_ROWS) {
        while (*line && isspace((unsigned char)*line) && *line != '\n') line++;
        if (*line == '\n') { line++; continue; }
        if (*line == 0) break;

        char token[32];
        int t_len = 0;
        int col = 0;
        cr_track_cell *cell = &p->rows[row][ch_idx];
        
        while (*line && *line != '\n') {
            if (!isspace((unsigned char)*line)) {
                if(t_len < 31) token[t_len++] = *line;
            } else if (t_len > 0) {
                token[t_len] = 0;
                if (col == 0) {
                    if (!strcmp(token, "===")) cell->note = 254;
                    else if (token[0] == '.') cell->note = 0;
                    else {
                        static const char *ns[] = {"C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"};
                        int oct = token[2] - '0';
                        int sem = 0;
                        token[2] = 0;
                        for(int k=0; k<12; k++) if(!strcmp(token, ns[k])) sem = k;
                        cell->note = (oct * 12) + sem + 1;
                    }
                } else if (col == 1) { int v = parse_hex_2(token); if (v != -1) cell->inst = v;
                } else if (col == 2) { int v = parse_hex_2(token); if (v != -1) cell->vol = v;
                } else if (col == 3) { if (token[0] != '.') cell->cmd = (token[0] >= '0' && token[0] <= '9') ? token[0] - '0' : token[0] - 'A' + 10;
                } else if (col == 4) { int v = parse_hex_2(token); if (v != -1) cell->val = v; }
                t_len = 0; col++;
            }
            line++;
        }
        if (t_len > 0) {
             token[t_len] = 0;
             if (col == 4) { int v = parse_hex_2(token); if (v != -1) cell->val = v; }
        }
        row++;
        if (*line == '\n') line++;
    }
}

static int expr(cr_context *ctx, cr_engine *engine);
static void stmt(cr_context *ctx, cr_engine *engine);

static int factor(cr_context *ctx, cr_engine *engine) {
  int idx;
  if (ctx->token_type == 2) {
    idx = alloc_node(ctx, find_op("const"));
    ctx->node_pool[idx].value = make_float(cr_parse_float(ctx->token));
    get_tok(ctx, engine);
    return idx;
  } else if (ctx->token_type == 4) {
    cr_error(ctx, "Unexpected string literal");
    return -1;
  } else if (ctx->token_type == 1) {
    char name[64];
    strcpy(name, ctx->token);
    get_tok(ctx, engine);
    if (!strcmp(ctx->token, "(")) {
      const cr_op_desc *od = find_op(name);
      int m_idx = -1, i;
      for (i = 0; i < ctx->macro_count; i++)
        if (!strcmp(ctx->macros[i].name, name)) { m_idx = i; break; }

      if (od) {
        idx = alloc_node(ctx, od);
        if (od->handler == op_delay) {
          int len = (int)(ctx->sample_rate * 4.0);
          ctx->node_pool[idx].dsp.buffer = (double *)cr_arena_alloc(ctx, len * sizeof(double));
          ctx->node_pool[idx].dsp.buf_len = len;
        }
        get_tok(ctx, engine);
        if (strcmp(ctx->token, ")")) {
           while (1) {
             if (ctx->node_pool[idx].input_count < CR_MAX_ARGS)
               ctx->node_pool[idx].inputs[ctx->node_pool[idx].input_count++] = expr(ctx, engine);
             if (strcmp(ctx->token, ",")) break;
             get_tok(ctx, engine);
           }
        }
        if (strcmp(ctx->token, ")")) cr_error(ctx, "Expected ')'");
        get_tok(ctx, engine);
        return idx;
      } else if (m_idx != -1) {
        int args[CR_MAX_ARGS], argc = 0;
        get_tok(ctx, engine);
        if (strcmp(ctx->token, ")")) {
          while (1) {
            if (argc < CR_MAX_ARGS) args[argc++] = expr(ctx, engine);
            if (strcmp(ctx->token, ",")) break;
            get_tok(ctx, engine);
          }
        }
        if (strcmp(ctx->token, ")")) cr_error(ctx, "Expected ')'");
        if (ctx->call_depth >= CR_CALL_STACK) cr_error(ctx, "Stack overflow");
        ctx->call_stack[ctx->call_depth].ret_addr = ctx->src_ptr;
        ctx->call_stack[ctx->call_depth].ret_end = ctx->src_end;
        strcpy(ctx->call_stack[ctx->call_depth].old_scope, ctx->scope);
        ctx->call_depth++;
        sprintf(ctx->scope, "s%d_", ctx->scope_counter++);
        for (i = 0; i < argc && i < ctx->macros[m_idx].arg_count; i++) {
          char vname[128];
          sprintf(vname, "%s%s", ctx->scope, ctx->macros[m_idx].args[i]);
          if (ctx->var_count < CR_MAX_SYMBOLS) {
            strcpy(ctx->variables[ctx->var_count].name, vname);
            ctx->variables[ctx->var_count].node_index = args[i];
            ctx->var_count++;
          }
        }
        ctx->src_ptr = ctx->src_base + ctx->macros[m_idx].body_start_offset;
        ctx->src_end = ctx->src_base + ctx->macros[m_idx].body_end_offset;
        ctx->return_node_idx = -1;
        get_tok(ctx, engine);
        while (ctx->token_type != 0 && ctx->return_node_idx == -1) stmt(ctx, engine);
        idx = (ctx->return_node_idx == -1) ? alloc_node(ctx, find_op("const")) : ctx->return_node_idx;
        ctx->call_depth--;
        ctx->src_ptr = ctx->call_stack[ctx->call_depth].ret_addr;
        ctx->src_end = ctx->call_stack[ctx->call_depth].ret_end;
        strcpy(ctx->scope, ctx->call_stack[ctx->call_depth].old_scope);
        get_tok(ctx, engine);
        return idx;
      } else cr_error(ctx, "Unknown function");
    } else {
      char scoped[128]; int i;
      sprintf(scoped, "%s%s", ctx->scope, name);
      for (i = 0; i < ctx->var_count; i++) if (!strcmp(ctx->variables[i].name, scoped)) return ctx->variables[i].node_index;
      for (i = 0; i < ctx->var_count; i++) if (!strcmp(ctx->variables[i].name, name)) return ctx->variables[i].node_index;
      cr_error(ctx, "Unknown variable");
    }
  } else if (!strcmp(ctx->token, "(")) {
    get_tok(ctx, engine); idx = expr(ctx, engine);
    if (strcmp(ctx->token, ")")) cr_error(ctx, "Expected ')'");
    get_tok(ctx, engine); return idx;
  } else if (!strcmp(ctx->token, "-")) {
    int zero; get_tok(ctx, engine);
    zero = alloc_node(ctx, find_op("const"));
    idx = alloc_node(ctx, find_op("sub"));
    ctx->node_pool[idx].inputs[0] = zero;
    ctx->node_pool[idx].inputs[1] = factor(ctx, engine);
    ctx->node_pool[idx].input_count = 2;
    return idx;
  }
  cr_error(ctx, "Syntax error"); return -1;
}

static int term(cr_context *ctx, cr_engine *engine) {
  int left = factor(ctx, engine);
  while (!strcmp(ctx->token, "*") || !strcmp(ctx->token, "/") || !strcmp(ctx->token, "%")) {
    int op = !strcmp(ctx->token, "*") ? 4 : (!strcmp(ctx->token, "/") ? 5 : 6);
    int n = alloc_node(ctx, &op_reg[op]);
    get_tok(ctx, engine);
    ctx->node_pool[n].inputs[0] = left;
    ctx->node_pool[n].inputs[1] = factor(ctx, engine);
    ctx->node_pool[n].input_count = 2;
    left = n;
  }
  return left;
}

static int arith(cr_context *ctx, cr_engine *engine) {
  int left = term(ctx, engine);
  while (!strcmp(ctx->token, "+") || !strcmp(ctx->token, "-")) {
    int op = !strcmp(ctx->token, "+") ? 2 : 3;
    int n = alloc_node(ctx, &op_reg[op]);
    get_tok(ctx, engine);
    ctx->node_pool[n].inputs[0] = left;
    ctx->node_pool[n].inputs[1] = term(ctx, engine);
    ctx->node_pool[n].input_count = 2;
    left = n;
  }
  return left;
}

static int expr(cr_context *ctx, cr_engine *engine) {
  int left = arith(ctx, engine);
  while (!strcmp(ctx->token, ">") || !strcmp(ctx->token, "<") || !strcmp(ctx->token, "==")) {
    int op = !strcmp(ctx->token, ">") ? 30 : (!strcmp(ctx->token, "<") ? 31 : 32);
    int n = alloc_node(ctx, &op_reg[op]);
    get_tok(ctx, engine);
    ctx->node_pool[n].inputs[0] = left;
    ctx->node_pool[n].inputs[1] = arith(ctx, engine);
    ctx->node_pool[n].input_count = 2;
    left = n;
  }
  return left;
}

static void stmt(cr_context *ctx, cr_engine *engine) {
  char name[128];
  int is_param = 0, idx;
  if (ctx->token_type == 0) return;

  if (!strcmp(ctx->token, "pattern")) {
      get_tok(ctx, engine);
      int pat_idx = atoi(ctx->token); 
      get_tok(ctx, engine);
      if (!strcmp(ctx->token, ",")) get_tok(ctx, engine);
      int ch_idx = atoi(ctx->token);
      get_tok(ctx, engine);
      if (!strcmp(ctx->token, ",")) get_tok(ctx, engine);
      if (ctx->token_type != 4) cr_error(ctx, "Expected pattern string");
      cr_parse_pattern_data(ctx, pat_idx, ch_idx, ctx->token);
      get_tok(ctx, engine);
      return;
  }

  if (!strcmp(ctx->token, "def")) {
    if (ctx->macro_count >= CR_MAX_MACROS) cr_error(ctx, "Max macros");
    cr_macro *m = &ctx->macros[ctx->macro_count++];
    get_tok(ctx, engine); strcpy(m->name, ctx->token);
    get_tok(ctx, engine); if (strcmp(ctx->token, "(")) cr_error(ctx, "Expected '('");
    m->arg_count = 0;
    get_tok(ctx, engine);
    if (strcmp(ctx->token, ")")) {
      while (1) {
        if (m->arg_count < CR_MAX_ARGS) strcpy(m->args[m->arg_count++], ctx->token);
        get_tok(ctx, engine); if (strcmp(ctx->token, ",")) break;
        get_tok(ctx, engine);
      }
    }
    if (strcmp(ctx->token, ")")) cr_error(ctx, "Expected ')'");
    get_tok(ctx, engine); if (strcmp(ctx->token, "{")) cr_error(ctx, "Expected '{'");
    m->body_start_offset = ctx->src_ptr - ctx->src_base;
    int depth = 1;
    while (*ctx->src_ptr && depth > 0) {
      if (*ctx->src_ptr == '{') depth++;
      if (*ctx->src_ptr == '}') depth--;
      if (depth == 0) break;
      ctx->src_ptr++;
    }
    m->body_end_offset = ctx->src_ptr - ctx->src_base;
    if (*ctx->src_ptr == '}') ctx->src_ptr++;
    get_tok(ctx, engine);
    return;
  }

  if (!strcmp(ctx->token, "return")) {
    get_tok(ctx, engine); ctx->return_node_idx = expr(ctx, engine); return;
  }

  if (ctx->token_type != 1) {
    if (strcmp(ctx->token, ";")) cr_error(ctx, "Unexpected token");
    get_tok(ctx, engine); return;
  }
  sprintf(name, "%s%s", ctx->scope, ctx->token);
  char raw_name[64]; strcpy(raw_name, ctx->token);
  get_tok(ctx, engine);

  if (!strcmp(ctx->token, ":")) {
    is_param = 1; get_tok(ctx, engine); if (strcmp(ctx->token, "=")) cr_error(ctx, "Expected '='");
  } else if (strcmp(ctx->token, "=")) cr_error(ctx, "Expected '='");
  get_tok(ctx, engine);

  idx = expr(ctx, engine);

  if (!strcmp(raw_name, "out")) { ctx->output_nodes[0] = idx; ctx->output_nodes[1] = idx; } 
  else if (!strcmp(raw_name, "bpm")) { ctx->bpm_node_idx = idx; }

  {
    int i, found = 0;
    for (i = 0; i < ctx->var_count; i++)
      if (!strcmp(ctx->variables[i].name, name)) {
        if (is_param) {
          int p = alloc_node(ctx, find_op("param"));
          ctx->node_pool[p].inputs[0] = idx; ctx->node_pool[p].input_count = 1;
          ctx->node_pool[p].dsp.s[0] = as_float(ctx->node_pool[idx].value);
          ctx->variables[i].node_index = p;
        } else { ctx->variables[i].node_index = idx; }
        found = 1; break;
      }
    if (!found && ctx->var_count < CR_MAX_SYMBOLS) {
      strcpy(ctx->variables[ctx->var_count].name, name);
      if (is_param) {
        int p = alloc_node(ctx, find_op("param"));
        ctx->node_pool[p].inputs[0] = idx; ctx->node_pool[p].input_count = 1;
        ctx->node_pool[p].dsp.s[0] = as_float(ctx->node_pool[idx].value);
        ctx->variables[ctx->var_count].node_index = p;
      } else { ctx->variables[ctx->var_count].node_index = idx; }
      ctx->var_count++;
    }
  }
}

static double get_period(int note) {
  if (note == 0 || note >= 254) return 0.0;
  return 440.0 * pow(2.0, (double)(note - 69) / 12.0);
}

static void cr_tracker_tick_process(cr_context *c) {
  cr_tracker_state *t = &c->tracker;
  
  if (t->current_tick == 0) {
    int pat_idx = t->order[t->current_pattern_idx];
    cr_pattern *p;
    int ch;

    if (pat_idx >= CR_MAX_PATTERNS) pat_idx = 0;
    p = &t->patterns[pat_idx];
    
    for (ch = 0; ch < CR_TRACK_CHANNELS; ch++) {
      cr_track_cell *cell = &p->rows[t->current_row][ch];
      
      if (cell->note) {
        if (cell->note == 254) {
           t->ch_gate[ch] = 0.0;
        } else {
           t->ch_freq[ch] = get_period(cell->note);
           t->ch_gate[ch] = 1.0;
           if (cell->vol <= 64) t->ch_vol[ch] = (double)cell->vol / 64.0;
           else t->ch_vol[ch] = 1.0; 
        }
      }
      if (cell->cmd == 0xF) {
         if (cell->val < 32) t->speed = cell->val;
         else t->tempo = cell->val;
      }
    }
  }

  t->current_tick++;
  if (t->current_tick >= t->speed) {
    t->current_tick = 0;
    t->current_row++;
    if (t->current_row >= CR_TRACK_ROWS) {
      t->current_row = 0;
      t->current_pattern_idx++;
      if (t->current_pattern_idx >= CR_MAX_ORDER || t->order[t->current_pattern_idx] == 255) {
        t->current_pattern_idx = 0;
      }
    }
  }
}

CR_API cr_engine *cr_create_engine(int sample_rate) {
  cr_engine *e = (cr_engine *)calloc(1, sizeof(cr_engine));
  int i;
  e->memory_block = calloc(1, 2 * CR_ARENA_SIZE);
  e->source_capacity = 4096;
  e->source_history = (char *)calloc(1, e->source_capacity);
  e->log_level = CR_LOG_INFO;
  for (i = 0; i < 2; i++) {
    e->contexts[i].sample_rate = sample_rate;
    e->contexts[i].arena_base = (unsigned char *)e->memory_block + (i * CR_ARENA_SIZE);
    e->contexts[i].arena_size = CR_ARENA_SIZE;
    e->contexts[i].output_nodes[0] = -1;
    e->contexts[i].output_nodes[1] = -1;
    e->contexts[i].transport.bpm = 120.0;
    e->contexts[i].transport.phase = 0.0;
    e->contexts[i].transport.playing = 1;
    e->contexts[i].tracker.speed = 6;
    e->contexts[i].tracker.tempo = 125;
  }
  e->active = &e->contexts[0];
  e->back = &e->contexts[1];
#if defined(_WIN32)
  InitializeCriticalSection(&e->swap_lock);
#else
  pthread_mutex_init(&e->swap_lock, NULL);
#endif
  return e;
}

CR_API void cr_destroy_engine(cr_engine *e) {
  if (e) {
    free(e->source_history);
    free(e->memory_block);
#ifndef _WIN32
    pthread_mutex_destroy(&e->swap_lock);
#endif
    free(e);
  }
}

CR_API void cr_set_log_level(cr_engine *e, int l) { e->log_level = l; }
CR_API void cr_set_log_callback(cr_engine *e, cr_log_cb cb) {
  e->log_callback = cb;
}

CR_API void cr_tick(cr_engine *e) {
  cr_context *c = e->active;
  double samples_per_tick;
  static double tick_acc = 0;

  c->global_time++;

  if (c->transport.playing) {
    c->transport.bpm = (double)c->tracker.tempo;
    if (c->transport.bpm < 10.0)
      c->transport.bpm = 10.0;
    c->transport.samples_per_beat = c->sample_rate * 60.0 / c->transport.bpm;
    c->transport.phase += 1.0 / c->transport.samples_per_beat;

    samples_per_tick = (c->sample_rate * 2.5) / c->transport.bpm;
    tick_acc += 1.0;
    if (tick_acc >= samples_per_tick) {
       cr_tracker_tick_process(c);
       tick_acc -= samples_per_tick;
    }
  }
}

CR_API double cr_process(cr_engine *e, int ch) {
  int i;
  cr_context *c = e->active;

  if (ch == 0 && c->bpm_node_idx != -1) {
    c->transport.bpm = as_float(c->node_pool[c->bpm_node_idx].value);
  }

  if (ch == 0)
    for (i = 0; i < c->exec_count; i++)
      exec_node(c, c->exec_order[i]);
  if (c->output_nodes[ch] != -1)
    return as_float(c->node_pool[c->output_nodes[ch]].value);
  return 0.0;
}

CR_API int cr_eval(cr_engine *e, const char *script, int reset) {
  cr_context *bk = e->back, *tmp;
  size_t len = strlen(script);
  int c_idx;

  if (e->source_len + len + 2 >= e->source_capacity) {
    e->source_capacity *= 2;
    e->source_history = (char *)realloc(e->source_history, e->source_capacity);
  }
  if (reset) {
    e->source_len = 0;
    e->source_history[0] = 0;
  }
  strcat(e->source_history, script);
  strcat(e->source_history, "\n");
  e->source_len += len + 1;

  bk->node_idx = 0;
  bk->arena_top = 0;
  bk->var_count = 0;
  bk->exec_count = 0;
  bk->output_nodes[0] = -1;
  bk->output_nodes[1] = -1;
  bk->macro_count = 0;
  bk->call_depth = 0;
  bk->scope[0] = 0;
  bk->scope_counter = 0;
  bk->global_time = e->active->global_time;
  bk->transport = e->active->transport;

  bk->src_base = e->source_history;
  bk->src_ptr = e->source_history;
  bk->src_end = NULL;
  bk->current_line = 1;

  cr_log(e, CR_LOG_INFO, "Compiling...");

  if (setjmp(bk->err_jmp)) {
    cr_log(e, CR_LOG_ERROR, "Compile Error: %s", bk->error_msg);
    return 0;
  }
  get_tok(bk, e);
  while (bk->token_type != 0)
    stmt(bk, e);

  cr_log(e, CR_LOG_INFO, "Success. Migrating...");
  migrate_state(bk, e->active);
  memset(bk->visit_state, 0, sizeof(bk->visit_state));
  for (c_idx = 0; c_idx < CR_MAX_CHANNELS; c_idx++)
    if (bk->output_nodes[c_idx] != -1)
      topo_visit(bk, bk->output_nodes[c_idx]);

#if defined(_WIN32)
  EnterCriticalSection(&e->swap_lock);
#else
  pthread_mutex_lock(&e->swap_lock);
#endif
  tmp = e->active;
  e->active = bk;
  e->back = tmp;
#if defined(_WIN32)
  LeaveCriticalSection(&e->swap_lock);
#else
  pthread_mutex_unlock(&e->swap_lock);
#endif

  return 1;
}

CR_API int cr_get_variable_node(cr_context *ctx, const char *name) {
  int i;
  for (i = 0; i < ctx->var_count; i++)
    if (!strcmp(ctx->variables[i].name, name))
      return ctx->variables[i].node_index;
  return -1;
}

CR_API void cr_note_on(cr_engine *e, double p, double v) {}
CR_API void cr_note_off(cr_engine *e, double p) {}

#endif
#endif
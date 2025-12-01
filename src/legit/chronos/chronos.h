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
/** @brief Macro to define API export visibility. */
#define CR_API
#endif

/** @brief Maximum number of audio output channels (Stereo). */
#define CR_MAX_CHANNELS 2
/** @brief Maximum number of variable names/symbols allowed in a script. */
#define CR_MAX_SYMBOLS 1024
/** @brief Maximum number of DSP nodes allowed in the execution graph. */
#define CR_MAX_NODES 16384
/** @brief Maximum number of arguments for an operator or macro. */
#define CR_MAX_ARGS 16
/** @brief Maximum number of simultaneous polyphonic voices (unused in current logic). */
#define CR_MAX_VOICES 8
/** @brief Maximum number of user-defined macros (functions). */
#define CR_MAX_MACROS 128
/** @brief Maximum recursion depth for macro calls. */
#define CR_CALL_STACK 32
/** @brief Mathematical constant PI. */
#define CR_PI 3.14159265359
/** @brief Size of the memory arena for real-time allocation (64MB). */
#define CR_ARENA_SIZE (64 * 1024 * 1024)

/**
 * @brief Logging levels for the engine.
 */
typedef enum {
  CR_LOG_NONE = 0, /**< No logging. */
  CR_LOG_ERROR,    /**< Critical errors. */
  CR_LOG_WARN,     /**< Warnings. */
  CR_LOG_INFO,     /**< Informational messages. */
  CR_LOG_DEBUG     /**< Debugging information. */
} cr_log_level;

/**
 * @brief Data types supported by the Chronos value system.
 */
typedef enum { 
    CR_FLOAT, /**< Double precision floating point. */
    CR_INT    /**< Integer value. */
} cr_type_t;

/**
 * @brief A generic value wrapper used for node inputs/outputs.
 */
typedef struct cr_val {
  cr_type_t type; /**< The type of the value. */
  union {
    double f; /**< Floating point value. */
    int i;    /**< Integer value. */
  } as;       /**< Union holding the actual data. */
} cr_val;

/**
 * @brief Represents a single polyphonic voice state.
 */
typedef struct {
  int active;        /**< 1 if the voice is currently playing, 0 otherwise. */
  int note_id;       /**< Unique identifier for the note event. */
  double pitch;      /**< Frequency of the note in Hz. */
  double velocity;   /**< Velocity of the note (0.0 to 1.0). */
  long start_time;   /**< Timestamp when the note started. */
  long release_time; /**< Timestamp when the note was released. */
} cr_voice;

struct cr_context;
struct cr_node;

/**
 * @brief Function pointer type for DSP operator handlers.
 * * @param ctx Pointer to the execution context.
 * @param n Pointer to the current node being executed.
 * @param inputs Array of input values resolved from input nodes.
 * @return The calculated output value for this node.
 */
typedef cr_val (*cr_op_func)(struct cr_context *ctx, struct cr_node *n,
                             cr_val *inputs);

/**
 * @brief Description of a DSP operator (opcode).
 */
typedef struct cr_op_desc {
  const char *name;  /**< Name of the operator used in scripts (e.g., "sin"). */
  int opcode;        /**< Internal numeric opcode. */
  cr_op_func handler;/**< Pointer to the function implementing the logic. */
} cr_op_desc;

/**
 * @brief Internal DSP state for stateful nodes (filters, oscillators).
 */
typedef struct {
  double s[8];      /**< Generic state registers (history, phase, etc.). */
  double *buffer;   /**< Pointer to allocated buffer (delay lines, samples). */
  int buf_len;      /**< Length of the allocated buffer. */
  int write_head;   /**< Current write position for ring buffers. */
} cr_dsp_state;

/**
 * @brief A node in the DSP graph.
 */
typedef struct cr_node {
  int id;                  /**< Unique index of the node in the pool. */
  const cr_op_desc *op_desc; /**< Pointer to the operator description. */
  int inputs[CR_MAX_ARGS]; /**< Indices of input nodes. */
  int input_count;         /**< Number of connected inputs. */
  cr_val value;            /**< The current output value of the node. */
  cr_dsp_state dsp;        /**< Internal DSP state. */
} cr_node;

/**
 * @brief A named variable mapping to a node index.
 */
typedef struct {
  char name[64];   /**< Variable name. */
  int node_index;  /**< Index of the node this variable refers to. */
} cr_variable;

/**
 * @brief A user-defined macro (function definition).
 */
typedef struct {
  char name[32];            /**< Macro name. */
  size_t body_start_offset; /**< Byte offset in source where body starts. */
  size_t body_end_offset;   /**< Byte offset in source where body ends. */
  char args[CR_MAX_ARGS][32]; /**< Argument names. */
  int arg_count;            /**< Number of arguments. */
} cr_macro;

/**
 * @brief Call stack frame for macro execution.
 */
typedef struct {
  char *ret_addr;     /**< Source pointer to return to after macro. */
  char *ret_end;      /**< End pointer of the calling scope. */
  char old_scope[64]; /**< Previous scope prefix string. */
} cr_call_frame;

/**
 * @brief Transport state (BPM, playback status).
 */
typedef struct {
  double bpm;              /**< Beats per minute. */
  double phase;            /**< Current beat phase (accumulating). */
  double samples_per_beat; /**< Calculated samples per beat. */
  long total_samples;      /**< Total samples processed. */
  int playing;             /**< 1 if transport is running, 0 if paused. */
} cr_transport;

/**
 * @brief The execution context containing the entire engine state.
 */
typedef struct cr_context {
  cr_transport transport;  /**< Transport information. */
  long global_time;        /**< Global sample counter. */

  int output_nodes[CR_MAX_CHANNELS]; /**< Indices of final output nodes. */
  double sample_rate;                /**< System sample rate. */

  cr_variable variables[CR_MAX_SYMBOLS]; /**< Symbol table. */
  int var_count;                         /**< Number of defined variables. */

  cr_macro macros[CR_MAX_MACROS]; /**< Macro table. */
  int macro_count;                /**< Number of defined macros. */

  cr_call_frame call_stack[CR_CALL_STACK]; /**< Recursion stack. */
  int call_depth;                          /**< Current stack depth. */
  char scope[64];                          /**< Current scope prefix. */
  int scope_counter;                       /**< Counter for generating unique scopes. */

  cr_node node_pool[CR_MAX_NODES]; /**< Pool of all DSP nodes. */
  int node_idx;                    /**< Number of allocated nodes. */

  int exec_order[CR_MAX_NODES];       /**< Topological sort order for execution. */
  int exec_count;                     /**< Number of nodes in execution list. */
  unsigned char visit_state[CR_MAX_NODES]; /**< Helper for topological sort (0=unvisited, 1=visiting, 2=visited). */

  unsigned char *arena_base; /**< Base address of memory arena. */
  size_t arena_size;         /**< Total size of memory arena. */
  size_t arena_top;          /**< Current allocation offset. */

  char *src_base;   /**< Base address of source script. */
  char *src_ptr;    /**< Current parsing pointer. */
  char *src_end;    /**< End of source buffer (or NULL). */
  char token[128];  /**< Current token string. */
  int token_type;   /**< Type of current token (1=ID, 2=Num, 3=Sym, 4=Str). */
  jmp_buf err_jmp;  /**< Jump buffer for error handling. */
  char error_msg[128]; /**< Error message buffer. */
  int current_line;    /**< Current line number in source. */

  int bpm_node_idx;    /**< Node index controlling BPM (or -1). */
  int return_node_idx; /**< Node index holding return value (for macros). */
} cr_context;

/**
 * @brief Callback signature for engine logging.
 */
typedef void (*cr_log_cb)(int level, const char *msg);

/**
 * @brief Main engine structure managing double-buffering.
 */
typedef struct cr_engine {
  cr_context contexts[2]; /**< Double buffered contexts (active and background). */
  cr_context *active;     /**< Pointer to the currently processing context. */
  cr_context *back;       /**< Pointer to the context being compiled/edited. */
  void *memory_block;     /**< Raw memory block for arenas. */
  char *source_history;   /**< Buffer storing the full script source. */
  size_t source_capacity; /**< Allocated capacity of source history. */
  size_t source_len;      /**< Current length of source history. */
  int log_level;          /**< Current log filtering level. */
  cr_voice voice_manager[CR_MAX_VOICES]; /**< Voice management state. */
  int current_note_id;    /**< Counter for note IDs. */
  cr_log_cb log_callback; /**< User-provided log callback. */
#if defined(_WIN32)
  CRITICAL_SECTION swap_lock; /**< Thread synchronization for Windows. */
#else
  pthread_mutex_t swap_lock; /**< Thread synchronization for POSIX. */
#endif
} cr_engine;

/**
 * @brief Creates and initializes a new audio engine instance.
 * @param sample_rate The audio sample rate (e.g., 44100 or 48000).
 * @return Pointer to the new cr_engine, or NULL on failure.
 */
CR_API cr_engine *cr_create_engine(int sample_rate);

/**
 * @brief Destroys the engine and frees all resources.
 * @param engine Pointer to the engine to destroy.
 */
CR_API void cr_destroy_engine(cr_engine *engine);

/**
 * @brief Sets the minimum logging level.
 * @param engine Pointer to the engine.
 * @param level The logging level threshold.
 */
CR_API void cr_set_log_level(cr_engine *engine, int level);

/**
 * @brief Sets the callback function for log messages.
 * @param engine Pointer to the engine.
 * @param cb The callback function.
 */
CR_API void cr_set_log_callback(cr_engine *engine, cr_log_cb cb);

/**
 * @brief Processes one sample for a specific channel.
 * @param engine Pointer to the engine.
 * @param channel The channel index (0 or 1).
 * @return The calculated audio sample (-1.0 to 1.0).
 */
CR_API double cr_process(cr_engine *engine, int channel);

/**
 * @brief Advances the engine's time state (call once per frame).
 * @param engine Pointer to the engine.
 */
CR_API void cr_tick(cr_engine *engine);

/**
 * @brief Evaluates a script and hot-swaps the DSP graph on success.
 * @param engine Pointer to the engine.
 * @param script The source code string to evaluate.
 * @param reset If 1, clears previous history; if 0, appends.
 * @return 1 on success, 0 on compilation error.
 */
CR_API int cr_eval(cr_engine *engine, const char *script, int reset);

/**
 * @brief Finds the node index associated with a variable name.
 * @param ctx The context to search.
 * @param name The variable name.
 * @return The node index, or -1 if not found.
 */
CR_API int cr_get_variable_node(cr_context *ctx, const char *name);

/**
 * @brief Triggers a note on event (unused in current demo).
 * @param engine Pointer to the engine.
 * @param pitch The pitch value.
 * @param velocity The velocity value.
 */
CR_API void cr_note_on(cr_engine *engine, double pitch, double velocity);

/**
 * @brief Triggers a note off event (unused in current demo).
 * @param engine Pointer to the engine.
 * @param pitch The pitch value to release.
 */
CR_API void cr_note_off(cr_engine *engine, double pitch);

#ifdef CR_IMPLEMENTATION

/**
 * @brief Internal helper to log messages.
 * @param e Pointer to the engine.
 * @param level Severity level.
 * @param fmt Printf-style format string.
 * @param ... Variadic arguments.
 */
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

/**
 * @brief Triggers a compilation error via longjmp.
 * @param ctx The active context.
 * @param msg The error message.
 */
static void cr_error(cr_context *ctx, const char *msg) {
  sprintf(ctx->error_msg, "Line %d: %s (Token: '%s')", ctx->current_line, msg,
          ctx->token);
  longjmp(ctx->err_jmp, 1);
}

/**
 * @brief Allocates memory from the context's linear arena.
 * @param ctx The active context.
 * @param size Number of bytes to allocate.
 * @return Pointer to allocated memory, or NULL if arena is full.
 */
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

/**
 * @brief Allocates a new node in the node pool.
 * @param ctx The active context.
 * @param op_desc The operator description to assign.
 * @return The index of the new node.
 */
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

/**
 * @brief Helper to get a double from a cr_val, converting int if necessary.
 * @param v The value to convert.
 * @return The floating point representation.
 */
static inline double as_float(cr_val v) {
  return (v.type == CR_INT) ? (double)v.as.i : v.as.f;
}

/**
 * @brief Helper to create a cr_val from a double.
 * @param f The floating point value.
 * @return The wrapped cr_val.
 */
static inline cr_val make_float(double f) {
  cr_val v;
  v.type = CR_FLOAT;
  v.as.f = f;
  return v;
}

/**
 * @brief Parses a string into a double value.
 * @param str The string to parse.
 * @return The parsed double.
 */
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

/**
 * @brief Converts a note name (e.g., "A4", "C#3") to frequency.
 * @param note The note string.
 * @return Frequency in Hz.
 */
static double note_to_freq(const char *note) {
  static const char *names[] = {"C",  "C#", "D",  "D#", "E",  "F",
                                "F#", "G",  "G#", "A",  "A#", "B"};
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

/**
 * @brief PolyBLEP residual function for anti-aliasing.
 * @param t Phase (0.0 to 1.0).
 * @param dt Phase increment per sample.
 * @return Correction value.
 */
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

/**
 * @brief Hermite cubic interpolation.
 * @param x Fractional position (0.0 to 1.0).
 * @param y0 Value at x-1.
 * @param y1 Value at x.
 * @param y2 Value at x+1.
 * @param y3 Value at x+2.
 * @return Interpolated value.
 */
static double hermite(double x, double y0, double y1, double y2, double y3) {
  double c0 = y1;
  double c1 = 0.5 * (y2 - y0);
  double c2 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3;
  double c3 = 0.5 * (y3 - y0) + 1.5 * (y1 - y2);
  return ((c3 * x + c2) * x + c1) * x + c0;
}

static const cr_op_desc *find_op(const char *name);

/**
 * @brief Helper to prepare pattern buffers for arithmetic ops.
 * @param c Context.
 * @param n Node.
 * @param mode Mode (0=Add, 1=Mul, 2=Div).
 */
static void build_pattern_op(cr_context *c, cr_node *n, int mode) {
  cr_node *in0, *in1;
  double *buf;
  int max_pat_len = 65536;

  if (n->dsp.buffer != NULL)
    return;

  in0 = &c->node_pool[n->inputs[0]];
  in1 = &c->node_pool[n->inputs[1]];

  if (mode == 0 && in0->dsp.buffer && in1->dsp.buffer) {
    int len;
    if (in0->dsp.buf_len + in1->dsp.buf_len > max_pat_len)
      return;
    len = in0->dsp.buf_len + in1->dsp.buf_len;
    buf = (double *)cr_arena_alloc(c, len * sizeof(double));
    if (buf) {
      memcpy(buf, in0->dsp.buffer, in0->dsp.buf_len * sizeof(double));
      memcpy(buf + in0->dsp.buf_len, in1->dsp.buffer,
             in1->dsp.buf_len * sizeof(double));
      n->dsp.buffer = buf;
      n->dsp.buf_len = len;
      n->op_desc = find_op("data");
      n->value.as.f = 0.0;
    }
  } else if (mode == 1) {
    cr_node *pat = in0->dsp.buffer ? in0 : (in1->dsp.buffer ? in1 : NULL);
    cr_node *num = (pat == in0) ? in1 : in0;
    if (pat && pat->dsp.buffer) {
      int i, reps = (int)as_float(num->value);
      if (reps < 1)
        reps = 1;
      if (pat->dsp.buf_len * reps > max_pat_len)
        return;
      buf =
          (double *)cr_arena_alloc(c, pat->dsp.buf_len * reps * sizeof(double));
      if (buf) {
        for (i = 0; i < reps; i++) {
          memcpy(buf + (i * pat->dsp.buf_len), pat->dsp.buffer,
                 pat->dsp.buf_len * sizeof(double));
        }
        n->dsp.buffer = buf;
        n->dsp.buf_len = pat->dsp.buf_len * reps;
        n->op_desc = find_op("data");
        n->value.as.f = 0.0;
      }
    }
  } else if (mode == 2 && in0->dsp.buffer) {
    double div = as_float(in1->value);
    int new_len;
    if (div < 1.0)
      div = 1.0;
    new_len = (int)(in0->dsp.buf_len / div);
    if (new_len < 1)
      new_len = 1;
    buf = (double *)cr_arena_alloc(c, new_len * sizeof(double));
    if (buf) {
      memcpy(buf, in0->dsp.buffer, new_len * sizeof(double));
      n->dsp.buffer = buf;
      n->dsp.buf_len = new_len;
      n->op_desc = find_op("data");
      n->value.as.f = 0.0;
    }
  }
}

/** @brief Handler for constant values. */
static cr_val op_const(cr_context *c, cr_node *n, cr_val *v) {
  return n->value;
}
/** @brief Handler for pre-calculated data patterns. */
static cr_val op_data(cr_context *c, cr_node *n, cr_val *v) {
  return make_float(0.0);
}
/** @brief Handler for addition and pattern concatenation. */
static cr_val op_add(cr_context *c, cr_node *n, cr_val *v) {
  build_pattern_op(c, n, 0);
  return make_float(as_float(v[0]) + as_float(v[1]));
}
/** @brief Handler for subtraction. */
static cr_val op_sub(cr_context *c, cr_node *n, cr_val *v) {
  return make_float(as_float(v[0]) - as_float(v[1]));
}
/** @brief Handler for multiplication and pattern repetition. */
static cr_val op_mul(cr_context *c, cr_node *n, cr_val *v) {
  build_pattern_op(c, n, 1);
  return make_float(as_float(v[0]) * as_float(v[1]));
}
/** @brief Handler for division and pattern truncation. */
static cr_val op_div(cr_context *c, cr_node *n, cr_val *v) {
  build_pattern_op(c, n, 2);
  double d = as_float(v[1]);
  return make_float((fabs(d) < 1e-9) ? 0.0 : as_float(v[0]) / d);
}
/** @brief Handler for modulo. */
static cr_val op_mod(cr_context *c, cr_node *n, cr_val *v) {
  double d = as_float(v[1]);
  return make_float((fabs(d) < 1e-9) ? 0.0 : fmod(as_float(v[0]), d));
}
/** @brief Handler for power. */
static cr_val op_pow(cr_context *c, cr_node *n, cr_val *v) {
  return make_float(pow(as_float(v[0]), as_float(v[1])));
}

/** @brief Handler for sine wave oscillator. */
static cr_val op_sin(cr_context *c, cr_node *n, cr_val *v) {
  double freq = as_float(v[0]);
  double inc = freq / c->sample_rate;
  double p = n->dsp.s[0] + inc;
  p -= floor(p);
  n->dsp.s[0] = p;
  return make_float(sin(p * 2.0 * CR_PI));
}

/** @brief Handler for white noise generator. */
static cr_val op_noise(cr_context *c, cr_node *n, cr_val *v) {
  return make_float(((double)rand() / RAND_MAX) * 2.0 - 1.0);
}
/** @brief Handler for global time retrieval. */
static cr_val op_time(cr_context *c, cr_node *n, cr_val *v) {
  return make_float((double)c->global_time / c->sample_rate);
}

/** @brief Handler for Greater Than comparison. */
static cr_val op_gt(cr_context *c, cr_node *n, cr_val *v) {
  return make_float(as_float(v[0]) > as_float(v[1]) ? 1.0 : 0.0);
}
/** @brief Handler for Less Than comparison. */
static cr_val op_lt(cr_context *c, cr_node *n, cr_val *v) {
  return make_float(as_float(v[0]) < as_float(v[1]) ? 1.0 : 0.0);
}
/** @brief Handler for Equality comparison. */
static cr_val op_eq(cr_context *c, cr_node *n, cr_val *v) {
  return make_float(fabs(as_float(v[0]) - as_float(v[1])) < 1e-5 ? 1.0 : 0.0);
}
/** @brief Handler for Conditional Logic (ternary). */
static cr_val op_if(cr_context *c, cr_node *n, cr_val *v) {
  return make_float(as_float(v[0]) > 0.5 ? as_float(v[1]) : as_float(v[2]));
}
/** @brief Handler for Select logic (switch case). */
static cr_val op_select(cr_context *c, cr_node *n, cr_val *v) {
  int sel = (int)as_float(v[0]);
  int num_options = n->input_count - 1;
  if (sel < 0)
    sel = 0;
  if (sel >= num_options)
    sel = num_options - 1;
  return v[sel + 1];
}

/** @brief Handler for smoothed parameter output. */
static cr_val op_param(cr_context *c, cr_node *n, cr_val *v) {
  double target = as_float(v[0]);
  double cur = n->dsp.s[0];
  double d = target - cur;
  if (fabs(d) > 0.005)
    cur += (d > 0) ? 0.005 : -0.005;
  else
    cur = target;
  n->dsp.s[0] = cur;
  return make_float(cur);
}

/** @brief Handler for phase accumulator (0.0 to 1.0). */
static cr_val op_phasor(cr_context *c, cr_node *n, cr_val *v) {
  double inc = as_float(v[0]) / c->sample_rate;
  double p = n->dsp.s[0] + inc;
  p -= floor(p);
  n->dsp.s[0] = p;
  return make_float(p);
}

/** @brief Handler for band-limited saw wave. */
static cr_val op_saw(cr_context *c, cr_node *n, cr_val *v) {
  double freq = as_float(v[0]);
  if (freq < 0.1)
    freq = 0.1;
  double inc = freq / c->sample_rate;
  double p = n->dsp.s[0] + inc;
  double out;
  p -= floor(p);
  n->dsp.s[0] = p;
  out = (2.0 * p - 1.0);
  out -= poly_blep(p, inc);
  return make_float(out);
}

/** @brief Handler for band-limited pulse/square wave. */
static cr_val op_pulse(cr_context *c, cr_node *n, cr_val *v) {
  double freq = as_float(v[0]);
  if (freq < 0.1)
    freq = 0.1;
  double width = (n->input_count > 1) ? as_float(v[1]) : 0.5;
  double inc = freq / c->sample_rate;
  double p = n->dsp.s[0] + inc;
  double out;
  p -= floor(p);
  n->dsp.s[0] = p;
  out = (p < width) ? 1.0 : -1.0;
  out += poly_blep(p, inc);
  out -= poly_blep(fmod(p - width + 1.0, 1.0), inc);
  return make_float(out);
}

/** @brief Handler for Chamberlin State Variable Filter. */
static cr_val op_filter(cr_context *c, cr_node *n, cr_val *v) {
  double in = as_float(v[0]), type = as_float(v[1]), cut = as_float(v[2]),
         q = as_float(v[3]);
  double f = 2.0 * sin(CR_PI * cut / c->sample_rate);
  if (f > 0.9)
    f = 0.9;
  if (f < 0.001)
    f = 0.001;
  if (q < 0.1)
    q = 0.1;
  if (q > 10.0)
    q = 10.0;
  n->dsp.s[1] += f * (in - n->dsp.s[1] - (1.0 / q) * n->dsp.s[0]);
  n->dsp.s[0] += f * n->dsp.s[1];
  if (n->dsp.s[0] > 10.0)
    n->dsp.s[0] = 10.0;
  else if (n->dsp.s[0] < -10.0)
    n->dsp.s[0] = -10.0;
  if (n->dsp.s[1] > 10.0)
    n->dsp.s[1] = 10.0;
  else if (n->dsp.s[1] < -10.0)
    n->dsp.s[1] = -10.0;
  if (type < 0.5)
    return make_float(n->dsp.s[0]);
  if (type < 1.5)
    return make_float(n->dsp.s[1]);
  return make_float(in - n->dsp.s[0]);
}

/** @brief Handler for delay line with feedback and saturation. */
static cr_val op_delay(cr_context *c, cr_node *n, cr_val *v) {
  double in = as_float(v[0]), tm = as_float(v[1]), fb = as_float(v[2]),
         out = 0.0;
  if (n->dsp.buffer && n->dsp.buf_len > 0) {
    double dspls = tm * c->sample_rate;
    double rp = n->dsp.write_head - dspls;
    while (rp < 0)
      rp += n->dsp.buf_len;
    int i0 = (int)rp;
    double frac = rp - i0;
    int idx0 = (i0 - 1 + n->dsp.buf_len) % n->dsp.buf_len;
    int idx1 = i0 % n->dsp.buf_len;
    int idx2 = (i0 + 1) % n->dsp.buf_len;
    int idx3 = (i0 + 2) % n->dsp.buf_len;
    out = hermite(frac, n->dsp.buffer[idx0], n->dsp.buffer[idx1],
                  n->dsp.buffer[idx2], n->dsp.buffer[idx3]);
    double fb_sig = in + out * fb;
    if (fb_sig > 1.2)
      fb_sig = 1.2 + tanh(fb_sig - 1.2) * 0.5;
    else if (fb_sig < -1.2)
      fb_sig = -1.2 + tanh(fb_sig + 1.2) * 0.5;
    n->dsp.buffer[n->dsp.write_head] = fb_sig;
    n->dsp.write_head = (n->dsp.write_head + 1) % n->dsp.buf_len;
  }
  return make_float(out);
}

/** @brief Handler for sequencer pattern reader. */
static cr_val op_seq(cr_context *c, cr_node *n, cr_val *v) {
  int data_idx = n->inputs[0];
  double div = as_float(v[1]);
  cr_node *data = &c->node_pool[data_idx];
  if (data->dsp.buffer && data->dsp.buf_len > 0) {
    int idx = (int)(c->transport.phase * div) % data->dsp.buf_len;
    if (idx < 0)
      idx += data->dsp.buf_len;

    double current_val = n->dsp.s[2];
    double target_val;

    if ((int)n->dsp.s[0] != idx) {
      target_val = data->dsp.buffer[idx];
      n->dsp.s[1] = target_val;
      n->dsp.s[0] = (double)idx;
    } else {
      target_val = n->dsp.s[1];
    }

    if (fabs(target_val - current_val) > 0.0001) {
      current_val += (target_val - current_val) * 0.9;
    } else {
      current_val = target_val;
    }

    n->dsp.s[2] = current_val;
    return make_float(current_val);
  }
  return make_float(0.0);
}

/** @brief Handler for trigger generator based on pattern. */
static cr_val op_trig(cr_context *c, cr_node *n, cr_val *v) {
  int data_idx = n->inputs[0];
  double div = as_float(v[1]);
  cr_node *data = &c->node_pool[data_idx];
  if (data->dsp.buffer && data->dsp.buf_len > 0) {
    int idx = (int)(c->transport.phase * div) % data->dsp.buf_len;
    if (idx < 0)
      idx += data->dsp.buf_len;
    double current_val = data->dsp.buffer[idx];
    double prev_idx = n->dsp.s[0];
    n->dsp.s[0] = (double)idx;
    if ((int)prev_idx != idx && current_val > 0.001) {
      return make_float(1.0);
    }
  }
  return make_float(0.0);
}

/** @brief Allocates an empty buffer of zeros. */
static cr_val op_zeros(cr_context *c, cr_node *n, cr_val *v) {
  int count = (int)as_float(v[0]);
  if (count < 1)
    count = 1;
  if (n->dsp.buffer == NULL) {
    n->dsp.buffer = (double *)cr_arena_alloc(c, count * sizeof(double));
    if (n->dsp.buffer) {
      memset(n->dsp.buffer, 0, count * sizeof(double));
      n->dsp.buf_len = count;
    }
  }
  return make_float(0.0);
}

/** @brief Generates clock pulses based on transport phase. */
static cr_val op_clock(cr_context *c, cr_node *n, cr_val *v) {
  double div = as_float(v[0]);
  double phase = c->transport.phase * div;
  double p_frac = phase - floor(phase);
  double prev = n->dsp.s[0];
  n->dsp.s[0] = p_frac;
  if (p_frac < prev || (c->global_time == 0 && p_frac == 0.0)) {
    return make_float(1.0);
  }
  return make_float(0.0);
}

/** @brief Simple AR envelope generator. */
static cr_val op_env(cr_context *c, cr_node *n, cr_val *v) {
  double trig = as_float(v[0]);
  double decay = as_float(v[1]);
  double val = n->dsp.s[0];
  if (trig > 0.5)
    val = 1.0;
  else
    val *= decay;
  if (val < 0.0001)
    val = 0.0;
  n->dsp.s[0] = val;
  return make_float(val);
}

/** @brief Percussive AD envelope generator. */
static cr_val op_perc(cr_context *c, cr_node *n, cr_val *v) {
  double trig = as_float(v[0]);
  double a = as_float(v[1]);
  double d = as_float(v[2]);
  double val = n->dsp.s[0];
  int state = (int)n->dsp.s[1];
  double prev_trig = n->dsp.s[2];
  double decay_coeff;

  if (a < 0.001)
    a = 0.001;
  if (d < 0.001)
    d = 0.001;

  if (trig > 0.5 && prev_trig <= 0.5)
    state = 1;
  n->dsp.s[2] = trig;

  if (state == 1) {
    double inc = 1.0 / (a * c->sample_rate);
    val += inc;
    if (val >= 1.0) {
      val = 1.0;
      state = 2;
    }
  } else if (state == 2) {
    decay_coeff = exp(-2.2 / (d * c->sample_rate));
    val *= decay_coeff;
    if (val < 0.0001) {
      val = 0.0;
      state = 0;
    }
  }

  n->dsp.s[0] = val;
  n->dsp.s[1] = (double)state;
  return make_float(val);
}

/** @brief Full ADSR envelope generator. */
static cr_val op_adsr(cr_context *c, cr_node *n, cr_val *v) {
  double gate = as_float(v[0]);
  double a = as_float(v[1]);
  double d = as_float(v[2]);
  double s_lvl = as_float(v[3]);
  double r = as_float(v[4]);
  double val = n->dsp.s[0];
  int state = (int)n->dsp.s[1];
  double prev_gate = n->dsp.s[2];
  double decay_coeff, release_coeff;

  if (a < 0.002)
    a = 0.002;
  if (d < 0.002)
    d = 0.002;
  if (r < 0.002)
    r = 0.002;

  if (gate > 0.5 && prev_gate <= 0.5)
    state = 1;
  else if (gate <= 0.5 && prev_gate > 0.5)
    state = 4;

  switch (state) {
  case 1:
    val += 1.0 / (a * c->sample_rate);
    if (val >= 1.0) {
      val = 1.0;
      state = 2;
    }
    break;
  case 2:
    decay_coeff = exp(-2.2 / (d * c->sample_rate));
    val = s_lvl + (val - s_lvl) * decay_coeff;
    if (val <= s_lvl + 0.001) {
      val = s_lvl;
      state = 3;
    }
    break;
  case 3:
    val = s_lvl;
    break;
  case 4:
    release_coeff = exp(-2.2 / (r * c->sample_rate));
    val *= release_coeff;
    if (val < 0.001) {
      val = 0.0;
      state = 0;
    }
    break;
  }

  n->dsp.s[0] = val;
  n->dsp.s[1] = (double)state;
  n->dsp.s[2] = gate;
  return make_float(val);
}

/** @brief Handler for summing mixer with soft clipping. */
static cr_val op_mix(cr_context *c, cr_node *n, cr_val *v) {
  double sum = 0.0;
  int i;
  for (i = 0; i < n->input_count; i++)
    sum += as_float(v[i]);

  if (sum > 1.5)
    sum = 1.5 + tanh(sum - 1.5) * 0.5;
  else if (sum < -1.5)
    sum = -1.5 + tanh(sum + 1.5) * 0.5;

  return make_float(sum);
}

/** @brief Registry of available operators. */
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
    {"trig", 54, op_trig},     {"zeros", 55, op_zeros},
    {"perc", 56, op_perc},     {"data", 20, op_data},
    {"nop", 99, NULL}};

/**
 * @brief Finds an operator description by name.
 * @param name The operator name.
 * @return Pointer to op_desc or NULL.
 */
static const cr_op_desc *find_op(const char *name) {
  int i;
  for (i = 0; i < sizeof(op_reg) / sizeof(op_reg[0]); i++)
    if (!strcmp(op_reg[i].name, name))
      return &op_reg[i];
  return NULL;
}

/**
 * @brief Migrates DSP state (buffers, phases) from an old context to a new one.
 * @param dst The destination context (newly compiled).
 * @param src The source context (previous active state).
 */
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
            if (nb && sn->dsp.buffer)
              memcpy(nb, sn->dsp.buffer,
                     (dn->dsp.buf_len < sn->dsp.buf_len ? dn->dsp.buf_len
                                                        : sn->dsp.buf_len) *
                         sizeof(double));
          }
        }
        break;
      }
    }
  }
}

/**
 * @brief Executes a single node by calling its handler.
 * @param ctx The active context.
 * @param idx The index of the node to execute.
 */
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

/**
 * @brief Performs a topological sort visit on the graph.
 * @param ctx The active context.
 * @param u The node index to visit.
 */
static void topo_visit(cr_context *ctx, int u) {
  int i;
  cr_node *n;
  if (ctx->visit_state[u] == 2)
    return;
  if (ctx->visit_state[u] == 1)
    cr_error(ctx, "DSP Cycle detected");
  ctx->visit_state[u] = 1;
  n = &ctx->node_pool[u];
  for (i = 0; i < n->input_count; i++)
    topo_visit(ctx, n->inputs[i]);
  ctx->visit_state[u] = 2;
  ctx->exec_order[ctx->exec_count++] = u;
}

/**
 * @brief Lexer function to extract the next token from the source.
 * @param ctx The active context.
 * @param engine Pointer to engine for logging.
 */
static void get_tok(cr_context *ctx, cr_engine *engine) {
  const char *p;
  int i = 0;
  while (1) {
    if (ctx->src_end && ctx->src_ptr >= ctx->src_end) {
      ctx->token_type = 0;
      return;
    }
    while (isspace((unsigned char)*ctx->src_ptr)) {
      if (*ctx->src_ptr == '\n')
        ctx->current_line++;
      ctx->src_ptr++;
      if (ctx->src_end && ctx->src_ptr >= ctx->src_end) {
        ctx->token_type = 0;
        return;
      }
    }
    if (*ctx->src_ptr == '#') {
      while (*ctx->src_ptr && *ctx->src_ptr != '\n')
        ctx->src_ptr++;
      continue;
    }
    break;
  }
  if (*ctx->src_ptr == 0) {
    ctx->token_type = 0;
    return;
  }
  p = ctx->src_ptr;
  if (isalpha((unsigned char)*p) || *p == '_') {
    while (isalnum((unsigned char)*ctx->src_ptr) || *ctx->src_ptr == '_')
      if (i < 127)
        ctx->token[i++] = *ctx->src_ptr++;
    ctx->token[i] = 0;
    ctx->token_type = 1;
  } else if (isdigit((unsigned char)*p) ||
             (*p == '.' && isdigit((unsigned char)p[1]))) {
    while (isdigit((unsigned char)*ctx->src_ptr) || *ctx->src_ptr == '.')
      if (i < 127)
        ctx->token[i++] = *ctx->src_ptr++;
    ctx->token[i] = 0;
    ctx->token_type = 2;
  } else if (*p == '"') {
    ctx->src_ptr++;
    while (*ctx->src_ptr && *ctx->src_ptr != '"')
      if (i < 127)
        ctx->token[i++] = *ctx->src_ptr++;
    if (*ctx->src_ptr == '"')
      ctx->src_ptr++;
    ctx->token[i] = 0;
    ctx->token_type = 4;
  } else {
    if (i < 127)
      ctx->token[i++] = *ctx->src_ptr++;
    ctx->token[i] = 0;
    ctx->token_type = 3;
  }
  if (engine)
    cr_log(engine, CR_LOG_DEBUG, "Token: '%s' (%d)", ctx->token,
           ctx->token_type);
}

static int expr(cr_context *ctx, cr_engine *engine);
static void stmt(cr_context *ctx, cr_engine *engine);

/**
 * @brief Parses factors (numbers, variables, function calls, parens).
 * @param ctx The active context.
 * @param engine The engine instance.
 * @return The node index of the result.
 */
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
        if (!strcmp(ctx->macros[i].name, name)) {
          m_idx = i;
          break;
        }

      if (od) {
        idx = alloc_node(ctx, od);
        if (od->handler == op_delay) {
          int len = (int)(ctx->sample_rate * 4.0);
          ctx->node_pool[idx].dsp.buffer =
              (double *)cr_arena_alloc(ctx, len * sizeof(double));
          ctx->node_pool[idx].dsp.buf_len = len;
        }
        get_tok(ctx, engine);

        if (!strcmp(od->name, "gen") || !strcmp(od->name, "hex")) {
          if (ctx->token_type != 4)
            cr_error(ctx, "Expected string literal");
          idx = alloc_node(ctx, find_op("data"));
          int cnt = 0;
          char *scan = ctx->token;
          if (!strcmp(od->name, "hex")) {
            while (*scan) {
              cnt += 4;
              scan++;
            }
          } else {
            while (*scan) {
              while (isspace(*scan))
                scan++;
              if (!*scan)
                break;
              cnt++;
              while (*scan && !isspace(*scan))
                scan++;
            }
          }
          if (cnt == 0)
            cnt = 1;

          double *buf = (double *)cr_arena_alloc(ctx, cnt * sizeof(double));
          int w_idx = 0;

          if (!strcmp(od->name, "hex")) {
            char *p = ctx->token;
            while (*p) {
              int v = 0;
              if (*p >= '0' && *p <= '9')
                v = *p - '0';
              else if (*p >= 'A' && *p <= 'F')
                v = *p - 'A' + 10;
              else if (*p >= 'a' && *p <= 'f')
                v = *p - 'a' + 10;
              for (i = 3; i >= 0; i--) {
                if (w_idx < cnt)
                  buf[w_idx++] = (v & (1 << i)) ? 1.0 : 0.0;
              }
              p++;
            }
          } else {
            char *p = ctx->token;
            while (*p && w_idx < cnt) {
              while (isspace((unsigned char)*p))
                p++;
              if (!*p)
                break;
              if (isalpha((unsigned char)*p)) {
                char nb[8];
                int k = 0;
                while (isalnum((unsigned char)*p) || *p == '#')
                  nb[k++] = *p++;
                nb[k] = 0;
                buf[w_idx++] = note_to_freq(nb);
              } else {
                char *end;
                double v = strtod(p, &end);
                if (p == end) {
                  p++;
                  continue;
                }
                buf[w_idx++] = v;
                p = end;
              }
            }
          }
          ctx->node_pool[idx].dsp.buffer = buf;
          ctx->node_pool[idx].dsp.buf_len = w_idx;
          get_tok(ctx, engine);
        } else {
          if (strcmp(ctx->token, ")")) {
            while (1) {
              if (ctx->node_pool[idx].input_count < CR_MAX_ARGS)
                ctx->node_pool[idx].inputs[ctx->node_pool[idx].input_count++] =
                    expr(ctx, engine);
              if (strcmp(ctx->token, ","))
                break;
              get_tok(ctx, engine);
            }
          }
        }
        if (strcmp(ctx->token, ")"))
          cr_error(ctx, "Expected ')'");
        get_tok(ctx, engine);
        return idx;
      } else if (m_idx != -1) {
        int args[CR_MAX_ARGS], argc = 0;
        get_tok(ctx, engine);
        if (strcmp(ctx->token, ")")) {
          while (1) {
            if (argc < CR_MAX_ARGS)
              args[argc++] = expr(ctx, engine);
            if (strcmp(ctx->token, ","))
              break;
            get_tok(ctx, engine);
          }
        }
        if (strcmp(ctx->token, ")"))
          cr_error(ctx, "Expected ')'");

        if (ctx->call_depth >= CR_CALL_STACK)
          cr_error(ctx, "Stack overflow");

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
        while (ctx->token_type != 0 && ctx->return_node_idx == -1)
          stmt(ctx, engine);

        idx = (ctx->return_node_idx == -1) ? alloc_node(ctx, find_op("const"))
                                           : ctx->return_node_idx;
        ctx->call_depth--;
        ctx->src_ptr = ctx->call_stack[ctx->call_depth].ret_addr;
        ctx->src_end = ctx->call_stack[ctx->call_depth].ret_end;
        strcpy(ctx->scope, ctx->call_stack[ctx->call_depth].old_scope);

        get_tok(ctx, engine);
        return idx;
      } else {
        cr_error(ctx, "Unknown function");
      }
    } else {
      char scoped[128];
      int i;
      sprintf(scoped, "%s%s", ctx->scope, name);
      for (i = 0; i < ctx->var_count; i++)
        if (!strcmp(ctx->variables[i].name, scoped))
          return ctx->variables[i].node_index;
      for (i = 0; i < ctx->var_count; i++)
        if (!strcmp(ctx->variables[i].name, name))
          return ctx->variables[i].node_index;
      cr_error(ctx, "Unknown variable");
    }
  } else if (!strcmp(ctx->token, "(")) {
    get_tok(ctx, engine);
    idx = expr(ctx, engine);
    if (strcmp(ctx->token, ")"))
      cr_error(ctx, "Expected ')'");
    get_tok(ctx, engine);
    return idx;
  } else if (!strcmp(ctx->token, "-")) {
    int zero;
    get_tok(ctx, engine);
    zero = alloc_node(ctx, find_op("const"));
    idx = alloc_node(ctx, find_op("sub"));
    ctx->node_pool[idx].inputs[0] = zero;
    ctx->node_pool[idx].inputs[1] = factor(ctx, engine);
    ctx->node_pool[idx].input_count = 2;
    return idx;
  }
  cr_error(ctx, "Syntax error");
  return -1;
}

/**
 * @brief Parses terms (multiplication, division, modulo).
 * @param ctx The active context.
 * @param engine The engine instance.
 * @return The node index of the result.
 */
static int term(cr_context *ctx, cr_engine *engine) {
  int left = factor(ctx, engine);
  while (!strcmp(ctx->token, "*") || !strcmp(ctx->token, "/") ||
         !strcmp(ctx->token, "%")) {
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

/**
 * @brief Parses arithmetic expressions (addition, subtraction).
 * @param ctx The active context.
 * @param engine The engine instance.
 * @return The node index of the result.
 */
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

/**
 * @brief Parses logical expressions and basic math.
 * @param ctx The active context.
 * @param engine The engine instance.
 * @return The node index of the result.
 */
static int expr(cr_context *ctx, cr_engine *engine) {
  int left = arith(ctx, engine);
  while (!strcmp(ctx->token, ">") || !strcmp(ctx->token, "<") ||
         !strcmp(ctx->token, "==")) {
    int op =
        !strcmp(ctx->token, ">") ? 30 : (!strcmp(ctx->token, "<") ? 31 : 32);
    int n = alloc_node(ctx, &op_reg[op]);
    get_tok(ctx, engine);
    ctx->node_pool[n].inputs[0] = left;
    ctx->node_pool[n].inputs[1] = arith(ctx, engine);
    ctx->node_pool[n].input_count = 2;
    left = n;
  }
  return left;
}

/**
 * @brief Parses statements (assignments, definitions).
 * @param ctx The active context.
 * @param engine The engine instance.
 */
static void stmt(cr_context *ctx, cr_engine *engine) {
  char name[128];
  int is_param = 0, idx;
  if (ctx->token_type == 0)
    return;

  if (!strcmp(ctx->token, "def")) {
    if (ctx->macro_count >= CR_MAX_MACROS)
      cr_error(ctx, "Max macros");
    cr_macro *m = &ctx->macros[ctx->macro_count++];
    get_tok(ctx, engine);
    strcpy(m->name, ctx->token);
    get_tok(ctx, engine);
    if (strcmp(ctx->token, "("))
      cr_error(ctx, "Expected '('");
    m->arg_count = 0;
    get_tok(ctx, engine);
    if (strcmp(ctx->token, ")")) {
      while (1) {
        if (m->arg_count < CR_MAX_ARGS)
          strcpy(m->args[m->arg_count++], ctx->token);
        get_tok(ctx, engine);
        if (strcmp(ctx->token, ","))
          break;
        get_tok(ctx, engine);
      }
    }
    if (strcmp(ctx->token, ")"))
      cr_error(ctx, "Expected ')'");
    get_tok(ctx, engine);
    if (strcmp(ctx->token, "{"))
      cr_error(ctx, "Expected '{'");

    m->body_start_offset = ctx->src_ptr - ctx->src_base;
    int depth = 1;
    while (*ctx->src_ptr && depth > 0) {
      if (*ctx->src_ptr == '{')
        depth++;
      if (*ctx->src_ptr == '}')
        depth--;
      if (depth == 0)
        break;
      ctx->src_ptr++;
    }
    m->body_end_offset = ctx->src_ptr - ctx->src_base;
    if (*ctx->src_ptr == '}')
      ctx->src_ptr++;
    get_tok(ctx, engine);
    return;
  }

  if (!strcmp(ctx->token, "return")) {
    get_tok(ctx, engine);
    ctx->return_node_idx = expr(ctx, engine);
    return;
  }

  if (ctx->token_type != 1) {
    if (strcmp(ctx->token, ";"))
      cr_error(ctx, "Unexpected token");
    get_tok(ctx, engine);
    return;
  }
  sprintf(name, "%s%s", ctx->scope, ctx->token);
  char raw_name[64];
  strcpy(raw_name, ctx->token);
  get_tok(ctx, engine);

  if (!strcmp(ctx->token, ":")) {
    is_param = 1;
    get_tok(ctx, engine);
    if (strcmp(ctx->token, "="))
      cr_error(ctx, "Expected '='");
  } else if (strcmp(ctx->token, "="))
    cr_error(ctx, "Expected '='");
  get_tok(ctx, engine);

  idx = expr(ctx, engine);

  if (!strcmp(raw_name, "out")) {
    ctx->output_nodes[0] = idx;
    ctx->output_nodes[1] = idx;
  } else if (!strcmp(raw_name, "bpm")) {
    ctx->bpm_node_idx = idx;
  }

  {
    int i, found = 0;
    for (i = 0; i < ctx->var_count; i++)
      if (!strcmp(ctx->variables[i].name, name)) {
        if (is_param) {
          int p = alloc_node(ctx, find_op("param"));
          ctx->node_pool[p].inputs[0] = idx;
          ctx->node_pool[p].input_count = 1;
          ctx->node_pool[p].dsp.s[0] = as_float(ctx->node_pool[idx].value);
          ctx->variables[i].node_index = p;
        } else {
          ctx->variables[i].node_index = idx;
        }
        found = 1;
        break;
      }
    if (!found && ctx->var_count < CR_MAX_SYMBOLS) {
      strcpy(ctx->variables[ctx->var_count].name, name);
      if (is_param) {
        int p = alloc_node(ctx, find_op("param"));
        ctx->node_pool[p].inputs[0] = idx;
        ctx->node_pool[p].input_count = 1;
        ctx->node_pool[p].dsp.s[0] = as_float(ctx->node_pool[idx].value);
        ctx->variables[ctx->var_count].node_index = p;
      } else {
        ctx->variables[ctx->var_count].node_index = idx;
      }
      ctx->var_count++;
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
    e->contexts[i].arena_base =
        (unsigned char *)e->memory_block + (i * CR_ARENA_SIZE);
    e->contexts[i].arena_size = CR_ARENA_SIZE;
    e->contexts[i].output_nodes[0] = -1;
    e->contexts[i].output_nodes[1] = -1;
    e->contexts[i].transport.bpm = 120.0;
    e->contexts[i].transport.phase = 0.0;
    e->contexts[i].transport.playing = 1;
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
  c->global_time++;

  if (c->transport.playing) {
    if (c->transport.bpm < 10.0)
      c->transport.bpm = 10.0;
    c->transport.samples_per_beat = c->sample_rate * 60.0 / c->transport.bpm;
    c->transport.phase += 1.0 / c->transport.samples_per_beat;
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
  for (int c_idx = 0; c_idx < CR_MAX_CHANNELS; c_idx++)
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

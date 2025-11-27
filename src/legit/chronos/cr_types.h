#ifndef CR_TYPES_H
#define CR_TYPES_H

#include <setjmp.h>
#include <stddef.h>
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
#include <termios.h>
#include <unistd.h>
#endif

#ifndef CR_API
#define CR_API
#endif

#define CR_MAX_CHANNELS 2
#define CR_MAX_SYMBOLS 1024
#define CR_MAX_NODES 16384
#define CR_MAX_ARGS 32
#define CR_MAX_FUNCS 256
#define CR_MAX_MACROS 256
#define CR_MAX_MACRO_SIZE 8192
#define CR_MAX_BLOCKS 64
#define CR_PI 3.14159265359
#define CR_ARENA_SIZE (64 * 1024 * 1024) 

typedef enum {
  CR_LOG_NONE = 0,
  CR_LOG_ERROR,
  CR_LOG_WARN,
  CR_LOG_INFO,
  CR_LOG_DEBUG
} cr_log_level;

enum {
  OP_HALT = 0,
  OP_CONST, OP_LOAD, OP_STORE,
  OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, OP_POW,
  OP_GT, OP_LT, OP_GE, OP_LE, OP_EQ, OP_NE, OP_AND, OP_OR, OP_NOT,
  OP_BIT_AND, OP_BIT_OR, OP_BIT_XOR, OP_BIT_NOT, OP_BIT_LSHIFT, OP_BIT_RSHIFT,
  OP_SINE, OP_PHASOR, OP_NOISE,
  OP_SEQ, OP_PATTERN, OP_FILTER, OP_PEAK_EQ, 
  OP_DELAY, OP_REVERB, OP_COMPRESS, OP_LIMIT, OP_ADSR, OP_CLIP,
  OP_TANH, OP_FOLD, OP_SLEW, OP_SAH,
  OP_TIME, OP_BARS, OP_BRANCH_CTRL, OP_SELECT,
  OP_FLOOR, OP_CEIL, OP_ABS, OP_SIGN, OP_MTOF, OP_LERP, OP_CLAMP, OP_MAP,
  OP_SQUARE, OP_SQRT, OP_EXP, OP_LOG, OP_ATAN2, OP_HPF, OP_DIFF, OP_INTEGRATE
};

#define OP_COUNT OP_INTEGRATE + 1

typedef enum { CR_FLOAT, CR_INT } cr_type_t;

typedef struct cr_val {
    cr_type_t type;
    union { double f; int i; } as;
} cr_val;

typedef struct {
    double s[16]; 
    double* buffer;
    int buf_len;
    int write_head;
} cr_dsp_state;

typedef struct cr_op_desc cr_op_desc;

typedef struct cr_node {
  int id;
  const cr_op_desc *op_desc;
  int inputs[CR_MAX_ARGS];
  int input_count;
  cr_val value;
  int is_constant;
  cr_dsp_state dsp;
  int i_data;
  int block_id;
  int control_node;
} cr_node;

typedef struct {
  char name[64];
  int node_index;
} cr_variable;

typedef struct {
  char name[32];
  char args[CR_MAX_ARGS][32];
  int arg_count;
  char body[CR_MAX_MACRO_SIZE];
} cr_macro;

struct cr_context;
struct cr_node;
struct cr_val;

typedef struct cr_context {
  long global_time;
  int output_nodes[CR_MAX_CHANNELS];
  double sample_rate; 
  
  cr_variable variables[CR_MAX_SYMBOLS];
  int var_count;
  
  cr_macro macros[CR_MAX_MACROS];
  int macro_count;
  
  cr_node node_pool[CR_MAX_NODES];
  int node_idx;
  
  int exec_order[CR_MAX_NODES];
  int exec_count;
  unsigned char visit_state[CR_MAX_NODES]; 
  
  unsigned char *arena_base;
  size_t arena_size;
  size_t arena_top;

  char scope[128];
  int scope_id_ctr;
  int return_reg;
  int returning;
  const char* src_ptr;
  char token[128];
  int token_type;
  jmp_buf err_jmp;
  char error_msg[128];
  int current_line;
  
  int bpm_node_idx;

  int active_block_stack[16]; 
  int active_block_ptr;
  int block_dependency_stack[16]; 
  int block_id_counter;
  unsigned char block_skip_flags[CR_MAX_BLOCKS]; 
} cr_context;

// FIX 1: Define cr_engine here so cr_vm.h can see it.
typedef struct cr_engine {
    cr_context contexts[2];
    cr_context *active;
    cr_context *back;
    void *memory_block;
    char *source_history;
    size_t source_capacity;
    size_t source_len;
    int log_level;
    #if defined(_WIN32)
    CRITICAL_SECTION swap_lock;
    #else
    pthread_mutex_t swap_lock;
    #endif
} cr_engine;


#endif

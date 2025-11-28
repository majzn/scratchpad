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

/**
 * @def CR_API
 * @brief Macro for API visibility (left empty in this implementation).
 */
#ifndef CR_API
#define CR_API
#endif

/** @def CR_MAX_CHANNELS
 * @brief Maximum number of output audio channels (e.g., stereo).
 */
#define CR_MAX_CHANNELS 2

/** @def CR_MAX_SYMBOLS
 * @brief Maximum number of script variables allowed in a context.
 */
#define CR_MAX_SYMBOLS 1024

/** @def CR_MAX_NODES
 * @brief Maximum number of nodes (DSP operations/values) in the DSP graph.
 */
#define CR_MAX_NODES 16384

/** @def CR_MAX_ARGS
 * @brief Maximum number of input arguments for a single DSP node/operation.
 */
#define CR_MAX_ARGS 32

/** @def CR_MAX_FUNCS
 * @brief Maximum number of available built-in DSP functions/operators.
 */
#define CR_MAX_FUNCS 256

/** @def CR_MAX_MACROS
 * @brief Maximum number of user-defined macros.
 */
#define CR_MAX_MACROS 256

/** @def CR_MAX_MACRO_SIZE
 * @brief Maximum size (in bytes) of a macro's body script.
 */
#define CR_MAX_MACRO_SIZE 8192

/** @def CR_MAX_BLOCKS
 * @brief Maximum number of conditional blocks (e.g., 'if' statements) for execution control.
 */
#define CR_MAX_BLOCKS 64

/** @def CR_PI
 * @brief Mathematical constant Pi.
 */
#define CR_PI 3.14159265359

/** @def CR_ARENA_SIZE
 * @brief Total size (in bytes) of the memory arena allocated per context for DSP buffers.
 */
#define CR_ARENA_SIZE (64 * 1024 * 1024) 

/**
 * @enum cr_log_level
 * @brief Logging verbosity levels.
 */
typedef enum {
  CR_LOG_NONE = 0, /**< No logging. */
  CR_LOG_ERROR,    /**< Only critical errors. */
  CR_LOG_WARN,     /**< Warnings and non-critical issues. */
  CR_LOG_INFO,     /**< General information (e.g., successful compilation). */
  CR_LOG_DEBUG     /**< Verbose debug output. */
} cr_log_level;

/**
 * @enum Anonymous Operations
 * @brief Opcodes for all Digital Signal Processing (DSP) and control operations.
 */
enum {
  OP_HALT = 0,    /**< Stop execution (reserved). */
  OP_CONST,       /**< A constant numeric value. */
  OP_LOAD,        /**< Load a value from a variable (reserved). */
  OP_STORE,       /**< Store a value to a variable (reserved). */
  OP_ADD,         /**< Addition (+). */
  OP_SUB,         /**< Subtraction (-). */
  OP_MUL,         /**< Multiplication (*). */
  OP_DIV,         /**< Division (/). */
  OP_MOD,         /**< Modulo (%). */
  OP_POW,         /**< Power (pow). */
  OP_GT,          /**< Greater Than (>). */
  OP_LT,          /**< Less Than (<). */
  OP_GE,          /**< Greater than or Equal To (>=). */
  OP_LE,          /**< Less than or Equal To (<=). */
  OP_EQ,          /**< Equal To (==). */
  OP_NE,          /**< Not Equal To (!=). */
  OP_AND,         /**< Logical AND (&&). */
  OP_OR,          /**< Logical OR (||). */
  OP_NOT,         /**< Logical NOT (!). */
  OP_BIT_AND,     /**< Bitwise AND (&). */
  OP_BIT_OR,      /**< Bitwise OR (|). */
  OP_BIT_XOR,     /**< Bitwise XOR (^). */
  OP_BIT_NOT,     /**< Bitwise NOT (~). */
  OP_BIT_LSHIFT,  /**< Bitwise Left Shift (<<). */
  OP_BIT_RSHIFT,  /**< Bitwise Right Shift (>>). */
  OP_SINE,        /**< Sine wave oscillator from phase input (sin). */
  OP_PHASOR,      /**< Phasor (sawtooth 0-1) oscillator from frequency input. */
  OP_NOISE,       /**< Random noise generator (-1 to 1). */
  OP_SEQ,         /**< Sequencer/step-sequencer. */
  OP_PATTERN,     /**< Rhythm pattern generator (string input). */
  OP_FILTER,      /**< State Variable Filter (SVF) with LP, HP, BP modes. */
  OP_PEAK_EQ,     /**< Peaking Equalizer filter. */ 
  OP_DELAY,       /**< Time-based delay effect with feedback. */
  OP_REVERB,      /**< All-pass/comb filter-based reverb effect. */
  OP_COMPRESS,    /**< Dynamic Range Compressor. */
  OP_LIMIT,       /**< Peak Limiter. */
  OP_ADSR,        /**< Attack-Decay-Sustain-Release envelope generator. */
  OP_CLIP,        /**< Hard clipping to -1.0 to 1.0. */
  OP_TANH,        /**< Hyperbolic Tangent (tanh) soft clipping/distortion. */
  OP_FOLD,        /**< Wavefolding distortion. */
  OP_SLEW,        /**< Slew limiter (rate limiter). */
  OP_SAH,         /**< Sample and Hold. */
  OP_TIME,        /**< Returns current time in seconds. */
  OP_BARS,        /**< Returns current time in musical bars based on BPM. */
  OP_BRANCH_CTRL, /**< Internal node for conditional execution (if/else). */
  OP_SELECT,      /**< Internal node for selecting one of two input values based on a condition. */
  OP_FLOOR,       /**< Floor function. */
  OP_CEIL,        /**< Ceiling function. */
  OP_ABS,         /**< Absolute value (fabs). */
  OP_SIGN,        /**< Sign function. */
  OP_MTOF,        /**< MIDI note to Frequency conversion. */
  OP_LERP,        /**< Linear interpolation. */
  OP_CLAMP,       /**< Clamp a value between a minimum and maximum. */
  OP_MAP,         /**< Map a value from one range to another. */
  OP_SQUARE,      /**< Square (x*x). */
  OP_SQRT,        /**< Square root (sqrt). */
  OP_EXP,         /**< Exponential (exp). */
  OP_LOG,         /**< Natural logarithm (log). */
  OP_ATAN2,       /**< Arc tangent of two variables (atan2). */
  OP_HPF,         /**< Simple High Pass Filter. */
  OP_DIFF,        /**< Difference between current and previous sample. */
  OP_INTEGRATE    /**< Accumulator/Integrator. */
};

/** @def OP_COUNT
 * @brief Total number of defined opcodes.
 */
#define OP_COUNT OP_INTEGRATE + 1

/**
 * @enum cr_type_t
 * @brief Data types used for node values.
 */
typedef enum { CR_FLOAT, CR_INT } cr_type_t;

/**
 * @struct cr_val
 * @brief A container for a value, which can be either a floating-point number or an integer.
 */
typedef struct cr_val {
    cr_type_t type; /**< The active type of the value (CR_FLOAT or CR_INT). */
    union { 
      double f;     /**< Value when type is CR_FLOAT (used for all DSP). */
      int i;        /**< Value when type is CR_INT. */
    } as;           /**< Union to hold the raw value. */
} cr_val;

/**
 * @struct cr_dsp_state
 * @brief State variables required for a single DSP node to maintain continuity over time.
 */
typedef struct {
    double s[16];   /**< Small array for storing various state parameters (e.g., filter coefficients, envelope stage, phasor phase). */
    double* buffer; /**< Pointer to a dynamically allocated memory buffer (e.g., for delay lines, reverb tanks). */
    int buf_len;    /**< Length of the allocated buffer. */
    int write_head; /**< Write index for circular buffers. */
} cr_dsp_state;

struct cr_op_desc;
/**
 * @typedef cr_op_desc
 * @brief Forward declaration for the operation description structure defined in cr_op_registry.h.
 */
typedef struct cr_op_desc cr_op_desc;

/**
 * @struct cr_node
 * @brief Represents a single element in the DSP graph (a variable, constant, or operation).
 */
typedef struct cr_node {
  int id;                     /**< Unique ID (index in the node_pool array). */
  const cr_op_desc *op_desc;  /**< Pointer to the operation metadata and handler function. */
  int inputs[CR_MAX_ARGS];    /**< Array of node IDs that feed into this node. */
  int input_count;            /**< Number of actual inputs used. */
  cr_val value;               /**< The computed output value of this node for the current sample. */
  int is_constant;            /**< Flag: 1 if this node is a constant and does not need execution. */
  cr_dsp_state dsp;           /**< DSP state variables (phase, filter history, buffers). */
  int i_data;                 /**< Auxiliary integer data (currently unused). */
  int block_id;               /**< ID of the conditional block this node belongs to (0 for global). */
  int control_node;           /**< The ID of the OP_BRANCH_CTRL node that determines if this node runs. */
} cr_node;

/**
 * @struct cr_variable
 * @brief Defines a script variable and its corresponding node in the graph.
 */
typedef struct {
  char name[64];        /**< The variable name (may include scope prefix). */
  int node_index;       /**< The ID of the cr_node that holds the variable's value. */
} cr_variable;

/**
 * @struct cr_macro
 * @brief Defines a user-defined macro/function in the script.
 */
typedef struct {
  char name[32];                        /**< Macro name. */
  char args[CR_MAX_ARGS][32];           /**< Array of argument names. */
  int arg_count;                        /**< Number of arguments. */
  char body[CR_MAX_MACRO_SIZE];         /**< The raw script content of the macro's body. */
} cr_macro;

struct cr_context;
struct cr_node;
struct cr_val;

/**
 * @struct cr_context
 * @brief The full state of the Chronos Virtual Machine and DSP Graph.
 *
 * The engine uses two contexts for hot-swapping (double-buffering the state).
 */
typedef struct cr_context {
  long global_time;                   /**< Current sample index since the start of execution. */
  int output_nodes[CR_MAX_CHANNELS];  /**< Node IDs that represent the final audio output for each channel. */
  double sample_rate;                 /**< Audio sample rate (e.g., 48000.0). */
  
  cr_variable variables[CR_MAX_SYMBOLS]; /**< Array of script variables. */
  int var_count;                      /**< Current number of variables. */
  
  cr_macro macros[CR_MAX_MACROS];     /**< Array of user-defined macros. */
  int macro_count;                    /**< Current number of macros. */
  
  cr_node node_pool[CR_MAX_NODES];    /**< The actual DSP graph nodes. */
  int node_idx;                       /**< Index of the next available node slot. */
  
  int exec_order[CR_MAX_NODES];       /**< Topological sort: Order in which nodes must be executed. */
  int exec_count;                     /**< Number of nodes in the execution order list. */
  unsigned char visit_state[CR_MAX_NODES]; /**< Temporary state used during topological sort. */
  
  unsigned char *arena_base;          /**< Base pointer for the memory arena. */
  size_t arena_size;                  /**< Total size of the memory arena. */
  size_t arena_top;                   /**< Current allocation pointer within the arena. */

  char scope[128];                    /**< Current variable scope prefix for macro execution. */
  int scope_id_ctr;                   /**< Counter for generating unique scope IDs. */
  int return_reg;                     /**< Node ID of the return value during macro execution. */
  int returning;                      /**< Flag indicating if the parser is currently returning from a macro. */
  const char* src_ptr;                /**< Current position in the source script string (parser state). */
  char token[128];                    /**< Current token parsed by the lexer. */
  int token_type;                     /**< Type of the current token (e.g., identifier, number, operator). */
  jmp_buf err_jmp;                    /**< Jump buffer for longjmp-based error handling. */
  char error_msg[128];                /**< Buffer to store the current error message. */
  int current_line;                   /**< Current line number in the script for error reporting. */
  
  int bpm_node_idx;                   /**< Node ID holding the current BPM value. */

  int active_block_stack[16];         /**< Stack of active conditional block IDs. */
  int active_block_ptr;               /**< Stack pointer for active_block_stack. */
  int block_dependency_stack[16];     /**< Stack of control node IDs for conditional dependencies. */
  int block_id_counter;               /**< Counter for assigning unique block IDs. */
  unsigned char block_skip_flags[CR_MAX_BLOCKS]; /**< Runtime flags indicating which blocks to skip execution. */
} cr_context;

/**
 * @struct cr_engine
 * @brief The top-level container for the Chronos system, managing two contexts for hot-swapping.
 */
typedef struct cr_engine {
    cr_context contexts[2]; /**< The two context buffers for double-buffering the DSP state. */
    cr_context *active;     /**< Pointer to the currently executing context. */
    cr_context *back;       /**< Pointer to the context used for compilation/hot-swap. */
    void *memory_block;     /**< The base memory allocation for both contexts' arenas. */
    char *source_history;   /**< Buffer storing the entire script history/source code. */
    size_t source_capacity; /**< Capacity of the source_history buffer. */
    size_t source_len;      /**< Current length of the source script. */
    int log_level;          /**< The engine's current logging verbosity level. */
    #if defined(_WIN32)
    CRITICAL_SECTION swap_lock; /**< Windows synchronization primitive for context swap. */
    #else
    pthread_mutex_t swap_lock;  /**< POSIX mutex for context swap synchronization. */
    #endif
} cr_engine;


#endif

/**
 * @file cr_op_registry.h
 * @brief Defines the structure and registry for all Digital Signal Processing (DSP) operations.
 *
 * This file declares the function pointers, the operation descriptor structure (cr_op_desc),
 * and the mechanism for looking up an operation by its name. It acts as the central
 * mapping between script function names and their corresponding execution handlers.
 */
#ifndef CR_OP_REGISTRY_H
#define CR_OP_REGISTRY_H

#include "cr_types.h"

struct cr_context;
struct cr_node;
struct cr_val;

/**
 * @brief Function pointer type for the core DSP execution handler of an operation.
 *
 * This function is called by the VM for every sample of every active node
 * in the execution graph. It takes the context, the node itself (for state),
 * and the pre-computed input values.
 *
 * @param ctx The current execution context (cr_context).
 * @param n The node currently being executed (cr_node).
 * @param inputs An array of cr_val containing the computed values of the node's inputs.
 * @return The result of the operation as a cr_val.
 */
typedef cr_val (*cr_op_func)(struct cr_context *ctx, struct cr_node *n, cr_val *inputs);

/**
 * @brief Function pointer type for validating the number of inputs during parsing.
 *
 * This is called during the script compilation phase to ensure an operation
 * is called with the correct number of arguments.
 *
 * @param ctx The current compilation context (cr_context).
 * @param op_name The name of the operation being validated.
 * @param input_count The number of input nodes provided by the parser.
 * @return 1 if validation passes, 0 if it fails (and should trigger a compile error).
 */
typedef int (*cr_input_validator)(struct cr_context *ctx, const char *op_name, int input_count);

/**
 * @struct cr_op_desc
 * @brief Descriptor structure for a single DSP operation.
 *
 * This structure links the script-facing name, the internal opcode, and the
 * function handlers for execution and compilation validation.
 */
typedef struct cr_op_desc {
  const char *name;           /**< The name of the operation as used in the script (e.g., "sine", "add"). */
  int opcode;                 /**< The unique operation code (OP_SINE, OP_ADD, etc. from cr_types.h). */
  cr_op_func handler;         /**< The function to execute this operation at runtime. */
  cr_input_validator validator; /**< The function to validate input count during parsing. */
} cr_op_desc;

/**
 * @brief Looks up an operation descriptor by its script name.
 * @param name The script name of the operation (e.g., "sine").
 * @return A constant pointer to the cr_op_desc structure, or NULL if not found.
 */
CR_API const cr_op_desc *cr_lookup_op_by_name(const char *name);

#ifdef CR_OP_REGISTRY_IMPLEMENTATION

/**
 * @brief Looks up an operation descriptor by its script name.
 * @param name The script name of the operation (e.g., "sine").
 * @return A constant pointer to the cr_op_desc structure, or NULL if not found.
 */
CR_API const cr_op_desc *cr_lookup_op_by_name(const char *name);

/** --- Operation Handler Declarations (Implemented in cr_vm.h and cr_dsp.h) --- */

extern cr_val op_handler_default(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_const(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_branch_ctrl(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_select(struct cr_context *ctx, struct cr_node *n, cr_val *v);

/** Arithmetic Handlers */
extern cr_val op_handler_add(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_sub(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_mul(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_div(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_mod(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_pow(struct cr_context *ctx, struct cr_node *n, cr_val *v);

/** Comparison Handlers */
extern cr_val op_handler_gt(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_lt(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_ge(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_le(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_eq(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_ne(struct cr_context *ctx, struct cr_node *n, cr_val *v);

/** Logical Handlers */
extern cr_val op_handler_and(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_or(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_not(struct cr_context *ctx, struct cr_node *n, cr_val *v);

/** Bitwise Handlers */
extern cr_val op_handler_bit_and(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_bit_or(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_bit_xor(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_bit_not(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_bit_lshift(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_bit_rshift(struct cr_context *ctx, struct cr_node *n, cr_val *v);

/** Oscillator/Source Handlers */
extern cr_val op_handler_sine(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_phasor(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_noise(struct cr_context *ctx, struct cr_node *n, cr_val *v);

/** Sequencing/Timing Handlers */
extern cr_val op_handler_seq(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_time(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_bars(struct cr_context *ctx, struct cr_node *n, cr_val *v);

/** Filter/EQ Handlers */
extern cr_val op_handler_filter(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_peakeq(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_hpf(struct cr_context *ctx, struct cr_node *n, cr_val *v);

/** Dynamics/Shapers/Envelope Handlers */
extern cr_val op_handler_delay(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_reverb(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_compress(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_limit(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_adsr(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_clip(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_tanh(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_fold(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_slew(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_sah(struct cr_context *ctx, struct cr_node *n, cr_val *v);

/** Utility/Math Handlers */
extern cr_val op_handler_floor(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_ceil(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_abs(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_sign(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_mtof(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_lerp(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_clamp(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_map(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_square(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_sqrt(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_exp(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_log(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_atan2(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_diff(struct cr_context *ctx, struct cr_node *n, cr_val *v);
extern cr_val op_handler_integrate(struct cr_context *ctx, struct cr_node *n, cr_val *v);

/**
 * @brief Default input validator that always returns success.
 * @param ctx The current context.
 * @param op_name The operation name.
 * @param input_count The number of inputs provided.
 * @return Always returns 1 (success).
 */
static int validate_none(struct cr_context *ctx, const char *op_name, int input_count) {
    return 1;
}

static const cr_op_desc op_registry[] = {
    {"halt", OP_HALT, op_handler_default, validate_none},
    {"const", OP_CONST, op_handler_const, validate_none},
    {"load", OP_LOAD, op_handler_default, validate_none},
    {"store", OP_STORE, op_handler_default, validate_none},
    {"add", OP_ADD, op_handler_add, validate_none},
    {"sub", OP_SUB, op_handler_sub, validate_none},
    {"mul", OP_MUL, op_handler_mul, validate_none},
    {"div", OP_DIV, op_handler_div, validate_none},
    {"mod", OP_MOD, op_handler_mod, validate_none},
    {"pow", OP_POW, op_handler_pow, validate_none},
    {"gt", OP_GT, op_handler_gt, validate_none},
    {"lt", OP_LT, op_handler_lt, validate_none},
    {"ge", OP_GE, op_handler_ge, validate_none},
    {"le", OP_LE, op_handler_le, validate_none},
    {"eq", OP_EQ, op_handler_eq, validate_none},
    {"ne", OP_NE, op_handler_ne, validate_none},
    {"and", OP_AND, op_handler_and, validate_none},
    {"or", OP_OR, op_handler_or, validate_none},
    {"not", OP_NOT, op_handler_not, validate_none},
    {"bitand", OP_BIT_AND, op_handler_bit_and, validate_none},
    {"bitor", OP_BIT_OR, op_handler_bit_or, validate_none},
    {"bitxor", OP_BIT_XOR, op_handler_bit_xor, validate_none},
    {"bitnot", OP_BIT_NOT, op_handler_bit_not, validate_none},
    {"bitshl", OP_BIT_LSHIFT, op_handler_bit_lshift, validate_none},
    {"bitshr", OP_BIT_RSHIFT, op_handler_bit_rshift, validate_none},
    {"sine", OP_SINE, op_handler_sine, validate_none},
    {"phasor", OP_PHASOR, op_handler_phasor, validate_none},
    {"noise", OP_NOISE, op_handler_noise, validate_none},
    {"seq", OP_SEQ, op_handler_seq, validate_none},
    {"rhythm", OP_PATTERN, op_handler_default, validate_none},
    {"filter", OP_FILTER, op_handler_filter, validate_none},
    {"peakeq", OP_PEAK_EQ, op_handler_peakeq, validate_none},
    {"delay", OP_DELAY, op_handler_delay, validate_none},
    {"reverb", OP_REVERB, op_handler_reverb, validate_none},
    {"compress", OP_COMPRESS, op_handler_compress, validate_none},
    {"limit", OP_LIMIT, op_handler_limit, validate_none},
    {"adsr", OP_ADSR, op_handler_adsr, validate_none},
    {"clip", OP_CLIP, op_handler_clip, validate_none},
    {"tanh", OP_TANH, op_handler_tanh, validate_none},
    {"fold", OP_FOLD, op_handler_fold, validate_none},
    {"slew", OP_SLEW, op_handler_slew, validate_none},
    {"sah", OP_SAH, op_handler_sah, validate_none},
    {"time", OP_TIME, op_handler_time, validate_none},
    {"bars", OP_BARS, op_handler_bars, validate_none},
    {"branch_ctrl", OP_BRANCH_CTRL, op_handler_branch_ctrl, validate_none},
    {"select", OP_SELECT, op_handler_select, validate_none},
    {"floor", OP_FLOOR, op_handler_floor, validate_none},
    {"ceil", OP_CEIL, op_handler_ceil, validate_none},
    {"abs", OP_ABS, op_handler_abs, validate_none},
    {"sign", OP_SIGN, op_handler_sign, validate_none},
    {"mtof", OP_MTOF, op_handler_mtof, validate_none},
    {"lerp", OP_LERP, op_handler_lerp, validate_none},
    {"clamp", OP_CLAMP, op_handler_clamp, validate_none},
    {"map", OP_MAP, op_handler_map, validate_none},
    {"square", OP_SQUARE, op_handler_square, validate_none},
    {"sqrt", OP_SQRT, op_handler_sqrt, validate_none},
    {"exp", OP_EXP, op_handler_exp, validate_none},
    {"log", OP_LOG, op_handler_log, validate_none},
    {"atan2", OP_ATAN2, op_handler_atan2, validate_none},
    {"hpf", OP_HPF, op_handler_hpf, validate_none},
    {"diff", OP_DIFF, op_handler_diff, validate_none},
    {"integrate", OP_INTEGRATE, op_handler_integrate, validate_none}
};

static const int op_registry_size = sizeof(op_registry) / sizeof(op_registry[0]);

CR_API const cr_op_desc *cr_lookup_op_by_name(const char *name) {
    int i;
    for (i = 0; i < op_registry_size; i++) {
        if (strcmp(op_registry[i].name, name) == 0) {
            return &op_registry[i];
        }
    }
    return NULL;
}

#endif

#endif

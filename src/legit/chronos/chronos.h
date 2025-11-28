/**
 * @file chronos.h
 * @brief Umbrella header for the Chronos music scripting engine.
 *
 * This header enables the single-file compilation model by defining the
 * necessary implementation macros and including the core headers.
 * When CHRONOS_IMPLEMENTATION is defined, this file brings in the
 * source code for the VM, Parser, Op Registry, Utilities, and DSP modules.
 */
#ifdef CHRONOS_IMPLEMENTATION
#define CR_VM_IMPLEMENTATION
#define CR_PARSER_IMPLEMENTATION
#define CR_OP_REGISTRY_IMPLEMENTATION
#define CR_UTILS_IMPLEMENTATION
#define CR_DSP_IMPLEMENTATION

/**
 * @brief Includes the parser header, which in turn includes cr_vm.h
 * which includes the other core components.
 */
#include "cr_parser.h"

#endif

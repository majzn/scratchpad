/**
 * @file cr_utils.h
 * @brief Utility functions for creating and casting the cr_val type.
 *
 * This header provides simple, inline-defined functions to safely wrap C primitive
 * types (double, int) into the engine's internal cr_val structure, and to safely
 * extract them, handling type coercion where necessary.
 *
 * It uses the standard single-header library pattern where function definitions
 * are included only if CR_UTILS_IMPLEMENTATION is defined.
 */
#ifndef CR_UTILS_H
#define CR_UTILS_H
#include "cr_types.h"

/**
 * @brief Creates a cr_val structure initialized as CR_FLOAT.
 * @param f The double value to wrap.
 * @return A cr_val structure with type set to CR_FLOAT.
 */
static cr_val make_float(double f);

/**
 * @brief Creates a cr_val structure initialized as CR_INT.
 * @param i The int value to wrap.
 * @return A cr_val structure with type set to CR_INT.
 */
static cr_val make_int(int i);

/**
 * @brief Safely extracts the floating-point representation of a cr_val.
 *
 * If the input cr_val is CR_INT, it is promoted to a double.
 * If the input cr_val is CR_FLOAT, the raw double value is returned.
 *
 * @param v The cr_val structure to extract from.
 * @return The value as a double.
 */
static double as_float(cr_val v);

/**
 * @brief Safely extracts the integer representation of a cr_val.
 *
 * If the input cr_val is CR_FLOAT, it is cast to an int (truncating the decimal part).
 * If the input cr_val is CR_INT, the raw int value is returned.
 *
 * @param v The cr_val structure to extract from.
 * @return The value as an int.
 */
static int as_int(cr_val v);

#ifdef CR_UTILS_IMPLEMENTATION
static cr_val make_float(double f) { cr_val v; v.type=CR_FLOAT; v.as.f=f; return v; }
static cr_val make_int(int i) { cr_val v; v.type=CR_INT; v.as.i=i; return v; }
static double as_float(cr_val v) { return (v.type==CR_INT)?(double)v.as.i : v.as.f; }
static int as_int(cr_val v) { return (v.type==CR_INT)?v.as.i : (int)v.as.f; }
#endif

#endif

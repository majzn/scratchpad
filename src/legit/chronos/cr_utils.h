#ifndef CR_UTILS_H
#define CR_UTILS_H
#include "cr_types.h"
static cr_val make_float(double f);
static cr_val make_int(int i);
static double as_float(cr_val v);
static int as_int(cr_val v);
#ifdef CR_UTILS_IMPLEMENTATION
static cr_val make_float(double f) { cr_val v; v.type=CR_FLOAT; v.as.f=f; return v; }
static cr_val make_int(int i) { cr_val v; v.type=CR_INT; v.as.i=i; return v; }
static double as_float(cr_val v) { return (v.type==CR_INT)?(double)v.as.i : v.as.f; }
static int as_int(cr_val v) { return (v.type==CR_INT)?v.as.i : (int)v.as.f; }
#endif
#endif

/* pc_detmath.h - deterministic libm/rand/qsort for game code (docs/detmath.md).
 *
 * Force-included (-include) into every game TU (decomp sources, the
 * src/game/shim layer, and gx_core.c whose matrices the game reads back).
 * It pulls in the system <math.h>/<stdlib.h> first -- they are include-
 * guarded, so later includes are no-ops -- and then maps the C runtime's
 * libm, rand, srand, qsort and atof to the port's own implementations
 * (src/detmath), which give bit-identical results on every compiler, CRT
 * and CPU (and follow the GameCube's MSL where it matters).
 *
 * Math names are function-like macros, so variables and struct fields
 * called "floor" or "tan" in the decomp are untouched.  A transcendental
 * function the port does not implement maps to an undefined symbol
 * (pc_libm_missing_*), so new uses fail at link time instead of silently
 * going back to the CRT.
 *
 * Define PC_DETMATH_NO_MAP before including to get only the declarations. */
#ifndef PC_DETMATH_H
#define PC_DETMATH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

double pc_sin(double x);
double pc_cos(double x);
double pc_tan(double x);
double pc_asin(double x);
double pc_acos(double x);
double pc_atan(double x);
double pc_atan2(double y, double x);
double pc_sqrt(double x);
double pc_pow(double x, double y);
double pc_floor(double x);
double pc_ceil(double x);
long pc_lrint(double x);

float pc_sinf(float x);
float pc_cosf(float x);
float pc_tanf(float x);
float pc_asinf(float x);
float pc_acosf(float x);
float pc_atanf(float x);
float pc_atan2f(float y, float x);
float pc_sqrtf(float x);
float pc_powf(float x, float y);
float pc_floorf(float x);
float pc_ceilf(float x);
long pc_lrintf(float x);

int pc_rand(void);
void pc_srand(unsigned int seed);
double pc_atof(const char *s);
#ifdef PC_DETMATH_NO_MAP
void pc_qsort(void *base, uint32_t count, uint32_t size,
              int (*compare)(const void *, const void *));
#else
/* Unprototyped on purpose: the decomp declares qsort itself with several
 * GC-era signatures (int/u32 counts, void * comparator); all of them are
 * compatible with this declaration.  Definition: pc_libc.c (32-bit counts,
 * like the GC's; the callers pass int/u32). */
void pc_qsort();
#endif

#ifdef __cplusplus
}
#endif

#if !defined(PC_DETMATH_NO_MAP) && !defined(__MWERKS__)

#if defined(__FAST_MATH__)
#error "game code must not be built with -ffast-math (docs/detmath.md)"
#endif
#if defined(__FLT_EVAL_METHOD__) && __FLT_EVAL_METHOD__ != 0
#error "game code needs FLT_EVAL_METHOD 0 (x86: -msse2 -mfpmath=sse), docs/detmath.md"
#endif

#include <math.h>
#include <stdlib.h>

#undef sin
#undef cos
#undef tan
#undef asin
#undef acos
#undef atan
#undef atan2
#undef sqrt
#undef pow
#undef floor
#undef ceil
#undef lrint
#undef sinf
#undef cosf
#undef tanf
#undef asinf
#undef acosf
#undef atanf
#undef atan2f
#undef sqrtf
#undef powf
#undef floorf
#undef ceilf
#undef lrintf

#define sin(x)      pc_sin(x)
#define cos(x)      pc_cos(x)
#define tan(x)      pc_tan(x)
#define asin(x)     pc_asin(x)
#define acos(x)     pc_acos(x)
#define atan(x)     pc_atan(x)
#define atan2(y, x) pc_atan2(y, x)
#define sqrt(x)     pc_sqrt(x)
#define pow(x, y)   pc_pow(x, y)
#define floor(x)    pc_floor(x)
#define ceil(x)     pc_ceil(x)
#define lrint(x)    pc_lrint(x)
#define sinf(x)      pc_sinf(x)
#define cosf(x)      pc_cosf(x)
#define tanf(x)      pc_tanf(x)
#define asinf(x)     pc_asinf(x)
#define acosf(x)     pc_acosf(x)
#define atanf(x)     pc_atanf(x)
#define atan2f(y, x) pc_atan2f(y, x)
#define sqrtf(x)     pc_sqrtf(x)
#define powf(x, y)   pc_powf(x, y)
#define floorf(x)    pc_floorf(x)
#define ceilf(x)     pc_ceilf(x)
#define lrintf(x)    pc_lrintf(x)

/* Not implemented (unused by game code today): fail at link time. */
double pc_libm_missing_exp(double), pc_libm_missing_log(double);
double pc_libm_missing_log10(double), pc_libm_missing_log2(double);
double pc_libm_missing_exp2(double), pc_libm_missing_hyperbolic(double);
double pc_libm_missing_cbrt(double), pc_libm_missing_hypot(double, double);
void pc_libm_missing_sincos();
#undef exp
#undef expf
#undef exp2
#undef exp2f
#undef log
#undef logf
#undef log10
#undef log10f
#undef log2
#undef log2f
#define exp(x)      pc_libm_missing_exp(x)
#define expf(x)     pc_libm_missing_exp(x)
#define exp2(x)     pc_libm_missing_exp2(x)
#define exp2f(x)    pc_libm_missing_exp2(x)
#define expm1(x)    pc_libm_missing_exp(x)
#define expm1f(x)   pc_libm_missing_exp(x)
#define log(x)      pc_libm_missing_log(x)
#define logf(x)     pc_libm_missing_log(x)
#define log1p(x)    pc_libm_missing_log(x)
#define log1pf(x)   pc_libm_missing_log(x)
#define log10(x)    pc_libm_missing_log10(x)
#define log10f(x)   pc_libm_missing_log10(x)
#define log2(x)     pc_libm_missing_log2(x)
#define log2f(x)    pc_libm_missing_log2(x)
#define sinh(x)     pc_libm_missing_hyperbolic(x)
#define sinhf(x)    pc_libm_missing_hyperbolic(x)
#define cosh(x)     pc_libm_missing_hyperbolic(x)
#define coshf(x)    pc_libm_missing_hyperbolic(x)
#define tanh(x)     pc_libm_missing_hyperbolic(x)
#define tanhf(x)    pc_libm_missing_hyperbolic(x)
#define cbrt(x)     pc_libm_missing_cbrt(x)
#define cbrtf(x)    pc_libm_missing_cbrt(x)
#define hypot(x, y)  pc_libm_missing_hypot(x, y)
#define hypotf(x, y) pc_libm_missing_hypot(x, y)
#define sincos(x, s, c)  pc_libm_missing_sincos(x, s, c)
#define sincosf(x, s, c) pc_libm_missing_sincos(x, s, c)

/* rand/srand/qsort/atof: object-like, so the decomp's own declarations
 * ("int rand(void);") are renamed along with the calls. */
#undef rand
#undef srand
#undef qsort
#undef atof
#define rand  pc_rand
#define srand pc_srand
#define qsort pc_qsort
#define atof  pc_atof

#endif /* !PC_DETMATH_NO_MAP */

#endif /* PC_DETMATH_H */

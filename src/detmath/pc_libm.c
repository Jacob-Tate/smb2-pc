/* pc_libm.c - deterministic math for game code (docs/detmath.md).
 *
 * The C runtimes the port links (msvcrt for the 32-bit GCC build, UCRT for
 * the x64 clang build, glibc/Apple libm/... for future ports) disagree in the
 * last bit of sinf, atan2f etc., which makes physics drift apart between
 * builds.  Game code therefore never calls the CRT's libm: the force-included
 * pc_detmath.h maps every libm call to the functions here.
 *
 * Everything is computed with IEEE +,-,*,/, correctly rounded sqrt and
 * integer ops only (fdlibm, see fdlibm/), so every build that keeps
 * FLT_EVAL_METHOD == 0 and does not contract a*b+c into FMA gets the same
 * bits.  The float functions are (float)fdlibm((double)x), which is exactly
 * how the GameCube's MSL implements sinf/cosf/tanf/acosf/powf/floorf. */
#include <stdint.h>
#include <string.h>

#define PC_DETMATH_NO_MAP
#include "pc_detmath.h"

#if defined(__FAST_MATH__)
#error "pc_libm must not be built with -ffast-math"
#endif
#if defined(__FLT_EVAL_METHOD__) && __FLT_EVAL_METHOD__ != 0
#error "pc_libm needs FLT_EVAL_METHOD 0 (SSE2 math on x86: -msse2 -mfpmath=sse)"
#endif

/* ---- sqrt: IEEE 754 requires it correctly rounded and every target has
 * an instruction for it (SSE sqrtsd/sqrtss, ARM fsqrt).  NaN results are
 * canonicalized so even their sign/payload does not depend on the target. */
static const union { uint64_t u; double d; } k_nan = { 0x7ff8000000000000ull };
static const union { uint32_t u; float f; } k_nanf = { 0x7fc00000u };

double pc_sqrt(double x)
{
    if (!(x >= 0.0))
        return x != x ? x : k_nan.d;   /* NaN in -> same NaN; x < 0 -> NaN */
    return __builtin_sqrt(x);
}

float pc_sqrtf(float x)
{
    if (!(x >= 0.0f))
        return x != x ? x : k_nanf.f;
    return __builtin_sqrtf(x);
}

/* ---- ceil: fdlibm s_ceil.c is floor with the rounding direction flipped;
 * -floor(-x) is the same function (exact, and keeps -0.0 as -0.0). */
double pc_ceil(double x)
{
    return -pc_floor(-x);
}

/* ---- float wrappers (MSL style: compute in double, round once) */
float pc_sinf(float x)            { return (float)pc_sin(x); }
float pc_cosf(float x)            { return (float)pc_cos(x); }
float pc_tanf(float x)            { return (float)pc_tan(x); }
float pc_asinf(float x)           { return (float)pc_asin(x); }
float pc_acosf(float x)           { return (float)pc_acos(x); }
float pc_atanf(float x)           { return (float)pc_atan(x); }
float pc_atan2f(float y, float x) { return (float)pc_atan2(y, x); }
float pc_powf(float x, float y)   { return (float)pc_pow(x, y); }
float pc_floorf(float x)          { return (float)pc_floor(x); }
float pc_ceilf(float x)           { return (float)pc_ceil(x); }

/* ---- lrint: round to nearest, ties to even (the default rounding mode the
 * port never changes), like PPC fctiw.  Out-of-range and NaN inputs give
 * fctiw's results (saturate to INT32_MIN/MAX, NaN -> INT32_MIN) instead of
 * the target-dependent ones of cvtss2si / fcvtns, so the value is the same on
 * every build.  The result always fits 32 bits, whatever sizeof(long). */
static int32_t lrint_d(double x)
{
    double r;

    if (x != x)
        return INT32_MIN;
    if (x >= 2147483647.0)
        return INT32_MAX;
    if (x <= -2147483648.0)
        return INT32_MIN;
    /* |x| < 2^31 < 2^52: adding and subtracting 2^52 rounds to an integer
     * with the current (nearest-even) mode; the sign trick keeps -0.5 -> -0. */
    if (x >= 0.0)
        r = (x + 4503599627370496.0) - 4503599627370496.0;
    else
        r = (x - 4503599627370496.0) + 4503599627370496.0;
    if (r >= 2147483647.0)
        return INT32_MAX;
    return (int32_t)r;
}

long pc_lrint(double x)  { return lrint_d(x); }
long pc_lrintf(float x)  { return lrint_d(x); }  /* float -> double is exact */

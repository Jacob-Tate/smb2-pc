/* pc_port deterministic libm (docs/detmath.md).
 *
 * The fdlibm sources in this directory are copied unchanged from SDL 2.30.9
 * src/libm (uClibc's copy of Sun fdlibm 5.3; Sun's permissive notice is at
 * the top of every file).  SDL's math_libm.h / math_private.h / SDL_assert.h
 * are replaced by these three small headers so the files build stand-alone:
 * no SDL, no system <math.h>, and endian-independent word access.
 *
 * Public names (pc_sin etc.) are declared in include/pc_detmath.h. */
#ifndef PC_FDLIBM_MATH_LIBM_H
#define PC_FDLIBM_MATH_LIBM_H

#include <stdint.h>
#include <string.h>

/* fdlibm entry points -> pc_ names (and private helpers -> pc_fdlibm_). */
#define atan             pc_atan
#define __ieee754_atan2  pc_atan2
#define cos              pc_cos
#define sin              pc_sin
#define tan              pc_tan
#define floor            pc_floor
#define __ieee754_pow    pc_pow
#define __ieee754_sqrt   pc_sqrt
#define __ieee754_acos   pc_acos
#define __ieee754_asin   pc_asin
#define copysign         pc_fdlibm_copysign
#define scalbln          pc_fdlibm_scalbln
#define scalbn           pc_fdlibm_scalbn
#define __ieee754_rem_pio2 pc_fdlibm_rem_pio2
#define __kernel_rem_pio2  pc_fdlibm_kernel_rem_pio2
#define __kernel_sin     pc_fdlibm_kernel_sin
#define __kernel_cos     pc_fdlibm_kernel_cos
#define __kernel_tan     pc_fdlibm_kernel_tan
/* fabs is exact on every target (clears the sign bit). */
#define fabs(x)          __builtin_fabs(x)

double pc_atan(double x);
double pc_atan2(double y, double x);
double pc_acos(double x);
double pc_asin(double x);
double pc_cos(double x);
double pc_sin(double x);
double pc_tan(double x);
double pc_floor(double x);
double pc_pow(double x, double y);
double pc_sqrt(double x);
double pc_fdlibm_copysign(double x, double y);
double pc_fdlibm_scalbln(double x, long n);
double pc_fdlibm_scalbn(double x, int n);

#endif

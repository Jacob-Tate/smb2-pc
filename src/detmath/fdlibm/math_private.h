/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/* Replacement for SDL's math_private.h (from fdlibm.h 5.1): the same word
 * access macros, written with 64-bit integer shifts so they do not depend
 * on the host's float word order. */
#ifndef PC_FDLIBM_MATH_PRIVATE_H
#define PC_FDLIBM_MATH_PRIVATE_H

#include <stdint.h>
#include "math_libm.h"

#define _IEEE_LIBM
#define attribute_hidden
#define libm_hidden_proto(x)
#define libm_hidden_def(x)
#define strong_alias(x, y)
#define u_int32_t uint32_t

static inline uint64_t pc_fdlibm_bits(double d)
{
    uint64_t u;
    memcpy(&u, &d, sizeof u);
    return u;
}
static inline double pc_fdlibm_from_bits(uint64_t u)
{
    double d;
    memcpy(&d, &u, sizeof d);
    return d;
}

#define EXTRACT_WORDS(ix0, ix1, d) do { uint64_t ew_u = pc_fdlibm_bits(d); \
    (ix0) = (uint32_t)(ew_u >> 32); (ix1) = (uint32_t)ew_u; } while (0)
#define GET_HIGH_WORD(i, d) do { (i) = (uint32_t)(pc_fdlibm_bits(d) >> 32); } while (0)
#define GET_LOW_WORD(i, d) do { (i) = (uint32_t)pc_fdlibm_bits(d); } while (0)
#define INSERT_WORDS(d, ix0, ix1) do { \
    (d) = pc_fdlibm_from_bits(((uint64_t)(uint32_t)(ix0) << 32) | (uint32_t)(ix1)); } while (0)
#define SET_HIGH_WORD(d, v) do { uint64_t sh_u = pc_fdlibm_bits(d); \
    (d) = pc_fdlibm_from_bits((sh_u & 0xffffffffull) | ((uint64_t)(uint32_t)(v) << 32)); } while (0)
#define SET_LOW_WORD(d, v) do { uint64_t sl_u = pc_fdlibm_bits(d); \
    (d) = pc_fdlibm_from_bits((sl_u & 0xffffffff00000000ull) | (uint32_t)(v)); } while (0)
#define GET_FLOAT_WORD(i, d) do { float gf_f = (d); uint32_t gf_u; \
    memcpy(&gf_u, &gf_f, 4); (i) = gf_u; } while (0)
#define SET_FLOAT_WORD(d, i) do { uint32_t sf_u = (i); float sf_f; \
    memcpy(&sf_f, &sf_u, 4); (d) = sf_f; } while (0)

/* k_rem_pio2.c uses these SDL helpers */
#define SDL_arraysize(a) (sizeof(a) / sizeof((a)[0]))
#define SDL_memset memset

extern int32_t __ieee754_rem_pio2(double, double *) attribute_hidden;
extern double __kernel_sin(double, double, int) attribute_hidden;
extern double __kernel_cos(double, double) attribute_hidden;
extern double __kernel_tan(double, double, int) attribute_hidden;
extern int32_t __kernel_rem_pio2(const double *, double *, int, int, const unsigned int,
                                 const int32_t *) attribute_hidden;

#endif

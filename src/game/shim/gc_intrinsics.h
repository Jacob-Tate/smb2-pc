/* MWCC PowerPC compiler intrinsics, implemented for the PC build.
 * Force-included (-include) into every decomp TU so the calls get real
 * prototypes; the decomp uses them bare, which C would otherwise treat
 * as implicit int functions and miscompile the float ones. */
#ifndef GC_INTRINSICS_H
#define GC_INTRINSICS_H

/* Self-contained on purpose: pulling in <string.h>/<math.h> here clashes
 * with the decomp TUs' own MSL-style libc prototypes. */

static inline int __abs(int x) { return x < 0 ? -x : x; }
static inline double __fabs(double x) { return x < 0.0 ? -x : x; }
static inline float __fabsf(float x) { return x < 0.0f ? -x : x; }
static inline float __fmadds(float a, float b, float c) { return a * b + c; }

/* frsqrte is a hardware estimate the game refines itself; a bit-hack
 * estimate plus Newton steps is faithful and needs no libm. */
static inline double __frsqrte(double x)
{
    union { double d; unsigned long long i; } u;
    double y;
    u.d = x;
    u.i = 0x5FE6EB50C7B537A9ull - (u.i >> 1);
    y = u.d;
    y = y * (1.5 - 0.5 * x * y * y);
    y = y * (1.5 - 0.5 * x * y * y);
    y = y * (1.5 - 0.5 * x * y * y);
    y = y * (1.5 - 0.5 * x * y * y);
    return y;
}

/* lwbrx = "load word byte-reversed": on the big-endian GC it reads a
 * little-endian u32; a native little-endian byte assembly does the same. */
static inline unsigned int __lwbrx(const void *base, int offset)
{
    const unsigned char *b = (const unsigned char *)base + offset;
    return (unsigned int)b[0] | ((unsigned int)b[1] << 8) |
           ((unsigned int)b[2] << 16) | ((unsigned int)b[3] << 24);
}

/* rlwimi: rotate src left by sh, then insert bits mb..me (IBM numbering,
 * 0 = MSB) into dst. */
static inline unsigned int __rlwimi(unsigned int dst, unsigned int src,
                                    int sh, int mb, int me)
{
    unsigned int r = (src << sh) | (src >> ((32 - sh) & 31));
    unsigned int mhi = 0xFFFFFFFFu >> mb;
    unsigned int mlo = 0xFFFFFFFFu << (31 - me);
    unsigned int mask = (mb <= me) ? (mhi & mlo) : (mhi | mlo);
    return (dst & ~mask) | (r & mask);
}

#endif /* GC_INTRINSICS_H */

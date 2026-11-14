/* detmath_test.c - cross-build determinism check for src/detmath
 * (docs/detmath.md).  Build it with every toolchain (build.sh), run each
 * binary, and diff the outputs: every "pc" line must be identical across
 * builds.  The "crt" lines show the same inputs through the C runtime's
 * libm for contrast (they are expected to differ between msvcrt, UCRT and
 * glibc), and "ulp" lines give pc's max error against the CRT.
 *
 *   detmath_test [stride]     (default stride 61: ~70M float inputs per
 *                              unary function; stride 1 = all 2^32)
 *   detmath_test --dump FUNC FILE   write every pc result (u32/u64 each) to
 *                              FILE, to locate a hash mismatch
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#define PC_DETMATH_NO_MAP
#include "pc_detmath.h"

static uint32_t f2u(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }
static float u2f(uint32_t u) { float f; memcpy(&f, &u, 4); return f; }
static uint64_t d2u(double d) { uint64_t u; memcpy(&u, &d, 8); return u; }
static double u2d(uint64_t u) { double d; memcpy(&d, &u, 8); return d; }

typedef struct { uint64_t h; uint64_t n; } Hash;
static void hinit(Hash *h) { h->h = 1469598103934665603ull; h->n = 0; }
static void hadd(Hash *h, uint64_t v)
{
    int i;
    for (i = 0; i < 8; i++) { h->h ^= (v >> (i * 8)) & 0xff; h->h *= 1099511628211ull; }
    h->n++;
}
/* NaNs: hash a canonical value, the payload/sign of a NaN is not part of
 * the contract for the CRT columns (pc canonicalizes sqrt only, fdlibm
 * propagates x-x / (x-x)/(x-x), which is the target's default NaN). */
static uint32_t fkey(float f) { return f != f ? 0x7fc00000u : f2u(f); }
static uint64_t dkey(double d) { return d != d ? 0x7ff8000000000000ull : d2u(d); }

/* distance in ulps between two floats (same sign assumed close) */
static int64_t ulpf(float a, float b)
{
    int64_t ia = (int32_t)f2u(a), ib = (int32_t)f2u(b);
    if (a != a && b != b) return 0;
    if (a != a || b != b) return 1 << 30;
    if (ia < 0) ia = (int64_t)INT32_MIN - ia;
    if (ib < 0) ib = (int64_t)INT32_MIN - ib;
    return ia > ib ? ia - ib : ib - ia;
}

static uint64_t s_rng = 0x9E3779B97F4A7C15ull;
static uint64_t rng(void)
{
    s_rng ^= s_rng << 13; s_rng ^= s_rng >> 7; s_rng ^= s_rng << 17;
    return s_rng;
}

static const float k_edge_f[] = {
    0.0f, -0.0f, 1.0f, -1.0f, 0.5f, -0.5f, 2.0f, 1e-45f, -1e-45f, 1.17549435e-38f,
    FLT_MAX, -FLT_MAX, FLT_EPSILON, 3.14159265f, -3.14159265f, 1.57079633f,
    6.28318531f, 0.785398163f, 1e10f, -1e10f, 1e20f, 1e38f, 0.999999940f,
    1.00000012f, 65536.0f, 32768.0f, 0.0000958738f,
};

/* ---- unary float functions ---- */
typedef float (*FnF)(float);
static float crt_sinf(float x) { return sinf(x); }
static float crt_cosf(float x) { return cosf(x); }
static float crt_tanf(float x) { return tanf(x); }
static float crt_asinf(float x) { return asinf(x); }
static float crt_acosf(float x) { return acosf(x); }
static float crt_atanf(float x) { return atanf(x); }
static float crt_sqrtf(float x) { return sqrtf(x); }
static float crt_floorf(float x) { return floorf(x); }
static float crt_ceilf(float x) { return ceilf(x); }
static float pcw_lrintf(float x) { return (float)pc_lrintf(x); }
static float crt_lrintf(float x)
{
    /* the CRT's lrintf is undefined out of range; compare in range only */
    if (!(x > -2147483648.0f && x < 2147483520.0f)) return (float)pc_lrintf(x);
    return (float)lrintf(x);
}

static const struct { const char *name; FnF pc, crt; } k_fnf[] = {
    { "sinf", pc_sinf, crt_sinf },
    { "cosf", pc_cosf, crt_cosf },
    { "tanf", pc_tanf, crt_tanf },
    { "asinf", pc_asinf, crt_asinf },
    { "acosf", pc_acosf, crt_acosf },
    { "atanf", pc_atanf, crt_atanf },
    { "sqrtf", pc_sqrtf, crt_sqrtf },
    { "floorf", pc_floorf, crt_floorf },
    { "ceilf", pc_ceilf, crt_ceilf },
    { "lrintf", pcw_lrintf, crt_lrintf },
};

static void test_unary_f(int k, uint32_t stride, FILE *dump)
{
    Hash hp, hc;
    uint64_t i, diff = 0;
    int64_t maxu = 0;
    float worst = 0;
    unsigned e;

    hinit(&hp); hinit(&hc);
    for (e = 0; e < sizeof k_edge_f / sizeof k_edge_f[0]; e++)
    {
        float x = k_edge_f[e];
        hadd(&hp, fkey(k_fnf[k].pc(x)));
        hadd(&hc, fkey(k_fnf[k].crt(x)));
    }
    hadd(&hp, fkey(k_fnf[k].pc(u2f(0x7f800000u))));   /* +inf */
    hadd(&hp, fkey(k_fnf[k].pc(u2f(0xff800000u))));   /* -inf */
    hadd(&hp, fkey(k_fnf[k].pc(u2f(0x7fc00000u))));   /* nan */
    for (i = 0; i <= 0xFFFFFFFFull; i += stride)
    {
        float x = u2f((uint32_t)i), p, c;
        int64_t u;

        p = k_fnf[k].pc(x);
        c = k_fnf[k].crt(x);
        hadd(&hp, fkey(p));
        hadd(&hc, fkey(c));
        if (dump) { uint32_t w = f2u(p); fwrite(&w, 4, 1, dump); }
        if (fkey(p) != fkey(c))
        {
            diff++;
            u = ulpf(p, c);
            /* only measure accuracy where the function is well conditioned
             * enough for the CRT to be a reference (|x| < 2^20) */
            if (u > maxu && __builtin_fabsf(x) < 1048576.0f) { maxu = u; worst = x; }
        }
    }
    /* game-like inputs: s16 angle -> radians, all 65536 */
    for (i = 0; i < 65536; i++)
    {
        float x = (float)(int16_t)i * (3.14159265358979323846f / 32768.0f);
        hadd(&hp, fkey(k_fnf[k].pc(x)));
        hadd(&hc, fkey(k_fnf[k].crt(x)));
    }
    printf("pc  %-7s n=%llu hash=%016llx\n", k_fnf[k].name,
           (unsigned long long)hp.n, (unsigned long long)hp.h);
    printf("crt %-7s n=%llu hash=%016llx  pc!=crt: %llu, max %lld ulp (x=%a)\n",
           k_fnf[k].name, (unsigned long long)hc.n, (unsigned long long)hc.h,
           (unsigned long long)diff, (long long)maxu, (double)worst);
}

/* ---- binary float: atan2f, powf ---- */
static float rnd_float(void)
{
    uint64_t r = rng();
    switch (r & 3)
    {
    case 0: return u2f((uint32_t)(r >> 32));                    /* any bits */
    case 1: return (float)((int32_t)(r >> 32)) / 2147483648.0f * 4.0f; /* [-4,4) */
    case 2: return (float)((int32_t)(r >> 32)) / 2147483648.0f * 2000.0f;
    default: return (float)((r >> 40) & 0xFFFF) / 256.0f;      /* [0,256) */
    }
}

static void test_binary_f(const char *name, float (*pc)(float, float),
                          float (*crt)(float, float), uint64_t n)
{
    Hash hp, hc;
    uint64_t i, diff = 0;
    unsigned a, b;
    const unsigned ne = sizeof k_edge_f / sizeof k_edge_f[0];

    hinit(&hp); hinit(&hc);
    s_rng = 0x9E3779B97F4A7C15ull;
    for (a = 0; a < ne; a++)
        for (b = 0; b < ne; b++)
        {
            hadd(&hp, fkey(pc(k_edge_f[a], k_edge_f[b])));
            hadd(&hc, fkey(crt(k_edge_f[a], k_edge_f[b])));
        }
    for (i = 0; i < n; i++)
    {
        float x = rnd_float(), y = rnd_float(), p = pc(x, y), c = crt(x, y);
        hadd(&hp, fkey(p));
        hadd(&hc, fkey(c));
        if (fkey(p) != fkey(c)) diff++;
    }
    printf("pc  %-7s n=%llu hash=%016llx\n", name, (unsigned long long)hp.n,
           (unsigned long long)hp.h);
    printf("crt %-7s n=%llu hash=%016llx  pc!=crt: %llu\n", name,
           (unsigned long long)hc.n, (unsigned long long)hc.h, (unsigned long long)diff);
}
static float crt_atan2f(float y, float x) { return atan2f(y, x); }
static float crt_powf(float x, float y) { return powf(x, y); }

/* ---- double functions on random doubles ---- */
typedef double (*FnD)(double);
static double crt_sin(double x) { return sin(x); }
static double crt_cos(double x) { return cos(x); }
static double crt_tan(double x) { return tan(x); }
static double crt_asin(double x) { return asin(x); }
static double crt_acos(double x) { return acos(x); }
static double crt_atan(double x) { return atan(x); }
static double crt_sqrt(double x) { return sqrt(x); }
static double crt_floor(double x) { return floor(x); }
static double crt_ceil(double x) { return ceil(x); }
static const struct { const char *name; FnD pc, crt; } k_fnd[] = {
    { "sin", pc_sin, crt_sin }, { "cos", pc_cos, crt_cos }, { "tan", pc_tan, crt_tan },
    { "asin", pc_asin, crt_asin }, { "acos", pc_acos, crt_acos }, { "atan", pc_atan, crt_atan },
    { "sqrt", pc_sqrt, crt_sqrt }, { "floor", pc_floor, crt_floor }, { "ceil", pc_ceil, crt_ceil },
};

static double rnd_double(void)
{
    uint64_t r = rng();
    switch (r & 3)
    {
    case 0: return u2d(r);
    case 1: return (double)(int64_t)r / 9223372036854775808.0 * 4.0;
    case 2: return (double)(int64_t)r / 9223372036854775808.0 * 1e6;
    default: return u2d((r & 0x800FFFFFFFFFFFFFull) | ((uint64_t)(1023 - 30 + (r >> 52) % 60) << 52));
    }
}

static void test_unary_d(int k, uint64_t n)
{
    Hash hp, hc;
    uint64_t i, diff = 0;

    hinit(&hp); hinit(&hc);
    s_rng = 0x2545F4914F6CDD1Dull;
    for (i = 0; i < n; i++)
    {
        double x = rnd_double(), p = k_fnd[k].pc(x), c = k_fnd[k].crt(x);
        hadd(&hp, dkey(p));
        hadd(&hc, dkey(c));
        if (dkey(p) != dkey(c)) diff++;
    }
    printf("pc  %-7s n=%llu hash=%016llx\n", k_fnd[k].name, (unsigned long long)hp.n,
           (unsigned long long)hp.h);
    printf("crt %-7s n=%llu hash=%016llx  pc!=crt: %llu\n", k_fnd[k].name,
           (unsigned long long)hc.n, (unsigned long long)hc.h, (unsigned long long)diff);
}

static void test_binary_d(const char *name, double (*pc)(double, double),
                          double (*crt)(double, double), uint64_t n)
{
    Hash hp, hc;
    uint64_t i, diff = 0;

    hinit(&hp); hinit(&hc);
    s_rng = 0x5851F42D4C957F2Dull;
    for (i = 0; i < n; i++)
    {
        double x = rnd_double(), y = rnd_double(), p, c;
        if (i & 1) y = (double)(int)((rng() % 41)) - 20.0;   /* integer exponents */
        p = pc(x, y); c = crt(x, y);
        hadd(&hp, dkey(p));
        hadd(&hc, dkey(c));
        if (dkey(p) != dkey(c)) diff++;
    }
    printf("pc  %-7s n=%llu hash=%016llx\n", name, (unsigned long long)hp.n, (unsigned long long)hp.h);
    printf("crt %-7s n=%llu hash=%016llx  pc!=crt: %llu\n", name, (unsigned long long)hc.n,
           (unsigned long long)hc.h, (unsigned long long)diff);
}
static double crt_atan2(double y, double x) { return atan2(y, x); }
static double crt_pow(double x, double y) { return pow(x, y); }

/* ---- rand / qsort ---- */
static int cmp_key(const void *a, const void *b)
{
    /* sort by the low byte only: lots of ties, so the order of equal keys
     * depends on the algorithm */
    int ka = *(const uint32_t *)a & 0xff, kb = *(const uint32_t *)b & 0xff;
    return ka - kb;
}

static void test_rand_qsort(void)
{
    Hash h;
    int i, n, t;
    static uint32_t arr[4096];

    hinit(&h);
    pc_srand(1);
    for (i = 0; i < 1000000; i++) hadd(&h, (uint64_t)pc_rand());
    pc_srand(0x12345678u);
    for (i = 0; i < 1000000; i++) hadd(&h, (uint64_t)pc_rand());
    printf("pc  rand    n=%llu hash=%016llx\n", (unsigned long long)h.n, (unsigned long long)h.h);
    /* GC MSL reference: srand(1) -> first values */
    pc_srand(1);
    {
        int r0 = pc_rand(), r1 = pc_rand(), r2 = pc_rand(), r3 = pc_rand();
        printf("pc  rand    first: %d %d %d %d (GC MSL LCG: 16838 5758 10113 17515)\n",
               r0, r1, r2, r3);
    }

    hinit(&h);
    s_rng = 0x1234567ull;
    for (t = 0; t < 2000; t++)
    {
        n = 1 + (int)(rng() % 64);
        if (t % 100 == 0) n = 4096;
        for (i = 0; i < n; i++) arr[i] = (uint32_t)rng();
        pc_qsort(arr, (uint32_t)n, 4u, cmp_key);
        for (i = 0; i < n; i++) hadd(&h, arr[i]);
    }
    printf("pc  qsort   n=%llu hash=%016llx\n", (unsigned long long)h.n, (unsigned long long)h.h);
    hinit(&h);
    s_rng = 0x1234567ull;
    for (t = 0; t < 2000; t++)
    {
        n = 1 + (int)(rng() % 64);
        if (t % 100 == 0) n = 4096;
        for (i = 0; i < n; i++) arr[i] = (uint32_t)rng();
        qsort(arr, (size_t)n, 4u, cmp_key);
        for (i = 0; i < n; i++) hadd(&h, arr[i]);
    }
    printf("crt qsort   n=%llu hash=%016llx\n", (unsigned long long)h.n, (unsigned long long)h.h);
    hinit(&h);
    srand(1);
    for (i = 0; i < 1000000; i++) hadd(&h, (uint64_t)rand());
    printf("crt rand    n=%llu hash=%016llx\n", (unsigned long long)h.n, (unsigned long long)h.h);
}

/* ---- atof: generated decimal strings (no printf, which differs per CRT) */
static void test_atof(void)
{
    Hash hp, hc;
    char buf[64];
    int i, j, n;
    uint64_t diff = 0, fast = 0;

    hinit(&hp); hinit(&hc);
    s_rng = 0x0DDBA11ull;
    for (i = 0; i < 2000000; i++)
    {
        uint64_t r = rng();
        char *p = buf;
        if (r & 1) *p++ = '-';
        n = 1 + (int)((r >> 1) % 6);                 /* integer digits */
        for (j = 0; j < n; j++) *p++ = (char)('0' + rng() % 10);
        if (r & 2)
        {
            *p++ = '.';
            n = (int)((r >> 8) % (r & 4 ? 20 : 8)); /* fraction digits */
            for (j = 0; j < n; j++) *p++ = (char)('0' + rng() % 10);
        }
        if ((r & 0x30) == 0x30)
        {
            int e = (int)((r >> 16) % 61) - 30;
            p += sprintf(p, "e%d", e);
        }
        *p = 0;
        {
            double a = pc_atof(buf), b = strtod(buf, NULL);
            hadd(&hp, d2u(a));
            hadd(&hc, d2u(b));
            if (d2u(a) != d2u(b)) diff++;
            {
                /* fast-path inputs: <= 15 significant digits, no exponent */
                int nd = 0; const char *q = buf; int lead = 1;
                for (; *q; q++)
                    if (*q >= '0' && *q <= '9') { if (*q != '0') lead = 0; if (!lead) nd++; }
                if (nd <= 15 && (r & 0x30) != 0x30 && d2u(a) != d2u(b)) fast++;
            }
        }
    }
    printf("pc  atof    n=%llu hash=%016llx\n", (unsigned long long)hp.n, (unsigned long long)hp.h);
    printf("crt atof    n=%llu hash=%016llx  pc!=crt: %llu (<= 15 digits, no exponent: %llu)\n",
           (unsigned long long)hc.n, (unsigned long long)hc.h, (unsigned long long)diff,
           (unsigned long long)fast);
}

int main(int argc, char **argv)
{
    uint32_t stride = 61;
    unsigned k;

    if (argc == 4 && strcmp(argv[1], "--dump") == 0)
    {
        FILE *f = fopen(argv[3], "wb");
        for (k = 0; k < sizeof k_fnf / sizeof k_fnf[0]; k++)
            if (strcmp(k_fnf[k].name, argv[2]) == 0)
                test_unary_f((int)k, 1, f);
        fclose(f);
        return 0;
    }
    if (argc > 1) stride = (uint32_t)strtoul(argv[1], NULL, 0);
    printf("detmath_test: sizeof(void*)=%d FLT_EVAL_METHOD=%d stride=%u\n",
           (int)sizeof(void *), (int)FLT_EVAL_METHOD, stride);
    for (k = 0; k < sizeof k_fnf / sizeof k_fnf[0]; k++)
        test_unary_f((int)k, stride, NULL);
    test_binary_f("atan2f", pc_atan2f, crt_atan2f, 20000000);
    test_binary_f("powf", pc_powf, crt_powf, 20000000);
    for (k = 0; k < sizeof k_fnd / sizeof k_fnd[0]; k++)
        test_unary_d((int)k, 10000000);
    test_binary_d("atan2", pc_atan2, crt_atan2, 10000000);
    test_binary_d("pow", pc_pow, crt_pow, 10000000);
    test_rand_qsort();
    test_atof();
    return 0;
}

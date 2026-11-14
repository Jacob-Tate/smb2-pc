/* mathutil_test.c - checks the GC-table trig of src/game/shim/mathutil.c
 * (docs/detmath.md): prints hashes that must match across builds, and the
 * error against libm for sanity.  Built by build.sh when a decomp dir is
 * given (mathutil.c needs decomp/include). */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "mathutil.h"

typedef struct { uint64_t h, n; } Hash;
static void hinit(Hash *h) { h->h = 1469598103934665603ull; h->n = 0; }
static void hadd(Hash *h, uint64_t v)
{
    int i;
    for (i = 0; i < 8; i++) { h->h ^= (v >> (i * 8)) & 0xff; h->h *= 1099511628211ull; }
    h->n++;
}
static uint32_t f2u(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }
static float u2f(uint32_t u) { float f; memcpy(&f, &u, 4); return f; }

static uint64_t s_rng = 0x9E3779B97F4A7C15ull;
static uint64_t rng(void)
{
    s_rng ^= s_rng << 13; s_rng ^= s_rng >> 7; s_rng ^= s_rng << 17;
    return s_rng;
}
static float rnd_float(void)
{
    uint64_t r = rng();
    switch (r & 3)
    {
    case 0: return u2f((uint32_t)(r >> 32));
    case 1: return (float)((int32_t)(r >> 32)) / 2147483648.0f * 4.0f;
    case 2: return (float)((int32_t)(r >> 32)) / 2147483648.0f * 2000.0f;
    default: return (float)((int32_t)(r >> 32) >> 20);   /* small integers: ties */
    }
}

int main(void)
{
    Hash h;
    int a, i;
    double maxs = 0, maxt = 0;
    long bad = 0, n = 0, d1 = 0, d2 = 0;

    hinit(&h);
    for (a = 0; a < 65536; a++)
    {
        float sc[2], s = math_sin((s16)a), t = math_tan((s16)a);
        double ref = sin(a * (3.14159265358979323846 / 32768.0));
        double reft = tan(a * (3.14159265358979323846 / 32768.0));

        math_sin_cos_v((s16)a, sc);
        hadd(&h, f2u(s)); hadd(&h, f2u(t)); hadd(&h, f2u(sc[0])); hadd(&h, f2u(sc[1]));
        if (fabs(s - ref) > maxs) maxs = fabs(s - ref);
        if (fabs(reft) < 1000 && fabs(t - reft) / (fabs(reft) > 1 ? fabs(reft) : 1) > maxt)
            maxt = fabs(t - reft) / (fabs(reft) > 1 ? fabs(reft) : 1);
        if (sc[0] != s || sc[1] != math_sin((s16)(a + 0x4000))) bad++;
    }
    printf("sin/tan/sincos  n=%llu hash=%016llx  max|sin err|=%.3g max tan relerr=%.3g sincos!=sin: %ld\n",
           (unsigned long long)h.n, (unsigned long long)h.h, maxs, maxt, bad);

    hinit(&h);
    for (i = 0; i < 20000000; i++)
    {
        float y = rnd_float(), x = rnd_float();
        s16 r = math_atan2(y, x);
        hadd(&h, (uint16_t)r);
        if (i < 5000000 && x == x && y == y && fabsf(x) < 1e30f && fabsf(y) < 1e30f && (x != 0 || y != 0))
        {
            double ref = atan2(y, x) * (32768.0 / 3.14159265358979323846);
            long rr = lrint(ref);
            int dd = (int)(s16)(r - (s16)rr);
            n++;
            if (dd) d1++;
            if (dd > 1 || dd < -1) d2++;
        }
    }
    for (a = 0; a < 65536; a++)
    {
        /* round trip through the tables, as camera/AI code does */
        float sc[2];
        math_sin_cos_v((s16)a, sc);
        hadd(&h, (uint16_t)math_atan2(sc[0], sc[1]));
        hadd(&h, (uint16_t)math_atan(u2f((uint32_t)a << 16)));
    }
    printf("atan2/atan      n=%llu hash=%016llx  vs libm (finite): %ld/%ld off by 1 unit, %ld by more\n",
           (unsigned long long)h.n, (unsigned long long)h.h, d1 - d2, n, d2);
    return 0;
}

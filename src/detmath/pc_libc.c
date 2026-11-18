/* pc_libc.c - the GameCube MSL's rand/srand/qsort for game code
 * (docs/detmath.md).
 *
 * The CRT versions are not the same on every target: msvcrt/UCRT rand is
 * a different LCG from the GC's (and from glibc's), and qsort orders equal
 * elements differently per implementation (boat/race rankings sort with
 * ties).  pc_detmath.h maps the game's calls here, so every build -- and the
 * GameCube -- produces the same sequence and the same order. */
#include <stdint.h>
#include <stddef.h>

#define PC_DETMATH_NO_MAP
#include "pc_detmath.h"

/* MSL rand (main.dol 0x80071F90): next = next * 1103515245 + 12345,
 * returns bits 16..30.  rand_next_value starts at 1. */
static uint32_t s_rand_next = 1;

int pc_rand(void)
{
    s_rand_next = s_rand_next * 1103515245u + 12345u;
    return (int)((s_rand_next >> 16) & 0x7FFF);
}

void pc_srand(unsigned int seed)
{
    s_rand_next = seed;
}

/* MSL qsort (main.dol 0x80071E18): a heapsort.  Transcribed from the asm;
 * comparisons and swaps happen in exactly the GC's order, so ties end up
 * where they do on the GameCube.  Counts are 32-bit like on the GC (the
 * decomp declares qsort with int/u32 counts, see pc_detmath.h). */
static void swap_bytes(char *a, char *b, uint32_t size)
{
    while (size--)
    {
        char t = *a;
        *a++ = *b;
        *b++ = t;
    }
}

void pc_qsort(void *base, uint32_t count, uint32_t size,
              int (*compare)(const void *, const void *))
{
    char *table = (char *)base;
    uint32_t l, r, j;
    char *lp, *rp, *ip, *jp, *kp;

    if (count < 2)
        return;
    r = count;
    l = (r >> 1) + 1;
    lp = table + (size_t)(l - 1) * size;
    rp = table + (size_t)(r - 1) * size;
    for (;;)
    {
        if (l > 1)
        {
            l--;
            lp -= size;
        }
        else
        {
            swap_bytes(lp, rp, size);
            if (--r == 1)
                return;
            rp -= size;
        }
        j = l;
        jp = table + (size_t)(j - 1) * size;
        while (j * 2 <= r)
        {
            j *= 2;
            ip = jp;
            jp = table + (size_t)(j - 1) * size;
            if (j < r)
            {
                kp = jp + size;
                if (compare(jp, kp) < 0)
                {
                    j++;
                    jp = kp;
                }
            }
            if (compare(ip, jp) < 0)
                swap_bytes(ip, jp, size);
            else
                break;
        }
    }
}

/* atof for game text (dialog.c parses "\t<seconds>" timings from the
 * script files): decimal only.  Up to 19 significant digits are gathered
 * into an integer; when it is < 2^53 and the decimal exponent is within
 * +-22 (every number in the game data), one IEEE multiply or divide by an
 * exact power of ten gives the correctly rounded result, the same as any
 * correct strtod.  Outside that range the result is still deterministic
 * (plain double arithmetic), just not guaranteed correctly rounded. */
double pc_atof(const char *s)
{
    static const double k_pow10[23] = {
        1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11,
        1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22,
    };
    uint64_t m = 0;
    int neg = 0, digits = 0, exp10 = 0, eneg = 0, e = 0;
    double r;

    while (*s == ' ' || (*s >= '\t' && *s <= '\r'))
        s++;
    if (*s == '+' || *s == '-')
        neg = *s++ == '-';
    for (; *s >= '0' && *s <= '9'; s++)
    {
        if (digits < 19) { m = m * 10 + (uint64_t)(*s - '0'); if (m) digits++; }
        else exp10++;
    }
    if (*s == '.')
    {
        for (s++; *s >= '0' && *s <= '9'; s++)
        {
            if (digits < 19) { m = m * 10 + (uint64_t)(*s - '0'); if (m) digits++; exp10--; }
        }
    }
    if ((*s == 'e' || *s == 'E') &&
        ((s[1] >= '0' && s[1] <= '9') ||
         ((s[1] == '+' || s[1] == '-') && s[2] >= '0' && s[2] <= '9')))
    {
        s++;
        if (*s == '+' || *s == '-')
            eneg = *s++ == '-';
        for (; *s >= '0' && *s <= '9'; s++)
            if (e < 100000)
                e = e * 10 + (*s - '0');
        exp10 += eneg ? -e : e;
    }
    while (m != 0 && m % 10 == 0)   /* "1.5000" -> 15e-1 */
    {
        m /= 10;
        exp10++;
    }
    r = (double)m;   /* exact when m < 2^53 */
    if (m == 0)
        r = 0.0;
    else if (m < (1ull << 53) && exp10 >= -22 && exp10 <= 22)
        r = exp10 >= 0 ? r * k_pow10[exp10] : r / k_pow10[-exp10];
    else
    {
        for (; exp10 > 22; exp10 -= 22)
            r *= 1e22;
        for (; exp10 < -22; exp10 += 22)
            r /= 1e22;
        r = exp10 >= 0 ? r * k_pow10[exp10] : r / k_pow10[-exp10];
    }
    return neg ? -r : r;
}

/* current rand() state (determinism logs / checksums, netplay E3) */
uint32_t pc_rand_state(void)
{
    return s_rand_next;
}

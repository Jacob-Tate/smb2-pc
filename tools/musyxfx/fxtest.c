/* fxtest: run the port's MusyX ReverbHI / Chorus (src/game/shim/musyx_fx.c)
 * on raw s32 L/R/S blocks.  Commands on argv:
 *   fxtest rev <coloration> <mix> <time> <damping> <preDelay> <crosstalk> <in> <out> [state]
 *   fxtest cho <baseDelay> <variation> <period> <in> <out>
 * in/out: little-endian s32, per block 160 L, 160 R, 160 S. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>
#include "audio_dsp.h"
void adsp_run(adsp_fn fn, const void *arg, uint32_t bytes) { (void)fn; (void)arg; (void)bytes; }
void adsp_aux_set(adsp_aux_fn a, void *ua, adsp_aux_fn b, void *ub) { (void)a; (void)ua; (void)b; (void)ub; }
void *musyx_fx_test_reverb_new(float, float, float, float, float, float);
void *musyx_fx_test_chorus_new(uint32_t, uint32_t, uint32_t);
void musyx_fx_test_reverb_update(void *, float, float, float, float, float, float);
void musyx_fx_test_chorus_update(void *, uint32_t, uint32_t, uint32_t);
void fn_8005205C(uint8_t reason, void *info, void *user);
void fn_80053430(uint8_t reason, void *info, void *user);
int main(int argc, char **argv)
{
    void *fx; void (*cb)(uint8_t, void *, void *); FILE *fi, *fo; int32_t blk[480]; int ai, nb = 0, ub = -1, rev;
    if (argc < 2) return 2;
    if (!strcmp(argv[1], "bench")) {   /* bench <blocks>: impulse then silence, time per 200 blocks */
        LARGE_INTEGER fq, t0, t1; int n = atoi(argv[2]), b; double worst = 0, sum = 0;
        void *r = musyx_fx_test_reverb_new(0.9f, 0.65f, 3.0f, 0.6f, 0.1f, 0.0f);
        void *c = musyx_fx_test_chorus_new(15, 0, 500);
        QueryPerformanceFrequency(&fq);
        for (b = 0; b < n; b++) {
            int32_t a[480] = {0}, d[480] = {0};
            ADSP_AUX_INFO ia = { a, a + 160, a + 320 }, id = { d, d + 160, d + 320 };
            if (b == 0) a[0] = a[160] = a[320] = d[0] = 1000000;
            QueryPerformanceCounter(&t0);
            fn_8005205C(0, &ia, r); fn_80053430(0, &id, c);
            QueryPerformanceCounter(&t1);
            { double us = (t1.QuadPart - t0.QuadPart) * 1e6 / fq.QuadPart; sum += us; if (us > worst) worst = us; }
            if (b % 2000 == 1999) { printf("%5.1f s: mean %.1f us/block, worst %.1f us\n", (b + 1) / 200.0, sum / 2000, worst); sum = 0; worst = 0; }
        }
        return 0;
    }
    rev = !strcmp(argv[1], "rev");
    if (rev) {
        fx = musyx_fx_test_reverb_new(atof(argv[2]), atof(argv[3]), atof(argv[4]), atof(argv[5]), atof(argv[6]), atof(argv[7]));
        cb = fn_8005205C; ai = 8;
    } else {
        fx = musyx_fx_test_chorus_new(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
        cb = fn_80053430; ai = 5;
    }
    if (!fx) { fprintf(stderr, "create failed\n"); return 1; }
    fi = fopen(argv[ai], "rb"); fo = fopen(argv[ai + 1], "wb");
    if (argc > ai + 2)   /* upd <block> <params> */
        ub = atoi(argv[ai + 3]);
    while (fread(blk, 4, 480, fi) == 480) {
        ADSP_AUX_INFO info = { blk, blk + 160, blk + 320 };
        if (nb++ == ub) {
            char **u = argv + ai + 4;
            if (rev)
                musyx_fx_test_reverb_update(fx, atof(u[0]), atof(u[1]), atof(u[2]), atof(u[3]), atof(u[4]), atof(u[5]));
            else
                musyx_fx_test_chorus_update(fx, atoi(u[0]), atoi(u[1]), atoi(u[2]));
        }
        cb(0, &info, fx);
        fwrite(blk, 4, 480, fo);
    }
    fclose(fo);
    return 0;
}

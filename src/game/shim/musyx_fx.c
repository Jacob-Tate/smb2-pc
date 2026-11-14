/* MusyX auxiliary effects ReverbHI (aux A) and Chorus (aux B), transcribed
 * from main.dol so they produce the same integers as the original: every
 * single rounding, fused multiply-add, fctiwz / fctiw and operation order is
 * kept.  The effect state lives in port-side mirrors of the game's structs
 * and runs on the audio worker; Prepare / Update / Shutdown are queued to it
 * in command order.  See docs/audio.md, Aux effects.
 * The game registers them on lbl_1_bss_259B0 / lbl_1_bss_25B90 (sound.c).
 * Tables: comb/all-pass lengths 0x80116320, rsmpTab12khz 0x80116340. */
#include "shim.h"
#include "audio_dsp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Gekko arithmetic                                                    */

/* Fused ops as Dolphin computes them: in double, rounded once to single
 * (the product of two singles is exact in double). */
static inline f32 fmadds(f32 a, f32 c, f32 b) { return (f32)((double)a * (double)c + (double)b); }
static inline f32 fnmsubs(f32 a, f32 c, f32 b) { return (f32)(-((double)a * (double)c - (double)b)); }

/* fctiwz: truncate toward zero, saturating (NaN -> 0x80000000) */
static inline s32 fctiwz(f32 x)
{
    if (x != x)
        return (s32)0x80000000u;
    if (x >= 2147483648.0f)
        return 0x7FFFFFFF;
    if (x <= -2147483648.0f)
        return (s32)0x80000000u;
    return (s32)x;
}

/* fctiw with FPSCR[RN] = 0: round to nearest even, saturating */
static inline s32 fctiw(f32 x)
{
    double d = x, r;
    if (x != x)
        return (s32)0x80000000u;
    if (d >= 2147483647.5)
        return 0x7FFFFFFF;
    if (d < -2147483648.5)
        return (s32)0x80000000u;
    r = (d + 6755399441055744.0) - 6755399441055744.0;   /* 1.5 * 2^52: ties to even */
    return (s32)r;
}

/* salMalloc (0x80052008) goes through the game's sndSetHooks allocator; the
 * port zero-fills, which matters only for the chorus history blocks that
 * PrepareChorus leaves uninitialised (see there). */
static void *salMalloc(u32 n) { return calloc(1, n ? n : 1); }
static void salFree(void *p) { free(p); }

/* ------------------------------------------------------------------ */
/* ReverbHI                                                            */

typedef struct
{
    s32 inPoint;     /* byte offsets into inputs */
    s32 outPoint;
    s32 length;      /* bytes */
    f32 *inputs;
    f32 lastOutput;
} REVHI_DELAYLINE;

typedef struct
{
    REVHI_DELAYLINE AP[9];   /* 0x000: 3 all-passes per channel */
    REVHI_DELAYLINE C[9];    /* 0x0B4: 3 combs per channel */
    f32 allPassCoeff;        /* 0x168 coloration */
    f32 combCoef[9];         /* 0x16C */
    f32 lpLastout[3];        /* 0x190 */
    f32 level;               /* 0x19C mix */
    f32 damping;             /* 0x1A0 */
    s32 preDelayTime;        /* 0x1A4 samples */
    f32 crosstalk;           /* 0x1A8 */
    f32 *preDelayLine[3];    /* 0x1AC */
    f32 *preDelayPtr[3];     /* 0x1B8 */
} REVHI_WORK;                /* 0x1C4 on the GC */

typedef struct
{
    REVHI_WORK rv;
    u8 tempDisableFX;        /* 0x1C4 */
    f32 coloration, mix, time, damping, preDelay, crosstalk;   /* 0x1C8.. */
    int live;                /* port: created and not freed */
} AUX_REVERBHI;

/* lbl_80116320: comb lengths (per channel), two all-passes, then one
 * all-pass per channel */
static const s32 s_revLens[8] = { 1789, 1999, 2333, 433, 149, 47, 73, 67 };

static void dl_init(REVHI_DELAYLINE *dl, s32 len, f32 zero)
{
    s32 n = len + 2;
    dl->length = n * 4;
    dl->inputs = salMalloc((u32)dl->length);
    memset(dl->inputs, 0, (size_t)dl->length);
    dl->lastOutput = zero;
    dl->outPoint = dl->inPoint - (n >> 1) * 4;
    while (dl->outPoint < 0)
        dl->outPoint += dl->length;
    dl->inPoint = 0;
    dl->outPoint = 0;
    dl->outPoint = dl->inPoint - len * 4;
    while (dl->outPoint < 0)
        dl->outPoint += dl->length;
}

static int revhi_params_ok(f32 coloration, f32 time, f32 mix, f32 damping, f32 preDelay, f32 crosstalk,
                           f32 preDelayMax)
{
    if (coloration < 0.0f || coloration > 1.0f)
        return 0;
    if (time < 0.01f || time > 10.0f)
        return 0;
    if (mix < 0.0f || mix > 1.0f)
        return 0;
    if (crosstalk < 0.0f || crosstalk > 1.0f)
        return 0;
    if (damping < 0.0f || damping > 1.0f)
        return 0;
    if (preDelay < 0.0f || !(preDelay <= preDelayMax))
        return 0;
    return 1;
}

static void revhi_set_mix_params(REVHI_WORK *rev, f32 coloration, f32 mix, f32 damping, f32 crosstalk)
{
    rev->allPassCoeff = coloration;
    rev->level = mix;
    rev->crosstalk = crosstalk;
    rev->damping = damping;
    if (!(rev->damping >= 0.05f))
        rev->damping = 0.05f;
    rev->damping = 1.0f - (0.05f + 0.8f * rev->damping);
}

/* ReverbHICreate (0x80052174) */
static int ReverbHICreate(REVHI_WORK *rev, f32 coloration, f32 time, f32 mix, f32 damping, f32 preDelay,
                          f32 crosstalk)
{
    f32 tm;
    int ch, i;
    if (!revhi_params_ok(coloration, time, mix, damping, preDelay, crosstalk, 0.1f))
        return 0;
    memset(rev, 0, sizeof(*rev));
    tm = 32000.0f * time;
    for (ch = 0; ch < 3; ch++) {
        for (i = 0; i < 3; i++) {
            s32 len = s_revLens[i];
            dl_init(&rev->C[ch * 3 + i], len, 0.0f);
            rev->combCoef[ch * 3 + i] = powf(10.0f, (f32)(-3 * len) / tm);
        }
        for (i = 0; i < 2; i++)
            dl_init(&rev->AP[ch * 3 + i], s_revLens[3 + i], 0.0f);
        dl_init(&rev->AP[ch * 3 + 2], s_revLens[5 + ch], 0.0f);
        rev->lpLastout[ch] = 0.0f;
    }
    revhi_set_mix_params(rev, coloration, mix, damping, crosstalk);
    if (preDelay != 0.0f) {
        rev->preDelayTime = fctiwz(32000.0f * preDelay);
        for (i = 0; i < 3; i++) {
            rev->preDelayLine[i] = salMalloc((u32)rev->preDelayTime * 4);
            memset(rev->preDelayLine[i], 0, (size_t)rev->preDelayTime * 4);
            rev->preDelayPtr[i] = rev->preDelayLine[i];
        }
    } else {
        rev->preDelayTime = 0;
        for (i = 0; i < 3; i++) {
            rev->preDelayPtr[i] = NULL;
            rev->preDelayLine[i] = NULL;
        }
    }
    return 1;
}

/* ReverbHIFree (0x80052FC8) */
static void ReverbHIFree(REVHI_WORK *rev)
{
    int i;
    for (i = 0; i < 9; i++)
        salFree(rev->AP[i].inputs);
    for (i = 0; i < 9; i++)
        salFree(rev->C[i].inputs);
    if (rev->preDelayTime != 0)
        for (i = 0; i < 3; i++)
            salFree(rev->preDelayLine[i]);
}

/* ReverbHIModify (0x80052640): note the looser pre-delay check (<= 100)
 * before the buffers are freed and ReverbHICreate (<= 0.1) runs. */
static int ReverbHIModify(REVHI_WORK *rev, f32 coloration, f32 time, f32 mix, f32 damping, f32 preDelay,
                          f32 crosstalk)
{
    if (!revhi_params_ok(coloration, time, mix, damping, preDelay, crosstalk, 100.0f))
        return 0;
    revhi_set_mix_params(rev, coloration, mix, damping, crosstalk);
    if (!(preDelay <= 0.1f)) {
        /* 0.1 < preDelay <= 100: the DOL frees every buffer, then
         * ReverbHICreate rejects the value and the effect keeps running on
         * the freed memory (whose contents survive until the heap reuses
         * it).  The port keeps the buffers instead of reading freed ones. */
        return 0;
    }
    ReverbHIFree(rev);
    return ReverbHICreate(rev, coloration, time, mix, damping, preDelay, crosstalk);
}

/* DoCrossTalk (0x80052834), paired-single code: per sample pair
 *   L' = L*b + R*a,  R' = L*(0.6a) + R*(0.6b)   (a = 0.5*crosstalk, b = 1-a)
 * each product and the sum rounded to single, fctiw (round to nearest). */
static void DoCrossTalk(s32 *l, s32 *r, f32 a, f32 b)
{
    f32 a6 = a * 0.6f, b6 = b * 0.6f;
    int i;
    for (i = 0; i < 160; i++) {
        f32 L = (f32)l[i], R = (f32)r[i];
        f32 nl = L * b + R * a;
        f32 nr = L * a6 + R * b6;
        l[i] = fctiw(nl);
        r[i] = fctiw(nr);
    }
}

#define DLF(dl, off) (*(f32 *)((u8 *)(dl)->inputs + (off)))

/* HandleReverb (0x800529C4): 3 parallel combs -> 2 all-passes -> *0.3 ->
 * one-pole damping -> per-channel all-pass, mixed with the dry input. */
static void HandleReverb(s32 *data, REVHI_WORK *rev, s32 k)
{
    REVHI_DELAYLINE *C = &rev->C[k * 3], *AP = &rev->AP[k * 3];
    const f32 ap = rev->allPassCoeff;
    const f32 damping = rev->damping;
    const f32 wet = rev->level * 0.6f;
    const f32 dry = 0.6f - wet;
    const f32 c0 = rev->combCoef[k * 3], c1 = rev->combCoef[k * 3 + 1], c2 = rev->combCoef[k * 3 + 2];
    f32 lp = rev->lpLastout[k];
    const s32 pdt = rev->preDelayTime;
    s32 in0 = C[0].inPoint, out0 = C[0].outPoint, in1 = C[1].inPoint, out1 = C[1].outPoint;
    s32 in2 = C[2].inPoint, out2 = C[2].outPoint;
    f32 cl0 = C[0].lastOutput, cl1 = C[1].lastOutput, cl2 = C[2].lastOutput;
    s32 ai0 = AP[0].inPoint, ao0 = AP[0].outPoint, ai1 = AP[1].inPoint, ao1 = AP[1].outPoint;
    s32 ai2 = AP[2].inPoint, ao2 = AP[2].outPoint;
    f32 al0 = AP[0].lastOutput, al1 = AP[1].lastOutput, al2 = AP[2].lastOutput;
    f32 x = (f32)data[0];
    int i;

    for (i = 0; i < 160; i++) {
        f32 xd = x, t, w;
        if (pdt != 0) {
            /* The DOL reloads preDelayPtr[k] every sample but stores the
             * advanced pointer to preDelayLine[k] + 0x1B8 bytes instead of
             * back into preDelayPtr[k] (stw r29, 0x1b8(r30) with r30 = the
             * line, not the effect struct).  So the pointer never moves and
             * the "pre-delay" is one sample, always through line[0]; the
             * stray store lands in a slot that is never read (line[110],
             * in bounds for any pre-delay above 110 samples / 3.4 ms). */
            f32 *p = rev->preDelayPtr[k];
            p++;
            xd = p[-1];
            p[-1] = x;
        }
        /* combs */
        t = fmadds(c0, cl0, xd);
        DLF(&C[0], in0) = t;
        in0 += 4;
        w = fmadds(c1, cl1, xd);
        DLF(&C[1], in1) = w;
        in1 += 4;
        t = DLF(&C[0], out0);
        out0 += 4;
        cl1 = DLF(&C[1], out1);
        out1 += 4;
        if (in0 == C[0].length) in0 = 0;
        if (out0 == C[0].length) out0 = 0;
        if (in1 == C[1].length) in1 = 0;
        if (out1 == C[1].length) out1 = 0;
        cl0 = t;
        t = t + cl1;
        w = fmadds(c2, cl2, xd);
        DLF(&C[2], in2) = w;
        in2 += 4;
        cl2 = DLF(&C[2], out2);
        out2 += 4;
        if (in2 == C[2].length) in2 = 0;
        if (out2 == C[2].length) out2 = 0;
        t = t + cl2;
        /* all-pass 1 */
        w = fmadds(ap, al0, t);
        DLF(&AP[0], ai0) = w;
        t = fnmsubs(ap, w, al0);
        ai0 += 4;
        al0 = DLF(&AP[0], ao0);
        ao0 += 4;
        if (ai0 == AP[0].length) ai0 = 0;
        if (ao0 == AP[0].length) ao0 = 0;
        /* all-pass 2 */
        w = fmadds(ap, al1, t);
        DLF(&AP[1], ai1) = w;
        t = fnmsubs(ap, w, al1);
        ai1 += 4;
        al1 = DLF(&AP[1], ao1);
        ao1 += 4;
        if (ai1 == AP[1].length) ai1 = 0;
        if (ao1 == AP[1].length) ao1 = 0;
        /* damping */
        t = t * 0.3f;
        t = fmadds(damping, lp, t);
        lp = t;
        /* per-channel all-pass */
        w = fmadds(ap, al2, t);
        DLF(&AP[2], ai2) = w;
        t = fnmsubs(ap, w, al2);
        al2 = DLF(&AP[2], ao2);
        ai2 += 4;
        ao2 += 4;
        if (ai2 == AP[2].length) ai2 = 0;
        if (ao2 == AP[2].length) ao2 = 0;
        /* output */
        t = fmadds(wet, t, dry * x);
        if (i < 159)
            x = (f32)data[i + 1];
        data[i] = fctiwz(t);
    }

    C[0].inPoint = in0; C[0].outPoint = out0;
    C[1].inPoint = in1; C[1].outPoint = out1;
    C[2].inPoint = in2; C[2].outPoint = out2;
    C[0].lastOutput = cl0; C[1].lastOutput = cl1; C[2].lastOutput = cl2;
    AP[0].inPoint = ai0; AP[0].outPoint = ao0;
    AP[1].inPoint = ai1; AP[1].outPoint = ao1;
    AP[2].inPoint = ai2; AP[2].outPoint = ao2;
    AP[0].lastOutput = al0; AP[1].lastOutput = al1; AP[2].lastOutput = al2;
    rev->lpLastout[k] = lp;
}

/* ReverbHICallback (0x80052ED0) */
static void ReverbHICallback(s32 *left, s32 *right, s32 *sur, REVHI_WORK *rev)
{
    if (0.0f != rev->crosstalk) {
        f32 a = 0.5f * rev->crosstalk;
        DoCrossTalk(left, right, a, 1.0f - a);
    }
    HandleReverb(left, rev, 0);
    HandleReverb(right, rev, 1);
    HandleReverb(sur, rev, 2);
}

/* ------------------------------------------------------------------ */
/* Chorus                                                              */

typedef struct
{
    s32 *dest;
    s32 *smpBase;
    s32 *old;
    u32 posLo, posHi;
    u32 pitchLo, pitchHi;
    u32 trigger, target;
} CHORUS_SRCINFO;

typedef struct
{
    s32 *lastLeft[3];        /* 0x00: 3 x 160-sample blocks = one 480-sample ring */
    s32 *lastRight[3];       /* 0x0C */
    s32 *lastSur[3];         /* 0x18 */
    u8 currentLast;          /* 0x24 */
    s32 oldLeft[4];          /* 0x28 */
    s32 oldRight[4];         /* 0x38 */
    s32 oldSur[4];           /* 0x48 */
    u32 currentPosLo;        /* 0x58 */
    u32 currentPosHi;        /* 0x5C */
    s32 pitchOffset;         /* 0x60 */
    u32 pitchOffsetPeriodCount; /* 0x64 */
    u32 pitchOffsetPeriod;   /* 0x68 */
    CHORUS_SRCINFO src;      /* 0x6C */
} CHORUS_WORK;

typedef struct
{
    CHORUS_WORK work;
    u32 baseDelay, variation, period;   /* 0x90.. */
    int live;
} AUX_CHORUS;

/* rsmpTab12khz (0x80116340): 128 phases x 4 taps */
static const u32 s_rsmpTabBits[512] = {
#include "musyx_rsmptab.inc"
};
#define RSMP(i) (((const f32 *)s_rsmpTabBits)[i])

/* Filter phase for the fractional position: the DOL computes the table
 * offset as rlwinm r10, posLo, 7, 21, 27, i.e. rotl(posLo, 7) & 0x7F0,
 * which takes posLo's top 3 bits and its low 4 bits - not the top 7 bits
 * (that would be a shift of 11).  posLo only ever has its upper 16 bits set
 * (pitchLo = fraction << 16), so the chorus interpolates with just 8 of the
 * 128 phases (0, 16, .., 112). */
static inline u32 rsmp_phase(u32 posLo)
{
    return (((posLo << 7) | (posLo >> 25)) & 0x7F0) >> 4;
}

/* do_src1 (0x800530AC): read the ring at a rate below 1 (pitchHi == 0) */
static void do_src1(CHORUS_SRCINFO *src)
{
    u32 posLo = src->posLo, posHi = src->posHi;
    const u32 pitchLo = src->pitchLo, trigger = src->trigger, target = src->target;
    const s32 *smp = src->smpBase;
    s32 *dest = src->dest;
    f32 f1 = (f32)src->old[0], f2 = (f32)src->old[1], f3 = (f32)src->old[2];
    f32 f4 = (f32)smp[posHi];
    int n;
    for (n = 0; n < 160; n++) {
        const f32 *t = &RSMP(rsmp_phase(posLo) * 4);
        u32 prev = posLo;
        f32 acc;
        posLo += pitchLo;
        acc = f1 * t[0];
        acc = fmadds(f2, t[1], acc);
        acc = fmadds(f3, t[2], acc);
        acc = fmadds(f4, t[3], acc);
        if (posLo < prev) {   /* carry: advance one input sample */
            posHi++;
            f1 = f2;
            f2 = f3;
            f3 = f4;
            if (posHi == trigger)
                posHi = target;
            if (n < 159)
                f4 = (f32)smp[posHi];
        }
        *dest++ = fctiwz(acc);
    }
    src->old[0] = fctiwz(f1);
    src->old[1] = fctiwz(f2);
    src->old[2] = fctiwz(f3);
    src->posLo = posLo;
    src->posHi = posHi;
}

/* do_src2 (0x80053244): rate 1..2 (pitchHi == 1) */
static void do_src2(CHORUS_SRCINFO *src)
{
    u32 posLo = src->posLo, posHi = src->posHi;
    const u32 pitchLo = src->pitchLo, trigger = src->trigger, target = src->target;
    const s32 *smp = src->smpBase;
    s32 *dest = src->dest;
    f32 f1 = (f32)src->old[0], f2 = (f32)src->old[1], f3 = (f32)src->old[2];
    f32 f4 = (f32)smp[posHi];
    int n;
    for (n = 0; n < 160; n++) {
        const f32 *t = &RSMP(rsmp_phase(posLo) * 4);
        u32 prev = posLo;
        f32 acc;
        posLo += pitchLo;
        posHi++;
        if (!(posLo < prev)) {   /* advance one input sample */
            acc = f1 * t[0];
            acc = fmadds(f2, t[1], acc);
            acc = fmadds(f3, t[2], acc);
            f1 = f2;
            f2 = f3;
            f3 = f4;
            if (posHi == trigger)
                posHi = target;
            acc = fmadds(f4, t[3], acc);
        } else {                 /* carry: advance two */
            f32 nx;
            if (posHi == trigger)
                posHi = target;
            nx = (f32)smp[posHi];
            posHi++;
            acc = f1 * t[0];
            acc = fmadds(f2, t[1], acc);
            acc = fmadds(f3, t[2], acc);
            f1 = f3;
            f2 = f4;
            f3 = nx;
            if (posHi == trigger)
                posHi = target;
            acc = fmadds(f4, t[3], acc);
        }
        if (n < 159)
            f4 = (f32)smp[posHi];
        *dest++ = fctiwz(acc);
    }
    src->old[0] = fctiwz(f1);
    src->old[1] = fctiwz(f2);
    src->old[2] = fctiwz(f3);
    src->posLo = posLo;
    src->posHi = posHi;
}

static void chorus_settings(AUX_CHORUS *ch)
{
    CHORUS_WORK *w = &ch->work;
    w->currentPosHi = 0x140 - (ch->baseDelay - 5) * 32;
    w->currentPosLo = 0;
    w->currentPosHi = (w->currentPosHi + (u32)(w->currentLast - 1) * 160) % 480;
    w->pitchOffsetPeriod = (ch->period / 5 + 1) & ~1u;
    w->pitchOffsetPeriodCount = w->pitchOffsetPeriod >> 1;
    /* divwu by zero (period < 5) is undefined on PPC; Dolphin yields 0 */
    w->pitchOffset = w->pitchOffsetPeriod
                   ? (s32)((ch->variation << 16) / (w->pitchOffsetPeriod * 5)) : 0;
}

/* sndAuxCallbackPrepareChorus (0x80053834) */
static int PrepareChorus(AUX_CHORUS *ch)
{
    CHORUS_WORK *w = &ch->work;
    s32 *base = salMalloc(0x1680);
    int i;
    if (!base)
        return 0;
    w->lastLeft[0] = base;
    w->lastRight[0] = base + 0x780 / 4;
    w->lastSur[0] = w->lastRight[0] + 0x780 / 4;
    for (i = 1; i < 3; i++) {
        w->lastLeft[i] = w->lastLeft[0] + i * 160;
        w->lastRight[i] = w->lastRight[0] + i * 160;
        w->lastSur[i] = w->lastSur[0] + i * 160;
    }
    /* The DOL clears only lastX[0] (40 x 4 words each); blocks 1 and 2 keep
     * whatever the game heap held, and block 1 is read (output) by the
     * second callback before it is first written.  salMalloc zero-fills
     * here, so the port outputs silence for that 5 ms block. */
    for (i = 0; i < 160; i++)
        w->lastLeft[0][i] = w->lastRight[0][i] = w->lastSur[0][i] = 0;
    w->currentLast = 1;
    for (i = 0; i < 4; i++)
        w->oldLeft[i] = w->oldRight[i] = w->oldSur[i] = 0;
    w->src.trigger = 480;
    w->src.target = 0;
    chorus_settings(ch);
    return 1;
}

/* sndAuxCallbackChorus (0x80053430) body for reason 0 */
static void ChorusCallback(s32 *left, s32 *right, s32 *sur, CHORUS_WORK *w)
{
    u8 cur = (u8)((w->currentLast + 1) % 3);
    int i, c;
    for (i = 0; i < 160; i++) {
        w->lastLeft[cur][i] = left[i];
        w->lastRight[cur][i] = right[i];
        w->lastSur[cur][i] = sur[i];
    }
    w->src.pitchHi = (u32)((w->pitchOffset >> 16) + 1);
    w->src.pitchLo = (u32)(w->pitchOffset & 0xFFFF) << 16;
    if (--w->pitchOffsetPeriodCount == 0) {
        w->pitchOffsetPeriodCount = w->pitchOffsetPeriod;
        w->pitchOffset = -w->pitchOffset;
    }
    for (c = 0; c < 3; c++) {
        w->src.posHi = w->currentPosHi;
        w->src.posLo = w->currentPosLo;
        if (c == 0) {
            w->src.smpBase = w->lastLeft[0];
            w->src.dest = left;
            w->src.old = w->oldLeft;
        } else if (c == 1) {
            w->src.smpBase = w->lastRight[0];
            w->src.dest = right;
            w->src.old = w->oldRight;
        } else {
            w->src.smpBase = w->lastSur[0];
            w->src.dest = sur;
            w->src.old = w->oldSur;
        }
        if ((s32)w->src.pitchHi == 0)
            do_src1(&w->src);
        else if ((s32)w->src.pitchHi == 1)
            do_src2(&w->src);
    }
    w->currentPosHi = w->src.posHi % 480;
    w->currentPosLo = w->src.posLo;
    w->currentLast = cur;
}

/* ------------------------------------------------------------------ */
/* callbacks (run on the audio worker)                                 */

typedef ADSP_AUX_INFO SND_AUX_INFO_BUF;   /* SND_AUX_INFO.data.bufferUpdate */

/* sndAuxCallbackReverbHI (0x8005205C) */
void fn_8005205C(u8 reason, void *info, void *user)
{
    AUX_REVERBHI *rev = user;
    SND_AUX_INFO_BUF *in = info;
    if (reason != 0)   /* SND_AUX_REASON_PARAMETERUPDATE etc.: ignored */
        return;
    if (!rev->live)    /* port guard: never created, or freed */
        return;
    if (rev->tempDisableFX == 0)
        ReverbHICallback(in->left, in->right, in->surround, &rev->rv);
}

/* sndAuxCallbackChorus (0x80053430) */
void fn_80053430(u8 reason, void *info, void *user)
{
    AUX_CHORUS *ch = user;
    SND_AUX_INFO_BUF *in = info;
    if (reason != 0)
        return;
    if (!ch->live)
        return;
    ChorusCallback(in->left, in->right, in->surround, &ch->work);
}

/* ------------------------------------------------------------------ */
/* game-facing API: mirrors + commands queued to the worker            */

/* game layouts (decomp sound.c: SND_AUX_REVERBHI / SND_AUX_CHORUS) */
typedef struct
{
    u8 work[0x1C4];
    u8 tempDisableFX;
    u8 pad[3];
    f32 coloration, mix, time, damping, preDelay, crosstalk;
} GAME_AUX_REVERBHI;

typedef struct
{
    u8 work[0x90];
    u32 baseDelay, variation, period;
} GAME_AUX_CHORUS;

#define MAX_MIRRORS 4
static struct { const void *game; AUX_REVERBHI m; } s_rev[MAX_MIRRORS];
static struct { const void *game; AUX_CHORUS m; } s_cho[MAX_MIRRORS];

static AUX_REVERBHI *rev_mirror(const void *game, int create)
{
    int i;
    for (i = 0; i < MAX_MIRRORS; i++)
        if (s_rev[i].game == game)
            return &s_rev[i].m;
    if (!create)
        return NULL;
    for (i = 0; i < MAX_MIRRORS; i++)
        if (!s_rev[i].game) {
            s_rev[i].game = game;
            return &s_rev[i].m;
        }
    fprintf(stderr, "[snd-fx] too many reverb structs\n");
    return NULL;
}

static AUX_CHORUS *cho_mirror(const void *game, int create)
{
    int i;
    for (i = 0; i < MAX_MIRRORS; i++)
        if (s_cho[i].game == game)
            return &s_cho[i].m;
    if (!create)
        return NULL;
    for (i = 0; i < MAX_MIRRORS; i++)
        if (!s_cho[i].game) {
            s_cho[i].game = game;
            return &s_cho[i].m;
        }
    fprintf(stderr, "[snd-fx] too many chorus structs\n");
    return NULL;
}

typedef struct
{
    AUX_REVERBHI *m;
    int op;          /* 0 prepare, 1 update, 2 shutdown */
    f32 coloration, mix, time, damping, preDelay, crosstalk;
} RevMsg;

static void rev_worker(void *arg)
{
    RevMsg *msg = arg;
    AUX_REVERBHI *rev = msg->m;
    if (msg->op != 2) {
        rev->coloration = msg->coloration;
        rev->mix = msg->mix;
        rev->time = msg->time;
        rev->damping = msg->damping;
        rev->preDelay = msg->preDelay;
        rev->crosstalk = msg->crosstalk;
    }
    switch (msg->op) {
    case 0:   /* sndAuxCallbackPrepareReverbHI (0x80052110) */
        rev->tempDisableFX = 0;
        if (rev->live) {   /* DOL: a second Prepare leaks the old buffers */
            ReverbHIFree(&rev->rv);
            rev->live = 0;
        }
        if (ReverbHICreate(&rev->rv, rev->coloration, rev->time, rev->mix, rev->damping, rev->preDelay,
                           rev->crosstalk))
            rev->live = 1;
        break;
    case 1:   /* sndAuxCallbackUpdateSettingsReverbHI (0x800520B8) */
        if (!rev->live)
            break;
        rev->tempDisableFX = 1;
        ReverbHIModify(&rev->rv, rev->coloration, rev->time, rev->mix, rev->damping, rev->preDelay,
                       rev->crosstalk);
        rev->tempDisableFX = 0;
        break;
    case 2:   /* sndAuxCallbackShutdownReverbHI */
        if (rev->live)
            ReverbHIFree(&rev->rv);
        rev->live = 0;
        break;
    }
}

static void rev_send(void *game, int op)
{
    GAME_AUX_REVERBHI *g = game;
    RevMsg msg;
    memset(&msg, 0, sizeof(msg));
    msg.m = rev_mirror(game, 1);
    if (!msg.m)
        return;
    msg.op = op;
    msg.coloration = g->coloration;
    msg.mix = g->mix;
    msg.time = g->time;
    msg.damping = g->damping;
    msg.preDelay = g->preDelay;
    msg.crosstalk = g->crosstalk;
    adsp_run(rev_worker, &msg, sizeof(msg));
}

int sndAuxCallbackPrepareReverbHI(void *rev)
{
    GAME_AUX_REVERBHI *g = rev;
    g->tempDisableFX = 0;
    rev_send(rev, 0);
    printf("[snd-fx] PrepareReverbHI coloration %g mix %g time %g damping %g preDelay %g crosstalk %g\n",
           (double)g->coloration, (double)g->mix, (double)g->time, (double)g->damping, (double)g->preDelay,
           (double)g->crosstalk);
    return revhi_params_ok(g->coloration, g->time, g->mix, g->damping, g->preDelay, g->crosstalk, 0.1f);
}

int sndAuxCallbackUpdateSettingsReverbHI(void *rev)
{
    rev_send(rev, 1);
    return 1;
}

/* sndAuxCallbackShutdownReverbHI (0x80052150) */
int g_something_with_sound8_wrapper(void *rev)
{
    rev_send(rev, 2);
    return 1;
}

typedef struct
{
    AUX_CHORUS *m;
    int op;
    u32 baseDelay, variation, period;
} ChoMsg;

static void cho_worker(void *arg)
{
    ChoMsg *msg = arg;
    AUX_CHORUS *ch = msg->m;
    if (msg->op != 2) {
        ch->baseDelay = msg->baseDelay;
        ch->variation = msg->variation;
        ch->period = msg->period;
    }
    switch (msg->op) {
    case 0:
        if (ch->live) {
            salFree(ch->work.lastLeft[0]);
            ch->live = 0;
        }
        if (PrepareChorus(ch))
            ch->live = 1;
        break;
    case 1:   /* sndAuxCallbackUpdateSettingsChorus (0x8005379C) */
        if (ch->live)
            chorus_settings(ch);
        break;
    case 2:   /* sndAuxCallbackShutdownChorus (0x80053A94) */
        if (ch->live)
            salFree(ch->work.lastLeft[0]);
        ch->live = 0;
        break;
    }
}

static void cho_send(void *game, int op)
{
    GAME_AUX_CHORUS *g = game;
    ChoMsg msg;
    memset(&msg, 0, sizeof(msg));
    msg.m = cho_mirror(game, 1);
    if (!msg.m)
        return;
    msg.op = op;
    msg.baseDelay = g->baseDelay;
    msg.variation = g->variation;
    msg.period = g->period;
    adsp_run(cho_worker, &msg, sizeof(msg));
}

int sndAuxCallbackPrepareChorus(void *ch)
{
    GAME_AUX_CHORUS *g = ch;
    cho_send(ch, 0);
    printf("[snd-fx] PrepareChorus baseDelay %u variation %u period %u\n", g->baseDelay, g->variation, g->period);
    return 1;
}

int sndAuxCallbackUpdateSettingsChorus(void *ch)
{
    cho_send(ch, 1);
    return 1;
}

int sndAuxCallbackShutdownChorus(void *ch)
{
    cho_send(ch, 2);
    return 1;
}

/* sndSetAuxProcessingCallbacks (0x8003DDD4) -> hwSetAUXProcessingCallbacks
 * (0x80050E1C): the studio keeps callback + user for aux A and B; the
 * user pointers stay set even when a callback is NULL.  A user pointer
 * that is one of the game's effect structs is replaced by its mirror. */
void sndSetAuxProcessingCallbacks(u8 studio, void (*auxA)(u8, void *, void *), void *userA, u8 midiA, u32 seqA,
                                  void (*auxB)(u8, void *, void *), void *userB, u8 midiB, u32 seqB)
{
    void *ua = userA, *ub = userB, *m;
    (void)seqA; (void)seqB;
    if (studio != 0) {
        fprintf(stderr, "[snd-fx] sndSetAuxProcessingCallbacks: studio %u not emulated\n", studio);
        return;
    }
    if ((m = rev_mirror(userA, 0)) != NULL || (m = cho_mirror(userA, 0)) != NULL)
        ua = m;
    if ((m = rev_mirror(userB, 0)) != NULL || (m = cho_mirror(userB, 0)) != NULL)
        ub = m;
    if ((auxA && midiA != 0xFF) || (auxB && midiB != 0xFF))
        fprintf(stderr, "[snd-fx] aux MIDI parameter updates (midi %u/%u) not implemented\n", midiA, midiB);
    adsp_aux_set((adsp_aux_fn)auxA, ua, (adsp_aux_fn)auxB, ub);
    printf("[snd-fx] aux A %s, aux B %s\n", auxA ? "on" : "off", auxB ? "on" : "off");
}

/* ------------------------------------------------------------------ */
/* self-test hooks (tools/musyxfx/fxtest.c links this file directly)  */

#ifdef MUSYX_FX_TEST
void *musyx_fx_test_reverb_new(f32 coloration, f32 mix, f32 time, f32 damping, f32 preDelay, f32 crosstalk)
{
    AUX_REVERBHI *r = calloc(1, sizeof(*r));
    if (!ReverbHICreate(&r->rv, coloration, time, mix, damping, preDelay, crosstalk)) {
        free(r);
        return NULL;
    }
    r->live = 1;
    return r;
}
void *musyx_fx_test_chorus_new(u32 baseDelay, u32 variation, u32 period)
{
    AUX_CHORUS *c = calloc(1, sizeof(*c));
    c->baseDelay = baseDelay;
    c->variation = variation;
    c->period = period;
    PrepareChorus(c);
    c->live = 1;
    return c;
}
void musyx_fx_test_reverb_update(void *p, f32 coloration, f32 mix, f32 time, f32 damping, f32 preDelay,
                                 f32 crosstalk)
{
    AUX_REVERBHI *r = p;
    r->tempDisableFX = 1;
    ReverbHIModify(&r->rv, coloration, time, mix, damping, preDelay, crosstalk);
    r->tempDisableFX = 0;
}
void musyx_fx_test_chorus_update(void *p, u32 baseDelay, u32 variation, u32 period)
{
    AUX_CHORUS *c = p;
    c->baseDelay = baseDelay;
    c->variation = variation;
    c->period = period;
    chorus_settings(c);
}
#endif

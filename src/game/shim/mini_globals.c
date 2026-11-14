/* main_loop REL data/bss that only the minigame RELs reference.  The
 * main_loop TUs that own them in the original are not (yet) decompiled with
 * these symbols, so they live here.  Sizes from config/GM2E8P/main_loop. */
#include "shim.h"
#include "mathutil.h"

void OSReport(const char *fmt, ...);

s8  lbl_1_bss_D4D2C;
s8  lbl_1_bss_D4D2D[3];
u32 lbl_1_bss_D4D30[2];
void (*lbl_1_bss_D4CE0)(void);   /* dogfight draw hook */
void (*lbl_1_bss_D49B8)(void);   /* pilot2 ripple hook (rend_efc_ripple.c) */
__attribute__((aligned(8))) u8 lbl_1_bss_D4A18[0x84];  /* DogfightPlayer[] */

u8 lbl_1_data_65980[4] = { 0x03, 0x05, 0x0A, 0x00 };

const char *lbl_1_data_6592C[17] = {
    "BMD_INIT", "BMD_REPLAY_INIT", "BMD_PREVIEW_INIT", "BMD_PREVIEW",
    "BMD_COUNTDOWN_INIT", "BMD_COUNTDOWN", "BMD_MAIN", "BMD_GOAL_INIT",
    "BMD_GOAL", "BMD_GPX_NEXT_INIT", "BMD_GPX_NEXT", "BMD_TA_GOAL_INIT",
    "BMD_TA_GOAL", "BMD_COMMEND_INIT", "BMD_COMMEND", "BMD_RANKING_INIT",
    "BMD_RANKING",
};

/* DOL 0x80069870: deformable-model vertex array (skin pre-header @0x08).
 * The pre-header is native-endian after pc_swap_gma. */
void *fn_80069870(void *model)
{
    u8 *m = (u8 *)model;
    if (*(u32 *)(m + 4) & 0x18) {
        u8 *p = m + *(u32 *)(m + 0x20);
        return p + *(u32 *)(p + 8);
    }
    OSReport("fn_80069870: model is not deformable\n");
    return NULL;
}

/* race2 tuning data owned by main_loop */
u16   lbl_1_data_65754 = 0xF300;        /* camera rotX offset */
float lbl_1_data_65778 = 0.92f;         /* 0x3F6B851F */
__attribute__((aligned(8))) u8 lbl_1_bss_D3B88[0x600];  /* Race2DebugCam[0x40] */

/* DOL 0x8006991C: deformable-model pre-header (first word = vertex count) */
void *fn_8006991C(void *model)
{
    u8 *m = (u8 *)model;
    if (*(u32 *)(m + 4) & 0x18)
        return m + *(u32 *)(m + 0x20);
    OSReport("fn_8006991C: model is not deformable\n");
    return NULL;
}

/* DOL 0x800698C0: unpack one 16-bit deformable vertex (0x20 bytes, still
 * big-endian on PC, see endian_gma.c) into the float layout.  GQR6 is s16
 * with scale 2^-13 for position/texcoords and 2^-14 for the normal; the
 * store order (including the 0x18 overwrite) mirrors the asm. */
static float be_s16(const u8 *p, float scale)
{
    return (float)(s16)((p[0] << 8) | p[1]) * scale;
}

void fn_800698C0(void *src, void *dst)
{
    const u8 *s = (const u8 *)src;
    float *d = (float *)dst;
    const float k13 = 1.0f / 8192.0f, k14 = 1.0f / 16384.0f;

    d[0] = be_s16(s + 0x0, k13);  d[1] = be_s16(s + 0x2, k13);
    d[2] = be_s16(s + 0x4, k13);
    d[6] = be_s16(s + 0xC, k13);  d[7] = be_s16(s + 0xE, k13);
    d[8] = be_s16(s + 0x10, k13); d[9] = be_s16(s + 0x12, k13);
    d[10] = be_s16(s + 0x14, k13); d[11] = be_s16(s + 0x16, k13);
    d[3] = be_s16(s + 0x6, k14);
    d[5] = be_s16(s + 0xA, k14);  d[6] = be_s16(s + 0xC, k14);
}

/* DOL 0x80064474: decompose MTXA's rotation into Z/Y/X angles */

void g_math_unk14(s16 *rotZ, s16 *rotY, s16 *rotX)
{
    Vec x = { 1.0f, 0.0f, 0.0f }, y = { 0.0f, 1.0f, 0.0f };

    mtxa_push();
    mtxa_tf_vec(&x, &x);
    mtxa_tf_vec(&y, &y);
    *rotZ = math_atan2(x.y, x.x);
    *rotY = -math_atan2(x.z, math_sqrt(x.x * x.x + x.y * x.y));
    mtxa_from_rotate_z(*rotZ);
    mtxa_rotate_y(*rotY);
    mtxa_rigid_inv_tf_vec(&y, &y);
    *rotX = math_atan2(y.z, y.y);
    mtxa_pop();
}

/* DOL 0x80061AF0: resets a DOL-side debug text page (texobj 0x8018EDC0 +
 * 0x35x0x50 char grid) that nothing in the port renders. */
void fn_80061AF0(void)
{
}

void *get_console_reset_callback(void)
{
    return NULL;
}

/* ---- DTK (disc track streaming) - baseball's stadium music.  There is
 * no DVD audio stream on PC; report "stopped, nothing queued". ---- */
void DTKInit(void) {}
u32  DTKQueueTrack(char *name, void *track, u32 mask, void *cb)
{
    (void)name; (void)track; (void)mask; (void)cb;
    return 1;   /* nonzero = not queued */
}
void DTKFlushTracks(void *cb) { if (cb) ((void (*)(void))cb)(); }
void DTKSetState(u32 s) { (void)s; }
void DTKSetVolume(u8 l, u8 r) { (void)l; (void)r; }
void DTKNextTrack(void) {}
void DTKPrevTrack(void) {}
u32  g_DTK_get_something1(void) { return 0; }   /* DTK state */
u32  g_GTK_get_something2(void) { return 0; }   /* track count */
void g_DTK_set_something(u32 v) { (void)v; }
u32  AIGetStreamSampleCount(void) { return 0; }

/* main_loop .bss 0xD418C: race2 keeps 13 per-phase perf counters here (on GC
 * it runs over the next 12 unnamed words).  The generic 32-byte stub was too
 * small and the overrun zeroed lbl_1_bss_D4188, race2's draw mask. */
u32 lbl_1_bss_D418C[13];

/* DOL .sdata 0x801988E0 points at fullscreen_texture_buf (.data, 0x8C000
 * bytes): the pause menu copies the frame there (take_pausemenu_screenshot)
 * and draws it behind the menu.  The zero stub left the pause screen grey.
 * Retail size is 0x8C000 (640x448 RGB5A3); the PC render mode copies
 * fbWidth x efbHeight = 640x480, so size it for that. */
static u8 fullscreen_texture_buf[640 * 480 * 2] __attribute__((aligned(32)));
void *lbl_801988E0 = fullscreen_texture_buf;

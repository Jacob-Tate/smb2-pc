/* PC port: PlayStation / Xbox / Switch button prompts (see
 * include/button_prompts.h).
 *
 * Icons: the pad-button glyphs of the asc_24x24 ARAM font are 24x24 CMPR
 * pages (textdraw.c g_load_specific_font inits codes 0x839F-0x83BE as
 * GX_TF_CMPR).  For a non-GameCube style:
 *  - pc_button_prompts_hires hands textdraw.c a 128x128 GX RGBA8 icon made
 *    from the style's PNG (assets/prompts/<style>/), which it binds in place
 *    of the 24x24 page, so the icons stay sharp at high resolutions and keep
 *    their soft edges (CMPR only has 1-bit alpha);
 *  - aram_font_to_mram_font (dvd_shim.c) asks pc_button_prompts_glyph for a
 *    replacement 24x24 CMPR page: the PNG scaled down, or, if the PNG is
 *    missing, an icon drawn here procedurally.
 * When the style changes at run time, pc_textdraw_reload_button_glyphs
 * (textdraw.c) re-fetches the loaded pages.
 *
 * Glyphs of the font (decoded from fnt_asc_24x24.bin):
 *   839F A   83A0 B   83A1 L   83A4 Z   83A6 C stick   83AD control stick
 *   83AF R   83B2 Y   83B4 X   83B5 +Control Pad
 *
 * Text: English button words ("A Button", "L,R Button", "START/PAUSE", ...)
 * are rewritten in textdraw_print and the string width helpers (sprite.c). */
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "pc_settings.h"
#include "button_prompts.h"
#include "assets.h"

#define STBI_NO_STDIO
#include "stb_image.h"   /* PNG decoder, stb_image_impl.c */
#include "netplay/session.h"

extern const char *g_button_style_arg;   /* main.c: --button-style */
void pc_textdraw_reload_button_glyphs(void); /* textdraw.c */

/* SDL_GamepadType values (SDL 3.4), kept local so this file does not need the
 * SDL headers.  (SDL2's Luna/Stadia/Shield types are gone: they report as
 * STANDARD, which keeps the GameCube glyphs like any unknown pad.) */
enum {
    SDLT_XBOX360 = 2, SDLT_XBOXONE = 3, SDLT_PS3 = 4, SDLT_PS4 = 5, SDLT_PS5 = 6,
    SDLT_SWITCH_PRO = 7, SDLT_JOYCON_L = 8, SDLT_JOYCON_R = 9, SDLT_JOYCON_PAIR = 10,
};

static const char *const s_style_names[PCB_STYLE_COUNT] = {
    "GameCube", "PlayStation 4", "PlayStation 5", "Xbox", "Switch",
};

/* ------------------------------------------------------------------ style */
static int s_style = PCB_STYLE_GC;
static int s_alt_start;   /* PS3 / Xbox 360: the start button says START */
static int s_resolved;

int pc_button_style_parse(const char *s)
{
    if (!s || !*s) return -1;
    if (!strcmp(s, "auto")) return PCB_SETTING_AUTO;
    if (!strcmp(s, "gc") || !strcmp(s, "gamecube") || !strcmp(s, "ngc")) return PCB_SETTING_GC;
    if (!strcmp(s, "ps") || !strcmp(s, "playstation")) return PCB_SETTING_PS;
    if (!strcmp(s, "ps4")) return PCB_SETTING_PS4;
    if (!strcmp(s, "ps5")) return PCB_SETTING_PS5;
    if (!strcmp(s, "xbox") || !strcmp(s, "xb") || !strcmp(s, "xboxone")) return PCB_SETTING_XBOX;
    if (!strcmp(s, "switch") || !strcmp(s, "nintendo") || !strcmp(s, "ns")) return PCB_SETTING_SWITCH;
    return -1;
}

static void resolve_style(int *style, int *alt_start)
{
    static int s_cli = -2;
    int setting, type;

    if (s_cli == -2) {
        const char *v = g_button_style_arg ? g_button_style_arg : getenv("SMB2_BUTTON_STYLE");
        s_cli = pc_button_style_parse(v);
        if (v && s_cli < 0)
            fprintf(stderr, "[prompts] unknown button style '%s' (auto|gc|ps|ps4|ps5|xbox|switch)\n", v);
    }
    setting = s_cli >= 0 ? s_cli : g_pcs.button_style;
    if (pc_session_active())   /* E1 D9: string widths must not depend on the local device */
        setting = PCB_SETTING_GC;
    type = input_controller_type();
    *alt_start = 0;
    switch (setting) {
    case PCB_SETTING_GC:     *style = PCB_STYLE_GC; return;
    case PCB_SETTING_PS:     *style = type == SDLT_PS4 || type == SDLT_PS3 ? PCB_STYLE_PS4 : PCB_STYLE_PS5;
                             *alt_start = type == SDLT_PS3; return;
    case PCB_SETTING_PS4:    *style = PCB_STYLE_PS4; return;
    case PCB_SETTING_PS5:    *style = PCB_STYLE_PS5; return;
    case PCB_SETTING_XBOX:   *style = PCB_STYLE_XBOX; *alt_start = type == SDLT_XBOX360; return;
    case PCB_SETTING_SWITCH: *style = PCB_STYLE_SWITCH; return;
    }
    switch (type) {
    case SDLT_PS3:     *alt_start = 1; /* fall through */
    case SDLT_PS4:     *style = PCB_STYLE_PS4; return;
    case SDLT_PS5:     *style = PCB_STYLE_PS5; return;
    case SDLT_XBOX360: *alt_start = 1; /* fall through */
    case SDLT_XBOXONE:
                       *style = PCB_STYLE_XBOX; return;   /* Xbox face layout */
    case SDLT_SWITCH_PRO:
    case SDLT_JOYCON_L:
    case SDLT_JOYCON_R:
    case SDLT_JOYCON_PAIR: *style = PCB_STYLE_SWITCH; return;
    }
    *style = PCB_STYLE_GC;   /* keyboard / no / unknown controller */
}

int pc_button_style(void)
{
    if (!s_resolved) {
        resolve_style(&s_style, &s_alt_start);
        s_resolved = 1;
    }
    return s_style;
}

void pc_button_prompts_update(void)
{
    int style, alt;

    resolve_style(&style, &alt);
    if (!s_resolved || style != s_style || alt != s_alt_start) {
        int changed = s_resolved && style != s_style;
        s_style = style;
        s_alt_start = alt;
        s_resolved = 1;
        if (changed) {
            printf("[prompts] button style -> %s\n", s_style_names[style]);
            pc_textdraw_reload_button_glyphs();
        }
    }
}

static int is_ps(int style) { return style == PCB_STYLE_PS4 || style == PCB_STYLE_PS5; }

/* ------------------------------------------------------------- rasterizer
 * 24x24 canvas, 4x4 supersampled; shapes are painted back to front as
 * signed-distance tests (capsule strokes, arcs, discs, rounded boxes). */
#define GW 24
#define SS 4

typedef struct { float r, g, b, a; } Px;
static Px s_img[GW][GW];

typedef struct { float x, y; } V2;

enum { SH_DISC, SH_RING, SH_SEG, SH_ARC, SH_RBOX, SH_RBOX_STROKE };
typedef struct {
    int kind;
    float x0, y0, x1, y1;   /* disc/ring: centre, radius in x1; seg: ends;
                             * arc: centre, radius x1, angles y1..a1 (deg);
                             * rbox: corners */
    float a1, w;            /* arc end angle / box radius; stroke width */
} Shape;

static float seg_dist(float px, float py, float ax, float ay, float bx, float by)
{
    float dx = bx - ax, dy = by - ay, l2 = dx * dx + dy * dy, t;
    t = l2 > 0 ? ((px - ax) * dx + (py - ay) * dy) / l2 : 0;
    t = t < 0 ? 0 : t > 1 ? 1 : t;
    dx = px - (ax + t * dx);
    dy = py - (ay + t * dy);
    return sqrtf(dx * dx + dy * dy);
}

static int inside(const Shape *s, float px, float py)
{
    float dx, dy, d;
    switch (s->kind) {
    case SH_DISC:
        dx = px - s->x0; dy = py - s->y0;
        return dx * dx + dy * dy <= s->x1 * s->x1;
    case SH_RING:
        dx = px - s->x0; dy = py - s->y0;
        d = sqrtf(dx * dx + dy * dy);
        return fabsf(d - s->x1) <= s->w * 0.5f;
    case SH_SEG:
        return seg_dist(px, py, s->x0, s->y0, s->x1, s->y1) <= s->w * 0.5f;
    case SH_ARC: {
        /* screen angles (y down): 0 = right, -90 = up, 90 = down */
        float a, r = s->x1, a0 = s->y1, a1 = s->a1;
        dx = px - s->x0; dy = py - s->y0;
        a = atan2f(dy, dx) * (180.0f / 3.14159265f);
        while (a < a0) a += 360.0f;
        if (a <= a1)
            return fabsf(sqrtf(dx * dx + dy * dy) - r) <= s->w * 0.5f;
        {
            float e0x = s->x0 + r * cosf(a0 * 3.14159265f / 180.0f);
            float e0y = s->y0 + r * sinf(a0 * 3.14159265f / 180.0f);
            float e1x = s->x0 + r * cosf(a1 * 3.14159265f / 180.0f);
            float e1y = s->y0 + r * sinf(a1 * 3.14159265f / 180.0f);
            float d0 = (px - e0x) * (px - e0x) + (py - e0y) * (py - e0y);
            float d1 = (px - e1x) * (px - e1x) + (py - e1y) * (py - e1y);
            float hw = s->w * 0.5f;
            return d0 <= hw * hw || d1 <= hw * hw;
        }
    }
    case SH_RBOX:
    case SH_RBOX_STROKE: {
        float r = s->a1;
        float cx = 0.5f * (s->x0 + s->x1), cy = 0.5f * (s->y0 + s->y1);
        float hx = 0.5f * (s->x1 - s->x0) - r, hy = 0.5f * (s->y1 - s->y0) - r;
        float qx = fabsf(px - cx) - hx, qy = fabsf(py - cy) - hy;
        float ox = qx > 0 ? qx : 0, oy = qy > 0 ? qy : 0;
        float in = qx > qy ? qx : qy;
        d = sqrtf(ox * ox + oy * oy) + (in < 0 ? in : 0) - r;
        if (s->kind == SH_RBOX)
            return d <= 0;
        return fabsf(d + s->w * 0.5f) <= s->w * 0.5f;   /* inner stroke */
    }
    }
    return 0;
}

static void paint(const Shape *s, unsigned rgb)
{
    float cr = ((rgb >> 16) & 0xFF) / 255.0f, cg = ((rgb >> 8) & 0xFF) / 255.0f,
          cb = (rgb & 0xFF) / 255.0f;
    int x, y, i, j;

    for (y = 0; y < GW; y++)
        for (x = 0; x < GW; x++) {
            int hits = 0;
            float c;
            Px *p = &s_img[y][x];
            for (j = 0; j < SS; j++)
                for (i = 0; i < SS; i++)
                    hits += inside(s, x + (i + 0.5f) / SS, y + (j + 0.5f) / SS);
            if (!hits) continue;
            c = (float)hits / (SS * SS);
            /* "over" in premultiplied form */
            p->r = cr * c + p->r * (1 - c);
            p->g = cg * c + p->g * (1 - c);
            p->b = cb * c + p->b * (1 - c);
            p->a = c + p->a * (1 - c);
        }
}

static void disc(float cx, float cy, float r, unsigned rgb)
{ Shape s = { SH_DISC, cx, cy, r, 0, 0, 0 }; paint(&s, rgb); }
static void ring(float cx, float cy, float r, float w, unsigned rgb)
{ Shape s = { SH_RING, cx, cy, r, 0, 0, w }; paint(&s, rgb); }
static void seg(float x0, float y0, float x1, float y1, float w, unsigned rgb)
{ Shape s = { SH_SEG, x0, y0, x1, y1, 0, w }; paint(&s, rgb); }
static void arc(float cx, float cy, float r, float a0, float a1, float w, unsigned rgb)
{ Shape s = { SH_ARC, cx, cy, r, a0, a1, w }; paint(&s, rgb); }
static void rbox(float x0, float y0, float x1, float y1, float r, unsigned rgb)
{ Shape s = { SH_RBOX, x0, y0, x1, y1, r, 0 }; paint(&s, rgb); }
static void rbox_stroke(float x0, float y0, float x1, float y1, float r, float w, unsigned rgb)
{ Shape s = { SH_RBOX_STROKE, x0, y0, x1, y1, r, w }; paint(&s, rgb); }

/* Stroke letters in the box (x0,y0)-(x1,y1). */
static void letter(char ch, float x0, float y0, float x1, float y1, float w, unsigned rgb)
{
    float xm = 0.5f * (x0 + x1), ym = 0.5f * (y0 + y1), h = y1 - y0;
    switch (ch) {
    case 'A':
        seg(x0, y1, xm, y0, w, rgb);
        seg(xm, y0, x1, y1, w, rgb);
        seg(x0 + 0.25f * (x1 - x0), y0 + 0.64f * h, x1 - 0.25f * (x1 - x0), y0 + 0.64f * h, w, rgb);
        break;
    case 'B': {
        float rt = 0.25f * h, rb = 0.26f * h;
        float xt = x1 - rt - 0.08f * (x1 - x0), xb = x1 - rb;
        seg(x0, y0, x0, y1, w, rgb);
        seg(x0, y0, xt, y0, w, rgb);
        arc(xt, y0 + rt, rt, -90, 90, w, rgb);
        seg(xt, ym, x0, ym, w, rgb);
        seg(x0, ym, xb, ym, w, rgb);
        arc(xb, y1 - rb, rb, -90, 90, w, rgb);
        seg(xb, y1, x0, y1, w, rgb);
        break;
    }
    case 'X':
        seg(x0, y0, x1, y1, w, rgb);
        seg(x1, y0, x0, y1, w, rgb);
        break;
    case 'Y':
        seg(x0, y0, xm, ym, w, rgb);
        seg(x1, y0, xm, ym, w, rgb);
        seg(xm, ym, xm, y1, w, rgb);
        break;
    case 'L':
        seg(x0, y0, x0, y1, w, rgb);
        seg(x0, y1, x1, y1, w, rgb);
        break;
    case 'T':
        seg(x0, y0, x1, y0, w, rgb);
        seg(xm, y0, xm, y1, w, rgb);
        break;
    case 'R': {
        float r = 0.25f * h, xr = x1 - r;
        seg(x0, y0, x0, y1, w, rgb);
        seg(x0, y0, xr, y0, w, rgb);
        arc(xr, y0 + r, r, -90, 90, w, rgb);
        seg(xr, ym, x0, ym, w, rgb);
        seg(xr - 0.1f * (x1 - x0), ym, x1, y1, w, rgb);
        break;
    }
    case '2': {
        float r = 0.5f * (x1 - x0);
        arc(xm, y0 + r, r, -180, 35, w, rgb);
        seg(xm + r * 0.819f, y0 + r + r * 0.574f, x0, y1, w, rgb);
        seg(x0, y1, x1, y1, w, rgb);
        break;
    }
    case '1':
        seg(xm - 0.3f * (x1 - x0), y0 + 0.2f * h, xm + 0.1f * (x1 - x0), y0, w, rgb);
        seg(xm + 0.1f * (x1 - x0), y0, xm + 0.1f * (x1 - x0), y1, w, rgb);
        break;
    case 'Z':
        seg(x0, y0, x1, y0, w, rgb);
        seg(x1, y0, x0, y1, w, rgb);
        seg(x0, y1, x1, y1, w, rgb);
        break;
    }
}

/* palette */

/* GC pad-button icons of the font */
enum { IC_A, IC_B, IC_X, IC_Y, IC_L, IC_R, IC_Z, IC_STICK, IC_CSTICK, IC_DPAD, IC_COUNT };

static int glyph_icon(unsigned code)
{
    switch (code) {
    case 0x839F: return IC_A;
    case 0x83A0: return IC_B;
    case 0x83B4: return IC_X;
    case 0x83B2: return IC_Y;
    case 0x83A1: return IC_L;
    case 0x83AF: return IC_R;
    case 0x83A4: return IC_Z;
    case 0x83AD: return IC_STICK;    /* control stick -> left stick */
    case 0x83A6: return IC_CSTICK;   /* C stick -> right stick */
    case 0x83B5: return IC_DPAD;     /* +Control Pad */
    }
    return -1;
}

/* palette (code-drawn fallback icons) */
#define C_BODY   0x1E1F26u   /* dark button body */
#define C_RIM    0x9AA0ABu   /* light rim, keeps the icon readable on dark */
#define C_WHITE  0xF4F4F4u

static void face_button_base(void)
{
    /* two tones only: CMPR blocks hold 3-4 colours, so a separate outline
     * ring would dither */
    disc(12, 12, 11.5f, C_RIM);
    disc(12, 12, 9.9f, C_BODY);
}

/* PlayStation symbol colours (classic) and Xbox letter colours */
#define PS_CROSS    0x7DB5F5u
#define PS_CIRCLE   0xFF6A6Au
#define PS_SQUARE   0xF58CE6u
#define PS_TRIANGLE 0x3FDDA6u
#define XB_A        0x6CC24Au
#define XB_B        0xF0453Au
#define XB_X        0x3D8EF0u
#define XB_Y        0xF7C21Bu

/* Code-drawn fallback for a missing PNG.  Returns 0 if not drawn (the
 * GameCube glyph stays). */
static int draw_icon(int style, int icon)
{
    static const char gc_letter[] = "ABXY";
    /* Switch pads map by label (input_sdl2.c): GC A is Switch A, etc. */
    static const char sw_letter[] = "ABXY";

    memset(s_img, 0, sizeof s_img);
    switch (icon) {
    case IC_A: case IC_B: case IC_X: case IC_Y:
        face_button_base();
        if (is_ps(style)) {
            const float sw = 2.3f;
            switch (icon) {
            case IC_A: seg(8, 8, 16, 16, sw, PS_CROSS); seg(16, 8, 8, 16, sw, PS_CROSS); break;
            case IC_B: ring(12, 12, 4.9f, sw, PS_CIRCLE); break;
            case IC_X: rbox_stroke(7.4f, 7.4f, 16.6f, 16.6f, 0.6f, sw, PS_SQUARE); break;
            case IC_Y:
                seg(12, 7.3f, 16.9f, 15.6f, sw, PS_TRIANGLE);
                seg(16.9f, 15.6f, 7.1f, 15.6f, sw, PS_TRIANGLE);
                seg(7.1f, 15.6f, 12, 7.3f, sw, PS_TRIANGLE);
                break;
            }
        } else if (style == PCB_STYLE_SWITCH) {
            letter(sw_letter[icon - IC_A], 8.6f, 7.4f, 15.4f, 16.6f, 2.4f, C_WHITE);
        } else {
            static const unsigned c[4] = { XB_A, XB_B, XB_X, XB_Y };
            letter(gc_letter[icon - IC_A], 8.6f, 7.4f, 15.4f, 16.6f, 2.4f, c[icon - IC_A]);
        }
        return 1;
    case IC_L: case IC_R: {
        /* trigger: a rounded tab with "L2"/"R2", "LT"/"RT" or "ZL"/"ZR" */
        char first = icon == IC_L ? 'L' : 'R', second = is_ps(style) ? '2' : 'T';
        rbox(0.5f, 3.5f, 23.5f, 20.5f, 5.0f, C_RIM);
        rbox(2.1f, 5.1f, 21.9f, 18.9f, 3.6f, C_BODY);
        if (style == PCB_STYLE_SWITCH) { second = first; first = 'Z'; }
        letter(first, 5.4f, 8.0f, 10.6f, 16.0f, 1.8f, C_WHITE);
        letter(second, 13.2f, 8.0f, 18.6f, 16.0f, 1.8f, C_WHITE);
        return 1;
    }
    case IC_Z:
        /* right bumper: a flat tab with "RB" / "R1" / "R" */
        rbox(0.5f, 5.5f, 23.5f, 18.5f, 3.0f, C_RIM);
        rbox(2.1f, 7.1f, 21.9f, 16.9f, 2.0f, C_BODY);
        if (style == PCB_STYLE_SWITCH) {
            letter('R', 9.4f, 8.6f, 14.6f, 15.4f, 1.8f, C_WHITE);
        } else {
            letter('R', 5.4f, 8.6f, 10.6f, 15.4f, 1.8f, C_WHITE);
            letter(is_ps(style) ? '1' : 'B', 13.2f, 8.6f, 18.6f, 15.4f, 1.8f, C_WHITE);
        }
        return 1;
    case IC_STICK: case IC_CSTICK:
        /* analog stick seen from above: left (control stick) or right
         * (C stick) */
        disc(12, 12, 11.5f, 0x6A6F7Au);
        disc(12, 12, 8.8f, C_BODY);
        letter(icon == IC_STICK ? 'L' : 'R', 9.0f, 8.0f, 15.0f, 16.0f, 2.0f, C_WHITE);
        return 1;
    }
    return 0;   /* D-pad: the GameCube glyph is generic enough */
}

/* ------------------------------------------------------------ CMPR encode
 * GX CMPR: 8x8 tiles of four 4x4 DXT1 blocks (TL, TR, BL, BR), colours
 * big-endian RGB565, 2-bit indices MSB-first per row.  Blocks with
 * transparent texels use 3-colour mode (c0 <= c1, index 3 = transparent). */
static unsigned to565(const float *c)
{
    int r = (int)(c[0] * 31 + 0.5f), g = (int)(c[1] * 63 + 0.5f), b = (int)(c[2] * 31 + 0.5f);
    r = r < 0 ? 0 : r > 31 ? 31 : r;
    g = g < 0 ? 0 : g > 63 ? 63 : g;
    b = b < 0 ? 0 : b > 31 ? 31 : b;
    return (unsigned)(r << 11 | g << 5 | b);
}

static void from565(unsigned v, float *c)
{
    c[0] = ((v >> 11) & 31) / 31.0f;
    c[1] = ((v >> 5) & 63) / 63.0f;
    c[2] = (v & 31) / 31.0f;
}

/* palette of endpoints c0/c1 in the given mode (four: 4-colour mode) */
static void block_palette(unsigned c0, unsigned c1, int four, float pal[4][3])
{
    int i;
    from565(c0, pal[0]);
    from565(c1, pal[1]);
    for (i = 0; i < 3; i++) {
        if (four) {
            pal[2][i] = (2 * pal[0][i] + pal[1][i]) / 3;
            pal[3][i] = (pal[0][i] + 2 * pal[1][i]) / 3;
        } else {
            pal[2][i] = 0.5f * (pal[0][i] + pal[1][i]);
            pal[3][i] = 0;   /* transparent */
        }
    }
}

static int nearest(const float *c, float pal[4][3], int n, float *err)
{
    int m, idx = 0;
    float bd = 1e9f;
    for (m = 0; m < n; m++) {
        float d0 = c[0] - pal[m][0], d1 = c[1] - pal[m][1], d2 = c[2] - pal[m][2];
        float d = d0 * d0 + d1 * d1 + d2 * d2;
        if (d < bd) { bd = d; idx = m; }
    }
    *err = bd;
    return idx;
}

/* One 4x4 block: try every pair of the block's own colours as endpoints and
 * keep the pair (and mode) with the least error. */
static void encode_block(int bx, int by, unsigned char *out)
{
    float col[16][3], pal[4][3], err;
    int opaque[16], i, j, k, n = 0, any_clear = 0;
    unsigned c565[16], best0 = 0, best1 = 0;
    float best = 1e30f;

    for (i = 0; i < 16; i++) {
        const Px *p = &s_img[by + i / 4][bx + i % 4];
        opaque[i] = p->a >= 0.5f;
        if (opaque[i]) {
            col[i][0] = p->r / p->a;
            col[i][1] = p->g / p->a;
            col[i][2] = p->b / p->a;
            c565[n++] = to565(col[i]);
        } else {
            any_clear = 1;
        }
    }
    if (n == 0) {   /* fully transparent: 3-colour mode, all index 3 */
        out[0] = out[1] = out[2] = out[3] = 0;
        out[4] = out[5] = out[6] = out[7] = 0xFF;
        return;
    }
    for (i = 0; i < n; i++)
        for (j = i; j < n; j++) {
            unsigned hi = c565[i] > c565[j] ? c565[i] : c565[j];
            unsigned lo = c565[i] > c565[j] ? c565[j] : c565[i];
            int mode;
            for (mode = 0; mode < 2; mode++) {
                /* mode 0: 3 colours + transparent (c0 <= c1); mode 1: 4
                 * colours (c0 > c1), only without transparent texels */
                float e = 0;
                if (mode == 1 && (any_clear || hi == lo)) continue;
                if (mode == 0) block_palette(lo, hi, 0, pal);
                else block_palette(hi, lo, 1, pal);
                for (k = 0; k < 16 && e < best; k++)
                    if (opaque[k]) { nearest(col[k], pal, mode ? 4 : 3, &err); e += err; }
                if (e < best) {
                    best = e;
                    best0 = mode ? hi : lo;
                    best1 = mode ? lo : hi;
                }
            }
        }
    {
        int four = best0 > best1;
        block_palette(best0, best1, four, pal);
        out[0] = (unsigned char)(best0 >> 8); out[1] = (unsigned char)best0;
        out[2] = (unsigned char)(best1 >> 8); out[3] = (unsigned char)best1;
        for (j = 0; j < 4; j++) {
            unsigned char row = 0;
            for (i = 0; i < 4; i++) {
                int t = j * 4 + i;
                int idx = opaque[t] ? nearest(col[t], pal, four ? 4 : 3, &err) : 3;
                row |= (unsigned char)(idx << (6 - 2 * i));
            }
            out[4 + j] = row;
        }
    }
}

static void encode_cmpr(unsigned char *dest)
{
    int tx, ty, sb;
    for (ty = 0; ty < GW; ty += 8)
        for (tx = 0; tx < GW; tx += 8)
            for (sb = 0; sb < 4; sb++, dest += 8)
                encode_block(tx + (sb & 1) * 4, ty + (sb >> 1) * 4, dest);
}


/* ------------------------------------------------------------- PNG icons
 * assets/prompts/<style>/<file>.png: 128x128 RGBA icons (Default variant of
 * the controller icon pack).  Each icon's opaque bounding box is fitted into
 * the glyph cell, resampled in premultiplied linear light (tent filter as
 * wide as the scale factor, so downscaling averages every source texel), and
 * stored with straight alpha; fully transparent texels take the colour of
 * their neighbours so bilinear filtering / mipmapping does not pull in a
 * dark fringe. */
#define HI_N 128   /* high-resolution icon size (GX RGBA8, mipmapped) */

static const char *const s_style_dirs[PCB_STYLE_COUNT] = { NULL, "ps4", "ps5", "xbox", "switch" };
static const char *const s_icon_files[PCB_STYLE_COUNT][IC_COUNT] = {
    { NULL },
    { "T_P4_Cross_Color", "T_P4_Circle_Color", "T_P4_Square_Color", "T_P4_Triangle_Color",
      "T_P4_L2", "T_P4_R2", "T_P4_R1", "T_P4_L", "T_P4_R", "T_P4_Dpad" },
    { "T_P5_Cross_Color", "T_P5_Circle_Color", "T_P5_Square_Color", "T_P5_Triangle_Color",
      "T_P5_L2", "T_P5_R2", "T_P5_R1", "T_P5_L", "T_P5_R", "T_P5_Dpad" },
    { "T_X_A_Color", "T_X_B_Color", "T_X_X_Color", "T_X_Y_Color",
      "T_X_LT", "T_X_RT", "T_X_RB", "T_X_L", "T_X_R", "T_X_Dpad" },
    /* by label: GC A/B/X/Y = Switch A/B/X/Y; L/R = ZL/ZR, Z = R */
    { "T_S_A", "T_S_B", "T_S_X", "T_S_Y",
      "T_S_LT", "T_S_RT", "T_S_RB", "T_S_L", "T_S_R", "T_S_Dpad" },
};

typedef struct {
    int tried;
    unsigned char *gx;            /* HI_N x HI_N GX RGBA8, or NULL */
    unsigned char page[0x120];    /* 24x24 CMPR */
} IconEntry;
static IconEntry s_icons[PCB_STYLE_COUNT][IC_COUNT];

static float s_s2l[256];

static float lin_to_srgb(float v)
{
    v = v < 0 ? 0 : v > 1 ? 1 : v;
    return v <= 0.0031308f ? 12.92f * v : 1.055f * powf(v, 1.0f / 2.4f) - 0.055f;
}

static unsigned char *read_file(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    unsigned char *buf;
    long n;

    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = n > 0 ? malloc((size_t)n) : NULL;
    if (buf && fread(buf, 1, (size_t)n, f) != (size_t)n) { free(buf); buf = NULL; }
    fclose(f);
    *len = (size_t)n;
    return buf;
}

/* Fit the opaque part of `src` (w x h RGBA8) into an n x n cell; out = n*n
 * straight-alpha sRGB floats (r, g, b, a). */
static void fit_icon(const unsigned char *src, int w, int h, int n, float *out)
{
    int x, y, i, bx0 = w, by0 = h, bx1 = -1, by1 = -1, pass;
    float *pm, *tmp, s, ox, oy, rad;

    if (!s_s2l[255]) {
        for (i = 0; i < 256; i++) {
            float v = i / 255.0f;
            s_s2l[i] = v <= 0.04045f ? v / 12.92f : powf((v + 0.055f) / 1.055f, 2.4f);
        }
    }
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
            if (src[(y * w + x) * 4 + 3] >= 4) {
                if (x < bx0) bx0 = x;
                if (x > bx1) bx1 = x;
                if (y < by0) by0 = y;
                if (y > by1) by1 = y;
            }
    memset(out, 0, sizeof(float) * 4 * n * n);
    if (bx1 < 0) return;
    pm = malloc(sizeof(float) * 4 * w * h);
    tmp = malloc(sizeof(float) * 4 * n * h);
    if (!pm || !tmp) { free(pm); free(tmp); return; }
    /* premultiplied linear source */
    for (i = 0; i < w * h; i++) {
        float a = src[i * 4 + 3] / 255.0f;
        pm[i * 4 + 0] = s_s2l[src[i * 4 + 0]] * a;
        pm[i * 4 + 1] = s_s2l[src[i * 4 + 1]] * a;
        pm[i * 4 + 2] = s_s2l[src[i * 4 + 2]] * a;
        pm[i * 4 + 3] = a;
    }
    {
        int bw = bx1 - bx0 + 1, bh = by1 - by0 + 1, bm = bw > bh ? bw : bh;
        s = (n * (1.0f - 2.0f / 48.0f)) / (float)bm;   /* ~0.5 px margin at 24 */
        ox = 0.5f * (n - bw * s);
        oy = 0.5f * (n - bh * s);
    }
    rad = s < 1.0f ? 1.0f / s : 1.0f;   /* tent radius in source texels */
    /* horizontal: tmp[y][dx] */
    for (x = 0; x < n; x++) {
        float c = bx0 + (x + 0.5f - ox) / s - 0.5f;   /* source texel centre */
        int k0 = (int)floorf(c - rad), k1 = (int)ceilf(c + rad), k;
        for (y = 0; y < h; y++) {
            float acc[4] = { 0, 0, 0, 0 }, wsum = 0;
            for (k = k0; k <= k1; k++) {
                float wt = 1.0f - fabsf(k - c) / rad;
                if (wt <= 0) continue;
                wsum += wt;
                if (k < 0 || k >= w) continue;   /* transparent outside */
                for (i = 0; i < 4; i++) acc[i] += wt * pm[(y * w + k) * 4 + i];
            }
            for (i = 0; i < 4; i++) tmp[(y * n + x) * 4 + i] = wsum > 0 ? acc[i] / wsum : 0;
        }
    }
    /* vertical: out[dy][dx], straight alpha sRGB */
    for (y = 0; y < n; y++) {
        float c = by0 + (y + 0.5f - oy) / s - 0.5f;
        int k0 = (int)floorf(c - rad), k1 = (int)ceilf(c + rad), k;
        for (x = 0; x < n; x++) {
            float acc[4] = { 0, 0, 0, 0 }, wsum = 0, *o = &out[(y * n + x) * 4];
            for (k = k0; k <= k1; k++) {
                float wt = 1.0f - fabsf(k - c) / rad;
                if (wt <= 0) continue;
                wsum += wt;
                if (k < 0 || k >= h) continue;
                for (i = 0; i < 4; i++) acc[i] += wt * tmp[(k * n + x) * 4 + i];
            }
            if (wsum > 0) for (i = 0; i < 4; i++) acc[i] /= wsum;
            if (acc[3] > 1.0f / 512) {
                o[0] = lin_to_srgb(acc[0] / acc[3]);
                o[1] = lin_to_srgb(acc[1] / acc[3]);
                o[2] = lin_to_srgb(acc[2] / acc[3]);
                o[3] = acc[3] > 1 ? 1 : acc[3];
            } else {
                o[0] = o[1] = o[2] = -1;   /* no colour yet */
                o[3] = 0;
            }
        }
    }
    free(pm);
    free(tmp);
    /* bleed colours into the transparent texels */
    {
        unsigned char *have = malloc((size_t)n * n), *next = malloc((size_t)n * n);
        if (have && next) {
            for (i = 0; i < n * n; i++) have[i] = out[i * 4] >= 0;
            for (pass = 0; pass < n; pass++) {
                int todo = 0;
                memcpy(next, have, (size_t)n * n);
                for (y = 0; y < n; y++)
                    for (x = 0; x < n; x++) {
                        float *o = &out[(y * n + x) * 4], acc[3] = { 0, 0, 0 };
                        int cnt = 0, dx, dy;
                        if (have[y * n + x]) continue;
                        for (dy = -1; dy <= 1; dy++)
                            for (dx = -1; dx <= 1; dx++) {
                                int xx = x + dx, yy = y + dy;
                                const float *q;
                                if (xx < 0 || xx >= n || yy < 0 || yy >= n || !have[yy * n + xx])
                                    continue;
                                q = &out[(yy * n + xx) * 4];
                                acc[0] += q[0]; acc[1] += q[1]; acc[2] += q[2];
                                cnt++;
                            }
                        if (cnt) {
                            o[0] = acc[0] / cnt; o[1] = acc[1] / cnt; o[2] = acc[2] / cnt;
                            next[y * n + x] = 1;
                        } else {
                            todo = 1;
                        }
                    }
                memcpy(have, next, (size_t)n * n);
                if (!todo) break;
            }
        }
        free(have);
        free(next);
    }
    for (i = 0; i < n * n; i++)
        if (out[i * 4] < 0) out[i * 4] = out[i * 4 + 1] = out[i * 4 + 2] = 0;
}

static unsigned char to_u8(float v)
{
    int i = (int)(v * 255.0f + 0.5f);
    return (unsigned char)(i < 0 ? 0 : i > 255 ? 255 : i);
}

/* GX RGBA8: 4x4 tiles of 64 bytes, 16 (A,R) pairs then 16 (G,B) pairs */
static void encode_rgba8(const float *img, int n, unsigned char *dst)
{
    int tx, ty, i;
    for (ty = 0; ty < n; ty += 4)
        for (tx = 0; tx < n; tx += 4, dst += 64)
            for (i = 0; i < 16; i++) {
                const float *p = &img[((ty + i / 4) * n + tx + i % 4) * 4];
                dst[i * 2 + 0] = to_u8(p[3]);
                dst[i * 2 + 1] = to_u8(p[0]);
                dst[32 + i * 2 + 0] = to_u8(p[1]);
                dst[32 + i * 2 + 1] = to_u8(p[2]);
            }
}

static int load_png(const char *dir, const char *file, unsigned char **rgba, int *w, int *h)
{
    char path[1200];
    unsigned char *data = NULL;
    size_t len = 0;
    char *base = fs_get_base_path();
    int comp;

    if (base) {
        snprintf(path, sizeof path, "%sassets/prompts/%s/%s.png", base, dir, file);
        data = read_file(path, &len);
        free(base);
    }
    if (!data) {
        snprintf(path, sizeof path, "assets/prompts/%s/%s.png", dir, file);
        data = read_file(path, &len);
    }
    if (!data) {
        fprintf(stderr, "[prompts] missing %s/%s.png - using a drawn icon\n", dir, file);
        return 0;
    }
    *rgba = stbi_load_from_memory(data, (int)len, w, h, &comp, 4);
    free(data);
    if (!*rgba) {
        fprintf(stderr, "[prompts] cannot decode %s/%s.png: %s\n", dir, file, stbi_failure_reason());
        return 0;
    }
    return 1;
}

static IconEntry *icon_entry(int style, int icon)
{
    IconEntry *e = &s_icons[style][icon];
    unsigned char *rgba;
    int w, h, x, y;
    float *img;

    if (e->tried)
        return e;
    e->tried = 1;
    if (!s_style_dirs[style] || !load_png(s_style_dirs[style], s_icon_files[style][icon], &rgba, &w, &h))
        return e;
    img = malloc(sizeof(float) * 4 * HI_N * HI_N);
    e->gx = malloc(HI_N * HI_N * 4);
    if (!img || !e->gx) {
        free(img); free(e->gx); e->gx = NULL;
        stbi_image_free(rgba);
        return e;
    }
    fit_icon(rgba, w, h, HI_N, img);
    encode_rgba8(img, HI_N, e->gx);
    /* 24x24 CMPR page (premultiplied for encode_block) */
    fit_icon(rgba, w, h, GW, img);
    for (y = 0; y < GW; y++)
        for (x = 0; x < GW; x++) {
            const float *p = &img[(y * GW + x) * 4];
            s_img[y][x].r = p[0] * p[3];
            s_img[y][x].g = p[1] * p[3];
            s_img[y][x].b = p[2] * p[3];
            s_img[y][x].a = p[3];
        }
    encode_cmpr(e->page);
    free(img);
    stbi_image_free(rgba);
    return e;
}

int pc_button_prompts_glyph(unsigned code, void *dest)
{
    int style = pc_button_style(), icon = glyph_icon(code);
    IconEntry *e;

    if (style == PCB_STYLE_GC || !dest || icon < 0)
        return 0;
    e = icon_entry(style, icon);
    if (e->gx) {
        memcpy(dest, e->page, sizeof e->page);
        return 1;
    }
    if (!draw_icon(style, icon))
        return 0;
    encode_cmpr((unsigned char *)dest);   /* 9 tiles x 32 bytes = 0x120 */
    return 1;
}

int pc_button_prompts_hires(unsigned code, void **image, int *size)
{
    int style = pc_button_style(), icon = glyph_icon(code);
    IconEntry *e;

    if (style == PCB_STYLE_GC || icon < 0)
        return 0;
    e = icon_entry(style, icon);
    if (!e->gx)
        return 0;
    *image = e->gx;
    *size = HI_N;
    return 1;
}

/* ----------------------------------------------------------------- text */
struct Repl { const char *from, *to; };

/* Applied in one pass (replacements are not rescanned, so swaps are safe);
 * longer phrases first.  The English rules apply in every language (the
 * START/PAUSE label is used verbatim everywhere, and the German and Italian
 * texts use "Control Stick" as an English term); the game's German, French,
 * Spanish and Italian button words (cp1252, as the game stores them) get the
 * per-language rules below first.  Japanese names the GC buttons with
 * fullwidth letters ("\x82\x60\x83\x7B\x83\x5E\x83\x93" = A-button) and is left as is. */
static const struct Repl s_ps_repl[] = {
    { "A and B Button", "Cross and Circle Button" },
    { "L and R Button", "L2 and R2 Button" },
    { "L,R Button", "L2,R2 Button" },
    { "A Button", "Cross Button" },
    { "B Button", "Circle Button" },
    { "X Button", "Square Button" },
    { "Y Button", "Triangle Button" },
    { "L Button", "L2 Button" },
    { "R Button", "R2 Button" },
    { "Z Button", "R1 Button" },
    { "C Button", "Right Stick" },
    { "C Stick", "Right Stick" },
    { "Control Stick", "Left Stick" },
    { "START/PAUSE", "OPTIONS" },
    { NULL, NULL },
};
static const struct Repl s_xbox_repl[] = {
    { "L and R Button", "LT and RT Button" },
    { "L,R Button", "LT,RT Button" },
    { "L Button", "LT Button" },
    { "R Button", "RT Button" },
    { "Z Button", "RB Button" },
    { "C Button", "Right Stick" },
    { "C Stick", "Right Stick" },
    { "Control Stick", "Left Stick" },
    { "START/PAUSE", "MENU" },
    { NULL, NULL },
};
static const struct Repl s_switch_repl[] = {
    { "L and R Button", "ZL and ZR Button" },
    { "L,R Button", "ZL,ZR Button" },
    { "L Button", "ZL Button" },
    { "R Button", "ZR Button" },
    { "Z Button", "R Button" },
    { "C Button", "Right Stick" },
    { "C Stick", "Right Stick" },
    { "Control Stick", "Left Stick" },
    { "START/PAUSE", "+" },
    { NULL, NULL },
};

/* PlayStation */
static const struct Repl s_ps_repl_de[] = {
    { "L- und R-Taste", "L2- und R2-Taste" },
    { "A-Knopf", "Kreuz-Knopf" }, { "B-Knopf", "Kreis-Knopf" },
    { "X-Knopf", "Quadrat-Knopf" }, { "Y-Knopf", "Dreieck-Knopf" },
    { "Z-Knopf", "R1-Knopf" }, { "R-Taste", "R2-Taste" }, { "L-Taste", "L2-Taste" },
    { "C-Stick", "R-Stick" }, { "Control Stick", "L-Stick" },
    { NULL, NULL },
};
static const struct Repl s_ps_repl_fr[] = {
    { "Boutons L ou R", "Boutons L2 ou R2" }, { "Boutons L, R", "Boutons L2, R2" },
    { "Bouton A", "Bouton Croix" }, { "Bouton B", "Bouton Rond" },
    { "Bouton X", "Bouton Carr\xE9" }, { "Bouton Y", "Bouton Triangle" },
    { "Bouton Z", "Bouton R1" }, { "Bouton L", "Bouton L2" }, { "Bouton R", "Bouton R2" },
    { "Bouton C", "Stick droit" }, { "Stick C", "Stick droit" },
    { "Stick directionnel", "Stick gauche" }, { "Stick direct.", "Stick gauche" },
    { NULL, NULL },
};
static const struct Repl s_ps_repl_es[] = {
    { "Bot\xF3n A", "Bot\xF3n Cruz" }, { "Bot\xF3n B", "Bot\xF3n C\xEDrculo" },
    { "Bot\xF3n X", "Bot\xF3n Cuadrado" }, { "Bot\xF3n Y", "Bot\xF3n Tri\xE1ngulo" },
    { "Bot\xF3n Z", "Bot\xF3n R1" }, { "Bot\xF3n L", "Bot\xF3n L2" }, { "Bot\xF3n R", "Bot\xF3n R2" },
    { "Bot\xF3n C", "Stick derecho" }, { "Stick C", "Stick derecho" },
    { "Stick de Control", "Stick izquierdo" },
    { NULL, NULL },
};
static const struct Repl s_ps_repl_it[] = {
    { "Pulsanti L e R", "Pulsanti L2 e R2" }, { "Pulsante L, R", "Pulsante L2, R2" },
    { "Pulsante A", "Pulsante Croce" }, { "Pulsante B", "Pulsante Cerchio" },
    { "Pulsante X", "Pulsante Quadrato" }, { "Pulsante Y", "Pulsante Triangolo" },
    { "Pulsante Z", "Pulsante R1" }, { "Pulsante L", "Pulsante L2" }, { "Pulsante R", "Pulsante R2" },
    { "Pulsante C", "Stick destro" }, { "Stick C", "Stick destro" },
    { "Control Stick", "Stick sinistro" },
    { NULL, NULL },
};
/* Xbox: same face letters */
static const struct Repl s_xbox_repl_de[] = {
    { "L- und R-Taste", "LT- und RT-Taste" },
    { "Z-Knopf", "RB-Knopf" }, { "R-Taste", "RT-Taste" }, { "L-Taste", "LT-Taste" },
    { "C-Stick", "R-Stick" }, { "Control Stick", "L-Stick" },
    { NULL, NULL },
};
static const struct Repl s_xbox_repl_fr[] = {
    { "Boutons L ou R", "Boutons LT ou RT" }, { "Boutons L, R", "Boutons LT, RT" },
    { "Bouton Z", "Bouton RB" }, { "Bouton L", "Bouton LT" }, { "Bouton R", "Bouton RT" },
    { "Bouton C", "Stick droit" }, { "Stick C", "Stick droit" },
    { "Stick directionnel", "Stick gauche" }, { "Stick direct.", "Stick gauche" },
    { NULL, NULL },
};
static const struct Repl s_xbox_repl_es[] = {
    { "Bot\xF3n Z", "Bot\xF3n RB" }, { "Bot\xF3n L", "Bot\xF3n LT" }, { "Bot\xF3n R", "Bot\xF3n RT" },
    { "Bot\xF3n C", "Stick derecho" }, { "Stick C", "Stick derecho" },
    { "Stick de Control", "Stick izquierdo" },
    { NULL, NULL },
};
static const struct Repl s_xbox_repl_it[] = {
    { "Pulsanti L e R", "Pulsanti LT e RT" }, { "Pulsante L, R", "Pulsante LT, RT" },
    { "Pulsante Z", "Pulsante RB" }, { "Pulsante L", "Pulsante LT" }, { "Pulsante R", "Pulsante RT" },
    { "Pulsante C", "Stick destro" }, { "Stick C", "Stick destro" },
    { "Control Stick", "Stick sinistro" },
    { NULL, NULL },
};
/* Switch */
static const struct Repl s_switch_repl_de[] = {
    { "L- und R-Taste", "ZL- und ZR-Taste" },
    { "Z-Knopf", "R-Knopf" }, { "R-Taste", "ZR-Taste" }, { "L-Taste", "ZL-Taste" },
    { "C-Stick", "R-Stick" }, { "Control Stick", "L-Stick" },
    { NULL, NULL },
};
static const struct Repl s_switch_repl_fr[] = {
    { "Boutons L ou R", "Boutons ZL ou ZR" }, { "Boutons L, R", "Boutons ZL, ZR" },
    { "Bouton Z", "Bouton R" }, { "Bouton L", "Bouton ZL" }, { "Bouton R", "Bouton ZR" },
    { "Bouton C", "Stick droit" }, { "Stick C", "Stick droit" },
    { "Stick directionnel", "Stick gauche" }, { "Stick direct.", "Stick gauche" },
    { NULL, NULL },
};
static const struct Repl s_switch_repl_es[] = {
    { "Bot\xF3n Z", "Bot\xF3n R" }, { "Bot\xF3n L", "Bot\xF3n ZL" }, { "Bot\xF3n R", "Bot\xF3n ZR" },
    { "Bot\xF3n C", "Stick derecho" }, { "Stick C", "Stick derecho" },
    { "Stick de Control", "Stick izquierdo" },
    { NULL, NULL },
};
static const struct Repl s_switch_repl_it[] = {
    { "Pulsanti L e R", "Pulsanti ZL e ZR" }, { "Pulsante L, R", "Pulsante ZL, ZR" },
    { "Pulsante Z", "Pulsante R" }, { "Pulsante L", "Pulsante ZL" }, { "Pulsante R", "Pulsante ZR" },
    { "Pulsante C", "Stick destro" }, { "Stick C", "Stick destro" },
    { "Control Stick", "Stick sinistro" },
    { NULL, NULL },
};

extern s32 locale;

/* match one rule of tbl at s (whole words); returns it or NULL */
static const struct Repl *match_rule(const struct Repl *tbl, const char *str, const char *s)
{
    const struct Repl *r;
    if (!tbl)
        return NULL;
    for (r = tbl; r->from; r++) {
        size_t fl = strlen(r->from);
        if (!strncmp(s, r->from, fl) && !(s > str && isalnum((unsigned char)s[-1]))
            && !isalnum((unsigned char)s[fl]))
            return r;
    }
    return NULL;
}

char *pc_button_prompts_text(char *str)
{
    static char bufs[4][1024];
    static int next;
    const struct Repl *tbl, *ltbl = NULL;
    char *out;
    const char *s;
    size_t len = 0;
    int changed = 0, style = pc_button_style();

    if (!str || style == PCB_STYLE_GC)
        return str;
    tbl = is_ps(style) ? s_ps_repl : style == PCB_STYLE_SWITCH ? s_switch_repl : s_xbox_repl;
    if (locale >= 1 && locale <= 4) {
        static const struct Repl *const loc[3][4] = {
            { s_ps_repl_de, s_ps_repl_fr, s_ps_repl_es, s_ps_repl_it },
            { s_xbox_repl_de, s_xbox_repl_fr, s_xbox_repl_es, s_xbox_repl_it },
            { s_switch_repl_de, s_switch_repl_fr, s_switch_repl_es, s_switch_repl_it },
        };
        ltbl = loc[is_ps(style) ? 0 : style == PCB_STYLE_SWITCH ? 2 : 1][locale - 1];
    } else if (!strstr(str, "Button") && !strstr(str, "Stick") && !strstr(str, "START/")) {
        /* quick reject: every English pattern has "Button", "Stick" or "START/" */
        return str;
    }
    out = bufs[next];
    for (s = str; *s; ) {
        const struct Repl *r = match_rule(ltbl, str, s);
        if (r) {   /* the game's own language's button words */
            size_t tl = strlen(r->to);
            if (len + tl >= sizeof bufs[0]) return str;
            memcpy(out + len, r->to, tl);
            len += tl;
            s += strlen(r->from);
            changed = 1;
            continue;
        }
        for (r = tbl; r->from; r++) {
            size_t fl = strlen(r->from);
            /* whole words only: "A Button", not "...A Button" inside a word */
            if (!strncmp(s, r->from, fl) && !(s > str && isalnum((unsigned char)s[-1]))) {
                const char *to = r->to;
                size_t tl;
                /* PS3 / Xbox 360 label the start button START */
                if (s_alt_start && !strcmp(r->from, "START/PAUSE")) to = "START";
                tl = strlen(to);
                if (len + tl >= sizeof bufs[0]) return str;
                memcpy(out + len, to, tl);
                len += tl;
                s += fl;
                changed = 1;
                break;
            }
        }
        if (!r->from) {
            if (len + 1 >= sizeof bufs[0]) return str;
            out[len++] = *s++;
        }
    }
    if (!changed)
        return str;
    out[len] = 0;
    next = (next + 1) & 3;
    return out;
}

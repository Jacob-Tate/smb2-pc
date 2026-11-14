/* Latin-1 glyphs composed from the menu font's own letters: see
 * pc_font_compose.h.  The font's pages are 24x24 IA4 (8x4 texel tiles,
 * byte = alpha << 4 | intensity, intensity always 15).  Lowercase letters
 * sit in rows 7-21 with room above for an accent; capitals fill rows 1-21,
 * so an accented capital is drawn 16 rows tall (rows 6-21) under its accent,
 * the way small-cap accents are done in tight bitmap fonts.  The accent
 * shapes are the font's own ` ^ ~ . , glyphs scaled down. */
#include "pc_font_compose.h"
#include <string.h>
#include <math.h>

#define N 24
typedef float Img[N][N];

#define UC(ch) ((unsigned short)(0x8260 + ((ch) - 'A')))
#define LC(ch) ((unsigned short)(0x8281 + ((ch) - 'a')))
#define F_GRAVE 0x814D
#define F_CIRC 0x814F
#define F_TILDE 0x8160
#define F_DOT 0x8144
#define F_COMMA 0x8143
#define F_EXCL 0x8149
#define F_QUEST 0x8148

int pcfc_recipe(unsigned char b, PcfcRecipe *r)
{
    static const unsigned char vowel_marks[6] = {
        PCFC_GRAVE, PCFC_ACUTE, PCFC_CIRC, PCFC_TILDE, PCFC_DIAER, PCFC_RING };
    static const unsigned char gacd[4] = { PCFC_GRAVE, PCFC_ACUTE, PCFC_CIRC, PCFC_DIAER };
    static const unsigned char o_marks[5] = { PCFC_GRAVE, PCFC_ACUTE, PCFC_CIRC, PCFC_TILDE, PCFC_DIAER };
    int lower = b >= 0xE0;
    unsigned char k = (unsigned char)(b & ~0x20);  /* uppercase form for 0xC0-0xFE */
#define L(ch) (lower ? LC((ch) + 32) : UC(ch))

    memset(r, 0, sizeof *r);
    switch (b) {   /* cp1252 punctuation the font already has */
    case 0x85: r->base = 0x8163; r->existing = 1; return 1; /* ... */
    case 0x91: r->base = 0x8165; r->existing = 1; return 1;
    case 0x92: r->base = 0x8166; r->existing = 1; return 1;
    case 0x93: r->base = 0x8167; r->existing = 1; return 1;
    case 0x94: r->base = 0x8168; r->existing = 1; return 1;
    case 0x96: r->base = 0x817C; r->existing = 1; return 1; /* en dash */
    case 0x97: r->base = 0x815C; r->existing = 1; return 1; /* em dash */
    case 0xAB: r->base = 0x8183; r->existing = 1; return 1; /* << */
    case 0xBB: r->base = 0x8184; r->existing = 1; return 1; /* >> */
    case 0xD7: r->base = 0x817E; r->existing = 1; return 1; /* x */
    case 0xF7: r->base = 0x8180; r->existing = 1; return 1; /* / */
    case 0xA1: r->base = F_EXCL; r->mark = PCFC_FLIPV; return 1;
    case 0xBF: r->base = F_QUEST; r->mark = PCFC_ROT180; return 1;
    case 0xB0: r->mark = PCFC_DEGREE; return 1;
    case 0xBA: r->mark = PCFC_DEGREE; return 1;   /* o ordinal: as degree */
    case 0xB4: r->mark = PCFC_ACUTE; return 1;
    case 0xA8: r->mark = PCFC_DIAER; return 1;
    case 0xDF: r->base = UC('B'); r->mark = PCFC_SZLIG; return 1;
    case 0xD0: r->base = UC('D'); r->existing = 1; return 1;
    }
    if (b < 0xC0 || b == 0xD7 || b == 0xF7)
        return 0;
    if (k >= 0xC0 && k <= 0xC5) { r->base = L('A'); r->mark = vowel_marks[k - 0xC0]; return 1; }
    if (k == 0xC7) { r->base = L('C'); r->mark = PCFC_CEDILLA; return 1; }
    if (k >= 0xC8 && k <= 0xCB) { r->base = L('E'); r->mark = gacd[k - 0xC8]; return 1; }
    if (k >= 0xCC && k <= 0xCF) { r->base = L('I'); r->mark = gacd[k - 0xCC]; return 1; }
    if (k == 0xD1) { r->base = L('N'); r->mark = PCFC_TILDE; return 1; }
    if (k >= 0xD2 && k <= 0xD6) { r->base = L('O'); r->mark = o_marks[k - 0xD2]; return 1; }
    if (k == 0xD8) { r->base = L('O'); r->mark = PCFC_STROKE; return 1; }
    if (k >= 0xD9 && k <= 0xDC) { r->base = L('U'); r->mark = gacd[k - 0xD9]; return 1; }
    if (k == 0xDD) { r->base = L('Y'); r->mark = PCFC_ACUTE; return 1; }
    if (b == 0xFF) { r->base = LC('y'); r->mark = PCFC_DIAER; return 1; }
#undef L
    return 0;   /* AE, thorn, eth: no glyph */
}

/* ---- image helpers ---- */
static void decode(const unsigned char *p, Img a)
{
    int tx, ty, x, y, i = 0;
    for (ty = 0; ty < N / 4; ty++)
        for (tx = 0; tx < N / 8; tx++)
            for (y = 0; y < 4; y++)
                for (x = 0; x < 8; x++)
                    a[ty * 4 + y][tx * 8 + x] = (p[i++] >> 4) / 15.0f;
}

static void encode(Img a, unsigned char *p)
{
    int tx, ty, x, y, i = 0;
    for (ty = 0; ty < N / 4; ty++)
        for (tx = 0; tx < N / 8; tx++)
            for (y = 0; y < 4; y++)
                for (x = 0; x < 8; x++) {
                    float v = a[ty * 4 + y][tx * 8 + x];
                    int q = (int)(v * 15.0f + 0.5f);
                    q = q < 0 ? 0 : q > 15 ? 15 : q;
                    p[i++] = (unsigned char)((q << 4) | 0xF);
                }
}

typedef struct { float x0, y0, x1, y1; } Box; /* edges: [x0,x1) x [y0,y1) */

static int bbox(Img a, Box *b)
{
    int x, y, x0 = N, y0 = N, x1 = -1, y1 = -1;
    for (y = 0; y < N; y++)
        for (x = 0; x < N; x++)
            if (a[y][x] > 0.02f) {
                if (x < x0) x0 = x;
                if (x > x1) x1 = x;
                if (y < y0) y0 = y;
                if (y > y1) y1 = y;
            }
    if (x1 < 0) return 0;
    b->x0 = (float)x0; b->y0 = (float)y0; b->x1 = (float)(x1 + 1); b->y1 = (float)(y1 + 1);
    return 1;
}

static float sample(Img a, float x, float y)   /* bilinear, texel centres at +0.5 */
{
    int ix, iy;
    float fx, fy, v00, v10, v01, v11;
    x -= 0.5f; y -= 0.5f;
    ix = (int)floorf(x); iy = (int)floorf(y);
    fx = x - ix; fy = y - iy;
#define AT(X, Y) (((X) < 0 || (Y) < 0 || (X) >= N || (Y) >= N) ? 0.0f : a[Y][X])
    v00 = AT(ix, iy); v10 = AT(ix + 1, iy); v01 = AT(ix, iy + 1); v11 = AT(ix + 1, iy + 1);
#undef AT
    return (v00 * (1 - fx) + v10 * fx) * (1 - fy) + (v01 * (1 - fx) + v11 * fx) * fy;
}

/* draw src box s into dst box d (max blend), 4x4 supersampled */
static void blit(Img src, Box s, Img dst, Box d, int flipx, int flipy)
{
    int x, y, i, j;
    for (y = (int)floorf(d.y0); y < (int)ceilf(d.y1); y++) {
        for (x = (int)floorf(d.x0); x < (int)ceilf(d.x1); x++) {
            float acc = 0.0f;
            if (x < 0 || y < 0 || x >= N || y >= N) continue;
            for (j = 0; j < 4; j++)
                for (i = 0; i < 4; i++) {
                    float px = x + (i + 0.5f) / 4.0f, py = y + (j + 0.5f) / 4.0f;
                    float u, v;
                    if (px < d.x0 || px >= d.x1 || py < d.y0 || py >= d.y1) continue;
                    u = (px - d.x0) / (d.x1 - d.x0);
                    v = (py - d.y0) / (d.y1 - d.y0);
                    if (flipx) u = 1.0f - u;
                    if (flipy) v = 1.0f - v;
                    acc += sample(src, s.x0 + u * (s.x1 - s.x0), s.y0 + v * (s.y1 - s.y0));
                }
            acc /= 16.0f;
            if (acc > dst[y][x]) dst[y][x] = acc > 1.0f ? 1.0f : acc;
        }
    }
}

static void ring(Img dst, float cx, float cy, float ro, float ri)
{
    int x, y, i, j;
    for (y = 0; y < N; y++)
        for (x = 0; x < N; x++) {
            float acc = 0.0f;
            for (j = 0; j < 4; j++)
                for (i = 0; i < 4; i++) {
                    float dx = x + (i + 0.5f) / 4.0f - cx, dy = y + (j + 0.5f) / 4.0f - cy;
                    float r2 = dx * dx + dy * dy;
                    if (r2 <= ro * ro && r2 >= ri * ri) acc += 1.0f;
                }
            acc /= 16.0f;
            if (acc > dst[y][x]) dst[y][x] = acc;
        }
}

static void line(Img dst, float x0, float y0, float x1, float y1, float w)
{
    int x, y, i, j;
    float dx = x1 - x0, dy = y1 - y0, len2 = dx * dx + dy * dy;
    for (y = 0; y < N; y++)
        for (x = 0; x < N; x++) {
            float acc = 0.0f;
            for (j = 0; j < 4; j++)
                for (i = 0; i < 4; i++) {
                    float px = x + (i + 0.5f) / 4.0f - x0, py = y + (j + 0.5f) / 4.0f - y0;
                    float t = (px * dx + py * dy) / len2, ex, ey;
                    t = t < 0 ? 0 : t > 1 ? 1 : t;
                    ex = px - t * dx; ey = py - t * dy;
                    if (ex * ex + ey * ey <= w * w * 0.25f) acc += 1.0f;
                }
            acc /= 16.0f;
            if (acc > dst[y][x]) dst[y][x] = acc;
        }
}

static int load(PcfcGetPage page, void *user, unsigned short code, Img a, Box *b)
{
    const unsigned char *p = page(code, user);
    if (!p) return 0;
    decode(p, a);
    return bbox(a, b);
}

/* accent `mark` centred on cx in rows [y0, y1) */
static int accent(int mark, PcfcGetPage page, void *user, Img dst, float cx, float y0, float y1)
{
    Img g;
    Box b, d;
    float h = y1 - y0;
    switch (mark) {
    case PCFC_GRAVE:
    case PCFC_ACUTE:
        if (!load(page, user, F_GRAVE, g, &b)) return 0;
        d.x0 = cx - 3.0f; d.x1 = cx + 3.0f; d.y0 = y0; d.y1 = y1;
        blit(g, b, dst, d, mark == PCFC_ACUTE, 0);
        return 1;
    case PCFC_CIRC:
        if (!load(page, user, F_CIRC, g, &b)) return 0;
        d.x0 = cx - 5.0f; d.x1 = cx + 5.0f; d.y0 = y0; d.y1 = y1;
        blit(g, b, dst, d, 0, 0);
        return 1;
    case PCFC_TILDE:
        if (!load(page, user, F_TILDE, g, &b)) return 0;
        d.x0 = cx - 6.0f; d.x1 = cx + 6.0f; d.y0 = y0 + 0.5f; d.y1 = y1 - 0.5f;
        blit(g, b, dst, d, 0, 0);
        return 1;
    case PCFC_DIAER:
        if (!load(page, user, F_DOT, g, &b)) return 0;
        d.y0 = y1 - 4.0f; d.y1 = y1;
        d.x0 = cx - 6.0f; d.x1 = cx - 2.0f;
        blit(g, b, dst, d, 0, 0);
        d.x0 = cx + 2.0f; d.x1 = cx + 6.0f;
        blit(g, b, dst, d, 0, 0);
        return 1;
    case PCFC_RING:
        ring(dst, cx, y0 + h * 0.5f, h * 0.5f + 0.4f, h * 0.5f - 1.4f);
        return 1;
    }
    return 0;
}

int pcfc_compose(const PcfcRecipe *r, PcfcGetPage page, void *user,
                 unsigned char out[PCFC_PAGE_SIZE])
{
    Img base, dst, g;
    Box bb, d, gb;
    float cx;
    int caps;

    memset(dst, 0, sizeof dst);
    if (r->existing)
        return 0;
    if (r->base == 0) {   /* lone mark */
        if (r->mark == PCFC_DEGREE)
            ring(dst, 12.0f, 5.0f, 4.2f, 2.0f);
        else if (!accent(r->mark, page, user, dst, 12.0f, 2.0f, 8.0f))
            return 0;
        encode(dst, out);
        return 1;
    }
    if (!load(page, user, r->base, base, &bb))
        return 0;
    caps = r->base >= UC('A') && r->base <= UC('Z');
    cx = 0.5f * (bb.x0 + bb.x1);

    switch (r->mark) {
    case PCFC_FLIPV:   /* inverted ! : flipped, hanging 2 rows lower */
        d = bb; d.y0 += 2.0f; d.y1 += 2.0f;
        if (d.y1 > N) { d.y0 -= d.y1 - N; d.y1 = N; }
        blit(base, bb, dst, d, 0, 1);
        break;
    case PCFC_ROT180:  /* inverted ? */
        d = bb; d.y0 += 2.0f; d.y1 += 2.0f;
        if (d.y1 > N) { d.y0 -= d.y1 - N; d.y1 = N; }
        blit(base, bb, dst, d, 1, 1);
        break;
    case PCFC_SZLIG: { /* sharp s: a narrow B whose stem runs down 2 rows */
        int x, y, x0;
        d = bb; d.x1 -= 2.0f; d.y0 += 1.0f;
        blit(base, bb, dst, d, 0, 0);
        x0 = (int)d.x0;
        for (y = (int)d.y1; y < N && y < (int)d.y1 + 3; y++)
            for (x = x0; x < x0 + 5 && x < N; x++)
                dst[y][x] = dst[(int)d.y1 - 1][x];
        break;
    }
    case PCFC_STROKE:
        memcpy(dst, base, sizeof dst);
        line(dst, bb.x0 + 0.5f, bb.y1 + 0.5f, bb.x1 - 0.5f, bb.y0 - 0.5f, 2.2f);
        break;
    case PCFC_CEDILLA:
        d = bb;
        if (caps) { d.y1 = 19.0f; }
        else { d.y0 -= 2.0f; d.y1 -= 2.0f; }
        blit(base, bb, dst, d, 0, 0);
        if (!load(page, user, F_COMMA, g, &gb)) return 0;
        d.x0 = cx - 2.5f; d.x1 = cx + 2.5f; d.y0 = 19.0f; d.y1 = 24.0f;
        blit(g, gb, dst, d, 0, 0);
        break;
    default: {         /* accents above */
        int x, y;
        if (r->base == LC('i'))   /* dotless i under the accent */
            for (y = 0; y < 7; y++)
                for (x = 0; x < N; x++)
                    base[y][x] = 0.0f;
        if (caps) {
            d = bb; d.y0 = 6.0f;
            blit(base, bb, dst, d, 0, 0);
            if (!accent(r->mark, page, user, dst, cx, 0.0f, 5.0f)) return 0;
        } else {
            memcpy(dst, base, sizeof dst);
            if (!accent(r->mark, page, user, dst, cx, 0.5f, 6.0f)) return 0;
        }
        break;
    }
    }
    encode(dst, out);
    return 1;
}

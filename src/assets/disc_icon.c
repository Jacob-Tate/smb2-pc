/* Game icon from the user's disc: decode the memory-card save icon and
 * upscale it with Scale2x (EPX).  See include/disc_icon.h. */
#include "disc_icon.h"
#include "gamedata.h"
#include "iso.h"
#include <stdlib.h>
#include <string.h>

/* ---- decode: GX RGB5A3, 4x4 tiles, big-endian u16 ---- */
void disc_icon_decode(const uint8_t* src, uint8_t out[32 * 32 * 4])
{
    int tx, ty, x, y;
    for (ty = 0; ty < 8; ty++)
        for (tx = 0; tx < 8; tx++)
            for (y = 0; y < 4; y++)
                for (x = 0; x < 4; x++) {
                    unsigned v = ((unsigned)src[0] << 8) | src[1];
                    uint8_t* p = out + (((ty * 4 + y) * 32) + tx * 4 + x) * 4;
                    src += 2;
                    if (v & 0x8000) {               /* RGB555, opaque */
                        unsigned r = (v >> 10) & 31, g = (v >> 5) & 31, b = v & 31;
                        p[0] = (uint8_t)((r << 3) | (r >> 2));
                        p[1] = (uint8_t)((g << 3) | (g >> 2));
                        p[2] = (uint8_t)((b << 3) | (b >> 2));
                        p[3] = 255;
                    } else {                        /* A3 RGB444 */
                        unsigned a = (v >> 12) & 7;
                        p[0] = (uint8_t)(((v >> 8) & 15) * 17);
                        p[1] = (uint8_t)(((v >> 4) & 15) * 17);
                        p[2] = (uint8_t)((v & 15) * 17);
                        p[3] = (uint8_t)((a << 5) | (a << 2) | (a >> 1));
                    }
                }
}

bool disc_icon_from_archive(struct ISOArchive* arc, uint8_t out[32 * 32 * 4])
{
    uint8_t raw[DISC_ICON_BYTES];
    int e = arc ? iso_path_to_index(arc, "/memcard/sys.bin") : -1;
    if (e < 0 || iso_entry_is_dir(arc, e) ||
        iso_entry_size(arc, e) < DISC_ICON_OFFSET + DISC_ICON_BYTES ||
        iso_file_read_at(arc, e, DISC_ICON_OFFSET, DISC_ICON_BYTES, raw) != 0)
        return false;
    disc_icon_decode(raw, out);
    return true;
}

/* ---- Scale2x (EPX) on whole RGBA pixels, edges clamped ---- */
static void scale2x(const uint32_t* s, int n, uint32_t* d)
{
    int x, y;
    for (y = 0; y < n; y++)
        for (x = 0; x < n; x++) {
            uint32_t P = s[y * n + x];
            uint32_t A = s[(y > 0 ? y - 1 : y) * n + x];          /* up */
            uint32_t B = s[y * n + (x < n - 1 ? x + 1 : x)];      /* right */
            uint32_t C = s[y * n + (x > 0 ? x - 1 : x)];          /* left */
            uint32_t D = s[(y < n - 1 ? y + 1 : y) * n + x];      /* down */
            uint32_t e0 = P, e1 = P, e2 = P, e3 = P;
            if (A != D && C != B) {
                if (C == A) e0 = C;
                if (A == B) e1 = B;
                if (D == C) e2 = C;
                if (B == D) e3 = B;
            }
            d[(2 * y) * 2 * n + 2 * x] = e0;
            d[(2 * y) * 2 * n + 2 * x + 1] = e1;
            d[(2 * y + 1) * 2 * n + 2 * x] = e2;
            d[(2 * y + 1) * 2 * n + 2 * x + 1] = e3;
        }
}

/* Area (box-coverage) resample n x n -> m x m (m <= n), premultiplied alpha. */
static void area_down(const uint8_t* s, int n, uint8_t* d, int m)
{
    int ox, oy, x, y;
    double scale = (double)n / m;
    for (oy = 0; oy < m; oy++)
        for (ox = 0; ox < m; ox++) {
            double y0 = oy * scale, y1 = (oy + 1) * scale;
            double x0 = ox * scale, x1 = (ox + 1) * scale;
            double acc[4] = { 0, 0, 0, 0 }, wsum = 0;
            for (y = (int)y0; y < n && y < y1; y++) {
                double wy = (y + 1 < y1 ? y + 1 : y1) - (y > y0 ? y : y0);
                for (x = (int)x0; x < n && x < x1; x++) {
                    double wx = (x + 1 < x1 ? x + 1 : x1) - (x > x0 ? x : x0);
                    double w = wx * wy;
                    const uint8_t* p = s + (y * n + x) * 4;
                    double a = p[3] / 255.0;
                    acc[0] += p[0] * a * w;
                    acc[1] += p[1] * a * w;
                    acc[2] += p[2] * a * w;
                    acc[3] += a * w;
                    wsum += w;
                }
            }
            {
                uint8_t* q = d + (oy * m + ox) * 4;
                double a = wsum > 0 ? acc[3] / wsum : 0;
                int i;
                for (i = 0; i < 3; i++) {
                    double c = acc[3] > 0 ? acc[i] / acc[3] : 0;
                    q[i] = (uint8_t)(c + 0.5 > 255 ? 255 : c + 0.5);
                }
                q[3] = (uint8_t)(a * 255.0 + 0.5);
            }
        }
}

void disc_icon_render(const uint8_t src32[32 * 32 * 4], int size, uint8_t* out)
{
    uint32_t *a, *b, *t;
    int n = 32;

    if (size == 32) { memcpy(out, src32, 32 * 32 * 4); return; }
    if (size == 16) { area_down(src32, 32, out, 16); return; }   /* = 2x2 box */
    if (size < 32) { area_down(src32, 32, out, size); return; }
    /* Scale2x up to the first level >= size, then area-downsample to size */
    a = (uint32_t*)malloc(1024 * 1024 * 4);
    b = (uint32_t*)malloc(1024 * 1024 * 4);
    if (!a || !b) { free(a); free(b); memset(out, 0, (size_t)size * size * 4); return; }
    memcpy(a, src32, 32 * 32 * 4);
    while (n < size && n < 1024) {
        scale2x(a, n, b);
        t = a; a = b; b = t;
        n *= 2;
    }
    if (n == size) memcpy(out, a, (size_t)n * n * 4);
    else if (n > size) area_down((const uint8_t*)a, n, out, size);
    else memset(out, 0, (size_t)size * size * 4);
    free(a);
    free(b);
}

/* ---- PNG (RGBA8, stored deflate) ---- */
static void put32be(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);  p[3] = (uint8_t)v;
}

uint8_t* disc_icon_png(const uint8_t* rgba, int w, int h, size_t* out_size)
{
    size_t raw = (size_t)h * (1 + (size_t)w * 4);
    size_t nblk = (raw + 65534) / 65535;
    size_t zlen = 2 + raw + nblk * 5 + 4;
    size_t total = 8 + 25 + (12 + zlen) + 12;
    uint8_t *buf = (uint8_t*)malloc(total), *p, *z;
    uint32_t s1 = 1, s2 = 0;
    size_t i, left, row = 0, col = 0;
    static const uint8_t sig[8] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };

    if (!buf) return NULL;
    p = buf;
    memcpy(p, sig, 8); p += 8;
    put32be(p, 13); memcpy(p + 4, "IHDR", 4);
    put32be(p + 8, (uint32_t)w); put32be(p + 12, (uint32_t)h);
    p[16] = 8; p[17] = 6; p[18] = 0; p[19] = 0; p[20] = 0;
    put32be(p + 21, gd_crc32(0, p + 4, 17));
    p += 25;
    put32be(p, (uint32_t)zlen); memcpy(p + 4, "IDAT", 4);
    z = p + 8;
    *z++ = 0x78; *z++ = 0x01;
    left = raw;
    while (left > 0) {
        size_t n = left > 65535 ? 65535 : left;
        left -= n;
        *z++ = left == 0 ? 1 : 0;
        *z++ = (uint8_t)n; *z++ = (uint8_t)(n >> 8);
        *z++ = (uint8_t)~n; *z++ = (uint8_t)((~n) >> 8);
        for (i = 0; i < n; i++) {
            uint8_t c;
            if (col == 0) c = 0;                               /* filter: none */
            else c = rgba[(row * (size_t)w * 4) + col - 1];
            if (++col == 1 + (size_t)w * 4) { col = 0; row++; }
            *z++ = c;
            s1 = (s1 + c) % 65521;
            s2 = (s2 + s1) % 65521;
        }
    }
    put32be(z, (s2 << 16) | s1); z += 4;
    put32be(z, gd_crc32(0, p + 4, 4 + zlen));
    p = z + 4;
    put32be(p, 0); memcpy(p + 4, "IEND", 4); put32be(p + 8, gd_crc32(0, p + 4, 4));
    *out_size = total;
    return buf;
}

/* ---- ICO ---- */
static void put32le(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

uint8_t* disc_icon_ico(const uint8_t src32[32 * 32 * 4], size_t* out_size)
{
    static const int sizes[6] = { 16, 32, 48, 64, 128, 256 };
    uint8_t* img[6] = { 0 };
    size_t len[6] = { 0 }, total = 6 + 16 * 6, off;
    uint8_t *ico = NULL, *rgba = (uint8_t*)malloc(256 * 256 * 4);
    int k;

    if (!rgba) return NULL;
    for (k = 0; k < 6; k++) {
        int s = sizes[k], x, y;
        disc_icon_render(src32, s, rgba);
        if (s == 256) {
            img[k] = disc_icon_png(rgba, s, s, &len[k]);
        } else {
            size_t mask_row = (size_t)((s + 31) / 32) * 4;
            len[k] = 40 + (size_t)s * s * 4 + mask_row * s;
            img[k] = (uint8_t*)calloc(1, len[k]);
            if (img[k]) {
                uint8_t* h = img[k];
                put32le(h, 40); put32le(h + 4, (uint32_t)s); put32le(h + 8, (uint32_t)s * 2);
                h[12] = 1; h[14] = 32;
                put32le(h + 20, (uint32_t)(len[k] - 40));
                for (y = 0; y < s; y++)                    /* bottom-up BGRA */
                    for (x = 0; x < s; x++) {
                        const uint8_t* p = rgba + ((size_t)(s - 1 - y) * s + x) * 4;
                        uint8_t* q = h + 40 + ((size_t)y * s + x) * 4;
                        q[0] = p[2]; q[1] = p[1]; q[2] = p[0]; q[3] = p[3];
                        if (p[3] == 0)                     /* AND mask: transparent */
                            h[40 + (size_t)s * s * 4 + (size_t)y * mask_row + x / 8] |=
                                (uint8_t)(0x80 >> (x & 7));
                    }
            }
        }
        if (!img[k]) goto out;
        total += len[k];
    }
    ico = (uint8_t*)malloc(total);
    if (!ico) goto out;
    ico[0] = 0; ico[1] = 0; ico[2] = 1; ico[3] = 0; ico[4] = 6; ico[5] = 0;
    off = 6 + 16 * 6;
    for (k = 0; k < 6; k++) {
        uint8_t* e = ico + 6 + 16 * k;
        e[0] = (uint8_t)(sizes[k] == 256 ? 0 : sizes[k]);
        e[1] = e[0]; e[2] = 0; e[3] = 0;
        e[4] = 1; e[5] = 0; e[6] = 32; e[7] = 0;
        put32le(e + 8, (uint32_t)len[k]);
        put32le(e + 12, (uint32_t)off);
        memcpy(ico + off, img[k], len[k]);
        off += len[k];
    }
    *out_size = total;
out:
    for (k = 0; k < 6; k++) free(img[k]);
    free(rgba);
    return ico;
}

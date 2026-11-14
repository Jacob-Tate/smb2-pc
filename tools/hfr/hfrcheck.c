/* hfrcheck: look for popping in HFR in-between frame dumps (docs/hfr.md).
 *
 *   hfrcheck <dir> <first_tick> <last_tick> <k> [outdir]
 *
 * For every tick T in [first+1, last] the sequence frame_{T-1}.ppm,
 * frame_T_1.ppm .. frame_T_{k-1}.ppm, frame_T.ppm (k steps) is compared step
 * by step in 32x32 blocks.  Smooth motion spreads a block's change over the
 * k steps; a pop puts it into one step.  A block "pops" when one step holds
 * more than 70% of its total change and that step's mean abs difference is
 * > 6 (out of 255, RGB averaged).  Per tick: the mean abs difference of each
 * step and the number / bounding box of popping blocks.  With outdir, a PNG
 * strip (the sequence, cropped to the pop box and enlarged) is written for
 * every tick that has pops, for visual review (a digit or a sprite frame
 * changing at the midpoint is a legitimate discrete change).
 *
 * Build: gcc -O2 -o hfrcheck hfrcheck.c -lz
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

typedef struct { int w, h; unsigned char* p; } Img;

static int rd(const char* path, Img* im) {
    FILE* f = fopen(path, "rb");
    char magic[3] = {0};
    int maxv;
    if (!f) return 0;
    if (fscanf(f, "%2s %d %d %d", magic, &im->w, &im->h, &maxv) != 4 || strcmp(magic, "P6")) {
        fclose(f); return 0;
    }
    fgetc(f);
    im->p = malloc((size_t)im->w * im->h * 3);
    if (fread(im->p, 3, (size_t)im->w * im->h, f) != (size_t)im->w * im->h) { fclose(f); return 0; }
    fclose(f);
    return 1;
}

static void be32(unsigned char* b, unsigned v) { b[0] = v >> 24; b[1] = v >> 16; b[2] = v >> 8; b[3] = v; }
static void chunk(FILE* f, const char* t, const unsigned char* d, unsigned n) {
    unsigned char b[4];
    unsigned long c = crc32(0, (const unsigned char*)t, 4);
    be32(b, n); fwrite(b, 1, 4, f); fwrite(t, 1, 4, f);
    if (n) { fwrite(d, 1, n, f); c = crc32(c, d, n); }
    be32(b, (unsigned)c); fwrite(b, 1, 4, f);
}
static void wr_png(const char* path, int w, int h, const unsigned char* rgb) {
    FILE* f = fopen(path, "wb");
    unsigned char hdr[13];
    unsigned char* raw;
    uLongf zn;
    unsigned char* z;
    int y;
    if (!f) return;
    raw = malloc((size_t)(w * 3 + 1) * h);
    for (y = 0; y < h; y++) { raw[(size_t)y * (w * 3 + 1)] = 0; memcpy(raw + (size_t)y * (w * 3 + 1) + 1, rgb + (size_t)y * w * 3, (size_t)w * 3); }
    zn = compressBound((uLong)(w * 3 + 1) * h);
    z = malloc(zn);
    compress2(z, &zn, raw, (uLong)(w * 3 + 1) * h, 6);
    fwrite("\x89PNG\r\n\x1a\n", 1, 8, f);
    be32(hdr, (unsigned)w); be32(hdr + 4, (unsigned)h);
    hdr[8] = 8; hdr[9] = 2; hdr[10] = hdr[11] = hdr[12] = 0;
    chunk(f, "IHDR", hdr, 13);
    chunk(f, "IDAT", z, (unsigned)zn);
    chunk(f, "IEND", NULL, 0);
    fclose(f);
    free(raw); free(z);
}

#define BS 32
int main(int argc, char** argv) {
    const char* dir;
    int t0, t1, k, T, j, i;
    const char* out = NULL;
    long total_pops = 0, ticks = 0;
    if (argc < 5) { fprintf(stderr, "usage: hfrcheck dir first last k [outdir]\n"); return 1; }
    dir = argv[1]; t0 = atoi(argv[2]); t1 = atoi(argv[3]); k = atoi(argv[4]);
    if (argc > 5) out = argv[5];
    for (T = t0 + 1; T <= t1; T++) {
        Img seq[64];
        char path[1024];
        int n = 0, ok = 1, bw, bh, w, h, pops = 0, bx0 = 1 << 30, by0 = 1 << 30, bx1 = -1, by1 = -1;
        double stepmean[64];
        snprintf(path, sizeof path, "%s/frame_%05d.ppm", dir, T - 1);
        if (!rd(path, &seq[n++])) continue;
        for (j = 1; j < k && ok; j++) {
            snprintf(path, sizeof path, "%s/frame_%05d_%d.ppm", dir, T, j);
            ok = rd(path, &seq[n++]);
        }
        snprintf(path, sizeof path, "%s/frame_%05d.ppm", dir, T);
        if (!ok || !rd(path, &seq[n++])) { printf("tick %d: missing frames\n", T); continue; }
        w = seq[0].w; h = seq[0].h;
        bw = (w + BS - 1) / BS; bh = (h + BS - 1) / BS;
        for (i = 0; i < n - 1; i++) {
            const unsigned char *a = seq[i].p, *b = seq[i + 1].p;
            double s = 0;
            long q;
            for (q = 0; q < (long)w * h * 3; q++) s += abs(a[q] - b[q]);
            stepmean[i] = s / ((double)w * h * 3);
        }
        for (int by = 0; by < bh; by++)
        for (int bx = 0; bx < bw; bx++) {
            double sd[64], tot = 0, mx = 0;
            int cnt = 0;
            for (i = 0; i < n - 1; i++) {
                const unsigned char *a = seq[i].p, *b = seq[i + 1].p;
                double s = 0;
                cnt = 0;
                for (int y = by * BS; y < (by + 1) * BS && y < h; y++)
                for (int x = bx * BS; x < (bx + 1) * BS && x < w; x++) {
                    size_t o = ((size_t)y * w + x) * 3;
                    s += abs(a[o] - b[o]) + abs(a[o + 1] - b[o + 1]) + abs(a[o + 2] - b[o + 2]);
                    cnt += 3;
                }
                sd[i] = s / cnt;
                tot += sd[i];
                if (sd[i] > mx) mx = sd[i];
            }
            if (mx > 6.0 && mx > 0.7 * tot) {
                pops++;
                if (bx < bx0) bx0 = bx;
                if (by < by0) by0 = by;
                if (bx > bx1) bx1 = bx;
                if (by > by1) by1 = by;
            }
        }
        ticks++;
        total_pops += pops;
        printf("tick %d steps", T);
        for (i = 0; i < n - 1; i++) printf(" %.2f", stepmean[i]);
        if (pops) printf(" | pops %d box %d,%d-%d,%d", pops, bx0 * BS, by0 * BS, (bx1 + 1) * BS, (by1 + 1) * BS);
        printf("\n");
        if (pops && out) {
            /* crop to the box (min 192 px), enlarge to ~256 px high, strip */
            int cx0 = bx0 * BS, cy0 = by0 * BS, cx1 = (bx1 + 1) * BS, cy1 = (by1 + 1) * BS, cw, ch, sc, W, H;
            unsigned char* strip;
            if (cx1 > w) cx1 = w;
            if (cy1 > h) cy1 = h;
            while (cx1 - cx0 < 192) { if (cx0 > 0) cx0--; if (cx1 < w) cx1++; if (cx0 == 0 && cx1 == w) break; }
            while (cy1 - cy0 < 192) { if (cy0 > 0) cy0--; if (cy1 < h) cy1++; if (cy0 == 0 && cy1 == h) break; }
            cw = cx1 - cx0; ch = cy1 - cy0;
            sc = 384 / ch; if (sc < 1) sc = 1; if (sc > 4) sc = 4;
            if (cw * sc * n > 6000) sc = 1;
            W = (cw * sc + 4) * n; H = ch * sc;
            strip = calloc((size_t)W * H, 3);
            for (i = 0; i < n; i++)
                for (int y = 0; y < H; y++)
                for (int x = 0; x < cw * sc; x++) {
                    size_t so = ((size_t)(cy0 + y / sc) * w + cx0 + x / sc) * 3;
                    size_t d = ((size_t)y * W + i * (cw * sc + 4) + x) * 3;
                    memcpy(strip + d, seq[i].p + so, 3);
                }
            snprintf(path, sizeof path, "%s/pop_%05d.png", out, T);
            wr_png(path, W, H, strip);
            free(strip);
        }
        for (i = 0; i < n; i++) free(seq[i].p);
    }
    printf("summary: %ld ticks, %ld popping blocks\n", ticks, total_pops);
    return 0;
}

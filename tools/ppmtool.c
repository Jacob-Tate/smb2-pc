/* ppmtool: frame-dump comparison helper for renderer parity testing.
 *
 *   ppmtool diff a.ppm b.ppm [heat.ppm]
 *       prints: identical | maxdiff, mean abs diff, % pixels with any diff,
 *       % pixels with sum-abs-diff > 24, and the bounding box of the > 24
 *       pixels; optional heat map (diff * 8, grey = equal background)
 *   ppmtool sheet out.png scale a.ppm b.ppm [c.ppm ...]
 *       side-by-side PNG of the inputs, each downscaled by `scale` (box)
 *   ppmtool topng in.ppm out.png [scale]
 *   ppmtool crop in.ppm out.ppm x y w h
 *
 * Build: gcc -O2 -o ppmtool ppmtool.c -lz
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
    if (!f) { fprintf(stderr, "cannot open %s\n", path); return 0; }
    if (fscanf(f, "%2s %d %d %d", magic, &im->w, &im->h, &maxv) != 4 || strcmp(magic, "P6")) {
        fprintf(stderr, "%s: not a P6 ppm\n", path); fclose(f); return 0;
    }
    fgetc(f);
    im->p = malloc((size_t)im->w * im->h * 3);
    if (fread(im->p, 3, (size_t)im->w * im->h, f) != (size_t)im->w * im->h) {
        fprintf(stderr, "%s: short read\n", path); fclose(f); return 0;
    }
    fclose(f);
    return 1;
}

static void wr_ppm(const char* path, const Img* im) {
    FILE* f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", im->w, im->h);
    fwrite(im->p, 3, (size_t)im->w * im->h, f);
    fclose(f);
}

static void put32(unsigned char* b, unsigned v) {
    b[0] = v >> 24; b[1] = v >> 16; b[2] = v >> 8; b[3] = v;
}
static void chunk(FILE* f, const char* t, const unsigned char* d, unsigned n) {
    unsigned char b[4];
    unsigned crc = crc32(0, (const unsigned char*)t, 4);
    crc = crc32(crc, d, n);
    put32(b, n); fwrite(b, 1, 4, f);
    fwrite(t, 1, 4, f);
    fwrite(d, 1, n, f);
    put32(b, crc); fwrite(b, 1, 4, f);
}
static void wr_png(const char* path, const Img* im) {
    size_t rawn = (size_t)(im->w * 3 + 1) * im->h;
    unsigned char* raw = malloc(rawn);
    uLongf zn = compressBound(rawn);
    unsigned char* z = malloc(zn);
    unsigned char ihdr[13];
    FILE* f;
    int y;
    for (y = 0; y < im->h; y++) {
        raw[(size_t)y * (im->w * 3 + 1)] = 0;
        memcpy(raw + (size_t)y * (im->w * 3 + 1) + 1, im->p + (size_t)y * im->w * 3, (size_t)im->w * 3);
    }
    compress2(z, &zn, raw, rawn, 6);
    f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "cannot write %s\n", path); return; }
    fwrite("\x89PNG\r\n\x1a\n", 1, 8, f);
    put32(ihdr, im->w); put32(ihdr + 4, im->h);
    ihdr[8] = 8; ihdr[9] = 2; ihdr[10] = ihdr[11] = ihdr[12] = 0;
    chunk(f, "IHDR", ihdr, 13);
    chunk(f, "IDAT", z, (unsigned)zn);
    chunk(f, "IEND", NULL, 0);
    fclose(f);
    free(raw); free(z);
}

static Img down(const Img* a, int s) {
    Img o;
    int x, y, c, i, j;
    o.w = a->w / s; o.h = a->h / s;
    o.p = malloc((size_t)o.w * o.h * 3);
    for (y = 0; y < o.h; y++)
        for (x = 0; x < o.w; x++)
            for (c = 0; c < 3; c++) {
                int acc = 0;
                for (j = 0; j < s; j++)
                    for (i = 0; i < s; i++)
                        acc += a->p[((size_t)(y * s + j) * a->w + x * s + i) * 3 + c];
                o.p[((size_t)y * o.w + x) * 3 + c] = (unsigned char)(acc / (s * s));
            }
    return o;
}

int main(int argc, char** argv) {
    if (argc >= 4 && !strcmp(argv[1], "diff")) {
        Img a, b;
        long n, i, any = 0, big = 0;
        int mx = 0, x0 = 1 << 30, y0 = 1 << 30, x1 = -1, y1 = -1;
        double sum = 0;
        Img heat;
        if (!rd(argv[2], &a) || !rd(argv[3], &b)) return 2;
        if (a.w != b.w || a.h != b.h) { printf("size mismatch %dx%d vs %dx%d\n", a.w, a.h, b.w, b.h); return 2; }
        n = (long)a.w * a.h;
        heat.w = a.w; heat.h = a.h; heat.p = malloc((size_t)n * 3);
        for (i = 0; i < n; i++) {
            int c, s = 0;
            for (c = 0; c < 3; c++) {
                int d = abs(a.p[i * 3 + c] - b.p[i * 3 + c]);
                if (d > mx) mx = d;
                s += d;
                sum += d;
                heat.p[i * 3 + c] = (unsigned char)(d * 8 > 255 ? 255 : d * 8);
            }
            if (s) any++;
            if (s > 24) {
                int x = (int)(i % a.w), y = (int)(i / a.w);
                big++;
                if (x < x0) x0 = x;
                if (y < y0) y0 = y;
                if (x > x1) x1 = x;
                if (y > y1) y1 = y;
            }
            if (!s) {   /* dim copy of the image so the heat map has context */
                for (c = 0; c < 3; c++) heat.p[i * 3 + c] = a.p[i * 3 + c] / 5;
            }
        }
        if (!any) printf("identical\n");
        else if (big)
            printf("max %d mean %.4f anydiff %.3f%% >24 %.3f%% bbox %d,%d-%d,%d\n", mx, sum / (n * 3.0),
                   100.0 * any / n, 100.0 * big / n, x0, y0, x1, y1);
        else
            printf("max %d mean %.4f anydiff %.3f%% >24 0%%\n", mx, sum / (n * 3.0), 100.0 * any / n);
        if (argc >= 5) wr_ppm(argv[4], &heat);
        return 0;
    }
    if (argc >= 5 && !strcmp(argv[1], "sheet")) {
        int s = atoi(argv[3]), k, nimg = argc - 4, gap = 6, W = 0, H = 0, x = 0;
        Img* ims = calloc(nimg, sizeof(Img)), o;
        for (k = 0; k < nimg; k++) {
            Img t;
            if (!rd(argv[4 + k], &t)) return 2;
            ims[k] = s > 1 ? down(&t, s) : t;
            W += ims[k].w + (k ? gap : 0);
            if (ims[k].h > H) H = ims[k].h;
        }
        o.w = W; o.h = H; o.p = malloc((size_t)W * H * 3);
        memset(o.p, 255, (size_t)W * H * 3);
        for (k = 0; k < nimg; k++) {
            int y;
            for (y = 0; y < ims[k].h; y++)
                memcpy(o.p + ((size_t)y * W + x) * 3, ims[k].p + (size_t)y * ims[k].w * 3, (size_t)ims[k].w * 3);
            x += ims[k].w + gap;
        }
        wr_png(argv[2], &o);
        return 0;
    }
    if (argc >= 4 && !strcmp(argv[1], "topng")) {
        Img a;
        int s = argc >= 5 ? atoi(argv[4]) : 1;
        if (!rd(argv[2], &a)) return 2;
        if (s > 1) a = down(&a, s);
        wr_png(argv[3], &a);
        return 0;
    }
    if (argc >= 8 && !strcmp(argv[1], "crop")) {
        Img a, o;
        int x = atoi(argv[4]), y = atoi(argv[5]), w = atoi(argv[6]), h = atoi(argv[7]), j;
        if (!rd(argv[2], &a)) return 2;
        o.w = w; o.h = h; o.p = malloc((size_t)w * h * 3);
        for (j = 0; j < h; j++) memcpy(o.p + (size_t)j * w * 3, a.p + ((size_t)(y + j) * a.w + x) * 3, (size_t)w * 3);
        wr_ppm(argv[3], &o);
        return 0;
    }
    fprintf(stderr, "usage: see header\n");
    return 1;
}

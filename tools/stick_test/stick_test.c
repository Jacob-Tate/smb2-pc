/* Unit test of the GC stick emulation (include/gc_stick.h).
 *   cc -O2 -I../../include stick_test.c ../../src/input/gc_stick.c -lm -o stick_test && ./stick_test [-v]
 * Checks: SDL axis normalisation, the emulated GC gate against Dolphin's
 * reference values (cardinal/diagonal magnitudes, octagon shape, rounding),
 * monotonicity, no clamping artefacts, calibration, and the game's own
 * stick processing (full deflection -> 60 on both axes at the diagonals). */
#include "gc_stick.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.14159265358979323846
static int fails, checks, verbose;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* Independent reference: Dolphin's octagon gate (vertex radius R at every
 * multiple of 45 deg, straight edges between) and MapFloat, written from the
 * geometry rather than from gc_stick.c. */
static double ref_gate(double R, double ang)
{
    double s = fmod(ang, PI / 4); if (s < 0) s += PI / 4;
    /* edge between vertices at 0 and 45 deg: distance to the edge along ang */
    double ax = R, ay = 0, bx = R * cos(PI / 4), by = R * sin(PI / 4);
    double dx = cos(s), dy = sin(s), ex = bx - ax, ey = by - ay;
    return (ax * ey - ay * ex) / (dx * ey - dy * ex);
}
static int ref_map(double v)
{
    long long u = v > 0 ? llround(127 * v + 128) : llround(127 * v + 128);
    return (int)(u - 128);
}

static void game_tilt(const GcsPadConfig* c, float x, float y, int* gx, int* gy, int* rx, int* ry)
{
    int8_t ox, oy;
    gcs_map_stick(c, GCS_MAIN, x, y, &ox, &oy, NULL);
    if (rx) { *rx = ox; *ry = oy; }
    gcs_game_stick(&ox, &oy, gcs_game_main_gate);
    *gx = ox; *gy = oy;
}

int main(int argc, char** argv)
{
    GcsPadConfig c;
    int i, k;
    verbose = argc > 1 && !strcmp(argv[1], "-v");
    gcs_config_defaults(&c);

    /* --- SDL axis normalisation (asymmetric -32768..32767) */
    CHECK(gcs_axis_norm(-32768) == -1.0f, "min");
    CHECK(gcs_axis_norm(32767) == 1.0f, "max");
    CHECK(gcs_axis_norm(0) == 0.0f, "zero");

    /* --- full deflection: Dolphin reference values */
    {
        static const struct { int stick; double deg; int x, y; } ref[] = {
            { GCS_MAIN, 0, 101, 0 }, { GCS_MAIN, 90, 0, 101 }, { GCS_MAIN, 180, -101, 0 },
            { GCS_MAIN, 270, 0, -101 }, { GCS_MAIN, 45, 71, 71 }, { GCS_MAIN, 135, -71, 71 },
            { GCS_MAIN, 225, -71, -71 }, { GCS_MAIN, 315, 71, -71 },
            { GCS_C, 0, 92, 0 }, { GCS_C, 90, 0, 92 }, { GCS_C, 45, 65, 65 }, { GCS_C, 225, -65, -65 },
        };
        for (i = 0; i < (int)(sizeof ref / sizeof ref[0]); i++) {
            int8_t ox, oy;
            double a = ref[i].deg * PI / 180;
            gcs_map_stick(&c, ref[i].stick, (float)cos(a), (float)sin(a), &ox, &oy, NULL);
            CHECK(ox == ref[i].x && oy == ref[i].y, "%s stick %g deg: got (%d,%d) want (%d,%d)",
                  ref[i].stick ? "C" : "main", ref[i].deg, ox, oy, ref[i].x, ref[i].y);
            gcs_full_deflection(ref[i].stick, a, &ox, &oy);
            CHECK(ox == ref[i].x && oy == ref[i].y, "full deflection %g deg", ref[i].deg);
        }
    }

    /* --- whole edge vs the independent reference, 3600 angles, both sticks */
    for (k = 0; k < 2; k++) {
        double R = k ? GCS_C_GATE_RADIUS : GCS_MAIN_GATE_RADIUS;
        int worst = 0;
        for (i = 0; i < 3600; i++) {
            double a = i * 2 * PI / 3600;
            int8_t ox, oy;
            int ex = ref_map(cos(a) * ref_gate(R, a)), ey = ref_map(sin(a) * ref_gate(R, a));
            gcs_map_stick(&c, k, (float)cos(a), (float)sin(a), &ox, &oy, NULL);
            if (abs(ox - ex) > worst) worst = abs(ox - ex);
            if (abs(oy - ey) > worst) worst = abs(oy - ey);
        }
        /* float input (x, y) vs exact angle: allow 1 unit of rounding */
        CHECK(worst <= 1, "%s stick edge differs from reference by %d", k ? "C" : "main", worst);
    }

    /* --- octagon shape: radius at 22.5 deg = R cos(22.5) (edge midpoint) */
    {
        double r = gcs_gate_radius(GCS_MAIN, GCS_GATE_OCTAGON, PI / 8);
        CHECK(fabs(r - GCS_MAIN_GATE_RADIUS * cos(PI / 8)) < 1e-9, "edge midpoint %f", r);
        CHECK(fabs(gcs_gate_radius(GCS_MAIN, GCS_GATE_CIRCLE, PI / 8) - GCS_MAIN_GATE_RADIUS) < 1e-12, "circle");
    }

    /* --- monotonic along every ray, never beyond the gate, no clamping
     * (no axis reaches the s8 limits; cardinal never above 101) */
    for (k = 0; k < 2; k++) {
        int bad = 0, maxv = 0;
        for (i = 0; i < 360; i++) {
            double a = i * PI / 180;
            double prev = -1;
            int s;
            for (s = 0; s <= 200; s++) {
                double d = s / 200.0;
                int8_t ox, oy;
                double m;
                gcs_map_stick(&c, k, (float)(d * cos(a)), (float)(d * sin(a)), &ox, &oy, NULL);
                m = sqrt((double)ox * ox + (double)oy * oy);
                if (m + 1.5 < prev) bad++;            /* quantisation may wobble by <1.5 */
                if (m > prev) prev = m;
                if (abs(ox) > maxv) maxv = abs(ox);
                if (abs(oy) > maxv) maxv = abs(oy);
            }
        }
        CHECK(bad == 0, "%s stick not monotonic (%d)", k ? "C" : "main", bad);
        CHECK(maxv == (k ? 92 : 101), "%s stick max axis %d", k ? "C" : "main", maxv);
    }

    /* --- deadzone: inside -> 0, just outside -> small */
    {
        int8_t ox, oy;
        gcs_map_stick(&c, GCS_MAIN, 0.049f, 0, &ox, &oy, NULL);
        CHECK(ox == 0 && oy == 0, "inside deadzone (%d,%d)", ox, oy);
        gcs_map_stick(&c, GCS_MAIN, 0.06f, 0, &ox, &oy, NULL);
        CHECK(ox == 1, "just outside deadzone %d", ox);
    }

    /* --- a circular host stick rotated at full deflection gives the octagon,
     * a square-ish one (corners up to 1,1) too once calibrated */
    {
        GcsPadConfig cc;
        double prevx = 1, prevy = 0;
        gcs_config_defaults(&cc);
        gcs_cal_begin(&cc.cal[GCS_MAIN]);
        for (i = 0; i <= 720; i++) {   /* two slow turns of a squircle stick */
            double a = i * PI / 180, ca = cos(a), sa = sin(a);
            double r = 1.0 / pow(pow(fabs(ca), 4) + pow(fabs(sa), 4), 0.25) * 0.97;   /* reaches 0.97..1.15 */
            double x = r * ca, y = r * sa;
            if (x > 1) x = 1; if (x < -1) x = -1; if (y > 1) y = 1; if (y < -1) y = -1;
            gcs_cal_update(&cc.cal[GCS_MAIN], (float)prevx, (float)prevy, (float)x, (float)y);
            prevx = x; prevy = y;
        }
        CHECK(gcs_cal_done(&cc.cal[GCS_MAIN]), "calibration incomplete (%d)", gcs_cal_coverage(&cc.cal[GCS_MAIN]));
        cc.cal[GCS_MAIN].valid = 1;
        for (i = 0; i < 360; i += 5) {
            double a = i * PI / 180, ca = cos(a), sa = sin(a);
            double r = 1.0 / pow(pow(fabs(ca), 4) + pow(fabs(sa), 4), 0.25) * 0.97;
            int8_t ox, oy, fx, fy;
            gcs_map_stick(&cc, GCS_MAIN, (float)(r * ca), (float)(r * sa), &ox, &oy, NULL);
            gcs_full_deflection(GCS_MAIN, a, &fx, &fy);
            CHECK(abs(ox - fx) <= 1 && abs(oy - fy) <= 1, "calibrated edge %d deg: (%d,%d) want (%d,%d)", i, ox, oy, fx, fy);
            /* half way out: half the gate */
            gcs_map_stick(&cc, GCS_MAIN, (float)(0.5 * r * ca), (float)(0.5 * r * sa), &ox, &oy, NULL);
            {
                double want = (0.5 - 0.05) / 0.95 * gcs_gate_radius(GCS_MAIN, 0, a) * 127;
                double got = sqrt((double)ox * ox + (double)oy * oy);
                CHECK(fabs(got - want) <= 1.5, "calibrated half deflection %d deg: %f want %f", i, got, want);
            }
        }
    }

    /* --- the game's processing (0x80009A10): full deflection everywhere
     * saturates (60) on each driven axis, i.e. the octagon becomes a square */
    for (i = 0; i < 360; i += 45) {
        double a = i * PI / 180;
        int gx, gy;
        game_tilt(&c, (float)cos(a), (float)sin(a), &gx, &gy, NULL, NULL);
        CHECK(abs(gx) == (i % 180 == 90 ? 0 : 60) && abs(gy) == (i % 180 == 0 ? 0 : 60),
              "game value at %d deg: (%d,%d)", i, gx, gy);
    }
    {   /* C stick through its own gate (80/56) */
        int8_t x = 92, y = 0, dx = 65, dy = 65;
        gcs_game_stick(&x, &y, gcs_game_c_gate);
        gcs_game_stick(&dx, &dy, gcs_game_c_gate);
        CHECK(x == 60 && y == 0 && dx == 60 && dy == 60, "C stick game values (%d,%d) (%d,%d)", x, y, dx, dy);
    }
    {   /* raw values from the curve's own definition: gate point 84 -> square 105 */
        int8_t x = 42, y = 0;   /* half of the gate -> square 52.5 -> curve[52] = 16 */
        gcs_game_stick(&x, &y, gcs_game_main_gate);
        CHECK(x == gcs_game_curve[52] && y == 0, "half gate -> %d", x);
        x = -42; y = 0;
        gcs_game_stick(&x, &y, gcs_game_main_gate);
        CHECK(x == -gcs_game_curve[52], "negative half gate -> %d", x);
    }

    /* --- response table: a naive linear mapping (SDL/256 straight to the
     * game, "old") vs the emulated GC pad + the game's processing ("new").
     * Tilt = value / 60. */
    printf("deflection  old fwd  new fwd  | new diag (x,y) | raw GC fwd / diag\n");
    for (i = 0; i <= 20; i++) {
        double d = i / 20.0;
        int oldv = (int)(d * 32767) / 256;   /* linear SDL/256 */
        int gx, gy, dx, dy, rx, ry, rdx, rdy;
        game_tilt(&c, 0, (float)d, &gx, &gy, &rx, &ry);
        game_tilt(&c, (float)(d * cos(PI / 4)), (float)(d * sin(PI / 4)), &dx, &dy, &rdx, &rdy);
        if (oldv > 127) oldv = 127;
        printf("  %4.0f%%     %5.2f    %5.2f   | %5.2f,%5.2f    | %4d / %d,%d\n", d * 100,
               (oldv > 60 ? 60 : oldv) / 60.0, gy / 60.0, dx / 60.0, dy / 60.0, ry, rdx, rdy);
    }

    printf("%d checks, %d failed\n", checks, fails);
    return fails != 0;
}

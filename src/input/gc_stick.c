/* GameCube analog stick emulation: see include/gc_stick.h and docs/stick.md. */
#include "gc_stick.h"
#include <math.h>
#include <string.h>

#define PI  3.14159265358979323846
#define TAU (2.0 * PI)

/* ------------------------------------------------------------------ game data
 * Read from the DOL (build/GM2E8P/asm/auto_06_8007F1E0_rodata.s,
 * auto_07_80081640_data.s). */

/* 0x8007F1E0: response curve, |square value| 0..99 -> stick value; 60 at the
 * top, which world.c divides by (stick / 60.0 = full tilt). */
const uint8_t gcs_game_curve[100] = {
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  2,
     2,  2,  2,  3,  3,  3,  4,  4,  4,  5,  5,  5,  6,  6,  7,  7,  7,  8,  8,  9,
     9, 10, 10, 11, 11, 12, 12, 13, 14, 14, 15, 15, 16, 17, 17, 18, 19, 19, 20, 21,
    22, 22, 23, 24, 25, 25, 26, 27, 28, 29, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
    39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 54, 55, 56, 57, 58, 60,
};

/* 0x8007F244: the square each gate is mapped onto */
const int8_t gcs_game_target[8][2] = {
    { 105, 0 }, { 105, 105 }, { 0, 105 }, { -105, 105 },
    { -105, 0 }, { -105, -105 }, { 0, -105 }, { 105, -105 },
};

/* 0x80081AC0 (.data, per pad; the Options stick adjustment overwrites it):
 * the control stick's expected gate */
const int8_t gcs_game_main_gate[8][2] = {
    { 84, 0 }, { 59, 59 }, { 0, 84 }, { -59, 59 },
    { -84, 0 }, { -59, -59 }, { 0, -84 }, { 59, -59 },
};

/* 0x8007F254 (.rodata, 4 identical copies): the C stick's gate */
const int8_t gcs_game_c_gate[8][2] = {
    { 80, 0 }, { 56, 56 }, { 0, 80 }, { -56, 56 },
    { -80, 0 }, { -56, -56 }, { 0, -80 }, { 56, -56 },
};

/* --------------------------------------------------------- stage 2: the game */

/* |v| clamped to 0..99 through the curve, sign kept (0x80009E5C..) */
static int8_t curve_lookup(float v)
{
    int i = (int)v;              /* fctiwz: toward zero */
    int neg = i < 0;
    int a = neg ? -i : i;
    if (a > 99) a = 99;
    return (int8_t)((neg ? -1 : 1) * gcs_game_curve[a]);
}

void gcs_game_stick(int8_t* px, int8_t* py, const int8_t gate[8][2])
{
    int j;
    for (j = 0; j < 8; j++) {
        int n = (j == 7) ? 0 : j + 1;
        float x1 = gate[j][0], y1 = gate[j][1];
        float x2 = gate[n][0], y2 = gate[n][1];
        float sx, sy, inv, a, b;
        float det = x1 * y2 - x2 * y1;
        if (fabs(det) < 1.1920928955078125e-7)   /* lbl_80199480 (double) */
            continue;
        inv = 1.0f / det;
        sx = *px;
        sy = *py;
        /* barycentric coordinates of the stick in the triangle (0, Pj, Pn) */
        a = inv * (y2 * sx - x2 * sy);
        b = inv * (-y1 * sx + x1 * sy);
        if (a < 0.0f || b < 0.0f)
            continue;
        *px = curve_lookup(a * (float)gcs_game_target[j][0] + b * (float)gcs_game_target[n][0]);
        *py = curve_lookup(a * (float)gcs_game_target[j][1] + b * (float)gcs_game_target[n][1]);
        return;
    }
}

/* ------------------------------------------------------- stage 1: host -> GC */

void gcs_config_defaults(GcsPadConfig* c)
{
    memset(c, 0, sizeof *c);
    c->gate = GCS_GATE_OCTAGON;
    c->deadzone = GCS_DEFAULT_DEADZONE;
    c->sensitivity = GCS_DEFAULT_SENSITIVITY;
}

float gcs_axis_norm(int v)
{
    float f = v < 0 ? (float)v / 32768.0f : (float)v / 32767.0f;
    return f < -1.0f ? -1.0f : f > 1.0f ? 1.0f : f;
}

/* Dolphin OctagonStickGate::GetRadiusAtAngle: vertices at the cardinals and
 * diagonals, all at the gate radius (law of sines on the ASA triangle) */
static double octagon_radius(double radius, double angle)
{
    const double half_int_angle = (8 - 2) * PI / 8 / 2;
    angle = fmod(angle, TAU / 8);
    if (angle < 0) angle += TAU / 8;
    return radius / sin(PI - angle - half_int_angle) * sin(half_int_angle);
}

double gcs_gate_radius(int stick, int gate, double angle)
{
    double r = stick == GCS_C ? GCS_C_GATE_RADIUS : GCS_MAIN_GATE_RADIUS;
    return gate == GCS_GATE_CIRCLE ? r : octagon_radius(r, angle);
}

/* Ray from the origin at `angle` against the segment p1-p2: distance, or -1 */
static double ray_segment(double angle, double x1, double y1, double x2, double y2)
{
    double dx = cos(angle), dy = sin(angle);
    double ex = x2 - x1, ey = y2 - y1;
    double den = dx * ey - dy * ex;
    double t, u;
    if (fabs(den) < 1e-12)
        return -1.0;
    t = (x1 * ey - y1 * ex) / den;      /* along the ray */
    u = (x1 * dy - y1 * dx) / den;      /* along the segment */
    if (t < 0.0 || u < -1e-9 || u > 1.0 + 1e-9)
        return -1.0;
    return t;
}

double gcs_cal_radius(const GcsCal* cal, double angle)
{
    double step = TAU / GCS_CAL_SAMPLES, a0, a1, d;
    int i, n;
    if (!cal || !cal->valid)
        return 1.0;
    angle = fmod(angle, TAU);
    if (angle < 0) angle += TAU;
    i = (int)(angle / step);
    if (i >= GCS_CAL_SAMPLES) i = GCS_CAL_SAMPLES - 1;
    n = (i + 1) % GCS_CAL_SAMPLES;
    a0 = i * step;
    a1 = (i + 1) * step;
    /* Dolphin GetCalibrationDataRadiusAtAngle: the polygon edge between the
     * two neighbouring samples */
    d = ray_segment(angle, cal->r[i] * cos(a0), cal->r[i] * sin(a0),
                    cal->r[n] * cos(a1), cal->r[n] * sin(a1));
    return d > 0.0 ? d : cal->r[i];
}

void gcs_cal_begin(GcsCal* cal)
{
    memset(cal, 0, sizeof *cal);
}

/* Dolphin ReshapableInput::UpdateCalibrationData: every sample ray the
 * movement crossed takes the crossing distance if it is larger. */
void gcs_cal_update(GcsCal* cal, float px, float py, float x, float y)
{
    int i;
    for (i = 0; i < GCS_CAL_SAMPLES; i++) {
        double a = TAU * i / GCS_CAL_SAMPLES, d;
        d = ray_segment(a, px, py, x, y);
        if (d < 0.0) {
            /* not moving: a point right on the ray still counts */
            double r = sqrt((double)x * x + (double)y * y);
            double ang = atan2(y, x), diff;
            if (r <= 0.0) continue;
            diff = fabs(remainder(ang - a, TAU));
            if (diff > 0.5 * TAU / GCS_CAL_SAMPLES / 4) continue;
            d = r * cos(diff);
        }
        if (d > cal->r[i])
            cal->r[i] = (float)(d > 1.5 ? 1.5 : d);
    }
}

int gcs_cal_coverage(const GcsCal* cal)
{
    int i, n = 0;
    for (i = 0; i < GCS_CAL_SAMPLES; i++)
        if (cal->r[i] >= 0.5f) n++;
    return n;
}

int gcs_cal_done(const GcsCal* cal)
{
    return gcs_cal_coverage(cal) == GCS_CAL_SAMPLES;
}

/* Dolphin MapFloat<u8>(v, 0x80, 1) for a stick axis, then PADRead's
 * origin subtraction (s8 = u8 - 0x80) */
static int8_t map_axis(double v)
{
    long long u;
    if (v > 0)
        u = llround((255 - 128) * v + 128);
    else
        u = llround((128 - 1) * v + 128);
    if (u < 1) u = 1;
    if (u > 255) u = 255;
    return (int8_t)(u - 128);
}

void gcs_map_stick(const GcsPadConfig* c, int stick, float x, float y,
                   int8_t* ox, int8_t* oy, float* norm)
{
    double len = sqrt((double)x * x + (double)y * y), dist, angle, dz;
    const GcsCal* cal = c ? &c->cal[stick] : NULL;
    int gate = c ? c->gate : GCS_GATE_OCTAGON;
    angle = atan2((double)y, (double)x);
    if (angle < 0) angle += TAU;
    /* distance as a fraction of the stick's calibrated edge at this angle */
    dist = len / gcs_cal_radius(cal, angle);
    /* deadzone as a fraction of the calibrated radius, remaining range
     * rescaled (Dolphin ApplyDeadzone) */
    dz = c ? c->deadzone / 100.0 : GCS_DEFAULT_DEADZONE / 100.0;
    dist = dist > dz ? (dist - dz) / (1.0 - dz) : 0.0;
    if (c) dist *= c->sensitivity / 100.0;
    if (dist > 1.0) dist = 1.0;
    if (norm) *norm = (float)dist;
    /* scale to the gate */
    dist *= gcs_gate_radius(stick, gate, angle);
    *ox = map_axis(cos(angle) * dist);
    *oy = map_axis(sin(angle) * dist);
}

void gcs_full_deflection(int stick, double angle, int8_t* ox, int8_t* oy)
{
    double d = gcs_gate_radius(stick, GCS_GATE_OCTAGON, angle);
    *ox = map_axis(cos(angle) * d);
    *oy = map_axis(sin(angle) * d);
}

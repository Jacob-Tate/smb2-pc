/* GameCube analog stick emulation (docs/stick.md).
 *
 * Two stages, both pure functions (no SDL), so tools/stick_test can check them:
 *
 *  1. Host stick -> emulated GC pad (what PADRead returns): radial deadzone,
 *     per-controller calibration (the stick's measured edge, rotate-the-stick
 *     like Dolphin), sensitivity, then the GC octagonal gate with Dolphin's
 *     radii (main stick 0.7937125, C stick 0.7221375 of 127: cardinal 101,
 *     diagonal 71/71; C stick 92, 65/65) and Dolphin's MapFloat rounding.
 *
 *  2. The game's own stick processing, which the GC version runs on every
 *     PADRead result (g_read_inputs_and_maybe_reset_console, 0x80009A10 and
 *     fn_8000A380): each stick is mapped from a per-pad gate octagon onto a
 *     square (+-105) and through a 100-entry response curve whose top is 60,
 *     the value world.c divides by. */
#ifndef GC_STICK_H
#define GC_STICK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GCS_MAIN 0
#define GCS_C    1

#define GCS_GATE_OCTAGON 0   /* GameCube octagon (default) */
#define GCS_GATE_CIRCLE  1   /* raw circle: same radius in every direction */

#define GCS_CAL_SAMPLES 32   /* calibration rays, as Dolphin's CALIBRATION_SAMPLE_COUNT */

/* Dolphin GCPadEmu.h: "Values averaged from multiple genuine GameCube controllers." */
#define GCS_MAIN_GATE_RADIUS 0.7937125
#define GCS_C_GATE_RADIUS    0.7221375

typedef struct {
    float r[GCS_CAL_SAMPLES];   /* edge distance at angle 2*pi*i/N (normalised axis units) */
    int valid;                  /* 0: no calibration (circle of radius 1) */
} GcsCal;

typedef struct {
    int gate;           /* GCS_GATE_* */
    int deadzone;       /* percent of the calibrated radius, 0..50 */
    int sensitivity;    /* percent, 50..200; 100 = a real GC stick */
    GcsCal cal[2];      /* GCS_MAIN, GCS_C */
} GcsPadConfig;

#define GCS_DEFAULT_DEADZONE    5
#define GCS_DEFAULT_SENSITIVITY 100

void gcs_config_defaults(GcsPadConfig* c);

/* SDL axis value (-32768..32767) -> -1..1.  SDL's range is asymmetric, so the
 * two halves are scaled separately (both ends reach exactly 1). */
float gcs_axis_norm(int v);

/* Gate radius (fraction of 127) at an angle (radians). */
double gcs_gate_radius(int stick, int gate, double angle);

/* Calibrated edge distance at an angle (1 without calibration). */
double gcs_cal_radius(const GcsCal* cal, double angle);

/* Calibration capture: begin clears, update feeds the segment between the
 * previous and the current stick position (normalised, y up), done tells
 * whether every ray has been reached far enough to be trusted. */
void gcs_cal_begin(GcsCal* cal);
void gcs_cal_update(GcsCal* cal, float px, float py, float x, float y);
int  gcs_cal_done(const GcsCal* cal);
int  gcs_cal_coverage(const GcsCal* cal);   /* rays captured, 0..GCS_CAL_SAMPLES */

/* Stage 1: host stick (normalised, y up) -> emulated GC pad value (s8, as
 * PADRead reports it, y up).  Also returns the reshaped unit-disc input
 * (after deadzone/calibration/sensitivity) when `norm` is not NULL. */
void gcs_map_stick(const GcsPadConfig* c, int stick, float x, float y,
                   int8_t* ox, int8_t* oy, float* norm);

/* A full deflection of the emulated stick at an angle (keyboard / tests). */
void gcs_full_deflection(int stick, double angle, int8_t* ox, int8_t* oy);

/* Stage 2: the game's processing of one stick (exact port of the loop in
 * 0x80009A10 / fn_8000A380).  `gate` is the pad's 8 gate points (x, y). */
void gcs_game_stick(int8_t* x, int8_t* y, const int8_t gate[8][2]);

extern const uint8_t gcs_game_curve[100];        /* 0x8007F1E0 */
extern const int8_t  gcs_game_target[8][2];      /* 0x8007F244: square, +-105 */
extern const int8_t  gcs_game_main_gate[8][2];   /* 0x80081AC0 default: 84 / 59 */
extern const int8_t  gcs_game_c_gate[8][2];      /* 0x8007F254: 80 / 56 */

#ifdef __cplusplus
}
#endif
#endif /* GC_STICK_H */

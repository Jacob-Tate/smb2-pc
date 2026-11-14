/* Per-controller stick settings / calibration (controllers.ini), and the
 * port API the stick calibration screen uses (input_sdl2.c). */
#ifndef PAD_CONFIG_H
#define PAD_CONFIG_H

#include "gc_stick.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PADCFG_DEADZONE_MAX 50
#define PADCFG_SENS_MIN 50
#define PADCFG_SENS_MAX 200

void padcfg_sanitize(GcsPadConfig* c);
void padcfg_get(const char* guid, GcsPadConfig* out);   /* defaults if unknown */
void padcfg_put(const char* guid, const char* name, const GcsPadConfig* cfg);   /* + save */

/* ---- ports (input_sdl2.c) ---- */
typedef struct {
    float host[2][2];     /* host stick, normalised -1..1, y up (main, C) */
    int8_t gc[2][2];      /* emulated GC pad (what PADRead returns) */
    int8_t game[2][2];    /* after the game's processing (what it plays with) */
    float norm[2];        /* reshaped distance 0..1 (after deadzone / calibration) */
} PadStickView;

int  input_port_connected(int port);            /* a pad is open on this port (0-3) */
const char* input_port_name(int port);
GcsPadConfig* input_port_config(int port);      /* NULL without a pad */
void input_port_config_save(int port);          /* store the port's config by GUID */
int  input_port_view(int port, PadStickView* v);   /* 0 without a pad */

#ifdef __cplusplus
}
#endif
#endif /* PAD_CONFIG_H */

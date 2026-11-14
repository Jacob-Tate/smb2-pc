/* Per-controller stick settings and calibration, kept by SDL GUID in
 * controllers.ini in the save directory (docs/stick.md):
 *
 *   [030000005e040000000b000000007200]
 *   name=Xbox One Elite Controller
 *   gate=octagon            ; or circle
 *   deadzone=5              ; percent
 *   sensitivity=100         ; percent
 *   main=1000 998 ...       ; 32 calibrated edge distances x1000, or absent
 *   cstick=...
 */
#include "pad_config.h"
#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ENTRIES 32

typedef struct {
    char guid[40];
    char name[96];
    GcsPadConfig cfg;
} Entry;

static Entry s_entries[MAX_ENTRIES];
static int s_count = -1;

static void ini_path(char* out, size_t n)
{
    const char* d = platform_get_save_dir();
    snprintf(out, n, "%scontrollers.ini", d ? d : "");
}

static void parse_cal(GcsCal* cal, const char* v)
{
    int i;
    char* end;
    memset(cal, 0, sizeof *cal);
    for (i = 0; i < GCS_CAL_SAMPLES; i++) {
        long x = strtol(v, &end, 10);
        if (end == v) break;
        cal->r[i] = (float)x / 1000.0f;
        v = end;
    }
    cal->valid = (i == GCS_CAL_SAMPLES && gcs_cal_done(cal));
}

static void load_all(void)
{
    char path[1100], line[1024];
    FILE* f;
    Entry* e = NULL;
    s_count = 0;
    ini_path(path, sizeof path);
    f = fopen(path, "rb");
    if (!f) return;
    while (fgets(line, sizeof line, f)) {
        char* v;
        line[strcspn(line, "\r\n")] = 0;
        if (line[0] == '[') {
            char* close = strchr(line, ']');
            e = NULL;
            if (!close || s_count >= MAX_ENTRIES) continue;
            *close = 0;
            e = &s_entries[s_count++];
            memset(e, 0, sizeof *e);
            snprintf(e->guid, sizeof e->guid, "%s", line + 1);
            gcs_config_defaults(&e->cfg);
            continue;
        }
        if (!e || line[0] == ';' || !(v = strchr(line, '='))) continue;
        *v++ = 0;
        if      (!strcmp(line, "name")) snprintf(e->name, sizeof e->name, "%s", v);
        else if (!strcmp(line, "gate")) e->cfg.gate = !strcmp(v, "circle") ? GCS_GATE_CIRCLE : GCS_GATE_OCTAGON;
        else if (!strcmp(line, "deadzone")) e->cfg.deadzone = atoi(v);
        else if (!strcmp(line, "sensitivity")) e->cfg.sensitivity = atoi(v);
        else if (!strcmp(line, "main")) parse_cal(&e->cfg.cal[GCS_MAIN], v);
        else if (!strcmp(line, "cstick")) parse_cal(&e->cfg.cal[GCS_C], v);
    }
    fclose(f);
    for (int i = 0; i < s_count; i++) padcfg_sanitize(&s_entries[i].cfg);
    printf("[pad] %s: %d controller(s)\n", path, s_count);
}

void padcfg_sanitize(GcsPadConfig* c)
{
    if (c->gate != GCS_GATE_CIRCLE) c->gate = GCS_GATE_OCTAGON;
    if (c->deadzone < 0) c->deadzone = 0;
    if (c->deadzone > PADCFG_DEADZONE_MAX) c->deadzone = PADCFG_DEADZONE_MAX;
    if (c->sensitivity < PADCFG_SENS_MIN) c->sensitivity = PADCFG_SENS_MIN;
    if (c->sensitivity > PADCFG_SENS_MAX) c->sensitivity = PADCFG_SENS_MAX;
}

void padcfg_get(const char* guid, GcsPadConfig* out)
{
    int i;
    if (s_count < 0) load_all();
    gcs_config_defaults(out);
    for (i = 0; i < s_count; i++)
        if (!strcmp(s_entries[i].guid, guid)) {
            *out = s_entries[i].cfg;
            return;
        }
}

static void write_cal(FILE* f, const char* key, const GcsCal* cal)
{
    int i;
    if (!cal->valid) return;
    fprintf(f, "%s=", key);
    for (i = 0; i < GCS_CAL_SAMPLES; i++)
        fprintf(f, i ? " %d" : "%d", (int)(cal->r[i] * 1000.0f + 0.5f));
    fprintf(f, "\n");
}

void padcfg_put(const char* guid, const char* name, const GcsPadConfig* cfg)
{
    char path[1100];
    FILE* f;
    Entry* e = NULL;
    int i;
    if (s_count < 0) load_all();
    for (i = 0; i < s_count; i++)
        if (!strcmp(s_entries[i].guid, guid)) e = &s_entries[i];
    if (!e) {
        if (s_count >= MAX_ENTRIES) {   /* drop the oldest */
            memmove(&s_entries[0], &s_entries[1], sizeof(Entry) * (MAX_ENTRIES - 1));
            s_count--;
        }
        e = &s_entries[s_count++];
        memset(e, 0, sizeof *e);
        snprintf(e->guid, sizeof e->guid, "%s", guid);
    }
    if (name) snprintf(e->name, sizeof e->name, "%s", name);
    e->cfg = *cfg;

    ini_path(path, sizeof path);
    f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "[pad] cannot write %s\n", path); return; }
    fprintf(f, "; SMB2 PC port: analog stick settings per controller (SDL GUID)\n");
    for (i = 0; i < s_count; i++) {
        const Entry* x = &s_entries[i];
        fprintf(f, "\n[%s]\nname=%s\ngate=%s\ndeadzone=%d\nsensitivity=%d\n", x->guid, x->name,
                x->cfg.gate == GCS_GATE_CIRCLE ? "circle" : "octagon",
                x->cfg.deadzone, x->cfg.sensitivity);
        write_cal(f, "main", &x->cfg.cal[GCS_MAIN]);
        write_cal(f, "cstick", &x->cfg.cal[GCS_C]);
    }
    fclose(f);
    printf("[pad] saved %s (%s)\n", path, guid);
}

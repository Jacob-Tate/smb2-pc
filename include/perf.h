/* --perf: lightweight per-frame timing + counters.
 *
 * Off by default; every hook is one predictable branch on g_perf_on.  The
 * frame boundary is the buffer swap (sdl2_swap_buffers); a summary line is
 * printed every PERF_REPORT_FRAMES frames. */
#ifndef SMB2_PERF_H
#define SMB2_PERF_H

#include <stdint.h>

#define PERF_REPORT_FRAMES 300

enum {
    PERF_T_SWAP,      /* SDL_GL_SwapWindow (vsync wait shows up here) */
    PERF_T_PACE,      /* video_finish_frame sleep */
    PERF_T_FLUSH,     /* gl_flush_batch: uniforms + buffer upload + draw */
    PERF_T_COPYTEX,   /* GXCopyTex: readback/encode/upload */
    PERF_T_TEXCREATE, /* texture decode + glTexImage2D (cache misses) */
    PERF_T_TEXINIT,   /* GXInitTexObj total (includes TEXCREATE) */
    PERF_T_AUDIO,     /* musyx_frame */
    PERF_T_COUNT
};

enum {
    PERF_C_DRAWS,
    PERF_C_VERTS,
    PERF_C_TEXUPLOAD,
    PERF_C_COPYTEX,
    PERF_C_READPIX,
    PERF_C_HASHBYTES,
    PERF_C_TEXINIT,
    PERF_C_COUNT
};

extern int g_perf_on;
extern double g_perf_t[PERF_T_COUNT];
extern long long g_perf_c[PERF_C_COUNT];

uint64_t perf_now(void);           /* raw performance counter */
double perf_ms(uint64_t a, uint64_t b);
void perf_frame_end(void);         /* call once per presented frame */

#define PERF_BEGIN(v) uint64_t v = g_perf_on ? perf_now() : 0
#define PERF_END(v, slot) do { if (g_perf_on) g_perf_t[slot] += perf_ms(v, perf_now()); } while (0)
#define PERF_COUNT(slot, n) do { if (g_perf_on) g_perf_c[slot] += (n); } while (0)

#endif

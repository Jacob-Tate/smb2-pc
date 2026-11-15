/* VI / render-mode shim, plus the per-frame hook the game's render.c calls.
 *
 * video_finish_frame() stands in for the GC's VIWaitForRetrace + XFB flip at
 * the end of every frame: it pumps SDL events, refreshes the GC pad globals,
 * presents the frame and paces the loop to 60 Hz. */
#include "shim.h"
#include "gx.h"
#include <stdio.h>
#include <stdlib.h>
#include "perf.h"

/* ---- render modes (SDK GXNtsc480IntDf and friends) ---- */
#define SP_DEFAULT {{6,6},{6,6},{6,6},{6,6},{6,6},{6,6},{6,6},{6,6},{6,6},{6,6},{6,6},{6,6}}
#define VF_DEFAULT {8, 8, 10, 12, 10, 8, 8}
#define VF_PROG    {0, 0, 21, 22, 21, 0, 0}

GXRenderModeObj ntsc_interlaced_render_mode = {
    0 /* VI_TVMODE_NTSC_INT */, 640, 480, 480, 40, 0, 640, 480,
    1 /* VI_XFBMODE_DF */, 0, 0, SP_DEFAULT, VF_DEFAULT
};

GXRenderModeObj ntsc_progressive_render_mode = {
    2 /* VI_TVMODE_NTSC_PROG */, 640, 480, 480, 40, 0, 640, 480,
    0 /* VI_XFBMODE_SF */, 0, 0, SP_DEFAULT, VF_PROG
};

GXRenderModeObj pal_interlaced_render_mode = {
    4 /* VI_TVMODE_PAL_INT */, 640, 480, 480, 40, 23, 640, 528,
    1, 0, 0, SP_DEFAULT, VF_DEFAULT
};

GXRenderModeObj mpal_interlaced_render_mode = {
    8 /* VI_TVMODE_MPAL_INT */, 640, 480, 480, 40, 0, 640, 480,
    1, 0, 0, SP_DEFAULT, VF_DEFAULT
};

GXRenderModeObj overscan_adjusted_render_mode; /* filled by set_render_mode */

GXRenderModeObj *current_render_mode = &ntsc_interlaced_render_mode;

void GXAdjustForOverscan(GXRenderModeObj *rmin, GXRenderModeObj *rmout, u16 hor, u16 ver)
{
    *rmout = *rmin;
    rmout->fbWidth = rmin->fbWidth - hor * 2;
    rmout->efbHeight = rmin->efbHeight - ver * 2;
    if (rmin->xFBmode == 0 /* SF */ && !rmin->field_rendering)
        rmout->xfbHeight = rmin->xfbHeight - ver;
    else
        rmout->xfbHeight = rmin->xfbHeight - ver * 2;
    rmout->viWidth = rmin->viWidth - hor * 2;
    if (rmin->viTVmode == 4 /* PAL int */ || rmin->viTVmode == 8)
        rmout->viHeight = rmin->viHeight - ver * 2;
    else
        rmout->viHeight = rmin->viHeight - ver * 2;
    rmout->viXOrigin = rmin->viXOrigin + hor;
    rmout->viYOrigin = rmin->viYOrigin + ver;
}

void GXSetDispCopySrc(u16 left, u16 top, u16 width, u16 height);
void GXSetDispCopySrc_cached(u16 left, u16 top, u16 width, u16 height)
{
    GXSetDispCopySrc(left, top, width, height);
}

void VIConfigure(GXRenderModeObj *rm)
{
    current_render_mode = rm;
}

u32 VIGetTvFormat(void)
{
    return 0; /* VI_NTSC */
}

u32 VIGetDTVStatus(void)
{
    return 0; /* no component cable -> no progressive prompt */
}

/* ---- per-frame hooks ---- */

/* gameplay telemetry view of decomp's struct Ball (include/ball.h) */
struct TelemBall { u32 pad0; float px, py, pz; float prevx, prevy, prevz; float vx, vy, vz; };
extern struct TelemBall balls[];

void gp_wait(void)
{
    /* GC: spin until the graphics processor drains its FIFO.  No-op here. */
}

/* ---- HFR in-between frames (docs/hfr.md) ----
 * Called once per tick after the frame was drawn, before the 60 Hz wait.
 * tick0 is when the previous tick's frame went on screen, next_ns when this
 * tick's frame will (gx_start_new_frame presents it).  The frames between
 * show the recorded frames at alpha = (present time - tick0) / period.
 * The game never waits for them: a frame is only started when it and the
 * tick frame's redraw still fit before next_ns, and g_hfr_kcap (the most
 * in-between frames per tick) drops whenever a tick still ran late (GPU
 * backpressure the estimates missed) and recovers slowly. */
extern int  hfr_active(void);
extern int  hfr_tick_valid(void);
extern void hfr_tick_end(void);
extern void hfr_render(float alpha);
extern int  hfr_final_pending(void);
extern double hfr_gpu_frame_est_ms(void);
extern int  g_hfr_cur_fps;
extern int  g_hfr_kcap;
extern int  pcs_effective_fps(void);
extern void platform_present_subframe(int j);
extern long g_dump_from_arg;
extern int g_pcs_vsync_on(void);

static int s_hfr_presented;     /* in-between frames of this tick */
static int s_hfr_skip;          /* no in-between frames this tick (F1 overlay ran) */

static void hfr_subframes(unsigned long long next_ns, unsigned long long period_ns)
{
    extern unsigned long long platform_get_time_ns(void);
    extern void platform_wait_until_ns(unsigned long long target_ns);
    static double est_render = 1000000.0;   /* replay CPU time, running average (ns) */
    extern double g_hfr_present_est_ns;     /* present call time, every present (gx_core) */
    unsigned long long tick0 = next_ns - period_ns, now, t0, slot;
    int fps = pcs_effective_fps(), presented = 0, j;
    int vsync = g_pcs_vsync_on();
    /* frame dumps: render every in-between frame at the exact j/k phases
     * regardless of time (the dump itself takes longer than a slot) */
    int fixed = g_dump_from_arg > 0 || getenv("SMB2_HFR_FIXED") != NULL;

    g_hfr_cur_fps = fps;
    s_hfr_presented = 0;
    if (fps == 60 || !hfr_tick_valid() || s_hfr_skip) {
        s_hfr_skip = 0;
        if (hfr_final_pending()) hfr_render(1.0f);
        return;
    }
    if (fixed) {
        int k = fps > 0 ? (fps + 30) / 60 : 4;
        if (k < 2) k = 2;
        for (j = 1; j < k; j++) {
            hfr_render((float)j / (float)k);
            platform_present_subframe(j);
            presented++;
        }
    } else {
        /* Phases on the target's grid (tick0 + j/fps; exact j/k for
         * multiples of 60).  A phase the slow tick already passed is not
         * dropped: the frame goes out as soon as it is drawn, at the alpha
         * of its real present time, if it still fits before the tick
         * frame (which needs one more replay).  Uncapped: back to back.
         * With V-Sync the present waits for the display (pacing, not
         * cost), so only the replay counts; without, both do. */
        unsigned long long sub = fps > 0 ? 1000000000ULL / (unsigned long long)fps : 0;
        /* GPU timeline (D3D12 timestamps; 0 = unknown, GL): a frame's GPU
         * work starts when it is presented and the GPU is free; the tick
         * frame presented at tick0 keeps it busy until tick0 + gpu.  An
         * in-between frame must be finished on the GPU, and the tick
         * frame's replay done on the CPU, before next_ns. */
        double gpu = hfr_gpu_frame_est_ms() * 1e6;
        double gbusy = (double)tick0 + gpu;
        slot = tick0 + sub;
        for (j = 1; j <= g_hfr_kcap && j < 64; j++) {
            unsigned long long tp;
            double cpu = est_render + (vsync ? 0.0 : g_hfr_present_est_ns);
            double gstart, gend;
            float alpha;
            now = platform_get_time_ns();
            while (sub && slot + sub <= now)   /* skip whole missed phases */
                slot += sub;
            tp = (double)now + cpu > (double)slot ? now + (unsigned long long)cpu : slot;
            gstart = (double)tp > gbusy ? (double)tp : gbusy;
            gend = gstart + gpu;
            if (gend + 1000000.0 > (double)next_ns || (double)tp + cpu + 1000000.0 > (double)next_ns)
                break;                       /* leave the tick frame its slot */
            /* shown when its GPU work is done: phase from the GPU start */
            alpha = (float)((gstart - (double)tick0) / (double)period_ns);
            if (fps > 0 && fps % 60 == 0 && tp == slot && gstart == (double)tp) {
                int k = fps / 60;            /* exact phase j/k */
                alpha = (float)((slot - tick0 + sub / 2) / sub) / (float)k;
            }
            if (alpha > 1.0f) alpha = 1.0f;
            t0 = now;
            hfr_render(alpha);
            now = platform_get_time_ns();
            est_render = est_render * 0.9 + (double)(now - t0) * 0.1;
            if (sub && now < tp)
                platform_wait_until_ns(tp);

            platform_present_subframe(j);
            now = platform_get_time_ns();
            if (j == 1) {
                /* latency statistic: tick start (input already read) to
                 * the first present showing this tick; 60 fps: next_ns */
                extern double g_hfr_first_ms_acc; extern long g_hfr_first_n;
                g_hfr_first_ms_acc += (double)(now - tick0) / 1e6;
                g_hfr_first_n++;
            }
            presented++;
            slot = (tp > slot ? tp : slot) + sub;
            gbusy = gend;
        }
    }
    s_hfr_presented = presented;
    /* the tick frame itself, presented at next_ns: drawn again after
     * in-between frames, and always with deferred recording */
    if (presented || hfr_final_pending()) {
        unsigned long long tr = platform_get_time_ns();
        hfr_render(1.0f);
        est_render = est_render * 0.9 + (double)(platform_get_time_ns() - tr) * 0.1;
    }
}

/* after the tick's wait: a tick that still ended late lowers the cap.
 * Raising it again is a probe: one more in-between frame after `wait` calm
 * ticks; a probe that makes a tick late doubles the wait (up to 8 s), a
 * long calm stretch halves it again. */
static void hfr_adapt(long long late_ns)
{
    static int calm, wait = 30, since_raise = 1 << 30;
    since_raise++;
    if (late_ns > 1500000LL && s_hfr_presented == 0 && hfr_tick_valid()) {
        /* late with no in-between frame at all: recording + matching +
         * the replay cost more than this machine has; plain 60 fps for 2 s */
        extern void hfr_degrade(int ticks);
        hfr_degrade(120);
        calm = 0;
    } else if (late_ns > 2500000LL && s_hfr_presented > 0) {
        g_hfr_kcap = s_hfr_presented - 1;
        if (since_raise < 60 && wait < 480) wait *= 2;   /* the last probe failed */
        calm = 0;
    } else if (++calm >= wait && g_hfr_kcap < 63) {
        g_hfr_kcap++;
        since_raise = 0;
        calm = 0;
        if (wait > 30 && since_raise == 0 && g_hfr_kcap > s_hfr_presented + 1)
            wait /= 2;   /* the cap is not what limits: probing is free */
    }
}

void video_finish_frame(void)
{
    static unsigned frame_no;

    {
        /* HFR: a main mode change is a camera cut; then the tick's frame
         * stream is complete (both no-ops when off) */
        extern s32 main_mode;
        extern void pc_hfr_cut(int reason);
        static s32 last_main_mode = -1;
        /* test hook: SMB2_HFR_SWITCH="tick:fps,tick:fps" changes the frame
         * rate setting at those ticks (live switching, docs/hfr.md) */
        {
            static const char* sw = (const char*)1;
            extern int pcs_debug_set_fps(int fps);
            if (sw == (const char*)1) sw = getenv("SMB2_HFR_SWITCH");
            if (sw) {
                const char* q = sw;
                while (*q) {
                    char* e;
                    long t = strtol(q, &e, 10);
                    if (*e != ':') break;
                    if ((unsigned)t == frame_no) pcs_debug_set_fps((int)strtol(e + 1, &e, 10));
                    else strtol(e + 1, &e, 10);
                    q = *e == ',' ? e + 1 : e;
                    if (!*e) break;
                }
            }
        }
        if (main_mode != last_main_mode) {
            last_main_mode = main_mode;
            pc_hfr_cut(2);
        }
        hfr_tick_end();
    }

    {
        /* audio: deferred sound DVD callbacks + 5 ms synth ticks of game time */
        extern void musyx_frame(void);
        PERF_BEGIN(ta);
        musyx_frame();
        PERF_END(ta, PERF_T_AUDIO);
    }

#ifdef _WIN32
    {
        extern volatile long g_shim_frame_pulse;
        g_shim_frame_pulse++;
    }
#endif
    /* SMB2_BALL_LOG=1: ball position / velocity every gameplay frame
     * (stick tests, docs/stick.md) */
    {
        static int ball_log = -1;
        extern s32 main_mode;
        if (ball_log < 0) ball_log = getenv("SMB2_BALL_LOG") && atoi(getenv("SMB2_BALL_LOG"));
        if (ball_log && main_mode == 2) {
            struct TelemBall *b = &balls[0];
            printf("[ballf] %u pos %.4f %.4f %.4f vel %.5f %.5f %.5f\n",
                   frame_no, b->px, b->py, b->pz, b->vx, b->vy, b->vz);
        }
    }
    /* test hook: SMB2_PC_ROLL=1 switches a game in its READY phase to the staff
     * roll (SMD_GAME_READY_MAIN -> SMD_GAME_ROLL_INIT), like a Gecko code on Dolphin */
    {
        static int roll = -1;
        extern s32 main_mode, sub_mode, sub_mode_request;
        if (roll < 0) roll = getenv("SMB2_PC_ROLL") && atoi(getenv("SMB2_PC_ROLL")) > 0;
        if (roll && main_mode == 2 && sub_mode == 0x31)
            sub_mode_request = 0x53;
    }
    /* once-a-second heartbeat so logs show where the game is */
    {
        extern s32 main_mode, sub_mode, main_mode_request, sub_mode_request;
        if (frame_no % 60 == 0) {
            printf("[frame %u] mode %d/%d (req %d/%d)\n",
                   frame_no, main_mode, sub_mode, main_mode_request, sub_mode_request);
            if (main_mode == 2) {
                /* gameplay telemetry for scripted-input aiming */
                struct TelemGoal { float px, py, pz; };
                extern void *stagedef;
                struct TelemBall *b = &balls[0];
                printf("[ball] pos %.2f %.2f %.2f vel %.3f %.3f %.3f\n",
                       b->px, b->py, b->pz, b->vx, b->vy, b->vz);
                if (stagedef) {
                    u8 *sd = (u8 *)stagedef;
                    s32 gc = *(s32 *)(sd + 0x18);
                    /* goals is a 32-bit GC-layout pointer (StageDef DPTR):
                     * read 4 bytes, not a native pointer, on x64 */
                    struct TelemGoal *g = (struct TelemGoal *)(uintptr_t)*(u32 *)(sd + 0x1C);
                    if (gc > 0 && g)
                        printf("[goal] n=%d pos %.2f %.2f %.2f\n", (int)gc, g->px, g->py, g->pz);
                }
            }
            fflush(stdout);
        }
    }

    input_poll_gc_pads();
    platform_poll_events();
    {
        /* window closed (SDL_QUIT) or Quit to Desktop chosen in a menu:
         * pc_quit.c exits once no memory card job is running */
        extern void pc_quit_request(const char *reason);
        extern void pc_quit_poll(void);
        if (platform_should_close())
            pc_quit_request("window closed");
        pc_quit_poll();
    }
    {
        /* F1: PC settings overlay (pc_settings_menu.c), modal over the
         * finished-but-unpresented frame */
        extern int pc_settings_overlay_take_request(void);
        extern void pc_settings_overlay_run(void);
        extern int g_gx_presents_frames;
        if (pc_settings_overlay_take_request() && g_gx_presents_frames) {
            pc_settings_overlay_run();
            s_hfr_skip = 1;   /* the overlay presented its own frames */
        }
        /* Controller Sticks screen requested by the Options page */
        extern int pc_stick_screen_take_request(void);
        extern void pc_stick_screen_run(int captured);
        if (pc_stick_screen_take_request() && g_gx_presents_frames) {
            pc_stick_screen_run(0);
            s_hfr_skip = 1;
        }
        /* updater (pc_update_ui.c): results of the background check, the
         * release notes box (requested at a safe moment by pc_quit.c's
         * pc_menu_quit_tick) and the Updates screen (Options page) */
        extern void pc_update_poll(void);
        extern int pc_update_ui_run_requested(void);
        pc_update_poll();
        if (g_gx_presents_frames && pc_update_ui_run_requested())
            s_hfr_skip = 1;
        /* Memory Card screen (Options page) / drop-in import notice */
        extern int pc_memcard_ui_run_requested(void);
        if (g_gx_presents_frames && pc_memcard_ui_run_requested())
            s_hfr_skip = 1;
    }
    /* gx_start_new_frame (GXCopyDisp) presents on PC once the game's render
     * loop is running; swap here only during early boot before that. */
    {
        extern int g_gx_presents_frames;
        if (!g_gx_presents_frames)
            platform_swap_buffers();
    }

    /* Pace to 60 Hz on the nanosecond clock: SDL_Delay(ms) can overshoot by
     * a whole scheduler tick (15.6 ms when Windows ignores timeBeginPeriod). */
    {
        extern unsigned long long platform_get_time_ns(void);
        extern void platform_wait_until_ns(unsigned long long target_ns);
        static unsigned long long next_ns;
        const unsigned long long period_ns = 16666667ULL;
        unsigned long long now_ns = platform_get_time_ns();
        frame_no++;
        if (next_ns == 0)
            next_ns = now_ns;
        next_ns += period_ns;
        if (hfr_active()) {
            hfr_subframes(next_ns, period_ns);
            now_ns = platform_get_time_ns();
            hfr_adapt((long long)(now_ns - next_ns));
        }
        if (now_ns < next_ns) {
            PERF_BEGIN(tp);
            platform_wait_until_ns(next_ns);
            PERF_END(tp, PERF_T_PACE);
        }
        else if (now_ns > next_ns + 100000000ULL)
            next_ns = now_ns; /* fell behind; don't try to catch up */
    }
}

/* The pc_gx_* write-gather pipe sinks live in render_opengl.c: they must be
 * compiled WITHOUT the decomp's gx.h in scope, because there GXPosition3f32
 * etc. are static inlines that call pc_gx_* (self-recursion otherwise). */

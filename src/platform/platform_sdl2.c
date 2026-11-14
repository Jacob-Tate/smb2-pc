/* SDL2 Platform Backend */
#include "platform.h"
#include "iso.h"
#include "perf.h"
#include "pc_settings.h"
#include "input.h"
#include "render.h"
#include "../render/gx_gpu.h"
#include "gamedata.h"
#include "disc_icon.h"
#include "install_mode.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_syswm.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>

typedef struct {
    SDL_Window* window;
    SDL_GLContext gl_context;
    bool initialized;
    bool should_close;
    bool opengl;        /* window created for the GL backend */
    uint64_t start_ticks;
} SDL2BackendData;

static const PlatformConfig* s_cfg;
static PlatformConfig s_cfg_copy;

/* Create the window (and for GL the context).  A D3D12 window is a plain
 * window: the backend gets its HWND through platform_sdl_window(). */
static bool sdl2_create_window(SDL2BackendData* data, const PlatformConfig* config, bool opengl) {
    uint32_t window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    if (opengl) {
        /* OpenGL attributes */
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

        if (config->msaa_samples > 1) {
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, config->msaa_samples);
        }
        window_flags |= SDL_WINDOW_OPENGL;
    }
    if (config->fullscreen) {
        window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

    data->window = SDL_CreateWindow(
        config->window_title ? config->window_title : "SMB2",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        config->window_width, config->window_height,
        window_flags
    );

    printf("DEBUG: SDL_CreateWindow done (%s)\n", opengl ? "OpenGL" : "D3D12");
    fflush(stdout);

    if (!data->window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }
    data->opengl = opengl;
    if (!opengl) return true;

    data->gl_context = SDL_GL_CreateContext(data->window);
    printf("DEBUG: SDL_GL_CreateContext done\n");
    fflush(stdout);

    if (!data->gl_context) {
        fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(data->window);
        data->window = NULL;
        return false;
    }

    if (config->vsync) {
        SDL_GL_SetSwapInterval(1);
    } else {
        SDL_GL_SetSwapInterval(0);
    }

    printf("DEBUG: SDL_GL_SetSwapInterval done\n");
    fflush(stdout);
    return true;
}

static bool sdl2_init(PlatformContext* ctx, const PlatformConfig* config) {
    printf("DEBUG: sdl2_init started\n");
    fflush(stdout);
    SDL2BackendData* data = calloc(1, sizeof(SDL2BackendData));
    if (!data) return false;
    s_cfg_copy = *config;
    s_cfg = &s_cfg_copy;

    /* Declare per-monitor DPI awareness before the video subsystem starts.
     * Without it, Windows display scaling (e.g. 150% on a 3840x2160 screen)
     * makes the desktop look like 2560x1440 to us: the game rendered at the
     * scaled size and Windows stretched it back up (blurry, never real 4K).
     * SDL_WINDOW_ALLOW_HIGHDPI alone does nothing on Windows. */
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");

    printf("DEBUG: SDL_Init...\n");
    fflush(stdout);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        free(data);
        return false;
    }
    printf("DEBUG: SDL_Init done\n");
    fflush(stdout);

    if (!sdl2_create_window(data, config, config->opengl)) {
        SDL_Quit();
        free(data);
        return false;
    }

    /* Initialize Glad or similar for OpenGL function loading */
    /* For now, assume core profile functions are available */

    printf("DEBUG: About to init start_ticks\n");
    fflush(stdout);
    data->start_ticks = SDL_GetPerformanceCounter();
    printf("DEBUG: start_ticks done\n");
    fflush(stdout);
    data->initialized = true;
    ctx->backend_data = data;

    printf("SDL2 platform initialized: %dx%d %s\n",
           config->window_width, config->window_height,
           config->fullscreen ? "(fullscreen)" : "");

    return true;
}

static void sdl2_shutdown(PlatformContext* ctx) {
    SDL2BackendData* data = (SDL2BackendData*)ctx->backend_data;
    if (!data) return;

    if (data->gl_context) {
        SDL_GL_DeleteContext(data->gl_context);
    }
    if (data->window) {
        SDL_DestroyWindow(data->window);
    }
    SDL_Quit();
    free(data);
    ctx->backend_data = NULL;
}

static void sdl2_poll_events(PlatformContext* ctx) {
    SDL2BackendData* data = (SDL2BackendData*)ctx->backend_data;
    if (!data) return;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                data->should_close = true;
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    pcs_on_window_resized();   /* renderer follows the drawable */
                break;
            case SDL_KEYDOWN:
                /* PC port hotkeys (game buttons are read from the keyboard
                 * state in input_poll_gc_pads) */
                if (!event.key.repeat && event.key.keysym.sym == SDLK_F1)
                    pc_settings_overlay_request();
                else if (!event.key.repeat && event.key.keysym.sym == SDLK_RETURN &&
                         (event.key.keysym.mod & KMOD_ALT))
                    pcs_toggle_fullscreen();
                break;
            case SDL_KEYUP:
                break;
            /* Pad hot-plugging (input_sdl2.c).  Pad state itself is polled
             * once per frame in input_poll_gc_pads.  ADDED carries the device
             * index, REMOVED the instance id; a mapped controller sends both
             * the JOY and the CONTROLLER event, which the input code dedupes. */
            case SDL_CONTROLLERDEVICEADDED:
                input_on_device_added(event.cdevice.which);
                break;
            case SDL_JOYDEVICEADDED:
                input_on_device_added(event.jdevice.which);
                break;
            case SDL_CONTROLLERDEVICEREMOVED:
                input_on_device_removed(event.cdevice.which);
                break;
            case SDL_JOYDEVICEREMOVED:
                input_on_device_removed(event.jdevice.which);
                break;
        }
    }
}

static bool sdl2_should_close(PlatformContext* ctx) {
    SDL2BackendData* data = (SDL2BackendData*)ctx->backend_data;
    return data ? data->should_close : true;
}


/* ---- --perf instrumentation (see include/perf.h) ---- */
int g_perf_on;
double g_perf_t[PERF_T_COUNT];
long long g_perf_c[PERF_C_COUNT];
static double s_perf_inv_freq;

uint64_t perf_now(void) { return SDL_GetPerformanceCounter(); }
double perf_ms(uint64_t a, uint64_t b) {
    if (s_perf_inv_freq == 0) s_perf_inv_freq = 1000.0 / (double)SDL_GetPerformanceFrequency();
    return (double)(b - a) * s_perf_inv_freq;
}

/* SMB2_PERF_FRAMES=<file> (with --perf): one line per presented frame
 * (frame, frame ms, pace ms, swap ms, texcreate ms, copytex ms, audio ms,
 * game mode) plus process private bytes / working set every 60 frames, for
 * p50/p99/max analysis of hitches (tools/perf_frames.py). */
#ifdef _WIN32
#define PSAPI_VERSION 2
#include <windows.h>
#include <psapi.h>
#endif
extern long g_dbg_frame_counter;
static void perf_frame_log(double ft) {
    static FILE* f;
    static int opened;
    static long n;
    extern int main_mode, sub_mode;
    if (!opened) {
        const char* p = getenv("SMB2_PERF_FRAMES");
        opened = 1;
        if (p && p[0]) f = fopen(p, "w");
        if (f) fprintf(f, "# frame ms pace swap texcreate copytex audio mode sub\n");
    }
    if (!f) return;
    n++;
    fprintf(f, "%ld %.3f %.3f %.3f %.3f %.3f %.3f %d %d\n", g_dbg_frame_counter, ft,
            g_perf_t[PERF_T_PACE], g_perf_t[PERF_T_SWAP], g_perf_t[PERF_T_TEXCREATE],
            g_perf_t[PERF_T_COPYTEX], g_perf_t[PERF_T_AUDIO], main_mode, sub_mode);
#ifdef _WIN32
    if (n % 60 == 0) {
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof pmc))
        {
            extern unsigned long long gx_gpu_mem_usage(void);
            extern long long g_tex_created, g_tex_created_texels, g_tex_alive, g_tex_alive_texels;
            fprintf(f, "#mem %ld private %llu ws %llu gpu %llu textures %lld texels %lld "
                    "alive %lld alive_texels %lld\n",
                    g_dbg_frame_counter, (unsigned long long)pmc.PrivateUsage,
                    (unsigned long long)pmc.WorkingSetSize, gx_gpu_mem_usage(),
                    g_tex_created, g_tex_created_texels, g_tex_alive, g_tex_alive_texels);
        }
    }
#endif
    {
        extern unsigned audio_underrun_count(void);
        static unsigned last_u;
        unsigned u = audio_underrun_count();
        if (u != last_u) fprintf(f, "#underrun %ld total %u\n", g_dbg_frame_counter, u);
        last_u = u;
    }
    if (n % 60 == 0) fflush(f);
}

void perf_frame_end(void) {
    static uint64_t last, win_start;
    static double sum_t[PERF_T_COUNT], max_frame, sum_frame;
    static long long sum_c[PERF_C_COUNT];
    static long frames, total_frames;
    static long over_17, over_25;
    uint64_t now;
    double ft;
    int i;
    if (!g_perf_on) return;
    now = perf_now();
    if (!last) { last = win_start = now; memset(g_perf_t, 0, sizeof g_perf_t);
                 memset(g_perf_c, 0, sizeof g_perf_c); return; }
    ft = perf_ms(last, now);
    last = now;
    perf_frame_log(ft);
    for (i = 0; i < PERF_T_COUNT; i++) { sum_t[i] += g_perf_t[i]; g_perf_t[i] = 0; }
    for (i = 0; i < PERF_C_COUNT; i++) { sum_c[i] += g_perf_c[i]; g_perf_c[i] = 0; }
    sum_frame += ft;
    if (ft > max_frame) max_frame = ft;
    if (ft > 17.5) over_17++;
    if (ft > 25.0) over_25++;
    frames++;
    total_frames++;
    if (frames >= PERF_REPORT_FRAMES) {
        double n = (double)frames, wall = perf_ms(win_start, now);
        double other = sum_frame - sum_t[PERF_T_SWAP] - sum_t[PERF_T_PACE] -
                       sum_t[PERF_T_FLUSH] - sum_t[PERF_T_COPYTEX] -
                       sum_t[PERF_T_TEXINIT] - sum_t[PERF_T_AUDIO];
        fprintf(stderr,
            "[perf] f%ld %.1ffps frame %.2fms (max %.1f, >17.5ms %ld, >25ms %ld) | "
            "game+cpu %.2f flush %.2f copytex %.2f texinit %.2f (create %.2f) audio %.2f "
            "pace %.2f swap %.2f | per frame: draws %.0f verts %.0f texup %.2f "
            "copytex %.2f readpix %.2f hashKB %.1f texinit %.1f\n",
            total_frames, n * 1000.0 / wall, sum_frame / n, max_frame, over_17, over_25,
            other / n, sum_t[PERF_T_FLUSH] / n, sum_t[PERF_T_COPYTEX] / n,
            sum_t[PERF_T_TEXINIT] / n, sum_t[PERF_T_TEXCREATE] / n, sum_t[PERF_T_AUDIO] / n,
            sum_t[PERF_T_PACE] / n, sum_t[PERF_T_SWAP] / n,
            sum_c[PERF_C_DRAWS] / n, sum_c[PERF_C_VERTS] / n, sum_c[PERF_C_TEXUPLOAD] / n,
            sum_c[PERF_C_COPYTEX] / n, sum_c[PERF_C_READPIX] / n,
            sum_c[PERF_C_HASHBYTES] / n / 1024.0, sum_c[PERF_C_TEXINIT] / n);
        memset(sum_t, 0, sizeof sum_t);
        memset(sum_c, 0, sizeof sum_c);
        sum_frame = max_frame = 0;
        frames = over_17 = over_25 = 0;
        win_start = now;
    }
}

static uint64_t sdl2_get_time_ns(PlatformContext* ctx) {
    SDL2BackendData* data = (SDL2BackendData*)ctx->backend_data;
    if (!data) return 0;

    uint64_t now = SDL_GetPerformanceCounter();
    uint64_t freq = SDL_GetPerformanceFrequency();
    return ((now - data->start_ticks) * 1000000000ULL) / freq;
}

static uint64_t sdl2_get_ticks_ms(PlatformContext* ctx) {
    return sdl2_get_time_ns(ctx) / 1000000ULL;
}

static void sdl2_sleep_ms(uint32_t ms) {
    SDL_Delay(ms);
}

/* Periodic framebuffer dump: lets a headless log confirm whether anything
 * is actually being rendered (non-black pixel count) and leaves PPM
 * snapshots next to the exe for visual inspection.  The renderer returns
 * the finished frame (before present) as top-down RGB. */
static void sdl2_dump_frame(SDL_Window *win, long frame) {
    unsigned char *buf = NULL;
    int w = 0, h = 0, y;
    long lit = 0, total;
    char name[64];
    FILE *f;

    (void)win;
    if (!render_read_frame(&buf, &w, &h) || !buf) return;
    total = (long)w * h;
    for (y = 0; y < total; y++)
        if (buf[y*3] | buf[y*3+1] | buf[y*3+2]) lit++;
    {
        extern long g_gl_draw_calls, g_gl_draw_verts;
        extern long g_mtx_trace, g_glyph_trace;
        extern void render_debug_dump_matrices(void);
        render_debug_dump_matrices();
        extern long g_dump_from_arg;
        if (!g_dump_from_arg) { /* not for --dump-from runs (every frame) */
            g_mtx_trace = 300; /* trace matrix loads + flushes for the next frame */
            g_glyph_trace = 120; /* trace font-glyph binds/flushes separately */
        }
        fprintf(stderr, "[framedump] frame %ld: %ldx%d, %ld/%ld non-black px, "
                "%ld draws / %ld verts so far\n",
                frame, (long)w, h, lit, total, g_gl_draw_calls, g_gl_draw_verts);
    }

    snprintf(name, sizeof name, "frame_%05ld.ppm", frame);
    f = fopen(name, "wb");
    if (f) {
        fprintf(f, "P6\n%d %d\n255\n", w, h);
        fwrite(buf, 3, (size_t)w * h, f);
        fclose(f);
    }
}

/* --dump-from N: also dump every frame from N on (HFR evidence) */
long g_dump_from_arg;
/* --dump-ranges a-b,c-d: dump every frame in these tick ranges (and their
 * HFR in-between frames); also sets g_dump_from_arg so HFR uses fixed phases */
static long s_dump_ranges[32][2];
static int s_ndump_ranges;
void platform_set_dump_ranges(const char* spec) {
    const char* p = spec;
    while (*p && s_ndump_ranges < 32) {
        char* e;
        long a = strtol(p, &e, 10), b = a;
        if (e == p) break;
        if (*e == '-') b = strtol(e + 1, &e, 10);
        s_dump_ranges[s_ndump_ranges][0] = a;
        s_dump_ranges[s_ndump_ranges][1] = b;
        s_ndump_ranges++;
        p = *e == ',' ? e + 1 : e;
        if (!*e) break;
    }
    if (s_ndump_ranges && !g_dump_from_arg) g_dump_from_arg = 1L << 30;
}
static int sdl2_dump_due(long interval, long frame) {
    int i;
    for (i = 0; i < s_ndump_ranges; i++)
        if (frame >= s_dump_ranges[i][0] && frame <= s_dump_ranges[i][1]) return 1;
    if (g_dump_from_arg > 0 && frame >= g_dump_from_arg) return 1;
    return interval > 0 && (frame == 120 || frame % interval == 0);
}

long g_dbg_frame_counter;
static long s_dump_interval_cached = -1;

/* HFR in-between frame (docs/hfr.md): present without advancing the tick
 * frame counter (--exit-frame, dump numbering and traces stay per tick).
 * When tick frame T+1 (the one being interpolated toward) is due for a
 * dump, in-between frame j is written as frame_<T+1>_<j>.ppm. */
void platform_present_subframe(int j) {
    SDL2BackendData* data = (SDL2BackendData*)g_platform.backend_data;
    if (!data || !data->window) return;
    if (s_dump_interval_cached > 0 || g_dump_from_arg > 0) {
        long f = g_dbg_frame_counter + 1;
        if (sdl2_dump_due(s_dump_interval_cached, f)) {
            unsigned char *buf = NULL;
            int w = 0, h = 0;
            if (render_read_frame(&buf, &w, &h) && buf) {
                char name[64];
                FILE *fp;
                snprintf(name, sizeof name, "frame_%05ld_%d.ppm", f, j);
                fp = fopen(name, "wb");
                if (fp) {
                    fprintf(fp, "P6\n%d %d\n255\n", w, h);
                    fwrite(buf, 3, (size_t)w * h, fp);
                    fclose(fp);
                }
            }
        }
    }
    {
        PERF_BEGIN(t0);
        render_present();
        PERF_END(t0, PERF_T_SWAP);
        perf_frame_end();
    }
}

/* refresh rate of the window's display (0 = unknown) */
int platform_display_refresh_hz(void) {
    SDL2BackendData* data = (SDL2BackendData*)g_platform.backend_data;
    SDL_DisplayMode m;
    if (!data || !data->window) return 0;
    if (SDL_GetWindowDisplayMode(data->window, &m) == 0 && m.refresh_rate > 0)
        return m.refresh_rate;
    if (SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(data->window), &m) == 0)
        return m.refresh_rate;
    return 0;
}

static void sdl2_swap_buffers(PlatformContext* ctx) {
    SDL2BackendData* data = (SDL2BackendData*)ctx->backend_data;
    if (data && data->window) {
        static long s_frame;
        static long s_dump_interval = -1;
        extern long g_mtx_trace;
        if (s_dump_interval < 0) {
            extern long g_dump_interval_arg;
            const char *e = getenv("SMB2_DUMP_INTERVAL");
            long v = g_dump_interval_arg > 0 ? g_dump_interval_arg
                                             : (e ? atol(e) : 0);
            s_dump_interval = v > 0 ? v : 0;  /* dumps only on request */
            s_dump_interval_cached = s_dump_interval;
        }
        s_frame++;
        g_dbg_frame_counter = s_frame;
        {
            extern long g_exit_frame_arg;
            if (g_exit_frame_arg > 0 && s_frame >= g_exit_frame_arg) {
                printf("[exit-frame] reached frame %ld - exiting.\n", s_frame);
                fflush(NULL);
                exit(0);
            }
        }
        {
            /* --trace-frame N: trace every GL flush in frame N (the frame
             * whose framedump is frame_N.ppm, i.e. draws after swap N-1). */
            extern long g_trace_frame_arg;
            if (g_trace_frame_arg > 0 && s_frame + 1 == g_trace_frame_arg)
                g_mtx_trace = 400000;
            else if (g_trace_frame_arg > 0 && s_frame == g_trace_frame_arg)
                g_mtx_trace = 0; /* stop after the one frame */
        }
        if (sdl2_dump_due(s_dump_interval, s_frame))
            sdl2_dump_frame(data->window, s_frame);
        else if (g_mtx_trace > 0)
            fprintf(stderr, "[trace] ---- swap (frame %ld) ----\n", s_frame);
        {
            PERF_BEGIN(t0);
            render_present();
            PERF_END(t0, PERF_T_SWAP);
            perf_frame_end();
        }
    }
}

static void sdl2_set_window_title(PlatformContext* ctx, const char* title) {
    SDL2BackendData* data = (SDL2BackendData*)ctx->backend_data;
    if (data && data->window) {
        SDL_SetWindowTitle(data->window, title);
    }
}

static void sdl2_get_window_size(PlatformContext* ctx, int* w, int* h) {
    SDL2BackendData* data = (SDL2BackendData*)ctx->backend_data;
    if (data && data->window) {
        SDL_GetWindowSize(data->window, w, h);
    } else {
        *w = 0; *h = 0;
    }
}

static void sdl2_set_fullscreen(PlatformContext* ctx, bool fullscreen) {
    SDL2BackendData* data = (SDL2BackendData*)ctx->backend_data;
    if (data && data->window) {
        SDL_SetWindowFullscreen(data->window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    }
}

static bool sdl2_show_resolution_picker(PlatformContext* ctx, int* out_w, int* out_h, bool* out_fullscreen) {
    /* Simple console-based picker for now - could be replaced with a GUI dialog */
    printf("\n=== Resolution Picker ===\n");
    printf("Available display modes:\n");

    int num_displays = SDL_GetNumVideoDisplays();
    for (int i = 0; i < num_displays; i++) {
        int num_modes = SDL_GetNumDisplayModes(i);
        for (int m = 0; m < num_modes; m++) {
            SDL_DisplayMode mode;
            if (SDL_GetDisplayMode(i, m, &mode) == 0) {
                printf("  Display %d: %dx%d @ %dHz\n", i, mode.w, mode.h, mode.refresh_rate);
            }
        }
    }

    printf("\nEnter width (or 0 for default %d): ", *out_w);
    char input[64];
    if (fgets(input, sizeof(input), stdin)) {
        int w = atoi(input);
        if (w > 0) *out_w = w;
    }

    printf("Enter height (or 0 for default %d): ", *out_h);
    if (fgets(input, sizeof(input), stdin)) {
        int h = atoi(input);
        if (h > 0) *out_h = h;
    }

    printf("Fullscreen? (y/n) [current: %s]: ", *out_fullscreen ? "y" : "n");
    if (fgets(input, sizeof(input), stdin)) {
        *out_fullscreen = (input[0] == 'y' || input[0] == 'Y');
    }

    printf("Selected: %dx%d %s\n", *out_w, *out_h, *out_fullscreen ? "(fullscreen)" : "");
    return true;
}

/* Save file system implementation */
static char s_save_dir[1024] = {0};

static void sdl2_init_save_dir(void) {
    if (s_save_dir[0]) return;

    {   /* --save-dir <dir>: settings.dat / saves somewhere else (tests) */
        extern const char* g_save_dir_arg;
        if (g_save_dir_arg && g_save_dir_arg[0]) {
            size_t n;
            snprintf(s_save_dir, sizeof(s_save_dir) - 1, "%s", g_save_dir_arg);
            n = strlen(s_save_dir);
            if (s_save_dir[n - 1] != '/' && s_save_dir[n - 1] != '\\')
                s_save_dir[n] = '/';
            goto make_dir;
        }
    }
    {   /* installed mode (install.ini): userdata= */
        const char* ud = pc_install_userdata();
        if (ud) {
            snprintf(s_save_dir, sizeof(s_save_dir), "%s", ud);
            goto make_dir;
        }
    }

    const char* base = SDL_GetPrefPath("smb2", "smb2_pc_port");
    if (base) {
        strncpy(s_save_dir, base, sizeof(s_save_dir) - 1);
        SDL_free((void*)base);
    } else {
        /* Fallback */
        const char* home = getenv("HOME");
        if (home) {
            snprintf(s_save_dir, sizeof(s_save_dir), "%s/.local/share/smb2_pc_port/", home);
        } else {
            strcpy(s_save_dir, "./saves/");
        }
    }

make_dir:;
    /* Create directory if it doesn't exist */
    struct stat st = {0};
    if (stat(s_save_dir, &st) == -1) {
#if defined(_WIN32) || defined(__CYGWIN__)
        mkdir(s_save_dir);
#else
        mkdir(s_save_dir, 0755);
#endif
    }
}

const char* platform_get_save_dir(void) {
    sdl2_init_save_dir();
    return s_save_dir;
}

static char* sdl2_get_save_path(int slot) {
    static char path[1024];
    sdl2_init_save_dir();
    const char* slot_names[] = {"save_A.sav", "save_B.sav", "save_C.sav"};
    if (slot >= 0 && slot < 3) {
        snprintf(path, sizeof(path), "%s%s", s_save_dir, slot_names[slot]);
    } else {
        snprintf(path, sizeof(path), "%ssave_%d.sav", s_save_dir, slot);
    }
    return path;
}

bool platform_save_exists(int slot) {
    char* path = sdl2_get_save_path(slot);
    struct stat st;
    return stat(path, &st) == 0;
}

bool platform_save_read(int slot, void* buffer, size_t size) {
    char* path = sdl2_get_save_path(slot);
    FILE* f = fopen(path, "rb");
    if (!f) return false;

    size_t read = fread(buffer, 1, size, f);
    fclose(f);
    return read == size;
}

bool platform_save_write(int slot, const void* buffer, size_t size) {
    char* path = sdl2_get_save_path(slot);
    FILE* f = fopen(path, "wb");
    if (!f) return false;

    size_t written = fwrite(buffer, 1, size, f);
    fclose(f);

    /* Also write a .meta file with timestamp for backup */
    char meta_path[1024];
    snprintf(meta_path, sizeof(meta_path), "%s.meta", path);
    FILE* mf = fopen(meta_path, "wb");
    if (mf) {
        time_t now = time(NULL);
        fwrite(&now, sizeof(time_t), 1, mf);
        fwrite(&size, sizeof(size_t), 1, mf);
        fclose(mf);
    }

    return written == size;
}

bool platform_save_delete(int slot) {
    char* path = sdl2_get_save_path(slot);
    remove(path);
    char meta_path[1024];
    snprintf(meta_path, sizeof(meta_path), "%s.meta", path);
    remove(meta_path);
    return true;
}

int platform_save_list(char*** out_names) {
    sdl2_init_save_dir();

    DIR* dir = opendir(s_save_dir);
    if (!dir) return 0;

    struct dirent* entry;
    int count = 0;
    char** names = NULL;

    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".sav") == entry->d_name + strlen(entry->d_name) - 4) {
            names = realloc(names, (count + 1) * sizeof(char*));
            names[count] = strdup(entry->d_name);
            count++;
        }
    }
    closedir(dir);

    *out_names = names;
    return count;
}

/* ISO picker - uses SDL native file dialog */

bool platform_show_iso_picker(char** out_path) {
    /* SDL2 doesn't have native file dialog in 2.0, use zenity/kdialog on Linux */
    /* For cross-platform, we'd use SDL3 or a separate library */
    pc_user_msg("\n=== ISO Selector ===\n");
    pc_user_msg("Enter path to GameCube ISO/GCM file (blank to skip):\n> ");

    char input[1024];
    if (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\r\n")] = '\0';
        /* Strip surrounding quotes (Windows "Copy as path" adds them) */
        char* p = input;
        size_t len = strlen(p);
        if (len >= 2 && p[0] == '"' && p[len - 1] == '"') {
            p[len - 1] = '\0';
            p++;
        }
        if (p[0] != '\0') {
            *out_path = strdup(p);
            pc_user_msg("Selected: %s\n", *out_path);
            return true;
        }
    }
    return false;
}

/* Settings */
static GameSettings s_settings = {0};
static ISOArchive* s_iso_archive = NULL;

/* settings_get_defaults / settings_load / settings_save: src/platform/pc_settings.c */

bool platform_show_settings_menu(GameSettings* settings) {
    if (!settings) return false;

    printf("\n=== Settings ===\n");
    printf("1. Video\n");
    printf("2. Audio\n");
    printf("3. Controls\n");
    printf("4. Widescreen\n");
    printf("5. ISO/Asset\n");
    printf("0. Back\n> ");

    char input[64];
    if (fgets(input, sizeof(input), stdin)) {
        int choice = atoi(input);
        switch (choice) {
            case 1: /* Video */
                printf("Width: "); fgets(input, sizeof(input), stdin); settings->window_width = atoi(input);
                printf("Height: "); fgets(input, sizeof(input), stdin); settings->window_height = atoi(input);
                printf("Fullscreen (y/n): "); fgets(input, sizeof(input), stdin); settings->fullscreen = (input[0] == 'y');
                printf("VSync (y/n): "); fgets(input, sizeof(input), stdin); settings->vsync = (input[0] == 'y');
                printf("MSAA (0/2/4/8): "); fgets(input, sizeof(input), stdin); settings->msaa_samples = atoi(input);
                break;
            case 2: /* Audio */
                printf("Master Volume (0-1): "); fgets(input, sizeof(input), stdin); settings->master_volume = atof(input);
                printf("Music Volume (0-1): "); fgets(input, sizeof(input), stdin); settings->music_volume = atof(input);
                printf("SFX Volume (0-1): "); fgets(input, sizeof(input), stdin); settings->sfx_volume = atof(input);
                break;
            case 3: /* Controls */
                printf("Controller (0=GC, 1=Classic, 2=KB): "); fgets(input, sizeof(input), stdin); settings->controller_type = atoi(input);
                printf("Rumble (y/n): "); fgets(input, sizeof(input), stdin); settings->rumble_enabled = (input[0] == 'y');
                break;
            case 4: /* Widescreen */
                printf("Enabled (y/n): "); fgets(input, sizeof(input), stdin); settings->widescreen_enabled = (input[0] == 'y');
                printf("Aspect (1.777=16:9, 2.37=21:9): "); fgets(input, sizeof(input), stdin); settings->widescreen_aspect = atof(input);
                printf("Stretch HUD (y/n): "); fgets(input, sizeof(input), stdin); settings->stretch_hud = (input[0] == 'y');
                printf("Stretch 2D (y/n): "); fgets(input, sizeof(input), stdin); settings->stretch_2d = (input[0] == 'y');
                break;
            case 5: /* ISO */
                if (platform_show_iso_picker(&settings->iso_path)) {
                    if (s_iso_archive) {
                        iso_close(s_iso_archive);
                    }
                    s_iso_archive = iso_open(settings->iso_path);
                    if (s_iso_archive) {
                        printf("ISO loaded successfully!\n");
                    } else {
                        printf("Failed to load ISO.\n");
                    }
                }
                break;
            default:
                return false;
        }
    }
    settings_save(settings);
    return true;
}

/* Startup ISO flow: try the saved path, otherwise prompt until a valid
 * image is given or the user skips (blank input). */
bool platform_startup_iso(void) {
    settings_load(&s_settings);

    if (s_settings.iso_path) {
        s_iso_archive = iso_open(s_settings.iso_path);
        if (s_iso_archive) {
            printf("Loaded ISO from saved settings: %s\n", s_settings.iso_path);
            return true;
        }
        fprintf(stderr, "Saved ISO path could not be opened: %s\n", s_settings.iso_path);
        free(s_settings.iso_path);
        s_settings.iso_path = NULL;
    }

    while (!s_iso_archive) {
        char* path = NULL;
        if (!platform_show_iso_picker(&path)) {
            printf("No ISO selected - continuing without game assets.\n");
            return false;
        }
        s_iso_archive = iso_open(path);
        if (s_iso_archive) {
            s_settings.iso_path = path;
            settings_save(&s_settings);
        } else {
            fprintf(stderr, "Could not open '%s'.\n"
                    "If this is an RVZ, convert it first:\n"
                    "  dolphin-tool convert -i game.rvz -o game.iso -f iso\n"
                    "(or Dolphin GUI: right-click game -> Convert File -> ISO)\n", path);
            free(path);
        }
    }
    return true;
}

ISOArchive* platform_get_iso_archive(void) {
    return s_iso_archive;
}

bool platform_load_iso(const char* path) {
    if (s_iso_archive) {
        iso_close(s_iso_archive);
    }
    s_iso_archive = iso_open(path);
    if (s_iso_archive && s_settings.iso_path) {
        free(s_settings.iso_path);
    }
    if (s_iso_archive) {
        s_settings.iso_path = strdup(path);
        settings_save(&s_settings);
    }
    return s_iso_archive != NULL;
}

static const PlatformVTable sdl2_vtable = {
    .init = sdl2_init,
    .shutdown = sdl2_shutdown,
    .poll_events = sdl2_poll_events,
    .should_close = sdl2_should_close,
    .get_time_ns = sdl2_get_time_ns,
    .get_ticks_ms = sdl2_get_ticks_ms,
    .sleep_ms = sdl2_sleep_ms,
    .swap_buffers = sdl2_swap_buffers,
    .set_window_title = sdl2_set_window_title,
    .get_window_size = sdl2_get_window_size,
    .set_fullscreen = sdl2_set_fullscreen,
    .show_resolution_picker = sdl2_show_resolution_picker,
};

bool platform_init(const PlatformConfig* config) {
    g_platform.vtable = &sdl2_vtable;
    g_platform.backend_data = NULL;
    return g_platform.vtable->init(&g_platform, config);
}

void platform_shutdown(void) {
    if (g_platform.vtable && g_platform.vtable->shutdown) {
        g_platform.vtable->shutdown(&g_platform);
    }
}

void platform_poll_events(void) {
    if (g_platform.vtable && g_platform.vtable->poll_events) {
        g_platform.vtable->poll_events(&g_platform);
    }
}

bool platform_should_close(void) {
    if (g_platform.vtable && g_platform.vtable->should_close) {
        return g_platform.vtable->should_close(&g_platform);
    }
    return true;
}

uint64_t platform_get_time_ns(void) {
    if (g_platform.vtable && g_platform.vtable->get_time_ns) {
        return g_platform.vtable->get_time_ns(&g_platform);
    }
    return 0;
}

uint64_t platform_get_ticks_ms(void) {
    if (g_platform.vtable && g_platform.vtable->get_ticks_ms) {
        return g_platform.vtable->get_ticks_ms(&g_platform);
    }
    return 0;
}

void platform_sleep_ms(uint32_t ms) {
    if (g_platform.vtable && g_platform.vtable->sleep_ms) {
        g_platform.vtable->sleep_ms(ms);
    }
}

/* Block until platform_get_time_ns() >= target_ns, without the up-to-15.6ms
 * overshoot of a plain Sleep/SDL_Delay (Windows can ignore timeBeginPeriod
 * for background/occluded windows).  Uses a high-resolution waitable timer
 * when available, then spins the last ~1ms. */
#ifdef _WIN32
#include <windows.h>
#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif
#endif
void platform_wait_until_ns(uint64_t target_ns) {
    uint64_t now = platform_get_time_ns();
    const uint64_t spin_ns = 1000000ULL;
#ifdef _WIN32
    static HANDLE s_timer;
    static int s_timer_init;
    if (!s_timer_init) {
        s_timer_init = 1;
        timeBeginPeriod(1);
        s_timer = CreateWaitableTimerExW(NULL, NULL,
                                         CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                         TIMER_ALL_ACCESS);
    }
    if (now + spin_ns < target_ns) {
        uint64_t wait_ns = target_ns - now - spin_ns;
        if (s_timer) {
            LARGE_INTEGER due;
            due.QuadPart = -(LONGLONG)(wait_ns / 100ULL); /* relative, 100ns */
            if (SetWaitableTimer(s_timer, &due, 0, NULL, NULL, FALSE))
                WaitForSingleObject(s_timer, INFINITE);
        } else if (wait_ns >= 1000000ULL) {
            Sleep((DWORD)(wait_ns / 1000000ULL));
        }
    }
#else
    if (now + spin_ns < target_ns)
        SDL_Delay((uint32_t)((target_ns - now - spin_ns) / 1000000ULL));
#endif
    while (platform_get_time_ns() < target_ns)
        ;
}

void platform_swap_buffers(void) {
    if (g_platform.vtable && g_platform.vtable->swap_buffers) {
        g_platform.vtable->swap_buffers(&g_platform);
    }
}

bool platform_show_resolution_picker(int* out_width, int* out_height, bool* out_fullscreen) {
    if (g_platform.vtable && g_platform.vtable->show_resolution_picker) {
        return g_platform.vtable->show_resolution_picker(&g_platform, out_width, out_height, out_fullscreen);
    }
    return false;
}

/* The SDL window (renderers / settings need it; GL may not be current). */
void* platform_sdl_window(void) {
    SDL2BackendData* data = (SDL2BackendData*)g_platform.backend_data;
    return data ? data->window : NULL;
}

/* Drawable size in pixels of the window. */
void platform_drawable_size(int* w, int* h) {
    SDL2BackendData* data = (SDL2BackendData*)g_platform.backend_data;
    *w = *h = 0;
    if (!data || !data->window) return;
    if (data->opengl) SDL_GL_GetDrawableSize(data->window, w, h);
    else SDL_GetWindowSizeInPixels(data->window, w, h);
}

/* Replace the window with one for the other API (renderer fallback). */
bool platform_recreate_window(bool opengl) {
    SDL2BackendData* data = (SDL2BackendData*)g_platform.backend_data;
    if (!data || !s_cfg) return false;
    if (data->gl_context) { SDL_GL_DeleteContext(data->gl_context); data->gl_context = NULL; }
    if (data->window) { SDL_DestroyWindow(data->window); data->window = NULL; }
    if (!sdl2_create_window(data, s_cfg, opengl)) return false;
    platform_apply_window_icon();
    return true;
}

/* Global platform instance */
PlatformContext g_platform = {0};
/* ------------------------------------------------------------------------
 * Extracted game data (--gamedata, install.ini) and the installed-mode
 * "game data missing" recovery flow.  docs/installer.md. */

bool platform_load_gamedata(const char* dir) {
    char msg[1024];
    ISOArchive* arc;
    if (!gamedata_quick_check(dir, msg, sizeof msg)) {
        fprintf(stderr, "Game data folder not usable: %s\n", msg);
        return false;
    }
    arc = iso_open_gamedata(dir);
    if (!arc) return false;
    if (s_iso_archive) iso_close(s_iso_archive);
    s_iso_archive = arc;
    printf("Loaded game data folder: %s\n", dir);
    return true;
}

typedef struct { int last_pct; } RecoverProg;

static void recover_progress(uint64_t done, uint64_t total, void* user) {
    RecoverProg* r = (RecoverProg*)user;
    int pct = total ? (int)(done * 100 / total) : 100;
    if (pct / 5 != r->last_pct / 5 || r->last_pct < 0) {
        pc_user_msg("  extracting game data... %3d%%\n", pct);
        r->last_pct = pct;
    }
    SDL_PumpEvents();   /* keep the (still black) window responsive */
}

static void recover_box(const char* title, const char* text, bool error) {
    if (pc_stdin_interactive())
        SDL_ShowSimpleMessageBox(error ? SDL_MESSAGEBOX_ERROR : SDL_MESSAGEBOX_INFORMATION,
                                 title, text, (SDL_Window*)platform_sdl_window());
}

/* Installed mode, no usable game data: explain, pick an ISO with the usual
 * prompt, extract it into %LOCALAPPDATA%\SMB2\gamedata (per user,
 * no admin rights needed) and load that. */
bool platform_recover_gamedata(const char* why) {
    const char* dest = pc_local_gamedata_dir();
    char text[2048];
    if (!dest) {
        fprintf(stderr, "Cannot determine %%LOCALAPPDATA%% for the game data\n");
        return false;
    }
    snprintf(text, sizeof text,
             "The game data is missing or damaged:\n%s\n\n"
             "Select your Super Monkey Ball 2 (USA, GM2E8P) disc image (.iso/.gcm) in the console "
             "window. It will be extracted to\n%s\n(about 1 GB, no administrator rights needed).",
             why, dest);
    pc_user_msg("\n%s\n", text);
    if (pc_stdin_interactive()) {
        const SDL_MessageBoxButtonData buttons[2] = {
            { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Select ISO..." },
            { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Quit" },
        };
        const SDL_MessageBoxData box = {
            SDL_MESSAGEBOX_WARNING, (SDL_Window*)platform_sdl_window(),
            "SMB2 - game data missing", text, 2, buttons, NULL };
        int which = 0;
        if (SDL_ShowMessageBox(&box, &which) == 0 && which != 1) return false;
    }
    for (;;) {
        char* path = NULL;
        char msg[1024];
        RecoverProg rp = { -1 };
        int rc;
        if (!platform_show_iso_picker(&path)) return false;
        pc_user_msg("Extracting %s\n  to %s ...\n", path, dest);
        rc = gamedata_extract(path, dest, recover_progress, &rp, msg, sizeof msg);
        free(path);
        if (rc == GD_OK) {
            pc_user_msg("Game data extracted (%s).\n", msg);
            return platform_load_gamedata(dest);
        }
        pc_user_msg("Extraction failed (error %d): %s\nTry another file, or leave it blank to quit.\n",
                    rc, msg);
        snprintf(text, sizeof text, "Extraction failed:\n%s", msg);
        recover_box("SMB2", text, true);
    }
}

/* Game data search order (docs/installer.md): --gamedata, --iso, install.ini
 * gamedata, (installed) the per-user re-extracted copy, the settings.dat ISO
 * path, the ISO prompt.  Portable runs without --gamedata take exactly the
 * old --iso -> settings -> prompt path. */
bool platform_resolve_game_data(const char* gamedata_arg, const char* iso_arg) {
    char msg[1024];
    const char* why = "no game data folder is configured";
    if (gamedata_arg) {
        if (platform_load_gamedata(gamedata_arg)) return true;
        fprintf(stderr, "Could not use --gamedata folder: %s\n", gamedata_arg);
    }
    if (iso_arg) {
        if (platform_load_iso(iso_arg)) return true;
        fprintf(stderr, "Could not open --iso path: %s\n", iso_arg);
    }
    if (!pc_installed())
        return platform_startup_iso();

    if (pc_install_gamedata()) {
        if (gamedata_quick_check(pc_install_gamedata(), msg, sizeof msg) &&
            platform_load_gamedata(pc_install_gamedata()))
            return true;
        fprintf(stderr, "[install] gamedata %s: %s\n", pc_install_gamedata(), msg);
        why = msg;
    }
    {
        const char* local = pc_local_gamedata_dir();
        char lmsg[1024];
        if (local && gamedata_quick_check(local, lmsg, sizeof lmsg) &&
            platform_load_gamedata(local))
            return true;
    }
    settings_load(&s_settings);
    if (s_settings.iso_path) {
        s_iso_archive = iso_open(s_settings.iso_path);
        if (s_iso_archive) {
            printf("Loaded ISO from saved settings: %s\n", s_settings.iso_path);
            return true;
        }
        fprintf(stderr, "Saved ISO path could not be opened: %s\n", s_settings.iso_path);
    }
    if (platform_recover_gamedata(why)) return true;
    pc_user_msg("No game data - exiting.\n");
    return false;
}

/* Window/taskbar icon from the disc in use (generated, never shipped). */
void platform_apply_window_icon(void) {
    static uint8_t px[32 * 32 * 4];
    static int have = -1;
    SDL_Window* win = (SDL_Window*)platform_sdl_window();
    if (!win || !s_iso_archive) return;
    if (have < 0) have = disc_icon_from_archive(s_iso_archive, px) ? 1 : 0;
    if (!have) return;
#ifdef _WIN32
    {
        SDL_SysWMinfo wm;
        SDL_VERSION(&wm.version);
        if (SDL_GetWindowWMInfo(win, &wm) && wm.subsystem == SDL_SYSWM_WINDOWS) {
            int which;
            for (which = 0; which < 2; which++) {
                int sz = GetSystemMetrics(which ? SM_CXSMICON : SM_CXICON);
                BITMAPV5HEADER bi;
                void* bits = NULL;
                HBITMAP color, mask;
                ICONINFO ii;
                HICON icon;
                uint8_t* rgba;
                int i;
                if (sz <= 0 || sz > 512) sz = which ? 16 : 32;
                rgba = (uint8_t*)malloc((size_t)sz * sz * 4);
                if (!rgba) return;
                disc_icon_render(px, sz, rgba);
                memset(&bi, 0, sizeof bi);
                bi.bV5Size = sizeof bi;
                bi.bV5Width = sz;
                bi.bV5Height = -sz;
                bi.bV5Planes = 1;
                bi.bV5BitCount = 32;
                bi.bV5Compression = BI_BITFIELDS;
                bi.bV5RedMask = 0x00FF0000;
                bi.bV5GreenMask = 0x0000FF00;
                bi.bV5BlueMask = 0x000000FF;
                bi.bV5AlphaMask = 0xFF000000;
                color = CreateDIBSection(NULL, (BITMAPINFO*)&bi, DIB_RGB_COLORS, &bits, NULL, 0);
                mask = CreateBitmap(sz, sz, 1, 1, NULL);
                if (!color || !mask || !bits) {
                    if (color) DeleteObject(color);
                    if (mask) DeleteObject(mask);
                    free(rgba);
                    return;
                }
                for (i = 0; i < sz * sz; i++) {
                    uint8_t* d = (uint8_t*)bits + i * 4;
                    d[0] = rgba[i * 4 + 2]; d[1] = rgba[i * 4 + 1];
                    d[2] = rgba[i * 4 + 0]; d[3] = rgba[i * 4 + 3];
                }
                free(rgba);
                memset(&ii, 0, sizeof ii);
                ii.fIcon = TRUE;
                ii.hbmMask = mask;
                ii.hbmColor = color;
                icon = CreateIconIndirect(&ii);
                DeleteObject(color);
                DeleteObject(mask);
                if (icon)
                    SendMessage(wm.info.win.window, WM_SETICON, which ? ICON_SMALL : ICON_BIG,
                                (LPARAM)icon);
            }
            printf("[icon] window icon set from the disc's memory-card icon\n");
            return;
        }
    }
#endif
    {
        static uint8_t big[64 * 64 * 4];
        SDL_Surface* s;
        disc_icon_render(px, 64, big);
        s = SDL_CreateRGBSurfaceWithFormatFrom(big, 64, 64, 32, 64 * 4, SDL_PIXELFORMAT_RGBA32);
        if (s) {
            SDL_SetWindowIcon(win, s);
            SDL_FreeSurface(s);
        }
    }
}

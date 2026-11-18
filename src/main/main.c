/* Main entry point for the SMB2 PC port */
#include "platform.h"
#include "input.h"
#include "audio.h"
#include "render.h"
#include "overlay.h"
#include "assets.h"
#include "pc_settings.h"
#include "gamedata.h"
#include "../assets/disc_reader.h"
#include <time.h>
#include "install_mode.h"
#include "pc_update.h"
#include "build_id.h"
#include "pc_card.h"
#include "netplay/session.h"
#include "netplay/inputsync.h"
#include "netplay/netplay_active.h"
#include "netplay/checksum.h"
#include "netplay/netplay.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Game state */
typedef enum {
    GAME_STATE_INIT,
    GAME_STATE_RESOLUTION_PICKER,
    GAME_STATE_MAIN_MENU,
    GAME_STATE_GAMEPLAY,
    GAME_STATE_EXIT,
} GameState;

static GameState g_game_state = GAME_STATE_INIT;
static bool g_running = true;

/* Resolution picker defaults */
static int g_window_width = 1280;
static int g_window_height = 720;
static bool g_fullscreen = false;
static bool g_vsync = true;
static int g_msaa = 4;
static const char* g_fps_arg = NULL;  /* --fps 60|120|...|uncapped|display */
static const char* g_iso_arg = NULL;
static const char* g_gamedata_arg = NULL;    /* --gamedata <dir> */
static const char* g_extract_iso_arg = NULL; /* --extract-iso <iso> (headless) */
static const char* g_verify_arg = NULL;      /* --verify-gamedata <dir> (headless) */
static int g_progress_arg = 0;               /* --progress */
static const char* g_renderer_arg = NULL; /* --renderer gl|d3d12 */
static const char* g_language_arg = NULL; /* --language auto|en|de|fr|es|it|ja */
const char* g_save_dir_arg = NULL;    /* --save-dir: settings.dat location */
static int g_cli_overrides = 0;       /* PCS_OVR_* given on the command line */
/* Test hooks (read by input_sdl2.c / platform_sdl2.c); env vars don't cross
 * the WSL->Windows interop boundary here, so these come in as arguments. */
const char* g_input_script_arg = NULL;
long g_dump_interval_arg = 0;
long g_exit_frame_arg = 0;
long g_trace_frame_arg = 0;
const char* g_audio_wav_arg = NULL;   /* --audio-wav: dump mixed output */
int g_audio_null_arg = 0;             /* --audio-null: no audio device */
const char* g_button_style_arg = NULL; /* --button-style: auto|gc|ps|ps4|ps5|xbox|switch */
static int g_det_session_arg = 0;          /* --det-session (docs/handoff/E2.md) */
static SessionConfig g_session_cfg;
static int g_session_epoch_given, g_session_locale_given;
static const char* g_record_arg = NULL;       /* --record <file> (E3) */
static const char* g_replay_arg = NULL;       /* --replay <file> (E3) */
static const char* g_checksum_log_arg = NULL; /* --checksum-log <file> (E3) */
static long g_max_frames_arg = 0;             /* --max-frames <n>: stop after n input polls (E3) */
static int g_checksum_ring_arg = 0;           /* --checksum-ring */
static NetplayOptions g_net_opt = { NULL, NULL, 0, NULL, -1, 0, NULL, 0 };  /* --server ... (E6) */
static const char* g_lobby_script_arg = NULL;  /* --lobby-test-script (E8) */
static const char* g_np_resume_arg = NULL;      /* --np-resume CODE,ID,TOKEN: restarted into a session (E8) */
static const char* g_np_notice_arg = NULL;      /* --np-notice <text>: toast after a restart */
static unsigned long g_np_wait_pid_arg = 0;     /* --np-wait-pid: wait for the old process */

/* Forward declarations from decompiled game (weak stubs for standalone build) */
#if !defined(SMB2_LINK_GAME)
__attribute__((weak)) void game_init(void) { printf("[STUB] game_init\n"); }
__attribute__((weak)) void game_main_loop(void) { }
__attribute__((weak)) void game_shutdown(void) { printf("[STUB] game_shutdown\n"); }
__attribute__((weak)) void game_set_resolution(int width, int height) {
    printf("[STUB] game_set_resolution(%d, %d)\n", width, height);
}
__attribute__((weak)) void game_set_fullscreen(bool fullscreen) {
    printf("[STUB] game_set_fullscreen(%s)\n", fullscreen ? "true" : "false");
}
#else
/* Real game: the decompiled main_loop.rel entry point (init + frame loop).
 * GC SDK services come from the shim layer in src/game/shim/. */
extern void _prolog(void);
extern int gc_mem_init(void);
extern void pc_check_address_space(void);
#endif

void show_resolution_picker(void) {
    int width = g_window_width;
    int height = g_window_height;
    bool fullscreen = g_fullscreen;

    if (platform_show_resolution_picker(&width, &height, &fullscreen)) {
        g_window_width = width;
        g_window_height = height;
        g_fullscreen = fullscreen;

        /* Apply to platform */
        if (g_platform.vtable && g_platform.vtable->set_fullscreen) {
            g_platform.vtable->set_fullscreen(&g_platform, fullscreen);
        }
        /* the window size is not applied (unused picker) */
    }
}

int main(int argc, char* argv[]) {
    /* SDL3 declares per-monitor-v2 DPI awareness itself when video initialises
     * (SDL_HINT_WINDOWS_DPI_AWARENESS no longer exists), before any window is sized. */
#ifdef SMB2_LINK_GAME
    extern void shim_install_crash_handler(void);
    extern void shim_start_watchdog(void);
    shim_install_crash_handler();
#endif
    /* Hidden verification tool: --rvz-dump <in.rvz|iso> <out.iso> decodes a whole disc image. */
    for (int i = 1; i + 2 < argc; i++) {
        if (strcmp(argv[i], "--rvz-dump") == 0) {
            char why[256] = "";
            DiscReader* dr = disc_reader_open(argv[i + 1], why, sizeof why);
            FILE* out;
            static unsigned char dbuf[1 << 20];
            int64_t total, pos = 0;
            clock_t t0 = clock();
            if (!dr) { printf("rvz-dump: %s\n", why); return 1; }
            out = fopen(argv[i + 2], "wb");
            if (!out) { printf("rvz-dump: cannot create %s\n", argv[i + 2]); disc_reader_close(dr); return 1; }
            total = disc_reader_size(dr);
            while (pos < total) {
                uint32_t n = (total - pos) > (int64_t)sizeof dbuf ? (uint32_t)sizeof dbuf : (uint32_t)(total - pos);
                if (!disc_reader_read(dr, (uint64_t)pos, n, dbuf) || fwrite(dbuf, 1, n, out) != n) {
                    printf("rvz-dump: failed at offset %lld\n", (long long)pos);
                    fclose(out); disc_reader_close(dr);
                    return 1;
                }
                pos += n;
            }
            fclose(out);
            {
                double secs = (double)(clock() - t0) / CLOCKS_PER_SEC;
                printf("rvz-dump: format=%s %lld bytes in %.2f s (%.1f MB/s)\n", disc_reader_format(dr),
                       (long long)total, secs, secs > 0 ? total / 1048576.0 / secs : 0.0);
            }
            disc_reader_close(dr);
            return 0;
        }
    }
    /* Headless installer modes: no window, audio or game, only the
     * PROGRESS/RESULT protocol on stdout (docs/installer.md). */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--extract-iso") == 0 || strcmp(argv[i], "--verify-gamedata") == 0) {
            const char* iso = NULL; const char* gd = NULL; const char* ver = NULL;
            bool progress = false, bad = false;
            for (int j = 1; j < argc; j++) {
                if (strcmp(argv[j], "--extract-iso") == 0 && j + 1 < argc) iso = argv[++j];
                else if (strcmp(argv[j], "--verify-gamedata") == 0 && j + 1 < argc) ver = argv[++j];
                else if (strcmp(argv[j], "--gamedata") == 0 && j + 1 < argc) gd = argv[++j];
                else if (strcmp(argv[j], "--progress") == 0) progress = true;
                else if (strcmp(argv[j], "--extract-iso") == 0 || strcmp(argv[j], "--verify-gamedata") == 0 ||
                         strcmp(argv[j], "--gamedata") == 0) bad = true;
            }
            if (bad || (iso && ver) || (iso && !gd) || (!iso && !ver)) {
                printf("RESULT ERROR %d usage: --extract-iso <iso> --gamedata <dir> [--progress]"
                       " | --verify-gamedata <dir> [--progress]\n", GD_ERR_OTHER);
                fflush(stdout);
                return GD_ERR_OTHER;
            }
            return iso ? gamedata_cli_extract(iso, gd, progress) : gamedata_cli_verify(ver, progress);
        }
    }
    for (int i = 1; i < argc; i++)
        if (strcmp(argv[i], "--version") == 0) {
            printf("SMB2 PC port %s, build %s\n", pc_update_current_version(), smb2_build_id());
            return 0;
        }
    /* --list-controllers: diagnostic, no window (input_sdl2.c) */
    for (int i = 1; i < argc; i++)
        if (strcmp(argv[i], "--list-controllers") == 0)
            return input_print_controllers();
    printf("SMB2 PC port %s, build %s\n", pc_update_current_version(), smb2_build_id());
    printf("DEBUG: main() started\n");
    fflush(stdout);
    printf("========================================\n");
    printf("  SMB2 - unofficial PC port\n");
    printf("========================================\n\n");
    fflush(stdout);

    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--width") == 0 && i + 1 < argc) {
            g_window_width = atoi(argv[++i]);
            g_cli_overrides |= PCS_OVR_SIZE;
        } else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc) {
            g_window_height = atoi(argv[++i]);
            g_cli_overrides |= PCS_OVR_SIZE;
        } else if (strcmp(argv[i], "--fullscreen") == 0) {
            g_fullscreen = true;
            g_cli_overrides |= PCS_OVR_MODE;
        } else if (strcmp(argv[i], "--windowed") == 0) {
            g_fullscreen = false;
            g_cli_overrides |= PCS_OVR_MODE;
        } else if (strcmp(argv[i], "--no-vsync") == 0) {
            g_vsync = false;
            g_cli_overrides |= PCS_OVR_VSYNC;
        } else if (strcmp(argv[i], "--msaa") == 0 && i + 1 < argc) {
            g_msaa = atoi(argv[++i]);
            g_cli_overrides |= PCS_OVR_MSAA;
        } else if (strcmp(argv[i], "--fps") == 0 && i + 1 < argc) {
            g_fps_arg = argv[++i];
            g_cli_overrides |= PCS_OVR_FPS;
        } else if (strcmp(argv[i], "--dump-from") == 0 && i + 1 < argc) {
            extern long g_dump_from_arg;
            g_dump_from_arg = atol(argv[++i]);
        } else if (strcmp(argv[i], "--dump-ranges") == 0 && i + 1 < argc) {
            extern void platform_set_dump_ranges(const char* spec);
            platform_set_dump_ranges(argv[++i]);
        } else if (strcmp(argv[i], "--overlay-open") == 0) {
            overlay_test_open_at_start();
        } else if (strcmp(argv[i], "--overlay-test-set") == 0 && i + 1 < argc) {
            overlay_test_set(argv[++i]);   /* key=value, same setters as the overlay rows */
        } else if (strcmp(argv[i], "--overlay-test-online") == 0) {
            netplay_test_force_active(1);   /* test: overlay behaves as online (no pause) */
        } else if (strcmp(argv[i], "--overlay-open-frames") == 0 && i + 1 < argc) {
            long a = 0, b = 0;
            if (sscanf(argv[++i], "%ld-%ld", &a, &b) == 2 && b > a) overlay_test_open_frames(a, b);
        } else if (strcmp(argv[i], "--renderer") == 0 && i + 1 < argc) {
            g_renderer_arg = argv[++i];
        } else if (strcmp(argv[i], "--language") == 0 && i + 1 < argc) {
            g_language_arg = argv[++i];
        } else if (strcmp(argv[i], "--save-dir") == 0 && i + 1 < argc) {
            g_save_dir_arg = argv[++i];
        } else if (strcmp(argv[i], "--iso") == 0 && i + 1 < argc) {
            g_iso_arg = argv[++i];
        } else if (strcmp(argv[i], "--gamedata") == 0 && i + 1 < argc) {
            g_gamedata_arg = argv[++i];
        } else if (strcmp(argv[i], "--input-script") == 0 && i + 1 < argc) {
            g_input_script_arg = argv[++i];
        } else if (strcmp(argv[i], "--dump-interval") == 0 && i + 1 < argc) {
            g_dump_interval_arg = atol(argv[++i]);
        } else if (strcmp(argv[i], "--exit-frame") == 0 && i + 1 < argc) {
            g_exit_frame_arg = atol(argv[++i]);
        } else if (strcmp(argv[i], "--trace-frame") == 0 && i + 1 < argc) {
            g_trace_frame_arg = atol(argv[++i]);
        } else if (strcmp(argv[i], "--audio-wav") == 0 && i + 1 < argc) {
            g_audio_wav_arg = argv[++i];
        } else if (strcmp(argv[i], "--button-style") == 0 && i + 1 < argc) {
            g_button_style_arg = argv[++i];
        } else if (strcmp(argv[i], "--det-session") == 0) {
            g_det_session_arg = 1;
        } else if (strcmp(argv[i], "--record") == 0 && i + 1 < argc) {
            g_record_arg = argv[++i];
        } else if (strcmp(argv[i], "--replay") == 0 && i + 1 < argc) {
            g_replay_arg = argv[++i];
        } else if (strcmp(argv[i], "--checksum-log") == 0 && i + 1 < argc) {
            g_checksum_log_arg = argv[++i];
        } else if (strcmp(argv[i], "--max-frames") == 0 && i + 1 < argc) {
            g_max_frames_arg = atol(argv[++i]);
        } else if (strcmp(argv[i], "--server") == 0 && i + 1 < argc) {
            g_net_opt.server = argv[++i];
        } else if (strcmp(argv[i], "--create") == 0) {
            g_net_opt.create = 1;
        } else if (strcmp(argv[i], "--join") == 0 && i + 1 < argc) {
            g_net_opt.join_code = argv[++i];
        } else if (strcmp(argv[i], "--name") == 0 && i + 1 < argc) {
            g_net_opt.name = argv[++i];
        } else if (strcmp(argv[i], "--delay") == 0 && i + 1 < argc) {
            g_net_opt.delay = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--players") == 0 && i + 1 < argc) {
            g_net_opt.players = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--room-code-file") == 0 && i + 1 < argc) {
            g_net_opt.code_file = argv[++i];
        } else if (strcmp(argv[i], "--lobby-timeout") == 0 && i + 1 < argc) {
            g_net_opt.lobby_timeout_s = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--lobby-test-script") == 0 && i + 1 < argc) {
            g_lobby_script_arg = argv[++i];
        } else if (strcmp(argv[i], "--np-resume") == 0 && i + 1 < argc) {
            g_np_resume_arg = argv[++i];
        } else if (strcmp(argv[i], "--np-notice") == 0 && i + 1 < argc) {
            g_np_notice_arg = argv[++i];
        } else if (strcmp(argv[i], "--np-wait-pid") == 0 && i + 1 < argc) {
            g_np_wait_pid_arg = strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--np-exit-file") == 0 && i + 1 < argc) {
            netplay_set_exit_file(argv[++i]);
        } else if (strcmp(argv[i], "--checksum-ring") == 0) {
            g_checksum_ring_arg = 1;
        } else if (strcmp(argv[i], "--session-epoch") == 0 && i + 1 < argc) {
            g_session_cfg.epoch = strtoull(argv[++i], NULL, 0);
            g_session_epoch_given = 1;
        } else if (strcmp(argv[i], "--session-locale") == 0 && i + 1 < argc) {
            g_session_cfg.locale = (uint8_t)atoi(argv[++i]);
            g_session_locale_given = 1;
        } else if (strcmp(argv[i], "--audio-null") == 0) {
            g_audio_null_arg = 1;
        } else if (strcmp(argv[i], "--perf") == 0) {
            extern int g_perf_on;
            g_perf_on = 1;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  (video options override settings.dat for this run; change them\n");
            printf("   in game: Options > Game Screen, or F1 anywhere)\n");
            printf("  --width <w>       Window width (default: 1280 or fit desktop)\n");
            printf("  --height <h>      Window height (default: 720)\n");
            printf("  --fullscreen      Start in (borderless) fullscreen\n");
            printf("  --windowed        Start in windowed mode\n");
            printf("  --no-vsync        Disable VSync\n");
            printf("  --msaa <n>        MSAA samples (default: 4)\n");
            printf("  --fps <n>         Rendered frames per second: 60 (default, no\n"
                   "                    interpolation), 120, 144, 240, ..., uncapped or\n"
                   "                    display (refresh rate); the game still runs at 60\n");
            printf("  --overlay-open    Open the (test) overlay at start; --overlay-open-frames a-b\n"
                   "                    opens it for swapped frames a..b-1. F2 toggles it.\n");
            printf("  --renderer <r>    Renderer: gl or d3d12 (default: settings.dat, else gl)\n");
            printf("  --language <l>    Game language: auto, en, de, fr, es, it or ja\n"
                   "                    (this run only; default: settings.dat, else auto)\n");
            printf("  --save-dir <dir>  Directory for settings.dat and saves\n");
            printf("  --version         Print the version and build id, then exit\n");
            printf("  --iso <file>      Game disc image (GM2E8P .iso/.gcm/.rvz)\n");
            printf("  --gamedata <dir>  Extracted game data folder (instead of an ISO)\n");
            printf("  --extract-iso <file> --gamedata <dir> [--progress]\n"
                   "                    Extract + verify the disc into <dir> (headless)\n");
            printf("  --verify-gamedata <dir> [--progress]\n"
                   "                    Check <dir> against its manifest.txt (headless)\n");
            printf("  --import-save <file>\n"
                   "                    Import a GameCube save: a .gci file (Dolphin Memory\n"
                   "                    Card Manager > Export GCI, GCMM) or a whole memory\n"
                   "                    card image (.raw); then exit\n");
            printf("  --export-save <folder>\n"
                   "                    Write the game's saves as .gci files; then exit\n");
            printf("  --audio-wav <f>   Write the mixed audio output to a WAV file\n");
            printf("  --audio-null      Run audio without an output device\n");
            printf("  --button-style <s> Button prompts: auto, gc, ps, ps4, ps5, xbox or switch\n"
                   "                    (this run only)\n");
            printf("  --det-session     Deterministic session: frame-derived clock, in-memory\n"
                   "                    empty memory card with all party games unlocked, forced\n"
                   "                    locale/button style, no PC rows or dialogs\n"
                   "                    (--session-epoch <n>, --session-locale <0-5>)\n");
            printf("  --record <file>   Record every poll's 4 pads to <file> (implies --det-session)\n"
                   "  --replay <file>   Play a recording: pads come from the file, the session\n"
                   "                    config (epoch, locale) from its header (implies --det-session;\n"
                   "                    local input ignored; exits at the end of the file)\n"
                   "  --checksum-log <file> Write 'poll N hash XXXXXXXX' every 60 polls (session only)\n"
                   "  --max-frames <n>  Exit(0) after n input polls (one per game frame; unlike\n"
                   "                    --exit-frame, which counts presented frames)\n"
                   "  --checksum-ring   Hash every poll into a 600-entry ring (desync bisecting)\n");
            printf("  --server <host[:port]> Online play (lockstep via the relay server, default port 7855):\n"
                   "                    with --create (host) or --join <CODE>; --name <n>, --delay <polls>\n"
                   "                    (host, default 2), --players <n> (host starts when n are in,\n"
                   "                    default 2), --room-code-file <f>, --lobby-timeout <s>.\n"
                   "                    Implies --det-session; --record/--checksum-log/--max-frames work.\n");
            printf("  --list-controllers Print every joystick / gamepad SDL sees (name, VID/PID,\n"
                   "                    GUID, mapping), wait 1 s for late devices, then exit\n");
            printf("  --perf            Print frame timing / GL counters every 300 frames\n");
            printf("  --help, -h        Show this help\n");
            return 0;
        }
    }

    if (g_record_arg && g_replay_arg) {
        fprintf(stderr, "--record and --replay cannot be combined\n");
        return 1;
    }
    if (g_replay_arg) {
        /* the recording carries the session config (epoch, locale, ...) */
        SessionConfig rc;
        if (!inputsync_open_replay(g_replay_arg, &rc))
            return 1;
        g_session_cfg = rc;
        g_session_epoch_given = g_session_locale_given = 0;
        g_det_session_arg = 2;   /* config already set */
    }
    /* E8: in-game lobby support.  The restart after START_GAME carries --np-resume; --lobby-test-script drives
     * the lobby API from the frame loop (test hook).  Flags that belong to the SESSION (recording, checksum log,
     * input script, frame limit) are not applied in the offline lobby process of a test script: they stay in
     * argv, so the restarted session process gets them. */
    netplay_set_argv(argc, argv);
    if (g_np_wait_pid_arg)
        netplay_wait_for_parent(g_np_wait_pid_arg);
    if (g_np_notice_arg)
        netplay_push_notice(NP_EV_INFO, g_np_notice_arg);
    netplay_set_automated(g_max_frames_arg > 0 || g_input_script_arg != NULL);
    if (g_np_resume_arg && !netplay_set_resume(g_np_resume_arg)) {
        fprintf(stderr, "bad --np-resume (expected CODE,ID,TOKEN)\n");
        return 1;
    }
    if (g_lobby_script_arg) {
        if (!netplay_set_lobby_script(g_lobby_script_arg)) {
            fprintf(stderr, "bad --lobby-test-script (see docs/handoff/E8.md)\n");
            return 1;
        }
        g_record_arg = NULL;
        g_checksum_log_arg = NULL;
        g_max_frames_arg = 0;
        g_input_script_arg = NULL;
        g_checksum_ring_arg = 0;
        netplay_set_automated(0);
    }
    if (g_net_opt.server) {
        /* online (E6/E8): the session starts with default settings now (no saves, in-memory card ...) and takes the
         * room's config (epoch chosen by the server, disc hash) once the lobby is done, after the game data is
         * known (below).  Without a restart: this process is fresh. */
        if (g_replay_arg) {
            fprintf(stderr, "--server and --replay cannot be combined\n");
            return 1;
        }
        netplay_set_options(&g_net_opt);
        g_det_session_arg = 1;
    }
    if (g_record_arg)
        g_det_session_arg = g_det_session_arg ? g_det_session_arg : 1;
    if (g_det_session_arg) {
        /* Deterministic session (E2): config from the defaults + --session-*
         * flags (netplay fills it from START_GAME later).  Everything the
         * session pins is described in docs/handoff/E2.md. */
        SessionConfig c;
        if (g_det_session_arg == 2) {
            c = g_session_cfg;   /* from the recording */
        } else {
            pc_session_default_config(&c);
            if (g_session_epoch_given) c.epoch = g_session_cfg.epoch;
            if (g_session_locale_given) c.locale = g_session_cfg.locale;
        }
        g_session_cfg = c;
        if (!pc_session_begin(&g_session_cfg)) {
            fprintf(stderr, "invalid session config (locale must be 0..5)\n");
            return 1;
        }
        if (g_record_arg && !g_net_opt.server && !inputsync_open_record(g_record_arg))
            return 1;   /* online: opened after the lobby, the header carries the room's config */
        checksum_open_log(g_checksum_log_arg);
        checksum_enable_ring(g_checksum_ring_arg);
        if (g_max_frames_arg > 0)
            inputsync_set_max_polls((uint64_t)g_max_frames_arg);
        g_language_arg = NULL;       /* the session forces the locale */
        g_button_style_arg = NULL;   /* ... and the GameCube button style */
    }

    /* install.ini next to the exe (installed mode); absent = portable */
    pc_install_init(g_save_dir_arg);
#ifdef SMB2_LINK_GAME
    {   /* save import/export from the command line: before the log
         * redirect, so the result shows in the console */
        int rc = pc_session_active() ? -1 : pc_gci_cli(argc, argv);   /* no save import/export in a session */
        if (rc >= 0)
            return rc;
    }
#endif
    pc_install_setup_logs();
#ifdef SMB2_LINK_GAME
    if (!pc_session_active())   /* the session card is in memory (E1 D4/D11) */
        pc_gci_startup();   /* memcard\import\ drop-in saves, before the game reads the card */
#endif

    /* Saved settings (or first-run defaults), then command-line overrides */
    pcs_load();
    /* updater (docs/updater.md): version to the log, automatic check on a
     * background thread (at most daily; not in automated runs) */
    pc_update_startup(pc_session_active() || g_input_script_arg || g_dump_interval_arg || g_exit_frame_arg ||
                      getenv("SMB2_INPUT_SCRIPT"));
    /* Automated runs (input scripts, frame dumps, --exit-frame) stay
     * windowed unless asked otherwise, so they never take over the screen. */
    if (!(g_cli_overrides & PCS_OVR_MODE) &&
        (g_input_script_arg || g_dump_interval_arg || g_exit_frame_arg || getenv("SMB2_INPUT_SCRIPT"))) {
        g_fullscreen = false;
        g_cli_overrides |= PCS_OVR_MODE;
    }
    {
        PcSettings ovr = g_pcs;
        ovr.width = g_window_width;
        ovr.height = g_window_height;
        if (g_cli_overrides & PCS_OVR_MODE)
            ovr.display_mode = g_fullscreen ? PCS_BORDERLESS : PCS_WINDOWED;
        ovr.vsync = g_vsync;
        ovr.msaa = g_msaa;
        if (g_fps_arg) ovr.fps = pcs_parse_fps(g_fps_arg);
        if (g_cli_overrides & PCS_OVR_SIZE) {   /* --width alone keeps the saved height */
            int w_given = 0, h_given = 0;
            for (int i = 1; i < argc; i++) {
                if (!strcmp(argv[i], "--width")) w_given = 1;
                if (!strcmp(argv[i], "--height")) h_given = 1;
            }
            if (!w_given) ovr.width = g_pcs.width;
            if (!h_given) ovr.height = g_pcs.height;
        }
        {   /* renderer: --renderer > SMB2_RENDERER > settings.dat > gl */
            const char* r = g_renderer_arg ? g_renderer_arg : getenv("SMB2_RENDERER");
            if (r && r[0]) {
                ovr.renderer = (!strcmp(r, "d3d12") || !strcmp(r, "dx12") || !strcmp(r, "D3D12"))
                             ? PCS_RENDERER_D3D12 : PCS_RENDERER_GL;
                g_cli_overrides |= PCS_OVR_RENDERER;
            }
        }
        if (g_language_arg) {
            int l = pcs_parse_language(g_language_arg);
            if (l < 0)
                fprintf(stderr, "unknown --language '%s' (auto, en, de, fr, es, it, ja)\n", g_language_arg);
            else {
                ovr.language = l;
                g_cli_overrides |= PCS_OVR_LANGUAGE;
            }
        }
        pcs_cli_override(g_cli_overrides, &ovr);
        g_window_width = g_pcs.width;
        g_window_height = g_pcs.height;
        g_fullscreen = g_pcs.display_mode == PCS_BORDERLESS;
        g_vsync = g_pcs.vsync != 0;
        g_msaa = g_pcs.msaa;
    }

    /* Platform configuration */
    char window_title[96];
    snprintf(window_title, sizeof window_title, "Super Monkey Ball 2 – build %s (%s)",
             smb2_build_number(), smb2_build_sha());
    PlatformConfig config = {
        .window_width = g_window_width,
        .window_height = g_window_height,
        .fullscreen = g_fullscreen,
        .vsync = g_vsync,
        .msaa_samples = g_msaa,
        .window_title = window_title,
        .opengl = g_pcs.renderer != PCS_RENDERER_D3D12,
    };

    /* Locate the game data before the window exists: --gamedata, --iso,
     * install.ini gamedata, saved settings.dat ISO path, or the file dialog
     * (platform_resolve_game_data).  A (fullscreen) game window would cover
     * the dialog. */
    if (!platform_resolve_game_data(g_gamedata_arg, g_iso_arg)) {
        pc_user_msg("No game data - exiting.\n");
        return 1;
    }

    if (g_net_opt.server) {
        /* online: lobby (command line) or seat takeover after the restart (E8).  Needs the disc identity, so it
         * runs once the game data is resolved; the process is fresh (no game code has run) so the session can
         * simply take the room's config and start from poll 0. */
        SessionConfig nc;
        int ok = netplay_is_resume() ? netplay_lobby_resume(&nc) : netplay_lobby_run(&nc);
        if (!ok)
            return 1;
        if (!pc_session_reconfigure(&nc)) {
            fprintf(stderr, "invalid session config from the server\n");
            return 1;
        }
        g_session_cfg = nc;
        if (g_record_arg && !inputsync_open_record(g_record_arg))
            return 1;
        if (!netplay_begin())
            return 1;
    }

    /* Initialize platform */
    printf("DEBUG: Calling platform_init...\n");
    fflush(stdout);
    if (!platform_init(&config)) {
        fprintf(stderr, "Failed to initialize platform\n");
        return 1;
    }
    printf("DEBUG: platform_init returned\n");
    fflush(stdout);

    /* Video settings are in game (Options > Game Screen, F1);
     * show_resolution_picker() is unused. */
    (void)show_resolution_picker;

    platform_apply_window_icon();
#ifdef SMB2_LINK_GAME
    /* after the (possibly interactive / extracting) data step: the watchdog
     * expects a frame every 8 s */
    shim_start_watchdog();
#endif


    /* Initialize subsystems */
    printf("DEBUG: Initializing renderer...\n");
    fflush(stdout);
    {
        int use_d3d12 = g_pcs.renderer == PCS_RENDERER_D3D12;
        bool ok = render_init(use_d3d12 ? "d3d12" : "gl", g_window_width, g_window_height,
                              g_vsync, g_msaa);
        if (!ok && use_d3d12) {
            fprintf(stderr, "D3D12 renderer unavailable - falling back to OpenGL\n");
            use_d3d12 = 0;
            ok = platform_recreate_window(true) &&
                 render_init("gl", g_window_width, g_window_height, g_vsync, g_msaa);
        }
        if (!ok) {
            fprintf(stderr, "Failed to initialize renderer\n");
            platform_shutdown();
            return 1;
        }
        pcs_set_active_renderer(use_d3d12 ? PCS_RENDERER_D3D12 : PCS_RENDERER_GL);
        overlay_init((SDL_Window*)platform_sdl_window(),
                     use_d3d12 ? OVERLAY_RENDERER_D3D12 : OVERLAY_RENDERER_GL);
    }
    printf("DEBUG: Renderer initialized\n");
    fflush(stdout);
    pcs_apply_startup();   /* exclusive fullscreen, drawable size */
    pcs_first_run_aspect(pc_session_active() || g_input_script_arg || g_dump_interval_arg || g_exit_frame_arg ||
                         getenv("SMB2_INPUT_SCRIPT"));

    printf("DEBUG: Initializing audio...\n");
    fflush(stdout);
    if (!audio_init()) {
        fprintf(stderr, "Failed to initialize audio\n");
    }
    printf("DEBUG: Audio initialized\n");
    fflush(stdout);
    pcs_apply_volume();

    printf("DEBUG: Initializing input...\n");
    fflush(stdout);
    input_init();
    printf("DEBUG: Input initialized\n");
    fflush(stdout);

    printf("DEBUG: Initializing assets...\n");
    fflush(stdout);
    if (!assets_init("assets")) {
        fprintf(stderr, "Failed to initialize asset manager\n");
    }
    printf("DEBUG: Assets initialized\n");
    fflush(stdout);

#ifdef SMB2_LINK_GAME
    /* Map GameCube main RAM at 0x80000000 and hand control to the game.
     * _prolog() contains the game's own init + infinite frame loop; the
     * shim's video_finish_frame() pumps SDL and exits on window close. */
    if (!gc_mem_init() && pc_session_active()) {
        /* E1 D15/D16: a relocated arena shifts every heap pointer */
        fprintf(stderr, "FATAL: deterministic session needs GC RAM at 0x80000000\n");
        return 1;
    }
    pc_check_address_space();
    {   /* The game's language: the DOL's main() sets `locale` once at boot
         * (US disc: always 0, 0x800069F8); the port's main stands in for it
         * and sets the chosen one.  docs/language.md */
        extern int locale;
        int l = pc_session_active() ? (int)pc_session_config()->locale   /* E1 D7 */
                                    : pcs_language_locale(g_pcs.language);
        extern void pc_font_ext_init(int locale);
        locale = l;
        pcs_set_active_locale(l);
        printf("[lang] locale %d (%s)\n", l, "en\0de\0fr\0es\0it\0ja" + 3 * l);
        pc_font_ext_init(l);   /* Japanese: the font's kanji pages */
    }
    printf("\nHanding off to SMB2 _prolog()...\n\n");
    fflush(stdout);
    _prolog();

    printf("\nGame loop exited - shutting down.\n");
    assets_shutdown();
    input_shutdown();
    audio_shutdown();
    overlay_shutdown();
    render_shutdown();
    platform_shutdown();
    return 0;
#else
    /* Initialize game */
    game_init();
    game_set_resolution(g_window_width, g_window_height);
    game_set_fullscreen(g_fullscreen);

    g_game_state = GAME_STATE_MAIN_MENU;
    g_running = true;

    printf("\nStarting main loop...\n");
    printf("Press ESC or close window to exit\n\n");

    uint64_t last_time = platform_get_ticks_ms();
    const double target_frame_time = 1000.0 / 60.0; /* 60 FPS */

    /* Main loop */
    while (g_running && !platform_should_close()) {
        uint64_t current_time = platform_get_ticks_ms();
        double dt = (current_time - last_time) / 1000.0;
        last_time = current_time;

        /* Cap frame time */
        if (dt > 0.1) dt = 0.1;

        /* Process events */
        platform_poll_events();
        process_inputs();
        threshold_analog_inputs();
        g_calc_frames_since_last_input_change();
        merge_inputs();

        /* Update audio */
        audio_update();

        /* Render frame */
        render_begin_frame();

        if (g_game_state == GAME_STATE_MAIN_MENU) {
            /* standalone build: no menu to draw */
        } else if (g_game_state == GAME_STATE_GAMEPLAY) {
            game_main_loop();
        }

        render_end_frame();
        platform_swap_buffers();

        /* Frame rate limiting */
        uint64_t frame_end = platform_get_ticks_ms();
        double frame_time = frame_end - current_time;
        if (frame_time < target_frame_time) {
            platform_sleep_ms((uint32_t)(target_frame_time - frame_time));
        }
    }

    printf("\nShutting down...\n");

    /* Cleanup */
    game_shutdown();
    assets_shutdown();
    input_shutdown();
    audio_shutdown();
    overlay_shutdown();
    render_shutdown();
    platform_shutdown();

    printf("Goodbye!\n");
    return 0;
#endif /* SMB2_LINK_GAME */
}

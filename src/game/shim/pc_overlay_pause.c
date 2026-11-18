/* Offline pause for the ImGui overlay (D3).  Called from video_finish_frame
 * after the poll, over the finished-but-unpresented game frame (same spot and
 * technique as the old F1 overlay, pc_settings_menu.c): keeps presenting that
 * frame (the overlay is drawn on top by the renderer's present) and pumps
 * events until the overlay closes.  No game pad poll happens meanwhile, so the
 * sim, the input script and the input-sync poll counter all stand still. */
#include <stdio.h>
#include "overlay.h"
#include "netplay/inputsync.h"
#include "netplay/netplay_active.h"
#include "netplay/netplay.h"

extern void platform_poll_events(void);
extern int  platform_should_close(void);
extern void platform_swap_buffers(void);
extern unsigned long long platform_get_time_ns(void);
extern void platform_wait_until_ns(unsigned long long target_ns);
extern int  pc_frame_capture(void);
extern void pc_frame_restore(float brightness);
extern volatile long g_shim_frame_pulse;
extern long g_dbg_frame_counter;

/* Returns 1 if it ran (the caller then skips its own present for this frame). */
int pc_overlay_pause_run(void)
{
    unsigned long long next_ns;
    unsigned long long polls = inputsync_poll_count();
    long presented = 0;

    if (!overlay_is_open() || netplay_active() || !pc_frame_capture())
        return 0;
    printf("[overlay] game paused at poll %llu\n", (unsigned long long)polls);
    fflush(stdout);
    next_ns = platform_get_time_ns();
    while (overlay_is_open() && !platform_should_close()) {
        unsigned long long now;
        pc_frame_restore(0.6f);
        platform_swap_buffers();   /* renderer present draws the overlay */
        presented++;
#ifdef _WIN32
        g_shim_frame_pulse++;      /* watchdog */
#endif
        platform_poll_events();
        netplay_lobby_tick();      /* E8: keep the online lobby alive while the overlay pauses the game */
        next_ns += 16666667ULL;
        now = platform_get_time_ns();
        if (now < next_ns)
            platform_wait_until_ns(next_ns);
        else if (now > next_ns + 100000000ULL)
            next_ns = now;
    }
    pc_frame_restore(1.0f);
    printf("[overlay] game resumed at poll %llu (%ld frames presented while paused)\n",
           (unsigned long long)inputsync_poll_count(), presented);
    fflush(stdout);
    return 1;
}

/* In-game overlay (Dear ImGui, third_party/imgui).  C API; the implementation
 * is C++ (src/overlay/overlay.cpp).  D1: SDL3 + OpenGL.  D2: Direct3D 12 (bridge:
 * src/overlay/overlay_d3d12.cpp), D3 input routing,
 * D4 tabs.  All calls are made on the main (game/render) thread. */
#ifndef SMB2_OVERLAY_H
#define SMB2_OVERLAY_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SDL_Window SDL_Window;
typedef union SDL_Event SDL_Event;

typedef enum {
    OVERLAY_RENDERER_GL = 0,
    OVERLAY_RENDERER_D3D12 = 1,
} OverlayRenderer;

/* After the renderer is initialised (the GL context must be current for GL).
 * Returns false if the overlay is unavailable (it then stays closed). */
bool overlay_init(SDL_Window* window, OverlayRenderer renderer);
void overlay_shutdown(void);

/* Feed every SDL event.  Returns true if the overlay used it (it is open and
 * ImGui wants that mouse/keyboard input, or it is the toggle key).  D1: the
 * caller does not block game input yet. */
bool overlay_process_event(const SDL_Event* e);

/* D3: call once per platform_poll_events, after the event loop.  Controller
 * chord (Back + Start held 0.5 s toggles; the Guide button toggles from
 * overlay_process_event) and mouse cursor (shown while open, hidden after
 * 3 s without mouse activity otherwise). */
void overlay_poll_tick(void);
/* D3: true while local pads must read neutral (the overlay is open).  The
 * input code also holds neutral after closing until the buttons are released. */
bool overlay_input_blocked(void);

/* Start the ImGui frame and build the UI (no-op when closed).  Called by
 * overlay_render if the renderer did not call it earlier in the frame. */
void overlay_new_frame(void);
/* Draw the overlay into the current back buffer at native window size.
 * Call after the game frame, before present.  Restores the GL state it
 * touches.  Cheap no-op when closed.  Safe to call twice per frame. */
void overlay_render(void);

bool overlay_is_open(void);

/* D5: true while the overlay is open OR something else it draws is visible (the FPS
 * counter at the g_pcs.show_fps corner, toasts).  The renderers run their overlay draw
 * path (overlay_render) whenever this is true; when false nothing is drawn and no ImGui
 * frame is built.  overlay_is_open() stays "the menu is open". */
bool overlay_wants_draw(void);

/* D5: toast = small timed message stacked at the top right (partyboard look: blue
 * panel, coloured upper-case heading, fades in and out).  Not thread safe: main
 * thread only.  title/body are copied; body may be NULL.  Netplay (E7/E9) and
 * controller hot-plug use it.  No-op before overlay_init. */
typedef enum {
    OVERLAY_TOAST_INFO = 0,
    OVERLAY_TOAST_SUCCESS = 1,
    OVERLAY_TOAST_WARNING = 2,
    OVERLAY_TOAST_ERROR = 3,
} OverlayToastType;
void overlay_toast(int type /* OverlayToastType */, const char* title, const char* body);
/* test hook --overlay-test-toast "FRAME:type:Title:Body" (repeatable, max 4): fires the
 * toast when the swapped-frame counter reaches FRAME.  Spec must stay valid (argv). */
void overlay_test_toast(const char* spec);
void overlay_toggle(void);
void overlay_set_open(bool open);
/* true while the overlay wants mouse/keyboard (open and ImGui captured it) */
bool overlay_wants_input(void);

/* Direct3D 12 only (called by render_d3d12.c).  Before overlay_render(): the open
 * graphics command list with the destination RTV (R8G8B8A8_UNORM, 1 sample) bound
 * and in RENDER_TARGET state; pass NULL afterwards.  The overlay binds its own
 * descriptor heap and root signature, so the renderer must re-apply its list state.
 * overlay_d3d12_invalidate(): GPU idle, swap chain / targets were recreated. */
void overlay_d3d12_set_target(void* id3d12_graphics_command_list);
void overlay_d3d12_invalidate(void);

/* Test hooks: --overlay-open (open from start), --overlay-open-frames a-b
 * (open while a <= swapped frame < b; applied by overlay_render). */
void overlay_test_open_at_start(void);
void overlay_test_open_frames(long from, long to);
/* --overlay-test-online-script "T:connect=HOST:PORT/NAME,T:host=DELAY,T:join=CODE|@FILE,T:codefile=FILE,T:port=P,
 * T:start=N,T:leave,T:disconnect,T:leave_session" (E7): drives the Online tab's actions (the same functions its
 * buttons call) from platform_poll_events; T = tick (poll count).  Returns false on a bad spec.  Spec must stay
 * valid (argv). */
bool overlay_test_online_script(const char* spec);
/* --overlay-test-set key=value (repeatable): applied once at overlay_init through the
 * same setters the UI rows use (pcs_test_set; keys in pc_settings.h).  key_value must
 * stay valid (argv). */
void overlay_test_set(const char* key_value);

#ifdef __cplusplus
}
#endif
#endif

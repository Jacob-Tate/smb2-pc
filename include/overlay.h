/* In-game overlay (Dear ImGui, third_party/imgui).  C API; the implementation
 * is C++ (src/overlay/overlay.cpp).  D1: SDL3 + OpenGL.  D2 adds Direct3D 12
 * (until then overlay_toggle is a logged no-op there), D3 input routing,
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

/* Start the ImGui frame and build the UI (no-op when closed).  Called by
 * overlay_render if the renderer did not call it earlier in the frame. */
void overlay_new_frame(void);
/* Draw the overlay into the current back buffer at native window size.
 * Call after the game frame, before present.  Restores the GL state it
 * touches.  Cheap no-op when closed.  Safe to call twice per frame. */
void overlay_render(void);

bool overlay_is_open(void);
void overlay_toggle(void);
void overlay_set_open(bool open);
/* true while the overlay wants mouse/keyboard (open and ImGui captured it) */
bool overlay_wants_input(void);

/* Test hooks: --overlay-open (open from start), --overlay-open-frames a-b
 * (open while a <= swapped frame < b; applied by overlay_render). */
void overlay_test_open_at_start(void);
void overlay_test_open_frames(long from, long to);

#ifdef __cplusplus
}
#endif
#endif

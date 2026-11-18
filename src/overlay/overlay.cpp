// In-game overlay: Dear ImGui on SDL3 + OpenGL 3.3 (D1) / Direct3D 12 (D2).  See include/overlay.h.
#include "overlay.h"

#include <SDL3/SDL.h>
#include <stdio.h>

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_opengl3.h"
#include "overlay_d3d12.h"
#include "overlay_ui.h"
#include "pc_settings.h"

extern "C" long g_dbg_frame_counter;   // swapped frames (platform_sdl2.c)
// render_d3d12.c: device / queue / frames in flight for the DX12 backend
extern "C" bool gx_d3d12_overlay_handles(void** device, void** queue, int* frames_in_flight);

namespace {

SDL_Window*     s_window;
OverlayRenderer s_kind;
bool            s_ready;          // ImGui + backends initialised
bool            s_open;
bool            s_frame_started;
bool            s_start_open;     // --overlay-open pending (applied at init)
long            s_open_from = -1, s_open_to = -1;
const char*     s_test_sets[16];  // --overlay-test-set key=value, applied at init
int             s_test_nsets;
Uint64          s_last_mouse_ms;
bool            s_cursor_shown = true;
Uint64          s_chord_since;    // 0 = Back+Start not both held
bool            s_chord_fired;    // fired for this hold; re-arms after release
bool            s_focus_pending;

void* s_d3d12_cl;          // command list to draw into (set by the renderer around overlay_render)

// Back + Start both held on one gamepad (read from SDL state)
bool chord_buttons_down() {
    bool both = false;
    int n = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&n);
    for (int i = 0; ids && i < n && !both; i++) {
        SDL_Gamepad* gp = SDL_GetGamepadFromID(ids[i]);
        if (gp && SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_BACK) && SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_START))
            both = true;
    }
    SDL_free(ids);
    return both;
}

void build_ui() {
    if (s_focus_pending) { ImGui::SetNextWindowFocus(); s_focus_pending = false; }
    ovl::build();
    if (ovl::close_requested()) overlay_set_open(false);
}

void apply_schedule() {
    if (s_open_from >= 0) {
        bool want = g_dbg_frame_counter >= s_open_from && g_dbg_frame_counter < s_open_to;
        if (want != s_open) overlay_set_open(want);
    }
}

}  // namespace

extern "C" {

bool overlay_init(SDL_Window* window, OverlayRenderer renderer) {
    s_window = window;
    s_kind = renderer;
    void* d3d_dev = nullptr; void* d3d_q = nullptr; int d3d_frames = 0;
    if (renderer == OVERLAY_RENDERER_D3D12 && !gx_d3d12_overlay_handles(&d3d_dev, &d3d_q, &d3d_frames)) return false;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;     // no imgui.ini next to the exe
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    ovl::style_init();
    bool ok = renderer == OVERLAY_RENDERER_GL
        ? (ImGui_ImplSDL3_InitForOpenGL(window, SDL_GL_GetCurrentContext()) &&
           ImGui_ImplOpenGL3_Init("#version 330 core"))
        : (ImGui_ImplSDL3_InitForD3D(window) && overlay_d3d12_init(d3d_dev, d3d_q, d3d_frames));
    if (!ok) {
        fprintf(stderr, "[overlay] ImGui backend init failed\n");
        ImGui::DestroyContext();
        return false;
    }
    ImGui_ImplSDL3_SetGamepadMode(ImGui_ImplSDL3_GamepadMode_AutoAll);   // any player's pad can navigate
    s_ready = true;
    for (int i = 0; i < s_test_nsets; i++) {   // same setters as the UI rows
        if (!strncmp(s_test_sets[i], "tab=", 4)) ovl::select_tab(s_test_sets[i] + 4);
        else pcs_test_set(s_test_sets[i]);
    }
    if (s_start_open) overlay_set_open(true);
    printf("[overlay] Dear ImGui %s ready (%s)\n", IMGUI_VERSION,
           renderer == OVERLAY_RENDERER_GL ? "OpenGL" : "Direct3D 12");
    return true;
}

void overlay_shutdown(void) {
    if (!s_ready) return;
    if (s_frame_started) { ImGui::EndFrame(); s_frame_started = false; }
    if (s_kind == OVERLAY_RENDERER_GL) ImGui_ImplOpenGL3_Shutdown(); else overlay_d3d12_shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    s_ready = false;
    s_open = false;
}

bool overlay_process_event(const SDL_Event* e) {
    if (!e) return false;
    if (e->type == SDL_EVENT_KEY_DOWN && !e->key.repeat && e->key.key == SDLK_F2) {
        overlay_toggle();
        return true;
    }
    switch (e->type) {   // cursor idle timer
        case SDL_EVENT_MOUSE_MOTION: case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP: case SDL_EVENT_MOUSE_WHEEL:
            s_last_mouse_ms = SDL_GetTicks();
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:   // Guide opens / closes
            if (e->gbutton.button == SDL_GAMEPAD_BUTTON_GUIDE) { overlay_toggle(); return false; }
            break;
        case SDL_EVENT_GAMEPAD_ADDED: case SDL_EVENT_GAMEPAD_REMOVED:   // ImGui tracks pads even while closed
            if (s_ready && !s_open) ImGui_ImplSDL3_ProcessEvent(e);
            break;
        default: break;
    }
    if (!s_ready || !s_open) return false;
    ImGui_ImplSDL3_ProcessEvent(e);
    ImGuiIO& io = ImGui::GetIO();
    switch (e->type) {
        case SDL_EVENT_MOUSE_MOTION: case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP: case SDL_EVENT_MOUSE_WHEEL:
            return io.WantCaptureMouse;
        case SDL_EVENT_KEY_DOWN: case SDL_EVENT_KEY_UP: case SDL_EVENT_TEXT_INPUT:
            return io.WantCaptureKeyboard;
        default:
            return false;
    }
}

// Once per platform_poll_events: controller chord (Back + Start held 0.5 s) and mouse cursor.
void overlay_poll_tick(void) {
    const Uint64 now = SDL_GetTicks();
    const bool both = chord_buttons_down();
    if (!both) { s_chord_since = 0; s_chord_fired = false; }
    else if (!s_chord_fired) {
        if (!s_chord_since) s_chord_since = now ? now : 1;
        else if (now - s_chord_since >= 500) {
            s_chord_fired = true;
            overlay_toggle();
            printf("[overlay] controller chord: %s\n", s_open ? "open" : "closed");
        }
    }
    // cursor: visible while open, else hidden after 3 s without mouse activity
    bool show = s_open || now - s_last_mouse_ms < 3000;
    if (show != s_cursor_shown) {
        if (show) SDL_ShowCursor(); else SDL_HideCursor();
        s_cursor_shown = show;
    }
}

void overlay_new_frame(void) {
    if (!s_ready || !s_open || s_frame_started) return;
    if (s_kind == OVERLAY_RENDERER_GL) ImGui_ImplOpenGL3_NewFrame(); else overlay_d3d12_new_frame();
    ImGui_ImplSDL3_NewFrame();
    if (s_kind == OVERLAY_RENDERER_D3D12) {   // the DX12 backend draws in DisplaySize units = pixels
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize.x *= io.DisplayFramebufferScale.x; io.DisplaySize.y *= io.DisplayFramebufferScale.y;
        io.DisplayFramebufferScale = ImVec2(1, 1);
    }
    ImGui::NewFrame();
    s_frame_started = true;
    build_ui();
}

void overlay_render(void) {
    apply_schedule();
    if (!s_ready) return;
    if (!s_open) {
        if (s_frame_started) { ImGui::EndFrame(); s_frame_started = false; }
        return;
    }
    if (!s_frame_started) overlay_new_frame();
    if (!s_frame_started) return;
    ImGui::Render();
    if (s_kind == OVERLAY_RENDERER_GL) ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    else overlay_d3d12_render(ImGui::GetDrawData(), s_d3d12_cl);
    s_frame_started = false;
}

bool overlay_is_open(void) { return s_open; }

void overlay_set_open(bool open) {
    if (open && !s_ready) return;
    if (open != s_open) {
        printf("[overlay] %s (swapped frame %ld)\n", open ? "open" : "closed", g_dbg_frame_counter);
        fflush(stdout);
        if (open) { s_focus_pending = true; ovl::on_open(); }   // gamepad nav needs a focused window
    }
    s_open = open;
}

// open, or the chord is being held (so its Start press never reaches the game)
bool overlay_input_blocked(void) { return s_open || chord_buttons_down(); }

void overlay_toggle(void) { overlay_set_open(!s_open); }

bool overlay_wants_input(void) {
    if (!s_ready || !s_open) return false;
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse || io.WantCaptureKeyboard;
}

void overlay_d3d12_set_target(void* cl) { s_d3d12_cl = cl; }

void overlay_test_open_at_start(void) {
    s_start_open = true;
    if (s_ready) overlay_set_open(true);
}

void overlay_test_open_frames(long from, long to) { s_open_from = from; s_open_to = to; }

void overlay_test_set(const char* key_value) {
    if (s_test_nsets < 16) s_test_sets[s_test_nsets++] = key_value;
}

}  // extern "C"

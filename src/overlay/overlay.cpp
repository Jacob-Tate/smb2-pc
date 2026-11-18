// In-game overlay: Dear ImGui on SDL3 + OpenGL 3.3 (D1).  See include/overlay.h.
#include "overlay.h"

#include <SDL3/SDL.h>
#include <stdio.h>

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_opengl3.h"

extern "C" long g_dbg_frame_counter;   // swapped frames (platform_sdl2.c)

namespace {

SDL_Window*     s_window;
OverlayRenderer s_kind;
bool            s_ready;          // ImGui + backends initialised (GL only)
bool            s_open;
bool            s_frame_started;
bool            s_warned_unavailable;
bool            s_start_open;     // --overlay-open pending (applied at init)
long            s_open_from = -1, s_open_to = -1;
bool            s_show_demo;

void warn_unavailable() {
    if (s_warned_unavailable) return;
    s_warned_unavailable = true;
    printf("[overlay] not available on Direct3D 12 yet (D2); overlay stays closed\n");
}

void build_ui() {
    ImGui::SetNextWindowPos(ImVec2(40, 40), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(420, 260), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("SMB2 overlay (placeholder)")) {
        static float volume = 0.8f;
        static int   mode = 0;
        static bool  check = true;
        ImGui::Text("Dear ImGui %s - F2 toggles", IMGUI_VERSION);
        ImGui::Text("Frame %ld, %.1f FPS", g_dbg_frame_counter, ImGui::GetIO().Framerate);
        ImGui::Separator();
        ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f);
        ImGui::Combo("Mode", &mode, "Windowed\0Borderless\0Fullscreen\0");
        ImGui::Checkbox("Example option", &check);
        if (ImGui::Button("Close")) s_open = false;
        ImGui::SameLine();
        ImGui::Checkbox("Show ImGui demo", &s_show_demo);
    }
    ImGui::End();
    if (s_show_demo) ImGui::ShowDemoWindow(&s_show_demo);
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
    if (renderer != OVERLAY_RENDERER_GL) {
        if (s_start_open) warn_unavailable();
        return false;
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;     // no imgui.ini next to the exe
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    if (!ImGui_ImplSDL3_InitForOpenGL(window, SDL_GL_GetCurrentContext()) ||
        !ImGui_ImplOpenGL3_Init("#version 330 core")) {
        fprintf(stderr, "[overlay] ImGui backend init failed\n");
        ImGui::DestroyContext();
        return false;
    }
    s_ready = true;
    if (s_start_open) overlay_set_open(true);
    printf("[overlay] Dear ImGui %s ready (OpenGL)\n", IMGUI_VERSION);
    return true;
}

void overlay_shutdown(void) {
    if (!s_ready) return;
    if (s_frame_started) { ImGui::EndFrame(); s_frame_started = false; }
    ImGui_ImplOpenGL3_Shutdown();
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

void overlay_new_frame(void) {
    if (!s_ready || !s_open || s_frame_started) return;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
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
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    s_frame_started = false;
}

bool overlay_is_open(void) { return s_open; }

void overlay_set_open(bool open) {
    if (open && !s_ready) { if (s_kind == OVERLAY_RENDERER_D3D12) warn_unavailable(); return; }
    s_open = open;
}

void overlay_toggle(void) { overlay_set_open(!s_open); }

bool overlay_wants_input(void) {
    if (!s_ready || !s_open) return false;
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse || io.WantCaptureKeyboard;
}

void overlay_test_open_at_start(void) {
    s_start_open = true;
    if (s_ready) overlay_set_open(true);
}

void overlay_test_open_frames(long from, long to) { s_open_from = from; s_open_to = to; }

}  // extern "C"

// In-game overlay: Dear ImGui on SDL3 + OpenGL 3.3 (D1) / Direct3D 12 (D2).  See include/overlay.h.
#include "overlay.h"

#include <SDL3/SDL.h>
#include <stdio.h>

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_opengl3.h"
#include "overlay_d3d12.h"
#include "overlay_ui.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
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


// ---- HUD: FPS counter + toasts (D5).  Drawn on the foreground draw list, so it works
// with the overlay closed too (an ImGui frame runs whenever something is visible).
struct Toast { int type; char title[64]; char body[200]; Uint64 t0; Uint64 dur; };
Toast s_toasts[5];
int   s_ntoasts;
struct TestToast { long frame; int type; char title[64]; char body[200]; bool done; };
TestToast s_test_toasts[4];
int   s_ntest_toasts;
constexpr Uint64 kFadeInMs = 200, kFadeOutMs = 400;

void prune_toasts() {
    const Uint64 now = SDL_GetTicks();
    int j = 0;
    for (int i = 0; i < s_ntoasts; i++)
        if (now - s_toasts[i].t0 < s_toasts[i].dur) s_toasts[j++] = s_toasts[i];
    s_ntoasts = j;
}

bool hud_visible() {
    prune_toasts();
    return s_ntoasts > 0 || g_pcs.show_fps > 0;
}

ImU32 with_alpha(ImU32 c, float a) {
    return (c & 0x00FFFFFFu) | ((ImU32)((float)(c >> 24) * a) << 24);
}

void draw_hud() {
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImGuiIO& io = ImGui::GetIO();
    const float u = ImGui::GetFontSize();
    const float s = u / 20.0f;                       // 1.0 at the base 20 px font
    const ImU32 kFill = IM_COL32(37, 59, 156, 235);  // partyboard: #253b9c
    const ImU32 kEdge = IM_COL32(0x00, 0x9d, 0xda, 255);
    const ImU32 kTxt = IM_COL32(0xE0, 0xDB, 0xC8, 255);
    const float rounding = 14.0f * s;
    float fps_bottom_tr = 0;
    if (g_pcs.show_fps > 0) {
        char buf[32];
        snprintf(buf, sizeof buf, "%d FPS", (int)(io.Framerate + 0.5f));
        const ImVec2 ts = ImGui::CalcTextSize(buf);
        const float padx = 12 * s, pady = 6 * s, m = 16 * s;
        const float w = ts.x + padx * 2, h = ts.y + pady * 2;
        const int c = g_pcs.show_fps;                // 1 TL, 2 TR, 3 BL, 4 BR
        const float x = (c == 2 || c == 4) ? io.DisplaySize.x - m - w : m;
        const float y = (c == 3 || c == 4) ? io.DisplaySize.y - m - h : m;
        dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), kFill, rounding * 0.6f);
        dl->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), kEdge, rounding * 0.6f, 0, 1.0f);
        dl->AddText(ImVec2(x + padx, y + pady), kTxt, buf);
        if (c == 2) fps_bottom_tr = y + h;
    }
    // toasts: top right, stacked downwards (oldest on top), under the FPS box when that sits there
    static const ImU32 kType[4] = {                  // info, success, warning, error
        IM_COL32(0x00, 0xb4, 0xfa, 255), IM_COL32(0x5e, 0xd6, 0x7a, 255),
        IM_COL32(0xC2, 0xA4, 0x2D, 255), IM_COL32(0xf0, 0x56, 0x6a, 255) };
    const Uint64 now = SDL_GetTicks();
    const float tw = u * 18.0f, padx = 24 * s * 0.8f, pady = 16 * s * 0.8f, gap = 10 * s;
    const float x1 = io.DisplaySize.x - 40 * s, x0 = x1 - tw;
    float y = (fps_bottom_tr > 0 ? fps_bottom_tr + gap : 40 * s);
    for (int i = 0; i < s_ntoasts; i++) {
        const Toast& t = s_toasts[i];
        const Uint64 age = now - t.t0;
        float a = 1.0f;
        if (age < kFadeInMs) a = (float)age / (float)kFadeInMs;
        else if (t.dur - age < kFadeOutMs) a = (float)(t.dur - age) / (float)kFadeOutMs;
        if (a <= 0) continue;
        char head[64];
        int k = 0;
        for (; t.title[k] && k < 63; k++) head[k] = (t.title[k] >= 'a' && t.title[k] <= 'z') ? t.title[k] - 32 : t.title[k];
        head[k] = 0;
        const float wrap = tw - padx * 2;
        const ImVec2 hs = ImGui::CalcTextSize(head, nullptr, false, wrap);
        const ImVec2 bs = t.body[0] ? ImGui::CalcTextSize(t.body, nullptr, false, wrap) : ImVec2(0, 0);
        const float h = pady * 2 + hs.y + (t.body[0] ? 8 * s + bs.y : 0);
        const ImU32 col = kType[t.type < 0 || t.type > 3 ? 0 : t.type];
        const float slide = (1.0f - a) * 12 * s;    // small slide-in / out
        const ImVec2 p0(x0 + slide, y), p1(x1 + slide, y + h);
        dl->AddRectFilled(ImVec2(p0.x + 3 * s, p0.y + 4 * s), ImVec2(p1.x + 3 * s, p1.y + 4 * s),
                          IM_COL32(0, 0, 0, (int)(90 * a)), rounding);   // soft shadow
        dl->AddRectFilled(p0, p1, with_alpha(kFill, a), rounding);
        dl->AddRect(p0, p1, with_alpha(t.type == 0 ? kEdge : col, a), rounding, 0, 1.0f);
        dl->AddText(nullptr, 0.0f, ImVec2(p0.x + padx, p0.y + pady), with_alpha(col, a), head, nullptr, wrap);
        if (t.body[0])
            dl->AddText(nullptr, 0.0f, ImVec2(p0.x + padx, p0.y + pady + hs.y + 8 * s), with_alpha(kTxt, a), t.body, nullptr, wrap);
        y += h + gap;
    }
}

void build_ui() {
    if (s_focus_pending) { ImGui::SetNextWindowFocus(); s_focus_pending = false; }
    ovl::build();
    if (ovl::close_requested()) overlay_set_open(false);
}

void apply_schedule() {
    for (int i = 0; i < s_ntest_toasts; i++)
        if (!s_test_toasts[i].done && g_dbg_frame_counter >= s_test_toasts[i].frame) {
            s_test_toasts[i].done = true;
            overlay_toast(s_test_toasts[i].type, s_test_toasts[i].title, s_test_toasts[i].body);
        }
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
    if (e->type == SDL_EVENT_KEY_DOWN && !e->key.repeat && (e->key.key == SDLK_F1 || e->key.key == SDLK_F2)) {   // F1; F2 stays as an alias
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
    if (!s_ready || s_frame_started) return;
    if (!s_open && !hud_visible()) return;   // nothing to draw: no ImGui frame at all
    if (s_kind == OVERLAY_RENDERER_GL) ImGui_ImplOpenGL3_NewFrame(); else overlay_d3d12_new_frame();
    {   // closed: the SDL backend must not touch the mouse cursor (overlay_poll_tick hides it when idle)
        ImGuiIO& cio = ImGui::GetIO();
        if (s_open) cio.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange; else cio.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    }
    ImGui_ImplSDL3_NewFrame();
    if (s_kind == OVERLAY_RENDERER_D3D12) {   // the DX12 backend draws in DisplaySize units = pixels
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize.x *= io.DisplayFramebufferScale.x; io.DisplaySize.y *= io.DisplayFramebufferScale.y;
        io.DisplayFramebufferScale = ImVec2(1, 1);
    }
    ImGui::NewFrame();
    s_frame_started = true;
    ImGui::GetIO().FontGlobalScale = ovl::scale_for(ImGui::GetIO().DisplaySize.y);
    if (s_open) build_ui();
    draw_hud();
}

void overlay_render(void) {
    apply_schedule();
    if (!s_ready) return;
    if (!s_open && !hud_visible()) {
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

// open, or an FPS counter / toast is on screen: the renderers must run the overlay draw path
bool overlay_wants_draw(void) { return s_ready && (s_open || hud_visible()); }

void overlay_toast(int type, const char* title, const char* body) {
    if (!s_ready) return;
    if (s_ntoasts == (int)(sizeof s_toasts / sizeof s_toasts[0])) {   // full: drop the oldest
        memmove(s_toasts, s_toasts + 1, sizeof s_toasts - sizeof s_toasts[0]);
        s_ntoasts--;
    }
    Toast& t = s_toasts[s_ntoasts++];
    t.type = type;
    snprintf(t.title, sizeof t.title, "%s", title ? title : "");
    snprintf(t.body, sizeof t.body, "%s", body ? body : "");
    t.t0 = SDL_GetTicks();
    t.dur = 4000 + (Uint64)strlen(t.body) * 30;
    printf("[overlay] toast (%d): %s - %s\n", type, t.title, t.body);
    fflush(stdout);
}

// "FRAME:type:Title:Body" (type 0..3 or info/success/warning/error)
void overlay_test_toast(const char* spec) {
    if (s_ntest_toasts >= 4) return;
    TestToast& t = s_test_toasts[s_ntest_toasts];
    char ty[16] = "";
    char title[64] = "", body[200] = "";
    long fr = 0;
    int n = 0;
    if (sscanf(spec, "%ld:%15[^:]:%63[^:]:%n", &fr, ty, title, &n) < 3) return;
    snprintf(body, sizeof body, "%s", spec + n);
    t.frame = fr;
    t.type = !strcmp(ty, "success") ? OVERLAY_TOAST_SUCCESS : !strcmp(ty, "warning") ? OVERLAY_TOAST_WARNING
           : !strcmp(ty, "error") ? OVERLAY_TOAST_ERROR : OVERLAY_TOAST_INFO;
    snprintf(t.title, sizeof t.title, "%s", title);
    snprintf(t.body, sizeof t.body, "%s", body);
    t.done = false;
    s_ntest_toasts++;
}

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

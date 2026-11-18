// Overlay UI shell: window, tab bar, left option list / right help pane, row helpers (D4).
// Look: loosely partyboard's res/rml/window.rcss + tabbing.rcss (blue panel, #009dda border,
// uppercase tabs with a #00b4fa underline, #E0DBC8 text, gold focus).
#include "overlay_ui.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "imgui.h"
#include "imgui_internal.h"

#include "pc_settings.h"

namespace ovl {

namespace {

struct Tab { const char* name; void (*fn)(); };
// One entry per tab.  E7: replace tab_online() (src/overlay/tab_online.cpp) with the lobby.
const Tab kTabs[] = {
    { "Video", tab_video },
    { "Audio", tab_audio },
    { "Input", tab_input },
    { "Game", tab_game },
    { "Online", tab_online },
    { "Interface", tab_interface },
};
constexpr int kNumTabs = (int)(sizeof kTabs / sizeof kTabs[0]);

int  s_tab;
bool s_focus_first_row;    // next tab body: put the nav focus on its first row
bool s_first_row_done;
bool s_close;
bool s_mouse_mode;         // help follows the mouse (true) or the nav focus (false)
char s_help_title[96];
char s_help_text[900];

const ImU32 kAccent = IM_COL32(0x00, 0xb4, 0xfa, 255);    // tab underline (#00b4fa)
const ImU32 kBorder = IM_COL32(0x00, 0x9d, 0xda, 255);    // window border (#009dda)
const ImU32 kText = IM_COL32(0xE0, 0xDB, 0xC8, 255);      // #E0DBC8
const ImU32 kGold = IM_COL32(0xC2, 0xA4, 0x2D, 255);      // focus / restart notes (#C2A42D)

float em() { return ImGui::GetFontSize(); }

bool nav_horizontal_move(int* dir) {
    ImGuiContext& g = *ImGui::GetCurrentContext();
    if (g.NavMoveSubmitted && (g.NavMoveDir == ImGuiDir_Left || g.NavMoveDir == ImGuiDir_Right)) {
        *dir = g.NavMoveDir == ImGuiDir_Left ? -1 : 1;
        ImGui::NavMoveRequestCancel();    // the row consumes Left/Right; focus stays
        return true;
    }
    return false;
}

struct RowOut { bool activated; int step; bool nav; };   // nav: Left/Right, not a press

// Shared row: label left, value right.  `arrows`: value is adjustable (< value > while focused).
RowOut row_core(const char* label, const char* value, bool enabled, bool adjustable,
                bool value_gold, const char* help, float bar01 = -1.0f) {
    RowOut out = { false, 0, false };
    ImGui::PushID(label);
    const float h = em() * 1.85f;
    const float w = ImGui::GetContentRegionAvail().x;
    ImVec2 p = ImGui::GetCursorScreenPos();
    if (s_focus_first_row && !s_first_row_done) {
        ImGui::SetKeyboardFocusHere();
        s_first_row_done = true;
        s_focus_first_row = false;
    }
    bool pressed = ImGui::Selectable("##row", false, ImGuiSelectableFlags_None, ImVec2(w, h));
    const bool focused = ImGui::IsItemFocused();
    const bool hovered = ImGui::IsItemHovered();
    if (enabled) {
        if (pressed) { out.activated = true; out.step = 1; }
        else if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) { out.activated = true; out.step = -1; }
        int dir;
        if (adjustable && focused && nav_horizontal_move(&dir)) { out.activated = true; out.step = dir; out.nav = true; }
    }
    if ((s_mouse_mode && hovered) || (!s_mouse_mode && focused)) set_help(label, help);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float ty = p.y + (h - em()) * 0.5f;
    const ImU32 col = enabled ? kText : IM_COL32(0xE0, 0xDB, 0xC8, 110);
    dl->AddText(ImVec2(p.x + em() * 0.8f, ty), col, label);
    if (value && value[0]) {
        char buf[128];
        if (adjustable && enabled && focused) snprintf(buf, sizeof buf, "<  %s  >", value);
        else snprintf(buf, sizeof buf, "%s", value);
        const ImVec2 sz = ImGui::CalcTextSize(buf);
        const float vx = p.x + w - em() * 0.8f - sz.x;
        dl->AddText(ImVec2(vx, ty), !enabled ? col : value_gold ? kGold : kAccent, buf);
        if (bar01 >= 0.0f) {   // slider bar to the left of the value
            const float bw = em() * 8.0f, bh = em() * 0.35f;
            const float bx = vx - em() * 1.0f - bw, by = p.y + (h - bh) * 0.5f;
            dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx + bw, by + bh), IM_COL32(224, 219, 200, 50), bh * 0.5f);
            if (bar01 > 0.0f)
                dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx + bw * bar01, by + bh), enabled ? kAccent : col, bh * 0.5f);
        }
    }
    ImGui::PopID();
    return out;
}

void draw_tab_bar() {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float h = em() * 2.6f;
    const float pad = em() * 1.3f;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0x00 / 255.f, 0xb4 / 255.f, 0xfa / 255.f, 0.15f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0x00 / 255.f, 0xb4 / 255.f, 0xfa / 255.f, 0.25f));
    const float x0 = ImGui::GetCursorScreenPos().x, y0 = ImGui::GetCursorScreenPos().y;
    const float full = ImGui::GetContentRegionAvail().x;
    for (int i = 0; i <= kNumTabs; i++) {
        const bool is_close = i == kNumTabs;
        char up[32];
        const char* name = is_close ? "Close" : kTabs[i].name;
        int n = 0;
        for (; name[n] && n < 31; n++) up[n] = (name[n] >= 'a' && name[n] <= 'z') ? (char)(name[n] - 32) : name[n];
        up[n] = 0;
        const float tw = ImGui::CalcTextSize(up).x + pad * 2;
        if (is_close) ImGui::SetCursorScreenPos(ImVec2(x0 + full - tw, y0));
        else if (i > 0) ImGui::SameLine();
        ImGui::PushID(i);
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const bool pressed = ImGui::Selectable("##tab", false, ImGuiSelectableFlags_None, ImVec2(tw, h));
        const bool focus = ImGui::IsItemFocused() || ImGui::IsItemHovered();
        const bool sel = !is_close && i == s_tab;
        ImU32 col = (sel || focus) ? kText : IM_COL32(0xE0, 0xDB, 0xC8, 100);
        dl->AddText(ImVec2(p.x + pad, p.y + (h - em()) * 0.5f), col, up);
        if (sel) dl->AddRectFilled(ImVec2(p.x, p.y + h - 4), ImVec2(p.x + tw, p.y + h), kAccent);
        if (pressed) {
            if (is_close) s_close = true;
            else if (i != s_tab) { s_tab = i; s_focus_first_row = true; s_first_row_done = false; s_help_title[0] = s_help_text[0] = 0; }
            else { s_focus_first_row = true; s_first_row_done = false; }
        }
        ImGui::PopID();
    }
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();
    // LB / RB switch tabs from anywhere
    int d = 0;
    if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false)) d = -1;
    if (ImGui::IsKeyPressed(ImGuiKey_GamepadR1, false)) d = 1;
    if (d) {
        s_tab = (s_tab + kNumTabs + d) % kNumTabs;
        s_focus_first_row = true; s_first_row_done = false;
        s_help_title[0] = s_help_text[0] = 0;
    }
}

}  // namespace

float scale_for(float display_h) {
    float s = display_h / 1000.0f;
    return s < 0.8f ? 0.8f : s > 1.8f ? 1.8f : s;
}

void style_init() {
    ImGuiIO& io = ImGui::GetIO();
    static const char* const fonts[] = { "C:/Windows/Fonts/segoeui.ttf", "C:/Windows/Fonts/arial.ttf" };
    bool loaded = false;
    for (const char* f : fonts)
        if (!loaded && io.Fonts->AddFontFromFileTTF(f, 20.0f)) loaded = true;
    if (!loaded) {
        ImFontConfig cfg;
        cfg.SizePixels = 20.0f;
        io.Fonts->AddFontDefault(&cfg);
    }
    ImGuiStyle& st = ImGui::GetStyle();
    ImGui::StyleColorsDark(&st);
    ImVec4* c = st.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(37 / 255.f, 59 / 255.f, 156 / 255.f, 0.94f);
    c[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_Border] = ImVec4(0x00 / 255.f, 0x9d / 255.f, 0xda / 255.f, 1.0f);
    c[ImGuiCol_Text] = ImVec4(0xE0 / 255.f, 0xDB / 255.f, 0xC8 / 255.f, 1.0f);
    c[ImGuiCol_TextDisabled] = ImVec4(0xE0 / 255.f, 0xDB / 255.f, 0xC8 / 255.f, 0.55f);
    c[ImGuiCol_Header] = ImVec4(0xC2 / 255.f, 0xA4 / 255.f, 0x2D / 255.f, 0.35f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0xC2 / 255.f, 0xA4 / 255.f, 0x2D / 255.f, 0.30f);
    c[ImGuiCol_HeaderActive] = ImVec4(0xC2 / 255.f, 0xA4 / 255.f, 0x2D / 255.f, 0.50f);
    c[ImGuiCol_Separator] = ImVec4(0x00 / 255.f, 0x9d / 255.f, 0xda / 255.f, 1.0f);
    c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab] = ImVec4(224 / 255.f, 219 / 255.f, 200 / 255.f, 0.45f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(194 / 255.f, 164 / 255.f, 45 / 255.f, 0.8f);
    c[ImGuiCol_ScrollbarGrabActive] = ImVec4(194 / 255.f, 164 / 255.f, 45 / 255.f, 1.0f);
    c[ImGuiCol_NavCursor] = ImVec4(0xC2 / 255.f, 0xA4 / 255.f, 0x2D / 255.f, 1.0f);
    st.WindowBorderSize = 2.0f;
    st.WindowRounding = 14.0f;
    st.ScrollbarSize = 8.0f;
    st.ScrollbarRounding = 2.0f;
    st.FrameRounding = 6.0f;
    st.ItemSpacing = ImVec2(8, 4);
    st.WindowPadding = ImVec2(0, 0);
}

void on_open() {
    s_focus_first_row = true;
    s_first_row_done = false;
    s_close = false;
    s_help_title[0] = s_help_text[0] = 0;
}

void select_tab(const char* name) {
    for (int i = 0; i < kNumTabs; i++)
        if (!strcasecmp(kTabs[i].name, name)) { s_tab = i; s_focus_first_row = true; s_first_row_done = false; }
}

void request_close() { s_close = true; }
bool close_requested() { bool c = s_close; s_close = false; return c; }

void set_help(const char* title, const char* text) {
    if (strcmp(s_help_title, title ? title : "") != 0) snprintf(s_help_title, sizeof s_help_title, "%s", title ? title : "");
    if (strcmp(s_help_text, text ? text : "") != 0) snprintf(s_help_text, sizeof s_help_text, "%s", text ? text : "");
}

void build() {
    ImGuiIO& io = ImGui::GetIO();
    ImGuiContext& g = *ImGui::GetCurrentContext();
    io.FontGlobalScale = scale_for(io.DisplaySize.y);
    // help follows whichever input was used last
    if (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f) s_mouse_mode = true;
    else if (g.NavMoveSubmitted || ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false) || ImGui::IsKeyPressed(ImGuiKey_GamepadR1, false)) s_mouse_mode = false;

    const float u = em();
    const float dw = io.DisplaySize.x, dh = io.DisplaySize.y;
    // dim the game behind the panel
    ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0, 0), ImVec2(dw, dh), IM_COL32(0, 0, 0, 50));
    float ww = dw - u * 4, wh = dh - u * 4;
    if (ww > u * 58) ww = u * 58;
    if (wh > u * 36) wh = u * 36;
    ImGui::SetNextWindowSize(ImVec2(ww, wh));
    ImGui::SetNextWindowPos(ImVec2(dw * 0.5f, dh * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    const ImGuiWindowFlags wf = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                                ImGuiWindowFlags_NoScrollWithMouse;
    if (!ImGui::Begin("SMB2 PC Port##overlay", nullptr, wf)) { ImGui::End(); return; }

    // tab bar (translucent strip, accent line under it)
    {
        ImVec2 p = ImGui::GetWindowPos();
        ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + ww, p.y + u * 2.6f), IM_COL32(217, 217, 217, 26), 14.0f, ImDrawFlags_RoundCornersTop);
        ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x, p.y + u * 2.6f), ImVec2(p.x + ww, p.y + u * 2.6f), kBorder, 2.0f);
    }
    draw_tab_bar();

    const float footer_h = u * 2.2f;
    const float body_y = u * 2.6f + 2.0f;
    const float body_h = wh - body_y - footer_h;
    const float left_w = ww * 0.58f;
    ImGui::SetCursorPos(ImVec2(0, body_y));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(u * 1.2f, u * 1.0f));
    const ImGuiChildFlags cf = ImGuiChildFlags_NavFlattened | ImGuiChildFlags_AlwaysUseWindowPadding;
    if (ImGui::BeginChild("##left", ImVec2(left_w, body_h), cf, ImGuiWindowFlags_None)) {
        kTabs[s_tab].fn();
        ImGui::Dummy(ImVec2(0, u * 0.5f));
        section("");
        if (button_row("Close overlay", nullptr,
                       "Close this window and go back to the game. F2, the Guide button or Back + Start (hold) also close it. "
                       "B only steps back out of a control, so use this row on a gamepad.")) request_close();
    }
    ImGui::EndChild();
    ImGui::SetCursorPos(ImVec2(left_w, body_y));
    {
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x, p.y), ImVec2(p.x, p.y + body_h), kBorder, 1.0f);
    }
    if (ImGui::BeginChild("##right", ImVec2(ww - left_w, body_h), cf, ImGuiWindowFlags_None)) {
        if (s_help_title[0]) {
            ImGui::PushStyleColor(ImGuiCol_Text, kGold);
            ImGui::TextUnformatted(s_help_title);
            ImGui::PopStyleColor();
            ImGui::Dummy(ImVec2(0, u * 0.3f));
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextUnformatted(s_help_text);
            ImGui::PopTextWrapPos();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextUnformatted("Pick an option on the left to see what it does.");
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();

    // footer hints
    {
        ImVec2 p = ImGui::GetWindowPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddLine(ImVec2(p.x, p.y + wh - footer_h), ImVec2(p.x + ww, p.y + wh - footer_h), kBorder, 1.0f);
        const char* hint = "Up/Down: select    Left/Right or A: change    LB/RB: tab    B: back    F2 / Guide: close";
        ImVec2 sz = ImGui::CalcTextSize(hint);
        dl->AddText(ImVec2(p.x + (ww - sz.x) * 0.5f, p.y + wh - footer_h + (footer_h - u) * 0.5f),
                    IM_COL32(0xE0, 0xDB, 0xC8, 150), hint);
    }
    ImGui::End();
}

// ---------------------------------------------------------------- rows
void section(const char* title) {
    const float u = em();
    if (title && title[0]) {
        ImGui::Dummy(ImVec2(0, u * 0.3f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + u * 0.8f);
        char up[48];
        int n = 0;
        for (; title[n] && n < 47; n++) up[n] = (title[n] >= 'a' && title[n] <= 'z') ? (char)(title[n] - 32) : title[n];
        up[n] = 0;
        ImGui::TextUnformatted(up);
        ImGui::PopStyleColor();
    } else {
        ImVec2 p = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;
        ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x, p.y), ImVec2(p.x + w, p.y), IM_COL32(0x00, 0x9d, 0xda, 120), 1.0f);
        ImGui::Dummy(ImVec2(0, u * 0.4f));
    }
}

void note(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGui::PushTextWrapPos(0.0f);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + em() * 0.8f);
    ImGui::TextUnformatted(text);
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();
}

bool choice_row(const char* label, int item, const char* help) {
    const int n = pcs_choice_count(item);
    const int cur = pcs_choice_current(item);
    const bool enabled = pcs_choice_enabled(item) && n > 0;
    char value[64];
    if (item == PCS_ITEM_RESOLUTION && !enabled) snprintf(value, sizeof value, "%s", pcs_item_value(item));
    else if (cur >= 0) snprintf(value, sizeof value, "%s", pcs_choice_label(item, cur));
    else if (item == PCS_ITEM_RESOLUTION) snprintf(value, sizeof value, "%dx%d", g_pcs.width, g_pcs.height);
    else snprintf(value, sizeof value, "Custom");
    const bool restart = pcs_item_restart_pending(item);
    if (restart) strncat(value, "  (restart to apply)", sizeof value - strlen(value) - 1);
    RowOut r = row_core(label, value, enabled, true, restart, help);
    if (r.activated && n > 0) {
        const int to = cur < 0 ? 0 : (cur + n + r.step) % n;
        pcs_choice_set(item, to);
        return true;
    }
    return false;
}

bool volume_row(const char* label, const char* help) {
    char value[16];
    snprintf(value, sizeof value, "%d%%", g_pcs.volume);
    RowOut r = row_core(label, value, true, true, false, help, g_pcs.volume / 100.0f);
    if (r.activated) {
        int v = g_pcs.volume + r.step * 10;   // the Game Screen menu steps by 10
        if (v > 100) v = r.nav ? 100 : 0;     // A / click at 100 wraps to 0; Left / Right clamp
        if (v < 0) v = 0;
        pcs_set_volume(v);
        return true;
    }
    return false;
}

bool button_row(const char* label, const char* value, const char* help, bool enabled) {
    RowOut r = row_core(label, value, enabled, false, false, help);
    return r.activated && r.step > 0;
}

void info_row(const char* label, const char* value, const char* help) {
    row_core(label, value, true, false, false, help);
}

}  // namespace ovl

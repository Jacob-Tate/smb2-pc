// Overlay UI shell and row helpers (D4).  Internal to src/overlay/.
//
// Layout (after partyboard's overlay window): tab bar on top, option list on the
// left, help text for the focused option on the right.  Each tab is ONE function
// in its own file (tab_video.cpp, tab_audio.cpp, ...); the shell calls it inside
// the left pane.  To add or replace a tab (E7: Online) edit kTabs in overlay_ui.cpp
// and put the function in src/overlay/tab_<name>.cpp (add the file to CMakeLists).
#pragma once

namespace ovl {

// ---- tab functions (one per file) ----
void tab_video();
void tab_audio();
void tab_input();
void tab_game();
void tab_online();      // placeholder until E7 (tab_online.cpp)
void tab_interface();

// ---- shell (overlay_ui.cpp) ----
void build();                 // the whole overlay UI for this frame (call between NewFrame and Render)
void on_open();               // overlay just opened: first tab row gets focus
void request_close();         // close the overlay after this frame
bool close_requested();       // polled by overlay.cpp (consumes the flag)
void style_init();            // fonts + colours; call once after CreateContext, before the first frame
float scale_for(float display_h);

// ---- rows (valid inside a tab function) ----
// Every row takes `help`: shown in the right pane while the row has focus / is hovered.
void section(const char* title);
void note(const char* text);                       // dim wrapped text, not focusable
// Pick-one row bound to pcs_choice_*(item).  A / click: next, right click: previous,
// Left / Right (d-pad, stick, keys): previous / next.  Returns true if the value changed.
bool choice_row(const char* label, int item, const char* help);
// 0..100 row in steps of 10 bound to the master volume (same as the Game Screen menu).
bool volume_row(const char* label, const char* help);
// Plain action row (optionally with a value text on the right).  Returns true when activated.
bool button_row(const char* label, const char* value, const char* help, bool enabled = true);
// Read-only row (label + value), focusable so its help can be read.
void info_row(const char* label, const char* value, const char* help);
// Right-pane text for the current row (copied); the row helpers call this.
void set_help(const char* title, const char* text);

}  // namespace ovl

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
void select_tab(const char* name);   // test hook: --overlay-test-set tab=<Name>
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
// Adjustable row with a free value text (E7): A / click / Right = +1, Left / right click = -1.
// Returns 0 when nothing happened, else the step (+1 / -1).
int stepper_row(const char* label, const char* value, const char* help, bool enabled = true);
// Like button_row but the value can be drawn gold (highlight, e.g. "you") (E7).
bool action_row(const char* label, const char* value, const char* help, bool enabled = true, bool gold = false);
// Editable text row (E7): shows `buf`; A / click / Enter starts editing with the keyboard (typing, Ctrl+V paste;
// Enter or B/Esc ends).  `upper_code`: letters/digits only, upper-cased (room codes).  Returns true when the
// value was changed and the edit finished.
bool text_row(const char* label, char* buf, int cap, const char* help, bool upper_code = false);
// True while the text row `label` is being edited (the shell then skips tab-wide hotkeys).
bool text_editing();
// E7: true while the online lobby is joined or a session runs: modal screens and sim-affecting settings are disabled
// (tab_online.cpp).
bool online_locked();

// ---- online actions (tab_online.cpp): the buttons AND the test script call these ----
void online_connect();                // connects with the server / name fields (saves them in settings.dat)
void online_disconnect();
void online_host();                   // create a room (delay from the Input delay row)
void online_join();                   // join the room whose code is in the Room code field
void online_pick_port(int port);      // 0..3
void online_start();                  // host
void online_leave_room();
void online_leave_session();          // deferred to the next online_tick (never from inside an ImGui frame)
void online_step_delay(int step);     // Input delay row: auto, 2..12 frames
void online_code_letter(int i, int step);   // gamepad letter picker of the Room code
void online_set_code(const char* code);
void online_set_server_name(const char* server, const char* name);
bool online_script_set(const char* spec);   // --overlay-test-online-script
void online_tick();                   // once per platform_poll_events: lobby events -> toasts, test script

}  // namespace ovl

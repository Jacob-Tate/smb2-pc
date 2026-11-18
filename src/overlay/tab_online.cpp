// Online tab: placeholder until E7 (lobby UI).  E7 replaces this file's body; keep the
// function name tab_online() (registered in kTabs in overlay_ui.cpp).
#include "overlay_ui.h"

namespace ovl {

void tab_online() {
    section("Online play");
    info_row("Status", "Coming soon",
        "Online multiplayer is not available yet. When it is, you will host or join a room with a 4-letter code here.");
    note("Lobby, rooms and player list will appear on this tab.");
}

}  // namespace ovl

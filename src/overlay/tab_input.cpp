// Input tab (D4): controller list, button prompts, stick settings, controller DB folder.
#include "overlay_ui.h"

#include <SDL3/SDL.h>
#include <stdio.h>

#include "pad_config.h"
#include "pc_settings.h"

extern "C" int pc_open_folder(const char* path);

namespace ovl {

void tab_input() {
    section("Controllers");
    for (int port = 0; port < 4; port++) {
        char label[24], value[96];
        snprintf(label, sizeof label, "Player %d", port + 1);
        if (input_port_connected(port)) snprintf(value, sizeof value, "%s", input_port_name(port));
        else snprintf(value, sizeof value, port == 0 ? "Keyboard / none" : "Not connected");
        info_row(label, value,
            "Controller assigned to this player port. Plug in a controller to use ports 2-4; "
            "player 1 can always use the keyboard.");
    }
    section("Prompts and sticks");
    choice_row("Button prompts", PCS_ITEM_BUTTONS,
        "Which button icons the game shows. Auto follows the controller in use.");
    if (button_row("Controller sticks...", nullptr,
            "Opens the Controller Sticks screen: GameCube gate shape, dead zone, sensitivity and calibration per controller. "
            "This closes the overlay. Not available while you are in an online lobby or session.", !online_locked())) {
        pc_stick_screen_request();
        request_close();
    }
    if (button_row("Open controller DB folder", nullptr,
            "Opens the folder with the game. A gamecontrollerdb.txt placed there adds mappings for controllers the game does not know.")) {
        const char* base = SDL_GetBasePath();
        if (base) pc_open_folder(base);
    }
}

}  // namespace ovl

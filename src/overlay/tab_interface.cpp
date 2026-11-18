// Interface tab (D4): data folder, version, quit.
#include "overlay_ui.h"

#include <stdio.h>

#include "build_id.h"
extern "C" {
#include "pc_quit.h"
}
#include "pc_settings.h"
#include "pc_update.h"
#include "platform.h"

extern "C" int pc_open_folder(const char* path);

namespace ovl {

namespace {
bool s_confirm_quit;
}

void tab_interface() {
    section("About");
    {
        char v[96];
        snprintf(v, sizeof v, "%s", pc_update_current_version());
        info_row("Version", v, "Version of this PC port.");
        info_row("Build", smb2_build_id(), "Build number and commit of this exe. Quote it when reporting problems.");
        info_row("Renderer in use", pcs_active_renderer() == PCS_RENDERER_D3D12 ? "Direct3D 12" : "OpenGL",
                 "Graphics API this run started with.");
    }

    section("Files");
    if (button_row("Open data folder", nullptr,
            "Opens the folder with settings.dat, controller settings and the memory card.")) {
        pc_open_folder(platform_get_save_dir());
    }

    section("Exit");
    if (!s_confirm_quit) {
        if (button_row("Quit to desktop", nullptr, "Closes the game. Unsaved progress is lost.")) s_confirm_quit = true;
    } else {
        if (button_row("Yes, quit the game", nullptr, "Closes the game now.")) { pc_quit_request("overlay"); request_close(); }
        if (button_row("No, keep playing", nullptr, "Cancel.")) s_confirm_quit = false;
    }
}

}  // namespace ovl

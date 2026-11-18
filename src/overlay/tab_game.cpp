// Game tab (D4): language, disc image, memory card, updates.
#include "overlay_ui.h"

#include <stdio.h>

#include "pc_settings.h"
extern "C" {
#include "pc_memcard_ui.h"
}
#include "pc_update.h"

namespace ovl {

namespace {

const char* update_status(char* buf, size_t n) {
    switch (pc_update_state()) {
    case PCU_IDLE: return "Not checked yet";
    case PCU_CHECKING: return "Checking...";
    case PCU_UP_TO_DATE: return "Up to date";
    case PCU_AVAILABLE: {
        const PcuRelease* r = pc_update_release();
        snprintf(buf, n, "%s available", r ? r->version : "New version");
        return buf;
    }
    case PCU_CHECK_FAILED: return "Check failed";
    case PCU_DOWNLOADING: return "Downloading...";
    case PCU_VERIFYING: return "Verifying...";
    case PCU_DOWNLOAD_FAILED: return "Download failed";
    case PCU_LAUNCHING: case PCU_LAUNCHED: return "Starting setup...";
    case PCU_LAUNCH_FAILED: return "Could not start setup";
    }
    return "";
}

}  // namespace

void tab_game() {
    section("Game");
    if (online_locked())
        info_row("Language", "locked online",
            "Online sessions force the same game language on every player, so it cannot be changed while you are in an online lobby or session.");
    else choice_row("Language", PCS_ITEM_LANGUAGE,
        "Game language. Auto follows the Windows UI language (Japanese, otherwise English). "
        "Takes effect after you restart the game.");

    section("Disc image");
    {
        const char* iso = g_pcs.iso_path[0] ? g_pcs.iso_path : "(none selected)";
        info_row("Disc image", iso,
            "The GameCube disc image (.iso, .gcm or .rvz) the game loads its data from. Changing it takes effect after a restart.");
        if (button_row("Choose a different disc image", nullptr,
                "Forgets the saved disc image path. The next time you start the game it asks for the disc image again "
                "(restart required). Not available while you are in an online lobby or session.", !online_locked())) {
            pcs_set_iso_path("");
        }
    }

    section("Memory card");
    if (button_row("Import / export saves...", nullptr,
            "Opens the Memory Card screen: import a save from Dolphin or a GameCube memory card, or export yours. Not available online. "
            "This closes the overlay.", !online_locked()))
    {
        pc_memcard_screen_request();
        request_close();
    }

    section("Updates");
    if (button_row("Check for updates at start", g_pcs.update_check ? "On" : "Off",
            "Looks for a new version when the game starts, at most once a day.")) {
        pcs_set_update_check(!g_pcs.update_check);
    }
    {
        char buf[64];
        info_row("Status", update_status(buf, sizeof buf), "Result of the last update check.");
    }
    if (button_row("Check now", nullptr, "Look for a newer version right now.", pc_update_enabled() != 0))
        pc_update_check_now();
    if (button_row("Updates screen...", nullptr,
            "Opens the Updates screen with release notes, download and install. This closes the overlay. Not available online.",
            pc_update_enabled() != 0 && !online_locked())) {
        pc_update_screen_request();
        request_close();
    }
}

}  // namespace ovl

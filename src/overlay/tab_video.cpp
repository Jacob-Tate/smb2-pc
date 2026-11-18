// Video tab (D4): every row is the matching Options > Game Screen item (pc_settings choice API).
#include "overlay_ui.h"
#include "pc_settings.h"

namespace ovl {

void tab_video() {
    section("Display");
    choice_row("Display mode", PCS_ITEM_DISPLAY,
        "Windowed, exclusive fullscreen or borderless fullscreen (a window the size of the desktop). Alt+Enter switches too.");
    choice_row("Resolution", PCS_ITEM_RESOLUTION,
        "Window size, or the display mode in exclusive fullscreen. Borderless always uses the desktop resolution.");
    choice_row("Aspect ratio", PCS_ITEM_ASPECT,
        "Auto keeps 4:3 or 16:9 to match the game's TV Monitor option. Stretch fills the whole window.");
    if (button_row("Restore default window size", nullptr,
            "Go back to windowed mode at the default size for this desktop (1280x720, scaled for high-DPI screens)."))
        pcs_reset_window_size();

    section("Performance");
    choice_row("Frame rate", PCS_ITEM_FPS,
        "Frames drawn per second. The game still runs at 60; the frames in between are interpolated. "
        "Display matches your monitor's refresh rate.");
    choice_row("V-Sync", PCS_ITEM_VSYNC, "Sync frames to the display refresh. Turn off for the lowest latency (tearing possible).");
    choice_row("Anti-aliasing", PCS_ITEM_MSAA, "Multisample anti-aliasing. Takes effect after you restart the game.");
    choice_row("Renderer", PCS_ITEM_RENDERER,
        "Graphics API: OpenGL or Direct3D 12. Takes effect after you restart the game.");

    section("Overlay");
    choice_row("Show FPS", PCS_CHOICE_SHOW_FPS,
        "Corner of the screen for the frame rate counter. The counter itself is drawn by the on-screen display.");
}

}  // namespace ovl

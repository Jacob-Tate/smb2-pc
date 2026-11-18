// Audio tab (D4).
#include "overlay_ui.h"

namespace ovl {

void tab_audio() {
    section("Sound");
    volume_row("Volume", "Master volume for music and sound effects, in steps of 10.");
}

}  // namespace ovl

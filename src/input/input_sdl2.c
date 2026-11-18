/* SDL3 Input Backend (gamepad API; file name kept from the SDL2 days)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 *
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * Controllers are mapped onto the GameCube pad by button position (Nintendo
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * pads by label, see below):
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 *   south / east / west / north face button = GC A / B / X / Y
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 *   LT / RT (L2 / R2, ZL / ZR) = GC L / R: the analog value, plus the
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 *     digital L / R click at the end of the travel, as on a GC trigger
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 *   RB (R1, R)                 = GC Z
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 *   LB (L1, L)                 = unused (the GC pad has nothing there)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 *   Start (Options, Menu, +)   = START/PAUSE
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 *   left stick / right stick / d-pad = control stick / C stick / +Control Pad
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * Face buttons map by the label printed on the pad (SDL_GetGamepadButtonLabel),
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * so a Switch pad maps like Nintendo's own ports: the button labelled A (right)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * is GC A.  See sdl_button_to_pad.
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 *
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * Pads are hot-plugged: SDL_EVENT_GAMEPAD_ADDED / SDL_EVENT_JOYSTICK_ADDED open a
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * new pad in the first free port (1-4), *_REMOVED closes it and frees its
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * port; the other players keep theirs.  Joysticks without an SDL controller
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * mapping get a generic DirectInput-style mapping.  The keyboard always
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * drives port 1 as well. */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#include "input.h"
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#include "button_prompts.h"
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#include "gc_stick.h"
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#include "pad_config.h"
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#include "netplay/inputsync.h"
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#include "overlay.h"
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#include <SDL3/SDL.h>
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#include <stdlib.h>
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#include <string.h>
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#include <stdio.h>
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#include <math.h>
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* Global state matching original game */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
ControllerInfo pad_status_groups[MAX_CONTROLLERS] = {0};
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
uint16_t analog_inputs[MAX_CONTROLLERS][5] = {0};
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* The game's player -> pad table.  ball.c keeps the GC idle-chatter counters
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * (lbl_1_bss_70F00[10]) right behind it as one object (boat reads [0..7]),
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * so the single definition must hold all 4 + 10 entries. */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
int32_t player_pad_map[4 + 10] = {0, 1, 2, 3};
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* held / previous / pressed / released / repeat, as the game indexes them
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * (0xC bytes each in the DOL); with 4 entries [4] of the digital one landed
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * on the analog one's [0] and the held stick directions were lost */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
uint16_t merged_digital_inputs[6] = {0};
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
uint16_t merged_analog_inputs[6] = {0};
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
int32_t frames_since_last_input_change[MAX_CONTROLLERS] = {0};
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* One GC pad port */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
typedef struct {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDL_Gamepad* gc;          /* pad with an SDL gamepad mapping, or */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDL_Joystick* joy;        /* a raw joystick (generic mapping) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    int32_t id;               /* SDL joystick instance id; -1 = port empty */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    int type;                 /* SDL_GamepadType; -1 for raw joysticks */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    uint8_t trig_click[2];    /* digital L / R state of the triggers */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    uint16_t engaged;         /* buttons / deflected axes last frame */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    char guid[40];            /* SDL GUID: key of the stick settings */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    GcsPadConfig cfg;         /* stick settings / calibration (controllers.ini) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    PadStickView view;        /* last frame's sticks, for the calibration screen */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
} PadPort;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static PadPort s_ports[MAX_CONTROLLERS] = { { .id = -1 }, { .id = -1 }, { .id = -1 }, { .id = -1 } };
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* The input device used last, for the "auto" button prompts: a port, the
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * keyboard, or none yet. */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#define ACTIVE_NONE     (-1)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#define ACTIVE_KEYBOARD MAX_CONTROLLERS
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static int s_active = ACTIVE_NONE;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static bool s_keyboard_fallback = false;   /* legacy input_on_key_* API */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* The Esc key went down in this poll (keyboard or the ESC script token). */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static int s_esc_pressed;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
int pc_input_esc_pressed(void) { return s_esc_pressed; }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* GC trigger: analog 0..0x8C as the game sees it (after the SDK rescale), the
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * digital click near the end of the travel.  Press at 85 %, release below
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * 70 % (hysteresis, so a trigger held at the threshold does not chatter). */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#define TRIGGER_MAX       0x8C
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#define TRIGGER_CLICK_ON  27852   /* 0.85 * 32767 */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#define TRIGGER_CLICK_OFF 22937   /* 0.70 * 32767 */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* Keyboard to gamepad mapping for player 1 (legacy input_on_key_* API; the
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * live keyboard mapping is kbmap in input_poll_gc_pads) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static int s_keyboard_map[INPUT_GAMEPAD_MAX] = {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDLK_Z,       /* A */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDLK_X,       /* B */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDLK_C,       /* X */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDLK_V,       /* Y */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDLK_ESCAPE,  /* BACK */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDLK_TAB,     /* GUIDE */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDLK_RETURN,  /* START */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDLK_LSHIFT,  /* LEFT_STICK */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDLK_RSHIFT,  /* RIGHT_STICK */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDLK_Q,       /* LEFT_SHOULDER (L) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDLK_E,       /* RIGHT_SHOULDER (R) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDLK_UP,      /* DPAD_UP */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDLK_DOWN,    /* DPAD_DOWN */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDLK_LEFT,    /* DPAD_LEFT */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDLK_RIGHT,   /* DPAD_RIGHT */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
};
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static const char* type_name(int type) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    switch (type) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    case SDL_GAMEPAD_TYPE_STANDARD: return "standard";
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    case SDL_GAMEPAD_TYPE_XBOX360: return "Xbox 360";
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    case SDL_GAMEPAD_TYPE_XBOXONE: return "Xbox One/Series";
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    case SDL_GAMEPAD_TYPE_PS3: return "PS3";
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    case SDL_GAMEPAD_TYPE_PS4: return "PS4";
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    case SDL_GAMEPAD_TYPE_PS5: return "PS5";
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO: return "Switch Pro";
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT: return "Joy-Con (L)";
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT: return "Joy-Con (R)";
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR: return "Joy-Con pair";
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    case SDL_GAMEPAD_TYPE_GAMECUBE: return "GameCube";
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    case -1: return "joystick, no controller mapping";
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    return "other";
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static const char* pad_name(const PadPort* p) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    const char* n = p->gc ? SDL_GetGamepadName(p->gc) : p->joy ? SDL_GetJoystickName(p->joy) : NULL;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    return n ? n : "pad";
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static void clear_port_status(int port) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    PADStatus* s = &pad_status_groups[port].status[0];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    s->button = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    s->stickX = s->stickY = s->substickX = s->substickY = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    s->triggerLeft = s->triggerRight = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    s->analogA = s->analogB = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* A device appeared (SDL joystick instance id): open it in the first free port.
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * Called for every pad at start-up and again for SDL's ADDED events, which
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * also arrive for pads present at start-up, so known pads are skipped. */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* Script token VADD*2: the script's virtual pad skips port 1 (so the
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * keyboard/script keeps player 1 and the virtual pad is player 2). */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static int s_vpad_skip_port1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static int s_announce;   /* D5: toast on connect/disconnect (off while input_init opens the start-up pads) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
void input_on_device_added(unsigned int instance_id) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDL_JoystickID id = (SDL_JoystickID)instance_id;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    PadPort* p = NULL;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    const char* dname;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    int i;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (id == 0) return;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    for (i = 0; i < MAX_CONTROLLERS; i++)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (s_ports[i].id == (int32_t)id) return;   /* already open */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    for (i = (s_vpad_skip_port1 && SDL_IsJoystickVirtual(id)) ? 1 : 0; i < MAX_CONTROLLERS; i++)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (s_ports[i].id < 0) { p = &s_ports[i]; break; }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    dname = SDL_GetJoystickNameForID(id);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (!p) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        static int32_t s_told = -1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (s_told != (int32_t)id)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            printf("[input] %s connected, but all 4 ports are in use\n", dname ? dname : "pad");
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        s_told = (int32_t)id;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        return;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    memset(p, 0, sizeof *p);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    p->id = -1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (SDL_IsGamepad(id)) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        SDL_Joystick* gj;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        p->gc = SDL_OpenGamepad(id);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (!p->gc) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            fprintf(stderr, "[input] cannot open controller %u: %s\n", (unsigned)id, SDL_GetError());
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            return;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        p->type = (int)SDL_GetGamepadType(p->gc);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        gj = SDL_GetGamepadJoystick(p->gc);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        p->id = (int32_t)SDL_GetJoystickID(gj);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        SDL_GUIDToString(SDL_GetJoystickGUID(gj), p->guid, sizeof p->guid);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    } else {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        char guid[64];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        p->joy = SDL_OpenJoystick(id);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (!p->joy) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            fprintf(stderr, "[input] cannot open joystick %u: %s\n", (unsigned)id, SDL_GetError());
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            return;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        p->type = -1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        p->id = (int32_t)SDL_GetJoystickID(p->joy);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        SDL_GUIDToString(SDL_GetJoystickGUID(p->joy), guid, sizeof guid);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        snprintf(p->guid, sizeof p->guid, "%s", guid);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        printf("[input] no controller mapping for GUID %s: using a generic layout "
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
               "(a gamecontrollerdb.txt next to the exe can add one)\n", guid);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    padcfg_get(p->guid, &p->cfg);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    clear_port_status(i);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    printf("[input] port %d: %s connected (%s), stick: %s gate, deadzone %d%%, sensitivity %d%%%s\n",
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
           i + 1, pad_name(p), type_name(p->type),
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
           p->cfg.gate == GCS_GATE_CIRCLE ? "circle" : "octagon", p->cfg.deadzone,
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
           p->cfg.sensitivity, p->cfg.cal[GCS_MAIN].valid ? ", calibrated" : "");
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (s_announce) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        char body[96];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        overlay_toast(OVERLAY_TOAST_SUCCESS, "Controller connected", body);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* A device went away (SDL instance id): close it and free its port. */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
void input_on_device_removed(unsigned int instance_id) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    int i;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    for (i = 0; i < MAX_CONTROLLERS; i++) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        PadPort* p = &s_ports[i];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (p->id < 0 || p->id != (int32_t)instance_id) continue;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        printf("[input] port %d: %s disconnected\n", i + 1, pad_name(p));
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (p->gc) SDL_CloseGamepad(p->gc);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (p->joy) SDL_CloseJoystick(p->joy);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        memset(p, 0, sizeof *p);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        p->id = -1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        clear_port_status(i);   /* nothing stays held */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (s_active == i) s_active = ACTIVE_NONE;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        /* a pad that found all ports taken can have this one now */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            int d, n = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            SDL_JoystickID* ids = SDL_GetJoysticks(&n);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            for (d = 0; ids && d < n; d++)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                if (ids[d] != (SDL_JoystickID)instance_id)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    input_on_device_added(ids[d]);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            SDL_free(ids);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        return;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
void input_init(void) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    int i, n = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDL_JoystickID* ids;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    for (i = 0; i < MAX_CONTROLLERS; i++)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        s_ports[i].id = -1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    /* Nintendo pads map by label (see sdl_button_to_pad); SDL3 has no
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
     * SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS any more. */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    /* Initialize SDL gamepad subsystem */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (!SDL_WasInit(SDL_INIT_GAMEPAD)) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            fprintf(stderr, "SDL_InitSubSystem(GAMEPAD) failed: %s\n", SDL_GetError());
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    /* Load gamecontrollerdb.txt if available */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDL_AddGamepadMappingsFromFile("gamecontrollerdb.txt");
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    /* Open the pads already connected; later ones arrive as events */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    ids = SDL_GetJoysticks(&n);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    for (i = 0; ids && i < n; i++)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        input_on_device_added(ids[i]);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDL_free(ids);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (n == 0)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        printf("No gamepads found (keyboard on port 1; pads can be plugged in any time)\n");
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    s_announce = 1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    s_keyboard_fallback = true;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    /* Initialize player pad map */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    for (i = 0; i < MAX_CONTROLLERS; i++) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        player_pad_map[i] = i;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    printf("Input system initialized\n");
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
void input_shutdown(void) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    for (int i = 0; i < MAX_CONTROLLERS; i++) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        PadPort* p = &s_ports[i];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (p->gc) SDL_CloseGamepad(p->gc);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (p->joy) SDL_CloseJoystick(p->joy);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        memset(p, 0, sizeof *p);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        p->id = -1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static uint16_t sdl_button_to_pad(SDL_Gamepad* gc, int sdl_button) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    switch (sdl_button) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case SDL_GAMEPAD_BUTTON_SOUTH:
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case SDL_GAMEPAD_BUTTON_EAST:
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case SDL_GAMEPAD_BUTTON_WEST:
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case SDL_GAMEPAD_BUTTON_NORTH:
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            /* Face buttons map by the label printed on the pad, as SDL2's
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
             * (default on) USE_BUTTON_LABELS did: the button labelled A is GC
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
             * A, B is GC B, X is GC X, Y is GC Y.  SDL3 buttons are positional;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
             * SDL_GetGamepadButtonLabel gives what the pad prints there (Xbox
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
             * south = A, PlayStation south = Cross, Nintendo south = B, whose A
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
             * label is EAST). */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            switch (SDL_GetGamepadButtonLabel(gc, (SDL_GamepadButton)sdl_button)) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            case SDL_GAMEPAD_BUTTON_LABEL_A:
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            case SDL_GAMEPAD_BUTTON_LABEL_CROSS:    return PAD_BUTTON_A;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            case SDL_GAMEPAD_BUTTON_LABEL_B:
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            case SDL_GAMEPAD_BUTTON_LABEL_CIRCLE:   return PAD_BUTTON_B;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            case SDL_GAMEPAD_BUTTON_LABEL_X:
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            case SDL_GAMEPAD_BUTTON_LABEL_SQUARE:   return PAD_BUTTON_X;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            case SDL_GAMEPAD_BUTTON_LABEL_Y:
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            case SDL_GAMEPAD_BUTTON_LABEL_TRIANGLE: return PAD_BUTTON_Y;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            default: break;   /* unlabelled: by position */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            return sdl_button == SDL_GAMEPAD_BUTTON_SOUTH ? PAD_BUTTON_A :
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                   sdl_button == SDL_GAMEPAD_BUTTON_EAST  ? PAD_BUTTON_B :
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                   sdl_button == SDL_GAMEPAD_BUTTON_WEST  ? PAD_BUTTON_X : PAD_BUTTON_Y;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case SDL_GAMEPAD_BUTTON_START:          return PAD_BUTTON_START;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return PAD_TRIGGER_Z;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        /* LEFT_SHOULDER, BACK, GUIDE, stick clicks: no GC counterpart */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case SDL_GAMEPAD_BUTTON_DPAD_UP:        return PAD_BUTTON_UP;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:      return PAD_BUTTON_DOWN;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:      return PAD_BUTTON_LEFT;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:     return PAD_BUTTON_RIGHT;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        default: return 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* One host stick (SDL axes, y down) -> the emulated GC stick as PADRead
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * reports it (gc_stick.c: deadzone, calibration, GC octagon gate).  The
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * game's own processing of that value happens once all inputs are in
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * (game_process_sticks). */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static void map_stick(PadPort* p, int stick, int sx, int sy, int8_t* ox, int8_t* oy) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    float x = gcs_axis_norm(sx), y = -gcs_axis_norm(sy);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    p->view.host[stick][0] = x;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    p->view.host[stick][1] = y;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    gcs_map_stick(&p->cfg, stick, x, y, ox, oy, &p->view.norm[stick]);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* Trigger travel 0..32767 -> GC analog value, and the digital click with
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * hysteresis.  At the click the analog value is full (0x8C), as on a GC
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * trigger, which the boat paddles check for exactly. */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static uint8_t trigger_axis(int raw, uint8_t* click, uint16_t* buttons, uint16_t bit) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (raw < 0) raw = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (*click ? raw < TRIGGER_CLICK_OFF : raw >= TRIGGER_CLICK_ON)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        *click = !*click;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (*click) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        *buttons |= bit;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        return TRIGGER_MAX;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    return (uint8_t)((raw * TRIGGER_MAX + 16383) / 32767);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* Read a port's pad into its GC pad status */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static void read_port(int port) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    PadPort* p = &s_ports[port];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    PADStatus* curr = &pad_status_groups[port].status[0];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    uint16_t buttons = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    int b;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (p->gc) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        SDL_Gamepad* gc = p->gc;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        for (b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; b++)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            if (SDL_GetGamepadButton(gc, (SDL_GamepadButton)b))
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                buttons |= sdl_button_to_pad(gc, b);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        map_stick(p, GCS_MAIN, SDL_GetGamepadAxis(gc, SDL_GAMEPAD_AXIS_LEFTX),
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                  SDL_GetGamepadAxis(gc, SDL_GAMEPAD_AXIS_LEFTY), &curr->stickX, &curr->stickY);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        map_stick(p, GCS_C, SDL_GetGamepadAxis(gc, SDL_GAMEPAD_AXIS_RIGHTX),
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                  SDL_GetGamepadAxis(gc, SDL_GAMEPAD_AXIS_RIGHTY), &curr->substickX, &curr->substickY);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        curr->triggerLeft = trigger_axis(SDL_GetGamepadAxis(gc, SDL_GAMEPAD_AXIS_LEFT_TRIGGER),
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                                         &p->trig_click[0], &buttons, PAD_TRIGGER_L);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        curr->triggerRight = trigger_axis(SDL_GetGamepadAxis(gc, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER),
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                                          &p->trig_click[1], &buttons, PAD_TRIGGER_R);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    } else if (p->joy) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        /* Generic DirectInput layout: buttons 1-4 = face (south, east, west,
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
         * north), 5/6 = L1/R1, 7/8 = L2/R2, 10 = start; axes 0/1 = left
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
         * stick, 2/3 = right stick; hat 0 = d-pad.  Digital L2/R2 = full
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
         * GC L/R. */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        static const uint16_t bmap[10] = {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            PAD_BUTTON_A, PAD_BUTTON_B, PAD_BUTTON_X, PAD_BUTTON_Y,
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            0, PAD_TRIGGER_Z, PAD_TRIGGER_L, PAD_TRIGGER_R, 0, PAD_BUTTON_START,
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        };
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        SDL_Joystick* j = p->joy;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        int nb = SDL_GetNumJoystickButtons(j), na = SDL_GetNumJoystickAxes(j);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        for (b = 0; b < nb && b < 10; b++)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            if (SDL_GetJoystickButton(j, b)) buttons |= bmap[b];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (SDL_GetNumJoystickHats(j) > 0) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            Uint8 h = SDL_GetJoystickHat(j, 0);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            if (h & SDL_HAT_UP)    buttons |= PAD_BUTTON_UP;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            if (h & SDL_HAT_DOWN)  buttons |= PAD_BUTTON_DOWN;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            if (h & SDL_HAT_LEFT)  buttons |= PAD_BUTTON_LEFT;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            if (h & SDL_HAT_RIGHT) buttons |= PAD_BUTTON_RIGHT;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        map_stick(p, GCS_MAIN, na > 0 ? SDL_GetJoystickAxis(j, 0) : 0,
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                  na > 1 ? SDL_GetJoystickAxis(j, 1) : 0, &curr->stickX, &curr->stickY);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        map_stick(p, GCS_C, na > 2 ? SDL_GetJoystickAxis(j, 2) : 0,
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                  na > 3 ? SDL_GetJoystickAxis(j, 3) : 0, &curr->substickX, &curr->substickY);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        curr->triggerLeft = (buttons & PAD_TRIGGER_L) ? TRIGGER_MAX : 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        curr->triggerRight = (buttons & PAD_TRIGGER_R) ? TRIGGER_MAX : 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    curr->button = buttons;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    /* Analog A/B buttons */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    curr->analogA = (buttons & PAD_BUTTON_A) ? 255 : 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    curr->analogB = (buttons & PAD_BUTTON_B) ? 255 : 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    /* "Used last" for the button prompts: a new press, or a stick / trigger
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
     * pushed well past its rest position (not drift) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        uint16_t eng = buttons;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (p->view.norm[GCS_MAIN] > 0.6f)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            eng |= 0x8000;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (p->view.norm[GCS_C] > 0.6f)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            eng |= 0x4000;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (curr->triggerLeft > TRIGGER_MAX / 2 || curr->triggerRight > TRIGGER_MAX / 2)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            eng |= 0x2000;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (eng & ~p->engaged)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            s_active = port;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        p->engaged = eng;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
void input_on_key_down(int keycode) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (!s_keyboard_fallback) return;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    for (int i = 0; i < INPUT_GAMEPAD_MAX; i++) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (s_keyboard_map[i] == keycode) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            uint16_t* button = &pad_status_groups[0].status[0].button;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            switch (i) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                case INPUT_GAMEPAD_A:          *button |= PAD_BUTTON_A; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                case INPUT_GAMEPAD_B:          *button |= PAD_BUTTON_B; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                case INPUT_GAMEPAD_X:          *button |= PAD_BUTTON_X; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                case INPUT_GAMEPAD_Y:          *button |= PAD_BUTTON_Y; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                case INPUT_GAMEPAD_START:      *button |= PAD_BUTTON_START; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                case INPUT_GAMEPAD_LEFT_SHOULDER: *button |= PAD_TRIGGER_L; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                case INPUT_GAMEPAD_RIGHT_SHOULDER: *button |= PAD_TRIGGER_R; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                case INPUT_GAMEPAD_DPAD_UP:    *button |= PAD_BUTTON_UP; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                case INPUT_GAMEPAD_DPAD_DOWN:  *button |= PAD_BUTTON_DOWN; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                case INPUT_GAMEPAD_DPAD_LEFT:  *button |= PAD_BUTTON_LEFT; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                case INPUT_GAMEPAD_DPAD_RIGHT: *button |= PAD_BUTTON_RIGHT; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                default: break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
void input_on_key_up(int keycode) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (!s_keyboard_fallback) return;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    for (int i = 0; i < INPUT_GAMEPAD_MAX; i++) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (s_keyboard_map[i] == keycode) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            uint16_t* button = &pad_status_groups[0].status[0].button;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            switch (i) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                case INPUT_GAMEPAD_A:          *button &= ~PAD_BUTTON_A; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                case INPUT_GAMEPAD_B:          *button &= ~PAD_BUTTON_B; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                case INPUT_GAMEPAD_X:          *button &= ~PAD_BUTTON_X; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                case INPUT_GAMEPAD_Y:          *button &= ~PAD_BUTTON_Y; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                case INPUT_GAMEPAD_START:      *button &= ~PAD_BUTTON_START; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                case INPUT_GAMEPAD_LEFT_SHOULDER: *button &= ~PAD_TRIGGER_L; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                case INPUT_GAMEPAD_RIGHT_SHOULDER: *button &= ~PAD_TRIGGER_R; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                case INPUT_GAMEPAD_DPAD_UP:    *button &= ~PAD_BUTTON_UP; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                case INPUT_GAMEPAD_DPAD_DOWN:  *button &= ~PAD_BUTTON_DOWN; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                case INPUT_GAMEPAD_DPAD_LEFT:  *button &= ~PAD_BUTTON_LEFT; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                case INPUT_GAMEPAD_DPAD_RIGHT: *button &= ~PAD_BUTTON_RIGHT; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                default: break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
void input_on_gamepad_button_down(int controller, int button) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (controller < 0 || controller >= MAX_CONTROLLERS) return;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    uint16_t* pad_button = &pad_status_groups[controller].status[0].button;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    switch (button) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_A:          *pad_button |= PAD_BUTTON_A; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_B:          *pad_button |= PAD_BUTTON_B; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_X:          *pad_button |= PAD_BUTTON_X; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_Y:          *pad_button |= PAD_BUTTON_Y; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_START:      *pad_button |= PAD_BUTTON_START; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_LEFT_SHOULDER: break; /* LB: unused */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_RIGHT_SHOULDER: *pad_button |= PAD_TRIGGER_Z; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_DPAD_UP:    *pad_button |= PAD_BUTTON_UP; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_DPAD_DOWN:  *pad_button |= PAD_BUTTON_DOWN; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_DPAD_LEFT:  *pad_button |= PAD_BUTTON_LEFT; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_DPAD_RIGHT: *pad_button |= PAD_BUTTON_RIGHT; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        default: break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
void input_on_gamepad_button_up(int controller, int button) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (controller < 0 || controller >= MAX_CONTROLLERS) return;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    uint16_t* pad_button = &pad_status_groups[controller].status[0].button;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    switch (button) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_A:          *pad_button &= ~PAD_BUTTON_A; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_B:          *pad_button &= ~PAD_BUTTON_B; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_X:          *pad_button &= ~PAD_BUTTON_X; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_Y:          *pad_button &= ~PAD_BUTTON_Y; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_START:      *pad_button &= ~PAD_BUTTON_START; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_LEFT_SHOULDER: break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_RIGHT_SHOULDER: *pad_button &= ~PAD_TRIGGER_Z; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_DPAD_UP:    *pad_button &= ~PAD_BUTTON_UP; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_DPAD_DOWN:  *pad_button &= ~PAD_BUTTON_DOWN; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_DPAD_LEFT:  *pad_button &= ~PAD_BUTTON_LEFT; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_DPAD_RIGHT: *pad_button &= ~PAD_BUTTON_RIGHT; break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        default: break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
void input_on_gamepad_axis(int controller, int axis, float value) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (controller < 0 || controller >= MAX_CONTROLLERS) return;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    PADStatus* status = &pad_status_groups[controller].status[0];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    switch (axis) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_AXIS_LEFTX:  status->stickX = (int8_t)(value * 127.0f); break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_AXIS_LEFTY:  status->stickY = (int8_t)(-value * 127.0f); break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_AXIS_RIGHTX: status->substickX = (int8_t)(value * 127.0f); break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_AXIS_RIGHTY: status->substickY = (int8_t)(-value * 127.0f); break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_AXIS_TRIGGERLEFT:  status->triggerLeft = (uint8_t)((value + 1.0f) * 127.5f); break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        case INPUT_GAMEPAD_AXIS_TRIGGERRIGHT: status->triggerRight = (uint8_t)((value + 1.0f) * 127.5f); break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        default: break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#ifdef SMB2_LINK_GAME
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* Scripted input injection for automated headless testing.
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * SMB2_INPUT_SCRIPT = comma-separated events: "frame:TOK" (button held 10
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * frames) or "start-end:TOK" (held across the range).  TOK is one of
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * START A B X Y L R Z UP DOWN LEFT RIGHT (buttons), SU SD SL SR (main stick
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * full deflection), TL TR / TLnnn TRnnn (analog triggers) or ROW / ROWnn
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * (alternate L/R trigger pulls; see parse_input_script).  Frames count
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * input polls (once per game frame).
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * Virtual pads (SDL virtual joysticks, to test hot-plugging and the pad
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * mapping through SDL without hardware; with a 2 suffix, VADDX2 etc., port 1
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * stays with the keyboard/script and the pad plugs into port 2): VADDX / VADDP4 / VADDP5 / VADDS
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * plug in an Xbox / PS4 / PS5 / Switch Pro pad, VDEL unplugs it, VBn holds
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * its SDL button n, VLT / VRT (VLTnnnnn / VRTnnnnn) pull its triggers,
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * VLXn / VLYn / VRXn / VRYn hold its stick axes at SDL value n (-32768..32767,
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * y down as SDL reports it) -- the whole host-stick path.
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * Analog sticks at exact values: SXn / SYn (control stick) and CXn / CYn
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * (C stick) hold port 1's emulated GC pad value n (-128..127, y up, what
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * PADRead would return), which then goes through the game's own stick
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * processing like a real pad.  SU / SD / SL / SR (and AUTO) instead set the
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * value the game sees after that processing (+-100: full tilt); GXn / GYn
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * set that processed value exactly (n != 0). */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
typedef struct {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    long start, end;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    uint16_t buttons;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    int8_t stickX, stickY;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    int8_t autopilot;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    int8_t hotkey;         /* F1 token: ImGui overlay toggle */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    int8_t esc;            /* ESC token: the Esc key (pc_input_esc_pressed) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    uint8_t trigL, trigR;  /* analog trigger values (TL/TR tokens) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    int16_t row;           /* ROW token: alternate L/R every `row` frames */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    int8_t vadd, vdel;     /* virtual pad: plug in (kind 1-4) / unplug */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    int8_t vport2;         /* VADD*2: plug it in on port 2 or later */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    int8_t vbtn;           /* virtual pad: SDL button + 1 held */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    int16_t vlt, vrt;      /* virtual pad: trigger axis values */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    uint8_t rawmask;       /* SX SY CX CY tokens present (bits 0-3) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    int8_t raw[4];         /* emulated GC stick values: SX SY CX CY */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    uint8_t vaxmask;       /* VLX VLY VRX VRY tokens present (bits 0-3) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    int16_t vax[4];        /* virtual pad stick axes (SDL values) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
} ScriptEvent;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* Closed-loop goal steering for automated testing ("AUTO" script token):
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * hold the stick up and steer the ball's x (and lateral velocity) back to 0,
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * which lines it up with the goal gate at the stage origin. */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
struct AutoBall { uint32_t pad0; float px, py, pz;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                  float prevx, prevy, prevz; float vx, vy, vz; };
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
extern struct AutoBall balls[];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
extern int main_mode;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#define SCRIPT_TRIGGER_FULL 0x8C
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#define SCRIPT_MAX_EVENTS 4096
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static ScriptEvent s_script[SCRIPT_MAX_EVENTS];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
long g_input_script_frame;   /* input polls so far (script frame numbers) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static int s_script_count = -1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static void parse_input_script(void) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    extern const char *g_input_script_arg;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    const char *env = g_input_script_arg ? g_input_script_arg
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                                         : getenv("SMB2_INPUT_SCRIPT");
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    char *buf;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    char *tok;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    s_script_count = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (!env) return;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    /* no fixed-size buffer: soak scripts hold hundreds of events */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    buf = strdup(env);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (!buf) return;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    for (tok = strtok(buf, ","); tok; tok = strtok(NULL, ",")) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (s_script_count >= SCRIPT_MAX_EVENTS) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            printf("[input-script] more than %d events - rest ignored\n", SCRIPT_MAX_EVENTS);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            break;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        long a = 0, b = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        char name[16] = {0};
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        ScriptEvent *ev = &s_script[s_script_count];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (sscanf(tok, "%ld-%ld:%15s", &a, &b, name) == 3) { /* range */ }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (sscanf(tok, "%ld:%15s", &a, name) == 2) b = a + 10;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else continue;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        memset(ev, 0, sizeof *ev);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        ev->start = a;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        ev->end = b;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if      (!strcmp(name, "START")) ev->buttons = PAD_BUTTON_START;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "A"))     ev->buttons = PAD_BUTTON_A;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "B"))     ev->buttons = PAD_BUTTON_B;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "X"))     ev->buttons = PAD_BUTTON_X;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "Y"))     ev->buttons = PAD_BUTTON_Y;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "L"))     ev->buttons = PAD_TRIGGER_L;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "R"))     ev->buttons = PAD_TRIGGER_R;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "Z"))     ev->buttons = PAD_TRIGGER_Z;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "UP"))    ev->buttons = PAD_BUTTON_UP;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "DOWN"))  ev->buttons = PAD_BUTTON_DOWN;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "LEFT"))  ev->buttons = PAD_BUTTON_LEFT;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "RIGHT")) ev->buttons = PAD_BUTTON_RIGHT;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "SU"))    ev->stickY = 100;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "SD"))    ev->stickY = -100;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "SL"))    ev->stickX = -100;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "SR"))    ev->stickX = 100;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (name[0] == 'G' && (name[1] == 'X' || name[1] == 'Y') && name[2]) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            /* GXn / GYn: the processed value the game sees (as SU..SR) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            int v = atoi(name + 2);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            v = v < -128 ? -128 : v > 127 ? 127 : v;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            if (name[1] == 'X') ev->stickX = (int8_t)v; else ev->stickY = (int8_t)v;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "AUTO"))  ev->autopilot = 1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "F1"))    ev->hotkey = 1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "ESC"))   ev->esc = 1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "STK"))   ev->hotkey = 2;   /* Controller Sticks screen */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "UPD"))   ev->hotkey = 3;   /* Updates screen (pc_update_ui.c) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "MC"))    ev->hotkey = 4;   /* Memory Card screen (pc_memcard_ui.c) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        /* Analog triggers: TL/TR = full press (0x8C, the value the game
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
         * treats as fully pulled), TLnnn/TRnnn = explicit 0-255 value.
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
         * ROW/ROWnn = alternate full L and R pulls, nn frames each (default
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
         * 15) -- the Monkey Boat paddling motion. */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strncmp(name, "TL", 2)) ev->trigL = name[2] ? (uint8_t)atoi(name + 2) : SCRIPT_TRIGGER_FULL;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strncmp(name, "TR", 2)) ev->trigR = name[2] ? (uint8_t)atoi(name + 2) : SCRIPT_TRIGGER_FULL;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "VADDX"))  ev->vadd = 1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "VADDP4")) ev->vadd = 2;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "VADDP5")) ev->vadd = 3;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "VADDS"))  ev->vadd = 4;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "VADDX2") || !strcmp(name, "VADDP42") ||
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                 !strcmp(name, "VADDP52") || !strcmp(name, "VADDS2")) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            ev->vadd = name[4] == 'X' ? 1 : name[4] == 'S' ? 4 : name[5] == '4' ? 2 : 3;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            ev->vport2 = 1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strcmp(name, "VDEL"))   ev->vdel = 1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if ((name[0] == 'S' || name[0] == 'C') && (name[1] == 'X' || name[1] == 'Y') && name[2]) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            int k = (name[0] == 'C') * 2 + (name[1] == 'Y');
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            int v = atoi(name + 2);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            ev->rawmask |= (uint8_t)(1 << k);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            ev->raw[k] = (int8_t)(v < -128 ? -128 : v > 127 ? 127 : v);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (name[0] == 'V' && (name[1] == 'L' || name[1] == 'R') &&
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                 (name[2] == 'X' || name[2] == 'Y') && name[3]) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            int k = (name[1] == 'R') * 2 + (name[2] == 'Y');
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            int v = atoi(name + 3);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            ev->vaxmask |= (uint8_t)(1 << k);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            ev->vax[k] = (int16_t)(v < -32768 ? -32768 : v > 32767 ? 32767 : v);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strncmp(name, "VB", 2) && name[2]) ev->vbtn = (int8_t)(atoi(name + 2) + 1);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strncmp(name, "VLT", 3)) ev->vlt = name[3] ? (int16_t)atoi(name + 3) : 32767;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strncmp(name, "VRT", 3)) ev->vrt = name[3] ? (int16_t)atoi(name + 3) : 32767;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (!strncmp(name, "ROW", 3)) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            ev->row = name[3] ? (int16_t)atoi(name + 3) : 15;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            if (ev->row <= 0) ev->row = 15;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else continue;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        printf("[input-script] %ld-%ld %s\n", a, b, name);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        s_script_count++;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    free(buf);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* Script virtual pad (VADD* / VDEL / VB / VLT / VRT tokens) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static int32_t s_vpad = -1;   /* instance id of the script's virtual pad */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static void vpad_attach(int kind) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    static const struct { Uint16 vid, pid; const char* name; } kinds[4] = {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        { 0x045E, 0x02EA, "Virtual Xbox One pad" },
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        { 0x054C, 0x09CC, "Virtual PS4 pad" },
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        { 0x054C, 0x0CE6, "Virtual PS5 pad" },
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        { 0x057E, 0x2009, "Virtual Switch Pro pad" },
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    };
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDL_VirtualJoystickDesc desc;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDL_JoystickID id;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (s_vpad >= 0 || kind < 1 || kind > 4) return;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    /* SDL drops joystick input while no window has focus (headless test
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
     * runs never get focus); the scripted pad has to be read anyway */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDL_INIT_INTERFACE(&desc);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    desc.vendor_id = kinds[kind - 1].vid;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    desc.product_id = kinds[kind - 1].pid;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    desc.name = kinds[kind - 1].name;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    id = SDL_AttachVirtualJoystick(&desc);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (!id) { printf("[vpad] attach failed: %s\n", SDL_GetError()); return; }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    s_vpad = (int32_t)id;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    printf("[vpad] attached %s (instance %u)\n", desc.name, (unsigned)id);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static void vpad_detach(void) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (s_vpad >= 0) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        SDL_DetachVirtualJoystick((SDL_JoystickID)s_vpad);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        printf("[vpad] detached\n");
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    s_vpad = -1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* Set the virtual pad's state; SDL reports it from the next update on. */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static void vpad_set(uint32_t buttons, int lt, int rt, const int16_t axes[4]) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDL_Joystick* j = s_vpad >= 0 ? SDL_GetJoystickFromID((SDL_JoystickID)s_vpad) : NULL;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    int b;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (!j) return;   /* not opened by the hot-plug code yet */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    for (b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; b++)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        SDL_SetJoystickVirtualButton(j, b, ((buttons >> b) & 1) != 0);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    /* the virtual mapping scales the full joystick axis range to the
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
     * trigger's 0..32767 */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDL_SetJoystickVirtualAxis(j, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, (Sint16)(lt * 2 - 32768));
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDL_SetJoystickVirtualAxis(j, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, (Sint16)(rt * 2 - 32768));
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    for (b = 0; b < 4; b++)   /* LEFTX LEFTY RIGHTX RIGHTY */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        SDL_SetJoystickVirtualAxis(j, b, axes[b]);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* ---- the game's stick processing (GC: g_read_inputs_and_maybe_reset_console,
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * 0x80009A10, run on every PADRead result before process_inputs).  The PC
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * build stubs that function and does its work here, at the end of the poll,
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * so every reader of pad_status_groups (the game, the F1 overlay's own loop)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * sees processed values.  Data the DOL keeps for it: */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
struct StickPoint { int8_t x, y; };
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* 0x80081AC0: per-pad control stick gate (the Options stick adjustment
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * stores its measurement here) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
int8_t lbl_80081AC0[MAX_CONTROLLERS][8][2] = {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#define GATE { { 84, 0 }, { 59, 59 }, { 0, 84 }, { -59, 59 }, { -84, 0 }, { -59, -59 }, { 0, -84 }, { 59, -59 } }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    GATE, GATE, GATE, GATE
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#undef GATE
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
};
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* 0x8000A730 / 0x8000A75C: the game data save keeps the gates (memcard_mkb2.c,
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * save offset 0x31C, 0x40 bytes), so a stick adjustment made in Options
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * persists.  Saves of earlier port builds hold zeros there (these were
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * stubs); a gate is only loaded when it is one the adjustment could store
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * (each axis point on its own side), else the current one is kept. */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
void fn_8000A730(void* dst) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    memcpy(dst, lbl_80081AC0, sizeof(lbl_80081AC0));
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
void fn_8000A75C(const void* src) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    const int8_t (*g)[8][2] = (const int8_t (*)[8][2])src;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    int i;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    for (i = 0; i < MAX_CONTROLLERS; i++)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (g[i][0][0] > 0 && g[i][2][1] > 0 && g[i][4][0] < 0 && g[i][6][1] < 0)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            memcpy(lbl_80081AC0[i], g[i], sizeof(lbl_80081AC0[i]));
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* 0x80145210: the unprocessed pads (PADRead result; the Options controller
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * screens draw from it) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
ControllerInfo lbl_80145210[MAX_CONTROLLERS];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* 0x8000A380: one stick point through a gate (Options stick adjustment) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
void fn_8000A380(struct StickPoint* point, struct StickPoint* gate) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    gcs_game_stick(&point->x, &point->y, (const int8_t (*)[2])gate);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static void game_process_sticks(int log) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    int i;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    for (i = 0; i < MAX_CONTROLLERS; i++) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        PADStatus* s = &pad_status_groups[i].status[0];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        ControllerInfo* r = &lbl_80145210[i];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        int8_t gx = s->stickX, gy = s->stickY, cx = s->substickX, cy = s->substickY;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        r->status[1].button = r->status[0].button;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        r->status[0] = *s;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        r->status[2].button = (uint16_t)(s->button & ~r->status[1].button);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        r->status[3].button = (uint16_t)(r->status[1].button & ~s->button);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        gcs_game_stick(&s->stickX, &s->stickY, (const int8_t (*)[2])lbl_80081AC0[i]);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        gcs_game_stick(&s->substickX, &s->substickY, gcs_game_c_gate);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (s_ports[i].id >= 0) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            PadStickView* v = &s_ports[i].view;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            v->gc[0][0] = gx; v->gc[0][1] = gy; v->gc[1][0] = cx; v->gc[1][1] = cy;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            v->game[0][0] = s->stickX; v->game[0][1] = s->stickY;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            v->game[1][0] = s->substickX; v->game[1][1] = s->substickY;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (log) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            static int8_t last[MAX_CONTROLLERS][8];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            int8_t now[8] = { gx, gy, cx, cy, s->stickX, s->stickY, s->substickX, s->substickY };
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            if (memcmp(now, last[i], 8)) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                extern long g_input_script_frame;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                printf("[stick] frame %ld port %d gc main (%d,%d) c (%d,%d) -> game main (%d,%d) c (%d,%d)\n",
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                       g_input_script_frame, i + 1, gx, gy, cx, cy,
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                       s->stickX, s->stickY, s->substickX, s->substickY);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                memcpy(last[i], now, 8);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* With the real game linked, the decompiled input.c provides process_inputs /
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * threshold_analog_inputs / merge_inputs / g_calc_frames_since_last_input_change,
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * operating on the pad_status_groups global defined above (which stands in for
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * the GC pad data the DOL would fill from PADRead).  This function is the SDL
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * poller that fills it, called once per frame by the shim's video_finish_frame. */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* game = 1 for the call from video_finish_frame (E3 pad hook, see below);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * the modal shim screens call input_poll_gc_pads() and stay local-only. */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* the local sample, kept apart from pad_status_groups[].status[0] while a
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * non-live source overwrites that with the exchanged pads (E1 D6) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static PADStatus s_local_pads[MAX_CONTROLLERS];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static void poll_gc_pads_impl(int game);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
void input_poll_gc_pads(void) { poll_gc_pads_impl(0); }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
void input_poll_gc_pads_game(void) { poll_gc_pads_impl(1); }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static void poll_gc_pads_impl(int game)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#else
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
void process_inputs(void)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#endif
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
{
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    /* Copy current status to previous (index 1) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    for (int i = 0; i < MAX_CONTROLLERS; i++) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        pad_status_groups[i].status[1] = pad_status_groups[i].status[0];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#ifdef SMB2_LINK_GAME
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    /* E3: with a non-live pad source status[0] holds the exchanged (remote /
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
     * replayed) pads, not the local state the sampling code below builds on
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
     * (keyboard OR-in, trigger and stick persistence): put the private
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
     * local sample back first. */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    const int sync = game && inputsync_overrides();
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (sync)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        for (int i = 0; i < MAX_CONTROLLERS; i++)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            pad_status_groups[i].status[0] = s_local_pads[i];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#endif
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    s_esc_pressed = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    /* Poll SDL events and update current status (index 0) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDL_UpdateGamepads();
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    for (int i = 0; i < MAX_CONTROLLERS; i++) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        /* PAD_ERR_NO_CONTROLLER (-1) for unplugged ports, as PADRead
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
         * reports.  Port 1 is always connected (keyboard / script).
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
         * sel_ngc counts connected pads for the player-count menus. */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        pad_status_groups[i].status[0].err = (i == 0 || s_ports[i].id >= 0) ? 0 : -1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (s_ports[i].id >= 0)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            read_port(i);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#ifdef SMB2_LINK_GAME
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    /* Keyboard for player 1, read from the key state.  ORed in, so the
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
     * keyboard also works with a gamepad on port 1.
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
     *   WASD = control stick, arrows = +Control Pad, Z/X/C/V = A/B/X/Y,
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
     *   Q/E = L/R (full trigger pull), Space = Z, Enter = START/PAUSE */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        static const struct { SDL_Scancode sc; uint16_t bit; } kbmap[] = {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            { SDL_SCANCODE_Z, PAD_BUTTON_A }, { SDL_SCANCODE_X, PAD_BUTTON_B },
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            { SDL_SCANCODE_C, PAD_BUTTON_X }, { SDL_SCANCODE_V, PAD_BUTTON_Y },
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            { SDL_SCANCODE_RETURN, PAD_BUTTON_START },
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            { SDL_SCANCODE_Q, PAD_TRIGGER_L }, { SDL_SCANCODE_E, PAD_TRIGGER_R },
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            { SDL_SCANCODE_SPACE, PAD_TRIGGER_Z },
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            { SDL_SCANCODE_UP, PAD_BUTTON_UP }, { SDL_SCANCODE_DOWN, PAD_BUTTON_DOWN },
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            { SDL_SCANCODE_LEFT, PAD_BUTTON_LEFT }, { SDL_SCANCODE_RIGHT, PAD_BUTTON_RIGHT },
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        };
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        static uint16_t prev_kb;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        static int prev_wasd;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        const bool* ks = SDL_GetKeyboardState(NULL);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        PADStatus* curr = &pad_status_groups[0].status[0];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        int pad1 = s_ports[0].id >= 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        int wasd = ks[SDL_SCANCODE_W] | ks[SDL_SCANCODE_A] << 1 |
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                   ks[SDL_SCANCODE_S] << 2 | ks[SDL_SCANCODE_D] << 3;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        uint16_t kb = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        size_t k;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        /* WASD stick: owns the stick without a pad on port 1, else only
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
         * while a key is down */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        /* full deflection of a GC stick in the key direction: the octagon's
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
         * cardinal (101) or diagonal (71, 71) notch */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (!pad1 || wasd) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            int dx = (ks[SDL_SCANCODE_D] ? 1 : 0) - (ks[SDL_SCANCODE_A] && !ks[SDL_SCANCODE_D] ? 1 : 0);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            int dy = (ks[SDL_SCANCODE_W] ? 1 : 0) - (ks[SDL_SCANCODE_S] && !ks[SDL_SCANCODE_W] ? 1 : 0);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            curr->stickX = curr->stickY = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            if (dx || dy)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                gcs_full_deflection(GCS_MAIN, atan2((double)dy, (double)dx), &curr->stickX, &curr->stickY);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        for (k = 0; k < sizeof kbmap / sizeof kbmap[0]; k++)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            if (ks[kbmap[k].sc]) kb |= kbmap[k].bit;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (ks[SDL_SCANCODE_LALT] || ks[SDL_SCANCODE_RALT])
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            kb &= ~PAD_BUTTON_START;   /* Alt+Enter is the fullscreen toggle */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        curr->button = (uint16_t)((curr->button & ~(prev_kb & ~kb)) | kb);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        /* Q/E: a GC trigger's click comes with the full analog value */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (kb & PAD_TRIGGER_L) curr->triggerLeft = TRIGGER_MAX;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if ((prev_kb & PAD_TRIGGER_L) && !pad1) curr->triggerLeft = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (kb & PAD_TRIGGER_R) curr->triggerRight = TRIGGER_MAX;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if ((prev_kb & PAD_TRIGGER_R) && !pad1) curr->triggerRight = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        /* Esc: not a pad button; the top-level menus (title screen, Mode
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
         * Select) open Quit to Desktop with it (pc_quit.c) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            static int prev_esc;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            int esc = ks[SDL_SCANCODE_ESCAPE] != 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            if (esc && !prev_esc) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                s_esc_pressed = 1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                s_active = ACTIVE_KEYBOARD;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            prev_esc = esc;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if ((kb & ~prev_kb) || (wasd & ~prev_wasd))
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            s_active = ACTIVE_KEYBOARD;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        prev_kb = kb;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        prev_wasd = wasd;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    /* Scripted input injection (after keyboard emulation so it wins).
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
     * Injected bits are released once their window ends: in keyboard-fallback
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
     * mode nothing else clears status[0].button, so without the explicit
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
     * release every later press of the same button would be "still held". */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#define script_frame g_input_script_frame
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        static uint16_t prev_inject;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        uint16_t inject = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        int8_t sx = 0, sy = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        uint8_t tl = 0, tr = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        uint32_t vbuttons = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        int vlt = 0, vrt = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        int8_t raw[4] = { 0 };
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        int rawmask = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        int16_t vax[4] = { 0 };
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        int e, k;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        PADStatus *curr = &pad_status_groups[0].status[0];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (s_script_count < 0)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            parse_input_script();
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        script_frame++;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        int auto_on = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        for (e = 0; e < s_script_count; e++) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            if (script_frame >= s_script[e].start && script_frame < s_script[e].end) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                inject |= s_script[e].buttons;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                if (s_script[e].stickX) sx = s_script[e].stickX;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                if (s_script[e].stickY) sy = s_script[e].stickY;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                if (s_script[e].autopilot) auto_on = 1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                if (s_script[e].esc && script_frame == s_script[e].start)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    s_esc_pressed = 1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                if (s_script[e].hotkey && script_frame == s_script[e].start) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    extern void pc_stick_screen_request(void);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    extern void pc_update_screen_request(void);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    if (s_script[e].hotkey == 2) pc_stick_screen_request();
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    else if (s_script[e].hotkey == 3) pc_update_screen_request();
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    else if (s_script[e].hotkey == 4) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                        extern void pc_memcard_screen_request(void);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                        pc_memcard_screen_request();
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    else overlay_toggle();
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                if (s_script[e].trigL) tl = s_script[e].trigL;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                if (s_script[e].trigR) tr = s_script[e].trigR;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                if (script_frame == s_script[e].start) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    if (s_script[e].vport2) s_vpad_skip_port1 = 1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    if (s_script[e].vadd) vpad_attach(s_script[e].vadd);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    if (s_script[e].vdel) vpad_detach();
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                if (s_script[e].vbtn) vbuttons |= 1u << (s_script[e].vbtn - 1);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                for (k = 0; k < 4; k++) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    if (s_script[e].rawmask & (1 << k)) { raw[k] = s_script[e].raw[k]; rawmask |= 1 << k; }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    if (s_script[e].vaxmask & (1 << k)) vax[k] = s_script[e].vax[k];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                if (s_script[e].vlt) vlt = s_script[e].vlt;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                if (s_script[e].vrt) vrt = s_script[e].vrt;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                if (s_script[e].row) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    if (((script_frame - s_script[e].start) / s_script[e].row) & 1)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                        tr = SCRIPT_TRIGGER_FULL;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    else
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                        tl = SCRIPT_TRIGGER_FULL;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (auto_on && main_mode == 2) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            struct AutoBall *b = &balls[0];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            float d = -(b->px * 8.0f + b->vx * 120.0f);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            if (d > 100.0f) d = 100.0f;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            else if (d < -100.0f) d = -100.0f;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            sx = (int8_t)d;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            sy = 100;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            if ((script_frame % 60) == 0)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                printf("[auto] f=%ld x=%.2f vx=%.3f -> sx=%d\n",
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                       script_frame, b->px, b->vx, (int)sx);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        /* exact emulated-pad values, then the game's processing for all
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
         * ports; SU/SD/SL/SR/AUTO set the processed value */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (rawmask & 1) curr->stickX = raw[0];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (rawmask & 2) curr->stickY = raw[1];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (rawmask & 4) curr->substickX = raw[2];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (rawmask & 8) curr->substickY = raw[3];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        game_process_sticks(s_script_count > 0);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        curr->button &= ~(prev_inject & ~inject);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        curr->button |= inject;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (sx) curr->stickX = sx;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (sy) curr->stickY = sy;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        /* Scripted triggers own the analog values while a script is active
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
         * (nothing else resets them in keyboard-fallback mode). */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (s_script_count > 0 && s_ports[0].id < 0) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            curr->triggerLeft = tl;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            curr->triggerRight = tr;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        } else {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            if (tl) curr->triggerLeft = tl;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            if (tr) curr->triggerRight = tr;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (inject & PAD_BUTTON_A) curr->analogA = 255;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (prev_inject & PAD_BUTTON_A) curr->analogA = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (inject & PAD_BUTTON_B) curr->analogB = 255;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        else if (prev_inject & PAD_BUTTON_B) curr->analogB = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (inject != prev_inject)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            printf("[input-script] frame %ld held=0x%04x\n", script_frame, inject);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        vpad_set(vbuttons, vlt, vrt, vax);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (s_vpad >= 0) {   /* log what the virtual pad's port reads */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            static PADStatus last;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            int vp;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            for (vp = 0; vp < MAX_CONTROLLERS && s_ports[vp].id != s_vpad; vp++) {}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            if (vp < MAX_CONTROLLERS) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                PADStatus* st = &pad_status_groups[vp].status[0];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                if (st->button != last.button || st->triggerLeft != last.triggerLeft ||
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    st->triggerRight != last.triggerRight)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    printf("[vpad] frame %ld port %d button=0x%04x trigL=%d trigR=%d\n",
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                           script_frame, vp + 1, st->button, st->triggerLeft, st->triggerRight);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                last = *st;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        prev_inject = inject;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#undef script_frame
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#ifdef SMB2_LINK_GAME
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    /* E3 pad hook (docs/handoff/E3.md): the local sample is complete here
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
     * (devices + keyboard + script, post stick gate).  Keep it as the private
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
     * local state, hand the 4 pads to inputsync and use what comes back for
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
     * all 4 ports (value + err). */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (game) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        /* D3 overlay gate: while the ImGui overlay is open (and until the buttons
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
         * that were down at close are released) every local pad reads neutral, so
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
         * record / netplay / the game never see the overlay's navigation input.
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
         * Replay is unaffected (its pads come from the file in the exchange). */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            static int blk_latch, blk_logged;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            int raw_any = 0, blocked = overlay_input_blocked();
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            uint16_t dropped = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            for (int i = 0; i < MAX_CONTROLLERS; i++) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                raw_any |= pad_status_groups[i].status[0].button != 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                dropped |= pad_status_groups[i].status[0].button;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            if (blocked) blk_latch = 1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            else if (blk_latch && !raw_any) blk_latch = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            if (blocked || blk_latch) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                for (int i = 0; i < MAX_CONTROLLERS; i++) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    PADStatus* st = &pad_status_groups[i].status[0];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    st->button = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    st->stickX = st->stickY = st->substickX = st->substickY = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    st->triggerLeft = st->triggerRight = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    st->analogA = st->analogB = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                s_esc_pressed = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                if (dropped && !blk_logged) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    printf("[overlay] local pad input gated (buttons 0x%04x dropped) at poll %llu\n",
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                           dropped, (unsigned long long)inputsync_poll_count());
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                    blk_logged = 1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            } else {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                blk_logged = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        SmbNetPad loc[INPUTSYNC_PORTS], out[INPUTSYNC_PORTS];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        for (int i = 0; i < MAX_CONTROLLERS; i++) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            const PADStatus* st = &pad_status_groups[i].status[0];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            s_local_pads[i] = *st;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            loc[i].buttons = st->button;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            loc[i].stickX = st->stickX; loc[i].stickY = st->stickY;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            loc[i].substickX = st->substickX; loc[i].substickY = st->substickY;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            loc[i].triggerL = st->triggerLeft; loc[i].triggerR = st->triggerRight;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            loc[i].err = st->err;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        inputsync_exchange(loc, out);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        for (int i = 0; sync && i < MAX_CONTROLLERS; i++) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            PADStatus* st = &pad_status_groups[i].status[0];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            ControllerInfo* r = &lbl_80145210[i];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            st->button = out[i].buttons;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            st->stickX = out[i].stickX; st->stickY = out[i].stickY;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            st->substickX = out[i].substickX; st->substickY = out[i].substickY;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            st->triggerLeft = out[i].triggerL; st->triggerRight = out[i].triggerR;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            st->analogA = st->analogB = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            st->err = out[i].err;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            /* the raw-pad copy the Options screens read (E1 D5): the exchanged
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
             * (post-gate) value; edge slots recomputed from its previous one */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            r->status[0] = *st;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            r->status[2].button = (uint16_t)(r->status[0].button & ~r->status[1].button);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            r->status[3].button = (uint16_t)(r->status[1].button & ~r->status[0].button);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#endif
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    /* Derive the pressed/released/repeat slots ([2]/[3]/[4]) the GC pad
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
     * processing would provide; the game reads these directly. */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    for (int i = 0; i < MAX_CONTROLLERS; i++) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        static int repeat_count[MAX_CONTROLLERS];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        uint16_t held = pad_status_groups[i].status[0].button;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        uint16_t prev = pad_status_groups[i].status[1].button;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        pad_status_groups[i].status[2] = pad_status_groups[i].status[0];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        pad_status_groups[i].status[2].button = (uint16_t)(held & ~prev);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        pad_status_groups[i].status[3].button = (uint16_t)(prev & ~held);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (held == prev && held != 0) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            repeat_count[i]++;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            pad_status_groups[i].status[4].button =
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                (repeat_count[i] > 20 && (repeat_count[i] % 4) == 0) ? held : 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        } else {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            repeat_count[i] = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            pad_status_groups[i].status[4].button = (uint16_t)(held & ~prev);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (pad_status_groups[0].status[2].button)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        printf("[input] pressed=0x%04x held=0x%04x\n",
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
               pad_status_groups[0].status[2].button,
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
               pad_status_groups[0].status[0].button);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    pc_button_prompts_update();   /* PS/Xbox/GC prompt style for this frame */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#endif
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* Button prompt style (button_prompts.c): the SDL type of the pad used last,
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * else of player 1's pad, else of the first open one; -1 for the keyboard,
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
 * no pad, or a joystick without a controller mapping. */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
int input_controller_type(void) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    int i;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (s_active == ACTIVE_KEYBOARD)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        return -1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (s_active >= 0 && s_active < MAX_CONTROLLERS && s_ports[s_active].id >= 0)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        return s_ports[s_active].type;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    for (i = 0; i < MAX_CONTROLLERS; i++)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (s_ports[i].id >= 0)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            return s_ports[i].type;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    return -1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#ifndef SMB2_LINK_GAME
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
void threshold_analog_inputs(void) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    /* Apply dead zones and convert to digital */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    for (int i = 0; i < MAX_CONTROLLERS; i++) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        PADStatus* curr = &pad_status_groups[i].status[0];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        /* Apply dead zone to sticks */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        int16_t stickX = curr->stickX;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        int16_t stickY = curr->stickY;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        int16_t substickX = curr->substickX;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        int16_t substickY = curr->substickY;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        const int16_t DEAD_ZONE = 15;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (stickX > -DEAD_ZONE && stickX < DEAD_ZONE) stickX = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (stickY > -DEAD_ZONE && stickY < DEAD_ZONE) stickY = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (substickX > -DEAD_ZONE && substickX < DEAD_ZONE) substickX = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (substickY > -DEAD_ZONE && substickY < DEAD_ZONE) substickY = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        /* Store analog inputs */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        analog_inputs[i][0] = (uint16_t)stickX;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        analog_inputs[i][1] = (uint16_t)stickY;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        analog_inputs[i][2] = (uint16_t)substickX;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        analog_inputs[i][3] = (uint16_t)substickY;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        analog_inputs[i][4] = curr->triggerLeft | (curr->triggerRight << 8);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
void g_calc_frames_since_last_input_change(void) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    for (int i = 0; i < MAX_CONTROLLERS; i++) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        uint16_t curr_digital = pad_status_groups[i].status[0].button;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        uint16_t prev_digital = pad_status_groups[i].status[1].button;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        int16_t curr_stickX = pad_status_groups[i].status[0].stickX;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        int16_t prev_stickX = pad_status_groups[i].status[1].stickX;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        int16_t curr_stickY = pad_status_groups[i].status[0].stickY;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        int16_t prev_stickY = pad_status_groups[i].status[1].stickY;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (curr_digital != prev_digital ||
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            curr_stickX != prev_stickX ||
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            curr_stickY != prev_stickY) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            frames_since_last_input_change[i] = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        } else {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            if (frames_since_last_input_change[i] < 30000) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                frames_since_last_input_change[i]++;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
void merge_inputs(void) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    for (int i = 0; i < MAX_CONTROLLERS; i++) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        uint16_t held = pad_status_groups[i].status[0].button;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        uint16_t prev = pad_status_groups[i].status[1].button;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        /* Pressed = held & ~prev */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        pad_status_groups[i].status[2].button = held & ~prev;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        /* Released = prev & ~held */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        pad_status_groups[i].status[3].button = prev & ~held;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        /* Repeat logic (simple) */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        static int repeat_count[MAX_CONTROLLERS] = {0};
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (held == prev && held != 0) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            repeat_count[i]++;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            if (repeat_count[i] > 30) { /* After ~0.5s at 60fps */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                pad_status_groups[i].status[4].button = held;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            } else {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                pad_status_groups[i].status[4].button = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        } else {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            repeat_count[i] = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            pad_status_groups[i].status[4].button = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        merged_digital_inputs[i] = held;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        merged_analog_inputs[i] = analog_inputs[i][0] | (analog_inputs[i][1] << 8);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
#endif /* !SMB2_LINK_GAME */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
void input_set_keyboard_mapping(int player, const int* key_map) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (player < 0 || player >= MAX_CONTROLLERS) return;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (key_map) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        memcpy(s_keyboard_map, key_map, sizeof(s_keyboard_map));
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* ---- port API for the stick calibration screen (pad_config.h) ---- */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
int input_port_connected(int port) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    return port >= 0 && port < MAX_CONTROLLERS && s_ports[port].id >= 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
const char* input_port_name(int port) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    return input_port_connected(port) ? pad_name(&s_ports[port]) : "";
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
GcsPadConfig* input_port_config(int port) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    return input_port_connected(port) ? &s_ports[port].cfg : NULL;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
void input_port_config_save(int port) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (!input_port_connected(port)) return;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    padcfg_sanitize(&s_ports[port].cfg);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    padcfg_put(s_ports[port].guid, pad_name(&s_ports[port]), &s_ports[port].cfg);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
int input_port_view(int port, PadStickView* v) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (!input_port_connected(port)) return 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    *v = s_ports[port].view;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    return 1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
/* ---- --list-controllers (main.c): what does SDL see?  No window. ---- */
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
static void lc_print_devices(void) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    int n = 0, i;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDL_JoystickID* ids = SDL_GetJoysticks(&n);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    int valve = 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    printf("Joysticks: %d\n", n);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    for (i = 0; ids && i < n; i++) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        SDL_JoystickID id = ids[i];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        char guid[64];
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        char* map = NULL;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        const char* name = SDL_GetJoystickNameForID(id);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        const char* path = SDL_GetJoystickPathForID(id);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        int vid = SDL_GetJoystickVendorForID(id), pid = SDL_GetJoystickProductForID(id);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        bool is_gp = SDL_IsGamepad(id);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        SDL_GUIDToString(SDL_GetJoystickGUIDForID(id), guid, sizeof guid);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        printf("\n[%d] instance id %u: \"%s\"\n", i, (unsigned)id, name ? name : "(no name)");
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        printf("    VID/PID      0x%04X / 0x%04X (version 0x%04X)%s\n", vid, pid,
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
               SDL_GetJoystickProductVersionForID(id),
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
               SDL_IsJoystickVirtual(id) ? " [virtual]" : "");
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        printf("    GUID         %s\n", guid);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        printf("    path         %s\n", path ? path : "(none)");
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        printf("    joystick type %d\n", (int)SDL_GetJoystickTypeForID(id));
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        printf("    is gamepad   %s\n", is_gp ? "yes" : "no");
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (is_gp) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            int t = (int)SDL_GetGamepadTypeForID(id);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            printf("    gamepad type %d (%s)\n", t, type_name(t));
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            map = SDL_GetGamepadMappingForID(id);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        printf("    mapping      %s\n", map ? map : "none");
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        SDL_free(map);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        if (vid == 0x28DE) valve = 1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDL_free(ids);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (n == 0)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        printf("\nSDL sees no joystick or controller devices. A controller that only does "
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
               "keyboard/mouse emulation is not listed.\n");
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    else if (valve)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        printf("\nValve device found (VID 0x28DE): SDL's HIDAPI driver handles it; if it is "
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
               "listed as a gamepad above it will work in the game.\n");
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
int input_print_controllers(void) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    static const char* const hints[] = {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        SDL_HINT_JOYSTICK_HIDAPI, SDL_HINT_JOYSTICK_HIDAPI_STEAM, SDL_HINT_JOYSTICK_RAWINPUT,
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        SDL_HINT_JOYSTICK_WGI, SDL_HINT_XINPUT_ENABLED, SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    };
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    int v, i;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    Uint64 t0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDL_Event ev;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        printf("SDL_Init(VIDEO|GAMEPAD) failed: %s\n", SDL_GetError());
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        return 1;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    v = SDL_GetVersion();
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    printf("SMB2 PC port --list-controllers\n");
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    printf("SDL version  compiled %d.%d.%d, runtime %d.%d.%d (%s)\n",
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
           SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION,
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
           SDL_VERSIONNUM_MAJOR(v), SDL_VERSIONNUM_MINOR(v), SDL_VERSIONNUM_MICRO(v),
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
           SDL_GetRevision());
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    printf("Platform     %s\n", SDL_GetPlatform());
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    printf("Hints       ");
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    for (i = 0; i < (int)(sizeof hints / sizeof hints[0]); i++) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        const char* h = SDL_GetHint(hints[i]);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        printf(" %s=%s", hints[i], h ? h : "(default)");
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    printf("\n");
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDL_AddGamepadMappingsFromFile("gamecontrollerdb.txt");
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    printf("Waiting 1 s for late devices ...\n");
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    fflush(stdout);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    t0 = SDL_GetTicks();
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    while (SDL_GetTicks() - t0 < 1000) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        while (SDL_PollEvent(&ev)) {
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            if (ev.type == SDL_EVENT_JOYSTICK_ADDED)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                printf("  event: joystick added (instance %u)\n", (unsigned)ev.jdevice.which);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
            else if (ev.type == SDL_EVENT_GAMEPAD_ADDED)
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
                printf("  event: gamepad added (instance %u)\n", (unsigned)ev.gdevice.which);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
        SDL_Delay(10);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    }
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    lc_print_devices();
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    fflush(stdout);
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    SDL_Quit();
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
    return 0;
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }
}
{            char body[96];            snprintf(body, sizeof body, "%.60s - player %d", pad_name(p), i + 1);            overlay_toast(OVERLAY_TOAST_WARNING, "Controller disconnected", body);        }

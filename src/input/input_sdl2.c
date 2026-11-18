/* SDL3 Input Backend (gamepad API; file name kept from the SDL2 days)
 *
 * Controllers are mapped onto the GameCube pad by button position (Nintendo
 * pads by label, see below):
 *   south / east / west / north face button = GC A / B / X / Y
 *   LT / RT (L2 / R2, ZL / ZR) = GC L / R: the analog value, plus the
 *     digital L / R click at the end of the travel, as on a GC trigger
 *   RB (R1, R)                 = GC Z
 *   LB (L1, L)                 = unused (the GC pad has nothing there)
 *   Start (Options, Menu, +)   = START/PAUSE
 *   left stick / right stick / d-pad = control stick / C stick / +Control Pad
 * Face buttons map by the label printed on the pad (SDL_GetGamepadButtonLabel),
 * so a Switch pad maps like Nintendo's own ports: the button labelled A (right)
 * is GC A.  See sdl_button_to_pad.
 *
 * Pads are hot-plugged: SDL_EVENT_GAMEPAD_ADDED / SDL_EVENT_JOYSTICK_ADDED open a
 * new pad in the first free port (1-4), *_REMOVED closes it and frees its
 * port; the other players keep theirs.  Joysticks without an SDL controller
 * mapping get a generic DirectInput-style mapping.  The keyboard always
 * drives port 1 as well. */
#include "input.h"
#include "button_prompts.h"
#include "gc_stick.h"
#include "pad_config.h"
#include "netplay/inputsync.h"
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* Global state matching original game */
ControllerInfo pad_status_groups[MAX_CONTROLLERS] = {0};
uint16_t analog_inputs[MAX_CONTROLLERS][5] = {0};
/* The game's player -> pad table.  ball.c keeps the GC idle-chatter counters
 * (lbl_1_bss_70F00[10]) right behind it as one object (boat reads [0..7]),
 * so the single definition must hold all 4 + 10 entries. */
int32_t player_pad_map[4 + 10] = {0, 1, 2, 3};
/* held / previous / pressed / released / repeat, as the game indexes them
 * (0xC bytes each in the DOL); with 4 entries [4] of the digital one landed
 * on the analog one's [0] and the held stick directions were lost */
uint16_t merged_digital_inputs[6] = {0};
uint16_t merged_analog_inputs[6] = {0};
int32_t frames_since_last_input_change[MAX_CONTROLLERS] = {0};

/* One GC pad port */
typedef struct {
    SDL_Gamepad* gc;          /* pad with an SDL gamepad mapping, or */
    SDL_Joystick* joy;        /* a raw joystick (generic mapping) */
    int32_t id;               /* SDL joystick instance id; -1 = port empty */
    int type;                 /* SDL_GamepadType; -1 for raw joysticks */
    uint8_t trig_click[2];    /* digital L / R state of the triggers */
    uint16_t engaged;         /* buttons / deflected axes last frame */
    char guid[40];            /* SDL GUID: key of the stick settings */
    GcsPadConfig cfg;         /* stick settings / calibration (controllers.ini) */
    PadStickView view;        /* last frame's sticks, for the calibration screen */
} PadPort;
static PadPort s_ports[MAX_CONTROLLERS] = { { .id = -1 }, { .id = -1 }, { .id = -1 }, { .id = -1 } };

/* The input device used last, for the "auto" button prompts: a port, the
 * keyboard, or none yet. */
#define ACTIVE_NONE     (-1)
#define ACTIVE_KEYBOARD MAX_CONTROLLERS
static int s_active = ACTIVE_NONE;

static bool s_keyboard_fallback = false;   /* legacy input_on_key_* API */

/* The Esc key went down in this poll (keyboard or the ESC script token). */
static int s_esc_pressed;
int pc_input_esc_pressed(void) { return s_esc_pressed; }

/* GC trigger: analog 0..0x8C as the game sees it (after the SDK rescale), the
 * digital click near the end of the travel.  Press at 85 %, release below
 * 70 % (hysteresis, so a trigger held at the threshold does not chatter). */
#define TRIGGER_MAX       0x8C
#define TRIGGER_CLICK_ON  27852   /* 0.85 * 32767 */
#define TRIGGER_CLICK_OFF 22937   /* 0.70 * 32767 */

/* Keyboard to gamepad mapping for player 1 (legacy input_on_key_* API; the
 * live keyboard mapping is kbmap in input_poll_gc_pads) */
static int s_keyboard_map[INPUT_GAMEPAD_MAX] = {
    SDLK_Z,       /* A */
    SDLK_X,       /* B */
    SDLK_C,       /* X */
    SDLK_V,       /* Y */
    SDLK_ESCAPE,  /* BACK */
    SDLK_TAB,     /* GUIDE */
    SDLK_RETURN,  /* START */
    SDLK_LSHIFT,  /* LEFT_STICK */
    SDLK_RSHIFT,  /* RIGHT_STICK */
    SDLK_Q,       /* LEFT_SHOULDER (L) */
    SDLK_E,       /* RIGHT_SHOULDER (R) */
    SDLK_UP,      /* DPAD_UP */
    SDLK_DOWN,    /* DPAD_DOWN */
    SDLK_LEFT,    /* DPAD_LEFT */
    SDLK_RIGHT,   /* DPAD_RIGHT */
};

static const char* type_name(int type) {
    switch (type) {
    case SDL_GAMEPAD_TYPE_STANDARD: return "standard";
    case SDL_GAMEPAD_TYPE_XBOX360: return "Xbox 360";
    case SDL_GAMEPAD_TYPE_XBOXONE: return "Xbox One/Series";
    case SDL_GAMEPAD_TYPE_PS3: return "PS3";
    case SDL_GAMEPAD_TYPE_PS4: return "PS4";
    case SDL_GAMEPAD_TYPE_PS5: return "PS5";
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO: return "Switch Pro";
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT: return "Joy-Con (L)";
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT: return "Joy-Con (R)";
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR: return "Joy-Con pair";
    case SDL_GAMEPAD_TYPE_GAMECUBE: return "GameCube";
    case -1: return "joystick, no controller mapping";
    }
    return "other";
}

static const char* pad_name(const PadPort* p) {
    const char* n = p->gc ? SDL_GetGamepadName(p->gc) : p->joy ? SDL_GetJoystickName(p->joy) : NULL;
    return n ? n : "pad";
}

static void clear_port_status(int port) {
    PADStatus* s = &pad_status_groups[port].status[0];
    s->button = 0;
    s->stickX = s->stickY = s->substickX = s->substickY = 0;
    s->triggerLeft = s->triggerRight = 0;
    s->analogA = s->analogB = 0;
}

/* A device appeared (SDL joystick instance id): open it in the first free port.
 * Called for every pad at start-up and again for SDL's ADDED events, which
 * also arrive for pads present at start-up, so known pads are skipped. */
/* Script token VADD*2: the script's virtual pad skips port 1 (so the
 * keyboard/script keeps player 1 and the virtual pad is player 2). */
static int s_vpad_skip_port1;

void input_on_device_added(unsigned int instance_id) {
    SDL_JoystickID id = (SDL_JoystickID)instance_id;
    PadPort* p = NULL;
    const char* dname;
    int i;

    if (id == 0) return;
    for (i = 0; i < MAX_CONTROLLERS; i++)
        if (s_ports[i].id == (int32_t)id) return;   /* already open */
    for (i = (s_vpad_skip_port1 && SDL_IsJoystickVirtual(id)) ? 1 : 0; i < MAX_CONTROLLERS; i++)
        if (s_ports[i].id < 0) { p = &s_ports[i]; break; }
    dname = SDL_GetJoystickNameForID(id);
    if (!p) {
        static int32_t s_told = -1;
        if (s_told != (int32_t)id)
            printf("[input] %s connected, but all 4 ports are in use\n", dname ? dname : "pad");
        s_told = (int32_t)id;
        return;
    }
    memset(p, 0, sizeof *p);
    p->id = -1;
    if (SDL_IsGamepad(id)) {
        SDL_Joystick* gj;
        p->gc = SDL_OpenGamepad(id);
        if (!p->gc) {
            fprintf(stderr, "[input] cannot open controller %u: %s\n", (unsigned)id, SDL_GetError());
            return;
        }
        p->type = (int)SDL_GetGamepadType(p->gc);
        gj = SDL_GetGamepadJoystick(p->gc);
        p->id = (int32_t)SDL_GetJoystickID(gj);
        SDL_GUIDToString(SDL_GetJoystickGUID(gj), p->guid, sizeof p->guid);
    } else {
        char guid[64];
        p->joy = SDL_OpenJoystick(id);
        if (!p->joy) {
            fprintf(stderr, "[input] cannot open joystick %u: %s\n", (unsigned)id, SDL_GetError());
            return;
        }
        p->type = -1;
        p->id = (int32_t)SDL_GetJoystickID(p->joy);
        SDL_GUIDToString(SDL_GetJoystickGUID(p->joy), guid, sizeof guid);
        snprintf(p->guid, sizeof p->guid, "%s", guid);
        printf("[input] no controller mapping for GUID %s: using a generic layout "
               "(a gamecontrollerdb.txt next to the exe can add one)\n", guid);
    }
    padcfg_get(p->guid, &p->cfg);
    clear_port_status(i);
    printf("[input] port %d: %s connected (%s), stick: %s gate, deadzone %d%%, sensitivity %d%%%s\n",
           i + 1, pad_name(p), type_name(p->type),
           p->cfg.gate == GCS_GATE_CIRCLE ? "circle" : "octagon", p->cfg.deadzone,
           p->cfg.sensitivity, p->cfg.cal[GCS_MAIN].valid ? ", calibrated" : "");
}

/* A device went away (SDL instance id): close it and free its port. */
void input_on_device_removed(unsigned int instance_id) {
    int i;
    for (i = 0; i < MAX_CONTROLLERS; i++) {
        PadPort* p = &s_ports[i];
        if (p->id < 0 || p->id != (int32_t)instance_id) continue;
        printf("[input] port %d: %s disconnected\n", i + 1, pad_name(p));
        if (p->gc) SDL_CloseGamepad(p->gc);
        if (p->joy) SDL_CloseJoystick(p->joy);
        memset(p, 0, sizeof *p);
        p->id = -1;
        clear_port_status(i);   /* nothing stays held */
        if (s_active == i) s_active = ACTIVE_NONE;
        /* a pad that found all ports taken can have this one now */
        {
            int d, n = 0;
            SDL_JoystickID* ids = SDL_GetJoysticks(&n);
            for (d = 0; ids && d < n; d++)
                if (ids[d] != (SDL_JoystickID)instance_id)
                    input_on_device_added(ids[d]);
            SDL_free(ids);
        }
        return;
    }
}

void input_init(void) {
    int i, n = 0;
    SDL_JoystickID* ids;

    for (i = 0; i < MAX_CONTROLLERS; i++)
        s_ports[i].id = -1;

    /* Nintendo pads map by label (see sdl_button_to_pad); SDL3 has no
     * SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS any more. */

    /* Initialize SDL gamepad subsystem */
    if (!SDL_WasInit(SDL_INIT_GAMEPAD)) {
        if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
            fprintf(stderr, "SDL_InitSubSystem(GAMEPAD) failed: %s\n", SDL_GetError());
        }
    }

    /* Load gamecontrollerdb.txt if available */
    SDL_AddGamepadMappingsFromFile("gamecontrollerdb.txt");

    /* Open the pads already connected; later ones arrive as events */
    ids = SDL_GetJoysticks(&n);
    for (i = 0; ids && i < n; i++)
        input_on_device_added(ids[i]);
    SDL_free(ids);
    if (n == 0)
        printf("No gamepads found (keyboard on port 1; pads can be plugged in any time)\n");
    s_keyboard_fallback = true;

    /* Initialize player pad map */
    for (i = 0; i < MAX_CONTROLLERS; i++) {
        player_pad_map[i] = i;
    }

    printf("Input system initialized\n");
}

void input_shutdown(void) {
    for (int i = 0; i < MAX_CONTROLLERS; i++) {
        PadPort* p = &s_ports[i];
        if (p->gc) SDL_CloseGamepad(p->gc);
        if (p->joy) SDL_CloseJoystick(p->joy);
        memset(p, 0, sizeof *p);
        p->id = -1;
    }
    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
}

static uint16_t sdl_button_to_pad(SDL_Gamepad* gc, int sdl_button) {
    switch (sdl_button) {
        case SDL_GAMEPAD_BUTTON_SOUTH:
        case SDL_GAMEPAD_BUTTON_EAST:
        case SDL_GAMEPAD_BUTTON_WEST:
        case SDL_GAMEPAD_BUTTON_NORTH:
            /* Face buttons map by the label printed on the pad, as SDL2's
             * (default on) USE_BUTTON_LABELS did: the button labelled A is GC
             * A, B is GC B, X is GC X, Y is GC Y.  SDL3 buttons are positional;
             * SDL_GetGamepadButtonLabel gives what the pad prints there (Xbox
             * south = A, PlayStation south = Cross, Nintendo south = B, whose A
             * label is EAST). */
            switch (SDL_GetGamepadButtonLabel(gc, (SDL_GamepadButton)sdl_button)) {
            case SDL_GAMEPAD_BUTTON_LABEL_A:
            case SDL_GAMEPAD_BUTTON_LABEL_CROSS:    return PAD_BUTTON_A;
            case SDL_GAMEPAD_BUTTON_LABEL_B:
            case SDL_GAMEPAD_BUTTON_LABEL_CIRCLE:   return PAD_BUTTON_B;
            case SDL_GAMEPAD_BUTTON_LABEL_X:
            case SDL_GAMEPAD_BUTTON_LABEL_SQUARE:   return PAD_BUTTON_X;
            case SDL_GAMEPAD_BUTTON_LABEL_Y:
            case SDL_GAMEPAD_BUTTON_LABEL_TRIANGLE: return PAD_BUTTON_Y;
            default: break;   /* unlabelled: by position */
            }
            return sdl_button == SDL_GAMEPAD_BUTTON_SOUTH ? PAD_BUTTON_A :
                   sdl_button == SDL_GAMEPAD_BUTTON_EAST  ? PAD_BUTTON_B :
                   sdl_button == SDL_GAMEPAD_BUTTON_WEST  ? PAD_BUTTON_X : PAD_BUTTON_Y;
        case SDL_GAMEPAD_BUTTON_START:          return PAD_BUTTON_START;
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return PAD_TRIGGER_Z;
        /* LEFT_SHOULDER, BACK, GUIDE, stick clicks: no GC counterpart */
        case SDL_GAMEPAD_BUTTON_DPAD_UP:        return PAD_BUTTON_UP;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:      return PAD_BUTTON_DOWN;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:      return PAD_BUTTON_LEFT;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:     return PAD_BUTTON_RIGHT;
        default: return 0;
    }
}

/* One host stick (SDL axes, y down) -> the emulated GC stick as PADRead
 * reports it (gc_stick.c: deadzone, calibration, GC octagon gate).  The
 * game's own processing of that value happens once all inputs are in
 * (game_process_sticks). */
static void map_stick(PadPort* p, int stick, int sx, int sy, int8_t* ox, int8_t* oy) {
    float x = gcs_axis_norm(sx), y = -gcs_axis_norm(sy);
    p->view.host[stick][0] = x;
    p->view.host[stick][1] = y;
    gcs_map_stick(&p->cfg, stick, x, y, ox, oy, &p->view.norm[stick]);
}

/* Trigger travel 0..32767 -> GC analog value, and the digital click with
 * hysteresis.  At the click the analog value is full (0x8C), as on a GC
 * trigger, which the boat paddles check for exactly. */
static uint8_t trigger_axis(int raw, uint8_t* click, uint16_t* buttons, uint16_t bit) {
    if (raw < 0) raw = 0;
    if (*click ? raw < TRIGGER_CLICK_OFF : raw >= TRIGGER_CLICK_ON)
        *click = !*click;
    if (*click) {
        *buttons |= bit;
        return TRIGGER_MAX;
    }
    return (uint8_t)((raw * TRIGGER_MAX + 16383) / 32767);
}

/* Read a port's pad into its GC pad status */
static void read_port(int port) {
    PadPort* p = &s_ports[port];
    PADStatus* curr = &pad_status_groups[port].status[0];
    uint16_t buttons = 0;
    int b;

    if (p->gc) {
        SDL_Gamepad* gc = p->gc;
        for (b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; b++)
            if (SDL_GetGamepadButton(gc, (SDL_GamepadButton)b))
                buttons |= sdl_button_to_pad(gc, b);
        map_stick(p, GCS_MAIN, SDL_GetGamepadAxis(gc, SDL_GAMEPAD_AXIS_LEFTX),
                  SDL_GetGamepadAxis(gc, SDL_GAMEPAD_AXIS_LEFTY), &curr->stickX, &curr->stickY);
        map_stick(p, GCS_C, SDL_GetGamepadAxis(gc, SDL_GAMEPAD_AXIS_RIGHTX),
                  SDL_GetGamepadAxis(gc, SDL_GAMEPAD_AXIS_RIGHTY), &curr->substickX, &curr->substickY);
        curr->triggerLeft = trigger_axis(SDL_GetGamepadAxis(gc, SDL_GAMEPAD_AXIS_LEFT_TRIGGER),
                                         &p->trig_click[0], &buttons, PAD_TRIGGER_L);
        curr->triggerRight = trigger_axis(SDL_GetGamepadAxis(gc, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER),
                                          &p->trig_click[1], &buttons, PAD_TRIGGER_R);
    } else if (p->joy) {
        /* Generic DirectInput layout: buttons 1-4 = face (south, east, west,
         * north), 5/6 = L1/R1, 7/8 = L2/R2, 10 = start; axes 0/1 = left
         * stick, 2/3 = right stick; hat 0 = d-pad.  Digital L2/R2 = full
         * GC L/R. */
        static const uint16_t bmap[10] = {
            PAD_BUTTON_A, PAD_BUTTON_B, PAD_BUTTON_X, PAD_BUTTON_Y,
            0, PAD_TRIGGER_Z, PAD_TRIGGER_L, PAD_TRIGGER_R, 0, PAD_BUTTON_START,
        };
        SDL_Joystick* j = p->joy;
        int nb = SDL_GetNumJoystickButtons(j), na = SDL_GetNumJoystickAxes(j);
        for (b = 0; b < nb && b < 10; b++)
            if (SDL_GetJoystickButton(j, b)) buttons |= bmap[b];
        if (SDL_GetNumJoystickHats(j) > 0) {
            Uint8 h = SDL_GetJoystickHat(j, 0);
            if (h & SDL_HAT_UP)    buttons |= PAD_BUTTON_UP;
            if (h & SDL_HAT_DOWN)  buttons |= PAD_BUTTON_DOWN;
            if (h & SDL_HAT_LEFT)  buttons |= PAD_BUTTON_LEFT;
            if (h & SDL_HAT_RIGHT) buttons |= PAD_BUTTON_RIGHT;
        }
        map_stick(p, GCS_MAIN, na > 0 ? SDL_GetJoystickAxis(j, 0) : 0,
                  na > 1 ? SDL_GetJoystickAxis(j, 1) : 0, &curr->stickX, &curr->stickY);
        map_stick(p, GCS_C, na > 2 ? SDL_GetJoystickAxis(j, 2) : 0,
                  na > 3 ? SDL_GetJoystickAxis(j, 3) : 0, &curr->substickX, &curr->substickY);
        curr->triggerLeft = (buttons & PAD_TRIGGER_L) ? TRIGGER_MAX : 0;
        curr->triggerRight = (buttons & PAD_TRIGGER_R) ? TRIGGER_MAX : 0;
    }
    curr->button = buttons;
    /* Analog A/B buttons */
    curr->analogA = (buttons & PAD_BUTTON_A) ? 255 : 0;
    curr->analogB = (buttons & PAD_BUTTON_B) ? 255 : 0;

    /* "Used last" for the button prompts: a new press, or a stick / trigger
     * pushed well past its rest position (not drift) */
    {
        uint16_t eng = buttons;
        if (p->view.norm[GCS_MAIN] > 0.6f)
            eng |= 0x8000;
        if (p->view.norm[GCS_C] > 0.6f)
            eng |= 0x4000;
        if (curr->triggerLeft > TRIGGER_MAX / 2 || curr->triggerRight > TRIGGER_MAX / 2)
            eng |= 0x2000;
        if (eng & ~p->engaged)
            s_active = port;
        p->engaged = eng;
    }
}

void input_on_key_down(int keycode) {
    if (!s_keyboard_fallback) return;

    for (int i = 0; i < INPUT_GAMEPAD_MAX; i++) {
        if (s_keyboard_map[i] == keycode) {
            uint16_t* button = &pad_status_groups[0].status[0].button;
            switch (i) {
                case INPUT_GAMEPAD_A:          *button |= PAD_BUTTON_A; break;
                case INPUT_GAMEPAD_B:          *button |= PAD_BUTTON_B; break;
                case INPUT_GAMEPAD_X:          *button |= PAD_BUTTON_X; break;
                case INPUT_GAMEPAD_Y:          *button |= PAD_BUTTON_Y; break;
                case INPUT_GAMEPAD_START:      *button |= PAD_BUTTON_START; break;
                case INPUT_GAMEPAD_LEFT_SHOULDER: *button |= PAD_TRIGGER_L; break;
                case INPUT_GAMEPAD_RIGHT_SHOULDER: *button |= PAD_TRIGGER_R; break;
                case INPUT_GAMEPAD_DPAD_UP:    *button |= PAD_BUTTON_UP; break;
                case INPUT_GAMEPAD_DPAD_DOWN:  *button |= PAD_BUTTON_DOWN; break;
                case INPUT_GAMEPAD_DPAD_LEFT:  *button |= PAD_BUTTON_LEFT; break;
                case INPUT_GAMEPAD_DPAD_RIGHT: *button |= PAD_BUTTON_RIGHT; break;
                default: break;
            }
            break;
        }
    }
}

void input_on_key_up(int keycode) {
    if (!s_keyboard_fallback) return;

    for (int i = 0; i < INPUT_GAMEPAD_MAX; i++) {
        if (s_keyboard_map[i] == keycode) {
            uint16_t* button = &pad_status_groups[0].status[0].button;
            switch (i) {
                case INPUT_GAMEPAD_A:          *button &= ~PAD_BUTTON_A; break;
                case INPUT_GAMEPAD_B:          *button &= ~PAD_BUTTON_B; break;
                case INPUT_GAMEPAD_X:          *button &= ~PAD_BUTTON_X; break;
                case INPUT_GAMEPAD_Y:          *button &= ~PAD_BUTTON_Y; break;
                case INPUT_GAMEPAD_START:      *button &= ~PAD_BUTTON_START; break;
                case INPUT_GAMEPAD_LEFT_SHOULDER: *button &= ~PAD_TRIGGER_L; break;
                case INPUT_GAMEPAD_RIGHT_SHOULDER: *button &= ~PAD_TRIGGER_R; break;
                case INPUT_GAMEPAD_DPAD_UP:    *button &= ~PAD_BUTTON_UP; break;
                case INPUT_GAMEPAD_DPAD_DOWN:  *button &= ~PAD_BUTTON_DOWN; break;
                case INPUT_GAMEPAD_DPAD_LEFT:  *button &= ~PAD_BUTTON_LEFT; break;
                case INPUT_GAMEPAD_DPAD_RIGHT: *button &= ~PAD_BUTTON_RIGHT; break;
                default: break;
            }
            break;
        }
    }
}

void input_on_gamepad_button_down(int controller, int button) {
    if (controller < 0 || controller >= MAX_CONTROLLERS) return;

    uint16_t* pad_button = &pad_status_groups[controller].status[0].button;
    switch (button) {
        case INPUT_GAMEPAD_A:          *pad_button |= PAD_BUTTON_A; break;
        case INPUT_GAMEPAD_B:          *pad_button |= PAD_BUTTON_B; break;
        case INPUT_GAMEPAD_X:          *pad_button |= PAD_BUTTON_X; break;
        case INPUT_GAMEPAD_Y:          *pad_button |= PAD_BUTTON_Y; break;
        case INPUT_GAMEPAD_START:      *pad_button |= PAD_BUTTON_START; break;
        case INPUT_GAMEPAD_LEFT_SHOULDER: break; /* LB: unused */
        case INPUT_GAMEPAD_RIGHT_SHOULDER: *pad_button |= PAD_TRIGGER_Z; break;
        case INPUT_GAMEPAD_DPAD_UP:    *pad_button |= PAD_BUTTON_UP; break;
        case INPUT_GAMEPAD_DPAD_DOWN:  *pad_button |= PAD_BUTTON_DOWN; break;
        case INPUT_GAMEPAD_DPAD_LEFT:  *pad_button |= PAD_BUTTON_LEFT; break;
        case INPUT_GAMEPAD_DPAD_RIGHT: *pad_button |= PAD_BUTTON_RIGHT; break;
        default: break;
    }
}

void input_on_gamepad_button_up(int controller, int button) {
    if (controller < 0 || controller >= MAX_CONTROLLERS) return;

    uint16_t* pad_button = &pad_status_groups[controller].status[0].button;
    switch (button) {
        case INPUT_GAMEPAD_A:          *pad_button &= ~PAD_BUTTON_A; break;
        case INPUT_GAMEPAD_B:          *pad_button &= ~PAD_BUTTON_B; break;
        case INPUT_GAMEPAD_X:          *pad_button &= ~PAD_BUTTON_X; break;
        case INPUT_GAMEPAD_Y:          *pad_button &= ~PAD_BUTTON_Y; break;
        case INPUT_GAMEPAD_START:      *pad_button &= ~PAD_BUTTON_START; break;
        case INPUT_GAMEPAD_LEFT_SHOULDER: break;
        case INPUT_GAMEPAD_RIGHT_SHOULDER: *pad_button &= ~PAD_TRIGGER_Z; break;
        case INPUT_GAMEPAD_DPAD_UP:    *pad_button &= ~PAD_BUTTON_UP; break;
        case INPUT_GAMEPAD_DPAD_DOWN:  *pad_button &= ~PAD_BUTTON_DOWN; break;
        case INPUT_GAMEPAD_DPAD_LEFT:  *pad_button &= ~PAD_BUTTON_LEFT; break;
        case INPUT_GAMEPAD_DPAD_RIGHT: *pad_button &= ~PAD_BUTTON_RIGHT; break;
        default: break;
    }
}

void input_on_gamepad_axis(int controller, int axis, float value) {
    if (controller < 0 || controller >= MAX_CONTROLLERS) return;

    PADStatus* status = &pad_status_groups[controller].status[0];
    switch (axis) {
        case INPUT_GAMEPAD_AXIS_LEFTX:  status->stickX = (int8_t)(value * 127.0f); break;
        case INPUT_GAMEPAD_AXIS_LEFTY:  status->stickY = (int8_t)(-value * 127.0f); break;
        case INPUT_GAMEPAD_AXIS_RIGHTX: status->substickX = (int8_t)(value * 127.0f); break;
        case INPUT_GAMEPAD_AXIS_RIGHTY: status->substickY = (int8_t)(-value * 127.0f); break;
        case INPUT_GAMEPAD_AXIS_TRIGGERLEFT:  status->triggerLeft = (uint8_t)((value + 1.0f) * 127.5f); break;
        case INPUT_GAMEPAD_AXIS_TRIGGERRIGHT: status->triggerRight = (uint8_t)((value + 1.0f) * 127.5f); break;
        default: break;
    }
}

#ifdef SMB2_LINK_GAME
/* Scripted input injection for automated headless testing.
 * SMB2_INPUT_SCRIPT = comma-separated events: "frame:TOK" (button held 10
 * frames) or "start-end:TOK" (held across the range).  TOK is one of
 * START A B X Y L R Z UP DOWN LEFT RIGHT (buttons), SU SD SL SR (main stick
 * full deflection), TL TR / TLnnn TRnnn (analog triggers) or ROW / ROWnn
 * (alternate L/R trigger pulls; see parse_input_script).  Frames count
 * input polls (once per game frame).
 * Virtual pads (SDL virtual joysticks, to test hot-plugging and the pad
 * mapping through SDL without hardware; with a 2 suffix, VADDX2 etc., port 1
 * stays with the keyboard/script and the pad plugs into port 2): VADDX / VADDP4 / VADDP5 / VADDS
 * plug in an Xbox / PS4 / PS5 / Switch Pro pad, VDEL unplugs it, VBn holds
 * its SDL button n, VLT / VRT (VLTnnnnn / VRTnnnnn) pull its triggers,
 * VLXn / VLYn / VRXn / VRYn hold its stick axes at SDL value n (-32768..32767,
 * y down as SDL reports it) -- the whole host-stick path.
 * Analog sticks at exact values: SXn / SYn (control stick) and CXn / CYn
 * (C stick) hold port 1's emulated GC pad value n (-128..127, y up, what
 * PADRead would return), which then goes through the game's own stick
 * processing like a real pad.  SU / SD / SL / SR (and AUTO) instead set the
 * value the game sees after that processing (+-100: full tilt); GXn / GYn
 * set that processed value exactly (n != 0). */
typedef struct {
    long start, end;
    uint16_t buttons;
    int8_t stickX, stickY;
    int8_t autopilot;
    int8_t hotkey;         /* F1 token: PC settings overlay toggle */
    int8_t esc;            /* ESC token: the Esc key (pc_input_esc_pressed) */
    uint8_t trigL, trigR;  /* analog trigger values (TL/TR tokens) */
    int16_t row;           /* ROW token: alternate L/R every `row` frames */
    int8_t vadd, vdel;     /* virtual pad: plug in (kind 1-4) / unplug */
    int8_t vport2;         /* VADD*2: plug it in on port 2 or later */
    int8_t vbtn;           /* virtual pad: SDL button + 1 held */
    int16_t vlt, vrt;      /* virtual pad: trigger axis values */
    uint8_t rawmask;       /* SX SY CX CY tokens present (bits 0-3) */
    int8_t raw[4];         /* emulated GC stick values: SX SY CX CY */
    uint8_t vaxmask;       /* VLX VLY VRX VRY tokens present (bits 0-3) */
    int16_t vax[4];        /* virtual pad stick axes (SDL values) */
} ScriptEvent;

/* Closed-loop goal steering for automated testing ("AUTO" script token):
 * hold the stick up and steer the ball's x (and lateral velocity) back to 0,
 * which lines it up with the goal gate at the stage origin. */
struct AutoBall { uint32_t pad0; float px, py, pz;
                  float prevx, prevy, prevz; float vx, vy, vz; };
extern struct AutoBall balls[];
extern int main_mode;
#define SCRIPT_TRIGGER_FULL 0x8C
#define SCRIPT_MAX_EVENTS 4096
static ScriptEvent s_script[SCRIPT_MAX_EVENTS];
long g_input_script_frame;   /* input polls so far (script frame numbers) */
static int s_script_count = -1;

static void parse_input_script(void) {
    extern const char *g_input_script_arg;
    const char *env = g_input_script_arg ? g_input_script_arg
                                         : getenv("SMB2_INPUT_SCRIPT");
    char *buf;
    char *tok;
    s_script_count = 0;
    if (!env) return;
    /* no fixed-size buffer: soak scripts hold hundreds of events */
    buf = strdup(env);
    if (!buf) return;
    for (tok = strtok(buf, ","); tok; tok = strtok(NULL, ",")) {
        if (s_script_count >= SCRIPT_MAX_EVENTS) {
            printf("[input-script] more than %d events - rest ignored\n", SCRIPT_MAX_EVENTS);
            break;
        }
        long a = 0, b = 0;
        char name[16] = {0};
        ScriptEvent *ev = &s_script[s_script_count];
        if (sscanf(tok, "%ld-%ld:%15s", &a, &b, name) == 3) { /* range */ }
        else if (sscanf(tok, "%ld:%15s", &a, name) == 2) b = a + 10;
        else continue;
        memset(ev, 0, sizeof *ev);
        ev->start = a;
        ev->end = b;
        if      (!strcmp(name, "START")) ev->buttons = PAD_BUTTON_START;
        else if (!strcmp(name, "A"))     ev->buttons = PAD_BUTTON_A;
        else if (!strcmp(name, "B"))     ev->buttons = PAD_BUTTON_B;
        else if (!strcmp(name, "X"))     ev->buttons = PAD_BUTTON_X;
        else if (!strcmp(name, "Y"))     ev->buttons = PAD_BUTTON_Y;
        else if (!strcmp(name, "L"))     ev->buttons = PAD_TRIGGER_L;
        else if (!strcmp(name, "R"))     ev->buttons = PAD_TRIGGER_R;
        else if (!strcmp(name, "Z"))     ev->buttons = PAD_TRIGGER_Z;
        else if (!strcmp(name, "UP"))    ev->buttons = PAD_BUTTON_UP;
        else if (!strcmp(name, "DOWN"))  ev->buttons = PAD_BUTTON_DOWN;
        else if (!strcmp(name, "LEFT"))  ev->buttons = PAD_BUTTON_LEFT;
        else if (!strcmp(name, "RIGHT")) ev->buttons = PAD_BUTTON_RIGHT;
        else if (!strcmp(name, "SU"))    ev->stickY = 100;
        else if (!strcmp(name, "SD"))    ev->stickY = -100;
        else if (!strcmp(name, "SL"))    ev->stickX = -100;
        else if (!strcmp(name, "SR"))    ev->stickX = 100;
        else if (name[0] == 'G' && (name[1] == 'X' || name[1] == 'Y') && name[2]) {
            /* GXn / GYn: the processed value the game sees (as SU..SR) */
            int v = atoi(name + 2);
            v = v < -128 ? -128 : v > 127 ? 127 : v;
            if (name[1] == 'X') ev->stickX = (int8_t)v; else ev->stickY = (int8_t)v;
        }
        else if (!strcmp(name, "AUTO"))  ev->autopilot = 1;
        else if (!strcmp(name, "F1"))    ev->hotkey = 1;
        else if (!strcmp(name, "ESC"))   ev->esc = 1;
        else if (!strcmp(name, "STK"))   ev->hotkey = 2;   /* Controller Sticks screen */
        else if (!strcmp(name, "UPD"))   ev->hotkey = 3;   /* Updates screen (pc_update_ui.c) */
        else if (!strcmp(name, "MC"))    ev->hotkey = 4;   /* Memory Card screen (pc_memcard_ui.c) */
        /* Analog triggers: TL/TR = full press (0x8C, the value the game
         * treats as fully pulled), TLnnn/TRnnn = explicit 0-255 value.
         * ROW/ROWnn = alternate full L and R pulls, nn frames each (default
         * 15) -- the Monkey Boat paddling motion. */
        else if (!strncmp(name, "TL", 2)) ev->trigL = name[2] ? (uint8_t)atoi(name + 2) : SCRIPT_TRIGGER_FULL;
        else if (!strncmp(name, "TR", 2)) ev->trigR = name[2] ? (uint8_t)atoi(name + 2) : SCRIPT_TRIGGER_FULL;
        else if (!strcmp(name, "VADDX"))  ev->vadd = 1;
        else if (!strcmp(name, "VADDP4")) ev->vadd = 2;
        else if (!strcmp(name, "VADDP5")) ev->vadd = 3;
        else if (!strcmp(name, "VADDS"))  ev->vadd = 4;
        else if (!strcmp(name, "VADDX2") || !strcmp(name, "VADDP42") ||
                 !strcmp(name, "VADDP52") || !strcmp(name, "VADDS2")) {
            ev->vadd = name[4] == 'X' ? 1 : name[4] == 'S' ? 4 : name[5] == '4' ? 2 : 3;
            ev->vport2 = 1;
        }
        else if (!strcmp(name, "VDEL"))   ev->vdel = 1;
        else if ((name[0] == 'S' || name[0] == 'C') && (name[1] == 'X' || name[1] == 'Y') && name[2]) {
            int k = (name[0] == 'C') * 2 + (name[1] == 'Y');
            int v = atoi(name + 2);
            ev->rawmask |= (uint8_t)(1 << k);
            ev->raw[k] = (int8_t)(v < -128 ? -128 : v > 127 ? 127 : v);
        }
        else if (name[0] == 'V' && (name[1] == 'L' || name[1] == 'R') &&
                 (name[2] == 'X' || name[2] == 'Y') && name[3]) {
            int k = (name[1] == 'R') * 2 + (name[2] == 'Y');
            int v = atoi(name + 3);
            ev->vaxmask |= (uint8_t)(1 << k);
            ev->vax[k] = (int16_t)(v < -32768 ? -32768 : v > 32767 ? 32767 : v);
        }
        else if (!strncmp(name, "VB", 2) && name[2]) ev->vbtn = (int8_t)(atoi(name + 2) + 1);
        else if (!strncmp(name, "VLT", 3)) ev->vlt = name[3] ? (int16_t)atoi(name + 3) : 32767;
        else if (!strncmp(name, "VRT", 3)) ev->vrt = name[3] ? (int16_t)atoi(name + 3) : 32767;
        else if (!strncmp(name, "ROW", 3)) {
            ev->row = name[3] ? (int16_t)atoi(name + 3) : 15;
            if (ev->row <= 0) ev->row = 15;
        }
        else continue;
        printf("[input-script] %ld-%ld %s\n", a, b, name);
        s_script_count++;
    }
    free(buf);
}

/* Script virtual pad (VADD* / VDEL / VB / VLT / VRT tokens) */
static int32_t s_vpad = -1;   /* instance id of the script's virtual pad */

static void vpad_attach(int kind) {
    static const struct { Uint16 vid, pid; const char* name; } kinds[4] = {
        { 0x045E, 0x02EA, "Virtual Xbox One pad" },
        { 0x054C, 0x09CC, "Virtual PS4 pad" },
        { 0x054C, 0x0CE6, "Virtual PS5 pad" },
        { 0x057E, 0x2009, "Virtual Switch Pro pad" },
    };
    SDL_VirtualJoystickDesc desc;
    SDL_JoystickID id;
    if (s_vpad >= 0 || kind < 1 || kind > 4) return;
    /* SDL drops joystick input while no window has focus (headless test
     * runs never get focus); the scripted pad has to be read anyway */
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.vendor_id = kinds[kind - 1].vid;
    desc.product_id = kinds[kind - 1].pid;
    desc.name = kinds[kind - 1].name;
    id = SDL_AttachVirtualJoystick(&desc);
    if (!id) { printf("[vpad] attach failed: %s\n", SDL_GetError()); return; }
    s_vpad = (int32_t)id;
    printf("[vpad] attached %s (instance %u)\n", desc.name, (unsigned)id);
}

static void vpad_detach(void) {
    if (s_vpad >= 0) {
        SDL_DetachVirtualJoystick((SDL_JoystickID)s_vpad);
        printf("[vpad] detached\n");
    }
    s_vpad = -1;
}

/* Set the virtual pad's state; SDL reports it from the next update on. */
static void vpad_set(uint32_t buttons, int lt, int rt, const int16_t axes[4]) {
    SDL_Joystick* j = s_vpad >= 0 ? SDL_GetJoystickFromID((SDL_JoystickID)s_vpad) : NULL;
    int b;
    if (!j) return;   /* not opened by the hot-plug code yet */
    for (b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; b++)
        SDL_SetJoystickVirtualButton(j, b, ((buttons >> b) & 1) != 0);
    /* the virtual mapping scales the full joystick axis range to the
     * trigger's 0..32767 */
    SDL_SetJoystickVirtualAxis(j, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, (Sint16)(lt * 2 - 32768));
    SDL_SetJoystickVirtualAxis(j, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, (Sint16)(rt * 2 - 32768));
    for (b = 0; b < 4; b++)   /* LEFTX LEFTY RIGHTX RIGHTY */
        SDL_SetJoystickVirtualAxis(j, b, axes[b]);
}

/* ---- the game's stick processing (GC: g_read_inputs_and_maybe_reset_console,
 * 0x80009A10, run on every PADRead result before process_inputs).  The PC
 * build stubs that function and does its work here, at the end of the poll,
 * so every reader of pad_status_groups (the game, the F1 overlay's own loop)
 * sees processed values.  Data the DOL keeps for it: */
struct StickPoint { int8_t x, y; };
/* 0x80081AC0: per-pad control stick gate (the Options stick adjustment
 * stores its measurement here) */
int8_t lbl_80081AC0[MAX_CONTROLLERS][8][2] = {
#define GATE { { 84, 0 }, { 59, 59 }, { 0, 84 }, { -59, 59 }, { -84, 0 }, { -59, -59 }, { 0, -84 }, { 59, -59 } }
    GATE, GATE, GATE, GATE
#undef GATE
};
/* 0x8000A730 / 0x8000A75C: the game data save keeps the gates (memcard_mkb2.c,
 * save offset 0x31C, 0x40 bytes), so a stick adjustment made in Options
 * persists.  Saves of earlier port builds hold zeros there (these were
 * stubs); a gate is only loaded when it is one the adjustment could store
 * (each axis point on its own side), else the current one is kept. */
void fn_8000A730(void* dst) {
    memcpy(dst, lbl_80081AC0, sizeof(lbl_80081AC0));
}

void fn_8000A75C(const void* src) {
    const int8_t (*g)[8][2] = (const int8_t (*)[8][2])src;
    int i;
    for (i = 0; i < MAX_CONTROLLERS; i++)
        if (g[i][0][0] > 0 && g[i][2][1] > 0 && g[i][4][0] < 0 && g[i][6][1] < 0)
            memcpy(lbl_80081AC0[i], g[i], sizeof(lbl_80081AC0[i]));
}

/* 0x80145210: the unprocessed pads (PADRead result; the Options controller
 * screens draw from it) */
ControllerInfo lbl_80145210[MAX_CONTROLLERS];

/* 0x8000A380: one stick point through a gate (Options stick adjustment) */
void fn_8000A380(struct StickPoint* point, struct StickPoint* gate) {
    gcs_game_stick(&point->x, &point->y, (const int8_t (*)[2])gate);
}

static void game_process_sticks(int log) {
    int i;
    for (i = 0; i < MAX_CONTROLLERS; i++) {
        PADStatus* s = &pad_status_groups[i].status[0];
        ControllerInfo* r = &lbl_80145210[i];
        int8_t gx = s->stickX, gy = s->stickY, cx = s->substickX, cy = s->substickY;
        r->status[1].button = r->status[0].button;
        r->status[0] = *s;
        r->status[2].button = (uint16_t)(s->button & ~r->status[1].button);
        r->status[3].button = (uint16_t)(r->status[1].button & ~s->button);
        gcs_game_stick(&s->stickX, &s->stickY, (const int8_t (*)[2])lbl_80081AC0[i]);
        gcs_game_stick(&s->substickX, &s->substickY, gcs_game_c_gate);
        if (s_ports[i].id >= 0) {
            PadStickView* v = &s_ports[i].view;
            v->gc[0][0] = gx; v->gc[0][1] = gy; v->gc[1][0] = cx; v->gc[1][1] = cy;
            v->game[0][0] = s->stickX; v->game[0][1] = s->stickY;
            v->game[1][0] = s->substickX; v->game[1][1] = s->substickY;
        }
        if (log) {
            static int8_t last[MAX_CONTROLLERS][8];
            int8_t now[8] = { gx, gy, cx, cy, s->stickX, s->stickY, s->substickX, s->substickY };
            if (memcmp(now, last[i], 8)) {
                extern long g_input_script_frame;
                printf("[stick] frame %ld port %d gc main (%d,%d) c (%d,%d) -> game main (%d,%d) c (%d,%d)\n",
                       g_input_script_frame, i + 1, gx, gy, cx, cy,
                       s->stickX, s->stickY, s->substickX, s->substickY);
                memcpy(last[i], now, 8);
            }
        }
    }
}

/* With the real game linked, the decompiled input.c provides process_inputs /
 * threshold_analog_inputs / merge_inputs / g_calc_frames_since_last_input_change,
 * operating on the pad_status_groups global defined above (which stands in for
 * the GC pad data the DOL would fill from PADRead).  This function is the SDL
 * poller that fills it, called once per frame by the shim's video_finish_frame. */
/* game = 1 for the call from video_finish_frame (E3 pad hook, see below);
 * the modal shim screens call input_poll_gc_pads() and stay local-only. */
/* the local sample, kept apart from pad_status_groups[].status[0] while a
 * non-live source overwrites that with the exchanged pads (E1 D6) */
static PADStatus s_local_pads[MAX_CONTROLLERS];
static void poll_gc_pads_impl(int game);
void input_poll_gc_pads(void) { poll_gc_pads_impl(0); }
void input_poll_gc_pads_game(void) { poll_gc_pads_impl(1); }
static void poll_gc_pads_impl(int game)
#else
void process_inputs(void)
#endif
{
    /* Copy current status to previous (index 1) */
    for (int i = 0; i < MAX_CONTROLLERS; i++) {
        pad_status_groups[i].status[1] = pad_status_groups[i].status[0];
    }
#ifdef SMB2_LINK_GAME
    /* E3: with a non-live pad source status[0] holds the exchanged (remote /
     * replayed) pads, not the local state the sampling code below builds on
     * (keyboard OR-in, trigger and stick persistence): put the private
     * local sample back first. */
    const int sync = game && inputsync_overrides();
    if (sync)
        for (int i = 0; i < MAX_CONTROLLERS; i++)
            pad_status_groups[i].status[0] = s_local_pads[i];
#endif
    s_esc_pressed = 0;

    /* Poll SDL events and update current status (index 0) */
    SDL_UpdateGamepads();

    for (int i = 0; i < MAX_CONTROLLERS; i++) {
        /* PAD_ERR_NO_CONTROLLER (-1) for unplugged ports, as PADRead
         * reports.  Port 1 is always connected (keyboard / script).
         * sel_ngc counts connected pads for the player-count menus. */
        pad_status_groups[i].status[0].err = (i == 0 || s_ports[i].id >= 0) ? 0 : -1;
        if (s_ports[i].id >= 0)
            read_port(i);
    }

#ifdef SMB2_LINK_GAME
    /* Keyboard for player 1, read from the key state.  ORed in, so the
     * keyboard also works with a gamepad on port 1.
     *   WASD = control stick, arrows = +Control Pad, Z/X/C/V = A/B/X/Y,
     *   Q/E = L/R (full trigger pull), Space = Z, Enter = START/PAUSE */
    {
        static const struct { SDL_Scancode sc; uint16_t bit; } kbmap[] = {
            { SDL_SCANCODE_Z, PAD_BUTTON_A }, { SDL_SCANCODE_X, PAD_BUTTON_B },
            { SDL_SCANCODE_C, PAD_BUTTON_X }, { SDL_SCANCODE_V, PAD_BUTTON_Y },
            { SDL_SCANCODE_RETURN, PAD_BUTTON_START },
            { SDL_SCANCODE_Q, PAD_TRIGGER_L }, { SDL_SCANCODE_E, PAD_TRIGGER_R },
            { SDL_SCANCODE_SPACE, PAD_TRIGGER_Z },
            { SDL_SCANCODE_UP, PAD_BUTTON_UP }, { SDL_SCANCODE_DOWN, PAD_BUTTON_DOWN },
            { SDL_SCANCODE_LEFT, PAD_BUTTON_LEFT }, { SDL_SCANCODE_RIGHT, PAD_BUTTON_RIGHT },
        };
        static uint16_t prev_kb;
        static int prev_wasd;
        const bool* ks = SDL_GetKeyboardState(NULL);
        PADStatus* curr = &pad_status_groups[0].status[0];
        int pad1 = s_ports[0].id >= 0;
        int wasd = ks[SDL_SCANCODE_W] | ks[SDL_SCANCODE_A] << 1 |
                   ks[SDL_SCANCODE_S] << 2 | ks[SDL_SCANCODE_D] << 3;
        uint16_t kb = 0;
        size_t k;

        /* WASD stick: owns the stick without a pad on port 1, else only
         * while a key is down */
        /* full deflection of a GC stick in the key direction: the octagon's
         * cardinal (101) or diagonal (71, 71) notch */
        if (!pad1 || wasd) {
            int dx = (ks[SDL_SCANCODE_D] ? 1 : 0) - (ks[SDL_SCANCODE_A] && !ks[SDL_SCANCODE_D] ? 1 : 0);
            int dy = (ks[SDL_SCANCODE_W] ? 1 : 0) - (ks[SDL_SCANCODE_S] && !ks[SDL_SCANCODE_W] ? 1 : 0);
            curr->stickX = curr->stickY = 0;
            if (dx || dy)
                gcs_full_deflection(GCS_MAIN, atan2((double)dy, (double)dx), &curr->stickX, &curr->stickY);
        }
        for (k = 0; k < sizeof kbmap / sizeof kbmap[0]; k++)
            if (ks[kbmap[k].sc]) kb |= kbmap[k].bit;
        if (ks[SDL_SCANCODE_LALT] || ks[SDL_SCANCODE_RALT])
            kb &= ~PAD_BUTTON_START;   /* Alt+Enter is the fullscreen toggle */
        curr->button = (uint16_t)((curr->button & ~(prev_kb & ~kb)) | kb);
        /* Q/E: a GC trigger's click comes with the full analog value */
        if (kb & PAD_TRIGGER_L) curr->triggerLeft = TRIGGER_MAX;
        else if ((prev_kb & PAD_TRIGGER_L) && !pad1) curr->triggerLeft = 0;
        if (kb & PAD_TRIGGER_R) curr->triggerRight = TRIGGER_MAX;
        else if ((prev_kb & PAD_TRIGGER_R) && !pad1) curr->triggerRight = 0;
        /* Esc: not a pad button; the top-level menus (title screen, Mode
         * Select) open Quit to Desktop with it (pc_quit.c) */
        {
            static int prev_esc;
            int esc = ks[SDL_SCANCODE_ESCAPE] != 0;
            if (esc && !prev_esc) {
                s_esc_pressed = 1;
                s_active = ACTIVE_KEYBOARD;
            }
            prev_esc = esc;
        }
        if ((kb & ~prev_kb) || (wasd & ~prev_wasd))
            s_active = ACTIVE_KEYBOARD;
        prev_kb = kb;
        prev_wasd = wasd;
    }

    /* Scripted input injection (after keyboard emulation so it wins).
     * Injected bits are released once their window ends: in keyboard-fallback
     * mode nothing else clears status[0].button, so without the explicit
     * release every later press of the same button would be "still held". */
    {
#define script_frame g_input_script_frame
        static uint16_t prev_inject;
        uint16_t inject = 0;
        int8_t sx = 0, sy = 0;
        uint8_t tl = 0, tr = 0;
        uint32_t vbuttons = 0;
        int vlt = 0, vrt = 0;
        int8_t raw[4] = { 0 };
        int rawmask = 0;
        int16_t vax[4] = { 0 };
        int e, k;
        PADStatus *curr = &pad_status_groups[0].status[0];
        if (s_script_count < 0)
            parse_input_script();
        script_frame++;
        int auto_on = 0;
        for (e = 0; e < s_script_count; e++) {
            if (script_frame >= s_script[e].start && script_frame < s_script[e].end) {
                inject |= s_script[e].buttons;
                if (s_script[e].stickX) sx = s_script[e].stickX;
                if (s_script[e].stickY) sy = s_script[e].stickY;
                if (s_script[e].autopilot) auto_on = 1;
                if (s_script[e].esc && script_frame == s_script[e].start)
                    s_esc_pressed = 1;
                if (s_script[e].hotkey && script_frame == s_script[e].start) {
                    extern void pc_settings_overlay_request(void);
                    extern void pc_stick_screen_request(void);
                    extern void pc_update_screen_request(void);
                    if (s_script[e].hotkey == 2) pc_stick_screen_request();
                    else if (s_script[e].hotkey == 3) pc_update_screen_request();
                    else if (s_script[e].hotkey == 4) {
                        extern void pc_memcard_screen_request(void);
                        pc_memcard_screen_request();
                    }
                    else pc_settings_overlay_request();
                }
                if (s_script[e].trigL) tl = s_script[e].trigL;
                if (s_script[e].trigR) tr = s_script[e].trigR;
                if (script_frame == s_script[e].start) {
                    if (s_script[e].vport2) s_vpad_skip_port1 = 1;
                    if (s_script[e].vadd) vpad_attach(s_script[e].vadd);
                    if (s_script[e].vdel) vpad_detach();
                }
                if (s_script[e].vbtn) vbuttons |= 1u << (s_script[e].vbtn - 1);
                for (k = 0; k < 4; k++) {
                    if (s_script[e].rawmask & (1 << k)) { raw[k] = s_script[e].raw[k]; rawmask |= 1 << k; }
                    if (s_script[e].vaxmask & (1 << k)) vax[k] = s_script[e].vax[k];
                }
                if (s_script[e].vlt) vlt = s_script[e].vlt;
                if (s_script[e].vrt) vrt = s_script[e].vrt;
                if (s_script[e].row) {
                    if (((script_frame - s_script[e].start) / s_script[e].row) & 1)
                        tr = SCRIPT_TRIGGER_FULL;
                    else
                        tl = SCRIPT_TRIGGER_FULL;
                }
            }
        }
        if (auto_on && main_mode == 2) {
            struct AutoBall *b = &balls[0];
            float d = -(b->px * 8.0f + b->vx * 120.0f);
            if (d > 100.0f) d = 100.0f;
            else if (d < -100.0f) d = -100.0f;
            sx = (int8_t)d;
            sy = 100;
            if ((script_frame % 60) == 0)
                printf("[auto] f=%ld x=%.2f vx=%.3f -> sx=%d\n",
                       script_frame, b->px, b->vx, (int)sx);
        }
        /* exact emulated-pad values, then the game's processing for all
         * ports; SU/SD/SL/SR/AUTO set the processed value */
        if (rawmask & 1) curr->stickX = raw[0];
        if (rawmask & 2) curr->stickY = raw[1];
        if (rawmask & 4) curr->substickX = raw[2];
        if (rawmask & 8) curr->substickY = raw[3];
        game_process_sticks(s_script_count > 0);
        curr->button &= ~(prev_inject & ~inject);
        curr->button |= inject;
        if (sx) curr->stickX = sx;
        if (sy) curr->stickY = sy;
        /* Scripted triggers own the analog values while a script is active
         * (nothing else resets them in keyboard-fallback mode). */
        if (s_script_count > 0 && s_ports[0].id < 0) {
            curr->triggerLeft = tl;
            curr->triggerRight = tr;
        } else {
            if (tl) curr->triggerLeft = tl;
            if (tr) curr->triggerRight = tr;
        }
        if (inject & PAD_BUTTON_A) curr->analogA = 255;
        else if (prev_inject & PAD_BUTTON_A) curr->analogA = 0;
        if (inject & PAD_BUTTON_B) curr->analogB = 255;
        else if (prev_inject & PAD_BUTTON_B) curr->analogB = 0;
        if (inject != prev_inject)
            printf("[input-script] frame %ld held=0x%04x\n", script_frame, inject);
        vpad_set(vbuttons, vlt, vrt, vax);
        if (s_vpad >= 0) {   /* log what the virtual pad's port reads */
            static PADStatus last;
            int vp;
            for (vp = 0; vp < MAX_CONTROLLERS && s_ports[vp].id != s_vpad; vp++) {}
            if (vp < MAX_CONTROLLERS) {
                PADStatus* st = &pad_status_groups[vp].status[0];
                if (st->button != last.button || st->triggerLeft != last.triggerLeft ||
                    st->triggerRight != last.triggerRight)
                    printf("[vpad] frame %ld port %d button=0x%04x trigL=%d trigR=%d\n",
                           script_frame, vp + 1, st->button, st->triggerLeft, st->triggerRight);
                last = *st;
            }
        }
        prev_inject = inject;
#undef script_frame
    }

#ifdef SMB2_LINK_GAME
    /* E3 pad hook (docs/handoff/E3.md): the local sample is complete here
     * (devices + keyboard + script, post stick gate).  Keep it as the private
     * local state, hand the 4 pads to inputsync and use what comes back for
     * all 4 ports (value + err). */
    if (game) {
        SmbNetPad loc[INPUTSYNC_PORTS], out[INPUTSYNC_PORTS];
        for (int i = 0; i < MAX_CONTROLLERS; i++) {
            const PADStatus* st = &pad_status_groups[i].status[0];
            s_local_pads[i] = *st;
            loc[i].buttons = st->button;
            loc[i].stickX = st->stickX; loc[i].stickY = st->stickY;
            loc[i].substickX = st->substickX; loc[i].substickY = st->substickY;
            loc[i].triggerL = st->triggerLeft; loc[i].triggerR = st->triggerRight;
            loc[i].err = st->err;
        }
        inputsync_exchange(loc, out);
        for (int i = 0; sync && i < MAX_CONTROLLERS; i++) {
            PADStatus* st = &pad_status_groups[i].status[0];
            ControllerInfo* r = &lbl_80145210[i];
            st->button = out[i].buttons;
            st->stickX = out[i].stickX; st->stickY = out[i].stickY;
            st->substickX = out[i].substickX; st->substickY = out[i].substickY;
            st->triggerLeft = out[i].triggerL; st->triggerRight = out[i].triggerR;
            st->analogA = st->analogB = 0;
            st->err = out[i].err;
            /* the raw-pad copy the Options screens read (E1 D5): the exchanged
             * (post-gate) value; edge slots recomputed from its previous one */
            r->status[0] = *st;
            r->status[2].button = (uint16_t)(r->status[0].button & ~r->status[1].button);
            r->status[3].button = (uint16_t)(r->status[1].button & ~r->status[0].button);
        }
    }
#endif

    /* Derive the pressed/released/repeat slots ([2]/[3]/[4]) the GC pad
     * processing would provide; the game reads these directly. */
    for (int i = 0; i < MAX_CONTROLLERS; i++) {
        static int repeat_count[MAX_CONTROLLERS];
        uint16_t held = pad_status_groups[i].status[0].button;
        uint16_t prev = pad_status_groups[i].status[1].button;
        pad_status_groups[i].status[2] = pad_status_groups[i].status[0];
        pad_status_groups[i].status[2].button = (uint16_t)(held & ~prev);
        pad_status_groups[i].status[3].button = (uint16_t)(prev & ~held);
        if (held == prev && held != 0) {
            repeat_count[i]++;
            pad_status_groups[i].status[4].button =
                (repeat_count[i] > 20 && (repeat_count[i] % 4) == 0) ? held : 0;
        } else {
            repeat_count[i] = 0;
            pad_status_groups[i].status[4].button = (uint16_t)(held & ~prev);
        }
    }
    if (pad_status_groups[0].status[2].button)
        printf("[input] pressed=0x%04x held=0x%04x\n",
               pad_status_groups[0].status[2].button,
               pad_status_groups[0].status[0].button);
    pc_button_prompts_update();   /* PS/Xbox/GC prompt style for this frame */
#endif
}


/* Button prompt style (button_prompts.c): the SDL type of the pad used last,
 * else of player 1's pad, else of the first open one; -1 for the keyboard,
 * no pad, or a joystick without a controller mapping. */
int input_controller_type(void) {
    int i;
    if (s_active == ACTIVE_KEYBOARD)
        return -1;
    if (s_active >= 0 && s_active < MAX_CONTROLLERS && s_ports[s_active].id >= 0)
        return s_ports[s_active].type;
    for (i = 0; i < MAX_CONTROLLERS; i++)
        if (s_ports[i].id >= 0)
            return s_ports[i].type;
    return -1;
}

#ifndef SMB2_LINK_GAME
void threshold_analog_inputs(void) {
    /* Apply dead zones and convert to digital */
    for (int i = 0; i < MAX_CONTROLLERS; i++) {
        PADStatus* curr = &pad_status_groups[i].status[0];

        /* Apply dead zone to sticks */
        int16_t stickX = curr->stickX;
        int16_t stickY = curr->stickY;
        int16_t substickX = curr->substickX;
        int16_t substickY = curr->substickY;

        const int16_t DEAD_ZONE = 15;

        if (stickX > -DEAD_ZONE && stickX < DEAD_ZONE) stickX = 0;
        if (stickY > -DEAD_ZONE && stickY < DEAD_ZONE) stickY = 0;
        if (substickX > -DEAD_ZONE && substickX < DEAD_ZONE) substickX = 0;
        if (substickY > -DEAD_ZONE && substickY < DEAD_ZONE) substickY = 0;

        /* Store analog inputs */
        analog_inputs[i][0] = (uint16_t)stickX;
        analog_inputs[i][1] = (uint16_t)stickY;
        analog_inputs[i][2] = (uint16_t)substickX;
        analog_inputs[i][3] = (uint16_t)substickY;
        analog_inputs[i][4] = curr->triggerLeft | (curr->triggerRight << 8);
    }
}

void g_calc_frames_since_last_input_change(void) {
    for (int i = 0; i < MAX_CONTROLLERS; i++) {
        uint16_t curr_digital = pad_status_groups[i].status[0].button;
        uint16_t prev_digital = pad_status_groups[i].status[1].button;

        int16_t curr_stickX = pad_status_groups[i].status[0].stickX;
        int16_t prev_stickX = pad_status_groups[i].status[1].stickX;
        int16_t curr_stickY = pad_status_groups[i].status[0].stickY;
        int16_t prev_stickY = pad_status_groups[i].status[1].stickY;

        if (curr_digital != prev_digital ||
            curr_stickX != prev_stickX ||
            curr_stickY != prev_stickY) {
            frames_since_last_input_change[i] = 0;
        } else {
            if (frames_since_last_input_change[i] < 30000) {
                frames_since_last_input_change[i]++;
            }
        }
    }
}

void merge_inputs(void) {
    for (int i = 0; i < MAX_CONTROLLERS; i++) {
        uint16_t held = pad_status_groups[i].status[0].button;
        uint16_t prev = pad_status_groups[i].status[1].button;

        /* Pressed = held & ~prev */
        pad_status_groups[i].status[2].button = held & ~prev;

        /* Released = prev & ~held */
        pad_status_groups[i].status[3].button = prev & ~held;

        /* Repeat logic (simple) */
        static int repeat_count[MAX_CONTROLLERS] = {0};
        if (held == prev && held != 0) {
            repeat_count[i]++;
            if (repeat_count[i] > 30) { /* After ~0.5s at 60fps */
                pad_status_groups[i].status[4].button = held;
            } else {
                pad_status_groups[i].status[4].button = 0;
            }
        } else {
            repeat_count[i] = 0;
            pad_status_groups[i].status[4].button = 0;
        }

        merged_digital_inputs[i] = held;
        merged_analog_inputs[i] = analog_inputs[i][0] | (analog_inputs[i][1] << 8);
    }
}
#endif /* !SMB2_LINK_GAME */

void input_set_keyboard_mapping(int player, const int* key_map) {
    if (player < 0 || player >= MAX_CONTROLLERS) return;
    if (key_map) {
        memcpy(s_keyboard_map, key_map, sizeof(s_keyboard_map));
    }
}
/* ---- port API for the stick calibration screen (pad_config.h) ---- */
int input_port_connected(int port) {
    return port >= 0 && port < MAX_CONTROLLERS && s_ports[port].id >= 0;
}

const char* input_port_name(int port) {
    return input_port_connected(port) ? pad_name(&s_ports[port]) : "";
}

GcsPadConfig* input_port_config(int port) {
    return input_port_connected(port) ? &s_ports[port].cfg : NULL;
}

void input_port_config_save(int port) {
    if (!input_port_connected(port)) return;
    padcfg_sanitize(&s_ports[port].cfg);
    padcfg_put(s_ports[port].guid, pad_name(&s_ports[port]), &s_ports[port].cfg);
}

int input_port_view(int port, PadStickView* v) {
    if (!input_port_connected(port)) return 0;
    *v = s_ports[port].view;
    return 1;
}

/* ---- --list-controllers (main.c): what does SDL see?  No window. ---- */
static void lc_print_devices(void) {
    int n = 0, i;
    SDL_JoystickID* ids = SDL_GetJoysticks(&n);
    int valve = 0;
    printf("Joysticks: %d\n", n);
    for (i = 0; ids && i < n; i++) {
        SDL_JoystickID id = ids[i];
        char guid[64];
        char* map = NULL;
        const char* name = SDL_GetJoystickNameForID(id);
        const char* path = SDL_GetJoystickPathForID(id);
        int vid = SDL_GetJoystickVendorForID(id), pid = SDL_GetJoystickProductForID(id);
        bool is_gp = SDL_IsGamepad(id);
        SDL_GUIDToString(SDL_GetJoystickGUIDForID(id), guid, sizeof guid);
        printf("\n[%d] instance id %u: \"%s\"\n", i, (unsigned)id, name ? name : "(no name)");
        printf("    VID/PID      0x%04X / 0x%04X (version 0x%04X)%s\n", vid, pid,
               SDL_GetJoystickProductVersionForID(id),
               SDL_IsJoystickVirtual(id) ? " [virtual]" : "");
        printf("    GUID         %s\n", guid);
        printf("    path         %s\n", path ? path : "(none)");
        printf("    joystick type %d\n", (int)SDL_GetJoystickTypeForID(id));
        printf("    is gamepad   %s\n", is_gp ? "yes" : "no");
        if (is_gp) {
            int t = (int)SDL_GetGamepadTypeForID(id);
            printf("    gamepad type %d (%s)\n", t, type_name(t));
            map = SDL_GetGamepadMappingForID(id);
        }
        printf("    mapping      %s\n", map ? map : "none");
        SDL_free(map);
        if (vid == 0x28DE) valve = 1;
    }
    SDL_free(ids);
    if (n == 0)
        printf("\nSDL sees no joystick or controller devices. A controller that only does "
               "keyboard/mouse emulation is not listed.\n");
    else if (valve)
        printf("\nValve device found (VID 0x28DE): SDL's HIDAPI driver handles it; if it is "
               "listed as a gamepad above it will work in the game.\n");
}

int input_print_controllers(void) {
    static const char* const hints[] = {
        SDL_HINT_JOYSTICK_HIDAPI, SDL_HINT_JOYSTICK_HIDAPI_STEAM, SDL_HINT_JOYSTICK_RAWINPUT,
        SDL_HINT_JOYSTICK_WGI, SDL_HINT_XINPUT_ENABLED, SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,
    };
    int v, i;
    Uint64 t0;
    SDL_Event ev;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        printf("SDL_Init(VIDEO|GAMEPAD) failed: %s\n", SDL_GetError());
        return 1;
    }
    v = SDL_GetVersion();
    printf("SMB2 PC port --list-controllers\n");
    printf("SDL version  compiled %d.%d.%d, runtime %d.%d.%d (%s)\n",
           SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION,
           SDL_VERSIONNUM_MAJOR(v), SDL_VERSIONNUM_MINOR(v), SDL_VERSIONNUM_MICRO(v),
           SDL_GetRevision());
    printf("Platform     %s\n", SDL_GetPlatform());
    printf("Hints       ");
    for (i = 0; i < (int)(sizeof hints / sizeof hints[0]); i++) {
        const char* h = SDL_GetHint(hints[i]);
        printf(" %s=%s", hints[i], h ? h : "(default)");
    }
    printf("\n");
    SDL_AddGamepadMappingsFromFile("gamecontrollerdb.txt");
    printf("Waiting 1 s for late devices ...\n");
    fflush(stdout);
    t0 = SDL_GetTicks();
    while (SDL_GetTicks() - t0 < 1000) {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_JOYSTICK_ADDED)
                printf("  event: joystick added (instance %u)\n", (unsigned)ev.jdevice.which);
            else if (ev.type == SDL_EVENT_GAMEPAD_ADDED)
                printf("  event: gamepad added (instance %u)\n", (unsigned)ev.gdevice.which);
        }
        SDL_Delay(10);
    }
    lc_print_devices();
    fflush(stdout);
    SDL_Quit();
    return 0;
}

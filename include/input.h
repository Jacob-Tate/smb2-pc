#ifndef INPUT_H
#define INPUT_H

#include "platform.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* GameCube PADStatus layout - matches Dolphin SDK */
typedef struct {
    uint16_t button;
    int8_t   stickX;
    int8_t   stickY;
    int8_t   substickX;
    int8_t   substickY;
    uint8_t  triggerLeft;
    uint8_t  triggerRight;
    uint8_t  analogA;
    uint8_t  analogB;
    int8_t   err;
} PADStatus; /* size = 0xC */

/* GameCube button bitmasks */
#define PAD_BUTTON_LEFT       0x0001
#define PAD_BUTTON_RIGHT      0x0002
#define PAD_BUTTON_DOWN       0x0004
#define PAD_BUTTON_UP         0x0008
#define PAD_TRIGGER_Z         0x0010
#define PAD_TRIGGER_R         0x0020
#define PAD_TRIGGER_L         0x0040
#define PAD_BUTTON_A          0x0100
#define PAD_BUTTON_B          0x0200
#define PAD_BUTTON_X          0x0400
#define PAD_BUTTON_Y          0x0800
#define PAD_BUTTON_START      0x1000

/* Controller info - matches original game structure */
typedef struct {
    PADStatus status[5];  /* [0]=held, [1]=prev, [2]=pressed, [3]=released, [4]=repeat */
} ControllerInfo; /* size = 0x3C */

/* Maximum controllers */
#define MAX_CONTROLLERS 4

/* External variables matching original game */
extern ControllerInfo pad_status_groups[MAX_CONTROLLERS];
extern uint16_t analog_inputs[MAX_CONTROLLERS][5];
extern int32_t player_pad_map[4 + 10];  /* + the idle-chatter counters (input_sdl2.c) */
extern uint16_t merged_digital_inputs[MAX_CONTROLLERS];
extern uint16_t merged_analog_inputs[MAX_CONTROLLERS];
extern int32_t frames_since_last_input_change[MAX_CONTROLLERS];

/* Input system API */
void input_init(void);
void input_shutdown(void);
void process_inputs(void);
void threshold_analog_inputs(void);
void g_calc_frames_since_last_input_change(void);
void merge_inputs(void);

/* Platform-specific: called from platform layer when events arrive */
/* Pad hot-plugging: SDL device index of a new joystick / controller, SDL
 * instance id of a removed one (platform_sdl2.c event loop). */
void input_on_device_added(int device_index);
void input_on_device_removed(int instance_id);
void input_on_key_down(int keycode);
void input_on_key_up(int keycode);
void input_on_gamepad_button_down(int controller, int button);
void input_on_gamepad_button_up(int controller, int button);
void input_on_gamepad_axis(int controller, int axis, float value);

/* Gamepad mapping (SDL_GameControllerButton) */
typedef enum {
    INPUT_GAMEPAD_A = 0,
    INPUT_GAMEPAD_B,
    INPUT_GAMEPAD_X,
    INPUT_GAMEPAD_Y,
    INPUT_GAMEPAD_BACK,
    INPUT_GAMEPAD_GUIDE,
    INPUT_GAMEPAD_START,
    INPUT_GAMEPAD_LEFT_STICK,
    INPUT_GAMEPAD_RIGHT_STICK,
    INPUT_GAMEPAD_LEFT_SHOULDER,
    INPUT_GAMEPAD_RIGHT_SHOULDER,
    INPUT_GAMEPAD_DPAD_UP,
    INPUT_GAMEPAD_DPAD_DOWN,
    INPUT_GAMEPAD_DPAD_LEFT,
    INPUT_GAMEPAD_DPAD_RIGHT,
    INPUT_GAMEPAD_MAX
} InputGamepadButton;

typedef enum {
    INPUT_GAMEPAD_AXIS_LEFTX = 0,
    INPUT_GAMEPAD_AXIS_LEFTY,
    INPUT_GAMEPAD_AXIS_RIGHTX,
    INPUT_GAMEPAD_AXIS_RIGHTY,
    INPUT_GAMEPAD_AXIS_TRIGGERLEFT,
    INPUT_GAMEPAD_AXIS_TRIGGERRIGHT,
    INPUT_GAMEPAD_AXIS_MAX
} InputGamepadAxis;

/* Keyboard to gamepad mapping for fallback */
void input_set_keyboard_mapping(int player, const int* key_map);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_H */
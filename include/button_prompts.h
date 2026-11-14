/* PC port: controller-specific on-screen button prompts.
 *
 * The game's prompts name GameCube buttons, either as pad-button icon glyphs
 * of the asc_24x24 ARAM font (SJIS 0x839F.. "p/BUTTON_A/", used by the menu
 * help bars, name entry, ...) or as words in text ("A Button",
 * "START/PAUSE").  With a PlayStation, Xbox or Switch controller in use, the
 * icon glyphs are replaced by that controller's button icons (PNGs under
 * assets/prompts/<style>/, code-drawn icons if a file is missing) and the
 * English button words are rewritten.
 *
 * The port maps SDL buttons by position (south = GC A, east = GC B, west =
 * GC X, north = GC Y; input_sdl2.c turns SDL's Nintendo label layout off), so
 * e.g. GC A is Cross on a PlayStation pad, A on an Xbox pad and B on a Switch
 * pad.  Triggers: LT/RT (L2/R2, ZL/ZR) = GC L/R, RB (R1, R) = GC Z.
 *
 * Style: --button-style / SMB2_BUTTON_STYLE (auto|gc|ps|ps4|ps5|xbox|switch),
 * else the "Buttons" PC setting.  Auto follows the controller that was used
 * last (input_controller_type) and falls back to the GameCube look. */
#ifndef BUTTON_PROMPTS_H
#define BUTTON_PROMPTS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Resolved styles */
enum { PCB_STYLE_GC = 0, PCB_STYLE_PS4, PCB_STYLE_PS5, PCB_STYLE_XBOX, PCB_STYLE_SWITCH,
       PCB_STYLE_COUNT };
/* g_pcs.button_style values.  PS picks the PS4 icons when the controller in
 * use is a PS4 pad, else the PS5 ones; PS4 / PS5 are command-line only. */
enum { PCB_SETTING_AUTO = 0, PCB_SETTING_GC = 1, PCB_SETTING_PS = 2, PCB_SETTING_XBOX = 3,
       PCB_SETTING_SWITCH = 4, PCB_SETTING_MENU_COUNT = 5,   /* cycled in the menu */
       PCB_SETTING_PS4 = 5, PCB_SETTING_PS5 = 6 };

/* Parse "auto"/"gc"/"ps"/"ps4"/"ps5"/"xbox"/"switch" (and a few aliases);
 * -1 if unknown. */
int pc_button_style_parse(const char *s);
/* The style in effect (PCB_STYLE_*), resolved once per frame. */
int pc_button_style(void);
/* Per-frame: re-resolve the style; refresh the font icons when it changed. */
void pc_button_prompts_update(void);
/* asc_24x24 page loader hook: if `code` is a pad-button glyph that the
 * current style replaces, overwrite the 24x24 CMPR page at `dest`. */
int pc_button_prompts_glyph(unsigned code, void *dest);
/* Texture hook (textdraw.c): if the current style has a high-resolution icon
 * for `code`, return 1 with a GX RGBA8 image of *size x *size texels. */
int pc_button_prompts_hires(unsigned code, void **image, int *size);
/* Text hook: returns `str`, or a static buffer with the button words of the
 * current style. */
char *pc_button_prompts_text(char *str);

/* SDL_GameControllerType of the controller in use (the one that had input
 * last, else player 1's, else any open one); -1 for none, the keyboard or a
 * joystick without a controller mapping (input_sdl2.c). */
int input_controller_type(void);

#ifdef __cplusplus
}
#endif
#endif

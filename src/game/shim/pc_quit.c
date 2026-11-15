/* PC port: "Quit to Desktop".  The GC game has no quit, and a controller
 * player in fullscreen needs one.  Offered in the pause menus (decomp
 * pause_menu.c / hud.c, baseball_33.c), the PC settings rows
 * (pc_settings_menu.c) and on the title / Mode Select screens.
 *
 *  - Confirmation: the game's own yes/no box (hud.c sprite_save_disp),
 *    default No.
 *  - Quitting: pc_quit_request() only sets a flag.  pc_quit_poll() keeps the
 *    game loop running until no memory card job can still touch the card (a
 *    job only waiting for a button press on its result box counts as done),
 *    then saves settings.dat and exits(0).  SDL_QUIT takes the same path.
 *  - Strings: one per game locale (0 en, 1 de, 2 fr, 3 es, 4 it, 5 ja), drawn
 *    with the menu font (font 3). */
#include "gc_intrinsics.h"

struct StickPoint;
struct MemcardRequest;
struct TextBox;
#include "sel_ngc.h"

#include "pc_settings.h"
#include "pc_quit.h"
#include "pc_text.h"
#include "pc_update.h"
#include "pc_memcard_ui.h"

extern u32 fn_1_C2478(void);                /* hud.c: yes/no cursor flash */
extern int pc_memcard_io_busy(void);        /* memcard.c (PC-only) */
extern int pc_memcard_job_state(int ch);    /* memcard.c (PC-only) */
extern void exit(int);
extern u16 merged_digital_inputs[];         /* input.c: newly pressed, all pads */
extern u16 merged_analog_inputs[];          /* input.c: stick as +Control Pad bits */
extern int pc_input_esc_pressed(void);      /* input_sdl2.c: Esc went down */

/* ---- strings ---- (UTF-8, converted by pc_text; ja as in sel_ngc_option.c
 * lbl_3_data_FF38 / FF5C) */
static const char *const s_label[6] = {
    "Quit to Desktop", "Zum Desktop", "Vers le bureau", "Ir al escritorio",
    "Al desktop", "デスクトップへ",
};
static const char *const s_title[6] = {
    "Quit to Desktop?", "Zum Desktop?", "Vers le bureau ?", "¿Ir al escritorio?",
    "Al desktop?", "デスクトップへ？",
};
static const char *const s_yes[6] = { "Yes", "Ja", "Oui", "Sí", "Sì", "はい" };
static const char *const s_no[6] = { "No", "Nein", "Non", "No", "No", "いいえ" };

/* short form for the "<button>:<label>" prompts of the top-level menus
 * (fits the 48 px label width of sel_ngc's prompt rows) */
static const char *const s_short[6] = {
    "Quit", "Beenden", "Quitter", "Salir", "Esci", "おわる",
};

const char *pc_quit_label(void) { return pc_text_pick(s_label); }
const char *pc_quit_short_label(void) { return pc_text_pick(s_short); }


/* Pause menu Quit row index, set by the pause menu constructors
 * (pause_menu.c, baseball_33.c); the rows before it are the game's. */
s32 pc_pause_quit_row = -1;

/* ======================================================================
 * Request / exit
 * ====================================================================== */
static int s_requested;
static const char *s_reason;
static int s_wait_frames;

void pc_quit_request(const char *reason)
{
    if (s_requested)
        return;
    s_requested = 1;
    s_reason = reason;
    s_wait_frames = 0;
    printf("[quit] requested (%s)\n", reason);
    fflush(stdout);
}

int pc_quit_requested(void) { return s_requested; }

/* A memory card job is running and not just waiting for a button press on
 * its result/error box (memcard.c pc_memcard_io_busy). */
static int memcard_busy(void)
{
    return pc_memcard_io_busy();
}

/* Once per frame from video_finish_frame, with the game loop still running
 * so a memory card job can finish. */
void pc_quit_poll(void)
{
    if (!s_requested)
        return;
    if (memcard_busy()) {
        /* card_shim ops complete synchronously; the card I/O part of a
         * memcard.c job takes a few frames.  The cap only guards against a
         * job stuck in a non-input step. */
        if (s_wait_frames % 30 == 0) {
            printf("[quit] waiting for the memory card job to finish (state %x %x %x)\n",
                   pc_memcard_job_state(0), pc_memcard_job_state(1), pc_memcard_job_state(2));
            fflush(stdout);
        }
        if (++s_wait_frames < 60 * 30)
            return;
        printf("[quit] memory card still busy after 30 s - exiting anyway\n");
    } else if (s_wait_frames > 0) {
        printf("[quit] memory card idle after %d frames (state %x %x %x)\n", s_wait_frames,
               pc_memcard_job_state(0), pc_memcard_job_state(1), pc_memcard_job_state(2));
    }
    pcs_save();
    if (s_reason != NULL && strcmp(s_reason, "window closed") == 0)
        printf("\nWindow closed - exiting.\n");
    else
        printf("\nQuit to Desktop - exiting.\n");
    fflush(NULL);
    exit(0);
}

/* ======================================================================
 * Confirmation box (game yes/no style, hud.c sprite_save_disp)
 * ====================================================================== */
static int s_confirm_open;
static int s_confirm_sel;  /* 0 Yes, 1 No */

void pc_quit_confirm_open(void)
{
    s_confirm_open = 1;
    s_confirm_sel = 1; /* default No */
}

void pc_quit_confirm_close(void) { s_confirm_open = 0; }
int pc_quit_confirm_active(void) { return s_confirm_open; }

/* press: newly pressed PAD_BUTTON_* bits (0x1000 = START).
 * Returns PCQ_* (pc_quit.h); on PCQ_YES the quit is requested. */
int pc_quit_confirm_tick(unsigned int press)
{
    if (!s_confirm_open)
        return PCQ_NONE;
    if (press & PAD_BUTTON_A) {
        s_confirm_open = 0;
        if (s_confirm_sel == 0) {
            pc_quit_request("menu");
            return PCQ_YES;
        }
        return PCQ_NO;
    }
    if (press & (PAD_BUTTON_B | 0x1000)) {
        s_confirm_open = 0;
        return PCQ_NO;
    }
    if (press & (PAD_BUTTON_LEFT | PAD_BUTTON_RIGHT | PAD_BUTTON_UP | PAD_BUTTON_DOWN)) {
        s_confirm_sel ^= 1;
        return PCQ_MOVED;
    }
    return PCQ_NONE;
}

/* Box centred on (x, y) in 640x480 UI space; boxZ behind textZ (smaller is
 * nearer).  Same pieces as sprite_save_disp: bmp 0x44 box 320x112, title in
 * 0xFFEF9A, Yes/No 64px either side with the "\x81m \x81n" bracket cursor. */
void pc_quit_confirm_draw(float x, float y, float boxZ, float textZ)
{
    u32 color;
    int i;

    if (!s_confirm_open)
        return;
    set_ui_widescreen_scale_mtx(0x140);
    init_ui_element_sprite_with_defaults();
    set_ui_element_sprite_pos(x, y);
    set_ui_element_sprite_depth(boxZ);
    if (g_get_bmp_width(0x44) > 0 && g_get_bmp_height(0x44) > 0)
        set_ui_element_sprite_scale(320.0f / (float)g_get_bmp_width(0x44),
                                    112.0f / (float)g_get_bmp_height(0x44));
    draw_ui_box(0x44);

    textdraw_reset();
    textdraw_set_font(3);
    textdraw_set_drop_shadow();
    textdraw_set_alignment(4);
    textdraw_set_flags(0x80000000U);
    textdraw_set_depth(textZ);
    textdraw_set_mul_color(0xFFEF9A00U);
    textdraw_set_pos(x, y - 24.0f);
    textdraw_print((char *)pc_text_pick(s_title));
    color = fn_1_C2478();
    textdraw_set_border();
    for (i = 0; i < 2; i++) {
        float tx = i == 0 ? x - 64.0f : x + 64.0f;
        if (s_confirm_sel == i) {
            textdraw_set_mul_color(0xFFFFFF00U);
            textdraw_set_add_color(color);
            textdraw_set_pos(tx, 24.0f + y);
            textdraw_print("\x81m      \x81n");
        } else {
            textdraw_set_mul_color(0xFF7F7F00U);
            textdraw_set_add_color(0);
        }
        textdraw_set_pos(tx, 24.0f + y);
        textdraw_print((char *)pc_text_pick(i == 0 ? s_yes : s_no));
    }
    textdraw_reset();
    reset_ui_widescreen_scale_mtx();
}

/* ======================================================================
 * Top-level menus: title screen ("PRESS START", adv.c smd_adv_title_tick)
 * and Mode Select (sel_ngc.c menu_tick).  The title opens the box with B
 * (it has no "back" of its own), Mode Select with X (B keeps going back to
 * the title); Esc opens it on both.  The decomp callers say when the screen
 * is idle (can_open) and play their own menu sounds.
 * ====================================================================== */
static int s_menu_where;   /* PCMQ_TITLE / PCMQ_MODE_SELECT owning the box, 0 none */

/* Returns -1 when the box is not open (the menu handles its input as
 * usual), else what happened (PCQ_* or PCQ_OPENED); the menu then skips its
 * own input this frame.  Also TRUE-ish (PCQ_NONE) while a quit requested
 * from this menu is pending, so the menu stays frozen until the exit. */
int pc_menu_quit_tick(int where, int can_open, unsigned int open_buttons)
{
    unsigned int press;
    int esc = pc_input_esc_pressed();

    if (s_menu_where == where && pc_quit_requested())
        return PCQ_NONE;
    /* updater: a safe moment for the release notes box of the automatic
     * check (idle title / Mode Select, no Quit box, no memory card job);
     * it runs modally at the end of this frame (pc_update_ui.c) */
    if (can_open && s_menu_where != where && !pc_quit_requested() && !pc_quit_confirm_active() &&
        !memcard_busy() && pc_update_should_prompt()) {
        printf("[update] safe moment on the %s: opening the release notes box\n",
               where == PCMQ_TITLE ? "title screen" : "Mode Select");
        fflush(stdout);
        pc_update_notes_request();
        return -1;
    }
    /* saves imported from memcard/import/ at startup: say so, same moment */
    if (can_open && s_menu_where != where && !pc_quit_requested() && !pc_quit_confirm_active() &&
        !memcard_busy() && pc_memcard_notice_pending()) {
        pc_memcard_notice_request();
        return -1;
    }
    if (s_menu_where == where && !pc_quit_confirm_active())
        s_menu_where = 0;   /* closed elsewhere */
    if (s_menu_where != where) {
        if (!can_open || pc_quit_requested() || pc_quit_confirm_active())
            return -1;
        if (!(merged_digital_inputs[2] & open_buttons) && !esc)
            return -1;
        pc_quit_confirm_open();
        s_menu_where = where;
        printf("[quit] Quit to Desktop box opened on the %s\n",
               where == PCMQ_TITLE ? "title screen" : "Mode Select");
        fflush(stdout);
        return PCQ_OPENED;
    }
    if (!can_open) {
        /* the screen changed under the box (should not happen: the menu
         * is frozen while it is open) */
        pc_quit_confirm_close();
        s_menu_where = 0;
        return -1;
    }
    press = merged_digital_inputs[2] & (PAD_BUTTON_A | PAD_BUTTON_B | 0x1000 |
                                        PAD_BUTTON_LEFT | PAD_BUTTON_RIGHT);
    press |= merged_analog_inputs[2] & (PAD_BUTTON_LEFT | PAD_BUTTON_RIGHT);
    if (esc)
        press |= PAD_BUTTON_B;
    switch (pc_quit_confirm_tick(press)) {
    case PCQ_MOVED: return PCQ_MOVED;
    case PCQ_NO:    s_menu_where = 0; return PCQ_NO;
    case PCQ_YES:   return PCQ_YES;   /* s_menu_where kept: frozen until exit */
    }
    return PCQ_NONE;
}

/* TRUE while the box of this menu is open or its quit is pending */
int pc_menu_quit_busy(int where)
{
    return s_menu_where == where && (pc_quit_confirm_active() || pc_quit_requested());
}

void pc_menu_quit_draw(int where)
{
    if (s_menu_where == where && pc_quit_confirm_active())
        pc_quit_confirm_draw(320.0f, 240.0f, 0.006f, 0.005f);
}

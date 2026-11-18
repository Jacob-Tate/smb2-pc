/* PC settings UI drawn with the game's own UI pieces (wood option box, cursor
 * bullet, menu font, menu sounds).
 *
 *  1. Options > Game Screen page (sel_ngc).  sel_ngc.c menu_option_screen_tick
 *     calls pc_screen_page_create/tick/destroy here instead of its
 *     fn_3_20618/fn_3_20698/fn_3_20A28 (#ifndef __MWERKS__ hook).  The page
 *     keeps the game's two rows (TV monitor = modeCtrl.wideMode, split screen)
 *     and adds the PC video/audio rows from pc_settings.c.  Same structure and
 *     controls as the original page: up/down pick a row, A to edit, left/right
 *     to change, A to confirm, B to cancel (B on the list leaves the page).
 *
 *  2. F1 overlay (pc_settings_overlay_run), opened from video_finish_frame on
 *     any screen: freezes the game on its last frame and shows the PC rows in
 *     a modal loop.
 *
 *  Both end with a "Quit to Desktop" row (pc_quit.c): A opens the game's
 *  yes/no box (default No); Yes requests the quit, which video_finish_frame
 *  carries out once no memory card job is running.
 *
 * Also provides pc_game_wide_mode() for the renderer's aspect box. */
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
#include "netplay/session.h"
#include "build_id.h"

/* ---- game symbols not (correctly) declared in the headers ---- */
struct ScreenOption {
    char **name;
    char **desc;
    u8 *value;
    s32 count;
    char ***options;
    void (*callback)(void);
};
extern struct ScreenOption lbl_3_data_F9B0[2];   /* sel_ngc_option.c */
s32 did_any_pad_press_input(u32 input);          /* sel_ngc.c */
void g_b_button_pressed_on_menu(s32 arg0);       /* sel_ngc.c */
void fn_3_140CC(int counter);                    /* sel_ngc_sprite.c */
void C_MTXOrtho(Mtx44 m, float t, float b, float l, float r, float n, float f);
extern s32 g_smth_with_sprite_draw_reqs_widescreen; /* bitmap.c */
#define menuWork ((struct SelNgcMenuWork *)&g_currently_visible_menu_screen)

/* platform / renderer */
extern void input_poll_gc_pads(void);
extern void platform_poll_events(void);
extern int  platform_should_close(void);
extern void platform_swap_buffers(void);
extern unsigned long long platform_get_time_ns(void);
extern void platform_wait_until_ns(unsigned long long target_ns);
extern int  pc_frame_capture(void);
extern void pc_frame_restore(float brightness);
extern void exit(int);
#ifdef _WIN32
extern volatile long g_shim_frame_pulse;
#endif

int pc_game_wide_mode(void) { return modeCtrl.wideMode; }
void pc_game_set_wide_mode(int wide) { modeCtrl.wideMode = wide ? 1 : 0; }

/* ======================================================================
 * Rows: the game's 2 rows, then the PC items, then Quit to Desktop.
 * ====================================================================== */
#define GAME_ROWS 2
#define QUIT_ROW (GAME_ROWS + PCS_ITEM_COUNT)
#define ROW_COUNT_FULL (QUIT_ROW + 1)
/* E1 D10: a session shows only the game's 2 rows (the PC rows depend on local
 * state and mutate local settings from replicated menu input) */
#define ROW_COUNT (pc_session_active() ? GAME_ROWS : ROW_COUNT_FULL)
#define ROW_SPACING 20.5f   /* F1 overlay: 14 rows */
/* the Options page has to fit 16 rows (2 game + 13 PC + Quit) above the
 * game's description box */
#define PAGE_ROW_SPACING 17.5f
#define PAGE_TEXT_SCALE_Y 0.8f

static const char *row_name(int r)
{
    if (r < GAME_ROWS) return lbl_3_data_F9B0[r].name[locale];
    if (r == QUIT_ROW) return pc_quit_label();
    return pcs_item_name(r - GAME_ROWS);
}

static const char *row_value(int r)
{
    if (r < GAME_ROWS) {
        struct ScreenOption *opt = &lbl_3_data_F9B0[r];
        return opt->options[*opt->value][locale];
    }
    if (r == QUIT_ROW) return "";
    return pcs_item_value(r - GAME_ROWS);
}

static const char *row_desc(int r)
{
    if (r < GAME_ROWS) return lbl_3_data_F9B0[r].desc[locale];
    if (r == QUIT_ROW) {
        static const char *const quit_desc[6] = {
            "Close the game and return to the desktop.",
            "Spiel beenden und zum Desktop zurückkehren.",
            "Quitter le jeu et revenir au bureau.",
            "Cerrar el juego y volver al escritorio.",
            "Chiudi il gioco e torna al desktop.",
            "ゲームを おわって デスクトップに もどります。",
        };
        return pc_text_pick(quit_desc);
    }
    return pcs_item_desc(r - GAME_ROWS);
}

static int row_enabled(int r)
{
    if (r < GAME_ROWS || r == QUIT_ROW) return 1;
    return pcs_item_enabled(r - GAME_ROWS);
}

/* ======================================================================
 * Shared list drawing (game's option-page style, fn_3_201C8)
 * ====================================================================== */
struct ListView {
    int first, count;    /* rows to show */
    int cursor, editing, timer;
    float x, y;          /* first row's name position */
    float valueX;        /* centre of the value column */
    int boxBmp;          /* ui box bitmap */
    float boxCX, boxCY, boxW, boxH;
    int bullet;          /* draw the 0x1700 cursor bullet (else a ">" glyph) */
    const char *title;   /* optional, above the list */
    const char *hint;    /* optional, below the description */
    const char *version; /* optional, small, top right (F1 overlay) */
    float descY, hintY;
    float spacing;       /* row pitch */
    float textScaleY;    /* row text height scale */
};

static u32 glow_color(int timer)
{
    u32 g = (u32)(64.0 * fabs(math_sin((s16)(timer * 0x180))));
    return g | (g << 8) | (g << 16);
}

static void draw_list(const struct ListView *v)
{
    int i;
    float bw, bh;

    GXSetZMode_cached(1U, 7, 1U);
    set_ui_widescreen_scale_mtx(0x140);

    init_ui_element_sprite_with_defaults();
    set_ui_element_sprite_depth(0.11f);
    set_ui_element_sprite_pos(v->boxCX, v->boxCY);
    bw = (float)g_get_bmp_width(v->boxBmp);
    bh = (float)g_get_bmp_height(v->boxBmp);
    if (bw > 0.0f && bh > 0.0f)
        set_ui_element_sprite_scale(v->boxW / bw, v->boxH / bh);
    draw_ui_box(v->boxBmp);

    if (v->title != NULL) {
        textdraw_reset();
        textdraw_set_font(3);
        textdraw_set_flags(0xA0000000U);
        textdraw_set_alignment(3);
        textdraw_set_mul_color(0xFFFF40U);
        textdraw_set_scale(1.0f, 1.1f);
        textdraw_set_pos(320.0f, v->y - 40.0f);
        textdraw_print((char *)v->title);
    }
    if (v->version != NULL) {
        textdraw_reset();
        textdraw_set_font(3);
        textdraw_set_flags(0xA0000000U);
        textdraw_set_alignment(3);
        textdraw_set_mul_color(0xA0A0A0U);
        textdraw_set_scale(0.55f, 0.6f);
        textdraw_set_pos(v->boxCX + v->boxW * 0.5f - 50.0f, v->y - 34.0f);
        textdraw_print((char *)v->version);
    }

    /* the Quit to Desktop yes/no box replaces the rows while it is open.
     * Drawn before the hint text: as the overlay's very last draw, its final
     * glyph gets the wrong texture. */
    pc_quit_confirm_draw(320.0f, 240.0f, 0.05f, 0.04f);
    set_ui_widescreen_scale_mtx(0x140);
    for (i = 0; i < (pc_quit_confirm_active() ? 0 : v->count); i++) {
        int r = v->first + i;
        float y = v->y + v->spacing * (float)i;
        int sel = (r == v->cursor);
        int en = row_enabled(r);

        /* underline (scaled "_") */
        textdraw_reset();
        textdraw_set_font(3);
        textdraw_set_flags(0x80000000U);
        textdraw_set_alignment(3);
        if (sel) {
            textdraw_set_mul_color(0xFFFF40U);
            textdraw_set_add_color(glow_color(v->timer));
        } else {
            textdraw_set_mul_color(0xC0C040U);
            textdraw_set_add_color(0U);
        }
        textdraw_set_scale(15.0f, 1.0f);
        textdraw_set_pos(320.0f, y);
        textdraw_print("_");

        if (sel) {
            if (v->bullet && g_get_bmp_height(0x1700) > 0) {
                init_ui_element_sprite_with_defaults();
                set_ui_element_sprite_pos(v->x - 26.0f, 12.0f + y);
                set_ui_element_sprite_scale(26.0 / g_get_bmp_width(0x1700),
                                            26.0 / g_get_bmp_height(0x1700));
                draw_ui_element(0x1700);
            } else {
                textdraw_reset();
                textdraw_set_font(3);
                textdraw_set_flags(0xA0000000U);
                textdraw_set_mul_color(0xFFFF40U);
                textdraw_set_add_color(glow_color(v->timer));
                textdraw_set_scale(0.8f, 1.0f);
                textdraw_set_pos(v->x - 30.0f, y);
                textdraw_print(">");
            }
        }

        textdraw_reset();
        textdraw_set_font(3);
        textdraw_set_scale(0.8f, v->textScaleY);
        textdraw_set_flags(0xA0000000U);
        if (!en)
            textdraw_set_mul_color(0x808080U);
        textdraw_set_pos(v->x, y);
        textdraw_print((char *)row_name(r));
        textdraw_set_alignment(3);
        textdraw_set_pos(v->valueX, y);
        if (v->editing && sel)
            textdraw_printf("<%s>", row_value(r));
        else
            textdraw_print((char *)row_value(r));
    }

    if (!pc_quit_confirm_active()) {
        textdraw_reset();
        fn_3_140CC(v->timer);
        textdraw_set_pos(320.0f, v->descY);
        textdraw_print_and_fit_to_width(390.0f, (char *)row_desc(v->cursor));
    }

    if (v->hint != NULL) {
        textdraw_reset();
        textdraw_set_font(3);
        textdraw_set_flags(0xA0000000U);
        textdraw_set_alignment(3);
        textdraw_set_mul_color(0xC0C0C0U);
        textdraw_set_scale(0.6f, 0.7f);
        textdraw_set_pos(320.0f, v->hintY);
        textdraw_print((char *)v->hint);
    }

    reset_ui_widescreen_scale_mtx();
    GXSetZMode_cached(1U, 3, 1U);
}

/* ======================================================================
 * 1. Options > Game Screen page
 * ====================================================================== */
static struct {
    s32 state;      /* 0 choosing a row, 1 editing */
    s32 cursor;
    s32 savedValue; /* game rows: value restored on B */
    s32 timer;
} s_page;

static void page_draw(struct Sprite *sprite)
{
    struct ListView v;

    memset(&v, 0, sizeof(v));
    v.first = 0;
    v.count = ROW_COUNT;
    v.cursor = s_page.cursor;
    v.editing = (s_page.state == 1);
    v.timer = s_page.timer;
    v.x = sprite->x;
    v.y = sprite->y;
    v.valueX = sprite->x + 250.0f;
    v.spacing = PAGE_ROW_SPACING;
    v.textScaleY = PAGE_TEXT_SCALE_Y;
    v.boxBmp = 0x1701;
    v.boxCX = 320.0f;
    v.boxW = 400.0f;
    v.boxH = v.spacing * (ROW_COUNT - 1) + 24.0f + 24.0f;
    v.boxCY = sprite->y + (v.spacing * (ROW_COUNT - 1) + 24.0f) * 0.5f;
    v.bullet = 1;
    v.descY = 400.0f;
    draw_list(&v);
}

void pc_screen_page_create(void)
{
    struct Sprite *sprite;

    memset(&s_page, 0, sizeof(s_page));
    pc_quit_confirm_close();
    sprite = create_sprite();
    if (sprite != NULL) {
        sprite->x = 170.0f;
        sprite->y = 84.0f;   /* 16 rows at 17.5 px end above the description box */
        sprite->tag = 0x5D;
        sprite->drawFunc = page_draw;
        strcpy(sprite->text, "screen");
    }
}

void pc_screen_page_destroy(void)
{
    pc_quit_confirm_close();
    destroy_sprite_with_unique_id(0x5D);
}

void pc_screen_page_tick(void)
{
    u32 sound = 0;
    s32 sel;
    int idle = (menuWork->unkC == 0 && menuWork->unk18 == 0);

    if (s_page.state == 2) {
        /* Quit to Desktop yes/no box */
        u32 press = 0;
        if (did_any_pad_press_input(0) != 0) press |= PAD_BUTTON_LEFT;
        if (did_any_pad_press_input(1) != 0) press |= PAD_BUTTON_RIGHT;
        if (idle && did_any_pad_press_input(6) != 0) press |= PAD_BUTTON_A;
        if (idle && did_any_pad_press_input(7) != 0) press |= PAD_BUTTON_B;
        switch (pc_quit_confirm_tick(press)) {
        case PCQ_MOVED: sound |= 2; break;
        case PCQ_NO: sound |= 0x10; s_page.state = 0; break;
        case PCQ_YES: sound |= 8; s_page.state = 3; break;
        }
    } else if (s_page.state == 3) {
        /* quitting: ignore input until pc_quit_poll exits */
    } else if (s_page.state == 0) {
        sel = s_page.cursor;
        if (did_any_pad_press_input(2) != 0) {
            if (--sel < 0) sel = ROW_COUNT - 1;
        } else if (did_any_pad_press_input(3) != 0) {
            if (++sel >= ROW_COUNT) sel = 0;
        }
        if (sel != s_page.cursor) {
            sound |= 2;
            s_page.cursor = sel;
        }
        if (did_any_pad_press_input(6) != 0 && idle) {
            if (s_page.cursor == QUIT_ROW) {
                sound |= 8;
                pc_quit_confirm_open();
                s_page.state = 2;
            } else if (s_page.cursor == GAME_ROWS + PCS_ITEM_CONTROLLERS) {
                /* Controller Sticks screen, modal at the end of this frame */
                sound |= 8;
                pc_stick_screen_request();
            } else if (s_page.cursor == GAME_ROWS + PCS_ITEM_UPDATES) {
                /* Updates screen (pc_update_ui.c), modal at the end of this frame */
                sound |= 8;
                pc_update_screen_request();
            } else if (s_page.cursor == GAME_ROWS + PCS_ITEM_MEMCARD) {
                /* Memory Card screen (pc_memcard_ui.c), modal at the end of this frame */
                sound |= 8;
                pc_memcard_screen_request();
            } else if (row_enabled(s_page.cursor)) {
                sound |= 8;
                if (s_page.cursor < GAME_ROWS)
                    s_page.savedValue = *lbl_3_data_F9B0[s_page.cursor].value;
                else
                    pcs_item_begin_edit(s_page.cursor - GAME_ROWS);
                s_page.state = 1;
            } else {
                sound |= 0x10;
            }
        } else if (did_any_pad_press_input(7) != 0 && idle) {
            g_b_button_pressed_on_menu(1);
        }
    } else if (s_page.cursor < GAME_ROWS) {
        struct ScreenOption *opt = &lbl_3_data_F9B0[s_page.cursor];
        sel = *opt->value;
        if (did_any_pad_press_input(0) != 0) {
            if (--sel < 0) sel = opt->count - 1;
        } else if (did_any_pad_press_input(1) != 0) {
            if (++sel >= opt->count) sel = 0;
        }
        if (sel != *opt->value) {
            sound |= 2;
            *opt->value = sel;
            if (opt->callback != NULL) opt->callback();
        }
        if (did_any_pad_press_input(6) != 0 && idle) {
            sound |= 8;
            s_page.state = 0;
        } else if (did_any_pad_press_input(7) != 0 && idle) {
            sound |= 0x10;
            *opt->value = s_page.savedValue;
            if (opt->callback != NULL) opt->callback();
            s_page.state = 0;
        }
    } else {
        int item = s_page.cursor - GAME_ROWS;
        if (did_any_pad_press_input(0) != 0) {
            if (pcs_item_step(item, -1)) sound |= 2;
        } else if (did_any_pad_press_input(1) != 0) {
            if (pcs_item_step(item, 1)) sound |= 2;
        }
        if (did_any_pad_press_input(6) != 0 && idle) {
            sound |= 8;
            pcs_item_confirm(item);
            s_page.state = 0;
        } else if (did_any_pad_press_input(7) != 0 && idle) {
            sound |= 0x10;
            pcs_item_cancel(item);
            s_page.state = 0;
        }
    }

    if (sound & 2)
        call_SoundReqID_arg_0(0x6C);
    else if (sound & 8)
        call_SoundReqID_arg_0(0x65);
    else if (sound & 0x10)
        call_SoundReqID_arg_0(0x66);
    s_page.timer += 1;
}

/* ======================================================================
 * 2. F1 overlay (modal, any screen)
 * ====================================================================== */
static u16 pad_held(void)
{
    u16 b = 0;
    int i;

    for (i = 0; i < 4; i++) {
        PADStatus *s = &pad_status_groups[i].unk0[0];
        if (s->err != 0 && i != 0)
            continue;
        b |= s->button;
        if (s->stickX < -50) b |= PAD_BUTTON_LEFT;
        if (s->stickX > 50) b |= PAD_BUTTON_RIGHT;
        if (s->stickY > 50) b |= PAD_BUTTON_UP;
        if (s->stickY < -50) b |= PAD_BUTTON_DOWN;
    }
    return b;
}

#define OVERLAY_ROWS (PCS_ITEM_COUNT + 1)   /* PC items + Quit to Desktop */

static void overlay_draw(int cursor, int editing, int timer)
{
    struct ListView v;
    Mtx44 m;

    g_some_sprite_reset_gx_state_func();
    mtxa_from_identity();
    GXLoadPosMtxImm(mtxa->mtxA, 0);
    C_MTXOrtho(m, 0.0f, 480.0f, 0.0f, 640.0f, 0.0f, 20000.0f);
    g_something_with_GXSetProjectionv(m, 1);
    g_smth_with_sprite_draw_reqs_widescreen = 1;

    memset(&v, 0, sizeof(v));
    v.first = GAME_ROWS;
    v.count = OVERLAY_ROWS;
    v.cursor = GAME_ROWS + cursor;
    v.spacing = ROW_SPACING;
    v.textScaleY = 0.95f;
    v.editing = editing;
    v.timer = timer;
    v.x = 170.0f;
    v.y = 100.0f;
    v.valueX = 420.0f;
    v.boxBmp = 5;
    v.boxCX = 320.0f;
    v.boxW = 420.0f;
    v.descY = v.y + ROW_SPACING * OVERLAY_ROWS + 16.0f;
    v.hintY = v.descY + 36.0f;
    v.boxH = v.hintY + 34.0f - (v.y - 50.0f);   /* title (y-40) .. hint */
    v.boxCY = (v.y - 50.0f) + v.boxH * 0.5f;
    v.bullet = 0;
    {
        static const char *const title[6] = {
            "PC Settings", "PC-Einstellungen", "Réglages PC", "Ajustes de PC",
            "Impostazioni PC", "PCせってい" };
        static const char *const hint_confirm[6] = {
            "Left/Right: select   A: confirm   B: cancel",
            "Links/Rechts: wählen   A: OK   B: Abbrechen",
            "Gauche/Droite : choisir   A : valider   B : annuler",
            "Izq./Der.: elegir   A: aceptar   B: cancelar",
            "Sinistra/Destra: scegli   A: conferma   B: annulla",
            "ひだり/みぎ:えらぶ   A:けってい   B:キャンセル" };
        static const char *const hint_edit[6] = {
            "Left/Right: change   A: apply   B: cancel",
            "Links/Rechts: ändern   A: übernehmen   B: Abbrechen",
            "Gauche/Droite : changer   A : appliquer   B : annuler",
            "Izq./Der.: cambiar   A: aplicar   B: cancelar",
            "Sinistra/Destra: cambia   A: applica   B: annulla",
            "ひだり/みぎ:かえる   A:けってい   B:キャンセル" };
        static const char *const hint_list[6] = {
            "Up/Down: select   A: edit   B / F1: close",
            "Hoch/Runter: wählen   A: ändern   B / F1: schließen",
            "Haut/Bas : choisir   A : modifier   B / F1 : fermer",
            "Arriba/Abajo: elegir   A: editar   B / F1: cerrar",
            "Su/Giù: scegli   A: modifica   B / F1: chiudi",
            "うえ/した:えらぶ   A:へんこう   B / F1:とじる" };
        v.title = pc_text_pick(title);
        {
            static char ver[96];
            snprintf(ver, sizeof ver, "v%s  build %s", pc_update_current_version(), smb2_build_id());
            v.version = ver;
        }
        v.hint = pc_text_pick(pc_quit_confirm_active() ? hint_confirm
                              : editing ? hint_edit : hint_list);
    }
    draw_list(&v);
}

void pc_settings_overlay_run(void)
{
    int cursor = 0, editing = 0, timer = 0, rep = 0;
    u16 prev, held, press;
    unsigned long long next_ns;

    int outer_confirm;

    if (!pc_frame_capture())
        return;
    /* a yes/no box the game screen had open (pause menu / Options page)
     * comes back when the overlay closes */
    outer_confirm = pc_quit_confirm_active();
    pc_quit_confirm_close();
    printf("[settings] overlay open\n");
    fflush(stdout);

    prev = pad_held(); /* ignore buttons already down */
    next_ns = platform_get_time_ns();
    for (;;) {
        int close = 0;

        pc_frame_restore(0.45f); /* dimmed game frame behind the box */
        overlay_draw(cursor, editing, timer);
        platform_swap_buffers();
#ifdef _WIN32
        g_shim_frame_pulse++;
#endif
        input_poll_gc_pads();
        platform_poll_events();
        if (pc_settings_overlay_take_request())
            close = 1; /* F1 again */
        if (platform_should_close())
            close = 1; /* window closed: video_finish_frame quits */

        held = pad_held();
        press = held & ~prev;
        /* auto-repeat for the d-pad / stick */
        if (held & (PAD_BUTTON_UP | PAD_BUTTON_DOWN | PAD_BUTTON_LEFT | PAD_BUTTON_RIGHT)) {
            if (++rep > 20 && (rep % 6) == 0)
                press |= held & (PAD_BUTTON_UP | PAD_BUTTON_DOWN | PAD_BUTTON_LEFT | PAD_BUTTON_RIGHT);
        } else {
            rep = 0;
        }
        prev = held;

        if (pc_quit_confirm_active()) {
            switch (pc_quit_confirm_tick(press & (PAD_BUTTON_LEFT | PAD_BUTTON_RIGHT |
                                                  PAD_BUTTON_A | PAD_BUTTON_B | 0x1000))) {
            case PCQ_MOVED: call_SoundReqID_arg_0(0x6C); break;
            case PCQ_NO: call_SoundReqID_arg_0(0x66); break;
            case PCQ_YES:
                /* back to the game, which quits from video_finish_frame */
                call_SoundReqID_arg_0(0x65);
                close = 1;
                break;
            }
            if (close)
                pc_quit_confirm_close();
        } else if (!editing) {
            if (press & PAD_BUTTON_UP) {
                cursor = (cursor + OVERLAY_ROWS - 1) % OVERLAY_ROWS;
                call_SoundReqID_arg_0(0x6C);
            } else if (press & PAD_BUTTON_DOWN) {
                cursor = (cursor + 1) % OVERLAY_ROWS;
                call_SoundReqID_arg_0(0x6C);
            }
            if (press & PAD_BUTTON_A) {
                if (cursor == PCS_ITEM_COUNT) {
                    pc_quit_confirm_open();
                    call_SoundReqID_arg_0(0x65);
                } else if (cursor == PCS_ITEM_CONTROLLERS) {
                    call_SoundReqID_arg_0(0x65);
                    pc_stick_screen_run(1);
                    prev = pad_held();   /* buttons still down from that screen */
                } else if (cursor == PCS_ITEM_UPDATES) {
                    call_SoundReqID_arg_0(0x65);
                    pc_update_screen_run(1);
                    prev = pad_held();
                    if (pc_quit_requested())
                        close = 1;   /* Setup started: back to the game, which quits */
                } else if (cursor == PCS_ITEM_MEMCARD) {
                    call_SoundReqID_arg_0(0x65);
                    pc_memcard_screen_run(1);
                    prev = pad_held();
                    if (pc_quit_requested())
                        close = 1;   /* save imported: back to the game, which quits */
                } else if (pcs_item_enabled(cursor)) {
                    pcs_item_begin_edit(cursor);
                    editing = 1;
                    call_SoundReqID_arg_0(0x65);
                } else {
                    call_SoundReqID_arg_0(0x66);
                }
            } else if (press & (PAD_BUTTON_B | 0x1000 /* START */)) {
                close = 1;
            }
        } else {
            if (press & PAD_BUTTON_LEFT) {
                if (pcs_item_step(cursor, -1)) call_SoundReqID_arg_0(0x6C);
            } else if (press & PAD_BUTTON_RIGHT) {
                if (pcs_item_step(cursor, 1)) call_SoundReqID_arg_0(0x6C);
            }
            if (press & PAD_BUTTON_A) {
                pcs_item_confirm(cursor);
                editing = 0;
                call_SoundReqID_arg_0(0x65);
            } else if (press & PAD_BUTTON_B) {
                pcs_item_cancel(cursor);
                editing = 0;
                call_SoundReqID_arg_0(0x66);
            }
            if (close) {
                pcs_item_cancel(cursor);
                editing = 0;
            }
        }
        if (close)
            break;
        timer++;

        next_ns += 16666667ULL;
        {
            unsigned long long now = platform_get_time_ns();
            if (now < next_ns)
                platform_wait_until_ns(next_ns);
            else if (now > next_ns + 100000000ULL)
                next_ns = now;
        }
    }

    if (outer_confirm && !pc_quit_requested())
        pc_quit_confirm_open();

    /* put the frozen game frame back; the next gx_start_new_frame swap
     * presents it and the game carries on */
    pc_frame_restore(1.0f);
    printf("[settings] overlay closed\n");
    fflush(stdout);
}

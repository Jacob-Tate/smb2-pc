/* PC port "Quit to Desktop" (src/game/shim/pc_quit.c).  The decomp hooks
 * (pause_menu.c, hud.c, baseball_33.c) declare these themselves. */
#ifndef PC_QUIT_H
#define PC_QUIT_H

/* pc_quit_confirm_tick results */
#define PCQ_NONE  0
#define PCQ_MOVED 1   /* cursor moved: play the move sound */
#define PCQ_NO    2   /* closed with No / B / START */
#define PCQ_YES   3   /* confirmed: quit requested */
#define PCQ_OPENED 4  /* pc_menu_quit_tick: the box was just opened */

/* pc_menu_quit_* menus */
#define PCMQ_TITLE       1   /* title screen (adv.c) */
#define PCMQ_MODE_SELECT 2   /* Mode Select (sel_ngc.c) */

const char *pc_quit_label(void);
void pc_quit_request(const char *reason);
int  pc_quit_requested(void);
void pc_quit_poll(void);
void pc_quit_confirm_open(void);
void pc_quit_confirm_close(void);
int  pc_quit_confirm_active(void);
int  pc_quit_confirm_tick(unsigned int press);
void pc_quit_confirm_draw(float x, float y, float boxZ, float textZ);
const char *pc_quit_short_label(void);
int  pc_menu_quit_tick(int where, int can_open, unsigned int open_buttons);
int  pc_menu_quit_busy(int where);
void pc_menu_quit_draw(int where);

#endif

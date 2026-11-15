/* In-game Memory Card screen (src/game/shim/pc_memcard_ui.c): save import /
 * export from the PC settings. */
#ifndef PC_MEMCARD_UI_H
#define PC_MEMCARD_UI_H

/* run it modally (`captured`: the frame behind is already captured, as in
 * the F1 overlay), or request it from a game screen (opened at the end of
 * the frame) */
void pc_memcard_screen_run(int captured);
void pc_memcard_screen_request(void);
/* the drop-in import notice: requested at a safe moment on the title /
 * Mode Select (pc_quit.c) when memcard/import/ held files at startup */
int  pc_memcard_notice_pending(void);
void pc_memcard_notice_request(void);
/* video_finish_frame: runs a requested screen; 1 if it did */
int  pc_memcard_ui_run_requested(void);

#endif

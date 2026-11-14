#ifndef INSTALL_MODE_H
#define INSTALL_MODE_H
/* Installed vs portable mode (docs/installer.md).
 * install.ini next to the exe with mode=installed switches the port to the
 * installer layout; without it everything is exactly as in the portable zip. */
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Read <exe dir>/install.ini (once).  save_dir_arg = --save-dir or NULL. */
void pc_install_init(const char* save_dir_arg);
bool pc_installed(void);
const char* pc_install_gamedata(void);    /* expanded gamedata= or NULL */
const char* pc_install_userdata(void);    /* expanded userdata= + separator, or NULL */
const char* pc_install_version(void);     /* version= or "" */
/* Folder that holds memcard\ (+ trailing separator); NULL = portable
 * default (<exe dir>).  Installed: --save-dir if given, else userdata. */
const char* pc_userdata_base(void);
const char* pc_exe_dir(void);             /* with trailing separator ("" if unknown) */
/* %LOCALAPPDATA%\SMB2\gamedata (recovery extraction target).  Installed
 * mode moves the pre-1.0.4 "SMB2 PC Port" userdata / recovery folders to the
 * new names on start (install_mode.c, migrate_old_names). */
const char* pc_local_gamedata_dir(void);
/* Expand %APPDATA% / %LOCALAPPDATA% / %PROGRAMDATA% / other %VARS%. */
bool pc_expand_path(const char* in, char* out, size_t n);
/* Installed mode, interactive console run: stdout/stderr -> <userdata>\logs. */
void pc_install_setup_logs(void);
/* Print a line the user must see even when stdout goes to the log. */
void pc_user_msg(const char* fmt, ...);
/* stdin is an interactive console (prompts / message boxes make sense) */
bool pc_stdin_interactive(void);

#ifdef __cplusplus
}
#endif
#endif

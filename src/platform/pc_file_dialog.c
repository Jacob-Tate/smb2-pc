/* System Open / Save dialogs and "show this folder" for the in-game memory
 * card screen (src/game/shim/pc_memcard_ui.c).  Windows only; elsewhere the
 * dialogs report failure.
 *
 * SMB2_CARD_PICK_TEST="<path>|cancel|..." answers the dialogs in tests, one
 * entry per dialog (exhausted = cancel). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <objbase.h>
#include <shellapi.h>
#endif
#include "platform.h"

/* > 0 while a system dialog blocks the frame loop: the hang watchdog
 * (stubs_manual.c) waits */
volatile long g_shim_watchdog_hold;

static int pick_test(char *out, size_t outsz)
{
    static int s_call;
    const char *t = getenv("SMB2_CARD_PICK_TEST");
    int i;
    for (i = 0; i < s_call && t; i++) {
        t = strchr(t, '|');
        if (t) t++;
    }
    s_call++;
    if (t) {
        size_t n = strcspn(t, "|");
        if (n && n < outsz && !(n == 6 && !strncmp(t, "cancel", 6))) {
            memcpy(out, t, n);
            out[n] = 0;
            printf("[card-ui] dialog (SMB2_CARD_PICK_TEST): %s\n", out);
            return 1;
        }
    }
    printf("[card-ui] dialog (SMB2_CARD_PICK_TEST): cancel\n");
    return 0;
}

int pc_file_dialog_test_active(void)
{
    const char *t = getenv("SMB2_CARD_PICK_TEST");
    return t && t[0];
}

#ifdef _WIN32
static HWND game_hwnd(void)
{
    SDL_Window *win = (SDL_Window *)platform_sdl_window();
    SDL_SysWMinfo wm;
    if (!win)
        return NULL;
    SDL_VERSION(&wm.version);
    if (SDL_GetWindowWMInfo(win, &wm) && wm.subsystem == SDL_SYSWM_WINDOWS)
        return wm.info.win.window;
    return NULL;
}

/* UTF-16 path -> ANSI code page (what fopen() uses); the 8.3 name when the
 * long one doesn't fit the code page.  0 if neither does. */
static int wide_to_acp(const WCHAR *w, char *out, size_t outsz)
{
    WCHAR sh[4096];
    int pass;
    for (pass = 0; pass < 2; pass++) {
        BOOL lossy = FALSE;
        int utf8 = GetACP() == CP_UTF8;
        const WCHAR *src = w;
        if (pass == 1) {
            DWORD n = GetShortPathNameW(w, sh, (DWORD)(sizeof sh / sizeof sh[0]));
            if (!n || n >= sizeof sh / sizeof sh[0]) return 0;
            src = sh;
        }
        if (WideCharToMultiByte(CP_ACP, utf8 ? 0 : WC_NO_BEST_FIT_CHARS, src, -1, out, (int)outsz, NULL,
                                utf8 ? NULL : &lossy) > 0 && !lossy)
            return 1;
    }
    return 0;
}

/* An exclusive-fullscreen window would hide the dialog: minimised while it
 * is open.  The watchdog is held, the dialog blocks the frame loop. */
static int dialog_begin(void)
{
    SDL_Window *win = (SDL_Window *)platform_sdl_window();
    int minimized = 0;
    g_shim_watchdog_hold++;
    if (win && (SDL_GetWindowFlags(win) & SDL_WINDOW_FULLSCREEN_DESKTOP) == SDL_WINDOW_FULLSCREEN) {
        SDL_MinimizeWindow(win);
        minimized = 1;
    }
    return minimized;
}

static void dialog_end(int minimized)
{
    SDL_Window *win = (SDL_Window *)platform_sdl_window();
    if (minimized && win)
        SDL_RestoreWindow(win);
    if (win)
        SDL_RaiseWindow(win);
    g_shim_watchdog_hold--;
}

static void utf8_to_wide(const char *s, WCHAR *w, int n)
{
    if (!s || MultiByteToWideChar(CP_UTF8, 0, s, -1, w, n) <= 0)
        w[0] = 0;
}

/* save != 0: Save dialog with default_name; returns 1 picked, 0 cancelled,
 * -1 failed (or a path the game can't open, *bad_path set) */
static int file_dialog(int save, const char *title, const char *default_name, char *out, size_t outsz,
                       int *bad_path)
{
    WCHAR file[4096], wtitle[256];
    OPENFILENAMEW ofn;
    HRESULT co = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    int rc = 0, minimized;
    BOOL ok;

    *bad_path = 0;
    file[0] = 0;
    if (default_name)
        utf8_to_wide(default_name, file, 4096);
    utf8_to_wide(title, wtitle, 256);
    memset(&ofn, 0, sizeof ofn);
    ofn.lStructSize = sizeof ofn;
    ofn.hwndOwner = game_hwnd();
    ofn.lpstrFilter = save ? L"GameCube save file (*.gci)\0*.gci\0"
                           : L"GameCube saves and memory cards (*.gci;*.raw)\0*.gci;*.raw\0"
                             L"All files (*.*)\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = 4096;
    ofn.lpstrTitle = wtitle[0] ? wtitle : NULL;
    ofn.lpstrDefExt = save ? L"gci" : NULL;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR |
                (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    printf("[card-ui] showing the %s dialog\n", save ? "Save" : "Open");
    fflush(stdout);
    minimized = dialog_begin();
    ok = save ? GetSaveFileNameW(&ofn) : GetOpenFileNameW(&ofn);
    dialog_end(minimized);
    if (!ok) {
        DWORD err = CommDlgExtendedError();
        if (err) {
            fprintf(stderr, "[card-ui] file dialog failed (error 0x%lx)\n", (unsigned long)err);
            rc = -1;
        }
    } else if (wide_to_acp(file, out, outsz)) {
        printf("[card-ui] picked %s\n", out);
        rc = 1;
    } else {
        *bad_path = 1;
        rc = -1;
    }
    if (SUCCEEDED(co)) CoUninitialize();
    fflush(stdout);
    return rc;
}
#endif

int pc_file_dialog_open_save(const char *title, char *out, size_t outsz, int *bad_path)
{
    *bad_path = 0;
    if (pc_file_dialog_test_active())
        return pick_test(out, outsz);
#ifdef _WIN32
    return file_dialog(0, title, NULL, out, outsz, bad_path);
#else
    (void)title; (void)out; (void)outsz;
    return -1;
#endif
}

int pc_file_dialog_save_gci(const char *title, const char *default_name, char *out, size_t outsz,
                            int *bad_path)
{
    *bad_path = 0;
    if (pc_file_dialog_test_active())
        return pick_test(out, outsz);
#ifdef _WIN32
    return file_dialog(1, title, default_name, out, outsz, bad_path);
#else
    (void)title; (void)default_name; (void)out; (void)outsz;
    return -1;
#endif
}

/* Explorer on the folder (ANSI path); 1 if it started */
int pc_open_folder(const char *path)
{
    if (pc_file_dialog_test_active()) {
        printf("[card-ui] open folder (test): %s\n", path);
        return 1;
    }
#ifdef _WIN32
    {
        WCHAR w[1024];
        char p[1024];
        size_t i;
        int ok;
        snprintf(p, sizeof p, "%s", path);
        for (i = 0; p[i]; i++)
            if (p[i] == '/') p[i] = '\\';
        if (MultiByteToWideChar(CP_ACP, 0, p, -1, w, 1024) <= 0)
            return 0;
        {   /* fullscreen would cover the Explorer window */
            SDL_Window *win = (SDL_Window *)platform_sdl_window();
            if (win && (SDL_GetWindowFlags(win) & SDL_WINDOW_FULLSCREEN))
                SDL_MinimizeWindow(win);
        }
        ok = (INT_PTR)ShellExecuteW(NULL, L"open", w, NULL, NULL, SW_SHOWNORMAL) > 32;
        printf("[card-ui] open folder %s: %s\n", p, ok ? "ok" : "failed");
        fflush(stdout);
        return ok;
    }
#else
    (void)path;
    return 0;
#endif
}

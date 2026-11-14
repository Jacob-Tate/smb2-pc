/* In-game updater (docs/updater.md): checks the GitHub Releases of the repo
 * set at build time (-DSMB2_UPDATE_REPO=owner/name) for a newer version,
 * downloads + SHA-256-verifies the setup in installed mode, and starts it.
 * Network work runs on a background thread (WinHTTP, Windows only); the
 * game-side UI is src/game/shim/pc_update_ui.c. */
#ifndef PC_UPDATE_H
#define PC_UPDATE_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
    PCU_IDLE = 0,       /* no check made this run */
    PCU_CHECKING,
    PCU_UP_TO_DATE,
    PCU_AVAILABLE,      /* newer release found: pc_update_release() */
    PCU_CHECK_FAILED,   /* pc_update_error() */
    PCU_DOWNLOADING,    /* pc_update_progress() */
    PCU_VERIFYING,
    PCU_DOWNLOAD_FAILED,/* pc_update_error(); the release info stays valid */
    PCU_LAUNCHING,      /* starting Setup (the Windows admin prompt is up) */
    PCU_LAUNCHED,       /* Setup started: the game quits */
    PCU_LAUNCH_FAILED,  /* pc_update_error() */
};

enum {
    PCU_ERR_NONE = 0,
    PCU_ERR_OFFLINE,        /* no connection / name not resolved / timeout */
    PCU_ERR_HTTP,           /* unexpected HTTP status */
    PCU_ERR_BAD_REPLY,      /* reply is not a release */
    PCU_ERR_NO_RELEASE,     /* 404: the repo has no release yet */
    PCU_ERR_NO_ASSET,       /* the release has no SMB2_PC_Setup_<ver>.exe */
    PCU_ERR_NO_SUMS,        /* no SHA256SUMS.txt, or it does not list the setup */
    PCU_ERR_MISMATCH,       /* SHA-256 (or size) of the download is wrong */
    PCU_ERR_DOWNLOAD,       /* download broke off */
    PCU_ERR_WRITE,          /* cannot write the temp file */
    PCU_ERR_CANCELLED,      /* download cancelled by the player */
    PCU_ERR_LAUNCH,         /* Setup could not be started */
    PCU_ERR_LAUNCH_DENIED,  /* the admin prompt was declined */
    PCU_ERR_DISABLED,       /* no update repo in this build */
};

typedef struct {
    char version[32];   /* tag without the leading v */
    char tag[40];
    char name[128];     /* release title (UTF-8) */
    char html_url[512]; /* release page */
    char* body;         /* release notes as written (UTF-8 Markdown), never NULL once available */
    char setup_name[96];
    char setup_url[1024];
    long long setup_size;
    char sums_url[1024];
} PcuRelease;

const char* pc_update_current_version(void);   /* -DSMB2_VERSION */
const char* pc_update_repo(void);              /* -DSMB2_UPDATE_REPO ("" = disabled) */
int  pc_update_enabled(void);                  /* repo set, or the SMB2_UPDATE_TEST hook */
int  pc_update_installed(void);                /* install.ini: Setup can update in place */

/* main.c, after pcs_load: log the version and start the automatic check
 * (if on, at most once a day; never in automated runs unless the test hook
 * is set).  Never blocks. */
void pc_update_startup(int automated);
/* Once per frame on the main thread (video_finish_frame): applies results
 * of the worker (settings.dat writes). */
void pc_update_poll(void);

void pc_update_check_now(void);                /* manual "Check now" */
int  pc_update_state(void);
int  pc_update_error(void);
const PcuRelease* pc_update_release(void);
/* The automatic check found a release newer than this exe that the player
 * has not skipped, and the box has not been shown yet this run. */
int  pc_update_should_prompt(void);
void pc_update_prompt_shown(void);
void pc_update_skip_release(void);             /* "Skip this version" */

void pc_update_download_start(void);           /* installed mode: download + verify + start Setup */
void pc_update_cancel(void);
void pc_update_progress(long long* done, long long* total);
int  pc_update_open_page(void);                /* release page in the browser; 0 on failure */

/* semver-ish compare of "1.2.3[-pre]" (leading v ignored): <0, 0, >0 */
int  pc_update_vercmp(const char* a, const char* b);
/* Release notes (UTF-8 Markdown) -> plain UTF-8 text for the box.
 * Lines starting with '\x01' are headings. Caller frees. */
char* pc_update_plain_notes(const char* md);

/* game-side UI (src/game/shim/pc_update_ui.c) */
void pc_update_notes_request(void);    /* the box, at the end of this frame */
void pc_update_screen_request(void);   /* the Updates screen, at the end of this frame */
int  pc_update_ui_run_requested(void); /* video_finish_frame; 1 if a modal screen ran */
void pc_update_notes_run(int captured);
void pc_update_screen_run(int captured);

#ifdef __cplusplus
}
#endif
#endif

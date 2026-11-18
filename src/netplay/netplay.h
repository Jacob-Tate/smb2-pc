/* Netplay client (E6 lockstep, E8 lobby + session start): ENet relay client.
 * See docs/handoff/E6.md and docs/handoff/E8.md.
 *
 * Two halves:
 *
 *  1. LOBBY (offline, non-blocking): usable from inside the running game.  The
 *     platform loop calls netplay_lobby_tick() once per frame (also while the
 *     overlay pause loop runs); everything else just sets intents or reads
 *     state.  Typical flow (E7 puts the ImGui "Online" tab on top):
 *         netplay_lobby_connect("host:7855", "Alice");
 *         netplay_lobby_create(-1) / netplay_lobby_join("ABCD");
 *         netplay_lobby_room(&info) ... netplay_lobby_set_port(2);
 *         netplay_lobby_start();                       (host)
 *         while (netplay_lobby_next_event(&ev)) toast(ev.text);
 *     On START_GAME every client restarts the exe with session arguments
 *     (--server H:P --np-resume CODE,ID,TOKEN --name N): the new process
 *     reconnects, takes its seat back (server RESUME), gets the session config
 *     and boots a fresh deterministic session, lockstep from poll 0.  The old
 *     process quits.  When the session ends the exe restarts into offline mode.
 *
 *  2. LOCKSTEP (in a session): netplay_begin() installs the inputsync NETPLAY
 *     source: each poll sends the local pad for poll+delay and blocks until the
 *     server's FRAME for the poll arrives (pumping SDL events, the network and
 *     the hang watchdog).
 *
 * The command-line flags (--server --create --join ...) are a thin blocking
 * wrapper over the lobby API that runs in main() in a fresh process, so no
 * restart is needed on that path (netplay_lobby_run).
 */
#ifndef PC_NETPLAY_NETPLAY_H
#define PC_NETPLAY_NETPLAY_H

#include <stdint.h>
#include "netplay/session.h"

#define NETPLAY_MAX_PLAYERS 4
#define NETPLAY_NAME_MAX 24
#define NETPLAY_DEFAULT_SERVER "10.0.0.5:7855"

typedef struct NetplayOptions {
    const char *server;      /* "host" or "host:port" (default port 7855) */
    const char *name;        /* player name (default "Player") */
    int create;              /* 1 = create a room (host) */
    const char *join_code;   /* room code to join (when !create) */
    int delay;               /* host: input delay in polls (default 2, <= 30) */
    int players;             /* host: START_GAME when this many members are in (default 2) */
    const char *code_file;   /* host: write the room code to this file (test scripts) */
    int lobby_timeout_s;     /* give up waiting in the lobby after this long (default 120) */
} NetplayOptions;

typedef enum NetplayLobbyState {
    NP_LOBBY_OFFLINE = 0,    /* no connection */
    NP_LOBBY_CONNECTING,     /* connecting / HELLO sent */
    NP_LOBBY_READY,          /* connected, not in a room */
    NP_LOBBY_JOINING,        /* create/join sent, waiting for the room */
    NP_LOBBY_IN_ROOM,        /* in a room (lobby phase) */
    NP_LOBBY_STARTING,       /* GAME_START received: restarting into the session */
    NP_LOBBY_RESUMING,       /* (restarted process) taking the seat back */
    NP_LOBBY_IN_GAME         /* lockstep running (netplay_begin) */
} NetplayLobbyState;

typedef struct NetplayPlayer {
    int id;                  /* server client id */
    char name[NETPLAY_NAME_MAX + 1];
    int ping_ms;             /* round trip to the server, 0 = unknown */
    int port;                /* 0..3 = P1..P4 */
    int is_host;
    int is_self;
} NetplayPlayer;

typedef struct NetplayRoomInfo {
    char code[5];
    int count;
    NetplayPlayer players[NETPLAY_MAX_PLAYERS];   /* join order */
    int owner_id[NETPLAY_MAX_PLAYERS];            /* client id per port, -1 = free */
    int self_id, host_id;
    int self_is_host;
    int self_port;           /* -1 if none */
    int in_game;             /* 1 while the lockstep session runs (E7: netplay_lobby_room also answers then; delay = the session delay) */
    int delay;               /* input delay the host will start with (-1 = auto) */
    int suggested_delay;     /* from the pings */
} NetplayRoomInfo;

typedef enum NetplayEventKind {
    NP_EV_INFO = 0,          /* joined, left, connected ... */
    NP_EV_WARN,
    NP_EV_ERROR              /* server errors, lost connection, failed start */
} NetplayEventKind;

typedef struct NetplayEvent {
    int kind;
    char text[192];
} NetplayEvent;

#ifdef __cplusplus
extern "C" {
#endif

/* ---- command line (main.c) ---- */
void netplay_set_options(const NetplayOptions *opt);
int netplay_requested(void);                 /* 1 once --server was given */
/* Original argv (needed to restart the program). */
void netplay_set_argv(int argc, char **argv);
/* --np-resume CODE,ID,TOKEN (set by the restart); --np-notice text (shown as a toast). */
int netplay_set_resume(const char *spec);
int netplay_is_resume(void);
void netplay_push_notice(int kind, const char *text);
/* --np-wait-pid: block until that process exits (the old process of a restart). */
void netplay_wait_for_parent(unsigned long pid);
/* --np-exit-file F: write a line to F when a restarted session process exits (test scripts). */
void netplay_set_exit_file(const char *path);
/* Automated runs (--max-frames / --input-script) exit instead of restarting when a session ends. */
void netplay_set_automated(int on);
/* --lobby-test-script "F:action,..." : see docs/handoff/E8.md.  Drives the lobby API from the frame loop. */
int netplay_set_lobby_script(const char *spec);

/* ---- lobby API (non-blocking, call from the main thread) ---- */
/* Once per frame while offline (video_finish_frame and the overlay pause loop). */
void netplay_lobby_tick(void);

int  netplay_lobby_connect(const char *server, const char *name);   /* 1 = started */
void netplay_lobby_disconnect(void);
int  netplay_lobby_create(int delay);        /* delay in polls, -1 = auto from pings; may be called before connected */
int  netplay_lobby_join(const char *code);   /* 4 letters, case-insensitive */
int  netplay_lobby_set_port(int port);       /* 0..3 */
int  netplay_lobby_set_delay(int delay);     /* host: -1 = auto */
int  netplay_lobby_start(void);              /* host: START_GAME; 0 + toast if not possible */
void netplay_lobby_leave(void);              /* leave the room, stay connected */
void netplay_lobby_set_locale(int locale);   /* room language (all members must announce the same; default 0) */

NetplayLobbyState netplay_lobby_state(void);
const char *netplay_lobby_status(void);      /* short human text for a status line */
int  netplay_lobby_room(NetplayRoomInfo *out);   /* 1 if in a room */
int  netplay_lobby_next_event(NetplayEvent *out);/* pops the toast queue; 0 when empty */
int  netplay_lobby_server_can_start(void);   /* server supports starting a game from the game (RESUME) */

/* ---- blocking command-line wrappers (main(), fresh process, game data resolved) ---- */
/* 1 on success with *cfg = the room's session config (epoch chosen by the server), 0 on failure (message printed). */
int netplay_lobby_run(SessionConfig *cfg);
/* Restarted process: reconnect, take the seat back, get the session config. */
int netplay_lobby_resume(SessionConfig *cfg);

/* ---- session ---- */
/* After pc_session_begin(cfg): start lockstep.  Returns 1 on success. */
int netplay_begin(void);
/* 1 while a netplay session is running (lockstep installed). */
int netplay_active(void);
/* Input delay of the running session (polls). */
int netplay_input_delay(void);
/* E7: leave the running session: tells the server and restarts into offline play (exits in automated runs). */
void netplay_session_leave(void);
/* Leave the room and close the connection (also registered with atexit). */
void netplay_shutdown(void);

#ifdef __cplusplus
}
#endif
#endif

/* Netplay client (E6): lockstep input relay over ENet, see docs/handoff/E6.md.
 *
 * Flow (main.c):
 *   netplay_set_options(&opt);                 from --server/--create/--join/...
 *   netplay_lobby_run(&cfg);                   blocking command-line lobby:
 *                                              connect, HELLO, create/join, (host:
 *                                              wait for --players, START_GAME), wait
 *                                              for GAME_START -> session config
 *   pc_session_begin(&cfg); [inputsync_open_record]
 *   netplay_begin();                           installs the inputsync NETPLAY source
 *                                              and the SDL event watch
 * From then on inputsync_exchange() calls netplay's exchange once per poll:
 * send the local pad for poll+delay, block until the server's FRAME for the
 * poll arrives (pumping SDL events and the network, bumping the watchdog).
 */
#ifndef PC_NETPLAY_NETPLAY_H
#define PC_NETPLAY_NETPLAY_H

#include <stdint.h>
#include "netplay/session.h"

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

#ifdef __cplusplus
extern "C" {
#endif

void netplay_set_options(const NetplayOptions *opt);
/* 1 once --server was given (options set) */
int netplay_requested(void);

/* Blocking lobby; 1 on success with *cfg = the room's session config (epoch chosen by
 * the server), 0 on failure (message printed). */
int netplay_lobby_run(SessionConfig *cfg);

/* After pc_session_begin(cfg): start lockstep.  Returns 1 on success. */
int netplay_begin(void);

/* 1 while a netplay session is running (lockstep installed). */
int netplay_active(void);

/* Input delay of the running session (polls). */
int netplay_input_delay(void);

/* Leave the room and close the connection (also registered with atexit). */
void netplay_shutdown(void);

#ifdef __cplusplus
}
#endif
#endif

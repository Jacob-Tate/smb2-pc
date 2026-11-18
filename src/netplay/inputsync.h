/* Input sync (E3): the single place the game's per-poll pads come from.
 *
 * One "poll" = one video_finish_frame call (not one sim tick: Baseball makes
 * extra calls, docs/handoff/E1.md D18).  Each poll the input code
 * (input_sdl2.c, input_poll_gc_pads_game) samples the LOCAL devices, runs the
 * game's stick processing on them, and hands the 4 post-gate pads to
 * inputsync_exchange(); what comes back is what the game reads for all 4
 * ports (including `err`: -1 = no controller on that port).
 *
 * Sources:
 *   LIVE    local pads are used unchanged (default; exchange is a no-op copy).
 *   RECORD  as LIVE, and every poll's 4 pads are appended to a file.
 *   REPLAY  the pads come from a recorded file; local devices are ignored.
 *   NETPLAY a callback (installed by the E6 client) returns the bundle.
 *
 * Wire format per port = SmbNetPad (9 bytes, include/netplay/protocol.h).
 * Record file (little-endian):
 *   magic "SMB2REC1" (8), u16 version (1), u16 flags (0), u8 build_id[40],
 *   u32 disc_hash, u8 locale, u64 epoch, u32 session flags,
 *   u8 pointer_size (recording arch, informational), u8 ports (4),
 *   then per poll: 4 x SmbNetPad.
 */
#ifndef PC_NETPLAY_INPUTSYNC_H
#define PC_NETPLAY_INPUTSYNC_H

#include <stdint.h>
#include "netplay/protocol.h"
#include "netplay/session.h"

#define INPUTSYNC_PORTS 4

typedef enum InputSyncSource {
    INPUTSYNC_LIVE = 0,
    INPUTSYNC_RECORD,
    INPUTSYNC_REPLAY,
    INPUTSYNC_NETPLAY
} InputSyncSource;

/* NETPLAY callback (E6).  Called once per poll with the local post-gate pads
 * (poll = 0-based poll index, the same indexing as the wire protocol); it may
 * block (pumping events, bumping g_shim_frame_pulse) and must fill out[4].
 * Return 0 on success, non-zero to abort the session (the game then exits). */
typedef int (*InputSyncNetFn)(uint32_t poll, const SmbNetPad local[INPUTSYNC_PORTS],
                              SmbNetPad out[INPUTSYNC_PORTS], void *user);

#ifdef __cplusplus
extern "C" {
#endif

/* Setup (main.c, before the game starts).  Return 1 on success. */
int inputsync_open_record(const char *path);   /* after pc_session_begin */
/* Reads the header into *cfg (so main can pc_session_begin with it) and loads the file. */
int inputsync_open_replay(const char *path, SessionConfig *cfg);
void inputsync_set_netplay(InputSyncNetFn fn, void *user);
/* Stop after this many polls (--max-frames): flushes files and exits 0. */
void inputsync_set_max_polls(uint64_t n);

InputSyncSource inputsync_source(void);
/* 1 when the game's pads must be overwritten by the exchange result (not LIVE). */
int inputsync_overrides(void);
/* Polls exchanged so far. */
uint64_t inputsync_poll_count(void);

/* The per-poll exchange (see above).  `local` and `out` may alias. */
void inputsync_exchange(const SmbNetPad local[INPUTSYNC_PORTS], SmbNetPad out[INPUTSYNC_PORTS]);

#ifdef __cplusplus
}
#endif
#endif

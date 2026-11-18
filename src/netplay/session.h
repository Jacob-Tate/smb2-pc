/* Deterministic session mode (E2): makes a run reproducible from identical
 * input.  Used by --det-session now and by the netplay client later (E6/E8).
 *
 * SessionConfig carries the same fields as SmbNetSessionConfig in
 * include/netplay/protocol.h (build id, disc manifest hash, locale, epoch,
 * flags) but does not depend on that header, so the game side and the wire
 * format can evolve separately; the netplay client copies field by field.
 *
 * Rules (docs/handoff/E2.md has the full table):
 *   - pc_session_begin() is called once, early in main(), in a FRESH process
 *     (shim state such as the audio synth, rand state, card, quit flags must
 *     start from their initial values on every peer: E1 D20).
 *   - everything below is inert (and costs one branch) while
 *     pc_session_active() is 0, so normal play is unchanged.
 */
#ifndef PC_NETPLAY_SESSION_H
#define PC_NETPLAY_SESSION_H

#include <stdint.h>

#define PC_SESSION_BUILD_ID_MAX 40

/* SessionConfig.flags (same values as SMB_CFG_FLAG_*) */
#define PC_SESSION_FLAG_ALLOW_CROSS_ARCH 0x00000001u

typedef struct SessionConfig {
    char     build_id[PC_SESSION_BUILD_ID_MAX]; /* NUL-padded; must match on all peers */
    uint32_t disc_hash;                         /* disc manifest hash (fst_crc32) */
    uint8_t  locale;                            /* game locale 0..5 (en de fr es it ja) */
    uint64_t epoch;                             /* seeds the deterministic clock */
    uint32_t flags;                             /* PC_SESSION_FLAG_* */
} SessionConfig;

#ifdef __cplusplus
extern "C" {
#endif

/* Defaults used by --det-session: build id = smb2_build_id(), disc 0, locale 0,
 * a fixed epoch, no flags. */
void pc_session_default_config(SessionConfig *cfg);

/* Start deterministic mode with this config.  Returns 1 on success, 0 if a
 * session is already active or the config is invalid (locale > 5). */
int pc_session_begin(const SessionConfig *cfg);

int pc_session_active(void);
const SessionConfig *pc_session_config(void);   /* NULL when inactive */

/* Sim poll counter: number of video_finish_frame calls so far (E1 D18: count
 * polls, not ticks).  0 before the first poll finishes. */
uint64_t pc_session_poll_count(void);

/* Called once per video_finish_frame (shim) while active: advances the poll
 * counter and re-applies session-canonical game state (unlocked party
 * games).  E3 hooks its pad exchange / checksum next to this call. */
void pc_session_on_poll(void);

/* Deterministic replacements for OSGetTime / OSGetTick (os_shim.c) */
int64_t  pc_session_os_time(void);   /* GC ticks since 2000-01-01 */
uint32_t pc_session_os_tick(void);
uint32_t pc_session_card_time(void); /* fixed card timestamp (seconds, GC epoch) */

/* getenv() for test/debug hooks that change the simulation (E1 D13): returns
 * NULL while a session is active.  (--input-script is not one of them.) */
const char *pc_session_dbg_getenv(const char *name);


#ifdef __cplusplus
}
#endif
#endif

/* Deterministic session mode: config, clock, poll counter.  See session.h and
 * docs/handoff/E2.md.  The game-state side (unlock bits, logging) is in
 * src/game/shim/pc_session_shim.c, where the decomp's globals are visible. */
#include "netplay/session.h"
#include <string.h>
#include "build_id.h"
#include <stdio.h>

#define GC_TICK_HZ 40500000ull          /* bus clock / 4 */
#define TICKS_PER_POLL 675000ull        /* GC_TICK_HZ / 60 */
#define GC_EPOCH_2020 631152000ull      /* seconds from 2000-01-01 to 2020-01-01 */
#define EPOCH_SPAN_SECS 315360000ull    /* the clock starts within ten years after that */

static int s_active;
static SessionConfig s_cfg;
static uint64_t s_polls;
static uint32_t s_tick_calls;           /* OSGetTick calls since the last poll */
static uint64_t s_base_secs;            /* clock start, seconds since 2000-01-01 */

void pc_session_default_config(SessionConfig *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    /* the real build id (<=24 chars, fits PC_SESSION_BUILD_ID_MAX and the protocol's SMB_NET_MAX_BUILD_ID) */
    snprintf(cfg->build_id, sizeof cfg->build_id, "%s", smb2_build_id());
    cfg->disc_hash = 0;
    cfg->locale = 0;
    cfg->epoch = 0x534D423244455453ull;   /* "SMB2DETS" */
    cfg->flags = 0;
}

int pc_session_begin(const SessionConfig *cfg)
{
    uint64_t e;

    if (s_active || !cfg || cfg->locale > 5)
        return 0;
    s_cfg = *cfg;
    s_cfg.build_id[PC_SESSION_BUILD_ID_MAX - 1] = 0;
    e = s_cfg.epoch ^ (s_cfg.epoch >> 32) ^ (s_cfg.epoch >> 17);
    s_base_secs = GC_EPOCH_2020 + (e % EPOCH_SPAN_SECS);
    s_polls = 0;
    s_tick_calls = 0;
    s_active = 1;
    printf("[session] deterministic session: build '%s' disc %08x locale %u epoch %016llx flags %x\n",
           s_cfg.build_id, (unsigned)s_cfg.disc_hash, (unsigned)s_cfg.locale,
           (unsigned long long)s_cfg.epoch, (unsigned)s_cfg.flags);
    return 1;
}

int pc_session_active(void) { return s_active; }
const SessionConfig *pc_session_config(void) { return s_active ? &s_cfg : NULL; }
uint64_t pc_session_poll_count(void) { return s_polls; }

extern void pc_session_game_sync(void);   /* pc_session_shim.c */
extern void pc_session_state_log(void);

void pc_session_on_poll(void)
{
    if (!s_active)
        return;
    s_polls++;
    s_tick_calls = 0;
    pc_session_game_sync();
    pc_session_state_log();
}

int64_t pc_session_os_time(void)
{
    return (int64_t)(s_base_secs * GC_TICK_HZ + s_polls * TICKS_PER_POLL);
}

/* monotonic and frame-derived; the call index inside a poll keeps two reads in
 * one tick distinct, as on real hardware (the call sequence is deterministic) */
uint32_t pc_session_os_tick(void)
{
    return (uint32_t)((uint64_t)pc_session_os_time() + s_tick_calls++);
}

uint32_t pc_session_card_time(void) { return (uint32_t)s_base_secs; }

#include <stdlib.h>
const char *pc_session_dbg_getenv(const char *name)
{
    return s_active ? NULL : getenv(name);
}

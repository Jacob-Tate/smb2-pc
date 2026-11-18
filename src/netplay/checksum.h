/* State checksum (E3): a curated, layout-independent hash of the simulation,
 * computed every 60 polls in a deterministic session.
 *
 * The hash itself is built in a game TU (src/game/shim/pc_state_hash.c) with
 * the helpers below, so struct field access is layout independent (i686 and
 * x64 differ in struct sizes, docs/handoff/E1.md D15).  This file holds the
 * hasher, the log, the "latest checksum" for the netplay client (E6), an
 * optional per-poll ring for bisecting a desync, and the cost counters.
 *
 * Poll numbering: checksum_on_poll(n) is called from pc_session_on_poll with
 * n = pc_session_poll_count() (1 for the first video_finish_frame).  Lines are
 * written when n % 60 == 0:  "poll N hash XXXXXXXX".
 */
#ifndef PC_NETPLAY_CHECKSUM_H
#define PC_NETPLAY_CHECKSUM_H

#include <stdint.h>
#include <string.h>

#define CHECKSUM_PERIOD 60
#define CHECKSUM_REGIONS 8       /* sub-hashes for naming a diverging region */
#define CHECKSUM_RING 600

typedef struct PcHash {
    uint64_t h;
} PcHash;

static inline void pch_init(PcHash *s) { s->h = 0x9E3779B97F4A7C15ull; }
static inline void pch_u32(PcHash *s, uint32_t v)
{
    uint64_t x = (s->h ^ v) * 0xFF51AFD7ED558CCDull;
    s->h = x ^ (x >> 32);
}
static inline void pch_s32(PcHash *s, int32_t v) { pch_u32(s, (uint32_t)v); }
static inline void pch_f32(PcHash *s, float f)
{
    uint32_t u;
    memcpy(&u, &f, 4);
    pch_u32(s, u);
}
/* raw bytes: only for pointer-free structs whose size is the same on i686 and x64 */
static inline void pch_bytes(PcHash *s, const void *p, size_t n)
{
    const uint8_t *b = (const uint8_t *)p;
    uint32_t w;
    while (n >= 4) { memcpy(&w, b, 4); pch_u32(s, w); b += 4; n -= 4; }
    if (n) { w = 0; memcpy(&w, b, n); pch_u32(s, w); }
}
static inline uint64_t pch_final(const PcHash *s)
{
    uint64_t x = s->h;
    x ^= x >> 33; x *= 0xC4CEB9FE1A85EC53ull; x ^= x >> 33;
    return x;
}
static inline uint32_t pch_fold32(uint64_t h) { return (uint32_t)(h ^ (h >> 32)); }

#ifdef __cplusplus
extern "C" {
#endif

/* Game side (pc_state_hash.c): hash of the sim state, plus one sub-hash per
 * region (names in pc_state_hash_region_name). */
uint64_t pc_state_hash(uint64_t regions[CHECKSUM_REGIONS]);
const char *pc_state_hash_region_name(int i);

/* Open the checksum log (--checksum-log).  NULL/"" = no file. */
void checksum_open_log(const char *path);
/* Compute the hash on every poll into the ring (bisecting; costs one hash per poll). */
void checksum_enable_ring(int on);

/* Called by pc_session_on_poll after the poll counter advanced. */
void checksum_on_poll(uint64_t poll);

/* Most recent periodic checksum: returns its poll number (0 = none yet). */
uint32_t checksum_latest(uint32_t *hash_out);
/* Per-poll ring lookup (only the last CHECKSUM_RING polls, ring enabled): 1 = found. */
int checksum_ring_get(uint32_t poll, uint32_t *hash_out);
/* Region sub-hashes of the latest periodic checksum. */
const uint64_t *checksum_latest_regions(void);

/* Cost statistics (nanoseconds per pc_state_hash call) */
void checksum_get_cost(uint64_t *calls, double *avg_us, double *max_us);

#ifdef __cplusplus
}
#endif
#endif

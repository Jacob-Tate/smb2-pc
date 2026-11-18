/* Game-state side of the deterministic session (src/netplay/session.c). */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "netplay/session.h"

/* decomp unlock.c: struct UnlockInfo unlock_info (0x84 bytes), partyGames u32
 * at +0x10 (same layout on i686 and x64) */
extern unsigned char unlock_info[0x84];
#define UNLOCK_PARTY_GAMES_OFS 0x10
/* the six play-point locked party games (bits 9, 0xA, 0xC, 0xD, 0xF, 0x10) plus
 * the free ones: are_all_party_games_unlocked() is true (E1.md section 3, item 5) */
#define ALL_PARTY_GAMES 0x1B600u

/* Once per poll (pc_session_on_poll).  The session card starts empty, so the
 * game builds its own save from unlock_info; keeping the party-game bits set
 * from boot on makes that save, and the menus, identical on every peer. */
void pc_session_game_sync(void)
{
    uint32_t v;
    memcpy(&v, unlock_info + UNLOCK_PARTY_GAMES_OFS, sizeof v);
    v |= ALL_PARTY_GAMES;
    memcpy(unlock_info + UNLOCK_PARTY_GAMES_OFS, &v, sizeof v);
}

/* One line every 60 polls while a session is active: a fingerprint of the
 * simulation for diffing runs until E3's checksum exists.  Floats are printed
 * as raw bits.  Only ball 0 is read (struct Ball is 0x1B0 / 0x1B8 bytes on
 * i686 / x64, so other indices would need the real layout). */
extern uint32_t pc_rand_state(void);
extern int32_t main_mode, sub_mode, frame_counter;
struct DetBall { uint32_t pad0; float px, py, pz; float prevx, prevy, prevz; float vx, vy, vz; };
extern struct DetBall balls[];
extern long g_input_script_frame;

static uint32_t fbits(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }

void pc_session_state_log(void)
{
    uint64_t n = pc_session_poll_count();
    if (n % 60 != 0)
        return;
    printf("[detlog] poll %llu rand %08x mode %d/%d fc %d script %ld",
           (unsigned long long)n, (unsigned)pc_rand_state(), (int)main_mode, (int)sub_mode,
           (int)frame_counter, g_input_script_frame);
    if (main_mode == 2)
        printf(" ball %08x %08x %08x v %08x %08x %08x", fbits(balls[0].px), fbits(balls[0].py),
               fbits(balls[0].pz), fbits(balls[0].vx), fbits(balls[0].vy), fbits(balls[0].vz));
    printf("\n");
    fflush(stdout);
}

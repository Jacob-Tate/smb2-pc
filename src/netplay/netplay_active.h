/* netplay_active(): 1 while an online (lockstep) session is running.
 * D3 uses it to decide whether the overlay may pause the game (never online). */
#ifndef PC_NETPLAY_ACTIVE_H
#define PC_NETPLAY_ACTIVE_H
#ifdef __cplusplus
extern "C" {
#endif
int netplay_active(void);
/* Test hook (--overlay-test-online): pretend a session is active so the
 * overlay behaves as online (no pause) without a network. */
void netplay_test_force_active(int on);
#ifdef __cplusplus
}
#endif
#endif

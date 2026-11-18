/* STUB (D3).  E6 replaces this file: delete it from CMakeLists.txt and provide the
 * real netplay_active() (and netplay_test_force_active, or keep this one's
 * test override and fold the real state into it) in the netplay client. */
#include "netplay/netplay_active.h"

static int s_forced;

void netplay_test_force_active(int on) { s_forced = on; }
int netplay_active(void) { return s_forced; }

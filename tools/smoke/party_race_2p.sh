#!/bin/bash
# Party Games > Monkey Race 2 > One Course Race > 2 players, through the real menus.
# Player 1 = script tokens (keyboard path), player 2 = virtual Xbox pad on port 2
# (VADDX2). Ends in main mode 4 / sub mode 186 (MD_MINI / SMD_MINI_RACE2_MAIN) with a
# 2-player split screen; dumps a frame there. ~65 s.
# usage: party_race_2p.sh [exe]
. "$(dirname "${BASH_SOURCE[0]}")/common.sh"
S="1350:START,1450:START,1550:START"        # title -> Mode Select
S+=",1600:VADDX2"                           # plug virtual pad into port 2
S+=",1800:RIGHT,1900:A"                     # Party Games
S+=",2200:A"                                # Monkey Race 2
S+=",2400:A"                                # One Course Race
S+=",2600:RIGHT,2700:A"                     # 2 players
S+=",3000:A,3100:VB0"                       # P1 char, P2 char (VB0 = pad A button)
S+=",3300:A,3500:A,3700:A"                  # course, settings, Game Start
smoke_run 3901 3900 "$S" || fail "exe exit code $? (see $LOG)"
grep -aq "port 2: Virtual Xbox One pad connected" "$LOG" || fail "virtual pad not on port 2"
has_mode 1 32 || fail "never reached Mode Select"
has_mode 4 186 || fail "never reached mode 4/186 (Monkey Race 2)"
check_frame 3900
pass "Monkey Race 2, 2 players (mode 4/186, pad on port 2)"

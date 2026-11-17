#!/bin/bash
# Boots, skips the intro with START, reaches Mode Select (main mode 1 = MD_SEL,
# sub mode 32 = SMD_SEL_NGC_MAIN), dumps a frame there, exits 0. ~45 s.
# usage: boot_to_menu.sh [exe]
. "$(dirname "${BASH_SOURCE[0]}")/common.sh"
smoke_run 1801 1800 "1350:START,1450:START,1550:START" || fail "exe exit code $? (see $LOG)"
grep -aq "\[exit-frame\] reached frame 1801" "$LOG" || fail "did not reach exit frame"
has_mode 1 32 || fail "never reached mode 1/32 (Mode Select); modes seen: $(grep -ao '^\[frame [0-9]*\] mode [0-9]*/[0-9]*' "$LOG" | awk '{print $4}' | uniq | tr '\n' ' ')"
check_frame 1800
pass "Mode Select reached (mode 1/32)"

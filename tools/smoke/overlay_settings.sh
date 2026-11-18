#!/bin/bash
# D4: overlay options persist.  Run 1 changes every tested option through the overlay's setters
# (--overlay-test-set = the same code the UI rows call = the Options > Game Screen code), exits;
# settings.dat must hold the new values.  Run 2 (fresh process) must load them, then changes them
# all back and settings.dat must hold the defaults again.  ~20 s.
# usage: overlay_settings.sh [exe]
. "$(dirname "${BASH_SOURCE[0]}")/common.sh"
SAVE=$OUT/save
DAT=$SAVE/settings.dat
run() {   # <log> <extra args...>
  local l=$1; shift
  ( cd "$OUT" && timeout 120 "$EXE" --iso "$ISO" --save-dir "$(cygpath -w "$SAVE")" --windowed --audio-null \
      --exit-frame 40 "$@" ) > "$OUT/$l" 2>&1 || fail "exe exit code $? ($l)"
  grep -aq "\[exit-frame\] reached frame 40" "$OUT/$l" || fail "did not reach exit frame ($l)"
}
want() {  # <file> <line...>
  local f=$1; shift
  for l in "$@"; do grep -qx "$l" "$f" || fail "settings.dat lacks '$l':$(echo; cat "$f")"; done
}
S1=(--overlay-test-set resolution=1280x720 --overlay-test-set display=borderless --overlay-test-set vsync=off
    --overlay-test-set fps=120 --overlay-test-set msaa=8 --overlay-test-set aspect=stretch --overlay-test-set volume=40
    --overlay-test-set show_fps="Top right" --overlay-test-set language=de --overlay-test-set buttons=ps
    --overlay-test-set renderer=d3d12)
run run1.txt --renderer gl "${S1[@]}"
[ "$(grep -ac '\[overlay-test\] set .*: ok' "$OUT/run1.txt")" = 11 ] || fail "not all 11 sets reported ok (run1.txt)"
want "$DAT" width=1280 height=720 display=borderless vsync=0 fps=120 msaa=8 aspect=stretch volume=40 show_fps=2 language=de buttons=ps renderer=d3d12
cp "$DAT" "$OUT/after1.dat"
# restart: values are loaded from the file (no CLI overrides)
S2=(--overlay-test-set display=windowed --overlay-test-set vsync=on --overlay-test-set fps=60 --overlay-test-set msaa=4
    --overlay-test-set aspect=auto --overlay-test-set volume=100 --overlay-test-set show_fps=off --overlay-test-set language=auto
    --overlay-test-set buttons=auto --overlay-test-set renderer=gl)
run run2.txt "${S2[@]}"
grep -aq "\[settings\] loaded .*1280x720 borderless vsync=0 msaa=8 aspect=stretch vol=40" "$OUT/run2.txt" || fail "run 2 did not load run 1's values (run2.txt)"
[ "$(grep -ac '\[overlay-test\] set .*: ok' "$OUT/run2.txt")" = 10 ] || fail "not all 10 sets reported ok (run2.txt)"
want "$DAT" display=windowed vsync=1 fps=60 msaa=4 aspect=auto volume=100 show_fps=0 language=auto buttons=auto renderer=gl
FRAME=none
pass "display mode, resolution, frame rate, vsync, MSAA, aspect, volume, show FPS, language, buttons, renderer persist (set -> exit -> restart -> read settings.dat)"

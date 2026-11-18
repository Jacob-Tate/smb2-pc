#!/bin/bash
# Record a det-session run (Monkey Race 2, 2 pads) and replay it without any input:
# the checksum logs must be identical. usage: record_replay.sh [exe]   (~2.5 min)
. "$(dirname "${BASH_SOURCE[0]}")/common.sh"
S="1350:START,1450:START,1550:START,1600:VADDX2,1800:RIGHT,1900:A,2200:A,2400:A,2600:RIGHT,2700:A"
S+=",3000:A,3100:VB0,3300:A,3500:A,3700:A,4000-4300:SU,4300-4400:SL,4100:A,4000-4300:VLY-32768"
W=$(cygpath -w "$OUT/save")
( cd "$OUT" && timeout 600 "$EXE" --iso "$ISO" --save-dir "$W" --windowed --audio-null --det-session \
    --exit-frame 4500 --record rec.bin --checksum-log a.txt --input-script "$S" ) > "$LOG" 2>&1 || fail "record run failed"
mkdir -p "$OUT/save2"
( cd "$OUT" && timeout 600 "$EXE" --iso "$ISO" --save-dir "$(cygpath -w "$OUT/save2")" --windowed --audio-null \
    --replay rec.bin --checksum-log b.txt ) > "$OUT/replay.txt" 2>&1 || fail "replay run failed"
[ -s "$OUT/a.txt" ] || fail "empty checksum log"
cmp -s "$OUT/a.txt" "$OUT/b.txt" || fail "checksum logs differ (diff $OUT/a.txt $OUT/b.txt)"
FRAME=-; pass "record/replay checksum logs identical ($(wc -l < "$OUT/a.txt") lines)"

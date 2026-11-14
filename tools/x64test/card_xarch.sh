#!/bin/bash
# Memory-card compatibility test, 32-bit <-> x64 (docs/x64.md, "Shims, memcard, replays").
#
#   card_xarch.sh <bin32> <bin64> [renderer] [label]
#   (output in $WS/runs/<label>/, default card_xarch; SMB2_HEAP_CHECK is
#   passed through to the exe)
#
# 1. On each build, from the pristine card: clear Challenge Beginner 1 (AUTO
#    steering), pause during the goal replay, Save Replay (3 rep*.dat files).
# 2. On each build: Options > Game Data, Autosave ON, Save (overwrite).
#    The two arches' files must be byte-identical apart from the card
#    timestamp words (replay comment 0x36 / 0x2070, .meta time=).
# 3. Cross-load: each build plays the first replay from the other build's
#    card (Options > Replay) and from its own; frames are compared with
#    the 32-bit/32-bit run (expect only confetti/animation-phase noise).
# Cards and runs live in $OUT ($WS/runs/<label>/).  Only writes inside $WS, and
# replaces <bin>/memcard (restored from the pristine card at the end).
set -u
WS=${SMB2_X64_WS:-$(cd "$(dirname "$0")/../../.." && pwd)}
ISO=${SMB2_ISO:?set SMB2_ISO to your disc image as a Windows path, e.g. SMB2_ISO='D:\games\smb2.iso'}
PPM=$WS/tools/ppmtool
PRISTINE=$WS/tools/memcard_pristine
B32=$(realpath "$1"); B64=$(realpath "$2"); R=${3:-gl}
OUT=$WS/runs/${4:-card_xarch}
inws() { case "$(realpath -m "$1")" in "$WS"/*) return 0;; esac; echo "refusing path outside $WS: $1" >&2; exit 1; }
inws "$B32"; inws "$B64"; inws "$OUT"
mkdir -p "$OUT/cards"

REPSAVE="600:START,1200:START,1800:A,2100:A,2400:A,2700:A,3000-3500:AUTO,3640:START,3700:DOWN,3760:A,3950:A,4150:A,4350:A,4550:A"
GDSAVE="500:START,600:START,700:START,800:START,1300:RIGHT,1400:RIGHT,1500:A,1700:DOWN,1800:A,2000:DOWN,2060:DOWN,2120:A,2180:RIGHT,2240:A,2320:UP,2380:A,2480:LEFT,2540:A,2900:A,3100:A"
# with Autosave ON (game data cards) the boot card access shifts the menus,
# hence a separate, later-timed script to view Options > Game Data
GDVIEW="500:START,600:START,700:START,800:START,1300:RIGHT,1400:RIGHT,1500:A,1900:A,2200:RIGHT,2300:RIGHT,2400:A,2700:DOWN,2800:A"
REPLOAD="500:START,600:START,700:START,800:START,1300:RIGHT,1400:RIGHT,1500:A,1700:A,1900:A,2100:A,2300:A"

setcard() { # bin srcdir
  inws "$1/memcard"; rm -rf "$1/memcard"; mkdir -p "$1/memcard"; cp -r "$2" "$1/memcard/slotA"
}
run() { # label bin launcher script dumpint exitframe
  local o=$OUT/$1; inws "$o"; mkdir -p "$o/save"; rm -f "$o"/frame_*.ppm "$2"/frame_*.ppm
  ( cd "$2" && SMB2_PC_LAUNCHER=$3 WSLENV=SMB2_PC_LAUNCHER:SMB2_HEAP_CHECK timeout 900 ./smb2_pc_port.exe \
      --iso "$ISO" --input-script "$4" --dump-interval "$5" --exit-frame "$6" \
      --audio-null --renderer "$R" --save-dir "$(wslpath -w "$o/save")" > "$o/log.txt" 2>&1; echo "exit $?" >> "$o/log.txt" )
  mv "$2"/frame_*.ppm "$o"/ 2>/dev/null
  echo "$1: $(grep -ac 'CARDWriteAsync(0) -> 0' "$o/log.txt") card writes, $(tail -1 "$o/log.txt")"
}

for a in 32 64; do
  b=B$a; b=${!b}
  setcard "$b" "$PRISTINE/slotA"
  run save$a "$b" 1 "$REPSAVE" 500 5000
  inws "$OUT/cards/c$a"; rm -rf "$OUT/cards/c$a"; cp -r "$b/memcard/slotA" "$OUT/cards/c$a"
  run gd$a "$b" 0 "$GDSAVE" 500 3400
  inws "$OUT/cards/gd$a"; rm -rf "$OUT/cards/gd$a"; cp -r "$b/memcard/slotA" "$OUT/cards/gd$a"
done
echo "== byte compare (32 vs 64)"
cmp "$OUT/cards/gd32/super_monkey_ball_2.dat.bin" "$OUT/cards/gd64/super_monkey_ball_2.dat.bin" && echo "game data: identical"
r32=$(ls "$OUT/cards/c32"/rep*.bin | head -1); r64=$(ls "$OUT/cards/c64"/rep*.bin | head -1)
echo "replay: $(cmp -l "$r32" "$r64" | wc -l) differing bytes at $(cmp -l "$r32" "$r64" | awk '{printf "0x%x ", $1-1}')(timestamp only: 0x36 0x37 0x2070 0x2071)"

for a in 32 64; do for c in 32 64; do
  b=B$a; setcard "${!b}" "$OUT/cards/c$c"; run load${a}_c$c "${!b}" 0 "$REPLOAD" 100 3800
done; done
for a in 32 64; do for c in 32 64; do
  b=B$a; setcard "${!b}" "$OUT/cards/gd$c"; run gdload${a}_c$c "${!b}" 0 "$GDVIEW" 100 3050
done; done
echo "== Options > Game Data after cross-load (Autosave must read ON), vs gdload32_c32"
for x in gdload64_c32 gdload32_c64 gdload64_c64; do echo "$x: $($PPM diff "$OUT/gdload32_c32/frame_03000.ppm" "$OUT/$x/frame_03000.ppm")"; done
$PPM sheet "$OUT/gamedata_3000.png" 4 "$OUT"/gdload32_c32/frame_03000.ppm "$OUT"/gdload64_c32/frame_03000.ppm \
  "$OUT"/gdload32_c64/frame_03000.ppm "$OUT"/gdload64_c64/frame_03000.ppm
echo "== replay playback vs load32_c32"
for x in load64_c32 load32_c64 load64_c64; do
  for f in 02600 03000 03700; do echo "$x $f: $($PPM diff "$OUT/load32_c32/frame_$f.ppm" "$OUT/$x/frame_$f.ppm")"; done
done
$PPM sheet "$OUT/replay_3400.png" 4 "$OUT"/load32_c32/frame_03400.ppm "$OUT"/load64_c32/frame_03400.ppm \
  "$OUT"/load32_c64/frame_03400.ppm "$OUT"/load64_c64/frame_03400.ppm
setcard "$B32" "$PRISTINE/slotA"; setcard "$B64" "$PRISTINE/slotA"
echo "sheets: $OUT/replay_3400.png $OUT/gamedata_3000.png"

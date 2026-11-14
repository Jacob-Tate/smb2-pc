#!/bin/bash
# HFR evidence capture (docs/hfr.md section "Testing").
#
#   hfrcap.sh <label> <bindir> <mode> <ranges> [-- extra exe args]
#
# Runs a tools/x64test/modes.sh mode with --fps 240 (k = 4: three in-between
# frames per tick, override with --fps in the extra args) and --dump-ranges
# <ranges> (e.g. 2990-3010,3390-3400), then for every range runs hfrcheck
# (per-tick step differences, popping blocks, PNG strips of ticks with pops)
# and writes one overview strip per range (tick T-1, the in-between frames,
# tick T at the middle of the range).  The PPMs are deleted afterwards
# (unless KEEP=1).  Output: $WS/runs/<label>/<mode>/{log.txt,check_*.txt,pops/,strip_*.png}
set -u
WS=${SMB2_X64_WS:-$(cd "$(dirname "$0")/../../.." && pwd)}
HERE=$(cd "$(dirname "$0")" && pwd)
CHECK=$WS/tools/hfrcheck
PPM=$WS/tools/ppmtool
[ -x "$CHECK" ] || gcc -O2 -o "$CHECK" "$HERE/hfrcheck.c" -lz || exit 1
LABEL=$1 BIN=$2 MODE=$3 RANGES=$4; shift 4
[ "${1:-}" = "--" ] && shift
FPS=240; for ((i=1;i<=$#;i++)); do [ "${!i}" = "--fps" ] && { n=$((i+1)); FPS=${!n}; }; done
K=$(( (FPS + 30) / 60 )); [ $K -lt 2 ] && K=2
SMB2_X64_WS=$WS "$WS/pc_port/tools/x64test/modes.sh" "$LABEL" "$BIN" "$MODE" -- --fps 240 \
    --windowed --width 1280 --height 720 --dump-interval 1000000 --dump-ranges "$RANGES" "$@"
OUT=$WS/runs/$LABEL/$MODE
case "$(realpath -m "$OUT")" in "$WS"/*) ;; *) echo "bad out $OUT"; exit 1;; esac
mkdir -p "$OUT/pops"
IFS=',' read -ra RS <<< "$RANGES"
for r in "${RS[@]}"; do
  a=${r%-*}; b=${r#*-}
  "$CHECK" "$OUT" "$a" "$b" "$K" "$OUT/pops" > "$OUT/check_$a.txt"
  tail -1 "$OUT/check_$a.txt" | sed "s/^/$MODE $a-$b: /"
  m=$(( (a + b) / 2 ))
  files=("$OUT/frame_$(printf %05d $((m-1))).ppm")
  for ((j=1;j<K;j++)); do files+=("$OUT/frame_$(printf %05d $m)_$j.ppm"); done
  files+=("$OUT/frame_$(printf %05d $m).ppm")
  [ -f "${files[0]}" ] && "$PPM" sheet "$OUT/strip_$m.png" 2 "${files[@]}" > /dev/null
done
[ "${KEEP:-0}" = 1 ] || rm -f "$OUT"/frame_*.ppm

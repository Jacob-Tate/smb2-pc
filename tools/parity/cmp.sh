#!/bin/bash
# cmp.sh <dirA> <dirB> [scene...]: per-frame diff stats of matching frame dumps
A=$1; B=$2; shift 2
WS=${SMB2_X64_WS:-$(cd "$(dirname "$0")/../../.." && pwd)}   # workspace root with tools/ppmtool
SC=("$@"); [ ${#SC[@]} -eq 0 ] && SC=(intro menus challenge race target bowling boat dogfight baseball)
for sc in "${SC[@]}"; do
  for f in $A/$sc/frame_*.ppm; do
    [ -f "$f" ] || continue
    n=$(basename $f)
    [ -f $B/$sc/$n ] || { echo "$sc $n missing in B"; continue; }
    echo "$sc $n $($WS/tools/ppmtool diff $f $B/$sc/$n)"
  done
done

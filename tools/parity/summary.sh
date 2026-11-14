#!/bin/bash
# per scene: median of "% px >24" for GL-vs-GL (base vs ref1) and GL-vs-D3D12 (ref1 vs dx3); sheets
cd "${SMB2_X64_WS:-$(cd "$(dirname "$0")/../../.." && pwd)}" || exit 1   # workspace root: runs/, tools/ppmtool
T=tools/ppmtool
med() { sort -g | awk '{a[NR]=$1} END{print (NR? a[int((NR+1)/2)] : "-")}'; }
pct() { sed -n 's/.*>24 \([0-9.]*\)%.*/\1/p; s/^identical$/0/p; s/.*>24 0%$/0/p'; }
for sc in intro menus challenge race target bowling boat dogfight baseball; do
  gg=$(for f in runs/ref1/$sc/frame_*.ppm; do n=$(basename $f); [ -f runs/base/$sc/$n ] && $T diff runs/base/$sc/$n $f; done | pct | med)
  gd=$(for f in runs/ref1/$sc/frame_*.ppm; do n=$(basename $f); [ -f runs/dx3/$sc/$n ] && $T diff $f runs/dx3/$sc/$n; done | pct | med)
  echo "$sc  GL-vs-GL median >24: ${gg}%   GL-vs-D3D12 median >24: ${gd}%"
done

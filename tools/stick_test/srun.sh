#!/bin/bash
# Test helpers for docs/stick.md (workspace root: $SMB2_X64_WS, default the directory
# containing this pc_port checkout; disc image: $SMB2_ISO as a Windows path):
#   srun.sh: PC run with an input script + per-frame ball log;  dolrun.sh + dsu_server.py:
#   headless Dolphin with a DSU pad (LXnnn/LYnnn = stick bytes);  pngcrop.py: PNG crop to PPM
# srun.sh <label> <bin32|bin64> <launcher 0/1> <script> <exit_frame> [dump_interval] [extra args...]
# Runs the PC port with an input script; output in $W/runs/<label>
W=${SMB2_X64_WS:-$(cd "$(dirname "$0")/../../.." && pwd)}; L=$1; B=$W/build${2#bin}/bin; LA=$3; S=$4; EF=$5; DI=${6:-0}; shift 6 2>/dev/null || shift $#
O=$W/runs/$L; mkdir -p $O/save; rm -f $O/frame_*.ppm $B/frame_*.ppm
PR=$W/tools/memcard_pristine; ISO=${SMB2_ISO:?set SMB2_ISO to your disc image as a Windows path}
[ -d "$B/memcard" ] && rm -r "$B/memcard"; cp -r $PR $B/memcard
args=(--iso "$ISO" --exit-frame $EF --audio-null --save-dir "$(wslpath -w $O/save)" --input-script "$S")
[ "$DI" != 0 ] && args+=(--dump-interval $DI)
( cd $B && SMB2_PC_LAUNCHER=$LA SMB2_BALL_LOG=1 WSLENV=SMB2_PC_LAUNCHER:SMB2_BALL_LOG timeout 600 ./smb2_pc_port.exe "${args[@]}" "$@" > $O/log.txt 2>&1; echo "exit $?" >> $O/log.txt )
mv $B/frame_*.ppm $O/ 2>/dev/null
echo "$L: $(tail -1 $O/log.txt) frames=$(ls $O/frame_*.ppm 2>/dev/null | wc -l)"

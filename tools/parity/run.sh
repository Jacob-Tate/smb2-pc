#!/bin/bash
# run.sh <bindir> <outdir> <launcher 0|1> <script> <dump-interval> <exit-frame> [extra exe args...]
# Runs the exe on the Windows host with cwd=bindir, then moves the frame dumps
# to outdir and restores the pristine memcard.
set -u
BIN=$1; OUT=$2; L=$3; SCRIPT=$4; DI=$5; EF=$6; shift 6
# SMB2_ISO: disc image (Windows path); SMB2_X64_WS: workspace root (default:
# the directory containing this pc_port checkout) with tools/memcard_pristine.
# Only writes inside the workspace.
WS=${SMB2_X64_WS:-$(cd "$(dirname "$0")/../../.." && pwd)}
ISO=${SMB2_ISO:?set SMB2_ISO to your disc image as a Windows path}
case "$(realpath -m "$BIN")" in "$WS"/*) ;; *) echo "bad bin"; exit 1;; esac
case "$(realpath -m "$OUT")" in "$WS"/*) ;; *) echo "bad out"; exit 1;; esac
mkdir -p "$OUT" "$OUT/save"
cd "$BIN" || exit 1
rm -f "$BIN"/frame_*.ppm
SAVEW=$(wslpath -w "$OUT/save")
ARGS=(--iso "$ISO" --dump-interval "$DI" --exit-frame "$EF" --audio-null --save-dir "$SAVEW")
[ -n "$SCRIPT" ] && ARGS+=(--input-script "$SCRIPT")
SMB2_PC_LAUNCHER=$L WSLENV=SMB2_PC_LAUNCHER:SMB2_RENDERER:SMB2_D3D12_DEBUG:SMB2_D3D12_FLIPFACE:SMB2_D3D12_NOCULL:SMB2_D3D12_NODEPTH:SMB2_D3D12_TRACECOPY:SMB2_D3D12_DEBUGVIS:SMB2_D3D12_FORCEFAIL timeout 600 ./smb2_pc_port.exe "${ARGS[@]}" "$@" > "$OUT/log.txt" 2>&1
echo "exit $?" >> "$OUT/log.txt"
mv "$BIN"/frame_*.ppm "$OUT"/ 2>/dev/null
rm -rf "$BIN/memcard"
cp -r "$WS/tools/memcard_pristine" "$BIN/memcard"
tail -2 "$OUT/log.txt"

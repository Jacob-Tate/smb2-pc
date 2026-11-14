#!/bin/bash
# dolrun.sh <sched_file> <stop_frame> <max_seconds> : US disc in headless Dolphin, scripted pad via DSU (port 26795)
# Dolphin user dir: $SMB2_X64_WS/dolphin; disc image: $SMB2_ISO_LINUX (a Linux path)
WS=${SMB2_X64_WS:-$(cd "$(dirname "$0")/../../.." && pwd)}
ISO=${SMB2_ISO_LINUX:?set SMB2_ISO_LINUX to the disc image path}
W=$WS/dolphin; U=$W/user; FR=$U/Dump/Frames
mkdir -p $FR; find $FR -name '*.png' -delete; rm -f $U/Dump/dsu.log
cp "$1" $W/sched.txt
DSU_PORT=26795 python3 "$(cd "$(dirname "$0")" && pwd)/dsu_server.py" $FR $W/sched.txt &
SPID=$!
/usr/games/dolphin-emu -u $U -b -v Vulkan -e "$ISO" > $W/dolphin.log 2>&1 &
PID=$!
end=$(( $(date +%s) + $3 ))
while [ $(date +%s) -lt $end ] && kill -0 $PID 2>/dev/null; do
  sleep 1
  python3 - "$FR" "$2" <<'PY' && break
import os, sys
fr, stop = sys.argv[1], int(sys.argv[2]); mx = 0
for f in os.listdir(fr):
    n = int(f[10:-4]); mx = max(mx, n)
    if n < 2650 and n % 50 and os.path.exists(os.path.join(fr, f)):
        try: os.remove(os.path.join(fr, f))
        except OSError: pass
sys.exit(0 if mx >= stop else 1)
PY
done
kill $PID 2>/dev/null; sleep 3; kill -9 $PID 2>/dev/null; kill $SPID
ls $FR | wc -l

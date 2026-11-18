#!/bin/bash
# E7 acceptance: two local game instances play an online session set up THROUGH THE OVERLAY'S ONLINE TAB code path.
# --overlay-test-online-script calls the same functions the tab's buttons call (connect, host, join by code, pick a
# port, start, ...), each instance also opens the Online tab and dumps frames in the interesting states (disconnected,
# connected, hosting alone, hosting with 2 players, guest in the room), START_GAME restarts both into a fresh session
# (E8), the session process dumps the tab again (session view), then checksums / recordings / replay are compared like
# lobby_instances.sh.
# Run from Git Bash on Windows.   usage: online_ui_instances.sh [exe]
#   NP_SERVER=host:port  an E8 server (RELAUNCHING/RESUME); default: local build/server_win/smb2-server.exe
#   NP_FRAMES=3600  --max-frames   NP_DELAY=2   SMOKE_OUT=<dir>   (frame dumps: $OUT/shots/*.ppm, PNGs if python+PIL found)
. "$(dirname "${BASH_SOURCE[0]}")/../smoke/common.sh"
FRAMES=${NP_FRAMES:-3600}
DELAY=${NP_DELAY:-2}
SERVER_PID=
cleanup() { [ -n "$SERVER_PID" ] && kill $SERVER_PID 2>/dev/null; for p in $PA $PB; do kill $p 2>/dev/null; done; }
PA=; PB=
trap cleanup EXIT

if [ -n "${NP_SERVER:-}" ]; then
  SERVER=$NP_SERVER
else
  SRV=$REPO/build/server_win/smb2-server.exe
  [ -x "$SRV" ] || fail "no $SRV (build it: MSYS_NO_PATHCONV=1 wsl -d Ubuntu -- bash <repo>/tools/netplay/build_server_win.sh)"
  PORT=$((20000 + RANDOM % 20000))
  "$SRV" --port $PORT > "$OUT/server.log" 2>&1 &
  SERVER_PID=$!
  sleep 1
  SERVER=127.0.0.1:$PORT
fi
echo "server: $SERVER   frames: $FRAMES   delay: $DELAY   out: $OUT"

SA="1350:START,1450:START,1550:START"
SB="1400:A"

cd "$OUT" || exit 2
mkdir -p save_a save_b save_r wa wb shots
W() { cygpath -w "$1"; }
rm -f a.done b.done code.txt
# session-process options (the offline process ignores them or never reaches those frames): Online tab open at frame 1000-1120, dump 1100
OV="--overlay-open-frames 1000-1120 --overlay-test-set tab=Online --dump-ranges 1100-1100"
# host: tick = platform_poll_events calls.  disconnected -> connect -> connected -> host -> code file -> wait for the guest -> start
HOSTSCRIPT="20:tab=Online,20:open,40:dump=a_disconnected,60:wait=30,60:connect=$SERVER/Alice,60:wait=60,60:dump=a_connected,60:wait=30"
HOSTSCRIPT+=",60:host=$DELAY,60:codefile=$(W "$OUT/code.txt"),60:players=2,60:wait=60,60:dump=a_hosting2,60:wait=30,60:start=2"
GUESTSCRIPT="20:tab=Online,20:open,30:connect=$SERVER/Bob,30:join=@$(W "$OUT/code.txt"),30:port=1,30:players=2,30:wait=60,30:dump=b_in_room"
( cd wa && exec timeout 900 "$EXE" --iso "$ISO" --windowed --audio-null --save-dir "$(W "$OUT/save_a")" \
    --overlay-test-online-script "$HOSTSCRIPT" $OV \
    --max-frames "$FRAMES" --record "$(W "$OUT/a.bin")" --checksum-log "$(W "$OUT/a.txt")" --input-script "$SA" --np-exit-file "$(W "$OUT/a.done")" ) > a.log 2>&1 &
PA=$!
for i in $(seq 1 120); do [ -s code.txt ] && break; kill -0 $PA 2>/dev/null || break; sleep 0.5; done
[ -s code.txt ] || fail "host never created a room (see $OUT/a.log)"
( cd wb && exec timeout 900 "$EXE" --iso "$ISO" --windowed --audio-null --save-dir "$(W "$OUT/save_b")" \
    --overlay-test-online-script "$GUESTSCRIPT" $OV \
    --max-frames "$FRAMES" --record "$(W "$OUT/b.bin")" --checksum-log "$(W "$OUT/b.txt")" --input-script "$SB" --np-exit-file "$(W "$OUT/b.done")" ) > b.log 2>&1 &
PB=$!
wait $PA; wait $PB
for i in $(seq 1 1800); do [ -f a.done ] && [ -f b.done ] && break; sleep 1; done
[ -f a.done ] || fail "host session process never finished (see $OUT/a.log)"
[ -f b.done ] || fail "guest session process never finished (see $OUT/b.log)"
sleep 1

# frame dumps -> shots/
PYTHON=${PYTHON:-}
[ -z "$PYTHON" ] && for c in "/c/Program Files/Python313/python.exe" python3 python; do command -v "$c" >/dev/null 2>&1 && PYTHON=$c && break; done
conv() {   # <ppm> <name>
  cp "$1" "shots/$2.ppm"
  if [ -n "$PYTHON" ]; then "$PYTHON" "$REPO/tools/netplay/ppm2png.py" "$1" "shots/$2.png" 0.75 2>/dev/null; fi
}
for who in a b; do
  d=w$who
  grep -a "dump '" $who.log | sed -E "s/.*dump '([a-z0-9_]+)' = frame ([0-9]+).*/\1 \2/" | while read -r name fr; do
    f=$(printf '%s/frame_%05d.ppm' $d $fr); [ -s "$f" ] && conv "$f" "$name"
  done
done
[ -s wa/frame_01100.ppm ] && conv wa/frame_01100.ppm a_session
[ -s wb/frame_01100.ppm ] && conv wb/frame_01100.ppm b_session
echo "shots: $(ls shots | tr '\n' ' ')"
for n in a_disconnected a_connected a_hosting2 b_in_room a_session; do [ -s "shots/$n.ppm" ] || fail "missing frame dump $n"; done

grep -aq "overlay-test-online\] tick .*: start" a.log || fail "host script never pressed start (see $OUT/a.log)"
grep -aq "restarting into the session" a.log || fail "host did not restart into a session (see $OUT/a.log)"
grep -aq "restarting into the session" b.log || fail "guest did not restart into a session (see $OUT/b.log)"
grep -aq "taking the seat back" a.log && grep -aq "taking the seat back" b.log || fail "a restarted process didn't resume"
N=$((FRAMES / 60))
[ "$(wc -l < a.txt)" -ge $N ] || fail "host checksum log has $(wc -l < a.txt) lines, expected $N"
cmp -s a.txt b.txt || fail "checksum logs differ (diff $OUT/a.txt $OUT/b.txt)"
cmp -s a.bin b.bin || fail "recorded bundle streams differ ($OUT/a.bin $OUT/b.bin)"
timeout 900 "$EXE" --iso "$ISO" --windowed --audio-null --save-dir "$(W "$OUT/save_r")" \
    --replay a.bin --checksum-log r.txt > r.log 2>&1 || fail "replay failed (see $OUT/r.log)"
cmp -s a.txt r.txt || fail "replay checksum log differs from the netplay log (diff $OUT/a.txt $OUT/r.txt)"
FRAME=none
pass "2 instances set up through the Online tab actions via $SERVER: $N identical checksums, identical recordings, replay matches; shots in $OUT/shots"

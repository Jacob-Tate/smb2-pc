#!/bin/bash
# E6 acceptance: two local game instances play through a relay server with scripted input on both, then
#   - both checksum logs must be identical,
#   - both recorded bundle streams (--record = the exchanged pads of all 4 ports) must be byte-identical,
#   - a --replay of that recording must give the same checksum log.
# Run from Git Bash on Windows.   usage: two_instances.sh [exe]
#   NP_SERVER=host:port  use that server (e.g. the deployed 10.0.0.5:7855); default: start a local Windows
#                        server (build/server_win/smb2-server.exe, built by tools/netplay/build_server_win.sh)
#   NP_FRAMES=3600       --max-frames   NP_DELAY=2   input delay   NP_SCENARIO=race|menu (default race)
#   SMOKE_OUT=<dir>      keep outputs there
. "$(dirname "${BASH_SOURCE[0]}")/../smoke/common.sh"
FRAMES=${NP_FRAMES:-3600}
DELAY=${NP_DELAY:-2}
SERVER_PID=
cleanup() { [ -n "$SERVER_PID" ] && kill $SERVER_PID 2>/dev/null; }
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

# Alice (P1) drives the menus to Party Games > Monkey Race 2 > One Course Race > 2 players; Bob (P2) picks his
# character, then both steer.  Local script frames = poll numbers on both sides.
SA="1350:START,1450:START,1550:START,1800:RIGHT,1900:A,2200:A,2400:A,2600:RIGHT,2700:A"
SA+=",3000:A,3300:A,3500:A,3700:A,4000-4300:SU,4300-4400:SL,4100:A"
SB="3100:A,4000-4300:SD,4300-4500:SR"
if [ "${NP_SCENARIO:-race}" = menu ]; then SA="1350:START,1450:START,1550:START"; SB="1400:A"; fi

cd "$OUT" || exit 2
mkdir -p save_a save_b save_r
COMMON=(--iso "$ISO" --windowed --audio-null --max-frames "$FRAMES" --server "$SERVER")
timeout 900 "$EXE" "${COMMON[@]}" --save-dir "$(cygpath -w "$OUT/save_a")" --create --name Alice --delay "$DELAY" \
    --players 2 --room-code-file code.txt --record a.bin --checksum-log a.txt --input-script "$SA" > a.log 2>&1 &
PA=$!
for i in $(seq 1 60); do [ -s code.txt ] && break; kill -0 $PA 2>/dev/null || break; sleep 0.5; done
[ -s code.txt ] || fail "host never created a room (see $OUT/a.log)"
CODE=$(tr -d '\r\n' < code.txt)
timeout 900 "$EXE" "${COMMON[@]}" --save-dir "$(cygpath -w "$OUT/save_b")" --join "$CODE" --name Bob \
    --record b.bin --checksum-log b.txt --input-script "$SB" > b.log 2>&1 &
PB=$!
wait $PA; RA=$?
wait $PB; RB=$?
[ $RA -eq 0 ] || fail "host exited $RA (see $OUT/a.log)"
[ $RB -eq 0 ] || fail "guest exited $RB (see $OUT/b.log)"

N=$((FRAMES / 60))
[ "$(wc -l < a.txt)" -ge $N ] || fail "host checksum log has $(wc -l < a.txt) lines, expected $N"
cmp -s a.txt b.txt || fail "checksum logs differ (diff $OUT/a.txt $OUT/b.txt)"
cmp -s a.bin b.bin || fail "recorded bundle streams differ ($OUT/a.bin $OUT/b.bin)"
if [ "${NP_SCENARIO:-race}" = race ]; then
  grep -aqE "^\[frame [0-9]+\] mode 4/186 " a.log || echo "note: race not reached in $FRAMES polls (use NP_FRAMES=4500 to see it)"
fi
timeout 900 "$EXE" --iso "$ISO" --windowed --audio-null --save-dir "$(cygpath -w "$OUT/save_r")" \
    --replay a.bin --checksum-log r.txt > r.log 2>&1 || fail "replay failed (see $OUT/r.log)"
cmp -s a.txt r.txt || fail "replay checksum log differs from the netplay log (diff $OUT/a.txt $OUT/r.txt)"
grep -aq "netplay\] poll .*waiting for" a.log b.log && echo "note: lockstep waits were logged (see logs)"
FRAME=none
pass "2 instances through $SERVER: $N identical checksums, identical bundle recordings, replay matches"

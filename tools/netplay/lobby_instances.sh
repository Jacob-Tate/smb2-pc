#!/bin/bash
# E8 acceptance: two local game instances start an online session FROM THE IN-GAME LOBBY (not the --create/--join
# command line): each instance boots offline, a --lobby-test-script drives the non-blocking lobby API from the frame
# loop (host: connect, create, write the room code, start when 2 are in; guest: connect, join by the code file),
# START_GAME makes both restart into a fresh deterministic session, then scripted input plays through the menus
# into Monkey Race.  Checks: identical checksum logs, identical bundle recordings, a --replay of the recording
# gives the same log, and both restarts really happened.
# Run from Git Bash on Windows.   usage: lobby_instances.sh [exe]
#   NP_SERVER=host:port  use that server (it must be an E8 server: RELAUNCHING/RESUME); default: a local Windows
#                        server (build/server_win/smb2-server.exe, built by tools/netplay/build_server_win.sh)
#   NP_FRAMES=3600  --max-frames   NP_DELAY=2   NP_SCENARIO=race|menu (default race)   SMOKE_OUT=<dir>
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

# same menu script as two_instances.sh (frame numbers are session polls: the restarted process starts at poll 0)
SA="1350:START,1450:START,1550:START,1800:RIGHT,1900:A,2200:A,2400:A,2600:RIGHT,2700:A"
SA+=",3000:A,3300:A,3500:A,3700:A,4000-4300:SU,4300-4400:SL,4100:A"
SB="3100:A,4000-4300:SD,4300-4500:SR"
if [ "${NP_SCENARIO:-race}" = menu ]; then SA="1350:START,1450:START,1550:START"; SB="1400:A"; fi

cd "$OUT" || exit 2
mkdir -p save_a save_b save_r
W() { cygpath -w "$1"; }
rm -f a.done b.done code.txt
# the offline lobby process ignores --record/--checksum-log/--max-frames/--input-script (they apply to the session
# process it restarts into)
timeout 900 "$EXE" --iso "$ISO" --windowed --audio-null --save-dir "$(W "$OUT/save_a")" \
    --lobby-test-script "30:connect=$SERVER/Alice,40:create=$DELAY,60:codefile=$(W "$OUT/code.txt"),80:start=2" \
    --max-frames "$FRAMES" --record a.bin --checksum-log a.txt --input-script "$SA" --np-exit-file "$(W "$OUT/a.done")" > a.log 2>&1 &
PA=$!
for i in $(seq 1 120); do [ -s code.txt ] && break; kill -0 $PA 2>/dev/null || break; sleep 0.5; done
[ -s code.txt ] || fail "host never created a room (see $OUT/a.log)"
timeout 900 "$EXE" --iso "$ISO" --windowed --audio-null --save-dir "$(W "$OUT/save_b")" \
    --lobby-test-script "30:connect=$SERVER/Bob,40:join=@$(W "$OUT/code.txt")" \
    --max-frames "$FRAMES" --record b.bin --checksum-log b.txt --input-script "$SB" --np-exit-file "$(W "$OUT/b.done")" > b.log 2>&1 &
PB=$!
# the lobby processes exit after restarting; the session processes (children, not ours to wait on) write *.done
wait $PA; wait $PB
for i in $(seq 1 1800); do [ -f a.done ] && [ -f b.done ] && break; sleep 1; done
[ -f a.done ] || fail "host session process never finished (see $OUT/a.log)"
[ -f b.done ] || fail "guest session process never finished (see $OUT/b.log)"
sleep 1

grep -aq "restarting into the session" a.log || fail "host did not restart into a session (see $OUT/a.log)"
grep -aq "restarting into the session" b.log || fail "guest did not restart into a session (see $OUT/b.log)"
grep -aq "taking the seat back" a.log && grep -aq "taking the seat back" b.log || fail "a restarted process didn't resume"
N=$((FRAMES / 60))
[ "$(wc -l < a.txt)" -ge $N ] || fail "host checksum log has $(wc -l < a.txt) lines, expected $N"
cmp -s a.txt b.txt || fail "checksum logs differ (diff $OUT/a.txt $OUT/b.txt)"
cmp -s a.bin b.bin || fail "recorded bundle streams differ ($OUT/a.bin $OUT/b.bin)"
if [ "${NP_SCENARIO:-race}" = race ]; then
  grep -aqE "^\[frame [0-9]+\] mode 4/186 " a.log || echo "note: race not reached in $FRAMES polls (use NP_FRAMES=4500 to see it)"
fi
timeout 900 "$EXE" --iso "$ISO" --windowed --audio-null --save-dir "$(W "$OUT/save_r")" \
    --replay a.bin --checksum-log r.txt > r.log 2>&1 || fail "replay failed (see $OUT/r.log)"
cmp -s a.txt r.txt || fail "replay checksum log differs from the netplay log (diff $OUT/a.txt $OUT/r.txt)"
FRAME=none
pass "2 instances started from the in-game lobby via $SERVER: $N identical checksums, identical bundle recordings, replay matches"

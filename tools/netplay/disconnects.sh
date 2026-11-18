#!/bin/bash
# E9 acceptance: disconnects, desyncs, stalls.  Local game instances against a local relay server (Windows exe),
# CLI path (--server --create/--join) with the E9 test hooks (environment variables, see docs/handoff/E9.md).
#   drop     3 instances, one dies at poll 1500 (SMB_NETPLAY_TEST_DROP): the others keep matching checksums to the end,
#            see the SAME poll where its port goes neutral, and a replay of the recorded bundle reproduces the log.
#            NP_DROP_MODE=hard (default, TerminateProcess: no goodbye, the server needs its ENet timeout of ~5-15 s,
#            the others wait meanwhile: also a watchdog test) | quit (pause menu's Quit to Desktop path) | leave
#   desync   2 instances, one sends wrong checksums (SMB_NETPLAY_TEST_DESYNC): the server ends the game, BOTH instances
#            end cleanly and go back to the lobby of the SAME room (restart into an offline process that RESUMEs; the
#            returned process writes a lobby report: room code, players, host)
#   endgame  2 instances, the host ends the game (SMB_NETPLAY_TEST_ENDGAME): both back in the lobby, room kept
#   stall    2 instances, one stalls its main loop for 12 s with the connection up (SMB_NETPLAY_TEST_STALL, ":net"):
#            the other waits > 8 s WITHOUT tripping the hang watchdog, then both finish with identical checksums
#   timeout  2 instances, one stalls 40 s: the other gives up after 30 s with a message (exit 3), the staller carries on
# Run from Git Bash on Windows.   usage: disconnects.sh [exe]
#   NP_SCENARIOS="drop desync endgame stall timeout"   which ones (default: all)
#   NP_SERVER=host:port (needs the E9 server for desync/endgame) | NP_FRAMES=3600 | NP_DELAY=2 | SMOKE_OUT=<dir>
. "$(dirname "${BASH_SOURCE[0]}")/../smoke/common.sh"
FRAMES=${NP_FRAMES:-3600}
DELAY=${NP_DELAY:-2}
SCENARIOS=${NP_SCENARIOS:-drop desync endgame stall timeout}
SERVER_PID=
# Game processes are stopped by PID: the ones that did not exist before the test AND whose window title equals the
# title of this exe's first instance (build id), so other worktrees' game processes are never touched.
game_pids() { MSYS_NO_PATHCONV=1 tasklist /v /fi "imagename eq smb2_pc_port.exe" /fo csv /nh 2>/dev/null | tr -d '\r' | sed -E 's/^"[^"]*","([0-9]+)".*,"([^"]*)"$/\1\t\2/' | grep -E '^[0-9]+' ; }
BASE_PIDS=" $(game_pids | cut -f1 | tr '\n' ' ') "
MYTITLE=
learn_title() {   # call a few seconds after the first instance started
  [ -n "$MYTITLE" ] && return
  local pid title
  while IFS=$'\t' read -r pid title; do
    case "$BASE_PIDS" in *" $pid "*) ;; *) MYTITLE=$title; return;; esac
  done < <(game_pids)
}
cleanup() {
  local pid title
  [ -n "$SERVER_PID" ] && kill $SERVER_PID 2>/dev/null
  jobs -p | xargs -r kill 2>/dev/null
  learn_title
  [ -n "$MYTITLE" ] || return
  while IFS=$'\t' read -r pid title; do
    case "$BASE_PIDS" in *" $pid "*) continue;; esac
    { [ "$title" = "$MYTITLE" ] || [ "${title#SMB2 - waiting}" != "$title" ]; } && MSYS_NO_PATHCONV=1 taskkill /F /PID "$pid" > /dev/null 2>&1
  done < <(game_pids)
}
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
echo "server: $SERVER   frames: $FRAMES   delay: $DELAY   scenarios: $SCENARIOS   out: $OUT"
W() { cygpath -w "$1"; }

# menu input like two_instances.sh's "menu" scenario: Alice reaches the main menu, the others press A now and then
SCRIPT_A="1350:START,1450:START,1550:START"
SCRIPT_B="1400:A,2000:A"
SCRIPT_C="1450:A,2100:A"

# inst <dir> <name> <role: create|join> <players> <script> <env...>   (extra game args in EXTRA, env as VAR=val words)
inst() {
  local d=$1 name=$2 role=$3 players=$4 script=$5; shift 5
  local lname=$(echo "$name" | tr 'A-Z' 'a-z')
  local args=(--iso "$ISO" --windowed --audio-null --max-frames "$FRAMES" --server "$SERVER" --name "$name"
              --save-dir "$(W "$d/save_$lname")" --record "$lname.bin" --checksum-log "$lname.txt" --input-script "$script")
  if [ "$role" = create ]; then args+=(--create --delay "$DELAY" --players "$players" --room-code-file code.txt)
  else args+=(--join "$(tr -d '\r\n' < "$d/code.txt")"); fi
  ( cd "$d" && env "$@" timeout 900 "$EXE" "${args[@]}" "${EXTRA[@]}" > "$lname.log" 2>&1 ) &
  LAST_PID=$!
  sleep 2; learn_title
}
wait_code() { for i in $(seq 1 120); do [ -s "$1/code.txt" ] && return 0; sleep 0.5; done; fail "host never created a room (see $1/a.log)"; }
# wait_files <seconds> files...
wait_files() { local t=$1; shift; local f; for i in $(seq 1 $t); do local all=1; for f in "$@"; do [ -s "$f" ] || all=0; done; [ $all = 1 ] && return 0; sleep 1; done; return 1; }
count_lines() { wc -l < "$1" | tr -d ' '; }
# poll number of every "human mask" change in a log
mask_polls() { grep -a "human mask" "$1" | sed -E 's/.*poll ([0-9]+): human mask ([0-9a-f]+) -> ([0-9a-f]+).*/\1:\2>\3/' | tr '\n' ' '; }
no_watchdog() { grep -aq "WATCHDOG" "$@" && return 1; return 0; }

scenario_drop() {
  local D=$OUT/drop MODE=${NP_DROP_MODE:-hard} spec; mkdir -p "$D"; EXTRA=()
  case $MODE in hard) spec=1500;; quit) spec=1500:quit;; leave) spec=1500:leave;; *) fail "NP_DROP_MODE=$MODE";; esac
  echo "== drop ($MODE): 3 instances, Bob dies at poll 1500"
  inst "$D" Alice create 3 "$SCRIPT_A" X=1; PA=$LAST_PID; wait_code "$D"
  inst "$D" Bob join 3 "$SCRIPT_B" SMB_NETPLAY_TEST_DROP=$spec; PB=$LAST_PID
  inst "$D" Carol join 3 "$SCRIPT_C" X=1; PC=$LAST_PID
  wait $PA; RA=$?; wait $PC; RC=$?; wait $PB; RB=$?
  cd "$D" || exit 2
  [ $RA -eq 0 ] || fail "drop: alice exited $RA (see $D/alice.log)"
  [ $RC -eq 0 ] || fail "drop: carol exited $RC (see $D/carol.log)"
  grep -aq "TEST HOOK: dropping out" bob.log || fail "drop: the hook never fired in bob (see $D/bob.log)"
  no_watchdog alice.log carol.log || fail "drop: the hang watchdog fired while waiting"
  local N=$((FRAMES / 60))
  [ "$(count_lines alice.txt)" -ge $N ] || fail "drop: alice's checksum log has $(count_lines alice.txt) lines, expected $N"
  cmp -s alice.txt carol.txt || fail "drop: checksum logs differ (diff $D/alice.txt $D/carol.txt)"
  cmp -s alice.bin carol.bin || fail "drop: recorded bundles differ"
  local ma mc; ma=$(mask_polls alice.log); mc=$(mask_polls carol.log)
  echo "   human mask changes: alice [$ma] carol [$mc]"
  [ -n "$ma" ] && [ "$ma" = "$mc" ] || fail "drop: the port went neutral at different polls (alice [$ma] carol [$mc])"
  case "$ma" in 15[0-9][0-9]:07\>05*|1[6-9][0-9][0-9]:07\>05*|[2-9][0-9][0-9][0-9]:07\>05*) ;; *) fail "drop: unexpected mask change [$ma]";; esac
  grep -aq "dropped (ports 02)" alice.log && grep -aq "dropped (ports 02)" carol.log || fail "drop: no PLAYER_DROPPED for Bob's port"
  [ "$(count_lines bob.txt)" -lt $N ] || fail "drop: bob kept running?"
  local waited; waited=$(grep -aoE "waiting for [A-Za-z, ]+ \([0-9]+\.[0-9]s\)" alice.log | tail -1)
  [ -n "$waited" ] && echo "   alice: $waited"
  mkdir -p save_r
  timeout 900 "$EXE" --iso "$ISO" --windowed --audio-null --save-dir "$(W "$D/save_r")" --replay alice.bin --checksum-log r.txt > r.log 2>&1 || fail "drop: replay failed (see $D/r.log)"
  cmp -s alice.txt r.txt || fail "drop: replay log differs from the netplay log"
  echo "   PASS drop($MODE): $N identical checksums on both survivors, port 2 neutral from the same poll, replay matches"
}

# desync / endgame: both end cleanly and are back in the room's lobby
back_to_lobby_check() {   # <name of scenario> <reason text regex>
  local D=$OUT/$1
  cd "$D" || exit 2
  wait_files 150 ret_alice.txt ret_bob.txt || fail "$1: not both back in the lobby (see $D/alice.log $D/bob.log)"
  sleep 4
  local code; code=$(tr -d '\r\n' < code.txt)
  grep -aq "$2" alice.log || fail "$1: alice has no '$2' message"
  grep -aq "$2" bob.log || fail "$1: bob has no '$2' message"
  grep -aq "restarted back into the room lobby (room $code kept)" alice.log || fail "$1: alice did not restart into the lobby"
  grep -aq "restarted back into the room lobby (room $code kept)" bob.log || fail "$1: bob did not restart into the lobby"
  grep -aq "Back in the lobby" alice.log && grep -aq "Back in the lobby" bob.log || fail "$1: no lobby message (toast) on both"
  grep -aq "back in room $code (lobby): 2 player" alice.log && grep -aq "back in room $code (lobby): 2 player" bob.log || fail "$1: not both in room $code"
  grep -aq "^room=$code" ret_alice.txt && grep -aq "^room=$code" ret_bob.txt || fail "$1: lobby report shows a different room"
  grep -aq "^count=2" ret_alice.txt && grep -aq "^count=2" ret_bob.txt || fail "$1: lobby report: not 2 players"
  grep -aq "^names=Alice,Bob" ret_alice.txt || fail "$1: lobby report: unexpected members: $(cat ret_alice.txt)"
  grep -aq "^self_is_host=1" ret_alice.txt && grep -aq "^self_is_host=0" ret_bob.txt || fail "$1: host role changed"
  grep -aq "room $code: game ended" "$OUT/server.log" 2>/dev/null || [ -n "${NP_SERVER:-}" ] || fail "$1: server did not log the game end"
  if [ -z "${NP_SERVER:-}" ]; then
    # the room may only close after both members are back (they leave when the test script exits them)
    local lc lb; lc=$(grep -an "room $code: closed" "$OUT/server.log" | head -1 | cut -d: -f1)
    lb=$(grep -an "room $code: .* is back" "$OUT/server.log" | tail -1 | cut -d: -f1)
    [ -n "$lb" ] || fail "$1: the server never saw the members come back (RESUME)"
    [ -z "$lc" ] || [ "$lc" -gt "$lb" ] || fail "$1: the server closed the room before the members were back"
  fi
}

scenario_lobby_return() {   # <desync|endgame>
  local S=$1 D=$OUT/$1; mkdir -p "$D"
  rm -f "$D"/ret_*.txt "$D"/code.txt
  EXTRA=(--np-lobby-return)
  local RS_A="10:report=$(W "$D/ret_alice.txt"),20:exit" RS_B="10:report=$(W "$D/ret_bob.txt"),20:exit"
  local envA=X=1 envB=X=1 why
  if [ $S = desync ]; then envB=SMB_NETPLAY_TEST_DESYNC=1200; why="the game ended: DESYNC"
  else envA=SMB_NETPLAY_TEST_ENDGAME=1000; why="the game ended: the host ended it"; fi
  echo "== $S: 2 instances, $([ $S = desync ] && echo 'Bob sends wrong checksums from poll 1200' || echo 'the host ends the game at poll 1000')"
  EXTRA=(--np-lobby-return --np-return-script "$RS_A"); inst "$D" Alice create 2 "$SCRIPT_A" $envA; PA=$LAST_PID; wait_code "$D"
  EXTRA=(--np-lobby-return --np-return-script "$RS_B"); inst "$D" Bob join 2 "$SCRIPT_B" $envB; PB=$LAST_PID
  wait $PA; RA=$?; wait $PB; RB=$?
  [ $RA -eq 0 ] && [ $RB -eq 0 ] || fail "$S: first processes exited $RA/$RB (see logs in $D)"
  back_to_lobby_check $S "$why"
  echo "   PASS $S: both ended cleanly ('$why') and are back in lobby of room $(tr -d '\r\n' < "$D/code.txt") ($(grep -a '^names' "$D/ret_alice.txt"))"
}

scenario_stall() {
  local D=$OUT/stall; mkdir -p "$D"; EXTRA=()
  echo "== stall: Bob freezes his main loop for 12 s at poll 800 (connection stays up); Alice must not hit the watchdog"
  inst "$D" Alice create 2 "$SCRIPT_A" X=1; PA=$LAST_PID; wait_code "$D"
  inst "$D" Bob join 2 "$SCRIPT_B" SMB_NETPLAY_TEST_STALL=800:12000:net; PB=$LAST_PID
  wait $PA; RA=$?; wait $PB; RB=$?
  cd "$D" || exit 2
  [ $RA -eq 0 ] && [ $RB -eq 0 ] || fail "stall: exit codes $RA/$RB (see $D/alice.log, bob.log)"
  no_watchdog alice.log bob.log || fail "stall: the hang watchdog fired"
  local max; max=$(grep -aoE "waiting for [A-Za-z, ]+ \(([0-9]+)\.[0-9]s\)" alice.log | sed -E 's/.*\(([0-9]+)\..*/\1/' | sort -n | tail -1)
  [ -n "$max" ] && [ "$max" -ge 9 ] || fail "stall: alice never waited more than 8 s (max ${max:-none} s): stall hook not effective?"
  local N=$((FRAMES / 60))
  [ "$(count_lines alice.txt)" -ge $N ] && cmp -s alice.txt bob.txt && cmp -s alice.bin bob.bin || fail "stall: checksum logs/bundles differ or are short"
  echo "   PASS stall: alice waited ${max}+ s (> 8 s watchdog) without a watchdog kill, $N identical checksums after the stall"
}

scenario_timeout() {
  local D=$OUT/timeout; mkdir -p "$D"; EXTRA=()
  echo "== timeout: Bob freezes for 40 s (connection up): Alice gives up after 30 s"
  inst "$D" Alice create 2 "$SCRIPT_A" X=1; PA=$LAST_PID; wait_code "$D"
  inst "$D" Bob join 2 "$SCRIPT_B" SMB_NETPLAY_TEST_STALL=800:40000:net; PB=$LAST_PID
  wait $PA; RA=$?; wait $PB; RB=$?
  cd "$D" || exit 2
  [ $RA -eq 3 ] || fail "timeout: alice exited $RA, expected 3 (see $D/alice.log)"
  grep -aq "timed out waiting for the other players" alice.log || fail "timeout: no timeout message in alice's log"
  no_watchdog alice.log bob.log || fail "timeout: the hang watchdog fired"
  [ $RB -eq 0 ] || fail "timeout: bob (the survivor) exited $RB (see $D/bob.log)"
  grep -aq "human mask 03 -> 02" bob.log || fail "timeout: bob never saw alice's port go neutral"
  echo "   PASS timeout: alice ended with the message after 30 s (exit 3, no watchdog), bob carried on alone to the end"
}

for s in $SCENARIOS; do
  case $s in
    drop) scenario_drop;;
    desync) scenario_lobby_return desync;;
    endgame) scenario_lobby_return endgame;;
    stall) scenario_stall;;
    timeout) scenario_timeout;;
    *) fail "unknown scenario $s";;
  esac
  cd "$OUT" || exit 2
done
FRAME=none
pass "disconnect/desync scenarios passed: $SCENARIOS"

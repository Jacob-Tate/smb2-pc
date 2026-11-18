#!/bin/bash
# E3: record det-session runs and replay them without any input; the checksum logs must be identical.
#   scenario 1: Monkey Race 2, 2 pads, 4500 polls (75 s)
#   scenario 2: Main Game (story) stage with player 1, name entry + intro skip, 7500 polls (125 s)
#   negative 1: a recording with one input byte changed must give a DIFFERENT log
#   negative 2: same script recorded with --session-epoch 777 must give a different log
# Replay takes the session config (epoch, locale) from the recording header and implies --det-session.
# usage: record_replay.sh [exe]   (~7 min). The Steam Controller of the dev machine is ignored via SDL env
# (a stray live pad press made the intro demo flaky): set SMB2_TEST_IGNORE_PAD="" to keep devices.
. "$(dirname "${BASH_SOURCE[0]}")/common.sh"
export SDL_GAMECONTROLLER_IGNORE_DEVICES=${SMB2_TEST_IGNORE_PAD-0x28DE/0x1304}
export SDL_JOYSTICK_IGNORE_DEVICES=$SDL_GAMECONTROLLER_IGNORE_DEVICES

RACE="1350:START,1450:START,1550:START,1600:VADDX2,1800:RIGHT,1900:A,2200:A,2400:A,2600:RIGHT,2700:A"
RACE+=",3000:A,3100:VB0,3300:A,3500:A,3700:A,4000-4300:SU,4300-4400:SL,4100:A,4000-4300:VLY-32768"
MG="1350:START,1450:START,1550:START,1800:A,1890:A,2300:A"      # title -> Main Game -> name entry
f=2400; for i in $(seq 1 10); do MG+=",$f:RIGHT"; f=$((f+20)); done
MG+=",$f:DOWN,$((f+20)):DOWN,$((f+60)):A"; f=$((f+200))          # cursor on END, confirm
for i in $(seq 1 7); do MG+=",$f:A"; f=$((f+150)); done          # Yes, ... story intro
for i in $(seq 1 6); do MG+=",$f:START"; f=$((f+150)); done      # skip the cutscene
for i in $(seq 1 8); do MG+=",$f:A"; f=$((f+200)); done          # stage select / start (stage in play ~5400)
MG+=",5600-6200:SU,6200-6800:SL,6800-7300:SR,5800-6000:A"

# rec <name> <polls> <script> [extra args]: record run into $OUT/<name>.bin, log <name>.txt
rec() {
  local n=$1 polls=$2 s=$3; shift 3
  mkdir -p "$OUT/save_$n"
  ( cd "$OUT" && timeout 900 "$EXE" --iso "$ISO" --save-dir "$(cygpath -w "$OUT/save_$n")" --windowed --audio-null \
      --det-session "$@" --max-frames "$polls" --record "$n.bin" --checksum-log "$n.rec.txt" --input-script "$s" ) \
      > "$OUT/$n.rec.log" 2>&1 || fail "record run $n failed"
}
# rep <name> <bin>: replay run, log <name>.rep.txt
rep() {
  mkdir -p "$OUT/save_r$1"
  ( cd "$OUT" && timeout 900 "$EXE" --iso "$ISO" --save-dir "$(cygpath -w "$OUT/save_r$1")" --windowed --audio-null \
      --replay "$2" --checksum-log "$1.rep.txt" ) > "$OUT/$1.rep.log" 2>&1 || fail "replay run $1 failed"
}

rec race 4500 "$RACE"
grep -aqE "^\[frame [0-9]+\] mode 4/186 " "$OUT/race.rec.log" || fail "race scenario never reached mode 4/186"
rep race race.bin
cmp -s "$OUT/race.rec.txt" "$OUT/race.rep.txt" || fail "race: checksum logs differ (diff $OUT/race.rec.txt $OUT/race.rep.txt)"
[ "$(wc -l < "$OUT/race.rec.txt")" -ge 70 ] || fail "race: checksum log too short"

rec mg 7500 "$MG"
grep -aqE "^\[frame [0-9]+\] mode 2/51 " "$OUT/mg.rec.log" || fail "main game scenario never reached mode 2/51"
rep mg mg.bin
cmp -s "$OUT/mg.rec.txt" "$OUT/mg.rep.txt" || fail "main game: checksum logs differ (diff $OUT/mg.rec.txt $OUT/mg.rep.txt)"

# negative 1: flip one stick byte (poll 3600 = in the race, port 1, stickX at byte offset 2 of the pad)
HDR=$((8+2+2+40+4+1+8+4+1+1)); OFF=$((HDR + 3600*36 + 2))
cp "$OUT/race.bin" "$OUT/race_bad.bin"
printf '\x64' | dd of="$OUT/race_bad.bin" bs=1 seek=$OFF conv=notrunc 2>/dev/null
rep racebad race_bad.bin
cmp -s "$OUT/race.rec.txt" "$OUT/racebad.rep.txt" && fail "tampered recording gave an IDENTICAL log (checksum does not detect divergence)"
# negative 2: different session epoch, same inputs
rec epoch 1500 "$RACE" --session-epoch 777
head -25 "$OUT/race.rec.txt" > "$OUT/a25.txt"; head -25 "$OUT/epoch.rec.txt" > "$OUT/b25.txt"
cmp -s "$OUT/a25.txt" "$OUT/b25.txt" && fail "different --session-epoch gave an identical log"

FRAME=-; pass "race ($(wc -l < "$OUT/race.rec.txt") lines) and main game ($(wc -l < "$OUT/mg.rec.txt") lines) replays identical; tampered input and other epoch detected"

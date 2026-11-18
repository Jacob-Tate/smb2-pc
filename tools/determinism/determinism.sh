#!/bin/bash
# E4: replay one recording many times across a matrix of render/pacing settings, in parallel, and
# compare the --checksum-log files. Every log must be byte-identical to the recording's own log.
# Git Bash script (the exe is a Windows exe; no Windows Python exists, WSL interop is not needed).
#
# usage: determinism.sh [options] [exe]
#   --out DIR         work dir (default: mktemp -d); recordings are kept in DIR/rec and reused if present
#   --rec-dir DIR     use/create recordings in DIR (race.bin/mg.bin + race.rec.txt/mg.rec.txt)
#   --scenarios "race mg"   which scenarios (default both)
#   --runs N          replays per matrix cell (default 3)
#   --jobs N          parallel runs (default 4)
#   --fps "60 144"    fps values (default "60 144"); also accepts uncapped etc.
#   --renderers "gl d3d12"
#   --variants "name=args;name2=args"  REPLACES the fps x renderer matrix with explicit cells (extra exe args)
#   --t2              extra render-independence cells: uncapped, --no-vsync, --msaa 0/8, 1280x720 window
# Exit 0 = all logs identical; 1 = mismatch (diff printed); 2 = setup error.
REPO=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
OUT=; RECDIR=; SCEN="race mg"; RUNS=3; JOBS=4; FPSL="60 144"; RENL="gl d3d12"; VARIANTS=; T2=0; EXE=
while [ $# -gt 0 ]; do
  case $1 in
    --out) OUT=$2; shift 2;; --rec-dir) RECDIR=$2; shift 2;; --scenarios) SCEN=$2; shift 2;;
    --runs) RUNS=$2; shift 2;; --jobs) JOBS=$2; shift 2;; --fps) FPSL=$2; shift 2;;
    --renderers) RENL=$2; shift 2;; --variants) VARIANTS=$2; shift 2;; --t2) T2=1; shift;;
    -*) echo "unknown option $1"; exit 2;; *) EXE=$1; shift;;
  esac
done
EXE=${EXE:-$REPO/build64/bin/smb2_pc_port.exe}
case "$EXE" in /*|[A-Za-z]:*) ;; *) EXE=$(pwd)/$EXE ;; esac
ISO=${SMB2_ISO:-C:/Users/jacob/source/super_monkey/Super Monkey Ball 2 (USA).iso}
[ -f "$EXE" ] || { echo "exe not found: $EXE"; exit 2; }
[ -f "$ISO" ] || { echo "ISO not found: $ISO (set SMB2_ISO)"; exit 2; }
OUT=${OUT:-$(mktemp -d)}; mkdir -p "$OUT/runs"; RECDIR=${RECDIR:-$OUT/rec}; mkdir -p "$RECDIR"
# The dev PC's Steam Controller injects stray input: SDL must ignore it (recording captures live pads).
export SDL_GAMECONTROLLER_IGNORE_DEVICES=${SMB2_TEST_IGNORE_PAD-0x28DE/0x1304}
export SDL_JOYSTICK_IGNORE_DEVICES=$SDL_GAMECONTROLLER_IGNORE_DEVICES

# --- scenarios (same scripts as tools/smoke/record_replay.sh) ---
RACE="1350:START,1450:START,1550:START,1600:VADDX2,1800:RIGHT,1900:A,2200:A,2400:A,2600:RIGHT,2700:A"
RACE+=",3000:A,3100:VB0,3300:A,3500:A,3700:A,4000-4300:SU,4300-4400:SL,4100:A,4000-4300:VLY-32768"
MG="1350:START,1450:START,1550:START,1800:A,1890:A,2300:A"
f=2400; for i in $(seq 1 10); do MG+=",$f:RIGHT"; f=$((f+20)); done
MG+=",$f:DOWN,$((f+20)):DOWN,$((f+60)):A"; f=$((f+200))
for i in $(seq 1 7); do MG+=",$f:A"; f=$((f+150)); done
for i in $(seq 1 6); do MG+=",$f:START"; f=$((f+150)); done
for i in $(seq 1 8); do MG+=",$f:A"; f=$((f+200)); done
MG+=",5600-6200:SU,6200-6800:SL,6800-7300:SR,5800-6000:A"
declare -A POLLS=([race]=4500 [mg]=7500) SCRIPT=([race]="$RACE" [mg]="$MG") MODE=([race]="4/186" [mg]="2/51")

# run_exe <logbase> <dir> <args...>: run in its own cwd with its own throwaway save dir
run_exe() {
  local base=$1 dir=$2; shift 2
  mkdir -p "$dir/save"
  ( cd "$dir" && timeout 1200 "$EXE" --iso "$ISO" --save-dir "$(cygpath -w "$dir/save")" --windowed --audio-null "$@" ) > "$base.log" 2>&1
}

for s in $SCEN; do
  if [ ! -s "$RECDIR/$s.bin" ] || [ ! -s "$RECDIR/$s.rec.txt" ]; then
    echo "[rec] recording $s (${POLLS[$s]} polls)"
    d=$RECDIR/work_$s; mkdir -p "$d"
    run_exe "$RECDIR/$s.rec" "$d" --det-session --max-frames "${POLLS[$s]}" --record "$(cygpath -w "$RECDIR/$s.bin")" \
        --checksum-log "$(cygpath -w "$RECDIR/$s.rec.txt")" --input-script "${SCRIPT[$s]}" || { echo "record $s failed"; exit 2; }
    grep -aqE "^\[frame [0-9]+\] mode ${MODE[$s]} " "$RECDIR/$s.rec.log" || { echo "scenario $s never reached mode ${MODE[$s]} ($RECDIR/$s.rec.log)"; exit 2; }
  else echo "[rec] reusing $RECDIR/$s.bin"; fi
done

# --- matrix ---
CELLS=()    # "name|args"
if [ -n "$VARIANTS" ]; then
  IFS=';' read -ra V <<< "$VARIANTS"; for v in "${V[@]}"; do CELLS+=("${v%%=*}|${v#*=}"); done
else
  for r in $RENL; do for fps in $FPSL; do CELLS+=("$r-fps$fps|--renderer $r --fps $fps"); done; done
fi
if [ $T2 = 1 ]; then
  for r in $RENL; do
    CELLS+=("$r-uncapped|--renderer $r --fps uncapped" "$r-novsync|--renderer $r --fps 240 --no-vsync"
            "$r-msaa0|--renderer $r --msaa 0" "$r-msaa8|--renderer $r --msaa 8"
            "$r-1280x720|--renderer $r --width 1280 --height 720")
  done
fi

N=0
for s in $SCEN; do for c in "${CELLS[@]}"; do for k in $(seq 1 "$RUNS"); do
  name=${c%%|*}; args=${c#*|}; id=$s.$name.$k; d=$OUT/runs/$id; mkdir -p "$d"
  while [ "$(jobs -rp | wc -l)" -ge "$JOBS" ]; do wait -n; done
  echo "[replay] $id"
  ( run_exe "$d/out" "$d" $args --replay "$(cygpath -w "$RECDIR/$s.bin")" --checksum-log "$(cygpath -w "$d/ck.txt")"; echo $? > "$d/rc" ) &
  N=$((N+1))
done; done; done
wait

# --- compare every log to the recording's own log ---
BAD=0
for s in $SCEN; do for c in "${CELLS[@]}"; do for k in $(seq 1 "$RUNS"); do
  name=${c%%|*}; id=$s.$name.$k; d=$OUT/runs/$id
  if [ "$(cat "$d/rc" 2>/dev/null)" != 0 ]; then echo "FAIL $id: exe exit $(cat "$d/rc" 2>/dev/null) (see $d/out.log)"; BAD=$((BAD+1))
  elif cmp -s "$RECDIR/$s.rec.txt" "$d/ck.txt"; then echo "ok   $id ($(wc -l < "$d/ck.txt") lines)"
  else echo "DIFF $id:"; diff "$RECDIR/$s.rec.txt" "$d/ck.txt" | head -6; BAD=$((BAD+1)); fi
done; done; done
echo "runs: $N, bad: $BAD, artifacts: $OUT"
[ $BAD = 0 ] && echo "PASS: all checksum logs identical" || echo "FAIL: $BAD run(s) differ"
[ $BAD = 0 ]

# Shared helpers for tools/smoke/*.sh (source me). Run from Git Bash on Windows.
#   $1            exe path (default build64/bin/smb2_pc_port.exe in this repo)
#   SMB2_ISO      disc image, Windows or Git Bash path (default: the dev machine's ISO)
#   SMOKE_OUT     output dir (default: mktemp -d); frame dumps, log.txt, save/ go here
#   SMOKE_KEEP=1  keep the output dir on PASS too
REPO=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
EXE=${1:-$REPO/build64/bin/smb2_pc_port.exe}
ISO=${SMB2_ISO:-C:/Users/jacob/source/super_monkey/Super Monkey Ball 2 (USA).iso}
[ -f "$EXE" ] || { echo "FAIL: exe not found: $EXE"; exit 2; }
[ -f "$ISO" ] || { echo "FAIL: ISO not found: $ISO (set SMB2_ISO)"; exit 2; }
OUT=${SMOKE_OUT:-$(mktemp -d)}
mkdir -p "$OUT/save"
LOG=$OUT/log.txt

# smoke_run <exit-frame> <dump-interval> <input-script>
# cwd = $OUT because the game writes frame_NNNNN.ppm into its working directory.
smoke_run() {
  local rc
  ( cd "$OUT" && timeout 600 "$EXE" --iso "$ISO" --save-dir "$(cygpath -w "$OUT/save")" \
      --windowed --audio-null --exit-frame "$1" --dump-interval "$2" --input-script "$3" ) > "$LOG" 2>&1
  rc=$?
  return $rc
}

fail() { echo "FAIL: $*"; echo "  artifacts: $OUT"; exit 1; }
pass() { echo "PASS: $*"; echo "  frame dump: $FRAME"; echo "  artifacts: $OUT"; [ "${SMOKE_KEEP:-0}" = 1 ] || true; exit 0; }

# check_frame <n>: the dump exists and is mostly lit (not black)
check_frame() {
  FRAME=$(printf '%s/frame_%05d.ppm' "$OUT" "$1")
  [ -s "$FRAME" ] || fail "no frame dump $FRAME"
  local lit total
  read -r lit total < <(grep -a "framedump\] frame $1:" "$LOG" | sed -E 's/.* ([0-9]+)\/([0-9]+) non-black.*/\1 \2/')
  [ -n "${lit:-}" ] && [ $((lit*4)) -gt "$total" ] || fail "frame $1 is mostly black (${lit:-?}/${total:-?})"
}

# has_mode <main> <sub>: log shows the heartbeat "[frame N] mode main/sub"
has_mode() { grep -aqE "^\[frame [0-9]+\] mode $1/$2 " "$LOG"; }

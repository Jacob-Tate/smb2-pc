#!/bin/bash
# Scripted-run test harness (docs/x64_work_packages.md).
#
#   modes.sh <label> <bindir> <mode...> [-- extra exe args]
#       Runs each mode on the Windows host (cwd = bindir) and stores the
#       frame dumps + log in $WS/runs/<label>/<mode>/.
#   modes.sh --compare <labelA> <labelB> [<labelC>] [mode...]
#       Per-frame ppmtool diff A vs B (and A vs C) and one PNG sheet per
#       frame in $WS/runs/sheets/<A>_vs_<B>[_<C>]/<mode>/ (A | B | [C]).
#
# Modes (input scripts below; the game is not fully deterministic past the
#        attract demo, so compare sheets visually):
#   intro menus options2 challenge howto howto2 howto_race howto_bowl
#   race racelong raceend target targetlong targetend bowling boat dogfight
#   baseball story memcard quit quit_cancel quit_opt quit_f1
#   quit_title quit_ms quit_title_no
#   ALL = every mode above.  Memory-card 32<->x64 compatibility is the
#   separate card_xarch.sh (it needs both builds).
# Typical:
#   modes.sh gl32 $WS/build32/bin intro menus -- --renderer gl
#   modes.sh gl64 $WS/build64/bin intro menus -- --renderer gl
#   modes.sh dx64 $WS/build64/bin intro menus -- --renderer d3d12
#   modes.sh --compare gl32 gl64 dx64 intro menus
#
# Any stage: SMB2_PC_STAGE=<id> with a launcher Challenge mode plays that
# stage id as floor 1 (pc_launcher.c), e.g. 9 = HOPPERS (Story 2-3):
#   SMB2_PC_STAGE=9 modes.sh gl64 $WS/build64/bin hfr_roll -- --renderer gl
# Env passed to the exe: SMB2_RENDERER, SMB2_HEAP_CHECK=1 (heap overflow
# guards, see os_shim.c).
# Safety: only ever writes inside $WS (checked), memcard is restored from
# $WS/tools/memcard_pristine after every run, save dir is per run.
#
# Setup (runs the Windows exe from WSL):
#   SMB2_ISO      your disc image as a Windows path (required for runs)
#   SMB2_X64_WS   workspace root holding pc_port/, runs/ and tools/
#                 (default: the directory containing this pc_port checkout)
#   $WS/tools/ppmtool           gcc -O2 -o $WS/tools/ppmtool pc_port/tools/ppmtool.c -lz
#   $WS/tools/memcard_pristine  a memcard/ folder to start every run from
#                               (e.g. an empty folder, or a copy of one with saves)
set -u
ALL=(intro menus options2 challenge howto2 howto_race howto_bowl race racelong raceend target targetlong targetend bowling boat dogfight baseball story memcard)
WS=${SMB2_X64_WS:-$(cd "$(dirname "$0")/../../.." && pwd)}
PPM=$WS/tools/ppmtool
PRISTINE=$WS/tools/memcard_pristine
inws() { case "$(realpath -m "$1")" in "$WS"/*) return 0;; esac; echo "refusing path outside $WS: $1" >&2; exit 1; }

mini() { # n DOWN presses on Party Games
  local n=$1 s="600:START,1200:START" f=1500 i
  for ((i=0;i<n;i++)); do s+=",$f:DOWN"; f=$((f+20)); done
  s+=",1800:A"; for ((f=1900; f<=2580; f+=40)); do s+=",$f:A"; done
  echo "$s"
}
# mode -> "launcher script dump_interval exit_frame"
spec() {
  case $1 in
    intro)     echo "1|-|100|701";;
    menus)     echo "0|1350:START,1450:START,1550:START|100|2101";;
    challenge) echo "1|600:START,1200:START,1800:A,2100:A,2400:A,2700:A,3000:A,3400:START,3500:DOWN,3560:DOWN,3620:A|200|4001";;
    # Challenge pause menu: Continue / View stage / How to play / Exit, so two
    # DOWNs pick How to play (one DOWN was View stage); RIGHT pages through.
    howto|howto2) echo "1|600:START,1200:START,1800:A,2100:A,2400:A,2700:A,3000:A,3400:START,3500:DOWN,3560:DOWN,3620:A,3900:RIGHT,4100:RIGHT,4300:RIGHT,4500:RIGHT|100|4701";;
    # Party-game pause -> How to play. Race's pause menu is Continue /
    # Retry / How to play / Exit (two DOWNs); Bowling's has no Retry (one).
    howto_race) echo "1|$(mini 4),3200:START,3300:DOWN,3350:DOWN,3400:A,3700:RIGHT,3900:RIGHT,4100:RIGHT|100|4301";;
    howto_bowl) echo "1|$(mini 6),3200:START,3300:DOWN,3400:A,3700:RIGHT,3900:RIGHT,4100:RIGHT|100|4301";;
    # Options pages
    options2)  echo "0|1350:START,1450:START,1550:START,1700:RIGHT,1760:RIGHT,1850:A,2050:DOWN,2150:RIGHT,2250:RIGHT,2350:A,2600:DOWN,2700:DOWN,2800:DOWN,2900:DOWN,3000:DOWN,3100:DOWN,3200:B,3400:LEFT,3500:A,3800:DOWN,3900:DOWN|100|4101";;
    race)      echo "1|$(mini 4)|200|3401";;
    target)    echo "1|$(mini 5)|200|3401";;
    # longer race/target runs: SU/SD = stick up/down held over a range
    racelong)  echo "1|$(mini 4),2600-6600:SU,3400:A,4200:A,5000:A,5800:A|200|6801";;
    targetlong) echo "1|$(mini 5),2600-2800:SU,2950:A,3000-3150:SD,3600-3700:SU,4600-4800:SU,4950:A,5000-5150:SD,5600-5700:SU|200|6801";;
    raceend)   echo "1|$(mini 4),2600-11800:SU|400|12401";;
    targetend) echo "1|$(mini 5),2600-2800:SU,2950:A,3200-3400:SU,3550:A,3800-4000:SU,4150:A,4400-4600:SU,4750:A,5000-5200:SU,5350:A,5600-5800:SU,5950:A,6200-6400:SU,6550:A,6800-7000:SU,7150:A,7400-7600:SU,7750:A,8000-8200:SU,8350:A,8600-8800:SU,8950:A,9200-9400:SU,9550:A,9800-10000:SU,10150:A,10400-10600:SU,10750:A,11000-11200:SU,11350:A,11600-11800:SU,11950:A|400|12401";;
    bowling)   echo "1|$(mini 6)|200|3401";;
    boat)      echo "1|$(mini 7)|200|3401";;
    dogfight)  echo "1|$(mini 8)|200|3401";;
    baseball)  echo "1|$(mini 9)|200|3401";;
    # story: real menus (START skips the attract demo at 500, before the x64
    # attract hang) -> Main Game -> Story Mode -> Data Select slot 1 -> Name
    # Entry: A types "A", START jumps to END, A confirms -> the opening
    # cutscene (scndat_SCENE1A + dlg/scn1a, mode 6, frames ~1680-7920) ->
    # World 1 stage select.
    story)     echo "0|500:START,600:START,700:START,850:A,1050:A,1250:A,1450:A,1550:START,1650:A|200|8401";;
    memcard)   echo "0|1350:START,1450:START,1550:START,1700:A,1900:A,2100:A|100|2801";;
    # Quit to Desktop (pc_quit.c): the run must end by itself with
    # "Quit to Desktop - exiting." and exit 0 well before the exit frame.
    # Challenge pause: UP wraps to the Quit row, A opens the yes/no box
    # (default No), LEFT picks Yes, A quits.
    quit)      echo "1|600:START,1200:START,1800:A,2100:A,2400:A,2700:A,3000:A,3400:START,3600:UP,3700:A,3800:LEFT,3900:A|100|4501";;
    # cancel paths: A on No, then B, back on the Quit row; START resumes
    quit_cancel) echo "1|600:START,1200:START,1800:A,2100:A,2400:A,2700:A,3000:A,3400:START,3600:UP,3700:A,3800:A,3900:A,4000:LEFT,4100:B,4200:START|100|4501";;
    # Options > Game Screen: UP wraps to the Quit row
    quit_opt)  echo "0|1350:START,1450:START,1550:START,1700:RIGHT,1760:RIGHT,1850:A,2050:DOWN,2150:RIGHT,2250:RIGHT,2350:A,2600:UP,2700:A,2800:LEFT,2900:A|100|3501";;
    # F1 overlay in a Challenge stage
    quit_f1)   echo "1|600:START,1200:START,1800:A,2100:A,2400:A,2700:A,3000:A,3400:F1,3500:UP,3600:A,3700:LEFT,3800:A|100|4501";;
    # Top-level menus (pc_quit.c pc_menu_quit_*): title screen B opens the
    # box, LEFT picks Yes, A quits (exit 0 near frame 1750).
    quit_title) echo "0|1350:START,1550:B,1650:LEFT,1750:A|100|2501";;
    # Mode Select: X opens it, RIGHT picks Yes, A quits (exit 0 near 1850)
    quit_ms)   echo "0|1350:START,1450:START,1750:X,1800:RIGHT,1850:A|100|2501";;
    # cancel paths, the run must reach the exit frame: title B, A on No; B
    # again, START closes it; START -> Mode Select; X, B closes; B -> title;
    # Esc opens, Esc closes; idle -> the attract demo ("Controls
    # description", ~frame 3800: the 100 frames with the box open hold the
    # title timer).
    quit_title_no) echo "0|1350:START,1550:B,1650:A,1750:B,1850:START,1950:START,2250:X,2350:B,2450:B,2750:ESC,2850:ESC|100|4201";;
    # HFR evidence (docs/hfr.md): Challenge Beginner 1 rolled forward at full
    # tilt (fast ball spin, banana pickups, goal, replay)
    hfr_roll)  echo "1|600:START,1200:START,1800:A,2100:A,2200-2900:SU|200|4201";;
    # roll off the side (fall-out, respawn camera cut)
    hfr_fall)  echo "1|600:START,1200:START,1800:A,2100:A,2200-2600:SR|200|3401";;
    *) echo "";;
  esac
}

run_mode() { # label bindir mode extra...
  local label=$1 bin=$2 mode=$3; shift 3
  local sp; sp=$(spec "$mode"); [ -n "$sp" ] || { echo "unknown mode $mode"; return 1; }
  IFS='|' read -r L SCRIPT DI EF <<< "$sp"
  local out=$WS/runs/$label/$mode
  inws "$out"; inws "$bin"
  mkdir -p "$out/save"
  rm -f "$out"/frame_*.ppm "$bin"/frame_*.ppm
  [ -d "$bin/memcard" ] && inws "$bin/memcard" && rm -r "$bin/memcard"
  cp -r "$PRISTINE" "$bin/memcard"
  local iso=${SMB2_ISO:?set SMB2_ISO to your disc image as a Windows path}
  local args=(--iso "$iso" --dump-interval "$DI" --exit-frame "$EF" --audio-null --save-dir "$(wslpath -w "$out/save")")
  [ "$SCRIPT" != "-" ] && args+=(--input-script "$SCRIPT")
  local try
  for try in 1 2 3; do
    ( cd "$bin" && SMB2_PC_LAUNCHER=$L WSLENV=SMB2_PC_LAUNCHER:SMB2_PC_STAGE:SMB2_RENDERER:SMB2_HEAP_CHECK:SMB2_HFR_DEBUG:SMB2_HFR_FIXED:SMB2_HFR_DEBUG_FULL:SMB2_HFR_PROFILE:SMB2_HFR_DEFER:SMB2_HFR_SWITCH:SMB2_HFR_DEBUG2 timeout 600 ./smb2_pc_port.exe "${args[@]}" "$@" > "$out/log.txt" 2>&1; echo "exit $?" >> "$out/log.txt" )
    # WSL interop sometimes fails to start the Windows process at all
    # ("UtilAcceptVsock: accept4 failed"): retry those, nothing else
    grep -aq 'main() started' "$out/log.txt" && break
    grep -aq 'UtilAcceptVsock' "$out/log.txt" || break
    echo "$label $mode: WSL interop start failure, retry $try" >&2
  done
  mv "$bin"/frame_*.ppm "$out"/ 2>/dev/null
  rm -r "$bin/memcard"; cp -r "$PRISTINE" "$bin/memcard"
  printf '%-10s %-10s frames=%-3s %s %s %s\n' "$label" "$mode" "$(ls "$out"/frame_*.ppm 2>/dev/null | wc -l)" \
    "$(grep -am1 -o '\*\*\* CRASH[^*]*\|\*\*\* WATCHDOG[^*]*' "$out/log.txt")" "$(grep -a '\[heapcheck\]' "$out/log.txt" | grep -vc enabled | sed 's/^0$//;s/^[1-9].*/heapcheck errors: &/')" "$(tail -1 "$out/log.txt")"
}

compare() { # A B [C] modes...
  local A=$1 B=$2; shift 2; local C=""
  if [ $# -gt 0 ] && [ -d "$WS/runs/$1" ]; then C=$1; shift; fi
  local modes=("$@"); [ ${#modes[@]} -eq 0 ] && modes=("${ALL[@]}")
  local sdir=$WS/runs/sheets/${A}_vs_${B}${C:+_$C}; mkdir -p "$sdir"
  for m in "${modes[@]}"; do
    mkdir -p "$sdir/$m"
    for f in "$WS/runs/$A/$m"/frame_*.ppm; do
      [ -f "$f" ] || continue
      local n; n=$(basename "$f" .ppm)
      local fb=$WS/runs/$B/$m/$n.ppm fc=$WS/runs/$C/$m/$n.ppm
      [ -f "$fb" ] || { echo "$m $n: missing in $B"; continue; }
      local line="$m $n $A/$B: $($PPM diff "$f" "$fb")"
      if [ -n "$C" ] && [ -f "$fc" ]; then
        line+="  $A/$C: $($PPM diff "$f" "$fc")"
        $PPM sheet "$sdir/$m/$n.png" 2 "$f" "$fb" "$fc" >/dev/null
      else
        $PPM sheet "$sdir/$m/$n.png" 2 "$f" "$fb" >/dev/null
      fi
      echo "$line"
    done
  done | tee "$sdir/summary.txt"
  echo "sheets: $sdir"
}

if [ "${1:-}" = "--compare" ]; then shift; compare "$@"; exit 0; fi
LABEL=$1; BIN=$2; shift 2
MODES=(); while [ $# -gt 0 ] && [ "$1" != "--" ]; do MODES+=("$1"); shift; done
[ "${1:-}" = "--" ] && shift
[ "${MODES[*]}" = "ALL" ] && MODES=("${ALL[@]}")
for m in "${MODES[@]}"; do run_mode "$LABEL" "$BIN" "$m" "$@"; done

#!/bin/bash
# scenes.sh <bindir> <outroot> <scene...> -- [extra exe args]
# scenes: intro menus challenge race target bowling boat dogfight baseball
BIN=$1; OUTROOT=$2; shift 2
SC=(); while [ $# -gt 0 ] && [ "$1" != "--" ]; do SC+=("$1"); shift; done
[ "${1:-}" = "--" ] && shift
T=$(cd "$(dirname "$0")" && pwd)   # run.sh
mini() { # downs
  local n=$1 s="600:START,1200:START" f=1500 i
  for ((i=0;i<n;i++)); do s+=",$f:DOWN"; f=$((f+20)); done
  s+=",1800:A"; for ((f=1900; f<=2580; f+=40)); do s+=",$f:A"; done
  echo "$s"
}
for sc in "${SC[@]}"; do
  case $sc in
    intro)     $T/run.sh $BIN $OUTROOT/$sc 1 "" 100 701 "$@";;
    menus)     $T/run.sh $BIN $OUTROOT/$sc 0 "1350:START,1450:START,1550:START" 100 2101 "$@";;
    challenge) $T/run.sh $BIN $OUTROOT/$sc 1 "600:START,1200:START,1800:A,2100:A,2400:A,2700:A,3000:A,3400:START,3500:DOWN,3560:DOWN,3620:A" 200 4001 "$@";;
    race)      $T/run.sh $BIN $OUTROOT/$sc 1 "$(mini 4)" 200 3401 "$@";;
    target)    $T/run.sh $BIN $OUTROOT/$sc 1 "$(mini 5)" 200 3401 "$@";;
    bowling)   $T/run.sh $BIN $OUTROOT/$sc 1 "$(mini 6)" 200 3401 "$@";;
    boat)      $T/run.sh $BIN $OUTROOT/$sc 1 "$(mini 7)" 200 3401 "$@";;
    dogfight)  $T/run.sh $BIN $OUTROOT/$sc 1 "$(mini 8)" 200 3401 "$@";;
    baseball)  $T/run.sh $BIN $OUTROOT/$sc 1 "$(mini 9)" 200 3401 "$@";;
  esac
done

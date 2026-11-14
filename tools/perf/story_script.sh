#!/bin/bash
# story_script.sh <ncycles>: real-menu Story -> W1 stage select (~8000), then
# n cycles of: (cycle>1: LEFT back to stage 1) A, AUTO to the goal, return to
# stage select, ~600 frames idle there.  Prints "<script> <exit_frame>".
n=${1:-3}
s="500:START,600:START,700:START,850:A,1050:A,1250:A,1450:A,1550:START,1650:A"
T=8700
for ((i=0;i<n;i++)); do
  [ $i -gt 0 ] && s+=",$((T-150)):LEFT"
  s+=",$T:A,$((T+200))-$((T+1000)):AUTO"
  T=$((T+2000))
done
echo "$s $((T-200))"

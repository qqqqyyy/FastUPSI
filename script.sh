#!/usr/bin/env bash
set -euo pipefail
G=("18:8" "18:10" "18:12" "20:8" "20:10" "20:12" "22:8" "22:10" "22:12") 
LOG=logs
ts(){ date '+%F %T'; }

mkdir -p -- "$LOG/tree" "$LOG/adaptive"

for P in "${G[@]}"; do
  IFS=: read -r Ne ne <<<"$P"
  n=$((1<<ne))
  d=8
  start=$(((1<<Ne)-d*(1<<ne)-1))
  echo "$(ts) [SETUP] N=2^$Ne n=2^$ne days=$d start_size=$start"
  ./frontend/setup -add_size "$n" -days "$d" -start_size "$start"

  for C in "LAN:-LAN" "WAN_200:-WAN 200" "WAN_50:-WAN 50" "WAN_5:-WAN 5"; do
    IFS=: read -r L A <<<"$C"; ./../network_setup.sh off >/dev/null 2>&1 || true
    echo "$(ts) [RUN(tree)] N=2^$Ne n=2^$ne $L"
    stdbuf -oL -eL ./frontend/main -party 0 -days $d $A >"$LOG/tree/${Ne}_${ne}_${L}.log" 2>&1 & p0=$!
    sleep 0.03
    stdbuf -oL -eL ./frontend/main -party 1 -days $d >/dev/null 2>&1 & p1=$!
    wait "$p0" "$p1"
    echo "[DONE]"
  done
done

echo "tree done"
echo ""

for P in "${G[@]}"; do
  IFS=: read -r Ne ne <<<"$P"
  n=$((1<<ne))
  d=$((1<<(Ne-ne)))
  echo "$(ts) [SETUP] N=2^$Ne n=2^$ne days=$d"
  ./frontend/setup -add_size "$n" -days "$d"

  for C in "LAN:-LAN" "WAN_200:-WAN 200" "WAN_50:-WAN 50" "WAN_5:-WAN 5"; do
    IFS=: read -r L A <<<"$C"; ./../network_setup.sh off >/dev/null 2>&1 || true
    echo "$(ts) [RUN(adaptive)] N=2^$Ne n=2^$ne $L"
    stdbuf -oL -eL ./frontend/main -party 0 -func adaptive -days $d $A >"$LOG/adaptive/${Ne}_${ne}_${L}.log" 2>&1 & p0=$!
    sleep 0.03
    stdbuf -oL -eL ./frontend/main -party 1 -func adaptive -days $d >/dev/null 2>&1 & p1=$!
    wait "$p0" "$p1"
    echo "[DONE]"
  done
done

echo "adaptive done"
echo ""

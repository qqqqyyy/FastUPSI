#!/usr/bin/env bash
set -euo pipefail
G=("20:8" "20:10" "20:12" "22:8" "22:10" "22:12") 
LOG=logs
mkdir -p "$LOG"
ts(){ date '+%F %T'; }

for P in "${G[@]}"; do
  IFS=: read -r Ne ne <<<"$P"
  n=$((1<<ne))
  d=$((1<<(Ne-ne)))
  echo "$(ts) [SETUP] N=2^$Ne n=2^$ne days=$d"
  ./frontend/setup -add_size "$n" -days "$d"

  for C in "LAN:-LAN" "WAN_200:-WAN 200" "WAN_50:-WAN 50" "WAN_5:-WAN 5"; do
    IFS=: read -r L A <<<"$C"; ./../network_setup.sh off >/dev/null 2>&1 || true
    echo "$(ts) [RUN] N=2^$Ne n=2^$ne $L  (start p0 & p1)"
    stdbuf -oL -eL ./frontend/main -party 0 -func adaptive -days $d $A >"$LOG/N${Ne}_n${ne}_${L}.log" 2>&1 & p0=$!
    sleep 0.03
    stdbuf -oL -eL ./frontend/main -party 1 -func adaptive -days $d >/dev/null 2>&1 & p1=$!
    wait "$p0" "$p1"
    echo "[DONE]"
  done
done

echo "all done"

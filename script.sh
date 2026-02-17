#!/usr/bin/env bash
set -euo pipefail
G=("18:8" "18:10" "18:12" "20:8" "20:10" "20:12" "22:8" "22:10" "22:12") 
LOG=logs
ts(){ date '+%F %T'; }

mkdir -p -- "$LOG/tree" "$LOG/okvs" "$LOG/tree-del" "$LOG/hash" "$LOG/hash-del"

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
    echo "$(ts) [RUN(okvs)] N=2^$Ne n=2^$ne $L"
    stdbuf -oL -eL ./frontend/main -party 0 -prot okvs -days $d $A >"$LOG/okvs/${Ne}_${ne}_${L}.log" 2>&1 & p0=$!
    sleep 0.03
    stdbuf -oL -eL ./frontend/main -party 1 -prot okvs -days $d >/dev/null 2>&1 & p1=$!
    wait "$p0" "$p1"
    echo "[DONE]"
  done
done

echo "okvs done"
echo ""


D=("18:4" "18:5" "18:6" "20:4" "20:5" "20:6" "22:4" "22:5" "22:6") 
for P in "${D[@]}"; do
  IFS=: read -r Ne ne <<<"$P"
  n=$((1<<ne))
  d=2
  start=$(((1<<Ne)-1))
  echo "$(ts) [SETUP] N=2^$Ne n=2^$ne days=$d start_size=$start"
  ./frontend/setup -add_size 0 -del_size "$n" -days "$d" -start_size "$start"

  for C in "LAN:-LAN" "WAN_200:-WAN 200" "WAN_50:-WAN 50" "WAN_5:-WAN 5"; do
    IFS=: read -r L A <<<"$C"; ./../network_setup.sh off >/dev/null 2>&1 || true
    echo "$(ts) [RUN(tree deletion)] N=2^$Ne n=2^$ne $L"
    stdbuf -oL -eL ./frontend/main -party 0 -del -days $d $A >"$LOG/tree-del/${Ne}_${ne}_${L}.log" 2>&1 & p0=$!
    sleep 0.03
    stdbuf -oL -eL ./frontend/main -party 1 -del -days $d >/dev/null 2>&1 & p1=$!
    wait "$p0" "$p1"
    echo "[DONE]"
  done
done

echo "tree deletion done"
echo ""

for P in "${G[@]}"; do
  IFS=: read -r Ne ne <<<"$P"
  n=$((1<<ne))
  d=$((1<<(Ne-ne)))
  echo "$(ts) [SETUP] N=2^$Ne n=2^$ne days=$d"
  ./frontend/setup -add_size "$n" -days "$d"

  for C in "LAN:-LAN" "WAN_200:-WAN 200" "WAN_50:-WAN 50" "WAN_5:-WAN 5"; do
    IFS=: read -r L A <<<"$C"; ./../network_setup.sh off >/dev/null 2>&1 || true
    echo "$(ts) [RUN(hash addition)] N=2^$Ne n=2^$ne $L"
    stdbuf -oL -eL ./frontend/main -party 0 -prot hash -days $d $A >"$LOG/hash/${Ne}_${ne}_${L}.log" 2>&1 & p0=$!
    sleep 0.03
    stdbuf -oL -eL ./frontend/main -party 1 -prot hash -days $d >"$LOG/hash/${Ne}_${ne}_${L}_p1.log" 2>&1 & p1=$!
    wait "$p0" "$p1"
    echo "[DONE]"
  done
done

echo "hash addition done"
echo ""


D=("18:4" "18:5" "18:6" "20:4" "20:5" "20:6" "22:4" "22:5" "22:6") 
for P in "${D[@]}"; do
  IFS=: read -r Ne ne <<<"$P"
  n=$((1<<ne))
  d=2
  start=$(((1<<Ne)-1))
  echo "$(ts) [SETUP] N=2^$Ne n=2^$ne days=$d start_size=$start"
  ./frontend/setup -add_size 0 -del_size "$n" -days "$d" -start_size "$start"

  for C in "LAN:-LAN" "WAN_200:-WAN 200" "WAN_50:-WAN 50" "WAN_5:-WAN 5"; do
    IFS=: read -r L A <<<"$C"; ./../network_setup.sh off >/dev/null 2>&1 || true
    echo "$(ts) [RUN(hash deletion)] N=2^$Ne n=2^$ne $L"
    stdbuf -oL -eL ./frontend/main -party 0 -prot hash -del -days $d $A >"$LOG/hash-del/${Ne}_${ne}_${L}.log" 2>&1 & p0=$!
    sleep 0.03
    stdbuf -oL -eL ./frontend/main -party 1 -prot hash -del -days $d >/dev/null 2>&1 & p1=$!
    wait "$p0" "$p1"
    echo "[DONE]"
  done
done

echo "hash deletion done"
echo ""


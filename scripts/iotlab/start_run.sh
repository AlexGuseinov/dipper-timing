#!/bin/sh
# Start one long run on an IoT-LAB node. The serial capture runs detached on the
# node's site frontend (capture.sh), so this machine can disconnect afterwards.
#   scripts/iotlab/start_run.sh <exp_id> <node>.<site> <fw path on grenoble> <tag>
# Log: <site>:~/dipper-timing/logs/<tag>_<node>.<site>_<UTC time>.log
set -eu
. "$(dirname "$0")/common.sh"
exp=$1 node=$2 fw=$3 tag=$4
host=${node%%.*}; site=${node#*.}; archi=${host%-*}; id=${host##*-}
ts=$(date -u +%Y%m%dT%H%M%SZ)
log="dipper-timing/logs/${tag}_${host}.${site}_${ts}.log"
fe grenoble "iotlab-node -i $exp --flash $fw -l $site,$archi,$id" | tr -d ' \n'; echo
fe "$site" "mkdir -p dipper-timing/logs; nohup sh dipper-timing/scripts/iotlab/capture.sh $host $log > /dev/null 2>&1 < /dev/null &"
sleep 4
fe grenoble "iotlab-node -i $exp --reset -l $site,$archi,$id" > /dev/null
echo "started: $site:~/$log"

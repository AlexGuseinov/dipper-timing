#!/bin/sh
# x86 runs: seed 1 pinned to CPU 3, then seed 2 pinned to CPU 6 (N = 10^6).
cd "$(dirname "$0")/../.."
for spec in 1:3 2:6; do
  s=${spec%%:*}; c=${spec#*:}
  ts=$(date -u +%Y%m%dT%H%M%SZ)
  harness/posix/dudect_host -s $s -n 1000000 -c 10000 -r 131072 -p $c > results/raw/x86_s${s}_kvm-cpu${c}_${ts}.log 2>&1
done

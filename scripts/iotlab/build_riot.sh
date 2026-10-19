#!/bin/sh
# Build the RIOT app on the Grenoble frontend.
#   scripts/iotlab/build_riot.sh <board> <bindir> [make vars...]
#   e.g. scripts/iotlab/build_riot.sh iotlab-m3 bin-s1 SEED=1
# RIOT checkout: ~/dipper-mcu-bench/riot/RIOT (2026.07-branch, 50ae2eba).
set -eu
. "$(dirname "$0")/common.sh"
board=$1 bindir=$2; shift 2
fe grenoble "source /opt/riot.source && cd dipper-timing/firmware/riot && \
  make BOARD=$board BINDIRBASE=$bindir RIOTBASE=\$HOME/dipper-mcu-bench/riot/RIOT $* clean >/dev/null 2>&1; make BOARD=$board BINDIRBASE=$bindir RIOTBASE=\$HOME/dipper-mcu-bench/riot/RIOT $* -j4 >/dev/null 2>&1 && \
  arm-none-eabi-size $bindir/$board/dipper-timing.elf | tail -1"

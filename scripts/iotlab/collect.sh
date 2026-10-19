#!/bin/sh
# Copy finished logs from a site frontend into results/raw/ and show progress.
#   scripts/iotlab/collect.sh <site>
set -eu
. "$(dirname "$0")/common.sh"
site=$1
mkdir -p "$REPO/results/raw"
fe "$site" 'cd dipper-timing/logs 2>/dev/null && for f in *.log; do printf "%s  lines=%s  last: " "$f" "$(wc -l < "$f")"; grep -E "#(s|res) " "$f" | tail -1 | cut -d";" -f3-; done' || true
fe "$site" 'cd dipper-timing/logs && tar czf - $(grep -l "#END" *.log 2>/dev/null)' | tar xzf - -C "$REPO/results/raw" 2>/dev/null || true
ls "$REPO/results/raw"

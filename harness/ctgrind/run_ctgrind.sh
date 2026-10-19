#!/bin/sh
# Build ct_check with the same flags as the timing harness and run memcheck for
# every variant and phase. Writes one log per run to $OUT and a summary table.
#   harness/ctgrind/run_ctgrind.sh [outdir]
set -eu
cd "$(dirname "$0")"
OUT=${1:-../../results/raw/ctgrind}
mkdir -p "$OUT"
R=../..
CC=${CC:-gcc}
$CC -O2 -g -ffp-contract=off -Wall -I$R/impl -o ct_check ct_check.c $R/impl/variants.c \
    $R/impl/dipper.c $R/impl/dipper_fast.c $R/impl/dipper_fast_const.c $R/impl/dipper_ct.c $R/impl/dipper_leaky.c
echo "# $(valgrind --version), $($CC --version | head -1), -O2 -g" > "$OUT/SUMMARY.txt"
printf "%-11s %-4s %7s %7s %7s  %s\n" variant phase errors branch addr locations >> "$OUT/SUMMARY.txt"
for v in small fast fast_const ct leaky; do
  for p in ks enc; do
    log="$OUT/${v}_${p}.log"
    valgrind --tool=memcheck --error-limit=no --track-origins=yes --error-exitcode=0 ./ct_check "$v" "$p" > "$log" 2>&1
    err=$(grep -o 'ERROR SUMMARY: [0-9]*' "$log" | awk '{print $3}')
    br=$(grep -c 'Conditional jump or move depends on uninitialised' "$log" || true)
    ad=$(grep -c 'Use of uninitialised value of size' "$log" || true)
    loc=$(grep -A1 -E 'Conditional jump|Use of uninitialised value' "$log" | grep -oE '\(([a-z_]+\.c):[0-9]+\)' | sort | uniq -c | sort -rn | awk '{printf "%s%s", sep, $2"x"$1; sep=" "}')
    printf "%-11s %-4s %7s %7s %7s  %s\n" "$v" "$p" "$err" "$br" "$ad" "$loc" >> "$OUT/SUMMARY.txt"
  done
done
cat "$OUT/SUMMARY.txt"

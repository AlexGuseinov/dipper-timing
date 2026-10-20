#!/usr/bin/env python3
"""Effect size and sensitivity from the raw dumps (first 131072 samples of
every run; damaged dump lines are skipped).

For each run: per-class mean and standard deviation (uncropped), the mean
difference d = mean(random) - mean(fixed) in timer units with a 95% interval,
and the minimum mean difference that the uncropped Welch test would flag at
|t| = 4.5 with the run's full N (half the samples per class):
    MDE = 4.5 * sqrt(s0^2/(N/2) + s1^2/(N/2)).
The cropped tests used in the verdicts are at least as sensitive for
differences in the lower part of the distribution, so MDE is an upper bound
on what the reported max |t| can detect for a pure shift.

Output: results/summary/effect_size.csv
Usage:  python3 analysis/effect_size.py
"""
import csv
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from recompute import runs  # noqa: E402
from summarize import raw_logs, platform_of, PLATFORMS  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def stats(xs):
    n = len(xs)
    if n < 2:
        return n, float("nan"), float("nan")
    m = sum(xs) / n
    v = sum((x - m) ** 2 for x in xs) / (n - 1)
    return n, m, math.sqrt(v)


def main():
    out = []
    for p in sorted(raw_logs()):
        plat = platform_of(p)
        for run in runs(p):
            raw = run["raw"]
            if not raw:
                continue
            # samples outside [cal min / 2, 100 x cal max] are impossible here
            # and come from digits lost on the serial link (damaged dumps only)
            lo, hi = 0, float("inf")
            if run["bad"] and "cal" in run:
                lo, hi = run["cal"]["min"] / 2, run["cal"]["max"] * 100
            vals = [(w >> 31, float(w & 0x7FFFFFFF)) for w in raw]
            vals = [(c, x) for c, x in vals if lo <= x <= hi]
            c0 = [x for c, x in vals if not c]
            c1 = [x for c, x in vals if c]
            n0, m0, s0 = stats(c0)
            n1, m1, s1 = stats(c1)
            d = m1 - m0
            se = math.sqrt(s0 ** 2 / n0 + s1 ** 2 / n1)
            N = int(run["run"]["n"])
            mde = 4.5 * math.sqrt(s0 ** 2 / (N / 2) + s1 ** 2 / (N / 2))
            out.append([PLATFORMS[plat][0], os.path.basename(p), run["run"]["v"], run["run"]["t"], run["run"]["seed"],
                        n0 + n1, run["bad"], "%.2f" % m0, "%.2f" % s0, "%.2f" % m1, "%.2f" % s1,
                        "%.3f" % d, "%.3f" % (d - 1.96 * se), "%.3f" % (d + 1.96 * se), N, "%.3f" % mde])
    path = os.path.join(ROOT, "results", "summary", "effect_size.csv")
    with open(path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["platform", "log", "variant", "test", "seed", "samples_used", "damaged_dump_lines",
                    "mean_fixed", "sd_fixed", "mean_random", "sd_random", "diff_random_minus_fixed",
                    "ci95_low", "ci95_high", "N_full", "mde_uncropped_at_N"])
        w.writerows(out)
    print("wrote", path, len(out), "rows")


if __name__ == "__main__":
    main()

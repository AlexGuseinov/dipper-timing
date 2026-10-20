#!/usr/bin/env python3
"""Build the summary tables from the raw logs.

For each log the first complete boot block is used (see recompute.py). Output:
  results/summary/runs.csv      one row per (log, variant, test)
  results/summary/leakage.csv   one row per (platform, variant, test): both runs + verdict
  results/summary/cost.csv      cycles per block / per key schedule, per platform and variant
  results/summary/tables.md     the same as Markdown (pasted into RESULTS.md)

Verdict rule (brief section 1): a leak is reported only if it repeats in an
independent run (new seed, other node or CPU). Thresholds: 4.5 (TVLA) and 10
(dudect). Two runs above 4.5 -> "leak, repeated"; exactly one -> "unclear";
none -> "no leak found at N". If every measurement had the same cycle count
(zero variance) the t-test is undefined; this is reported as "constant".

Usage: python3 analysis/summarize.py [results/raw/*.log]
"""
import csv
import glob
import os
import re
import sys
from collections import defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from recompute import complete_block, kv  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "results", "summary")
VARIANTS = ["small", "fast", "fast_const", "ct", "leaky"]

# platform key from the file name prefix -> (display name, memory group, cost unit)
PLATFORMS = {
    "x86": ("x86-64 (KVM guest, AMD EPYC Zen 1)", "CPU data caches", "TSC cycles"),
    "esp32": ("ESP32 NodeMCU-32S, Wi-Fi off", "flash cache", "cycles"),
    "esp32wifi": ("ESP32 NodeMCU-32S, Wi-Fi scan load", "flash cache", "cycles"),
    "m3": ("STM32F103 (IoT-LAB m3)", "no data cache", "cycles"),
    "nrf52840dk": ("nRF52840 (IoT-LAB nrf52840dk)", "no data cache (per datasheet)", "cycles"),
    "samr21": ("SAMR21G18A (IoT-LAB samr21)", "flash cache", "cycles"),
    "st-iotnode": ("STM32L475VG (IoT-LAB st-iotnode)", "flash cache", "cycles"),
    "rpi-pico": ("RP2040 (IoT-LAB rpi-pico)", "flash cache", "cycles"),
    "esp32c6": ("ESP32-C6 (IoT-LAB esp32-c6c1)", "flash cache", "cycles"),
    "lopy4": ("ESP32 rev 1 (IoT-LAB lopy4)", "flash cache", "cycles"),
    "a8": ("TI AM3505 Cortex-A8 (IoT-LAB a8, Linux)", "CPU data caches", "ns"),
}
ORDER = ["m3", "nrf52840dk", "samr21", "st-iotnode", "rpi-pico", "esp32", "esp32wifi", "lopy4", "esp32c6", "a8", "x86"]


def raw_logs():
    """results/raw/*.log, or the committed .log.xz copy when the .log is absent."""
    d = os.path.join(ROOT, "results", "raw")
    logs = {p[:-3] if p.endswith(".xz") else p for p in glob.glob(os.path.join(d, "*.log")) + glob.glob(os.path.join(d, "*.log.xz"))}
    return [p if os.path.exists(p) else p + ".xz" for p in logs]


def platform_of(name):
    b = os.path.basename(name)
    if b.startswith("esp32") and "wifion" in b:
        return "esp32wifi"
    return b.split("_")[0]


def parse(path):
    block = complete_block(path)
    if block is None:
        return None
    meta = next((l for l in block if l.startswith("#meta ")), "")
    rows, cost, cur = [], {}, None
    for l in block:
        if l.startswith("#cost "):
            d = kv(l)
            nb = int(d["blocks"])
            cost[d["v"]] = (int(d["enc_med"]) / nb, int(d["ks_med"]) / nb)
        elif l.startswith("#run "):
            cur = kv(l)
        elif l.startswith("#res "):
            d = kv(l)
            cur.update(N=int(d["N"]), maxt=float(d["maxt"]), at=int(d["at"]), t0=float(d["t0"]))
        elif l.startswith("#rng ") and cur is not None:
            d = kv(l)
            cur.update({k: int(d[k]) for k in ("min0", "max0", "min1", "max1")})
            cur["constant"] = cur["min0"] == cur["max0"] == cur["min1"] == cur["max1"]
            rows.append(cur)
            cur = None
    return meta, rows, cost


def verdict(runs):
    if all(r["constant"] for r in runs):
        return "constant (all samples identical)"
    above = sum(r["maxt"] > 4.5 for r in runs)
    if len(runs) >= 2 and above == len(runs):
        strong = all(r["maxt"] > 10 for r in runs)
        return "leak, repeated" + (" (>10 in all runs)" if strong else " (>4.5 in all runs)")
    if above >= 1:
        return "unclear"
    return "no leak found at N=%s" % "/".join(sorted({fmt_n(r["N"]) for r in runs}))


def fmt_n(n):
    e = len(str(n)) - 1
    return "10^%d" % e if n == 10 ** e else str(n)


def main():
    paths = sys.argv[1:] or sorted(raw_logs())
    os.makedirs(OUT, exist_ok=True)
    allrows, costs, groups = [], defaultdict(list), defaultdict(list)
    for p in paths:
        r = parse(p)
        if r is None:
            print("skip (no complete block):", os.path.basename(p))
            continue
        meta, rows, cost = r
        plat = platform_of(p)
        main_set = "N1e7" not in os.path.basename(p)
        for row in rows:
            row.update(platform=plat, log=os.path.basename(p), main=main_set)
            allrows.append(row)
            groups[(plat, main_set, row["v"], row["t"])].append(row)
        costs[plat].append(cost)

    with open(os.path.join(OUT, "runs.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["platform", "log", "variant", "test", "seed", "N", "max_abs_t", "test_index",
                    "t_nocrop", "min0", "max0", "min1", "max1", "constant"])
        for r in allrows:
            w.writerow([r["platform"], r["log"], r["v"], r["t"], r["seed"], r["N"], "%.4f" % r["maxt"],
                        r["at"], "%.4f" % r["t0"], r["min0"], r["max0"], r["min1"], r["max1"], r["constant"]])

    md = []
    with open(os.path.join(OUT, "leakage.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["platform", "group", "variant", "test", "runs", "N", "max_abs_t_per_run", "verdict"])
        for main_set in (True, False):
            if not main_set and not any(not k[1] for k in groups):
                continue
            md.append("\n### %s\n" % ("Main runs" if main_set else "Extra runs (x86, N = 10^7, seeds 3 and 4)"))
            md.append("| platform | test | " + " | ".join(VARIANTS) + " |")
            md.append("|---|---|" + "---|" * len(VARIANTS))
            for plat in ORDER:
                for t in "AB":
                    cells = []
                    for v in VARIANTS:
                        rs = sorted(groups.get((plat, main_set, v, t), []), key=lambda r: int(r["seed"]))
                        if not rs:
                            cells.append("–")
                            continue
                        vd = verdict(rs)
                        w.writerow([PLATFORMS[plat][0], PLATFORMS[plat][1], v, t, len(rs),
                                    "/".join(fmt_n(r["N"]) for r in rs),
                                    "/".join("%.2f" % r["maxt"] for r in rs), vd])
                        if vd.startswith("constant"):
                            cells.append("const.")
                        else:
                            ts = " / ".join("%.1f" % r["maxt"] for r in rs)
                            mark = "**" if vd.startswith("leak") else ("*" if vd == "unclear" else "")
                            cells.append("%s%s%s" % (mark, ts, mark))
                    if any(c != "–" for c in cells):
                        md.append("| %s | %s | %s |" % (PLATFORMS[plat][0], t, " | ".join(cells)))

    md.append("\nCells: max |t| per run (seed 1 / seed 2 …). **bold** = leak, repeated (> 4.5 in every run); "
              "*italic* = unclear (> 4.5 in one run only); const. = every measurement had the same cycle count.\n")

    md.append("\n### Cost (median per block; cycles, x86: TSC ticks, a8: ns via clock_gettime)\n")
    md.append("| platform | " + " | ".join("%s enc" % v for v in VARIANTS[:4]) + " | key schedule (small) | key schedule (ct) |")
    md.append("|---|" + "---|" * 6)
    with open(os.path.join(OUT, "cost.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["platform", "variant", "runs", "enc_cycles_per_block", "keysched_cycles"])
        for plat in ORDER:
            if plat not in costs:
                continue
            per = defaultdict(list)
            for c in costs[plat]:
                for v, (e, k) in c.items():
                    per[v].append((e, k))
            for v in VARIANTS:
                if per[v]:
                    es = sorted(e for e, _ in per[v]); ks = sorted(k for _, k in per[v])
                    w.writerow([PLATFORMS[plat][0], v, len(es), "%.1f" % es[len(es) // 2], "%.1f" % ks[len(ks) // 2]])
            med = lambda v, i: sorted(x[i] for x in per[v])[len(per[v]) // 2] if per[v] else float("nan")
            md.append("| %s | %s | %.0f | %.0f |" % (
                PLATFORMS[plat][0], " | ".join("%.0f" % med(v, 0) for v in VARIANTS[:4]),
                med("small", 1), med("ct", 1)))
    with open(os.path.join(OUT, "tables.md"), "w") as f:
        f.write("\n".join(md) + "\n")
    print("\n".join(md))


if __name__ == "__main__":
    main()

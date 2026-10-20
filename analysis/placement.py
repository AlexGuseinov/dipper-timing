#!/usr/bin/env python3
"""Analysis of the controlled placement experiment (results/raw/placement/).

Outputs
  results/summary/placement_runs.csv    one row per run: max |t|, effect size
  results/summary/placement_mb.csv      microbenchmark summary per board/config
  paper/tables/placement.tex            verdict table (smaller of the two runs)
  paper/tables/microbench.tex           lookup-latency table

Microbenchmark summary: for each region, the latency of a load pair (R[0],
R[d]) for growing d; the "block" is the largest power-of-two d whose latency
still equals the d = 1 value (data in the same buffer/cache line), "hit" and
"miss" are the latencies inside and outside that block. Pair-matrix check:
the fraction of (a, b) pairs whose latency is the hit value when a and b lie
in the same aligned block of that size.
Usage: python3 analysis/placement.py
"""
import csv
import glob
import lzma
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from recompute import complete_block, blocks, runs, kv  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RAW = os.path.join(ROOT, "results", "raw", "placement")
VARIANTS = ["small_flash", "small_ram", "fast_flash", "fast_ram", "leaky1", "leaky32"]
CONFIGS = [  # file prefix, display name, cache note
    ("pl-m3", "STM32F103", "none"),
    ("pl-nrf52840dk", "nRF52840", "I-cache on (default)"),
    ("pl-nrf52840dk-icacheoff", "nRF52840", "I-cache off"),
    ("pl-nrf52840dk-ramfunc", "nRF52840", "cipher code in RAM"),
    ("pl-samr21", "SAMR21", "NVM cache 64\\,B"),
    ("pl-st-iotnode", "STM32L475", "ART D-cache 256\\,B"),
    ("pl-rpi-pico", "RP2040", "XIP cache 16\\,KB"),
]


def logs():
    d = {}
    for p in glob.glob(os.path.join(RAW, "*.log")) + glob.glob(os.path.join(RAW, "*.log.xz")):
        base = p[:-3] if p.endswith(".xz") else p
        if base not in d or not p.endswith(".xz"):
            d[base] = p
    return sorted(d.values())


def prefix_seed(path):
    b = os.path.basename(path)
    pre, rest = b.split("_s", 1)
    return pre, rest.split("_", 1)[0]


def mean_sd(xs):
    n = len(xs)
    m = sum(xs) / n
    return m, math.sqrt(sum((x - m) ** 2 for x in xs) / (n - 1)) if n > 1 else 0.0


def parse_runs(path):
    out = []
    block = complete_block(path) or []
    res = {}
    for l in block:
        if l.startswith("#res "):
            d = kv(l)
            res[(d["v"], d["t"])] = d
        if l.startswith("#rng "):
            d = kv(l)
            res[(d["v"], d["t"])]["const"] = d["min0"] == d["max0"] == d["min1"] == d["max1"]
    for r in runs(path):
        key = (r["run"]["v"], r["run"]["t"])
        if key not in res:
            continue
        d = res[key]
        raw = r["raw"]
        c0 = [float(w & 0x7FFFFFFF) for w in raw if not w >> 31]
        c1 = [float(w & 0x7FFFFFFF) for w in raw if w >> 31]
        m0, s0 = mean_sd(c0) if len(c0) > 1 else (float("nan"), 0)
        m1, s1 = mean_sd(c1) if len(c1) > 1 else (float("nan"), 0)
        maxt = float("inf") if d["maxt"] in ("inf", "big") else float(d["maxt"])
        out.append(dict(variant=key[0], test=key[1], maxt=maxt, at=d["at"], const=d.get("const", False),
                        diff=m1 - m0, sd0=s0, sd1=s1, N=int(d["N"])))
    return out


def parse_mb(path):
    # the microbenchmark runs before the timing runs, so use the last boot
    # block in which it is complete (24 stride lines)
    block = []
    for b in blocks(path):
        if sum(1 for l in b if l.startswith("#mbs ")) >= 24:
            block = b
    mbs = {"flash": {}, "ram": {}}
    mbp = {"flash": {}, "ram": {}}
    for l in block:
        if l.startswith("#mbs "):
            d = kv(l)
            mbs[d["region"]][int(d["d"])] = int(d["min"])
        elif l.startswith("#mbp "):
            d = kv(l)
            mbp[d["region"]][int(d["a"])] = [int(x) for x in d["min"].split(",")]
    summ = {}
    for reg in ("flash", "ram"):
        s = mbs[reg]
        if not s:
            continue
        hit = s[1]
        block_sz = 1
        for dd in sorted(s):
            if s[dd] == hit:
                block_sz = dd * 2       # d still hits -> same block is at least 2d
            else:
                break
        miss = max(s.values())
        # pair-matrix consistency with the block size
        ok = tot = 0
        if block_sz <= 32 and mbp[reg]:
            for a, row in mbp[reg].items():
                for b, v in enumerate(row):
                    same = a // block_sz == b // block_sz
                    tot += 1
                    ok += (v == hit) == same
        summ[reg] = dict(hit=hit, miss=miss, block=block_sz if miss != hit else None,
                         matrix_agree="%d/%d" % (ok, tot) if tot else "")
    return summ


def fmt_t(x):
    if x == float("inf"):
        return "$\\infty$"
    if x >= 1000:
        s = "%d" % round(x)
        return s[:-3] + "\\," + s[-3:]
    if x >= 100:
        return "%d" % round(x)
    return "%.1f" % x


def main():
    allr, mbrows = [], []
    by = {}
    for p in logs():
        pre, seed = prefix_seed(p)
        for r in parse_runs(p):
            r.update(config=pre, seed=seed)
            allr.append(r)
            by.setdefault((pre, r["variant"], r["test"]), []).append(r)
        mb = parse_mb(p)
        for reg, v in mb.items():
            mbrows.append([pre, seed, reg, v["hit"], v["miss"], v["block"] or "", v["matrix_agree"]])
    os.makedirs(os.path.join(ROOT, "results", "summary"), exist_ok=True)
    with open(os.path.join(ROOT, "results", "summary", "placement_runs.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["config", "seed", "variant", "test", "N", "max_abs_t", "test_index", "constant",
                    "diff_random_minus_fixed_first131072", "sd_fixed", "sd_random"])
        for r in allr:
            w.writerow([r["config"], r["seed"], r["variant"], r["test"], r["N"], r["maxt"], r["at"], r["const"],
                        "%.3f" % r["diff"], "%.2f" % r["sd0"], "%.2f" % r["sd1"]])
    with open(os.path.join(ROOT, "results", "summary", "placement_mb.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["config", "seed", "region", "hit_cycles", "miss_cycles", "block_bytes", "pair_matrix_agrees"])
        w.writerows(mbrows)

    def cell(rs):
        if not rs:
            return "--"
        if all(r["const"] for r in rs):
            return "$=$"
        ts = [r["maxt"] for r in rs]
        lo, hi = min(ts), max(ts)
        d = sum(r["diff"] for r in rs) / len(rs)
        if len(ts) >= 2 and lo > 4.5:
            return "\\textbf{%s} {\\scriptsize(%+.1f)}" % (fmt_t(lo), d)
        if hi > 4.5:
            return "%s$^{\\dagger}$" % fmt_t(lo)
        return fmt_t(lo)

    for t in "AB":
        lines = ["\\begin{tabular}{ll" + "r" * 6 + "}", "\\hline",
                 "Platform & Setting & S fl & S ram & T fl & T ram & lk1 & lk32\\\\", "\\hline"]
        for pre, name, note in CONFIGS:
            cells = [cell(by.get((pre, v, t), [])) for v in VARIANTS]
            if all(c == "--" for c in cells):
                continue
            lines.append("%s & %s & %s\\\\" % (name, note, " & ".join(cells)))
        lines += ["\\hline", "\\end{tabular}"]
        os.makedirs(os.path.join(ROOT, "paper", "tables"), exist_ok=True)
        open(os.path.join(ROOT, "paper", "tables", "placement_%s.tex" % t), "w").write("\n".join(lines) + "\n")
        print("\n".join(lines))
    mbt = ["\\begin{tabular}{llrrrr}", "\\hline",
           " & & \\multicolumn{3}{c}{Flash} & RAM\\\\",
           "Platform & Setting & same block & other block & block size & any\\\\", "\\hline"]
    seen = set()
    for pre, name, note in CONFIGS:
        rows = [r for r in mbrows if r[0] == pre]
        if not rows or pre in seen:
            continue
        seen.add(pre)
        fl = [r for r in rows if r[2] == "flash"][0]
        ra = [r for r in rows if r[2] == "ram"][0]
        blk = ("%d\\,B" % fl[5]) if fl[5] else "--"
        mbt.append("%s & %s & %d & %d & %s & %d\\\\" % (name, note, fl[3], fl[4], blk, ra[3]))
    mbt += ["\\hline", "\\end{tabular}"]
    open(os.path.join(ROOT, "paper", "tables", "microbench.tex"), "w").write("\n".join(mbt) + "\n")
    print("\n".join(mbt))
    for r in mbrows:
        print(r)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""LaTeX tables for the paper, generated from the result CSVs (no hand copying).

  paper/tables/results.tex   verdict matrix: platform x variant x test
  paper/tables/cost.tex      cycles per block and code size

Usage: python3 analysis/paper_tables.py   (after summarize.py and footprint.py)
"""
import csv
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SUM = os.path.join(ROOT, "results", "summary")
OUT = os.path.join(ROOT, "paper", "tables")
VARIANTS = ["small", "fast", "fast_const", "ct", "leaky"]

# short names for the cost table
ROWS = [
    ("STM32F103 (IoT-LAB m3)", True, "STM32F103 (M3)", "none"),
    ("nRF52840 (IoT-LAB nrf52840dk)", True, "nRF52840 (M4F)", "I-cache only"),
    ("SAMR21G18A (IoT-LAB samr21)", True, "SAMR21 (M0+)", "NVM cache 64\\,B"),
    ("STM32L475VG (IoT-LAB st-iotnode)", True, "STM32L475 (M4F)", "ART, D-cache 256\\,B"),
    ("RP2040 (IoT-LAB rpi-pico)", True, "RP2040 (M0+)", "XIP cache 16\\,KB"),
    ("ESP32 NodeMCU-32S, Wi-Fi off", True, "ESP32 NodeMCU", "cache 32\\,KB"),
    ("ESP32 NodeMCU-32S, Wi-Fi scan load", True, "ESP32 NodeMCU, Wi-Fi", "cache 32\\,KB"),
    ("ESP32 rev 1 (IoT-LAB lopy4)", True, "ESP32 rev\\,1 (LoPy4)", "cache 32\\,KB"),
    ("ESP32-C6 (IoT-LAB esp32-c6c1)", True, "ESP32-C6", "cache 32\\,KB"),
    ("TI AM3505 Cortex-A8 (IoT-LAB a8, Linux)", True, "Cortex-A8 (Linux)", "L1D 16\\,KB, L2"),
    ("TI AM3505 Cortex-A8 (IoT-LAB a8, Linux)", False, "Cortex-A8, $N=10^7$", "L1D 16\\,KB, L2"),
    ("x86-64 (KVM guest, AMD EPYC Zen 1)", True, "x86-64 (KVM)", "L1D 32\\,KB, L2, L3"),
    ("x86-64 (KVM guest, AMD EPYC Zen 1)", False, "x86-64, $N=10^7$", "L1D 32\\,KB, L2, L3"),
]


GROUPS = [
    ("No data cache (vendor documentation)", [
        ("STM32F103 (IoT-LAB m3)", True, "STM32F103", "none"),
        ("nRF52840 (IoT-LAB nrf52840dk)", True, "nRF52840", "I-cache only")]),
    ("Small flash cache", [
        ("SAMR21G18A (IoT-LAB samr21)", True, "SAMR21", "64\\,B"),
        ("STM32L475VG (IoT-LAB st-iotnode)", True, "STM32L475", "256\\,B"),
        ("RP2040 (IoT-LAB rpi-pico)", True, "RP2040", "16\\,KB")]),
    ("ESP32 family (32\\,KB flash cache)", [
        ("ESP32 NodeMCU-32S, Wi-Fi off", True, "ESP32 (NodeMCU)", "32\\,KB"),
        ("ESP32 NodeMCU-32S, Wi-Fi scan load", True, "ESP32 (NodeMCU), Wi-Fi", "32\\,KB"),
        ("ESP32 rev 1 (IoT-LAB lopy4)", True, "ESP32 rev\\,1 (LoPy4)", "32\\,KB"),
        ("ESP32-C6 (IoT-LAB esp32-c6c1)", True, "ESP32-C6", "32\\,KB")]),
    ("CPU data caches", [
        ("TI AM3505 Cortex-A8 (IoT-LAB a8, Linux)", True, "Cortex-A8", "L1D 16\\,KB"),
        ("TI AM3505 Cortex-A8 (IoT-LAB a8, Linux)", False, "Cortex-A8, $N=10^7$", "L1D 16\\,KB"),
        ("x86-64 (KVM guest, AMD EPYC Zen 1)", True, "x86-64", "L1D 32\\,KB"),
        ("x86-64 (KVM guest, AMD EPYC Zen 1)", False, "x86-64, $N=10^7$", "L1D 32\\,KB")]),
]


def fmt_t(x):
    if x >= 1000:
        s = "%d" % round(x)
        return s[:-3] + "\\," + s[-3:]
    if x >= 100:
        return "%d" % round(x)
    return "%.1f" % x


def cell(runs):
    """Smaller of the two max |t| values: above 4.5 means the leak repeated."""
    if not runs:
        return "--"
    if all(r["constant"] == "True" for r in runs):
        return "$=$"
    ts = [float(r["max_abs_t"]) for r in runs]
    lo, hi = min(ts), max(ts)
    if len(ts) >= 2 and lo > 4.5:
        return "\\textbf{%s}" % fmt_t(lo)
    if hi > 4.5:
        return "%s$^{\\dagger}$" % fmt_t(lo)
    return fmt_t(lo)


def results_table():
    runs = list(csv.DictReader(open(os.path.join(SUM, "runs.csv"))))
    from summarize import PLATFORMS  # display names used in runs.csv rows
    name = {k: v[0] for k, v in PLATFORMS.items()}
    by = {}
    for r in runs:
        main = "N1e7" not in r["log"]
        by.setdefault((name[r["platform"]], main, r["variant"], r["test"]), []).append(r)
    lines = [
        "\\begin{tabular}{llrrrrr|rrrrr}",
        "\\hline",
        " & & \\multicolumn{5}{c|}{Test A: fixed vs random plaintext} & \\multicolumn{5}{c}{Test B: fixed vs random key}\\\\",
        "Platform & Cache & sm & fa & fc & ct & lk & sm & fa & fc & ct & lk\\\\",
        "\\hline",
    ]
    for gname, rows in GROUPS:
        lines.append("\\multicolumn{12}{l}{\\emph{%s}}\\\\" % gname)
        for plat, main, short, mem in rows:
            cells = [cell(by.get((plat, main, v, t), [])) for t in "AB" for v in VARIANTS]
            lines.append("%s & %s & %s\\\\" % (short, mem, " & ".join(cells)))
    lines += ["\\hline", "\\end{tabular}"]
    return "\n".join(lines) + "\n"


def cost_table():
    cost = {(r["platform"], r["variant"]): float(r["enc_cycles_per_block"])
            for r in csv.DictReader(open(os.path.join(SUM, "cost.csv")))}
    ks = {(r["platform"], r["variant"]): float(r["keysched_cycles"])
          for r in csv.DictReader(open(os.path.join(SUM, "cost.csv")))}
    foot = {(r["platform"], r["variant"]): int(r["code_bytes"])
            for r in csv.DictReader(open(os.path.join(SUM, "footprint.csv")))}
    fmap = {  # cost-table platform -> footprint-table platform
        "STM32F103 (IoT-LAB m3)": "STM32F103 (m3)", "nRF52840 (IoT-LAB nrf52840dk)": "nRF52840",
        "SAMR21G18A (IoT-LAB samr21)": "SAMR21", "STM32L475VG (IoT-LAB st-iotnode)": "STM32L475 (st-iotnode)",
        "RP2040 (IoT-LAB rpi-pico)": "RP2040", "ESP32 NodeMCU-32S, Wi-Fi off": "ESP32 (NodeMCU-32S)",
        "ESP32-C6 (IoT-LAB esp32-c6c1)": "ESP32-C6", "TI AM3505 Cortex-A8 (IoT-LAB a8, Linux)": "AM3505 Cortex-A8 (Linux)",
        "x86-64 (KVM guest, AMD EPYC Zen 1)": "x86-64 (Linux)",
    }
    short = {}
    for p, _, s, _ in ROWS:
        short.setdefault(p, s)
    lines = [
        "\\begin{tabular}{lrrrrrrrr}",
        "\\hline",
        " & \\multicolumn{4}{c}{Encryption, cycles/block} & & \\multicolumn{2}{c}{Key schedule} & \\\\",
        "Platform & small & fast & fast\\_const & ct & ct/small & small & ct & code small/ct (B)\\\\",
        "\\hline",
    ]
    for plat, f in fmap.items():
        e = [cost[(plat, v)] for v in ("small", "fast", "fast_const", "ct")]
        code = "%d / %d" % (foot[(f, "small")], foot[(f, "ct")])
        lines.append("%s & %s & %.2f & %.0f & %.0f & %s\\\\" % (
            short[plat] + ("$^{\\dagger}$" if "a8" in plat else ("$^{\\ddagger}$" if "x86" in plat else "")),
            " & ".join("%.0f" % x for x in e), e[3] / e[0],
            ks[(plat, "small")], ks[(plat, "ct")], code))
    lines += ["\\hline", "\\end{tabular}"]
    return "\n".join(lines) + "\n"


def main():
    os.makedirs(OUT, exist_ok=True)
    open(os.path.join(OUT, "results.tex"), "w").write(results_table())
    open(os.path.join(OUT, "cost.tex"), "w").write(cost_table())
    print(open(os.path.join(OUT, "results.tex")).read())
    print(open(os.path.join(OUT, "cost.tex")).read())


if __name__ == "__main__":
    main()


def effect_table():
    """Size of the detected leaks and sensitivity (main experiment, first run)."""
    rows = list(csv.DictReader(open(os.path.join(SUM, "effect_size.csv"))))
    names = [("STM32F103 (IoT-LAB m3)", "STM32F103"), ("nRF52840 (IoT-LAB nrf52840dk)", "nRF52840"),
             ("SAMR21G18A (IoT-LAB samr21)", "SAMR21"), ("STM32L475VG (IoT-LAB st-iotnode)", "STM32L475"),
             ("RP2040 (IoT-LAB rpi-pico)", "RP2040"), ("ESP32 NodeMCU-32S, Wi-Fi off", "ESP32"),
             ("ESP32-C6 (IoT-LAB esp32-c6c1)", "ESP32-C6"),
             ("TI AM3505 Cortex-A8 (IoT-LAB a8, Linux)", "Cortex-A8$^{\\dagger}$"),
             ("x86-64 (KVM guest, AMD EPYC Zen 1)", "x86-64$^{\\ddagger}$")]
    def pick(plat, v, t):
        r = [x for x in rows if x["platform"] == plat and x["variant"] == v and x["test"] == t
             and x["seed"] == "1" and "N1e7" not in x["log"]]
        return r[0] if r else None
    lines = ["\\begin{tabular}{lrrrrrr}", "\\hline",
             " & \\multicolumn{4}{c}{Mean difference random $-$ fixed, Test A} & \\multicolumn{2}{c}{Mean-shift threshold} \\\\",
             "Platform & sm & fc & ct & lk & median & range\\\\", "\\hline"]
    for plat, short in names:
        cells = []
        for v in ("small", "fast_const", "ct", "leaky"):
            r = pick(plat, v, "A")
            cells.append("%+.1f" % float(r["diff_random_minus_fixed"]) if r else "--")
        mdes = sorted(float(x["mde_uncropped_at_N"]) for x in rows if x["platform"] == plat and x["seed"] == "1"
                      and "N1e7" not in x["log"])
        med = mdes[len(mdes) // 2]
        lines.append("%s & %s & %.2f & %.2f--%.2f\\\\" % (short, " & ".join(cells), med, mdes[0], mdes[-1]))
    lines += ["\\hline", "\\end{tabular}"]
    return "\n".join(lines) + "\n"


if __name__ == "__main__":
    open(os.path.join(OUT, "effect.tex"), "w").write(effect_table())
    print(effect_table())

#!/usr/bin/env python3
"""LaTeX tables for the paper, generated from the result CSVs (no hand copying).

  paper/tables/results.tex   verdict matrix: platform x variant x test
  paper/tables/cost.tex      cycles per block and code size

Usage: python3 analysis/paper_tables.py   (after summarize.py and footprint.py)
"""
import csv
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SUM = os.path.join(ROOT, "results", "summary")
OUT = os.path.join(ROOT, "paper", "tables")
VARIANTS = ["small", "fast", "fast_const", "ct", "leaky"]

# display order and short names in the paper
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


def symbol(verdict, ts):
    if verdict.startswith("constant"):
        return "$=$"
    if verdict.startswith("leak, repeated (>10"):
        return "$\\blacksquare$"
    if verdict.startswith("leak, repeated"):
        return "$\\blacktriangle$"
    if verdict == "unclear":
        return "?"
    return "$\\circ$"


def results_table():
    rows = list(csv.DictReader(open(os.path.join(SUM, "leakage.csv"))))
    big = {}
    for r in rows:
        n = r["N"].split("/")[0]
        main = n == "10^6"
        big[(r["platform"], main, r["variant"], r["test"])] = (r["verdict"], r["max_abs_t_per_run"])
    lines = [
        "\\begin{tabular}{llccccc|ccccc}",
        "\\hline",
        " & & \\multicolumn{5}{c|}{Test A (fixed vs random plaintext)} & \\multicolumn{5}{c}{Test B (fixed vs random key)}\\\\",
        "Platform & Memory feature & sm & fa & fc & ct & lk & sm & fa & fc & ct & lk\\\\",
        "\\hline",
    ]
    for plat, main, short, mem in ROWS:
        cells = []
        for t in "AB":
            for v in VARIANTS:
                vd = big.get((plat, main, v, t))
                cells.append(symbol(*vd) if vd else "--")
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

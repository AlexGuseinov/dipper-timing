#!/usr/bin/env python3
"""Figures from the raw logs.

  figures/maxt_vs_N_test{A,B}.png  max |t| against N (status lines at N = 2^k
                                  and the final N), one panel per platform,
                                  seed 1 solid, seed 2 dashed; thresholds 4.5 and 10
  figures/cost_relative.png        encryption cost of each variant relative to
                                  `small` on the same platform (cycles/block ratio)

Colours: categorical slots 1-5 of the reference palette in fixed order, with a
distinct marker per variant as secondary encoding.
Usage: .venv/bin/python analysis/figures.py
"""
import glob
import os
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from recompute import complete_block, kv  # noqa: E402
from summarize import PLATFORMS, ORDER, platform_of, raw_logs  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FIG = os.path.join(ROOT, "figures")
VARIANTS = ["small", "fast", "fast_const", "ct", "leaky"]
COLOR = {"small": "#2a78d6", "fast": "#eb6834", "fast_const": "#1baf7a", "ct": "#eda100", "leaky": "#e87ba4"}
MARK = {"small": "o", "fast": "s", "fast_const": "^", "ct": "D", "leaky": "v"}
INK, INK2, GRID, SURF = "#0b0b0b", "#52514e", "#e4e3df", "#fcfcfb"
FLOOR = 0.05            # zero-variance runs (t = 0) are drawn on this floor line
SHORT = {"m3": "STM32F103 (m3)", "nrf52840dk": "nRF52840", "samr21": "SAMR21", "st-iotnode": "STM32L475 (st-iotnode)",
         "rpi-pico": "RP2040 (rpi-pico)", "esp32": "ESP32 NodeMCU, Wi-Fi off", "esp32wifi": "ESP32 NodeMCU, Wi-Fi on",
         "lopy4": "ESP32 rev1 (lopy4)", "esp32c6": "ESP32-C6", "a8": "Cortex-A8 (a8, Linux)", "x86": "x86-64 (KVM guest)"}

plt.rcParams.update({
    "font.size": 8, "axes.edgecolor": INK2, "axes.labelcolor": INK2, "xtick.color": INK2,
    "ytick.color": INK2, "axes.facecolor": SURF, "figure.facecolor": SURF, "savefig.facecolor": SURF,
    "axes.spines.top": False, "axes.spines.right": False, "axes.titlesize": 8, "axes.titlecolor": INK,
})


def series(path):
    """{(variant, test): [(N, maxt)]} from the status lines of the complete block."""
    out = {}
    for l in complete_block(path) or []:
        if l.startswith("#s ") or l.startswith("#res "):
            d = kv(l)
            out.setdefault((d["v"], d["t"]), []).append((int(d["N"]), float(d["maxt"])))
    return out


def main_logs():
    logs = {}
    for p in sorted(raw_logs()):
        if "N1e7" in p:
            continue
        b = os.path.basename(p)
        seed = next((s for s in ("s1", "s2", "s3") if "_%s_" % s in b), "s1")
        logs.setdefault(platform_of(p), {})[seed] = series(p)
    return logs


def fig_maxt(logs, test):
    plats = [p for p in ORDER if p in logs and any(k[1] == test for ser in logs[p].values() for k in ser)]
    cols = 4
    rows = (len(plats) + cols - 1) // cols
    fig, axes = plt.subplots(rows, cols, figsize=(7.2, 1.9 * rows + 0.6), sharex=True, sharey=True)
    axes = axes.flatten()
    for ax, plat in zip(axes, plats):
        for y, lab in ((4.5, "4.5"), (10, "10")):
            ax.axhline(y, color=INK2, lw=0.8, ls=(0, (2, 2)), zorder=1)
        for seed, ls in (("s1", "-"), ("s2", "--"), ("s3", "-")):
            ser = logs[plat].get(seed)
            if not ser:
                continue
            for v in VARIANTS:
                pts = ser.get((v, test))
                if not pts:
                    continue
                pts = [(n, t) for n, t in pts if n > 10000]     # max |t| is defined only above 10^4
                if not pts:
                    continue
                xs = [n for n, _ in pts]
                ys = [max(t, FLOOR) for _, t in pts]
                ax.plot(xs, ys, ls=ls, lw=1.4 if seed != "s2" else 1.0, color=COLOR[v], zorder=3,
                        marker=MARK[v], markersize=3 if seed != "s2" else 0, markevery=[len(xs) - 1],
                        label=v if seed in ("s1", "s3") else None)
        ax.set_xscale("log"); ax.set_yscale("log"); ax.set_xlim(1e4, 1.3e6)
        ax.set_ylim(FLOOR * 0.8, 3e5)
        ax.grid(True, which="major", color=GRID, lw=0.6, zorder=0)
        ax.set_title(SHORT.get(plat, plat), loc="left")
        const = [v for v in VARIANTS if all(t == 0 for sd in logs[plat].values() for n, t in sd.get((v, test), []) if n > 10000)
                 and any(sd.get((v, test)) for sd in logs[plat].values())]
        if const:
            ax.text(0.97, 0.05, "zero variance: " + ", ".join(const), transform=ax.transAxes, ha="right",
                    va="bottom", fontsize=6, color=INK2)
    for ax in axes[len(plats):]:
        ax.axis("off")
    for ax in axes[::cols]:
        ax.set_ylabel("max |t|")
    for ax in axes[len(plats) - cols:len(plats)]:
        ax.set_xlabel("measurements N")
    h, l = axes[0].get_legend_handles_labels()
    fig.legend(h, l, loc="upper center", ncol=5, frameon=False, bbox_to_anchor=(0.5, 1.0))
    fig.text(0.5, 0.005, "Test %s. Solid: seed 1, dashed: seed 2 (other node or CPU). Dotted lines: thresholds 4.5 and 10.\n"
             "Runs in which every sample had the same cycle count (zero variance) are drawn at %.2f." % (test, FLOOR),
             ha="center", fontsize=7, color=INK2)
    fig.tight_layout(rect=(0, 0.05, 1, 0.95))
    out = os.path.join(FIG, "maxt_vs_N_test%s.png" % test)
    fig.savefig(out, dpi=200)
    plt.close(fig)
    return out


def fig_cost():
    import csv
    rows = list(csv.DictReader(open(os.path.join(ROOT, "results", "summary", "cost.csv"))))
    plats = []
    for r in rows:
        if r["platform"] not in plats:
            plats.append(r["platform"])
    cost = {(r["platform"], r["variant"]): float(r["enc_cycles_per_block"]) for r in rows}
    vs = ["fast", "fast_const", "ct"]
    fig, ax = plt.subplots(figsize=(7.2, 0.32 * len(plats) * len(vs) / 2 + 1.2))
    bar = 0.26
    for i, v in enumerate(vs):
        ys = [k + (i - 1) * (bar + 0.02) for k in range(len(plats))]
        xs = [cost[(p, v)] / cost[(p, "small")] for p in plats]
        ax.barh(ys, xs, height=bar, color=COLOR[v], label=v, zorder=3)
        for y, x in zip(ys, xs):
            ax.text(x + 0.01, y, "%.2f" % x, va="center", fontsize=6, color=INK2)
    ax.set_yticks(range(len(plats)))
    ax.set_yticklabels([p.replace(" (IoT-LAB", "\n(IoT-LAB") for p in plats], fontsize=7)
    ax.invert_yaxis()
    ax.axvline(1.0, color=INK2, lw=0.8, ls=(0, (2, 2)))
    ax.set_xlabel("encryption time relative to small (cycles per block, median)")
    ax.grid(True, axis="x", color=GRID, lw=0.6, zorder=0)
    ax.legend(loc="lower right", frameon=False, ncol=3)
    fig.tight_layout()
    out = os.path.join(FIG, "cost_relative.png")
    fig.savefig(out, dpi=200)
    plt.close(fig)
    return out


def main():
    os.makedirs(FIG, exist_ok=True)
    logs = main_logs()
    for t in "AB":
        print(fig_maxt(logs, t))
    print(fig_cost())


if __name__ == "__main__":
    main()

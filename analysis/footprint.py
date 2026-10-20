#!/usr/bin/env python3
"""Static footprint per variant from the linked builds (nm -S of the ELFs in
results/raw/nm/) and stack use from gcc -fstack-usage (Cortex-M builds).

Code = encryption function + 128-bit key-schedule function (+ the small
encryption for leaky, which calls it). Tables = the lookup tables the
encryption and key schedule read, with their memory (flash or RAM) decided
from the address map of each chip. Round keys: 28 x 8 = 224 B RAM, all variants.

Output: results/summary/footprint.csv and footprint.md
Usage:  python3 analysis/footprint.py
"""
import csv
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NM = os.path.join(ROOT, "results", "raw", "nm")
OUT = os.path.join(ROOT, "results", "summary")

FUNCS = {
    "small": ["dipper_encrypt_block", "dipper_expand_128"],
    "fast": ["dipper_encrypt_block_fast", "dipper_expand_128"],
    "fast_const": ["dipper_encrypt_block_fastc", "dipper_expand_128"],
    "ct": ["dipper_encrypt_block_ct", "dipper_expand_128_ct"],
    "leaky": ["dipper_encrypt_block_leaky", "dipper_encrypt_block", "dipper_expand_128"],
}
# tables read by encryption + key schedule; P/PI are filled by init_tables()
# (called from dipper_expand_*), SB2I/PTI by init_fast() (called from enc_fast)
TABLES = {
    "small": ["S", "P", "PI"],
    "fast": ["SB2", "PT", "SB2I", "PTI", "S", "P", "PI"],
    "fast_const": ["SB2C", "PTC", "S", "P", "PI"],
    "ct": [],
    "leaky": ["S", "P", "PI"],
}
PLAT = {
    "riot-iotlab-m3": "STM32F103 (m3)",
    "riot-nrf52840dk": "nRF52840",
    "riot-samr21-xpro": "SAMR21",
    "riot-b-l475e-iot01a": "STM32L475 (st-iotnode)",
    "riot-rpi-pico": "RP2040",
    "esp32-nodemcu32s": "ESP32 (NodeMCU-32S)",
    "esp32c6": "ESP32-C6",
    "a8": "AM3505 Cortex-A8 (Linux)",
    "x86": "x86-64 (Linux)",
}


def region(plat, addr):
    if plat.startswith("riot-"):
        return "RAM" if 0x20000000 <= addr < 0x30000000 else "flash"
    if plat.startswith("esp32") and plat != "esp32c6":
        if 0x3F400000 <= addr < 0x3F800000:
            return "flash (DROM, cached)"
        return "RAM (DRAM)"
    if plat == "esp32c6":
        if 0x42000000 <= addr < 0x43000000:
            return "flash (cached)"
        return "RAM"
    return "process memory"


def load(path):
    syms = {}
    for line in open(path):
        m = re.match(r"([0-9a-f]+)\s+([0-9a-f]+)\s+(\w)\s+(\S+)$", line.strip())
        if m:
            a, s, t, n = int(m.group(1), 16), int(m.group(2), 16), m.group(3), m.group(4)
            syms.setdefault(n, []).append((a, s, t))
    return syms


def main():
    rows = []
    for key, name in PLAT.items():
        p = os.path.join(NM, key + ".nm")
        if not os.path.exists(p):
            continue
        syms = load(p)
        for v in FUNCS:
            code = 0
            for fn in FUNCS[v]:
                if fn in syms:
                    code += syms[fn][0][1]
            tab = {}
            for t in TABLES[v]:
                if t not in syms:
                    continue
                a, s, _ = syms[t][0]          # S exists twice (dipper.c, dipper_fast.c); same size
                r = region(key, a)
                tab[r] = tab.get(r, 0) + s
            rows.append([name, v, code, "; ".join("%s %d B" % (r, s) for r, s in sorted(tab.items())) or "none",
                         sum(tab.values())])
    os.makedirs(OUT, exist_ok=True)
    with open(os.path.join(OUT, "footprint.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["platform", "variant", "code_bytes", "tables_by_memory", "tables_bytes"])
        w.writerows(rows)
    md = ["| platform | variant | code (B) | tables | round keys |", "|---|---|---|---|---|"]
    for r in rows:
        md.append("| %s | %s | %d | %s | 224 B RAM |" % (r[0], r[1], r[2], r[3]))
    open(os.path.join(OUT, "footprint.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))


if __name__ == "__main__":
    main()

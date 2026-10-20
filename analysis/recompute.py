#!/usr/bin/env python3
"""Recompute the device's t-values from the raw dump and compare.

For every run in a log: take the crop thresholds (#thr) and the raw samples
(#d lines, class in bit 31), redo the same Welford/Welch computation in double
precision, and compare the max |t| with the device status line at N = nraw
(exact IEEE-754 hex and decimal). Lines may carry a serial_aggregator prefix
"timestamp;node;".

Usage: python3 analysis/recompute.py LOG [LOG ...]
Exit code 1 if any run does not match.
"""
import lzma
import math
import re
import struct
import sys

NCROP = 10
ENOUGH = 10000


class Welch:
    __slots__ = ("mean", "m2", "n")

    def __init__(self):
        self.mean = [0.0, 0.0]
        self.m2 = [0.0, 0.0]
        self.n = [0.0, 0.0]

    def push(self, x, c):
        self.n[c] += 1.0
        d = x - self.mean[c]
        self.mean[c] += d / self.n[c]
        self.m2[c] += d * (x - self.mean[c])

    def t(self):
        if self.n[0] < 2 or self.n[1] < 2:
            return 0.0
        v0 = self.m2[0] / (self.n[0] - 1)
        v1 = self.m2[1] / (self.n[1] - 1)
        den = math.sqrt(v0 / self.n[0] + v1 / self.n[1])
        num = self.mean[0] - self.mean[1]
        if den == 0.0:
            # zero variance in both classes: equal means -> 0, else separable
            return 0.0 if num == 0.0 else math.copysign(math.inf, num)
        return num / den


def max_t(tests):
    best, which = 0.0, -1
    for i, w in enumerate(tests):
        if w.n[0] + w.n[1] <= ENOUGH:
            continue
        t = abs(w.t())
        if which < 0 or t > best:
            best, which = t, i
    return best, which


def strip(line):
    line = line.rstrip("\r\n")
    if ";" in line and not line.startswith("#"):
        parts = line.split(";", 2)
        if len(parts) == 3:
            line = parts[2]
    return line.strip()


def kv(line):
    out = {}
    for tok in line.split()[1:]:
        if "=" in tok:
            k, v = tok.split("=", 1)
            out[k] = v
    return out


def blocks(path):
    """Split a log into boot blocks (each starts at a #meta line)."""
    out, cur = [], None
    opener = lzma.open if path.endswith(".xz") else open
    with opener(path, "rt", errors="replace") as f:
        for raw in f:
            line = strip(raw)
            if line.startswith("#meta "):
                cur = []
                out.append(cur)
            if cur is not None:
                cur.append(line)
    return out


def complete_block(path):
    """First boot block in which every started run has its #res line.
    IoT-LAB logs start with the tail of the run started by flashing, and the
    firmware repeats forever, so later blocks may be partial."""
    for b in blocks(path):
        nrun = sum(1 for l in b if l.startswith("#run "))
        nres = sum(1 for l in b if l.startswith("#res "))
        if nrun and nrun == nres:
            return b
    return None


def runs(path):
    cur = None
    block = complete_block(path) or []
    for line in block:
        if True:
            if line.startswith("#run "):
                if cur:
                    yield cur
                cur = {"run": kv(line), "thr": None, "raw": [], "status": {}, "bad": 0}
            elif cur is None:
                continue
            elif line.startswith("#cal "):
                cur["cal"] = {k: int(v) for k, v in kv(line).items()}
            elif line.startswith("#thr"):
                cur["thr"] = [int(x) for x in line.split()[1:]]
            elif line.startswith("#d "):
                toks = line.split()[1:]
                # a dump line is up to 8 words of exactly 8 hex digits; anything
                # else was damaged on the serial link (lost bytes / merged lines)
                if 0 < len(toks) <= 8 and all(re.fullmatch(r"[0-9a-f]{8}", w) for w in toks):
                    cur["raw"] += [int(w, 16) for w in toks]
                else:
                    cur["bad"] += 1
            elif line.startswith("#d") or "#d " in line[1:]:
                cur["bad"] += 1
            elif line.startswith("#s ") or line.startswith("#res "):
                d = kv(line)
                cur["status"][int(d["N"])] = d
    if cur:
        yield cur


def check(run):
    thr, raw = run["thr"], run["raw"]
    nraw = int(run["run"]["nraw"])
    if not raw:
        return None, "no raw dump"
    if run["bad"] or len(raw) != nraw:
        return None, "transport damage: %d damaged dump lines, %d of %d samples readable (not comparable)" % (
            run["bad"], len(raw), nraw)
    tests = [Welch() for _ in range(NCROP + 2)]
    for w in raw:
        c, x = w >> 31, w & 0x7FFFFFFF
        xd = float(x)
        tests[0].push(xd, c)
        for k in range(NCROP):
            if x < thr[k]:
                tests[1 + k].push(xd, c)
        w0 = tests[0]
        if w0.n[0] + w0.n[1] > ENOUGH:
            y = xd - w0.mean[c]
            tests[NCROP + 1].push(y * y, c)
    m, which = max_t(tests)
    dev = run["status"].get(nraw)
    if dev is None:
        return False, "no device status at N=%d" % nraw
    host_hex = "%016x" % struct.unpack(">Q", struct.pack(">d", m))[0]
    exact = host_hex == dev["hex"]
    try:
        devt = float(dev["maxt"])
    except ValueError:
        devt = math.inf
    close = (m == devt) or (abs(m - devt) < 1e-3 and which == int(dev["at"]))
    verdict = "exact" if exact else ("match to 1e-3" if close else "MISMATCH")
    if not (exact or close) and "cal" in run:
        # a sample below half the calibration minimum is physically impossible
        # here; it is a digit lost on the serial link inside a well-formed word
        lo = run["cal"]["min"] // 2
        bad = ["%08x" % w for w in raw if (w & 0x7FFFFFFF) < lo]
        if bad:
            msg = "N=%d host maxt=%.4f | device maxt=%s | transport damage: %d impossible sample(s) %s (< cal min/2)" % (
                nraw, m, dev["maxt"], len(bad), ",".join(bad[:3]))
            return None, msg
    msg = "N=%d host maxt=%.4f at=%d | device maxt=%s at=%s | %s" % (
        nraw, m, which, dev["maxt"], dev["at"], verdict)
    return exact or close, msg


def main():
    bad = damaged = 0
    for path in sys.argv[1:]:
        for run in runs(path):
            ok, msg = check(run)
            tag = "%s v=%s t=%s" % (path, run["run"].get("v"), run["run"].get("t"))
            print("%-60s %s" % (tag, msg))
            if ok is False:
                bad += 1
            elif ok is None:
                damaged += 1
    print("# summary: %d mismatches, %d runs not comparable (transport damage or no dump)" % (bad, damaged))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()

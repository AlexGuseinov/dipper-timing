#!/usr/bin/env python3
"""List the branches, calls and memory accesses of the ct functions in the
disassembly files of results/raw/asm/ (made from the real build ELFs).

This is an aid for the manual review in docs/ct_asm_review.md, not a proof:
it prints every control-flow instruction and every load/store with its
addressing operand, per ISA and function, so that each one can be checked
to depend only on public values (loop counters, the round-key pointer, the
stack pointer).

Usage: python3 analysis/ct_asm_scan.py results/raw/asm/*.txt
"""
import re
import sys
from collections import Counter

# mnemonic patterns per ISA family
BRANCH = {
    "x86": re.compile(r"^(j[a-z]+|call|ret|loop\w*|cmov\w+)$"),
    "arm": re.compile(r"^(?!bic)(b[a-z]*(\.[nw])?|cb[n]?z|bl|blx|bx|tb[bh]|pop|it[te]*)$"),
    "xtensa": re.compile(r"^(b[a-z0-9]*(\.n)?|j|jx|call\w*|callx\w*|ret\w*(\.n)?|loop\w*|mov(eqz|nez|ltz|gez)(\.n)?)$"),
    "riscv": re.compile(r"^(b[a-z]+z?|j|jal|jalr|jr|ret|c\.(b\w+|j\w*))$"),
}
MEM = {
    "x86": re.compile(r"\("),
    "arm": re.compile(r"^(ldr\w*|str\w*|ldm\w*|stm\w*|push|pop|vldr|vstr)"),
    "xtensa": re.compile(r"^(l\d+\w*|s\d+\w*|l32r|l32i\w*|s32i\w*|l16\w+|l8\w+|s16\w+|s8\w+)(\.n)?$"),
    "riscv": re.compile(r"^(c\.)?(l[bhwd]u?|s[bhwd]|lw|sw|lwsp|swsp)$"),
}


def isa_of(path):
    p = path.lower()
    for k, v in (("x86", "x86"), ("thumb", "arm"), ("armv7", "arm"), ("xtensa", "xtensa"), ("riscv", "riscv")):
        if k in p:
            return v
    raise SystemExit("unknown ISA for " + path)


def scan(path):
    isa = isa_of(path)
    sec, out = None, {}
    for line in open(path):
        if line.startswith("### "):
            sec = line[4:].strip()
            out[sec] = {"n": 0, "branch": Counter(), "mem": Counter()}
            continue
        m = re.match(r"\s*[0-9a-f]+:\s+(\S+)\s*(.*)", line)
        if not m or sec is None:
            continue
        mn, ops = m.group(1), m.group(2).split("#")[0].split("<")[0].strip()
        out[sec]["n"] += 1
        if BRANCH[isa].match(mn):
            out[sec]["branch"][mn] += 1
        if (isa == "x86" and MEM["x86"].search(ops) and not mn.startswith("nop") and not mn.startswith("lea")) \
                or (isa != "x86" and MEM[isa].match(mn)):
            # addressing operand only (base register + offset form)
            addr = re.findall(r"\[[^\]]*\]|\([^)]*\)|-?0x[0-9a-f]+\(\w+\)|\d+\(\w+\)", ops)
            if isa == "xtensa":
                parts = [x.strip() for x in ops.split(",")]
                addr = [",".join(parts[1:])] if len(parts) > 1 else [ops]
            out[sec]["mem"][mn + " " + (" ".join(addr) if addr else ops)] += 1
    return isa, out


def main():
    for path in sys.argv[1:]:
        isa, res = scan(path)
        print("=" * 78)
        print(path, "(%s)" % isa)
        for sec, r in res.items():
            print("--- %s: %d instructions" % (sec, r["n"]))
            print("    control flow:", dict(r["branch"]) or "none")
            print("    memory accesses (%d):" % sum(r["mem"].values()))
            for k, c in sorted(r["mem"].items()):
                print("      %3d  %s" % (c, k))


if __name__ == "__main__":
    main()

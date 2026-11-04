#!/usr/bin/env python3
"""checksize.py -- region budgets and the RAM ledger, from the listing.

mule.asm brackets every module with an MB_* fence label. A region is the
fixed part at 8000H, page 0, the fixed part at E000H, or a page n >= 1
(each PHASEd to C000H); its fences are the MB_ labels whose names start
with the region's prefix, in address order, closed by its END fence.

RAM: ram.inc allocates variables upward from RAMBASE to RAMEND (raised by
the pages' phase variables); the stack owns STKLOW..STACK. RAMEND > STKLOW
fails the build. So does a region past its budget, or a duplicated label
(zmac has one namespace and doesn't always say).

Usage: checksize.py build/mule.lst
"""
import collections
import glob
import os
import re
import sys

WARN_SPARE = 100
REGIONS = [
    ("fixed 8000H", 0x4000, "MB_F"),
    ("page 0", 0x2000, "MB_P0"),
    ("fixed E000H", 0x1800, "MB_G"),
    ("page 1", 0x2000, "MB_P1"),
    ("page 2", 0x2000, "MB_P2"),
    ("page 3", 0x2000, "MB_P3"),
]


def read_symbols(path):
    syms = {}
    text = open(path, encoding="utf-8", errors="replace").read()
    tail = text[text.rfind("Symbol Table:"):]
    for name, val in re.findall(r"\b([a-z_][a-z0-9_]*)\s+=?\s*([0-9a-f]{1,5})\+?", tail):
        syms.setdefault(name.upper(), int(val, 16))
    return syms


def main():
    syms = read_symbols(sys.argv[1])
    bad = False
    print("checksize: region budgets")
    for name, budget, prefix in REGIONS:
        end = syms.get(prefix + "END")
        start = syms.get(prefix)
        if end is None or start is None:
            continue
        fences = sorted((v, k) for k, v in syms.items()
                        if k.startswith(prefix) and k != prefix + "END"
                        and re.fullmatch(prefix + r"(_[A-Z0-9]+)?", k))
        fences.append((end, prefix + "END"))
        for (a, k), (b, _) in zip(fences, fences[1:]):
            label = k[len(prefix) + 1:] or "(head)"
            if b - a:
                print("  %-12s %-8s %5d bytes" % (name, label, b - a))
        used = end - start
        spare = budget - used
        print("  %-12s total    %5d of %d (%d spare)" % (name, used, budget, spare))
        if spare < 0:
            print("checksize: %s is %d bytes over" % (name, -spare))
            bad = True
        elif spare < WARN_SPARE:
            print("checksize: warning: %s has only %d bytes spare" % (name, spare))
    ramend, stklow, base = syms["RAMEND"], syms["STKLOW"], syms["RAMBASE"]
    print("  RAM          variables %d of %d (%d spare), stack %d" % (
        ramend - base, stklow - base, stklow - ramend, syms["STACK"] - stklow))
    if ramend > stklow:
        print("checksize: the variables run %d bytes into the stack" % (ramend - stklow))
        bad = True
    here = os.path.dirname(os.path.abspath(sys.argv[1]))
    labels = collections.defaultdict(list)
    for f in sorted(glob.glob(os.path.join(here, "..", "*.inc")) + glob.glob(os.path.join(here, "..", "*.asm"))):
        for i, line in enumerate(open(f)):
            m = re.match(r"^([A-Za-z_][A-Za-z0-9_]*):", line)
            if m:
                labels[m.group(1).upper()].append("%s:%d" % (os.path.basename(f), i + 1))
    for k, v in labels.items():
        if len(v) > 1:
            print("checksize: label %s defined at %s" % (k, ", ".join(v)))
            bad = True
    if bad:
        sys.exit(1)


if __name__ == "__main__":
    main()

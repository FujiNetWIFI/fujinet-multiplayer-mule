#!/usr/bin/env python3
"""checksize.py -- per-region ROM budgets and the RAM ledger, from the listing.

mule.asm brackets every module with an MB_* fence label. A region is a page
(or the resident half); its fences are the MB_ labels whose names start with
the region's prefix, in address order, closed by the region's END fence.
Every page is PHASEd to 2000H, so each region is budgeted on its own.

RAM: ram.inc allocates variables upward from the end of the visible screen
(RAMBASE) to RAMEND; the stack owns STKLOW..STACK. RAMEND > STKLOW fails.

Usage: checksize.py build/mule.lst
"""

import re
import sys

WARN_SPARE = 100

# (name, budget, fence prefix). Fences named <prefix>... belong to the
# region; <prefix>END closes it.
REGIONS = [
    ("page 0",   4096, "MB_P0"),
    ("resident", 2816, "MB_R"),
    ("page 2",   4096, "MB_P2"),
    ("page 3",   4096, "MB_P3"),
    ("page 4",   4096, "MB_P4"),
    ("page 5",   4096, "MB_P5"),
    ("page 6",   4096, "MB_P6"),
    ("page 7",   4096, "MB_P7"),
    ("page 8",   4096, "MB_P8"),
    ("page 9",   4096, "MB_P9"),
    ("page 10",  4096, "MB_P10"),
    ("page 11",  4096, "MB_P11"),
    ("page 12",  4096, "MB_P12"),
]


def read_symbols(path):
    syms = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        text = f.read()
    tail = text[text.rfind("Symbol Table:"):]
    for name, val in re.findall(r"\b([a-z_][a-z0-9_]*)\s+=?\s*([0-9a-f]{1,5})\+?",
                                tail):
        syms[name.upper()] = int(val, 16)
    return syms


def main():
    if len(sys.argv) != 2:
        print(__doc__.strip(), file=sys.stderr)
        return 2
    syms = read_symbols(sys.argv[1])
    bad = 0
    print("checksize: region budgets")
    for region, budget, pre in REGIONS:
        fences = sorted((a, n) for n, a in syms.items()
                        if n.startswith(pre + "_") or n == pre + "END"
                        or n == pre)
        if len(fences) < 2:
            continue
        for (addr, name), (nxt, _) in zip(fences, fences[1:]):
            if nxt - addr:
                print(f"  {region:<9} {name[len(pre):].lstrip('_') or '(head)':<10}"
                      f" {nxt - addr:5} bytes")
        used = fences[-1][0] - fences[0][0]
        spare = budget - used
        print(f"  {region:<9} {'total':<10} {used:5} of {budget} ({spare} spare)")
        if spare < 0:
            print(f"checksize: {region} is {-spare} bytes over", file=sys.stderr)
            bad = 1
        elif spare < WARN_SPARE and region == "resident":
            print(f"checksize: {region} headroom under {WARN_SPARE} bytes",
                  file=sys.stderr)
    if "RAMEND" in syms and "STKLOW" in syms:
        base = syms.get("RAMBASE", 0)
        used = syms["RAMEND"] - base
        room = syms["STKLOW"] - base
        print(f"  RAM       variables  {used:5} of {room} "
              f"({room - used} spare), stack {syms.get('STACK', 0) - syms['STKLOW']}")
        if used > room:
            print(f"checksize: RAM is {used - room} bytes over", file=sys.stderr)
            bad = 1
    return bad


if __name__ == "__main__":
    sys.exit(main())

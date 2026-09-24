#!/usr/bin/env python3
"""checkcalls.py -- no page calls straight into another page.

Every page is PHASEd to 2000H, so a CALL/JP/JR from page A to a label of
page B assembles cleanly and lands on whatever page A has at that address.
This walks the zmac listing in order, notes which region (MB_P<n> fence /
the resident MB_R) each label and each jump belongs to, and fails the
build on a jump from one page into another. Resident code may be called
from anywhere; resident code calling a page label is allowed only for the
phase entries (PE_*), which are EQUs, not labels. Far calls go through
FARCALL (DB page / DW label), which is not a jump.

Usage: checkcalls.py build/mule.lst
"""
import re
import sys

SRC = re.compile(r"^\s*\d+:\s*(?:[0-9a-fA-F]{4}\s+[0-9a-fA-F]*\s*)?\t(.*)$")


def region_of_fence(name):
    m = re.match(r"MB_P(\d+)", name)
    if m:
        return "P" + m.group(1)
    if name.startswith("MB_R"):
        return "R"
    return None


def main():
    lines = open(sys.argv[1], encoding="utf-8", errors="replace").read().split("\n")
    end = next((i for i, l in enumerate(lines) if l.startswith("Symbol Table:")), len(lines))
    region = "P0"
    labels = {}
    jumps = []
    for i, raw in enumerate(lines[:end]):
        m = SRC.match(raw)
        if not m:
            continue
        text = m.group(1).split(";")[0]
        lm = re.match(r"^([A-Za-z_][A-Za-z0-9_]*):", text)
        if lm:
            name = lm.group(1).upper()
            r = region_of_fence(name)
            if r and not name.endswith("END"):
                region = r
            labels.setdefault(name, region)
            text = text[lm.end():]
        jm = re.match(r"^\s*(CALL|JP|JR|DJNZ)\s+(?:(?:NZ|Z|NC|C|PO|PE|P|M)\s*,\s*)?([A-Za-z_][A-Za-z0-9_]*)\s*$",
                      text, re.I)
        if jm:
            jumps.append((i + 1, region, jm.group(2).upper(), raw.strip()))
    bad = 0
    for ln, reg, tgt, raw in jumps:
        t = labels.get(tgt)
        if t is None or t == "R" or t == reg or tgt in ("PRGSTR",):
            continue
        print(f"checkcalls: listing line {ln}: {reg} jumps into {t}'s {tgt}: {raw}",
              file=sys.stderr)
        bad = 1
    if not bad:
        print(f"checkcalls: {len(jumps)} jumps, none across pages")
    return bad


if __name__ == "__main__":
    sys.exit(main())

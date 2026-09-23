#!/usr/bin/env python3
"""Fail if the ROM would land where it mustn't:
  $7000-$7FFF (the ECS), $8000-$9FFF (the FujiNet cart RAM and mailbox),
  $B800-$BFFF and $F800-$FFFF (GRAM shows through there on some consoles).
usage: checkcfg.py mule.cfg"""
import re
import sys

BAD = [(0x7000, 0x7FFF), (0x8000, 0x9FFF), (0xB800, 0xBFFF), (0xF800, 0xFFFF)]
bad = 0
for ln in open(sys.argv[1]):
    m = re.match(r"\s*\$([0-9A-Fa-f]+)\s*-\s*\$([0-9A-Fa-f]+)\s*=\s*\$([0-9A-Fa-f]+)", ln)
    if not m:
        continue
    lo, hi = int(m.group(3), 16), int(m.group(3), 16) + int(m.group(2), 16) - int(m.group(1), 16)
    for a, b in BAD:
        if lo <= b and hi >= a:
            print("ROM at $%04X-$%04X overlaps $%04X-$%04X" % (lo, hi, a, b))
            bad += 1
sys.exit(1 if bad else 0)

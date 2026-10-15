#!/usr/bin/env python3
"""checksize.py -- the image's budgets, from the AS listing's symbols.

  fixed    MB_FIX..MB_FIXEND   must end at or before the claim, $47FC
  pages    MB_Pn..MB_PnEND     8K each, in the window $4800-$67FF
  data     MB_P5..MB_P5END     8K, copied to RAM at DATA ($A000)
  RAM      RAMEND <= RAMTOP    (nothing past $F000: DC0 runs into the mailbox)

Each module's share of the fixed block is listed (the MB_ labels in order).
usage: checksize.py build/mule.lst
"""
import re
import sys

text = open(sys.argv[1], encoding="latin-1").read()
syms = {}
for name, val in re.findall(r"([A-Z_][A-Z0-9_]*)\s*:\s*([0-9A-F]+)\s+\S\s*\|", text):
    syms.setdefault(name, int(val, 16))

errs = []
mods = sorted((v, k) for k, v in syms.items() if k.startswith("MB_") and not re.match(r"MB_P\d", k))
fix0, fix1 = syms["MB_FIX"], syms["MB_FIXEND"]
print("fixed:   $%04X-$%04X  %5d of %5d  (%d free)" % (fix0, fix1, fix1 - fix0, 0x47FC - fix0, 0x47FC - fix1))
if fix1 > 0x47FC:
    errs.append("the fixed block runs past $47FC")
fixmods = [(v, k) for v, k in mods if fix0 <= v <= fix1]
for (v, k), (v2, _) in zip(fixmods, fixmods[1:] + [(fix1, "")]):
    if k not in ("MB_FIX", "MB_FIXEND"):
        print("  %-12s %5d" % (k[3:].lower(), v2 - v))
for n in range(5):
    a, b = syms.get("MB_P%d" % n), syms.get("MB_P%dEND" % n)
    if a is None:
        continue
    print("page %d:  %5d of 8192" % (n, b - a))
    if b - a > 0x2000:
        errs.append("page %d is over 8K" % n)
a, b = syms["MB_P5"], syms["MB_P5END"]
print("data:    %5d of 8192 (RAM $%04X)" % (b - a, a))
if b - a > 0x2000:
    errs.append("the data page is over 8K")
ramend, ramtop = syms["RAMEND"], syms["RAMTOP"]
print("RAM:     ends $%04X (top $%04X)" % (ramend, ramtop))
if ramend > ramtop:
    errs.append("RAM past RAMTOP")
for e in errs:
    print("checksize: " + e, file=sys.stderr)
sys.exit(1 if errs else 0)

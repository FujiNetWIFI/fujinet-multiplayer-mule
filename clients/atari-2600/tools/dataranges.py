#!/usr/bin/env python3
"""dataranges.py -- checkrom --data ranges, one per bank, from the listings.

A bank's binary tables (sprite shapes, colour tables) sit after its code and
start at the label DSTART; their bytes can be $81 or $91, which checkrom's
linear walk would read as an indirect store. Everything from DSTART to the
end of the bank is declared data for THAT bank only.

Usage: dataranges.py build/boot.lst build/lobby.lst ...   (in bank order;
       '-' for a bank with no listing)
"""
import re
import sys

SYM = re.compile(r"\bDSTART\s*:\s*([0-9A-F]{4})\s+C\b")

out = []
for bank, path in enumerate(sys.argv[1:]):
    if path == "-":
        continue
    for line in open(path):
        m = SYM.search(line)
        if m:
            out.append("--data=%d:$%s-$17FF" % (bank, m.group(1)))
            break
print(" ".join(out))

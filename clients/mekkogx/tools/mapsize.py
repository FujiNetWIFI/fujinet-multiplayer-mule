#!/usr/bin/env python3
"""mapsize.py MAPFILE -- bytes per module and segment from an ld65 map."""
import re, sys, collections
mod = None
tot = collections.Counter()
rows = []
sec = False
for line in open(sys.argv[1]):
    if line.startswith("Modules list"):
        sec = True
        continue
    if line.startswith("Segment list"):
        break
    if not sec:
        continue
    m = re.match(r"^(\S.*):$", line.rstrip())
    if m:
        mod = m.group(1)
        continue
    m = re.match(r"^\s+(\S+)\s+Offs=\w+\s+Size=(\w+)", line)
    if m and mod:
        size = int(m.group(2), 16)
        if size:
            rows.append((size, mod, m.group(1)))
            tot[m.group(1)] += size
for size, mod, seg in sorted(rows, reverse=True)[:int(sys.argv[2]) if len(sys.argv) > 2 else 40]:
    print("%6d  %-8s %s" % (size, seg, mod))
print("totals:", dict(tot))

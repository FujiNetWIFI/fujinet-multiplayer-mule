#!/usr/bin/env python3
"""pagemap.py -- how full each 256-byte page is, from an AS listing.

    pagemap.py build/mule_p0.lst [build/mule_p1.lst ...]

Conditional jumps and MOVP tables are confined to their page, so the free
bytes per page -- not per bank -- are what decide where code can still go.
"""
import re
import sys

LINE = re.compile(r"^(?:\(\d+\))?\s*\d+/\s*([0-9A-F]{3,4}) : ((?:[0-9A-F]{2} )+)")


def main(paths):
    for path in paths:
        used = set()
        for line in open(path, errors="ignore"):
            m = LINE.match(line)
            if m:
                a = int(m.group(1), 16)
                used.update(range(a, a + len(m.group(2).split())))
        print(path)
        for page in range(4, 16):
            n = sum(1 for a in range(page * 256, page * 256 + 256) if a in used)
            if n:
                top = max(a for a in used if a >> 8 == page) & 0xFF
                print("  $%X00: %3d used, top $%02X, %3d free above it" % (page, n, top, 255 - top))


main(sys.argv[1:])

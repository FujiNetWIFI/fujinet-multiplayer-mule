#!/usr/bin/env python3
"""addr2sym.py MAPFILE ADDR... -- the nearest exported symbol at or below each address."""
import re, sys
t = open(sys.argv[1]).read()
part = t.split("Exports list by name:")[1].split("Exports list by value:")[0]
syms = sorted(set((int(a, 16), n) for n, a in re.findall(r"(\S+)\s+([0-9A-F]{6})\s+\S+", part)))
for a in sys.argv[2:]:
    v = int(a, 16)
    best = [s for s in syms if s[0] <= v and not s[1].startswith("__")]
    print("%04X %s+%X" % (v, best[-1][1], v - best[-1][0]) if best else a)

#!/usr/bin/env python3
"""gaps.py TRACE MAPFILE [N] -- the longest stretches between frame_tick
calls in a MAME trace (a2sim 'trace'), and the functions they were spent in.
The clock misses a frame when a stretch runs past the ~4.4 ms blank
(roughly 1500 instructions)."""
import bisect, collections, re, sys

log = open(sys.argv[1]).read().splitlines()
t = open(sys.argv[2]).read()
part = t.split("Exports list by name:")[1].split("Exports list by value:")[0]
syms = sorted(set((int(a, 16), n) for n, a in re.findall(r"(\S+)\s+([0-9A-F]{6})\s+\S+", part)))
addrs = [a for a, n in syms]
ft = [a for a, n in syms if n == "_frame_tick"][0]


def sym(a):
    i = bisect.bisect_right(addrs, a) - 1
    return syms[i][1] if i >= 0 else "?"


cur = start = 0
region = collections.Counter()
gaps = []
for line in log:
    m = re.match(r"\s*\(loops for (\d+) instructions\)", line)
    if m:
        cur += int(m.group(1))
        continue
    m = re.match(r"([0-9A-F]{4}): ", line)
    if not m:
        continue
    cur += 1
    a = int(m.group(1), 16)
    if a == ft:
        gaps.append((cur - start, region.most_common(5)))
        start = cur
        region = collections.Counter()
    else:
        region[sym(a)] += 1
gaps.sort(key=lambda g: -g[0])
for g in gaps[:int(sys.argv[3]) if len(sys.argv) > 3 else 8]:
    print(g[0], g[1])

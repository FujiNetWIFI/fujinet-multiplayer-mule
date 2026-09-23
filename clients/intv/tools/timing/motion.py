#!/usr/bin/env python3
"""Summarise sprite motion in a cap.c trace as piecewise-linear segments.

usage: motion.py trace.csv [first_frame]
For each of hp0..hp3 (horizontal colour clocks) and p0y..p3y (scanlines)
prints: column  frame_from-frame_to  value_from->value_to  frames/step step
"""
import sys


def segs(fr, vals):
    out = []
    i = 0
    n = len(vals)
    while i < n - 1:
        j = i + 1
        # skip unchanged
        if vals[j] == vals[i]:
            k = j
            while k < n and vals[k] == vals[i]:
                k += 1
            out.append((fr[i], fr[k - 1], vals[i], vals[i], 0, 0))
            i = k - 1 if k - 1 > i else k
            continue
        # moving: collect change points with constant step
        pts = [i]
        k = i + 1
        while k < n:
            if vals[k] != vals[k - 1]:
                pts.append(k)
            if len(pts) >= 2 and (vals[pts[-1]] - vals[pts[-2]]) != (vals[pts[1]] - vals[pts[0]]):
                pts.pop()
                break
            if k - pts[-1] > 40:
                break
            k += 1
        a, b = pts[0], pts[-1]
        step = vals[pts[1]] - vals[pts[0]] if len(pts) > 1 else 0
        per = (fr[b] - fr[a]) / max(1, len(pts) - 1)
        out.append((fr[a], fr[b], vals[a], vals[b], per, step))
        i = b if b > i else i + 1
    return out


def main():
    rows = [l.strip().split(',') for l in open(sys.argv[1]) if not l.startswith('#')]
    hdr, rows = rows[0], rows[1:]
    base = int(sys.argv[2]) if len(sys.argv) > 2 else int(rows[0][0])
    fr = [int(r[0]) - base for r in rows]
    for col in ('hp0', 'hp1', 'hp2', 'hp3', 'p0y', 'p1y', 'p2y', 'p3y'):
        c = hdr.index(col)
        v = [int(r[c]) for r in rows]
        for s in segs(fr, v):
            if s[4] == 0 and s[1] - s[0] < 8:
                continue
            if s[4]:
                print(f'{col} {s[0]:6d}-{s[1]:6d} {s[2]:4d}->{s[3]:4d}  {s[4]:5.2f} f/step  step {s[5]:+d}')
            else:
                print(f'{col} {s[0]:6d}-{s[1]:6d} hold {s[2]}')


main()

#!/usr/bin/env python3
"""check.py -- read the M0 probe's snapshot line by line.

    check.py probe.json snap.png SYSTEM

Each zone should be one colour on all 8 lines (sampled at x 20), the sprite
white on every line (x 140). Allowed by the protocol: a 320A zone followed by
a 160A one may show its last line in 160A (the early switch); a 320A zone
after a 160A one its first line, when the zone before it was heavy (the late
switch). Prints a line per zone and a verdict.
"""
import json
import sys

from PIL import Image


def main():
    lay = json.load(open(sys.argv[1]))
    im = Image.open(sys.argv[2]).convert("RGB")
    sysname = sys.argv[3]
    w, h = im.size
    px = im.load()
    # zone 0's first line: the first non-black line at the sample point
    y0 = next(y for y in range(h) if px[40, y] != (0, 0, 0))
    bad = 0
    print("%s: %dx%d, zone 0 at y %d" % (sysname, w, h, y0))
    zones = lay["zones"]
    for z, zd in enumerate(zones):
        lines = [px[40, y0 + 8 * z + l] for l in range(8)]
        spr = [px[281, y0 + 8 * z + l] for l in range(8)]
        body = lines[3]
        ok = [c == body for c in lines]
        prev = zones[z - 1] if z else None
        nxt = zones[z + 1] if z + 1 < len(zones) else None
        notes = []
        allowed = set()
        if zd["kind"] == "T" and nxt and nxt["kind"] != "T":
            allowed.add(7)
        if zd["kind"] == "T" and prev and prev["kind"] != "T" and prev["dma"] > 200:
            allowed.add(0)              # the late switch: only after a heavy zone
        for l in range(8):
            if not ok[l]:
                if l in allowed:
                    notes.append("line %d %s" % (l, "early" if l == 7 else "late"))
                else:
                    notes.append("LINE %d WRONG" % l)
                    bad += 1
        if body == (0, 0, 0):
            notes.append("BLACK")
            bad += 1
        if any(s != spr[0] for s in spr) or spr[0] == (0, 0, 0):
            notes.append("SPRITE %s" % sorted(set(spr)))
            bad += 1
        print("zone %2d %s dma %3d (above %3s) %s" % (
            z, zd["kind"], zd["dma"], prev["dma"] if prev else "-", ", ".join(notes) or "ok"))
    print("VERDICT %s %s" % (sysname, "PASS" if not bad else "FAIL (%d)" % bad))


if __name__ == "__main__":
    main()

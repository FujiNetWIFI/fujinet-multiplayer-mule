#!/usr/bin/env python3
"""The Astrocade client's graphics, from the tables its build assembles.

usage: astrogfx.py [--preview DIR]

clients/astrocade/gen/spr.inc (tools/mkgfx_astro.py, from the original disk)
holds SHTAB, {bytes a row, rows, frame bytes, DW data} for the 14 shapes in
SH_* order, at 3/8 of the Atari's height; gen/map.inc the 8 x 6 pictures;
font46.inc the 4 x 6 font (ink in the high nibble).
"""
import argparse
import os
import re
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from atarigfx import SHAPES, NFRAMES  # noqa: E402

AC = os.path.join(HERE, "..", "..", "clients", "astrocade")


def db_blocks(src):
    """label -> the DB bytes after it (to the next label)."""
    out, cur = {}, None
    for ln in src.splitlines():
        m = re.match(r"^([A-Z_0-9]+):", ln)
        if m:
            cur = m.group(1)
            out[cur] = []
            continue
        m = re.match(r"\s+DB\s+(.*)", ln)
        if m and cur:
            for v in m.group(1).split(";")[0].split(","):
                v = v.strip()
                if v:
                    out[cur].append(int(v[:-1], 16) if v.upper().endswith("H") else int(v))
    return out


class AstroGfx:
    target = "astrocade"
    cell = (4, 6)

    def __init__(self):
        spr = open(os.path.join(AC, "gen", "spr.inc")).read()
        blocks = db_blocks(spr)
        tab = re.search(r"^SHTAB:(.*?)\n[A-Z_0-9]+:", spr, re.S | re.M).group(1)
        ents = re.findall(r"DB\s+(\d+),(\d+),(\d+)\s*\n\s*DW\s+(\w+)", tab)
        self.shapes = {}
        for (wb, h, fb, lab), name in zip(ents, SHAPES):
            wb, h, fb = int(wb), int(h), int(fb)
            data = blocks[lab]
            fr = []
            for k in range(NFRAMES[name]):
                raw = data[k * fb:(k + 1) * fb]
                fr.append([int.from_bytes(bytes(raw[r * wb:(r + 1) * wb]), "big") for r in range(h)])
            self.shapes[name] = (fr, wb, wb * 8, h)
        pics = db_blocks(open(os.path.join(AC, "gen", "map.inc")).read())
        self.pics = [pics[n] for n in ("PIC_STOREICON", "PIC_FLAG", "PIC_FIRE0", "PIC_FIRE1")]
        self.font = db_blocks(open(os.path.join(AC, "font46.inc")).read())["FONT46"]

    def glyphs(self):
        g = {}
        for c in range(64):
            rows = self.font[c * 6:c * 6 + 6]
            g[chr(32 + c)] = np.array([[(b >> (7 - x)) & 1 for x in range(4)] for b in rows], dtype=bool)
        for c in range(ord("a"), ord("z") + 1):
            g[chr(c)] = g[chr(c - 32)]
        return g

    def frame(self, name, k):
        return self.shapes[name][0][k]

    def width(self, name):
        return self.shapes[name][1] * 8


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--preview")
    a = ap.parse_args()
    g = AstroGfx()
    for n, (fr, wb, w, h) in g.shapes.items():
        print("  %-9s %3d frames, %2d x %2d" % (n, len(fr), w, h))
    if a.preview:
        from mgxgfx import preview
        preview(g, a.preview)


if __name__ == "__main__":
    main()

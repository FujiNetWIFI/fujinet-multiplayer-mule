#!/usr/bin/env python3
"""The Atari 2600 client's graphics, from the tables its build assembles.

usage: a26gfx.py [--preview DIR]

clients/atari-2600/build/gen (tools/mkgfx2600.py, from the original disk):
  townspr.inc  TFIGS: species * 88 + frame * 22, frames (0, 1, 4, 8) of the
               Atari's 11 (walk right x2, front, back); TMULE 2 x 24
  mapspr.inc   MFIGS: species * 56 + frame * 8, frames (0, 1, 4, 6, 7, 8, 9)
               of the Atari's 10; MMULE 2 x 8; MWAMP
The 2600 keeps only the right-facing frames and mirrors them (REFP), so the
left-facing ones are made the same way here. The font is the cartridge's:
fn-2600's vcs_font.h, 3 x 5 in a 4 x 6 cell. What the 2600 never draws (a
meteor, the store icon, the flag) comes from the stand-in art the 2600's
generator itself builds on (clients/intv/tools/mkgfx.py).
"""
import argparse
import os
import re
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
A26 = os.path.join(HERE, "..", "..", "clients", "atari-2600")
FONT = os.path.expanduser("~/Workspace/fn-2600/pico/atari-2600/firmware/include/vcs_font.h")


def db_blocks(src):
    out, cur = {}, None
    for ln in src.splitlines():
        m = re.match(r"^([A-Za-z_0-9]+):", ln)
        if m:
            cur = m.group(1)
            out[cur] = []
            continue
        m = re.match(r"\s+DB\s+(.*)", ln)
        if m and cur:
            for v in m.group(1).split(";")[0].split(","):
                v = v.strip()
                if v:
                    out[cur].append(int(v[1:], 16) if v.startswith("$") else int(v, 0))
    return out


def mirror(rows):
    return [int("{:08b}".format(b)[::-1], 2) for b in rows]


def art(lines):
    return [int("".join("1" if c == "X" else "0" for c in ln), 2) for ln in lines]


class A26Gfx:
    target = "a2600"
    cell = (4, 6)

    def __init__(self):
        town = db_blocks(open(os.path.join(A26, "build", "gen", "townspr.inc")).read())
        mp = db_blocks(open(os.path.join(A26, "build", "gen", "mapspr.inc")).read())
        t, m = town["TFIGS"], mp["MFIGS"]
        species, figure = [], []
        for s in range(8):
            tf = [t[s * 88 + k * 22:s * 88 + k * 22 + 22] for k in range(4)]   # 0 1 4 8
            r0, r1, front, back = tf
            species += [r0, r1, mirror(r0), mirror(r1), front, front, front, front, back, back, back]
            mf = [m[s * 56 + k * 8:s * 56 + k * 8 + 8] for k in range(7)]      # 0 1 4 6 7 8 9
            figure += [mf[0], mf[1], mirror(mf[0]), mirror(mf[1]), mf[2], mf[2], mf[3], mf[4], mf[5], mf[6]]
        tm, mm = town["TMULE"], mp["MMULE"]
        town_mule = [tm[0:24], tm[24:48]]
        map_mule = [mm[0:8] + [0] * 16, mm[8:16] + [0] * 16]
        mule = town_mule + map_mule
        mule += [mirror(f) for f in mule]
        sys.path.insert(0, os.path.join(HERE, "..", "..", "clients", "intv", "tools"))
        import mkgfx as intv
        cursor = [0xFF] * 2 + [0x81] * 28 + [0xFF] * 2           # a plot's outline, double width
        self.shapes = {
            "species": (species, 1, 8, 22), "figure": (figure, 1, 8, 8), "mule": (mule, 1, 8, 24),
            "wampus": ([mp["MWAMP"][:8]], 1, 8, 8), "cursor": ([cursor], 1, 8, 32),
            "meteor": ([art(intv.METEOR_ART)], 1, 8, 8),
        }
        self.pics = [art(intv.STOREICON_ART), art(intv.FLAG_ART), art(intv.FIRE_ART[0]), art(intv.FIRE_ART[1])]
        src = open(FONT).read()
        rows = re.findall(r"\{\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)\s*\}", src)
        self.font = [[int(v) for v in r] for r in rows]          # 96 glyphs from ' '

    def glyphs(self):
        g = {}
        for c, rows in enumerate(self.font):
            a = np.zeros((6, 4), dtype=bool)
            for y, b in enumerate(rows):
                for x in range(3):
                    a[y, x] = (b >> (2 - x)) & 1
            g[chr(32 + c)] = a
        return g

    def frame(self, name, k):
        return self.shapes[name][0][k]

    def width(self, name):
        return self.shapes[name][1] * 8


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--preview")
    a = ap.parse_args()
    g = A26Gfx()
    for n, (fr, wb, w, h) in g.shapes.items():
        print("  %-9s %3d frames, %2d x %2d" % (n, len(fr), w, h))
    if a.preview:
        from mgxgfx import preview
        g.cell = (4, 6)
        preview(g, a.preview)


if __name__ == "__main__":
    main()

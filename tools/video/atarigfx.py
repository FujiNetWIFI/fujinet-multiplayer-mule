#!/usr/bin/env python3
"""The Atari client's graphics, read out of its running RAM.

usage: atarigfx.py [--check] [--preview DIR] SNAPDIR

SNAPDIR holds an atarisim snapshot of the client (gfx.mem, the 64K RAM with
every block in place, and palette.bin, libatari800's colours); clips_atari.py
makes it. Blocks are found by their labels in build/mule-atari.map:

  font        the M.U.L.E. font: 64 characters from ' ', 8 bytes each
  shapes      atari_shapes[14] = {pointer, height, SIZEP}, in SH_* order
              (src/platform-specific/gfx.h), frames back to back
  atari_col   the 16 game colours (C_*)

--check compares them with the original disk, through the generator the
build uses (clients/mekkogx/tools/mkgfx_atari.py), which copies them as is.
"""
import argparse
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
MX = os.path.join(HERE, "..", "..", "clients", "mekkogx")
MAPFILE = os.path.join(MX, "build", "mule-atari.map")

SHAPES = ["species", "figure", "mule", "wampus", "cursor", "nose", "hull_top", "hull_bot",
          "dock", "pest", "cloud", "bolt", "spark", "meteor"]
NFRAMES = {"species": 88, "figure": 80, "mule": 8, "wampus": 1, "cursor": 1, "nose": 2,
           "hull_top": 2, "hull_bot": 2, "dock": 4, "pest": 4, "cloud": 4, "bolt": 1,
           "spark": 1, "meteor": 1}

# C_* (gfx.h)
C_BLACK, C_WHITE, C_BLUE, C_PURPLE, C_GREEN, C_ORANGE, C_RED, C_YELLOW = range(8)
C_GREY, C_CYAN, C_BROWN, C_TAN, C_LTBLUE, C_DKGREEN, C_PINK, C_P3 = range(8, 16)


def symbols(mapfile=MAPFILE):
    syms = {}
    on = False
    for ln in open(mapfile):
        if ln.startswith("Exports list by name"):
            on = True
            continue
        if on:
            if ln.startswith("Exports list by value"):
                break
            p = ln.split()
            for i in range(0, len(p) - 1, 3):
                if p[i].startswith("_"):
                    syms[p[i]] = int(p[i + 1], 16)
    return syms


class AtariGfx:
    def __init__(self, snapdir):
        self.mem = open(os.path.join(snapdir, "gfx.mem"), "rb").read()
        pal = np.frombuffer(open(os.path.join(snapdir, "palette.bin"), "rb").read(), dtype=np.uint8)
        self.pal = pal.reshape(256, 3)
        s = symbols()
        m = self.mem
        f = s["_font"]
        self.font = [list(m[f + c * 8:f + c * 8 + 8]) for c in range(64)]
        self.col = list(m[s["_atari_col"]:s["_atari_col"] + 16])
        # gfx_pic's pictures: store icon, flag, fire 0, fire 1 (8 x 8)
        self.pics = [list(m[s["_pics"] + k * 8:s["_pics"] + k * 8 + 8]) for k in range(4)]
        self.shapes = {}
        base = s["_atari_shapes"]
        for i, name in enumerate(SHAPES):
            p = m[base + i * 4] | m[base + i * 4 + 1] << 8
            h, size = m[base + i * 4 + 2], m[base + i * 4 + 3]
            fr = [list(m[p + k * h:p + k * h + h]) for k in range(NFRAMES[name])]
            self.shapes[name] = (fr, h, size)

    def glyphs(self):
        """char -> 8 x 8 bool (the font covers ' ' to '_'; lower case maps up)."""
        g = {}
        for c in range(64):
            g[chr(32 + c)] = np.array([[(b >> (7 - x)) & 1 for x in range(8)] for b in self.font[c]],
                                      dtype=bool)
        for c in range(ord("a"), ord("z") + 1):
            g[chr(c)] = g[chr(c - 32)]
        return g

    def frame(self, name, k):
        return self.shapes[name][0][k]


def check(g):
    sys.path.insert(0, os.path.join(MX, "tools"))
    import mkgfx_atari
    import mkgfx_hgr as hgr
    atr = os.path.join(MX, "mule.atr")
    atari = hgr.Atari(atr)
    m = atari.main
    bad = 0
    font = bytearray(m[0x1800:0x1A00])
    for ch in "%&'()*+":
        c = ord(ch) - 32
        font[c * 8:c * 8 + 8] = mkgfx_atari.standin_glyph(ch)
    if bytes(sum(g.font, [])) != bytes(font):
        print("MISMATCH font")
        bad += 1
    for sh in mkgfx_atari.shapes(atari):
        ours = [b for f in g.shapes[sh.name][0] for b in f]
        theirs = [b for f in sh.frames for b in f]
        if ours != theirs or g.shapes[sh.name][1] != sh.h:
            print("MISMATCH", sh.name)
            bad += 1
    print("check:", "OK" if not bad else "%d mismatches" % bad)
    return bad


def preview(g, out):
    from PIL import Image
    os.makedirs(out, exist_ok=True)
    for name, (fr, h, size) in g.shapes.items():
        n = len(fr)
        cols = min(n, 22)
        rows = (n + cols - 1) // cols
        im = Image.new("RGB", (cols * 20, rows * (h + 4)), (40, 40, 60))
        px = im.load()
        for k, f in enumerate(fr):
            ox, oy = (k % cols) * 20, (k // cols) * (h + 4)
            for y, b in enumerate(f):
                for x in range(8):
                    if (b >> (7 - x)) & 1:
                        px[ox + 2 * x, oy + y] = px[ox + 2 * x + 1, oy + y] = (255, 255, 255)
        im.resize((im.width * 3, im.height * 3), Image.NEAREST).save(os.path.join(out, name + ".png"))
    im = Image.new("RGB", (16 * 9, 4 * 9), (40, 40, 60))
    px = im.load()
    for c in range(64):
        for y, b in enumerate(g.font[c]):
            for x in range(8):
                if (b >> (7 - x)) & 1:
                    px[(c % 16) * 9 + x, (c // 16) * 9 + y] = (255, 255, 255)
    im.resize((im.width * 4, im.height * 4), Image.NEAREST).save(os.path.join(out, "font.png"))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--preview")
    ap.add_argument("snapdir")
    a = ap.parse_args()
    g = AtariGfx(a.snapdir)
    print("font 64 chars; colours", " ".join("%02X" % c for c in g.col))
    for n, (fr, h, size) in g.shapes.items():
        print("  %-9s %3d frames x %2d lines, size %d" % (n, len(fr), h, size))
    if a.preview:
        preview(g, a.preview)
    if a.check:
        sys.exit(1 if check(g) else 0)


if __name__ == "__main__":
    main()

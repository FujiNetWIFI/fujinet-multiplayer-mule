#!/usr/bin/env python3
"""The Apple II client's graphics, read out of its build.

usage: a2gfx.py [--check] [--preview DIR]

The program (r2r/apple2/mule.a2s, AppleSingle) is main RAM from its load
address; MULE.6 (mule.a2s.6) is the aux block AUXGFX, loaded at $8C00, that
holds the sprites. Labels come from build/mule-apple2.map:

  hgr_font    64 glyphs from ' ', 8 rows; bit 0 is the left dot (7 dots)
  hgr_shapes  14 x {data, wb, w, h}: data in aux, rows of wb bytes, one bit a
              colour pixel (two dots), MSB left; frames back to back
  hgr_pics    store icon, flag, fire 0, fire 1 (8 x 8, drawn 2 lines a row)

--check compares them with gen/apple2/gfx.c, which tools/mkgfx_hgr.py made
from the original disk and the build compiled.
"""
import argparse
import os
import re
import struct
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from atarigfx import symbols, SHAPES, NFRAMES  # noqa: E402

MX = os.path.join(HERE, "..", "..", "clients", "mekkogx")
MAPFILE = os.path.join(MX, "build", "mule-apple2.map")
AUXGFX = 0x8C00


def applesingle(path):
    """(load address, data fork) of an AppleSingle file."""
    d = open(path, "rb").read()
    n = struct.unpack(">H", d[24:26])[0]
    data, addr = None, None
    for i in range(n):
        eid, off, ln = struct.unpack(">III", d[26 + 12 * i:38 + 12 * i])
        if eid == 1:
            data = d[off:off + ln]
        elif eid == 11:
            addr = struct.unpack(">I", d[off + 4:off + 8])[0]
    return addr, data


class A2Gfx:
    def __init__(self):
        r2r = os.path.join(MX, "r2r", "apple2")
        addr, data = applesingle(os.path.join(r2r, "mule.a2s"))
        self.main = bytearray(0x10000)
        self.main[addr:addr + len(data)] = data
        aux = open(os.path.join(r2r, "mule.a2s.6"), "rb").read()
        self.aux = bytearray(0x10000)
        self.aux[AUXGFX:AUXGFX + len(aux)] = aux
        s = symbols(MAPFILE)
        m = self.main
        f = s["_hgr_font"]
        self.font = [list(m[f + c * 8:f + c * 8 + 8]) for c in range(64)]
        p = s["_hgr_pics"]
        self.pics = [list(m[p + k * 8:p + k * 8 + 8]) for k in range(4)]
        self.shapes = {}
        base = s["_hgr_shapes"]
        for i, name in enumerate(SHAPES):
            e = m[base + i * 5:base + i * 5 + 5]
            ptr, wb, w, h = e[0] | e[1] << 8, e[2], e[3], e[4]
            size = wb * h
            fr = []
            for k in range(NFRAMES[name]):
                raw = self.aux[ptr + k * size:ptr + (k + 1) * size]
                # a row of wb bytes -> one int, MSB the leftmost colour pixel
                fr.append([int.from_bytes(raw[r * wb:(r + 1) * wb], "big") for r in range(h)])
            self.shapes[name] = (fr, wb, w, h)

    def glyphs(self):
        """char -> 8 x 8 bool: the 7 dots of a glyph (bit 0 left), then a gap."""
        g = {}
        for c in range(64):
            g[chr(32 + c)] = np.array([[(b >> x) & 1 if x < 7 else 0 for x in range(8)]
                                       for b in self.font[c]], dtype=bool)
        # The game draws its bar graphs with % & ' ( ) * +; give the video
        # the generator's own glyphs for them back (as the Atari client does).
        sys.path.insert(0, os.path.join(MX, "tools"))
        import mkgfx_atari
        for ch in "%&'()*+":
            rows = mkgfx_atari.standin_glyph(ch)
            g[ch] = np.array([[(b >> (7 - x)) & 1 for x in range(8)] for b in rows], dtype=bool)
        for c in range(ord("a"), ord("z") + 1):
            g[chr(c)] = g[chr(c - 32)]
        return g

    def frame(self, name, k):
        return self.shapes[name][0][k]

    def width(self, name):
        """Bits a row, as Canvas.bits wants them (the data's, wb * 8)."""
        return self.shapes[name][1] * 8


def check(g):
    src = open(os.path.join(MX, "gen", "apple2", "gfx.c")).read()

    def arr(name):
        m = re.search(r"\b%s(?:\[[^\]]*\])+\s*=\s*\{(.*?)\};" % re.escape(name), src, re.S)
        return [int(v, 16) for v in re.findall(r"0x([0-9A-Fa-f]{2})", m.group(1))]
    bad = 0
    if sum(g.font, []) != arr("hgr_font"):
        print("MISMATCH font")
        bad += 1
    for name in SHAPES:
        fr, wb, w, h = g.shapes[name]
        ours = []
        for f in fr:
            for row in f:
                ours += list(row.to_bytes(wb, "big"))
        if ours != arr("sh_" + name):
            print("MISMATCH", name)
            bad += 1
    if sum(g.pics, []) != arr("hgr_pics"):
        print("MISMATCH pics")
        bad += 1
    print("check:", "OK" if not bad else "%d mismatches" % bad)
    return bad


def preview(g, out):
    from PIL import Image
    os.makedirs(out, exist_ok=True)
    for name, (fr, wb, w, h) in g.shapes.items():
        cols = min(len(fr), 22)
        rows = (len(fr) + cols - 1) // cols
        cw = wb * 8 * 2 + 4
        im = Image.new("RGB", (cols * cw, rows * (h + 4)), (40, 40, 60))
        px = im.load()
        for k, f in enumerate(fr):
            ox, oy = (k % cols) * cw, (k // cols) * (h + 4)
            for y, row in enumerate(f):
                for x in range(wb * 8):
                    if (row >> (wb * 8 - 1 - x)) & 1:
                        px[ox + 2 * x, oy + y] = px[ox + 2 * x + 1, oy + y] = (255, 255, 255)
        im.resize((im.width * 3, im.height * 3), Image.NEAREST).save(os.path.join(out, name + ".png"))
    im = Image.new("RGB", (16 * 9, 4 * 9), (40, 40, 60))
    px = im.load()
    gl = g.glyphs()
    for c in range(64):
        a = gl[chr(32 + c)]
        for y in range(8):
            for x in range(8):
                if a[y, x]:
                    px[(c % 16) * 9 + x, (c // 16) * 9 + y] = (255, 255, 255)
    im.resize((im.width * 4, im.height * 4), Image.NEAREST).save(os.path.join(out, "font.png"))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--preview")
    a = ap.parse_args()
    g = A2Gfx()
    for n, (fr, wb, w, h) in g.shapes.items():
        print("  %-9s %3d frames, %2d x %2d (%d bytes a row)" % (n, len(fr), w, h, wb))
    if a.preview:
        preview(g, a.preview)
    if a.check:
        sys.exit(1 if check(g) else 0)


if __name__ == "__main__":
    main()

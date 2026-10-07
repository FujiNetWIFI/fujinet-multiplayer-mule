#!/usr/bin/env python3
"""A MekkoGX client's graphics, from the tables its build compiles.

usage: mgxgfx.py TARGET [--preview DIR]       TARGET: msdos coco tms lynx

clients/mekkogx/gen/<dir>/gfx.c is what tools/mkgfx_*.py made from the
original disk for that client, and what its build links: the font, the
shape table (14 shapes in SH_* order, frames back to back, rows of wb bytes,
MSB left) and the four small pictures.

  msdos  dos_font (8 x 8)    dos_shapes {data, wb, w, h}    a bit: a colour clock
  coco   pm_font (8 x 8)     pm_shapes  {data, wb, w, h}    a bit: a colour pixel
  tms    tms_font (8 x 8)    tms_shapes {data, wb, w, h}    a bit: 1.5 pixels (wide12)
  lynx   lynx_font (3 x 5)   lynx_shapes {data, fsize, wb, w, h}, at half height
"""
import argparse
import os
import re
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from atarigfx import SHAPES, NFRAMES  # noqa: E402

MX = os.path.join(HERE, "..", "..", "clients", "mekkogx")
SPEC = {   # gen dir, font, shapes, pics, font cell
    "msdos": ("msdos", "dos_font", "dos_shapes", "dos_pics", (8, 8)),
    "coco": ("coco", "pm_font", "pm_shapes", "pm_pics", (8, 8)),
    "tms": ("tms9918", "tms_font", "tms_shapes", "tms_pics", (8, 8)),
    "lynx": ("lynx", "lynx_font", "lynx_shapes", "lynx_pics", (4, 6)),
}


def carray(src, name):
    """The numbers of a C array initialiser."""
    m = re.search(r"\b%s(?:\[[^\]]*\])+\s*=\s*\{(.*?)\};" % re.escape(name), src, re.S)
    if not m:
        raise KeyError(name)
    body = re.sub(r"/\*.*?\*/", "", m.group(1), flags=re.S)
    return [int(v, 0) for v in re.findall(r"0x[0-9A-Fa-f]+|\b\d+\b", body)]


class MgxGfx:
    def __init__(self, target):
        d, font, shapes, pics, self.cell = SPEC[target]
        self.target = target
        src = open(os.path.join(MX, "gen", d, "gfx.c")).read()
        self.src = src
        self.font = carray(src, font)
        p = carray(src, pics)
        self.pics = [p[k * 8:k * 8 + 8] for k in range(4)]
        m = re.search(r"\b%s\[\d+\]\s*=\s*\{(.*?)\};" % shapes, src, re.S)
        ents = re.findall(r"\{\s*(sh_\w+)\s*,([^}]*)\}", m.group(1))
        self.shapes = {}
        for (sym, rest), name in zip(ents, SHAPES):
            nums = [int(v) for v in rest.replace(",", " ").split()]
            wb, w, h = nums[-3:]
            data = carray(src, sym)
            size = wb * h
            fr = []
            for k in range(NFRAMES[name]):
                raw = data[k * size:(k + 1) * size]
                fr.append([int.from_bytes(bytes(raw[r * wb:(r + 1) * wb]), "big") for r in range(h)])
            self.shapes[name] = (fr, wb, w, h)
        self.wide12 = carray(src, "wide12") if target == "tms" else None

    def glyphs(self):
        cw, ch = self.cell
        g = {}
        for c in range(64):
            if self.target == "lynx":
                rows = self.font[c * 5:c * 5 + 5]       # top 3 bits of each line
                a = np.zeros((ch, cw), dtype=bool)
                for y, b in enumerate(rows):
                    for x in range(3):
                        a[y, x] = (b >> (7 - x)) & 1
            else:
                rows = self.font[c * 8:c * 8 + 8]
                a = np.array([[(b >> (7 - x)) & 1 for x in range(8)] for b in rows], dtype=bool)
            g[chr(32 + c)] = a
        for c in range(ord("a"), ord("z") + 1):
            g[chr(c)] = g[chr(c - 32)]
        return g

    def frame(self, name, k):
        return self.shapes[name][0][k]

    def width(self, name):
        return self.shapes[name][1] * 8

    def widened(self, name, k):
        """The TMS's 3/2 widening (wide12): each byte becomes 12 bits."""
        fr, wb, w, h = self.shapes[name]
        out = []
        for row in fr[k]:
            v = 0
            for i in range(wb):
                byte = (row >> (8 * (wb - 1 - i))) & 0xFF
                v = (v << 12) | self.wide12[byte]
            out.append(v)
        return out, wb * 12


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
    gl = g.glyphs()
    cw, ch = g.cell
    im = Image.new("RGB", (16 * (cw + 1), 4 * (ch + 1)), (40, 40, 60))
    px = im.load()
    for c in range(64):
        a = gl[chr(32 + c)]
        for y in range(ch):
            for x in range(cw):
                if a[y, x]:
                    px[(c % 16) * (cw + 1) + x, (c // 16) * (ch + 1) + y] = (255, 255, 255)
    im.resize((im.width * 6, im.height * 6), Image.NEAREST).save(os.path.join(out, "font.png"))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("target", choices=sorted(SPEC))
    ap.add_argument("--preview")
    a = ap.parse_args()
    g = MgxGfx(a.target)
    for n, (fr, wb, w, h) in g.shapes.items():
        print("  %-9s %3d frames, %2d x %2d (%d bytes a row)" % (n, len(fr), w, h, wb))
    if a.preview:
        preview(g, a.preview)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Build the NES client's graphics (gen/nes/gfx.c).

usage: mkgfx_nes.py [--atr "M.U.L.E. (1983)(Electronic Arts)(US).atr"] [--preview dir] out.c

The NES draws the TMS9918A clients' screen (src/nes/gfx.c: 256 x 192, x
scaled 3/2 from the Atari's colour clocks), so the art is the Adam's
(tools/mkgfx_tms.py): the Namco font, the plot parts three cells wide, the
shapes at 1 bit a clock, the pictures. Only the town differs: the NES gives
a 16 x 16 block one of four palettes of three colours (and the black
background), so the town is made here block by block, each taking the
building palette (src/nes/gfx.c pal_sets, PS_TOWN: grey with greens, or
with tan and red) that shows it best, and
written as tiles (16 bytes a cell: plane 0, then plane 1), packed.

Without --atr, stand-in art is used and the Makefile warns. The generated
file must not be committed.
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import mkgfx_hgr as hgr  # noqa: E402
import mkgfx_dos as dos  # noqa: E402
import mkgfx_tms as tms  # noqa: E402

intv = hgr.intv

# The 2C02's colours (an emulator's usual palette), for choosing and previews.
NES_RGB = [
    (84, 84, 84), (0, 30, 116), (8, 16, 144), (48, 0, 136), (68, 0, 100), (92, 0, 48),
    (84, 4, 0), (60, 24, 0), (32, 42, 0), (8, 58, 0), (0, 64, 0), (0, 60, 0), (0, 50, 60),
    (0, 0, 0), (0, 0, 0), (0, 0, 0),
    (152, 150, 152), (8, 76, 196), (48, 50, 236), (92, 30, 228), (136, 20, 176),
    (160, 20, 100), (152, 34, 32), (120, 60, 0), (84, 90, 0), (40, 114, 0), (8, 124, 0),
    (0, 118, 40), (0, 102, 120), (0, 0, 0), (0, 0, 0), (0, 0, 0),
    (236, 238, 236), (76, 154, 236), (120, 124, 236), (176, 98, 236), (228, 84, 236),
    (236, 88, 180), (236, 106, 100), (212, 136, 32), (160, 170, 0), (116, 196, 0),
    (76, 208, 32), (56, 204, 108), (56, 180, 204), (60, 60, 60), (0, 0, 0), (0, 0, 0),
    (236, 238, 236), (168, 204, 236), (188, 188, 236), (212, 178, 236), (236, 174, 236),
    (236, 174, 212), (236, 180, 176), (228, 196, 144), (204, 210, 120), (180, 222, 120),
    (168, 226, 144), (152, 226, 180), (160, 214, 228), (160, 162, 160), (0, 0, 0), (0, 0, 0),
]

# The town's palettes (src/nes/gfx.c pal_sets[PS_TOWN]): the background,
# then colours 1-3 of each.
TOWN_BG = 0x0F
TOWN_PALS = [(0x30, 0x21, 0x27), (0x30, 0x2A, 0x24), (0x10, 0x2A, 0x2B), (0x10, 0x38, 0x26)]
BUILDING_PALS = (2, 3)          # what the town's own blocks choose from
# The plots' preview (the map's set): the plains, white, black, blue.
MAP_BG, MAP_WHITE, MAP_BLACK, MAP_P0, RIVER = 0x10, 0x30, 0x0F, 0x21, 0x12


def dist(a, b):
    return sum((x - y) ** 2 for x, y in zip(a, b))


# The town's TMS colours (tools/mkgfx_tms.py ATARI_TO_TMS) as the NES's.
TMS_TO_NES = {1: 0x0F, 14: 0x10, 15: 0x10, 2: 0x2A, 12: 0x2A, 3: 0x2B,
              10: 0x38, 11: 0x38, 8: 0x26, 9: 0x26, 6: 0x26, 13: 0x26, 5: 0x2B, 4: 0x2B}


def town_tiles(atari):
    """The town as tiles: (stream of 16-byte cells, row by row), the
    palette of each block, and the pixels as they will show. A block takes
    the building palette that has the most of its pixels' colours; a colour
    it hasn't got becomes the nearest it has."""
    rows = tms.town_pixels(atari)              # TMS colours, 152 x 144
    want = [[TMS_TO_NES.get(c, 0x10) for c in r] for r in rows]
    w, h = len(rows[0]), len(rows)
    idx = [[0] * w for _ in range(h)]
    shown = [[NES_RGB[TOWN_BG]] * w for _ in range(h)]
    pal = [[0] * 10 for _ in range(9)]
    err_total = 0
    for by in range(9):
        for bx in range(10):
            px = [(x, y) for y in range(by * 16, min(h, by * 16 + 16))
                  for x in range(bx * 16, min(w, bx * 16 + 16))]
            best = None
            for p in BUILDING_PALS:
                choice = [TOWN_BG] + list(TOWN_PALS[p])
                miss = sum(1 for x, y in px if want[y][x] not in choice)
                if best is None or miss < best[0]:
                    best = (miss, p)
            miss, p = best
            err_total += miss
            pal[by][bx] = p
            choice = [TOWN_BG] + list(TOWN_PALS[p])
            for x, y in px:
                c = want[y][x]
                k = choice.index(c) if c in choice else min(
                    range(4), key=lambda i: dist(NES_RGB[c], NES_RGB[choice[i]]))
                idx[y][x] = k
                shown[y][x] = NES_RGB[choice[k]]
    stream = []
    for cy in range(18):
        for cx in range(19):
            p0, p1 = [], []
            for l in range(8):
                b0 = b1 = 0
                for k in range(8):
                    v = idx[cy * 8 + l][cx * 8 + k]
                    if v & 1:
                        b0 |= 0x80 >> k
                    if v & 2:
                        b1 |= 0x80 >> k
                p0.append(b0)
                p1.append(b1)
            stream += p0 + p1
    return stream, pal, shown, err_total


def preview(path, shown, font, parts):
    try:
        from PIL import Image
    except ImportError:
        return
    im = Image.new("RGB", (256, 192), NES_RGB[TOWN_BG])
    for y, r in enumerate(shown):
        for x, c in enumerate(r):
            im.putpixel((48 + x, 16 + y), c)
    for i, g in enumerate(font):
        for y, v in enumerate(g):
            for k in range(8):
                if v & (0x80 >> k):
                    im.putpixel(((i % 32) * 8 + k, 168 + (i // 32) * 9 + y), NES_RGB[0x30])
    im.resize((256 * 3, 192 * 3), Image.NEAREST).save(path)
    im = Image.new("RGB", (len(parts) * 28, 36), NES_RGB[MAP_BG])
    for n, (name, rows) in enumerate(parts):
        for y, r in enumerate(rows):
            for x, b in enumerate(r):
                if b:
                    im.putpixel((n * 28 + 2 + x, 2 + y), NES_RGB[
                        RIVER if name == "river" else MAP_WHITE if name == "store"
                        else MAP_BLACK if name.startswith("mount") else MAP_P0])
    im.resize((im.width * 3, im.height * 3), Image.NEAREST).save(
        os.path.join(os.path.dirname(path), "plots.png"))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--atr")
    ap.add_argument("--preview")
    ap.add_argument("out")
    a = ap.parse_args()
    atari = hgr.Atari(a.atr) if a.atr else None
    c_bytes = tms.c_bytes

    src = ["/* gfx.c -- GENERATED by tools/mkgfx_nes.py (%s). Do not edit or commit. */" %
           ("converted from the Atari disk" if atari else "STAND-IN art: no ATR given"),
           "#include <stdint.h>", '#include "../../src/nes/gfxdata.h"', ""]

    font = tms.namco_font()
    src.append("const uint8_t nes_font[64 * 8] = {")
    src.append(c_bytes([b for g in font for b in g]))
    src.append("};")

    images = dos.plot_images(atari)
    order = ["border", "icon_house", "icon_food", "icon_energy", "icon_smithore", "icon_crystite",
             "icon_mule", "mount1", "mount2", "mount3", "store", "river"]
    parts = [(n, [tms.widen_row(r) for r in images[n]]) for n in order]
    src.append("/* plot parts: 32 lines x 24 pixels (3 bytes, MSB left) */")
    src.append("const uint8_t plot_parts[%d][96] = {" % len(order))
    for n, rows in parts:
        src.append("  { /* %s */" % n)
        src.append(c_bytes([b for r in rows for b in tms.pack(r)]))
        src.append("  },")
    src.append("};")
    # The river's sprites (src/nes/gfx.c): the river part's pixels from its
    # first column (8 at most), two pairs across, two down, in colour 3.
    rb = [b for r in dict(parts)["river"] for b in tms.pack(r)]
    px = lambda l, c: c < 24 and (rb[l * 3 + (c >> 3)] >> (7 - (c & 7))) & 1
    x0 = min([c for l in range(32) for c in range(24) if px(l, c)] + [8])
    pats = []
    for gr in range(2):
        for gc in range(2):
            pat = [0] * 32
            for l in range(16):
                b = sum(0x80 >> c for c in range(8) if px(gr * 16 + l, x0 + gc * 8 + c))
                k = 16 + (l & 7) if l & 8 else l
                pat[k] = pat[k + 8] = b
            pats.append(pat)
    src.append("/* the river's sprites: pairs of 8 x 16 (planes 0, 1 of the top tile,")
    src.append(" * then the bottom's), from pixel river_x0 of its plot */")
    src.append("const uint8_t river_x0 = %d;" % x0)
    src.append("const uint8_t river_pat[4][32] = {")
    for pat in pats:
        src.append("  {" + c_bytes(pat).strip() + "},")
    src.append("};")
    src.append("/* production dots: line, pixel */")
    src.append("const uint8_t plot_dots[8][2] = {")
    src.append("    " + " ".join("{%d,%d}," % d for d in tms.plot_dots()))
    src.append("};")

    shs = dos.shapes(atari)
    src.append("/* sprites: 1 bit a colour clock, MSB left */")
    for s in shs:
        wb = (s.w + 7) // 8
        data = []
        for f in s.frames:
            for r in f:
                data += hgr.pack_row(list(r) + [0] * (wb * 8 - len(r)))
        src.append("static const uint8_t sh_%s[%d] = {" % (s.name, len(data)))
        src.append(c_bytes(data))
        src.append("};")
    src.append("const ShapeDef nes_shapes[%d] = {" % len(shs))
    for s in shs:
        src.append("    { sh_%s, %d, %d, %d }," % (s.name, (s.w + 7) // 8, s.w, s.h))
    src.append("};")

    src.append("const uint8_t nes_pics[%d][8] = {" % len(hgr.PICS))
    for n, art in hgr.PICS:
        src.append("    { %s }, /* %s */" % (",".join("0x%02X" % v for r in intv.from_art(art)
                                                   for v in hgr.pack_row(r)), n))
    src.append("};")

    src.append("const uint16_t wide12[256] = {")
    w = tms.wide12()
    src.append("\n".join("    " + ",".join("0x%03X" % v for v in w[i:i + 12]) + ","
                         for i in range(0, 256, 12)))
    src.append("};")

    stream, pal, shown, err = town_tiles(atari)
    packed = dos.rle(stream)
    src.append("/* (the town's in the development phase's overlay, which draws it) */")
    src.append('#pragma rodata-name (push, "OVERLAY5")')
    src.append("/* the town: 18 cell rows of 19 tiles (16 bytes: plane 0, plane 1),")
    src.append(" * packed: n < 128: n + 1 bytes follow; n >= 128: the next byte n - 125 times. */")
    src.append("const uint8_t town_rle[%d] = {" % len(packed))
    src.append(c_bytes(packed))
    src.append("};")
    src.append("/* the palette of each block of the town (rows 2..10, blocks 3..12) */")
    src.append("const uint8_t town_pal[9][10] = {")
    for r in pal:
        src.append("    { %s }," % ", ".join(str(p) for p in r))
    src.append("};")
    src.append("#pragma rodata-name (pop)")
    open(a.out, "w").write("\n".join(src) + "\n")

    if a.preview:
        os.makedirs(a.preview, exist_ok=True)
        preview(os.path.join(a.preview, "town.png"), shown, font, parts)
    print("%s: %s; town %d bytes packed, %d pixels recoloured" %
          (a.out, "Atari graphics" if atari else "STAND-IN art", len(packed), err))


if __name__ == "__main__":
    main()

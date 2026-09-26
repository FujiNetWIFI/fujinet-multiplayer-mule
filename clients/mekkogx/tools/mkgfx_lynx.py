#!/usr/bin/env python3
"""Build the Atari Lynx client's graphics (gen/lynx/gfx.c) from the original disk.

usage: mkgfx_lynx.py [--atr "M.U.L.E. (1983)(Electronic Arts)(US).atr"] [--preview dir] out.c

The art comes from the original Atari disk, as for the other clients (the
extraction is tools/mkgfx_hgr.py's and tools/mkgfx_dos.py's). Without --atr,
stand-in art is used and the Makefile warns. The generated file must not be
committed.

The Lynx is 160 x 102: across, a colour clock is a pixel, as on the Atari;
down, the map and the town (the Atari's lines 16..175) are halved, so every
picture here is half as tall: two lines ORed into one (src/lynx/gfx.c has
the whole mapping). Pictures are 1 bit a pixel, MSB left, a line after
another (gfx.c makes each a Suzy sprite as it draws it), drawn in a pen of
the game's choosing. The font is 3 x 5, drawn in 4-pixel cells. The town
is 4 bits a pixel, as the screen is, packed (mkgfx_dos.rle).
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import mkgfx_hgr as hgr  # noqa: E402  (the Atari extraction, plots, shapes)
import mkgfx_dos as dos  # noqa: E402  (plot images, shapes, the town, rle)

intv = hgr.intv

# ---------------------------------------------------------------------------
# The pens: gfx.h's colours in order (C_P3 is drawn in C_ORANGE's pen), but
# pen 14 (C_PINK, which the game doesn't use) is black again, for black in a
# sprite (whose pen 0 may be see-through), and pen 15 is the river. Atari
# colour -> 4-bit RGB, picked by eye.
ATARI_RGB = {
    0x00: (0, 0, 0), 0x04: (4, 4, 4), 0x06: (6, 6, 6), 0x08: (8, 8, 8), 0x0C: (12, 12, 12),
    0x0E: (15, 15, 15), 0x16: (11, 9, 2), 0x1A: (13, 12, 4), 0x1C: (14, 13, 5),
    0x24: (9, 5, 1), 0x2A: (13, 9, 5), 0x32: (11, 4, 2), 0x34: (14, 7, 2), 0x44: (12, 3, 3),
    0x54: (11, 3, 11), 0x5A: (15, 8, 13), 0x84: (3, 5, 14), 0x86: (5, 7, 15), 0x88: (6, 8, 15),
    0xA8: (4, 11, 11), 0xB4: (4, 10, 4), 0xC4: (3, 10, 3), 0xD4: (5, 9, 2), 0xF4: (8, 11, 3),
}
PENS = [0x00, 0x0E, 0x84, 0x54, 0xC4, 0x34, 0x32, 0x1C,
        0x08, 0xA8, 0x24, 0x2A, 0x88, 0xD4, 0x00, 0xF4]
PEN_RIVER = 15


def rgb_of(c):
    if c in ATARI_RGB:
        return ATARI_RGB[c]
    hue, lum = c >> 4, (c & 0x0E)
    v = lum + 1
    if hue == 0:
        return (v, v, v)
    base = ATARI_RGB.get(hue << 4 | 4, (8, 8, 8))
    return tuple(max(0, min(15, b + (lum - 4))) for b in base)


def nearest_pen(c, allowed=range(14)):
    r, g, b = rgb_of(c)
    return min(allowed, key=lambda p: (ATARI_RGB[PENS[p]][0] - r) ** 2 * 3 +
               (ATARI_RGB[PENS[p]][1] - g) ** 2 * 4 + (ATARI_RGB[PENS[p]][2] - b) ** 2 * 2)


# ---------------------------------------------------------------------------
# 3 x 5 font, ASCII 32..95 (upper case; the game prints lower case as upper).
FONT = {
    'A': "010 101 111 101 101", 'B': "110 101 110 101 110", 'C': "011 100 100 100 011",
    'D': "110 101 101 101 110", 'E': "111 100 110 100 111", 'F': "111 100 110 100 100",
    'G': "011 100 101 101 011", 'H': "101 101 111 101 101", 'I': "111 010 010 010 111",
    'J': "001 001 001 101 010", 'K': "101 101 110 101 101", 'L': "100 100 100 100 111",
    'M': "101 111 111 101 101", 'N': "110 101 101 101 101", 'O': "010 101 101 101 010",
    'P': "110 101 110 100 100", 'Q': "010 101 101 110 011", 'R': "110 101 110 101 101",
    'S': "011 100 010 001 110", 'T': "111 010 010 010 010", 'U': "101 101 101 101 111",
    'V': "101 101 101 101 010", 'W': "101 101 111 111 101", 'X': "101 101 010 101 101",
    'Y': "101 101 010 010 010", 'Z': "111 001 010 100 111",
    '0': "111 101 101 101 111", '1': "010 110 010 010 111", '2': "110 001 010 100 111",
    '3': "110 001 010 001 110", '4': "101 101 111 001 001", '5': "111 100 110 001 110",
    '6': "011 100 111 101 111", '7': "111 001 010 010 010", '8': "111 101 111 101 111",
    '9': "111 101 111 001 110",
    ' ': "000 000 000 000 000", '!': "010 010 010 000 010", '"': "101 101 000 000 000",
    '#': "101 111 101 111 101", '$': "011 110 010 011 110", '%': "101 001 010 100 101",
    '&': "010 101 010 101 011", "'": "010 010 000 000 000", '(': "001 010 010 010 001",
    ')': "100 010 010 010 100", '*': "000 101 010 101 000", '+': "000 010 111 010 000",
    ',': "000 000 000 010 100", '-': "000 000 111 000 000", '.': "000 000 000 000 010",
    '/': "001 001 010 100 100", ':': "000 010 000 010 000", ';': "000 010 000 010 100",
    '<': "001 010 100 010 001", '=': "000 111 000 111 000", '>': "100 010 001 010 100",
    '?': "110 001 010 000 010", '@': "010 101 111 100 011", '[': "011 010 010 010 011",
    '\\': "100 100 010 001 001", ']': "110 010 010 010 110", '^': "010 101 000 000 000",
    '_': "000 000 000 000 111",
}


def glyph(c):
    rows = FONT.get(chr(32 + c), FONT['?']).split()
    return [[1 if ch == '1' else 0 for ch in r] for r in rows]


# ---------------------------------------------------------------------------
def halve(rows):
    """Two lines ORed into one (an odd last line stands alone)."""
    out = []
    for i in range(0, len(rows), 2):
        a = rows[i]
        b = rows[i + 1] if i + 1 < len(rows) else [0] * len(a)
        out.append([1 if (x or y) else 0 for x, y in zip(a, b)])
    return out


def raw(rows, w):
    """1 bit a pixel, MSB left, (w + 7) / 8 bytes a line."""
    wb = (w + 7) // 8
    out = []
    for r in rows:
        r = list(r[:w]) + [0] * (wb * 8 - len(r[:w]))
        out += hgr.pack_row(r)
    return out


def bits_rows(img):
    """An image of lists of 0/1 as it is."""
    return [[1 if v else 0 for v in r] for r in img]


# ---------------------------------------------------------------------------
def town_pens(atari):
    """The town, 96 x 144 Atari colours -> 96 x 72 pens. Of two lines, a
    pixel keeps the rarer colour (the detail: a line of the fence, a sign)."""
    img = dos.town_colours(atari)
    count = {}
    for r in img:
        for c in r:
            count[c] = count.get(c, 0) + 1
    rows = []
    for y in range(0, 144, 2):
        a, b = img[y], img[y + 1]
        rows.append([p if count.get(p, 0) <= count.get(q, 0) else q for p, q in zip(a, b)])
    cache = {}
    out = []
    for r in rows:
        line = []
        for c in r:
            if c not in cache:
                cache[c] = nearest_pen(c)
            line.append(cache[c])
        out.append(line)
    return out


def c_bytes(vals, per=16, indent="    "):
    return dos.c_bytes(vals, per, indent)


def preview(path, town, pal):
    try:
        from PIL import Image
    except ImportError:
        return
    im = Image.new("RGB", (160, 102), (0, 0, 0))
    for y, r in enumerate(town):
        for x, p in enumerate(r):
            im.putpixel((32 + x, 10 + y), tuple(v * 17 for v in pal[p]))
    for i in range(64):
        g = glyph(i)
        for y, r in enumerate(g):
            for x, v in enumerate(r):
                if v:
                    im.putpixel(((i % 40) * 4 + x, 90 + (i // 40) * 6 + y), (255, 255, 255))
    im.resize((160 * 4, 102 * 4), Image.NEAREST).save(path)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--atr")
    ap.add_argument("--preview")
    ap.add_argument("out")
    a = ap.parse_args()
    atari = hgr.Atari(a.atr) if a.atr else None

    src = ["/* gfx.c -- GENERATED by tools/mkgfx_lynx.py (%s). Do not edit or commit. */" %
           ("converted from the Atari disk" if atari else "STAND-IN art: no ATR given"),
           "#include <stdint.h>", '#include "../../src/lynx/gfxdata.h"', ""]

    pal = [ATARI_RGB[c] for c in PENS]
    src.append("/* the pens: 16 green, then 16 blue << 4 | red (Mikey's order) */")
    src.append("const uint8_t lynx_pal[32] = {")
    src.append(c_bytes([g for (r, g, b) in pal] + [(b << 4) | r for (r, g, b) in pal]))
    src.append("};")

    src.append("/* the font: 3 x 5, ASCII 32..95, a byte a line */")
    src.append("const uint8_t lynx_font[64][5] = {")
    for c in range(64):
        src.append("  {" + ",".join("0x%02X" % v for v in raw(glyph(c), 3)) + "}, /* %r */" % chr(32 + c))
    src.append("};")

    parts = dos.plot_images(atari)
    order = ["border", "icon_house", "icon_food", "icon_energy", "icon_smithore", "icon_crystite",
             "icon_mule", "mount1", "mount2", "mount3", "store", "river"]
    src.append("/* plot parts: 16 x 16, 2 bytes a line (the Atari's 16 x 32) */")
    src.append("const uint8_t plot_spr[%d][32] = {" % len(order))
    for n in order:
        src.append("  { /* %s */" % n)
        src.append(c_bytes(raw(halve(bits_rows(parts[n])), 16)))
        src.append("  },")
    src.append("};")
    src.append("/* production dots: Lynx line (in the plot), x */")
    src.append("const uint8_t plot_dots[8][2] = {")
    src.append("    " + " ".join("{%d,%d}," % (y // 2, x) for (y, x) in dos.plot_dots()))
    src.append("};")

    shs = dos.shapes(atari)
    src.append("/* sprites: 1 bit a pixel, half the Atari's height */")
    for s in shs:
        data = []
        for f in s.frames:
            data += raw(halve(bits_rows(f)), s.w)
        src.append("static const uint8_t sh_%s[%d] = {" % (s.name, len(data)))
        src.append(c_bytes(data))
        src.append("};")
    src.append("const ShapeDef lynx_shapes[%d] = {" % len(shs))
    for s in shs:
        h = (s.h + 1) // 2
        wb = (s.w + 7) // 8
        src.append("    { sh_%s, %d, %d, %d, %d }, /* %s */" % (s.name, wb * h, wb, s.w, h, s.name))
    src.append("};")

    src.append("/* pictures: 8 x 8 (a line each of the Atari's two) */")
    src.append("const uint8_t lynx_pics[%d][8] = {" % len(hgr.PICS))
    for n, art in hgr.PICS:
        src.append("    { %s }, /* %s */" % (",".join("0x%02X" % v for v in raw(intv.from_art(art), 8)), n))
    src.append("};")

    town = town_pens(atari)
    packed = dos.rle([(r[x] << 4) | r[x + 1] for r in town for x in range(0, 96, 2)])
    src.append("/* the town, 72 lines x 48 bytes (2 pixels a byte, left in the high")
    src.append(" * nibble), packed: n < 128: n + 1 bytes follow; n >= 128: the next")
    src.append(" * byte n - 125 times. */")
    src.append("const uint8_t town_lx[%d] = {" % len(packed))
    src.append(c_bytes(packed))
    src.append("};")
    open(a.out, "w").write("\n".join(src) + "\n")

    if a.preview:
        os.makedirs(a.preview, exist_ok=True)
        preview(os.path.join(a.preview, "town.png"), town, pal)
    print("%s: %s" % (a.out, "Atari graphics" if atari else "STAND-IN art"))


if __name__ == "__main__":
    main()

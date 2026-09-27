#!/usr/bin/env python3
"""Build the MS-DOS client's graphics (gen/msdos/gfx.c) for CGA and PCjr.

usage: mkgfx_dos.py [--atr "M.U.L.E. (1983)(Electronic Arts)(US).atr"] [--preview dir] out.c

The art comes from the original Atari disk, as for the Apple II and the CoCo
(the extraction is tools/mkgfx_hgr.py's and the Intellivision client's).
Without --atr, stand-in art is used and the Makefile warns. The generated
file must not be committed.

No scaling is needed: 320 pixels across is exactly two a colour clock, so
the tables are the Atari's own. Everything but the town is 1 bit a colour
clock (the font 1 bit a pixel: the Namco font, as on the TMS9918 ports),
coloured when drawn (src/msdos/gfx.c); the town is packed RGBI, a byte a clock, which CGA
turns into its palette 1 (1 cyan, 2 magenta, 3 white) as it draws.
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import mkgfx_hgr as hgr  # noqa: E402  (the Atari extraction, plots, shapes)

intv = hgr.intv

# RGBI, for the previews.
RGBI = [(0, 0, 0), (0, 0, 170), (0, 170, 0), (0, 170, 170), (170, 0, 0), (170, 0, 170),
        (170, 85, 0), (170, 170, 170), (85, 85, 85), (85, 85, 255), (85, 255, 85),
        (85, 255, 255), (255, 85, 85), (255, 85, 255), (255, 255, 85), (255, 255, 255)]
CGA = [0, 11, 13, 15]           # palette 1, high intensity (colour 0: the background)

# Atari colours seen in the town -> PCjr RGBI.
ATARI_TO_RGBI = {0x00: 0, 0x04: 8, 0x08: 8, 0x0C: 7, 0x0E: 15, 0x1A: 14,
                 0x24: 6, 0x34: 12, 0x44: 4, 0x54: 13,
                 0x84: 9, 0x86: 9, 0xB4: 2, 0xC4: 2, 0xD4: 10, 0xF4: 10}


def lookup(table, c, default):
    return table.get(c, table.get(c & 0xF0 | 4, default))


# ---------------------------------------------------------------------------
def font_dos():
    """64 glyphs (ASCII 32..95) x 8 rows, 8 pixels, bit 7 left: the Namco
    arcade font the TMS9918 ports use (tools/tms/namco.bin)."""
    with open(os.path.join(HERE, "tms", "namco.bin"), "rb") as f:
        d = f.read()
    return [list(d[i * 8:i * 8 + 8]) for i in range(64)]


def plot_images(atari):
    keep = hgr.plot_bytes
    hgr.plot_bytes = lambda img: [list(r) for r in img]    # the images, unencoded
    try:
        parts, _ = hgr.plot_parts(atari)
    finally:
        hgr.plot_bytes = keep
    return parts


def pack_bits(r, nbytes):
    v = 0
    for k, b in enumerate(r):
        if b:
            v |= 1 << (nbytes * 8 - 1 - k)
    return [(v >> (8 * (nbytes - 1 - i))) & 255 for i in range(nbytes)]


def plot_dots():
    """(line, colour clock) of the 8 production dots (1 x 2 each)."""
    return [(22 + (i // 4) * 4, 2 + (i % 4) * 2 - (0 if i < 4 else 1)) for i in range(8)]


def shapes(atari):
    out = []
    for s in hgr.shapes(atari):
        if s.name == "cursor":          # a plot's outline, 16 clocks here
            cur = [[1] * 16] * 2 + [[1] + [0] * 14 + [1] for _ in range(28)] + [[1] * 16] * 2
            s = hgr.Shape("cursor", [cur], 16, 32)
        out.append(s)
    return out


# ---------------------------------------------------------------------------
def town_colours(atari):
    """The town: 144 lines x 96 clocks of Atari colours."""
    if atari:
        return atari.town()
    img = [[0x00] * 96 for _ in range(144)]
    pal = {4: 0xC4, 11: 0x24, 5: 0xD4, 2: 0x44, 8: 0x0C, 7: 0x0C, 0: 0x00}
    for name, (x, y, w, h, bg, fg, sign) in intv.TOWN_FALLBACK.items():
        for yy in range(y * 16, (y + h) * 16):
            for xx in range(x * 8, (x + w) * 8):
                img[yy][xx] = pal.get(bg, 0x0C)
    return img


# RGBI -> CGA palette 1 index, for making the CGA town from the PCjr's
# (src/msdos/gfx.c does it as it draws): its colours only.
RGBI_TO_CGA = [0, 1, 1, 1, 2, 2, 2, 3, 3, 1, 1, 1, 2, 2, 3, 3]


def corral(img):
    """The corral's grey box (x0, y0, x1, y1), or a box that never matches."""
    box = [(x, y) for y in range(64, 144) for x in range(64, 96) if img[y][x] == 0x08]
    if not box:
        return (255, 255, 0, 0)
    return (min(p[0] for p in box), min(p[1] for p in box),
            max(p[0] for p in box), max(p[1] for p in box))


def town_cga(img):
    """As src/msdos/gfx.c makes it from town_jr (for the preview)."""
    x0, y0, x1, y1 = corral(img)
    cols = []
    for y, r in enumerate(img):
        row = []
        for x, c in enumerate(r):
            v = RGBI_TO_CGA[lookup(ATARI_TO_RGBI, c, 7)]
            # the corral: a fence, as on the Apple (a white box swamps the screen)
            if c == 0x08 and x0 + 1 < x < x1 - 1 and y0 + 1 < y < y1 - 1:
                v = 0
            row.append(v)
        cols.append(row)
    rows = []
    for r in cols:
        row = []
        for i in range(0, 96, 2):       # 2 clocks = 4 pixels = a byte
            a, b = r[i], r[i + 1]
            row.append(a << 6 | a << 4 | b << 2 | b)
        rows.append(row)
    return rows, cols


def town_jr(img):
    cols = [[lookup(ATARI_TO_RGBI, c, 7) for c in r] for r in img]
    return [[c << 4 | c for c in r] for r in cols], cols


def rle(data):
    """Runs of 3..130 equal bytes: 128 + n - 3, byte. Else up to 128 bytes
    as they are: n - 1, bytes."""
    out, lit, i = [], [], 0

    def flush():
        while lit:
            part = lit[:128]
            del lit[:128]
            out.append(len(part) - 1)
            out.extend(part)
    while i < len(data):
        n = 1
        while i + n < len(data) and data[i + n] == data[i] and n < 130:
            n += 1
        if n >= 3:
            flush()
            out += [128 + n - 3, data[i]]
            i += n
        else:
            lit.append(data[i])
            i += 1
    flush()
    return out


def c_bytes(vals, per=16, indent="    "):
    return "\n".join(indent + ",".join("0x%02X" % v for v in vals[i:i + per]) + ","
                     for i in range(0, len(vals), per))


# ---------------------------------------------------------------------------
def preview(path, town_cols, pal, font, parts, bg):
    try:
        from PIL import Image
    except ImportError:
        return
    im = Image.new("RGB", (320, 200), pal[bg] if bg is not None else (0, 0, 0))
    for y, r in enumerate(town_cols):
        for x, c in enumerate(r):
            for k in (0, 1):
                im.putpixel((64 + 2 * x + k, 4 + 16 + y), pal[c])
    white = pal[-1]
    for i, g in enumerate(font):
        for y, v in enumerate(g):
            for k in range(8):
                if v & (0x80 >> k):
                    im.putpixel(((i % 40) * 8 + k, 4 + 164 + (i // 40) * 9 + y), white)
    im.resize((320 * 3, 200 * 3), Image.NEAREST).save(path)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--atr")
    ap.add_argument("--preview")
    ap.add_argument("out")
    a = ap.parse_args()
    atari = hgr.Atari(a.atr) if a.atr else None

    src = ["/* gfx.c -- GENERATED by tools/mkgfx_dos.py (%s). Do not edit or commit. */" %
           ("converted from the Atari disk" if atari else "STAND-IN art: no ATR given"),
           "#include <stdint.h>", '#include "../../src/msdos/gfxdata.h"', ""]

    font = font_dos()
    src.append("const uint8_t dos_font[64 * 8] = {")
    src.append(c_bytes([b for g in font for b in g]))
    src.append("};")

    parts = plot_images(atari)
    order = ["border", "icon_house", "icon_food", "icon_energy", "icon_smithore", "icon_crystite",
             "icon_mule", "mount1", "mount2", "mount3", "store", "river"]
    src.append("/* plot parts: 32 lines x 16 colour clocks (2 bytes, MSB left) */")
    src.append("const uint8_t plot_parts[%d][64] = {" % len(order))
    for n in order:
        src.append("  { /* %s */" % n)
        src.append(c_bytes([b for r in parts[n] for b in pack_bits(r, 2)]))
        src.append("  },")
    src.append("};")
    src.append("/* production dots: line, colour clock */")
    src.append("const uint8_t plot_dots[8][2] = {")
    src.append("    " + " ".join("{%d,%d}," % d for d in plot_dots()))
    src.append("};")

    shs = shapes(atari)
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
    src.append("const ShapeDef dos_shapes[%d] = {" % len(shs))
    for s in shs:
        src.append("    { sh_%s, %d, %d, %d }," % (s.name, (s.w + 7) // 8, s.w, s.h))
    src.append("};")

    src.append("const uint8_t dos_pics[%d][8] = {" % len(hgr.PICS))
    for n, art in hgr.PICS:
        src.append("    { %s }, /* %s */" % (",".join("0x%02X" % v for r in intv.from_art(art)
                                                   for v in hgr.pack_row(r)), n))
    src.append("};")

    img = town_colours(atari)
    cga, cga_cols = town_cga(img)
    jr, jr_cols = town_jr(img)
    packed = rle([b for r in jr for b in r])
    src.append("/* the town, 144 lines x 96 bytes a line, 4 bits a pixel (RGBI), packed:")
    src.append(" * n < 128: n + 1 bytes follow; n >= 128: the next byte n - 125 times.")
    src.append(" * CGA makes its own from it (rgbi_cga, and the corral's box). */")
    src.append("const uint8_t __far town_jr[%d] = {" % len(packed))
    src.append(c_bytes(packed))
    src.append("};")
    src.append("const uint8_t rgbi_cga[16] = { %s };" % ", ".join(str(v) for v in RGBI_TO_CGA))
    src.append("const uint8_t town_corral[4] = { %d, %d, %d, %d };  /* x0 y0 x1 y1 */"
               % corral(img))
    open(a.out, "w").write("\n".join(src) + "\n")

    if a.preview:
        os.makedirs(a.preview, exist_ok=True)
        preview(os.path.join(a.preview, "town-cga.png"), cga_cols,
                [RGBI[c] for c in CGA], font, parts, 0)
        preview(os.path.join(a.preview, "town-jr.png"), jr_cols, RGBI, font, parts, 0)
    print("%s: %s" % (a.out, "Atari graphics" if atari else "STAND-IN art"))


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Build the TMS9918A clients' graphics (gen/tms9918/gfx.c): the Coleco Adam
and the MSX share it.

usage: mkgfx_tms.py [--atr "M.U.L.E. (1983)(Electronic Arts)(US).atr"] [--preview dir] out.c

The art comes from the original Atari disk, as for the other ports (the
extraction is tools/mkgfx_hgr.py's and the Intellivision client's). Without
--atr, stand-in art is used and the Makefile warns. The generated file must
not be committed.

The screen is Graphics II, 256 x 192, two colours in each 8-pixel row of a
cell. x is scaled 3/2 from the Atari's colour clocks, 4 pixels in (see
src/tms9918/gfx.c): clock c covers pixels 1.5c .. 1.5(c + 1), rounded
down. So:
  - the font is the Namco arcade font the other Adam FujiNet games use
    (tools/tms/namco.bin, from fujinet-5cardstud's src/adam/font.bin:
    characters 32..127, 8 x 8), a whole cell a character;
  - the plot parts are 24 pixels (three cells) by 32 lines;
  - the shapes stay 1 bit a clock: gfx.c widens them with wide12 as it
    makes the sprites' patterns;
  - the town is made here, cell by cell, in VRAM order: each 8-pixel row
    keeps its two commonest colours and the rest go to the nearer.
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import mkgfx_hgr as hgr  # noqa: E402  (the Atari extraction, plots, shapes)
import mkgfx_dos as dos  # noqa: E402  (plot images, the town's colours, rle)

intv = hgr.intv

# The TMS9918A's colours (for the previews and choosing the nearest).
TMS = [(0, 0, 0), (0, 0, 0), (33, 200, 66), (94, 220, 120), (84, 85, 237), (125, 118, 252),
       (212, 82, 77), (66, 235, 245), (252, 85, 84), (255, 121, 120), (212, 193, 84),
       (230, 206, 128), (33, 176, 59), (201, 91, 186), (204, 204, 204), (255, 255, 255)]

# Atari colours seen in the town -> TMS colours.
ATARI_TO_TMS = {0x00: 1, 0x04: 1, 0x08: 14, 0x0C: 14, 0x0E: 15, 0x1A: 11,
                0x24: 10, 0x34: 9, 0x44: 8, 0x54: 13,
                0x84: 5, 0x86: 5, 0xB4: 2, 0xC4: 2, 0xD4: 3, 0xF4: 3}

# Colours the game draws with (src/tms9918/gfx.c tms_of), for the preview.
T_BLACK, T_WHITE, T_PLAINS, T_RIVER = 1, 15, 14, 3


def px_of(c):
    """The first pixel of colour clock c (relative: without the 4 in)."""
    return (3 * c) // 2


def widen_row(bits):
    """A row of clocks (0/1) -> its pixels."""
    out = [0] * px_of(len(bits))
    for c, b in enumerate(bits):
        for p in range(px_of(c), px_of(c + 1)):
            out[p] = b
    return out


def pack(pixels):
    """Pixels (0/1) -> bytes, MSB left."""
    pixels = list(pixels) + [0] * (-len(pixels) % 8)
    return [sum(pixels[i + k] << (7 - k) for k in range(8)) for i in range(0, len(pixels), 8)]


def wide12():
    out = []
    for v in range(256):
        px = widen_row([(v >> (7 - k)) & 1 for k in range(8)])
        out.append(sum(b << (11 - k) for k, b in enumerate(px)))
    return out


def plot_dots():
    """(line, pixel) of the 8 production dots, 2 pixels by 2 lines."""
    return [(line, px_of(c)) for line, c in dos.plot_dots()]


# ---------------------------------------------------------------------------
def nearest(c, choices):
    r, g, b = TMS[c]
    return min(choices, key=lambda k: (TMS[k][0] - r) ** 2 + (TMS[k][1] - g) ** 2
               + (TMS[k][2] - b) ** 2)


def town_pixels(atari):
    """The town as TMS colours, 144 lines of the 152 pixels of cells 6..24."""
    img = dos.town_colours(atari)
    rows = []
    for r in img:
        line = [T_BLACK] * 152
        for c, a in enumerate(r):
            t = dos.lookup(ATARI_TO_TMS, a, 14)
            for p in range(px_of(32 + c), px_of(33 + c)):
                line[p + 4 - 48] = t
        rows.append(line)
    return rows


def town_cells(rows):
    """Two colours an 8-pixel row: (pattern, colour) bytes in VRAM order, and
    the pixels as they will show (for the preview)."""
    shown = [list(r) for r in rows]
    stream = []
    clash = 0
    for cy in range(18):
        pat, col = [], []
        for cx in range(19):
            for l in range(8):
                y = cy * 8 + l
                px = rows[y][cx * 8:cx * 8 + 8]
                count = {}
                for c in px:
                    count[c] = count.get(c, 0) + 1
                order = sorted(count, key=lambda c: (-count[c], c))
                fg = order[0]
                bg = order[1] if len(order) > 1 else fg
                clash += sum(count[c] for c in order[2:])
                bits = 0
                for k, c in enumerate(px):
                    c = c if c in (fg, bg) else nearest(c, (fg, bg))
                    shown[y][cx * 8 + k] = c
                    if c == fg and fg != bg:
                        bits |= 0x80 >> k
                pat.append(bits)
                col.append(fg << 4 | bg)
        stream += pat[:]
        stream += col[:]
    # pat/col were made cell by cell, 8 lines each: VRAM's order within a
    # cell row too (cell * 8 + line)
    return stream, shown, clash


def c_bytes(vals, per=16, indent="    "):
    return dos.c_bytes(vals, per, indent)


# ---------------------------------------------------------------------------
def namco_font():
    """Characters 32..95 of the Namco font (upper case is all the game uses)."""
    with open(os.path.join(HERE, "tms", "namco.bin"), "rb") as f:
        d = f.read()
    return [list(d[i * 8:i * 8 + 8]) for i in range(64)]


def preview(path, town_shown, font, parts, shapes):
    try:
        from PIL import Image
    except ImportError:
        return
    im = Image.new("RGB", (256, 192), TMS[T_BLACK])
    for y, r in enumerate(town_shown):
        for x, c in enumerate(r):
            im.putpixel((48 + x, 16 + y), TMS[c])
    # the font, a cell a character, under the town
    for i, g in enumerate(font):
        for y, v in enumerate(g):
            for k in range(8):
                if v & (0x80 >> k):
                    im.putpixel(((i % 32) * 8 + k, 168 + (i // 32) * 9 + y), TMS[T_WHITE])
    im.resize((256 * 3, 192 * 3), Image.NEAREST).save(path)
    # the plot parts on the map's grey
    im = Image.new("RGB", (len(parts) * 28, 36), TMS[T_PLAINS])
    for n, (name, rows) in enumerate(parts):
        for y, r in enumerate(rows):
            for x, b in enumerate(r):
                if b:
                    im.putpixel((n * 28 + 2 + x, 2 + y),
                                TMS[T_RIVER if name == "river" else T_WHITE if name == "store"
                                    else T_BLACK if name.startswith("mount") else 5])
    im.resize((im.width * 3, im.height * 3), Image.NEAREST).save(
        os.path.join(os.path.dirname(path), "plots.png"))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--atr")
    ap.add_argument("--preview")
    ap.add_argument("out")
    a = ap.parse_args()
    atari = hgr.Atari(a.atr) if a.atr else None

    src = ["/* gfx.c -- GENERATED by tools/mkgfx_tms.py (%s). Do not edit or commit. */" %
           ("converted from the Atari disk" if atari else "STAND-IN art: no ATR given"),
           "#include <stdint.h>", '#include "../../src/tms9918/gfxdata.h"', ""]

    font = namco_font()
    src.append("const uint8_t tms_font[64 * 8] = {")
    src.append(c_bytes([b for g in font for b in g]))
    src.append("};")

    images = dos.plot_images(atari)
    order = ["border", "icon_house", "icon_food", "icon_energy", "icon_smithore", "icon_crystite",
             "icon_mule", "mount1", "mount2", "mount3", "store", "river"]
    parts = [(n, [widen_row(r) for r in images[n]]) for n in order]
    src.append("/* plot parts: 32 lines x 24 pixels (3 bytes, MSB left) */")
    src.append("const uint8_t plot_parts[%d][96] = {" % len(order))
    for n, rows in parts:
        src.append("  { /* %s */" % n)
        src.append(c_bytes([b for r in rows for b in pack(r)]))
        src.append("  },")
    src.append("};")
    src.append("/* production dots: line, pixel */")
    src.append("const uint8_t plot_dots[8][2] = {")
    src.append("    " + " ".join("{%d,%d}," % d for d in plot_dots()))
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
    src.append("const ShapeDef tms_shapes[%d] = {" % len(shs))
    for s in shs:
        src.append("    { sh_%s, %d, %d, %d }," % (s.name, (s.w + 7) // 8, s.w, s.h))
    src.append("};")

    src.append("const uint8_t tms_pics[%d][8] = {" % len(hgr.PICS))
    for n, art in hgr.PICS:
        src.append("    { %s }, /* %s */" % (",".join("0x%02X" % v for r in intv.from_art(art)
                                                   for v in hgr.pack_row(r)), n))
    src.append("};")

    src.append("const uint16_t wide12[256] = {")
    w = wide12()
    src.append("\n".join("    " + ",".join("0x%03X" % v for v in w[i:i + 12]) + ","
                         for i in range(0, 256, 12)))
    src.append("};")

    rows = town_pixels(atari)
    stream, shown, clash = town_cells(rows)
    packed = dos.rle(stream)
    src.append("/* the town: 18 cell rows, each 152 pattern then 152 colour bytes, packed:")
    src.append(" * n < 128: n + 1 bytes follow; n >= 128: the next byte n - 125 times. */")
    src.append("const uint8_t town_rle[%d] = {" % len(packed))
    src.append(c_bytes(packed))
    src.append("};")
    open(a.out, "w").write("\n".join(src) + "\n")

    if a.preview:
        os.makedirs(a.preview, exist_ok=True)
        preview(os.path.join(a.preview, "town.png"), shown, font, parts, shs)
    print("%s: %s; town %d bytes packed, %d pixels recoloured" %
          (a.out, "Atari graphics" if atari else "STAND-IN art", len(packed), clash))


if __name__ == "__main__":
    main()

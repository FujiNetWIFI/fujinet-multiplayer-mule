#!/usr/bin/env python3
"""Build the CoCo client's graphics (gen/coco/gfx.c) for PMODE 4.

usage: mkgfx_pmode4.py [--atr "M.U.L.E. (1983)(Electronic Arts)(US).atr"] [--preview dir] out.c

The art comes from the original Atari disk, as for the Apple II (the
extraction is tools/mkgfx_hgr.py's and the Intellivision client's); only the
encoding is the CoCo's. Without --atr, stand-in art is used and the Makefile
warns. The generated file must not be committed.

PMODE 4, briefly: 256 x 192, 1 bit a pixel, 32 bytes a line. On a CoCo 1/2
into a TV, two neighbouring pixels make one artifact colour pixel (128
across): 00 black, 11 white, 01 and 10 the two artifact colours ("A" and
"B": blue and orange, which is which depends on the machine's phase). This
file stores colour art with A = 01 and B = 10; src/coco/gfx.c swaps them if
the player says the colours are the wrong way round.

Scaling: Atari colour clocks x become colour pixels 3x/4 + 2 (160 -> 120,
a plot 12 colour pixels = 3 bytes, starting on a byte; a text cell is a
byte, 32 columns). Sprites of characters keep their
width (8 colour pixels); the plot cursor, the ships and the cloud, which
line up with the map, are squeezed too. Scanlines are kept 1:1.
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import mkgfx_hgr as hgr  # noqa: E402  (the Atari extraction, plots, shapes)

intv = hgr.intv

# Colours as stored (2 bits a colour pixel).
K, A, B, W = 0, 1, 2, 3         # black, blue (01), orange (10), white
RGB = {K: (0, 0, 0), A: (40, 110, 255), B: (255, 110, 30), W: (255, 255, 255)}

# Atari colours seen in the town -> PMODE 4 colour.
ATARI_TO_PM = {0x00: K, 0x04: K, 0x08: W, 0x0C: W, 0x0E: W,
               0x24: B, 0x34: B, 0x44: B, 0x54: B, 0xF4: B, 0x1A: B,
               0x84: A, 0x86: A, 0xB4: A, 0xC4: A, 0xD4: A}


def squeeze(row):
    """3/4 across: of each 4 columns a b c d, keep a, b|c, d."""
    out = []
    for i in range(0, len(row), 4):
        g = list(row[i:i + 4]) + [0] * (4 - len(row[i:i + 4]))
        out += [g[0], g[1] | g[2], g[3]]
    return out


# ---------------------------------------------------------------------------
# Font: the Namco arcade font the TMS9918 clients use (tools/tms/namco.bin),
# 8 x 8, a byte a cell (32 columns). src/coco/gfx.c draws coloured text from
# a glyph's odd pixels (columns 1, 3, 5, 7: one a colour pixel), which the
# 2-pixel strokes always reach; a lone pixel in an even column moves one
# right so it isn't lost, and '#' (lone pixels both sides) is redrawn.
# ---------------------------------------------------------------------------
FONT_FIX = {
    "#": [0x00, 0x44, 0xFE, 0x44, 0xFE, 0x44, 0x00, 0x00],
}


def namco_font():
    """Characters 32..95 (upper case is all the game uses)."""
    with open(os.path.join(HERE, "tms", "namco.bin"), "rb") as f:
        d = f.read()
    out = []
    for i in range(64):
        g = FONT_FIX.get(chr(32 + i)) or list(d[i * 8:i * 8 + 8])
        rows = []
        for b in g:
            for c in (6, 4, 2, 0):          # bit 7 - c: column c (even)
                m = 0x80 >> c
                if b & m and not b & (m >> 1) and not b & (m << 1):
                    b = (b & ~m) | (m >> 1)
            rows.append(b & 0xFF)
        out.append(rows)
    return out


# ---------------------------------------------------------------------------
# Plots: the Apple tool's images (16 x 32), squeezed to 12 x 32, as 2 bytes
# a row (12 bits, MSB left). Every lit pixel is a colour pixel.
# ---------------------------------------------------------------------------
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
    """(line, colour pixel) of the 8 production dots (1 x 2 each)."""
    d = []
    for i in range(8):
        x = 2 + (i % 4) * 2 - (0 if i < 4 else 1)
        y = 22 + (i // 4) * 4
        d.append((y, squeeze([1 if k == x else 0 for k in range(16)]).index(1)))
    return d


# ---------------------------------------------------------------------------
# Sprites: 1 bit a colour pixel, MSB left. Squeezed shapes are marked.
# ---------------------------------------------------------------------------
SQUEEZE = {"nose", "hull_top", "hull_bot", "cloud"}


def shapes(atari):
    out = []
    for s in hgr.shapes(atari):
        if s.name in SQUEEZE:
            frames = [[squeeze(r) for r in f] for f in s.frames]
            s = hgr.Shape(s.name, frames, len(frames[0][0]), s.h)
        elif s.name == "cursor":
            cur = [[1] * 12] * 2 + [[1] + [0] * 10 + [1] for _ in range(28)] + [[1] * 12] * 2
            s = hgr.Shape("cursor", [cur], 12, 32)
        out.append(s)
    return out


# ---------------------------------------------------------------------------
# The town: 96 colour clocks x 144 lines -> 72 colour pixels, from colour
# pixel 26 (clock 32). That isn't on a byte, so a line is 19 bytes from byte
# 6 (colour pixel 24), with two black colour pixels at each end.
# ---------------------------------------------------------------------------
def town_pm(atari):
    if atari:
        img = atari.town()
        cols = [[ATARI_TO_PM.get(c, ATARI_TO_PM.get(c & 0xF0 | 4, W)) for c in r] for r in img]
        # the corral: a fence, as on the Apple (a white box swamps the screen)
        box = [(x, y) for y in range(64, 144) for x in range(64, 96) if img[y][x] == 0x08]
        if box:
            x0, x1 = min(p[0] for p in box), max(p[0] for p in box)
            y0, y1 = min(p[1] for p in box), max(p[1] for p in box)
            for (x, y) in box:
                if x0 + 1 < x < x1 - 1 and y0 + 1 < y < y1 - 1:
                    cols[y][x] = K
    else:
        cols = [[K] * 96 for _ in range(144)]
        pal = {4: A, 11: B, 5: A, 2: B, 8: W, 7: W, 0: K}
        for name, (x, y, w, h, bg, fg, sign) in intv.TOWN_FALLBACK.items():
            for yy in range(y * 16, (y + h) * 16):
                for xx in range(x * 8, (x + w) * 8):
                    cols[yy][xx] = pal.get(bg, W)
    rows = []
    for r in cols:
        px = [K, K]
        for i in range(0, 96, 4):
            g = r[i:i + 4]
            px += [g[0], g[1] or g[2], g[3]]     # squeeze, colour kept
        px += [K, K]
        row = []
        for b in range(19):
            v = 0
            for k in range(4):
                v = (v << 2) | px[b * 4 + k]
            row.append(v)
        rows.append(row)
    return rows


# ---------------------------------------------------------------------------
def render(screen, path, scale=3):
    """screen: {(y, byte): value}. Roughly as a TV shows it: a lit pixel
    next to another is white, a lone one takes its column's colour."""
    try:
        from PIL import Image
    except ImportError:
        return
    im = Image.new("RGB", (256, 192))
    for y in range(192):
        row = [0] * 258
        for b in range(32):
            v = screen.get((y, b), 0)
            for k in range(8):
                row[1 + b * 8 + k] = (v >> (7 - k)) & 1
        for x in range(256):
            if not row[x + 1]:
                continue
            if row[x] or row[x + 2]:
                c = W
            else:
                c = A if x & 1 else B
            im.putpixel((x, y), RGB[c])
            if c != W and x < 255 and not row[x + 2]:
                im.putpixel((x + 1, y), RGB[c])
    im.resize((256 * scale, 192 * scale), Image.NEAREST).save(path)


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


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--atr")
    ap.add_argument("--preview")
    ap.add_argument("out")
    a = ap.parse_args()
    atari = hgr.Atari(a.atr) if a.atr else None
    if a.preview:
        os.makedirs(a.preview, exist_ok=True)

    src = ["/* gfx.c -- GENERATED by tools/mkgfx_pmode4.py (%s). Do not edit or commit. */" %
           ("converted from the Atari disk" if atari else "STAND-IN art: no ATR given"),
           "#include <stdint.h>", '#include "../../src/coco/gfxdata.h"', ""]

    font = namco_font()
    src.append("const uint8_t pm_font[64 * 8] = {")
    src.append(c_bytes([b for g in font for b in g]))
    src.append("};")

    parts = plot_images(atari)
    order = ["border", "icon_house", "icon_food", "icon_energy", "icon_smithore", "icon_crystite",
             "icon_mule", "mount1", "mount2", "mount3", "store", "river"]
    sq = {n: [squeeze(r) for r in parts[n]] for n in order}
    src.append("/* plot parts: 32 lines x 12 colour pixels (2 bytes, MSB left) */")
    src.append("const uint8_t plot_parts[%d][64] = {" % len(order))
    for n in order:
        src.append("  { /* %s */" % n)
        src.append(c_bytes([b for r in sq[n] for b in pack_bits(r, 2)]))
        src.append("  },")
    src.append("};")
    dots = plot_dots()
    src.append("/* production dots: line, colour pixel */")
    src.append("const uint8_t plot_dots[8][2] = {")
    src.append("    " + " ".join("{%d,%d}," % d for d in dots))
    src.append("};")

    shs = shapes(atari)
    src.append("/* sprites: 1 bit a colour pixel, MSB left */")
    for s in shs:
        wb = (s.w + 7) // 8
        data = []
        for f in s.frames:
            for r in f:
                data += hgr.pack_row(list(r) + [0] * (wb * 8 - len(r)))
        src.append("static const uint8_t sh_%s[%d] = {" % (s.name, len(data)))
        src.append(c_bytes(data))
        src.append("};")
    src.append("const ShapeDef pm_shapes[%d] = {" % len(shs))
    for s in shs:
        src.append("    { sh_%s, %d, %d, %d }," % (s.name, (s.w + 7) // 8, s.w, s.h))
    src.append("};")

    src.append("const uint8_t pm_pics[%d][8] = {" % len(hgr.PICS))
    for n, art in hgr.PICS:
        src.append("    { %s }, /* %s */" % (",".join("0x%02X" % v for r in intv.from_art(art)
                                                   for v in hgr.pack_row(r)), n))
    src.append("};")
    open(a.out, "w").write("\n".join(src) + "\n")

    # The town is big: packed, in its own file (placed in low RAM).
    town = town_pm(atari)
    packed = rle([b for r in town for b in r])
    tsrc = [src[0], "#include <stdint.h>", "",
            "/* 144 lines x 19 bytes (from byte 6), 2 bits a colour pixel (01 blue,",
            " * 10 orange), packed: n < 128: n + 1 bytes follow; n >= 128: the next",
            " * byte n - 125 times. */",
            "const uint8_t pm_town[%d] = {" % len(packed), c_bytes(packed), "};"]
    open(os.path.join(os.path.dirname(a.out), "town.c"), "w").write("\n".join(tsrc) + "\n")

    if a.preview:
        scr = {}
        for y, r in enumerate(town):
            for b, v in enumerate(r):
                scr[(16 + y, 6 + b)] = v
        for i, g in enumerate(font):          # the font under the town
            for y, v in enumerate(g):
                x = (i % 32) * 8
                yy = 164 + (i // 32) * 9 + y
                for k in range(8):
                    if v & (0x80 >> k):
                        m = x + k
                        scr[(yy, m // 8)] = scr.get((yy, m // 8), 0) | (0x80 >> (m % 8))
        render(scr, os.path.join(a.preview, "town.png"))
        scr = {}
        demo = [("icon_house", A), ("mount2", None), ("icon_food", B), ("store", None),
                ("mount3", None), ("river", None), ("icon_crystite", "S"), ("icon_mule", W)]
        for p, (n, owner) in enumerate(demo):
            bx, py = 2 + 3 * p, 16
            for y in range(32):
                own = [0] * 12
                if owner is not None:
                    own = [u | v for u, v in zip(sq["border"][y], sq[n][y])]
                c = owner if owner != "S" else (A if y % 2 == 0 else B)
                row = []
                for x in range(12):
                    if own[x]:
                        row.append(c)
                    elif owner is None and n != "river" and sq[n][y][x]:
                        row.append(W)
                    elif n == "river" and sq[n][y][x]:
                        row.append(B)
                    else:
                        row.append(K)
                for b in range(3):
                    v = 0
                    for k in range(4):
                        v = (v << 2) | row[b * 4 + k]
                    scr[(py + y, bx + b)] = v
        render(scr, os.path.join(a.preview, "plots.png"))
    print("%s: %s" % (a.out, "Atari graphics" if atari else "STAND-IN art"))


if __name__ == "__main__":
    main()

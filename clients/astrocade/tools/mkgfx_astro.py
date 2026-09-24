#!/usr/bin/env python3
"""mkgfx_astro.py -- the Astrocade's graphics, from the Atari disk.

usage: mkgfx_astro.py [--atr mule.atr] [--preview DIR]

The pictures come off the Atari M.U.L.E. disk through the Apple II and
Intellivision clients' extraction (clients/mekkogx/tools/mkgfx_hgr.py,
clients/intv/tools/mkgfx.py): the same species frames, map figures,
plot characters, mountains, ship, pest, cloud and the town template. With
no disk they fall back to those tools' stand-in art, and the first line
of every output says so (a stand-in build looks like lost work).

The Astrocade's screen is the Atari's 160 colour clocks across, 1:1, but
only 78 lines of 2: everything is squeezed to 3/8 of its height (a plot's
32 lines become 12). Rows are OR-ed together so thin lines survive.

Output (zmac, never committed):
  gen/spr.inc   the sprite shapes, 1 bit a pixel, MSB left; SHTAB
  gen/map.inc   the plot layers (16 x 12, 1 bit a pixel), the production
                dots, the town (2 bits a pixel, RLE), the small pictures
  gen/gfx.inc   a header line only (the build's stand-in check reads it)
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "mekkogx", "tools"))
import mkgfx_hgr as hgr  # noqa: E402
intv = hgr.intv

# the Astrocade colour codes (palette order in mule.asm): 0 black, 1 blue,
# 2 orange, 3 white
BLACK, BLUE, ORANGE, WHITE = 0, 1, 2, 3
RGB = {BLACK: (0, 0, 0), BLUE: (28, 86, 255), ORANGE: (208, 95, 0), WHITE: (255, 255, 255)}


def atari_col(c):
    """An Atari colour of the town -> one of the four."""
    lum = c & 0x0E
    hue = c >> 4
    if c == 0 or lum == 0:
        return BLACK
    if hue == 0:
        return WHITE if lum >= 6 else BLACK
    if hue in (1, 2, 3, 4, 5, 6):
        return ORANGE
    return BLUE


def squeeze(rows, n_out=None):
    """rows (lists of 0/1) -> n_out rows (3/8 of the height), OR-ed."""
    h = len(rows)
    if n_out is None:
        n_out = max(1, (h * 3 + 4) // 8)
    w = len(rows[0]) if rows else 0
    out = []
    for r in range(n_out):
        a, b = r * h // n_out, max(r * h // n_out + 1, (r + 1) * h // n_out)
        out.append([1 if any(rows[k][x] for k in range(a, min(b, h))) else 0 for x in range(w)])
    return out


def pack1(row, wb):
    """0/1 pixels -> wb bytes, MSB left."""
    out = []
    for b in range(wb):
        v = 0
        for i in range(8):
            x = b * 8 + i
            if x < len(row) and row[x]:
                v |= 0x80 >> i
        out.append(v)
    return out


def db_lines(vals, per=16):
    return ["        DB      " + ",".join("%03XH" % v for v in vals[i:i + per])
            for i in range(0, len(vals), per)]


# ---------------------------------------------------------------------------
SHAPE_ORDER = ["species", "figure", "mule", "wampus", "cursor", "nose", "hull_top", "hull_bot",
               "dock", "pest", "cloud", "bolt", "spark", "meteor"]


def sprites(atari):
    shapes = {s.name: s for s in hgr.shapes(atari)}
    out = {}
    for n in SHAPE_ORDER:
        s = shapes[n]
        wb = (s.w + 7) // 8
        h = max(1, (s.h * 3 + 4) // 8)
        frames = [squeeze(f, h) for f in s.frames]
        out[n] = (wb, h, frames)
    return out


def plot_layers(atari):
    """The plot's parts as 16 x 32 bitmaps (hgr.plot_parts, before encoding)."""
    mc = (lambda c: [hgr.bits(b) for b in atari.mapchar_bytes(c)]) if atari else None
    blank, paste = hgr.blank_img, hgr.paste
    if atari:
        border = blank()
        for i, c in enumerate((0x1D, 0x1E, 0x1E, 0x1F)):
            paste(border, mc(c), 0, i * 8)
        paste(border, mc(0x20), 8, 0)
        paste(border, mc(0x22), 8, 24)
        for y in range(8, 24):
            border[y][15] = 1
        codes = {"house": (0x11, 0x17), "food": (0x12, 0x18), "energy": (0x13, 0x19),
                 "smithore": (0x14, 0x1A), "crystite": (0x15, 0x1B), "mule": (0x16, 0x1C)}
        icons = {}
        for n, (a, b) in codes.items():
            img = blank()
            ra, rb = mc(a), mc(b)
            for y in range(8):
                ra[y][7] = rb[y][7] = 0
            paste(img, ra, 8, 8)
            paste(img, rb, 8, 16)
            icons[n] = img
        mshapes = atari.mountains()
    else:
        border = blank()
        for x in range(16):
            border[0][x] = border[1][x] = border[30][x] = border[31][x] = 1
        for y in range(32):
            border[y][0] = border[y][15] = 1
        icons = {}
        for n, art in intv.INTY_ICONS.items():
            img = blank()
            paste(img, [r for r in intv.from_art(art) for _ in (0, 1)], 8, 8)
            icons[n] = img
        mshapes = intv.MOUNTAINS_FALLBACK
    layers = [("border", border)]
    for n in ("house", "food", "energy", "smithore", "crystite", "mule"):
        layers.append(("icon_" + n, icons[n]))
    for m in (1, 2, 3):
        img = blank()
        for (shape, x, y) in intv.mountain_layout(mshapes, m):
            paste(img, shape, x + 1, y + 2)
        layers.append(("mount%d" % m, img))
    store = intv.from_art(intv.STORE_ART, 16)
    layers.append(("store", [r for r in store for _ in (0, 1)]))
    river = intv.from_art(intv.RIVER_ART)
    rimg = blank()
    for y in range(32):
        paste(rimg, [river[(y // 2) % 16]], 4, y)
    layers.append(("river", rimg))
    return layers


def dots():
    """Production dots: two rows of four, bottom left (Atari: 1 cc x 2 lines
    at x 2,4,6,8 / 1,3,5,7, lines 22 and 26) -> (line, 16-bit mask)."""
    out = []
    for i in range(8):
        x = 2 + (i % 4) * 2 - (0 if i < 4 else 1)
        y = 22 + (i // 4) * 4
        out.append((y * 12 // 32, 0x8000 >> x))
    return out


def town(atari):
    if atari:
        img = atari.town()
        cols = [[atari_col(c) for c in r] for r in img]
        box = [(x, y) for y in range(64, 144) for x in range(64, 96) if img[y][x] == 0x08]
        if box:     # the corral: a fence, not a solid grey box
            x0, x1 = min(p[0] for p in box), max(p[0] for p in box)
            y0, y1 = min(p[1] for p in box), max(p[1] for p in box)
            for (x, y) in box:
                if x0 + 1 < x < x1 - 1 and y0 + 1 < y < y1 - 1:
                    cols[y][x] = BLACK
    else:
        cols = [[BLACK] * 96 for _ in range(144)]
        pal = {4: BLUE, 11: ORANGE, 5: BLUE, 2: ORANGE, 8: WHITE, 7: WHITE, 0: BLACK}
        for name, (x, y, w, h, bg, fg, sign) in intv.TOWN_FALLBACK.items():
            for yy in range(y * 16, (y + h) * 16):
                for xx in range(x * 8, (x + w) * 8):
                    cols[yy][xx] = pal.get(bg, WHITE)
    n_out = 54
    rows = []
    for r in range(n_out):
        a, b = r * 144 // n_out, (r + 1) * 144 // n_out
        row = []
        for x in range(96):
            v = BLACK
            for k in range(a, b):       # the last non-black line of the group
                if cols[k][x] != BLACK:
                    v = cols[k][x]
            row.append(v)
        rows.append(row)
    return rows


def pack2(row):
    out = []
    for b in range(len(row) // 4):
        v = 0
        for i in range(4):
            v |= row[b * 4 + i] << (6 - 2 * i)
        out.append(v)
    return out


def rle(data):
    """n < 128: n+1 literal bytes follow; n >= 128: the next byte n-125 times."""
    out, i = [], 0
    while i < len(data):
        j = i
        while j < len(data) and data[j] == data[i] and j - i < 130:
            j += 1
        if j - i >= 3:
            out += [0x80 + (j - i) - 3, data[i]]
            i = j
            continue
        k = i
        lit = []
        while k < len(data) and len(lit) < 128:
            if k + 2 < len(data) and data[k] == data[k + 1] == data[k + 2]:
                break
            lit.append(data[k])
            k += 1
        out += [len(lit) - 1] + lit
        i = k
    return out


def preview(path, draw):
    try:
        from PIL import Image
    except ImportError:
        return
    im = Image.new("RGB", (160, 78))
    draw(im)
    im.resize((160 * 4, 78 * 8), Image.NEAREST).save(path)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--atr", default="mule.atr" if os.path.exists("mule.atr") else None)
    ap.add_argument("--preview", default="gen/preview")
    a = ap.parse_args()
    atari = None
    if a.atr and os.path.exists(a.atr):
        atari = hgr.Atari(a.atr)
    what = "converted from the Atari disk" if atari else "stand-in art: NO ATR (mule.atr)"
    os.makedirs("gen", exist_ok=True)
    head = "; GENERATED by tools/mkgfx_astro.py -- %s. Do not edit or commit." % what

    # ---- sprites
    sp = sprites(atari)
    L = [head, "", "; SHTAB: per shape {bytes a row, rows, frame bytes, DW data}"]
    L.append("SHTAB:")
    for n in SHAPE_ORDER:
        wb, h, fr = sp[n]
        L.append("        DB      %d,%d,%d" % (wb, h, wb * h))
        L.append("        DW      SHD_%s" % n.upper())
    for n in SHAPE_ORDER:
        wb, h, fr = sp[n]
        L.append("SHD_%s:  ; %d frames, %d x %d" % (n.upper(), len(fr), wb * 8, h))
        data = [b for f in fr for r in f for b in pack1(r, wb)]
        L += db_lines(data)
    L.append("REV8:   ; each byte mirrored (a flipped sprite)")
    L += db_lines([int("{:08b}".format(v)[::-1], 2) for v in range(256)])
    open("gen/spr.inc", "w").write("\n".join(L) + "\n")

    # ---- the map, the town, the pictures
    L = [head, ""]
    layers = plot_layers(atari)
    L.append("; plot layers: 16 x 12, 2 bytes a line")
    for n, img in layers:
        rows = squeeze(img, 12)
        L.append("PL_%s:" % n.upper())
        L += db_lines([b for r in rows for b in pack1(r, 2)])
    L.append("PDOTS:  ; 8 x {line, mask hi, mask lo}")
    for (y, m) in dots():
        L.append("        DB      %d,%03XH,%03XH" % (y, m >> 8, m & 0xFF))
    t = town(atari)
    data = [b for r in t for b in pack2(r)]
    packed = rle(data)
    L.append("TOWNW   EQU     24              ; bytes a line (96 pixels at x 32)")
    L.append("TOWNH   EQU     %d" % len(t))
    L.append("TOWNRLE: ; %d bytes packed from %d" % (len(packed), len(data)))
    L += db_lines(packed)
    L.append("; pictures: 8 x 6, 1 byte a line")
    for n, art in hgr.PICS:
        rows = squeeze([r for r in intv.from_art(art) for _ in (0, 1)], 6)
        L.append("PIC_%s:" % n.upper())
        L += db_lines([pack1(r, 1)[0] for r in rows])
    open("gen/map.inc", "w").write("\n".join(L) + "\n")
    open("gen/gfx.inc", "w").write(head + "\n")

    # ---- previews
    if a.preview:
        os.makedirs(a.preview, exist_ok=True)

        def town_im(im):
            for y, r in enumerate(t):
                for x, c in enumerate(r):
                    im.putpixel((32 + x, 6 + y), RGB[c])
        preview(os.path.join(a.preview, "town.png"), town_im)

        def plots_im(im):
            for i, (n, img) in enumerate(layers):
                rows = squeeze(img, 12)
                for y, r in enumerate(rows):
                    for x, v in enumerate(r):
                        if v:
                            im.putpixel((8 + (i % 9) * 16 + x, 6 + (i // 9) * 12 + y), RGB[WHITE])
            for k, n in enumerate(("species",)):
                wb, h, fr = sp[n]
                for f in range(min(len(fr), 44)):
                    for y, r in enumerate(fr[f]):
                        for x, v in enumerate(r[:8]):
                            if v:
                                im.putpixel(((f % 16) * 10 + x, 30 + (f // 16) * 12 + y), RGB[ORANGE])
        preview(os.path.join(a.preview, "parts.png"), plots_im)
    print("mkgfx_astro: %s; town %d bytes packed" % (what, len(packed)))


if __name__ == "__main__":
    main()

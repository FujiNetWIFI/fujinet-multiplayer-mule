#!/usr/bin/env python3
"""mkgfx_chf.py -- the Channel F's graphics, from the Atari disk.

usage: mkgfx_chf.py [--atr mule.atr] [--preview DIR]

The pictures come off the Atari M.U.L.E. disk through the Apple II and
Intellivision clients' extraction (clients/mekkogx/tools/mkgfx_hgr.py,
clients/intv/tools/mkgfx.py), as the Astrocade's do: the species frames, map
figures, plot parts, mountains, ship, pest, cloud and the town. With no disk
they fall back to those tools' stand-in art, and the output's first line says
so (a stand-in build looks like lost work).

The scale is the client's (disp.inc, the game's Atari coordinates):
x * 5/8, y * 1/4, so a plot's 16 x 32 becomes 10 x 8. Rows and columns are
OR-ed together so thin lines survive. A few sprites would be too thin at a
quarter of their height, and are made taller: the species (y * 1/3), the map
figure and the map M.U.L.E. (y * 1/2). Each shape keeps its feet where the
game puts them: yoff moves its top up by the extra rows.

Output (AS, for the data page, never committed): gen/art.inc
  SHTAB    per shape: width, rows, bytes a row, yoff, flippable, then DW the
           frames (1 bit a pixel, MSB left; spr.inc mirrors them itself)
  PLY_*    plot layers: the pixels lit, as offsets into the 10 x 8 plot
           (bit 7: the row is odd), $FF ending
  PDOTS    the production dots, offsets the same way
  TOWN*    the town: 60 x 36, a value a pixel (0-3, palette 2), RLE by row
  PIC_*    the small pictures: 5 x 4, lit offsets
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "mekkogx", "tools"))
import mkgfx_hgr as hgr  # noqa: E402
intv = hgr.intv

# palette 2's values: 0 light grey (the ground), 1 blue, 2 red, 3 green
GROUND, BLUE, RED, GREEN = 0, 1, 2, 3
RGB = {GROUND: (224, 224, 224), BLUE: (75, 63, 243), RED: (255, 49, 83), GREEN: (2, 204, 93)}

PLOT_W, PLOT_H = 10, 8


def xmap(w):
    """Atari width -> Channel F width (5/8, at least 1)."""
    return max(1, (w * 5 + 7) // 8)


def resample(rows, ow, oh):
    """OR-resample a 0/1 bitmap to ow x oh."""
    h = len(rows)
    w = len(rows[0]) if rows else 0
    out = []
    for oy in range(oh):
        y0, y1 = oy * h // oh, max(oy * h // oh + 1, (oy + 1) * h // oh)
        row = []
        for ox in range(ow):
            x0, x1 = ox * w // ow, max(ox * w // ow + 1, (ox + 1) * w // ow)
            row.append(1 if any(rows[y][x] for y in range(y0, min(y1, h))
                                for x in range(x0, min(x1, w))) else 0)
        out.append(row)
    return out


def pack1(row, bpr):
    out = []
    for b in range(bpr):
        v = 0
        for i in range(8):
            x = b * 8 + i
            if x < len(row) and row[x]:
                v |= 0x80 >> i
        out.append(v)
    return out


def db_lines(vals, per=16):
    return ["        DB " + ",".join("%03XH" % v for v in vals[i:i + per])
            for i in range(0, len(vals), per)]


# ---- sprites ---------------------------------------------------------------
# name: (y divisor, flippable). The order is gfx.h's SH_*.
SHAPES = [("species", 3, False), ("figure", 2, True), ("mule", None, True),
          ("wampus", 4, False), ("cursor", None, False), ("nose", 4, False),
          ("hull_top", 4, False), ("hull_bot", 4, False), ("dock", 4, False),
          ("pest", 4, False), ("cloud", 4, False), ("bolt", 4, False),
          ("spark", 4, False), ("meteor", 4, False)]


def sprites(atari):
    src = {s.name: s for s in hgr.shapes(atari)}
    out = []
    for name, ydiv, flip in SHAPES:
        s = src[name]
        w = xmap(s.w)
        nominal = max(1, (s.h + 3) // 4)        # where the game expects it
        if name == "cursor":                    # a plot's outline, exactly
            w, h = PLOT_W, PLOT_H
            frames = [[[1] * w] + [[1] + [0] * (w - 2) + [1] for _ in range(h - 2)] + [[1] * w]]
            nominal = h
        elif name == "mule":
            # town frames (16 lines) at 1/4, map frames (8 lines) at 1/2: 4 rows
            h = 4
            frames = []
            for k, f in enumerate(s.frames[:4]):
                real = f if k < 2 else f[:8]
                frames.append(resample(real, w, h))
        elif name == "spark":
            h = 1
            frames = [[[1] + [0] * (w - 1)]]
        else:
            h = max(1, (s.h + ydiv - 1) // ydiv)
            frames = [resample(f, w, h) for f in s.frames]
        bpr = (w + 7) // 8
        out.append((name, w, h, bpr, nominal - h, 1 if flip else 0, frames))
    return out


# ---- the plot ----------------------------------------------------------------
def plot_layers(atari):
    """The plot's parts at 16 x 32 (as mkgfx_astro.py), then 10 x 8."""
    mc = (lambda c: [hgr.bits(b) for b in atari.mapchar_bytes(c)]) if atari else None
    blank, paste = hgr.blank_img, hgr.paste
    if atari:
        icons = {}
        codes = {"house": (0x11, 0x17), "food": (0x12, 0x18), "energy": (0x13, 0x19),
                 "smithore": (0x14, 0x1A), "crystite": (0x15, 0x1B), "mule": (0x16, 0x1C)}
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
        icons = {}
        for n, art in intv.INTY_ICONS.items():
            img = blank()
            paste(img, [r for r in intv.from_art(art) for _ in (0, 1)], 8, 8)
            icons[n] = img
        mshapes = intv.MOUNTAINS_FALLBACK
    layers = []
    border = [[1] * PLOT_W] + [[1] + [0] * (PLOT_W - 2) + [1] for _ in range(PLOT_H - 2)] + [[1] * PLOT_W]
    layers.append(("border", border))
    for n in ("house", "food", "energy", "smithore", "crystite", "mule"):
        layers.append(("icon_" + n, resample(icons[n], PLOT_W, PLOT_H)))
    for m in (1, 2, 3):
        img = blank()
        for (shape, x, y) in intv.mountain_layout(mshapes, m):
            paste(img, shape, x + 1, y + 2)
        layers.append(("mount%d" % m, resample(img, PLOT_W, PLOT_H)))
    store = intv.from_art(intv.STORE_ART, 16)
    layers.append(("store", resample([r for r in store for _ in (0, 1)], PLOT_W, PLOT_H)))
    river = intv.from_art(intv.RIVER_ART)
    rimg = blank()
    for y in range(32):
        paste(rimg, [river[(y // 2) % 16]], 4, y)
    layers.append(("river", resample(rimg, PLOT_W, PLOT_H)))
    return layers


def offsets(img):
    """Lit pixels as offsets into the plot, bit 7 = an odd row."""
    out = []
    for y, r in enumerate(img):
        for x, v in enumerate(r):
            if v:
                out.append((y * len(r) + x) | (0x80 if y & 1 else 0))
    return out


def dots():
    """Production dots: two rows of four along the bottom left."""
    out = []
    for i in range(8):
        x = 1 + (i % 4) * 2 - (0 if i < 4 else 1) + 1
        y = 5 + i // 4
        out.append((y * PLOT_W + x) | (0x80 if y & 1 else 0))
    return out


# ---- the town ----------------------------------------------------------------
TOWN_W, TOWN_H = 60, 36


def atari_col(c):
    """An Atari colour of the town -> a palette-2 value."""
    lum = c & 0x0E
    hue = c >> 4
    if c == 0 or lum == 0:
        return GROUND
    if hue == 0:
        return BLUE if lum >= 6 else GROUND     # grey/white: ink
    if hue in (1, 2, 3, 4, 5, 6):
        return RED
    if hue in (11, 12, 13):
        return GREEN
    return BLUE


def town(atari):
    if atari:
        img = atari.town()
        cols = [[atari_col(c) for c in r] for r in img]
        # the corral: a fence, not a solid box (as the Astrocade's)
        box = [(x, y) for y in range(64, 144) for x in range(64, 96) if img[y][x] == 0x08]
        if box:
            x0, x1 = min(p[0] for p in box), max(p[0] for p in box)
            y0, y1 = min(p[1] for p in box), max(p[1] for p in box)
            for (x, y) in box:
                if x0 + 1 < x < x1 - 1 and y0 + 2 < y < y1 - 2:
                    cols[y][x] = GROUND
    else:
        cols = [[GROUND] * 96 for _ in range(144)]
        pal = {4: BLUE, 11: RED, 5: BLUE, 2: RED, 8: BLUE, 7: GREEN, 0: GROUND}
        for name, (x, y, w, h, bg, fg, sign) in intv.TOWN_FALLBACK.items():
            for yy in range(y * 16, (y + h) * 16):
                for xx in range(x * 8, (x + w) * 8):
                    cols[yy][xx] = pal.get(bg, BLUE)
    out = []
    for oy in range(TOWN_H):
        y0, y1 = oy * 144 // TOWN_H, (oy + 1) * 144 // TOWN_H
        row = []
        for ox in range(TOWN_W):
            x0, x1 = ox * 96 // TOWN_W, max(ox * 96 // TOWN_W + 1, (ox + 1) * 96 // TOWN_W)
            # the most common ink in the block, or ground
            counts = {}
            for y in range(y0, y1):
                for x in range(x0, x1):
                    v = cols[y][x]
                    if v != GROUND:
                        counts[v] = counts.get(v, 0) + 1
            n = (y1 - y0) * (x1 - x0)
            v = max(counts, key=counts.get) if counts and sum(counts.values()) * 3 >= n else GROUND
            row.append(v)
        out.append(row)
    return out


def rle(row):
    """n < 128: n+1 literal values follow; n >= 128: the next value n-125 times."""
    out, i = [], 0
    while i < len(row):
        j = i
        while j < len(row) and row[j] == row[i] and j - i < 130:
            j += 1
        if j - i >= 3:
            out += [0x80 + (j - i) - 3, row[i]]
            i = j
            continue
        lit = []
        k = i
        while k < len(row) and len(lit) < 128:
            if k + 2 < len(row) and row[k] == row[k + 1] == row[k + 2]:
                break
            lit.append(row[k])
            k += 1
        out += [len(lit) - 1] + lit
        i = k
    return out


# ---- the pictures --------------------------------------------------------------
PIC_W, PIC_H = 5, 4


def pics():
    out = []
    for n, art in hgr.PICS:
        img = resample([r for r in intv.from_art(art) for _ in (0, 1)], PIC_W, PIC_H)
        out.append((n, img))
    return out


def preview(path, w, h, draw, scale=6):
    try:
        from PIL import Image
    except ImportError:
        return
    im = Image.new("RGB", (w, h), RGB[GROUND])
    draw(im)
    im.resize((w * scale, h * scale), Image.NEAREST).save(path)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--atr", default="mule.atr" if os.path.exists("mule.atr") else None)
    ap.add_argument("--preview", default="gen/preview")
    a = ap.parse_args()
    atari = hgr.Atari(a.atr) if a.atr and os.path.exists(a.atr) else None
    what = "converted from the Atari disk" if atari else "stand-in art: NO ATR (mule.atr)"
    os.makedirs("gen", exist_ok=True)
    L = ["; GENERATED by tools/mkgfx_chf.py -- %s. Do not edit or commit." % what, ""]

    sp = sprites(atari)
    L.append("; SHTAB: per shape {width, rows, bytes a row, yoff, flippable}, DW data")
    L.append("SHTAB:")
    for name, w, h, bpr, yoff, nflip, frames in sp:
        L.append("        DB %d,%d,%d,%d,%d" % (w, h, bpr, yoff & 0xFF, nflip))
        L.append("        DW SHD_%s" % name.upper())
    for name, w, h, bpr, yoff, nflip, frames in sp:
        L.append("SHD_%s:  ; %d frames, %d x %d" % (name.upper(), len(frames), w, h))
        L += db_lines([b for f in frames for r in f for b in pack1(r, bpr)])

    layers = plot_layers(atari)
    L.append("; the layers: border, 6 icons (house food energy smithore crystite mule),")
    L.append("; mountains 1-3, store, river")
    L.append("PLYTAB:")
    L.append("        DW " + ",".join("PLY_%s" % n.upper() for n, _ in layers))
    for n, img in layers:
        L.append("PLY_%s:" % n.upper())
        L += db_lines(offsets(img) + [0xFF])
    L.append("PDOTS:")
    L += db_lines(dots())

    t = town(atari)
    L.append("TOWNW   EQU %d" % TOWN_W)
    L.append("TOWNH   EQU %d" % TOWN_H)
    L.append("TOWNRLE:")
    total = 0
    for r in t:
        enc = rle(r)
        total += len(enc)
        L += db_lines(enc)
    L.append("PICTAB:")
    L.append("        DW " + ",".join("PIC_%s" % n.upper() for n, _ in pics()))
    for n, img in pics():
        L.append("PIC_%s:" % n.upper())
        L += db_lines(offsets_w(img, PIC_W) + [0xFF])
    open("gen/art.inc", "w").write("\n".join(L) + "\n")

    if a.preview:
        os.makedirs(a.preview, exist_ok=True)

        def town_im(im):
            for y, r in enumerate(t):
                for x, v in enumerate(r):
                    im.putpixel((x, y), RGB[v])
        preview(os.path.join(a.preview, "town.png"), TOWN_W, TOWN_H, town_im)

        def plots_im(im):
            for i, (n, img) in enumerate(layers):
                for y, r in enumerate(img):
                    for x, v in enumerate(r):
                        if v:
                            im.putpixel(((i % 7) * 12 + x, (i // 7) * 10 + y), RGB[BLUE])
        preview(os.path.join(a.preview, "plots.png"), 84, 20, plots_im)

        def spr_im(im):
            yy = 0
            for name, w, h, bpr, yoff, nflip, frames in sp:
                for k, f in enumerate(frames[:16]):
                    for y, r in enumerate(f):
                        for x, v in enumerate(r):
                            if v and k * (w + 1) + x < 200:
                                im.putpixel((k * (w + 1) + x, yy + y), RGB[RED])
                yy += h + 1
        preview(os.path.join(a.preview, "sprites.png"), 200, 120, spr_im, 4)
    print("mkgfx_chf: %s; town %d bytes" % (what, total))


def offsets_w(img, w):
    """Lit pixels as offsets into a w-wide picture, bit 7 = an odd row."""
    out = []
    for y, r in enumerate(img):
        for x, v in enumerate(r):
            if v:
                out.append((y * w + x) | (0x80 if y & 1 else 0))
    return out


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Build the Apple II client's graphics (gen/apple2/gfx.c) for HGR.

usage: mkgfx_hgr.py [--atr "M.U.L.E. (1983)(Electronic Arts)(US).atr"] [--preview dir] out.c

With --atr the map characters, mountains, species, map figures, town, ships
and event sprites are converted from the original Atari 8-bit disk
(CRC32 F76D2896). Those graphics belong to their copyright holders; the
generated file must not be committed. Without --atr, stand-in art is used
(the Intellivision client's, clients/intv/tools/mkgfx.py), so the game
always builds -- but it looks nothing like M.U.L.E., so the Makefile warns.
The font is the Namco arcade font of the other clients (tools/tms/namco.bin)
either way.

HGR, briefly: 280 x 192, 7 pixels a byte (bit 0 leftmost, bit 7 picks the
palette). Two neighbouring pixels make one colour pixel: 140 across. With
bit 7 clear a pixel in an even column is violet and in an odd column green;
with bit 7 set they are blue and orange; two side by side are white.

Scaling: one Atari colour clock is one colour pixel for sprites. The map and
the town are squeezed from 16 to 14 colour clocks a plot (160 -> 140), so a
plot is exactly 4 bytes and starts on a byte. Scanlines are kept 1:1.
"""
import argparse
import os
import sys
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "intv", "tools"))
import mkgfx as intv  # noqa: E402  (stand-in art and the extraction offsets)

# src/apple2/gfx.c's sprite save buffers: the rings' rows come from it
SAVE_MAX = 170

EXPECTED_CRC = 0xF76D2896

# HGR colours (rendering for previews).
BLACK, WHITE, VIOLET, GREEN, BLUE, ORANGE = range(6)
RGB = {BLACK: (0, 0, 0), WHITE: (255, 255, 255), VIOLET: (255, 68, 253),
       GREEN: (20, 245, 60), BLUE: (20, 207, 253), ORANGE: (255, 106, 60)}

# Atari colours seen in the town -> HGR colour.
ATARI_TO_HGR = {0x00: BLACK, 0x08: WHITE, 0x0C: WHITE, 0x0E: WHITE, 0x04: BLACK,
                0x24: ORANGE, 0x34: ORANGE, 0x44: VIOLET, 0x54: VIOLET,
                0x84: BLUE, 0x86: BLUE, 0xB4: GREEN, 0xC4: GREEN, 0xD4: GREEN,
                0xF4: ORANGE, 0x1A: ORANGE}


def bits(b):
    return [(b >> (7 - x)) & 1 for x in range(8)]


# ---------------------------------------------------------------------------
# HGR encoding of a row of colour pixels (list of HGR colours) starting at
# colour pixel x0 of the screen. Returns {byte index: value}.
# ---------------------------------------------------------------------------
def encode_row(pixels, x0):
    on = {}          # mono column -> 1
    want = {}        # byte -> [count group0, count group1]
    for i, c in enumerate(pixels):
        m = 2 * (x0 + i)
        if c == BLACK:
            continue
        cols = {WHITE: (m, m + 1), VIOLET: (m,), BLUE: (m,), GREEN: (m + 1,), ORANGE: (m + 1,)}[c]
        for mc in cols:
            on[mc] = 1
            if c != WHITE:
                g = 1 if c in (BLUE, ORANGE) else 0
                want.setdefault(mc // 7, [0, 0])[g] += 1
    out = {}
    for mc in on:
        out.setdefault(mc // 7, 0)
        out[mc // 7] |= 1 << (mc % 7)
    for b in list(out):
        w = want.get(b, [0, 0])
        if w[1] > w[0]:
            out[b] |= 0x80
    return out


def render_hgr(screen, path, scale=3):
    """screen: dict (y, byte) -> value. Rough NTSC preview."""
    try:
        from PIL import Image
    except ImportError:
        return
    im = Image.new("RGB", (280, 192))
    for y in range(192):
        row = [0] * 280
        hib = [0] * 280
        for b in range(40):
            v = screen.get((y, b), 0)
            for k in range(7):
                row[b * 7 + k] = (v >> k) & 1
                hib[b * 7 + k] = v >> 7
        for x in range(280):
            if not row[x]:
                # a gap between two same-parity pixels takes their colour
                if 0 < x < 279 and row[x - 1] and row[x + 1] and not (x > 1 and row[x - 2]):
                    pass
                continue
            if (x > 0 and row[x - 1]) or (x < 279 and row[x + 1]):
                c = WHITE
            else:
                c = [[VIOLET, GREEN], [BLUE, ORANGE]][hib[x]][x & 1]
            im.putpixel((x, y), RGB[c])
            if x < 279 and not row[x + 1] and c != WHITE:
                im.putpixel((x + 1, y), RGB[c])
    im.resize((280 * scale, 192 * scale * 2 // 2), Image.NEAREST).save(path)


# ---------------------------------------------------------------------------
class Atari(intv.Atari):
    def mapchar_bytes(self, c):
        return self.main[0x1C00 + c * 8:0x1C00 + c * 8 + 8]


STANDIN_FONT = {
    # 5x7 glyphs as 7 rows of 5 bits (MSB left): mkgfx_atari.py's, without a disk
    'A': [14, 17, 17, 31, 17, 17, 17], 'B': [30, 17, 17, 30, 17, 17, 30], 'C': [14, 17, 16, 16, 16, 17, 14],
    'D': [30, 17, 17, 17, 17, 17, 30], 'E': [31, 16, 16, 30, 16, 16, 31], 'F': [31, 16, 16, 30, 16, 16, 16],
    'G': [14, 17, 16, 23, 17, 17, 15], 'H': [17, 17, 17, 31, 17, 17, 17], 'I': [14, 4, 4, 4, 4, 4, 14],
    'J': [1, 1, 1, 1, 17, 17, 14], 'K': [17, 18, 20, 24, 20, 18, 17], 'L': [16, 16, 16, 16, 16, 16, 31],
    'M': [17, 27, 21, 21, 17, 17, 17], 'N': [17, 25, 21, 19, 17, 17, 17], 'O': [14, 17, 17, 17, 17, 17, 14],
    'P': [30, 17, 17, 30, 16, 16, 16], 'Q': [14, 17, 17, 17, 21, 18, 13], 'R': [30, 17, 17, 30, 20, 18, 17],
    'S': [14, 17, 16, 14, 1, 17, 14], 'T': [31, 4, 4, 4, 4, 4, 4], 'U': [17, 17, 17, 17, 17, 17, 14],
    'V': [17, 17, 17, 17, 17, 10, 4], 'W': [17, 17, 17, 21, 21, 21, 10], 'X': [17, 17, 10, 4, 10, 17, 17],
    'Y': [17, 17, 10, 4, 4, 4, 4], 'Z': [31, 1, 2, 4, 8, 16, 31],
    '0': [14, 17, 19, 21, 25, 17, 14], '1': [4, 12, 4, 4, 4, 4, 14], '2': [14, 17, 1, 2, 4, 8, 31],
    '3': [31, 2, 4, 2, 1, 17, 14], '4': [2, 6, 10, 18, 31, 2, 2], '5': [31, 16, 30, 1, 1, 17, 14],
    '6': [6, 8, 16, 30, 17, 17, 14], '7': [31, 1, 2, 4, 8, 8, 8], '8': [14, 17, 17, 14, 17, 17, 14],
    '9': [14, 17, 17, 15, 1, 2, 12], '.': [0, 0, 0, 0, 0, 12, 12], ',': [0, 0, 0, 0, 12, 4, 8],
    '!': [4, 4, 4, 4, 4, 0, 4], '?': [14, 17, 1, 2, 4, 0, 4], "'": [4, 4, 8, 0, 0, 0, 0],
    '$': [4, 15, 20, 14, 5, 30, 4], '#': [10, 10, 31, 10, 31, 10, 10], '-': [0, 0, 0, 31, 0, 0, 0],
    '+': [0, 4, 4, 31, 4, 4, 0], ':': [0, 12, 12, 0, 12, 12, 0], '/': [0, 1, 2, 4, 8, 16, 0],
    '*': [0, 4, 21, 14, 21, 4, 0], '(': [2, 4, 8, 8, 8, 4, 2], ')': [8, 4, 2, 2, 2, 4, 8],
    '>': [8, 4, 2, 1, 2, 4, 8], '<': [2, 4, 8, 16, 8, 4, 2], '=': [0, 0, 31, 0, 31, 0, 0],
    '%': [24, 25, 2, 4, 8, 19, 3], '&': [12, 18, 20, 8, 21, 18, 13], ';': [0, 12, 12, 0, 12, 4, 8],
}


# ---------------------------------------------------------------------------
# Font: the Namco arcade font of the other clients (tools/tms/namco.bin),
# 8 x 8 with the glyphs in columns 0..6. A cell here is a byte, 7 pixels, so
# a glyph is narrowed to 6 and column 6 is the gap: of the merges of two
# neighbouring columns and the drops of one, the first that closes no gap
# and adds the fewest lone pixels (a lone pixel shows in colour, two side by
# side white). FONT_FIX redraws, with 2-pixel strokes, the glyphs still left
# with lone pixels and the symbols the game prints; M and W keep all 7
# columns and touch the next cell, as the Atari font's did.
# ---------------------------------------------------------------------------
FONT_FIX = {    # 6 columns, rows 0..6
    "#": ["......", ".##.##", "######", ".##.##", "######", ".##.##", "......"],
    "$": ["..##..", ".#####", "##....", ".####.", "....##", "#####.", "..##.."],
    "'": [".##...", ".##...", "##....", "......", "......", "......", "......"],
    "*": ["......", "##..##", ".####.", "######", ".####.", "##..##", "......"],
    ",": ["......", "......", "......", "......", ".##...", ".##...", "##...."],
    "/": ["....##", "....##", "...##.", "..##..", ".##...", "##....", "##...."],
    "0": ["..##..", ".####.", "##..##", "##..##", "##..##", ".####.", "..##.."],
    "8": [".####.", "##..##", "##..##", ".####.", "##..##", "##..##", ".####."],
    "@": [".####.", "##..##", "##.###", "##.###", "##.##.", "##....", ".####."],
    "A": ["..##..", ".####.", "##..##", "##..##", "######", "##..##", "##..##"],
    "K": ["##..##", "##.##.", "####..", "###...", "####..", "##.##.", "##..##"],
    "Q": [".####.", "##..##", "##..##", "##..##", "##.###", ".####.", "....##"],
    "V": ["##..##", "##..##", "##..##", "##..##", ".####.", ".####.", "..##.."],
    "X": ["##..##", "##..##", ".####.", "..##..", ".####.", "##..##", "##..##"],
}
FONT_FULL = "MW"


def runs(row):
    """The lengths of a row's runs of lit pixels."""
    out, n = [], 0
    for v in row + [0]:
        if v:
            n += 1
        elif n:
            out.append(n)
            n = 0
    return out


def narrow(rows):
    """7 columns -> 6 (see above)."""
    cands = [[r[:k] + [r[k] | r[k + 1]] + r[k + 2:] for r in rows] for k in range(6)]
    cands += [[r[:k] + r[k + 1:] for r in rows] for k in range(7)]

    def score(c):
        s = 0
        for a, b in zip(rows, c):
            ra, rb = runs(a), runs(b)
            s += 100 * (len(ra) == len(rb)) - 10 * max(0, rb.count(1) - ra.count(1))
        return s
    return max(cands, key=score)


def namco_font():
    """64 glyphs (ASCII 32..95) x 8 rows of HGR bytes (7 pixels, bit 0 left)."""
    with open(os.path.join(HERE, "tms", "namco.bin"), "rb") as f:
        d = f.read()
    out = []
    for c in range(64):
        ch = chr(32 + c)
        if ch in FONT_FIX:
            rows = [[int(p == "#") for p in r] for r in FONT_FIX[ch]] + [[0] * 6]
        else:
            rows = [[(b >> (7 - x)) & 1 for x in range(7)] for b in d[c * 8:c * 8 + 8]]
            if ch not in FONT_FULL:
                rows = narrow(rows)
        out.append([sum(1 << x for x, p in enumerate(r) if p) for r in rows])
    return out


# ---------------------------------------------------------------------------
# Plots: 14 colour pixels x 32 lines, as 4 HGR bytes a line with every lit
# colour pixel white (the game masks in the owner's colour).
# ---------------------------------------------------------------------------
def squeeze16(row16):
    """16 Atari colour clocks -> 14 colour pixels (OR-merging the blank-ish
    column next to each half's edge: cc 7 into 6, cc 14 into 13)."""
    out = [0] * 14
    for i, v in enumerate(row16):
        j = i if i < 7 else (6 if i == 7 else (i - 1 if i < 14 else (12 if i == 14 else 13)))
        out[j] |= v
    return out


def plot_bytes(img16x32):
    """img: 32 rows x 16 (0/1) -> 32 x 4 bytes, pixels white."""
    rows = []
    for r in img16x32:
        cp = squeeze16(r)
        enc = encode_row([WHITE if v else BLACK for v in cp], 0)
        rows.append([enc.get(b, 0) & 0x7F for b in range(4)])
    return rows


def blank_img(w=16, h=32):
    return [[0] * w for _ in range(h)]


def paste(dst, src_rows, x0, y0):
    for y, r in enumerate(src_rows):
        for x, v in enumerate(r):
            if v and 0 <= y0 + y < len(dst) and 0 <= x0 + x < len(dst[0]):
                dst[y0 + y][x0 + x] = 1
    return dst


def plot_parts(atari):
    parts = {}
    if atari:
        mc = lambda c: [bits(b) for b in atari.mapchar_bytes(c)]
        border = blank_img()
        for i, c in enumerate((0x1D, 0x1E, 0x1E, 0x1F)):
            paste(border, mc(c), 0, i * 8)
        paste(border, mc(0x20), 8, 0)
        paste(border, mc(0x22), 8, 24)
        for y in range(8, 24):
            border[y][15] = 1                   # the right edge behind the icon chars
        # (the pictures: $12 a plant, $13 a bolt, $14 a pick, $15 a crystal --
        # as the Intellivision client's redrawn icons show)
        icon_codes = {"house": (0x11, 0x17), "food": (0x12, 0x18), "energy": (0x13, 0x19),
                      "smithore": (0x14, 0x1A), "crystite": (0x15, 0x1B), "mule": (0x16, 0x1C)}
        icons = {}
        for n, (a, b) in icon_codes.items():
            img = blank_img()
            ra, rb = mc(a), mc(b)
            for y in range(8):
                ra[y][7] = 0                    # the border column is in `border`
                rb[y][7] = 0
            paste(img, ra, 8, 8)
            paste(img, rb, 8, 16)
            icons[n] = img
        shapes = atari.mountains()
    else:
        border = blank_img()
        for x in range(16):
            border[0][x] = border[1][x] = border[30][x] = border[31][x] = 1
        for y in range(32):
            border[y][0] = border[y][15] = 1
        icons = {}
        for n, art in intv.INTY_ICONS.items():
            img = blank_img()
            paste(img, [r for r in intv.from_art(art) for _ in (0, 1)], 8, 8)
            icons[n] = img
        shapes = intv.MOUNTAINS_FALLBACK
    parts["border"] = plot_bytes(border)
    for n in ("house", "food", "energy", "smithore", "crystite", "mule"):
        parts["icon_" + n] = plot_bytes(icons[n])
    for m in (1, 2, 3):
        img = blank_img()
        for (shape, x, y) in intv.mountain_layout(shapes, m):
            paste(img, shape, x + 1, y + 2)
        parts["mount%d" % m] = plot_bytes(img)
    store = intv.from_art(intv.STORE_ART, 16)
    parts["store"] = plot_bytes([r for r in store for _ in (0, 1)])
    river = intv.from_art(intv.RIVER_ART)
    rimg = blank_img()
    for y in range(32):
        paste(rimg, [river[(y // 2) % 16]], 4, y)
    parts["river"] = plot_bytes(rimg)
    # production dots: 1 cc x 2 lines each, two rows of four, bottom left
    dots = []
    for i in range(8):
        x = 2 + (i % 4) * 2 - (0 if i < 4 else 1)
        y = 22 + (i // 4) * 4
        cp = squeeze16([1 if k == x else 0 for k in range(16)]).index(1)
        m = 2 * cp
        dots.append((y, m // 7, (1 << (m % 7)) | (1 << ((m + 1) % 7) if (m + 1) // 7 == m // 7 else 0),
                     (m + 1) // 7, 1 << ((m + 1) % 7)))
    return parts, dots


# ---------------------------------------------------------------------------
# Sprites: 1 bit per colour pixel, MSB left, wb bytes a row.
# ---------------------------------------------------------------------------
class Shape:
    def __init__(self, name, frames, w, h):
        self.name, self.w, self.h = name, w, h
        self.frames = [pad_rows(f, w, h) for f in frames]


def pad_rows(rows, w, h):
    out = [list(r[:w]) + [0] * (w - len(r[:w])) for r in rows[:h]]
    while len(out) < h:
        out.append([0] * w)
    return out


def widen(rows, k):
    return [[v for v in r for _ in range(k)] for r in rows]


def taller(rows, k):
    return [r for r in rows for _ in range(k)]


def shapes(atari):
    out = []
    # species (8 x 22)
    sp = atari.species() if atari else {k: intv.species_fallback(k) for k in range(8)}
    out.append(Shape("species", [f for sid in range(8) for f in sp[sid]], 8, 22))
    fig = atari.map_species() if atari else {k: intv.map_species_fallback() for k in range(8)}
    out.append(Shape("figure", [f for sid in range(8) for f in fig[sid]], 8, 8))
    mule = [intv.from_art(f) for f in intv.MULE_ART]
    mule = [taller(f, 2) for f in mule] + mule
    out.append(Shape("mule", mule + [[r[::-1] for r in f] for f in mule], 8, 16))  # 4-7 mirrored
    out.append(Shape("wampus", [taller(intv.from_art(intv.WAMPUS_ART), 2)], 8, 16))
    cur = [[1] * 14, [1] * 14] + [[1] + [0] * 12 + [1] for _ in range(28)] + [[1] * 14, [1] * 14]
    out.append(Shape("cursor", [cur], 14, 32))
    # ships
    if atari:
        tnose = atari.bytes_at(0x747B, 10)
        pnose = atari.bytes_at(0x73B5, 7)
    else:
        tnose = intv.from_art([".....XXX", "...XXXXX", "..XX..XX", ".XXX..XX", "XX......", "XX......",
                               ".XXX..XX", "..XX..XX", "...XXXXX", ".....XXX"])
        pnose = intv.from_art([".....XXX", "..XX...X", ".XXXX.XX", "XXX.XXX.", "XXXX...X", ".XX.XXX.", ".....XXX"])
    out.append(Shape("nose", [tnose, [[0] * 8] * 1 + pnose], 8, 10))
    tbody = intv.from_art(["....XXX.", "...XXXX.", "..XXXXX.", "XXXXXX..", "XXXXXX..", "..XXXXX.", "...XXXX.", "....XXX."])
    pbody = intv.from_art(["........"] * 2 + ["....XXX.", "XXXXXX..", "XXXXXX..", "....XXX."] + ["........"] * 2)
    out.append(Shape("hull_top", [taller(widen(b[:4], 4), 4) for b in (tbody, pbody)], 32, 16))
    out.append(Shape("hull_bot", [taller(widen(b[4:], 4), 4) for b in (tbody, pbody)], 32, 16))
    # docking frames 0, 2, 4, 6 of eight
    if atari:
        seg = atari.main[0x7410:0x7480]
        cur_s, docks = [], []
        for b in seg:
            if b:
                cur_s.append(b)
            elif cur_s:
                docks.append(cur_s)
                cur_s = []
        docks = [[bits(b) for b in s] for s in docks if len(s) >= 2][:8]
    else:
        docks = []
    while len(docks) < 8:
        k = len(docks)
        w = max(1, 6 - k)
        docks.append([[1 if abs(x - 3.5) < w / 2 else 0 for x in range(8)] for _ in range(max(2, 8 - k))])
    dk = []
    for s in docks[0:8:2]:
        top = (16 - len(s)) // 2
        dk.append([[0] * 8] * top + s)
    out.append(Shape("dock", dk, 8, 16))
    if atari:
        pest = [[bits(b) for b in atari.main[a:a + 11]] for a in (0x778F, 0x779B, 0x77A6, 0x77B2)]
        cloud = [[bits(b) for b in atari.main[0x798E + 16 * k:0x798E + 16 * k + 14][1::2]] for k in range(4)]
        bolt = [bits(b) for b in atari.main[0x7983:0x7983 + 10]]
    else:
        p = intv.from_art(["....X...", ".XX.X...", ".X..X...", "...XXXX.", ".X.XXX.X", "X.XXX...", "X.XXXX..",
                           "X..X..X.", "....X...", "....X...", "...X...."])
        pest = [p, p, p, p]
        c = intv.from_art(["..XXXX..", ".XXXXXX.", "XXXXXXXX", "XXXXXXXX", "XXXXXXXX", ".XXXXXX.", "..XXXX.."])
        cloud = [c] * 4
        bolt = intv.from_art(["..X.....", "...X....", "....X...", "..XXXX..", "...X....", "....X...", "..XXXX..",
                              "...X....", "....X...", ".....X.."])
    out.append(Shape("pest", pest, 8, 11))
    out.append(Shape("cloud", [taller(widen(c, 2), 4) for c in cloud], 16, 28))
    out.append(Shape("bolt", [bolt], 8, 10))
    out.append(Shape("spark", [[[1], [1]]], 8, 2))
    out.append(Shape("meteor", [taller(intv.from_art(intv.METEOR_ART), 2)], 8, 16))
    return out


PICS = [("storeicon", intv.STOREICON_ART), ("flag", intv.FLAG_ART),
        ("fire0", intv.FIRE_ART[0]), ("fire1", intv.FIRE_ART[1])]


# ---------------------------------------------------------------------------
# The town: 96 colour clocks x 144 lines -> 84 colour pixels = 24 bytes a
# line, at byte 8 (colour clock 32).
# ---------------------------------------------------------------------------
def town_hgr(atari):
    if atari:
        img = atari.town()
        cols = [[ATARI_TO_HGR.get(c, ATARI_TO_HGR.get(c & 0xF0 | 4, WHITE)) for c in r] for r in img]
        # The corral is a grey box on the Atari; solid white would swamp the
        # screen here, so it becomes a fence (its outline).
        box = [(x, y) for y in range(64, 144) for x in range(64, 96) if img[y][x] == 0x08]
        if box:
            x0, x1 = min(p[0] for p in box), max(p[0] for p in box)
            y0, y1 = min(p[1] for p in box), max(p[1] for p in box)
            for (x, y) in box:
                if x0 + 1 < x < x1 - 1 and y0 + 1 < y < y1 - 1:
                    cols[y][x] = BLACK
    else:
        cols = [[BLACK] * 96 for _ in range(144)]
        pal = {4: GREEN, 11: ORANGE, 5: GREEN, 2: VIOLET, 8: WHITE, 7: WHITE, 0: BLACK}
        for name, (x, y, w, h, bg, fg, sign) in intv.TOWN_FALLBACK.items():
            for yy in range(y * 16, (y + h) * 16):
                for xx in range(x * 8, (x + w) * 8):
                    cols[yy][xx] = pal.get(bg, WHITE)
    # 8 clocks -> 7 pixels: in each group of 8 the clock that goes is the
    # one most like a neighbour (a copy loses nothing), then the one losing
    # the fewest black pixels (the signs' strokes), then the one nearest
    # clock 1 (cx_bias in gfx.c puts sprites there). A fixed clock 3 cut the
    # smithore sign's pick handle and the energy sign's bolt; a fixed clock 1
    # the crystite arrow's head.
    def lost(x, n):
        if n < 0 or n > 95:
            return (999, 999)
        d = [r[x] for r in cols if r[x] != r[n]]
        return (len(d), d.count(BLACK))
    drop = []
    for g in range(12):
        cost = [min(lost(g * 8 + k, g * 8 + k - 1), lost(g * 8 + k, g * 8 + k + 1)) for k in range(8)]
        drop.append(min(range(8), key=lambda k: (cost[k], abs(k - 1))))
    keep = [[g * 8 + k for k in range(8) if k != drop[g]] for g in range(12)]
    rows = []
    for r in cols:
        px = [r[keep[j // 7][j % 7]] for j in range(84)]
        enc = encode_row(px, 28)           # colour pixel 28 = byte 8
        rows.append([enc.get(8 + b, 0) for b in range(24)])
    return rows


# ---------------------------------------------------------------------------
def c_bytes(vals, per=16, indent="    "):
    lines = []
    for i in range(0, len(vals), per):
        lines.append(indent + ",".join("0x%02X" % v for v in vals[i:i + per]) + ",")
    return "\n".join(lines)


def pack_row(r):
    out = []
    for i in range(0, len(r), 8):
        v = 0
        for k, b in enumerate(r[i:i + 8]):
            if b:
                v |= 0x80 >> k
        out.append(v)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--atr")
    ap.add_argument("--preview")
    ap.add_argument("out")
    a = ap.parse_args()
    atari = Atari(a.atr) if a.atr else None
    if a.preview:
        os.makedirs(a.preview, exist_ok=True)

    src = ["/* gfx.c -- GENERATED by tools/mkgfx_hgr.py (%s). Do not edit or commit. */" %
           ("converted from the Atari disk" if atari else "STAND-IN art: no ATR given"),
           "#include <stdint.h>", '#include "../../src/apple2/gfxdata.h"', ""]

    # font
    font = namco_font()
    src.append('#pragma rodata-name (push, "LOWDATA")')
    src.append("const uint8_t hgr_font[64 * 8] = {")
    src.append(c_bytes([b for g in font for b in g]))
    src.append("};")

    # plots
    parts, dots = plot_parts(atari)
    order = ["border", "icon_house", "icon_food", "icon_energy", "icon_smithore", "icon_crystite",
             "icon_mule", "mount1", "mount2", "mount3", "store", "river"]
    src.append("/* plot parts: 32 lines x 4 bytes, lit pixels white */")
    src.append("const uint8_t plot_parts[%d][128] = {" % len(order))
    for n in order:
        src.append("  { /* %s */" % n)
        src.append(c_bytes([b for r in parts[n] for b in r]))
        src.append("  },")
    src.append("};")
    src.append("/* production dots: line, byte, mask, byte2, mask2 */")
    src.append("const uint8_t plot_dots[8][5] = {")
    for d in dots:
        src.append("    {%d,%d,0x%02X,%d,0x%02X}," % d)
    src.append("};")
    src.append("#pragma rodata-name (pop)")

    # sprites
    shs = shapes(atari)
    src.append("/* sprites: 1 bit a colour pixel, MSB left (in auxiliary RAM) */")
    src.append('#pragma rodata-name (push, "AUXGFX")')
    for s in shs:
        wb = (s.w + 7) // 8
        data = []
        for f in s.frames:
            for r in f:
                data += pack_row(r + [0] * (wb * 8 - len(r)))
        src.append("static const uint8_t sh_%s[%d] = {" % (s.name, len(data)))
        src.append(c_bytes(data))
        src.append("};")
    src.append("#pragma rodata-name (pop)")
    src.append("const ShapeDef hgr_shapes[%d] = {" % len(shs))
    for s in shs:
        wb = (s.w + 7) // 8
        st = (s.w * 2 + 5) // 7 + 1      # the bytes a row covers, at worst
        assert wb * s.h <= 64       # hires.s spr_one fetches a frame into 64 bytes
        src.append("    { sh_%s, %d, %d, %d, %d, %d, %d }," % (s.name, wb, s.w, s.h, wb * s.h, st,
                                                            SAVE_MAX // st))
    src.append("};")

    # pictures (8 x 8, drawn at 2 lines a row)
    src.append("const uint8_t hgr_pics[%d][8] = {" % len(PICS))
    for n, art in PICS:
        src.append("    { %s }, /* %s */" % (",".join("0x%02X" % v for r in intv.from_art(art) for v in pack_row(r)), n))
    src.append("};")

    # town
    town = town_hgr(atari)
    src.append('#pragma rodata-name (push, "AUXGFX")')
    src.append("const uint8_t hgr_town[144 * 24] = {")
    src.append(c_bytes([b for r in town for b in r], 24))
    src.append("};")
    src.append("#pragma rodata-name (pop)")

    open(a.out, "w").write("\n".join(src) + "\n")

    if a.preview:
        scr = {}
        for y, r in enumerate(town):
            for b, v in enumerate(r):
                scr[(16 + y, 8 + b)] = v
        render_hgr(scr, os.path.join(a.preview, "town.png"))
        scr = {}
        colmask = [(0x55, 0x2A, 0x55, 0x2A), (0x2A, 0x55, 0x2A, 0x55)]
        demo = [("border", 0, 1), ("icon_house", 0, 1), ("mount2", None, 0), ("icon_food", 1, 1),
                ("store", None, 0), ("mount3", None, 0), ("river", None, 0), ("icon_crystite", 3, 1),
                ("icon_mule", 2, 1)]
        for p, (n, owner, border) in enumerate(demo):
            px, py = 2 + 4 * (p % 9), 16 + 32 * (p // 9)
            for y in range(32):
                for b in range(4):
                    v = parts[n][y][b]
                    if owner is not None:
                        v |= parts["border"][y][b]
                        grp = 1 if owner in (0, 3) else 0
                        par = 0 if owner in (0, 1) else 1
                        v = (v & colmask[par][b]) | (0x80 if grp else 0)
                    scr[(py + y, px + b)] = v
        render_hgr(scr, os.path.join(a.preview, "plots.png"))
    print("%s: %s" % (a.out, "Atari graphics" if atari else "STAND-IN art"))


if __name__ == "__main__":
    main()

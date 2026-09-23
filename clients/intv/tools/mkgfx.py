#!/usr/bin/env python3
"""Build the Intellivision client's graphics (gen/gfx.bas).

usage: mkgfx.py [--atr "M.U.L.E. (1983)(Electronic Arts)(US).atr"] [--preview dir] gen/gfx.bas

With --atr, the map icons, mountains, species, town and event sprites are
converted from the original Atari 8-bit disk (CRC32 F76D2896). Those
graphics belong to their copyright holders; the generated file must not be
committed. Without --atr, the built-in stand-in art below is used, so the
game always builds. Both paths emit the same labels and card counts.

Conversion rules (see docs/ANIMATION.md for why they are exact):
  * 1 Atari colour clock      -> 1 Intellivision pixel (no horizontal scaling)
  * 2 Atari scanlines         -> 1 background pixel (cards halve vertically)
  * sprites keep scanline rows by using 0.5x-scale MOBs

Card data is written as IntyBASIC BITMAP statements (8 per card).
"""
import argparse
import os
import sys
import zlib

EXPECTED_CRC = 0xF76D2896

# Intellivision palette (jzIntv defaults), index -> RGB.
INTY = [(0x00, 0x00, 0x00), (0x00, 0x2D, 0xFF), (0xFF, 0x3D, 0x10), (0xC9, 0xCF, 0xAB),
        (0x38, 0x6B, 0x3F), (0x00, 0xA7, 0x56), (0xFA, 0xEA, 0x50), (0xFF, 0xFC, 0xFF),
        (0xBD, 0xAC, 0xC8), (0x24, 0xB8, 0xFF), (0xFF, 0xB4, 0x1F), (0x54, 0x6E, 0x00),
        (0xFF, 0x4E, 0x57), (0xA4, 0x96, 0xFF), (0x75, 0xCC, 0x80), (0xB5, 0x1A, 0x58)]


# ---------------------------------------------------------------------------
# Small bitmap helpers. A bitmap is a list of rows, each row a list of 0/1.
# ---------------------------------------------------------------------------
def from_art(lines, width=8):
    rows = []
    for ln in lines:
        ln = ln.ljust(width, ".")[:width]
        rows.append([0 if c in "._ " else 1 for c in ln])
    return rows


def from_bytes(bs):
    return [[(b >> (7 - x)) & 1 for x in range(8)] for b in bs]


def blank(w=8, h=8):
    return [[0] * w for _ in range(h)]


def pad(rows, h, top=0, w=8):
    out = blank(w, top) + [r[:] for r in rows]
    while len(out) < h:
        out.append([0] * w)
    return out[:h]


def halve(rows):
    """Two scanlines -> one pixel row: a pixel is set if either line has it."""
    out = []
    for i in range(0, len(rows), 2):
        a = rows[i]
        b = rows[i + 1] if i + 1 < len(rows) else [0] * len(a)
        out.append([x | y for x, y in zip(a, b)])
    return out


def orb(a, b):
    return [[x | y for x, y in zip(ra, rb)] for ra, rb in zip(a, b)]


def paste(dst, src, x0, y0):
    for y, r in enumerate(src):
        for x, v in enumerate(r):
            if v and 0 <= y0 + y < len(dst) and 0 <= x0 + x < len(dst[0]):
                dst[y0 + y][x0 + x] = 1
    return dst


def crop(rows, x0, y0, w, h):
    return [r[x0:x0 + w] for r in rows[y0:y0 + h]]


def widen(rows):
    """Expand each source bit to 2 pixels (for quad-width Atari players drawn
    by double-width MOBs)."""
    return [[v for v in r for _ in (0, 1)] for r in rows]


class Out:
    def __init__(self):
        self.lines = []
        self.consts = []
        self.cards = 0

    def label(self, name, comment=None):
        if comment:
            self.lines.append("' " + comment)
        self.lines.append(name + ":")

    def card(self, rows, note=None):
        assert len(rows) == 8 and all(len(r) == 8 for r in rows), (note, rows)
        if note:
            self.lines.append("    ' " + note)
        for r in rows:
            self.lines.append('    BITMAP "%s"' % "".join("X" if v else "." for v in r))
        self.cards += 1

    def cards16(self, rows, note=None):
        """An 8x16 MOB: two consecutive cards."""
        rows = pad(rows, 16)
        self.card(rows[:8], note)
        self.card(rows[8:16])

    def const(self, name, value):
        # Constants go to a separate file included before the code: IntyBASIC
        # reads a CONST used before its definition as a variable.
        self.consts.append("    CONST %s = %d" % (name, value))

    def data(self, label, values, comment=None):
        if comment:
            self.lines.append("' " + comment)
        self.lines.append(label + ":")
        for i in range(0, len(values), 12):
            self.lines.append("    DATA " + ",".join("$%04X" % (v & 0xFFFF) for v in values[i:i + 12]))

    def raw(self, text):
        self.lines.append(text)


# ---------------------------------------------------------------------------
# Built-in stand-in art (used without --atr, and for things the Atari draws
# with code rather than bitmaps).
# ---------------------------------------------------------------------------
MOUNTAINS_FALLBACK = [
    from_art(["...X....", "..XXX...", "XXXXXXX."]),
    from_art(["...X....", "..XXX...", ".XXXXX..", "XXXXXXX."]),
    from_art(["...X....", "...X....", "..XXX...", "..XXX...", ".XXXXX..", "XXXXXXX."]),
    from_art(["...X....", "...X....", "...X....", "..XXX...", ".XXXXX..", "XXXXXXX."]),
]

# The plot icons, redrawn for the Intellivision from the Atari's glyphs (map
# characters $11-$1C): an 8x8 card each, 7 wide so the plot's right border
# fits beside them. They go on the plots and on the outfitters' signs, as on
# the Atari, where the shop signs use the same pictures.
INTY_ICONS = {
    "house":    ["...X....", "..XXX...", ".XXXXX..", "..X.X...", "..XXX...", "..XXX...", "........", "........"],
    # Which picture is which good: the Atari's outfitters outfit crystite,
    # smithore, energy and food from left to right (checked on atari800:
    # "$100 TO OUTFIT MULE FOR CRYSTITE" at HPOS 88, smithore at 112), and
    # each shop's sign is its good's picture; the per-good status screens
    # show the same pictures in their corners.
    "crystite": [".....X..", "XXXXXXX.", ".....X..", "........", ".X......", "XXXXXXX.", ".X......", "........"],
    "smithore": ["XXXXX...", "..X.....", "..X.X...", "XXXXXX..", "..X.X...", "..X.....", "..X.....", "........"],
    "energy":   [".X..X...", ".XX.....", "..XX....", "..XXX...", "...XXX..", "...X....", "...X....", "..XXX..."],
    "food":     ["....X...", "XX..X...", ".X..X...", "XXXXXX..", "XX..X...", "....XX..", ".....X..", ".XXXX..."],
    "mule":     ["..XX....", ".XXXX...", ".XXXX...", ".XXXX...", ".X..X...", ".X..X...", ".X..X...", "........"],
}

STORE_ART = [
    "XXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXX", "XX..XX..XX..XXXX", "XX..XX..XX..XXXX",
    "XXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXX", "XX..XXXXXX....XX", "XX..XXXXXX....XX",
    "XX..XXXXXX....XX", "XX..XXXXXX....XX", "XXXXXXXXXX....XX", "XXXXXXXXXX....XX",
    "XXXXXXXXXX....XX", "XXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXX",
]

# River texture: an 8x16 MOB drawn at 2x (4 scanlines per row), repeated down
# column 4. Dots wander the way the Atari's player-3 river does.
RIVER_ART = [".X..X.X.", "..X.X.X.", ".X.X..X.", "..X.X...", ".X..X...", ".X..X.X.", "..X.X.X.", ".X.X.X..",
             "..X.X.X.", ".X..X...", "..X.X.X.", ".X.X.X..", ".X.X.X..", ".X.X.X..", ".X.X..X.", "..X.X.X."]

MULE_ART = [  # facing right, 8x8 drawn at 1x (16 scanlines); two walk frames
    ["......X.", ".....XXX", "X....XX.", ".XXXXXX.", ".XXXXXX.", ".XXXXX..", ".X...X..", ".X...X.."],
    ["......X.", ".....XXX", "X....XX.", ".XXXXXX.", ".XXXXXX.", ".XXXXX..", "X.....X.", "X.....X."],
]

WAMPUS_ART = ["..X..X..", ".XXXXXX.", "XX.XX.XX", ".XXXXXX.", "..X..X..", ".X....X.", "........", "........"]

CURSOR_ART = ["XXXXXXXX"] * 2 + ["XX....XX"] * 12 + ["XXXXXXXX"] * 2  # 8x16 at 1x, double width

# Status/auction store icon (the teal/brown "house" on the per-good screens).
STOREICON_ART = ["...XX...", "..XXXX..", ".XXXXXX.", "XXXXXXXX", ".XX..XX.", ".XX..XX.", ".XXXXXX.", "........"]

FLAG_ART = ["X.......", "XXXX....", "XXXXXXX.", "XXXX....", "X.......", "X.......", "X.......", "........"]

METEOR_ART = ["......XX", ".....XXX", "....XXX.", "...XXX..", ".XXXX...", "XXXXX...", "XXXX....", ".XX....."]

SPARK_ART = ["X......."] + ["........"] * 7

FIRE_ART = [
    ["X..X....", ".X..X.X.", "X.X..X..", ".XX.X..X", "X..XX.X.", ".X.X.XX.", "XX..X..X", ".XX.XX.."],
    [".X..X.X.", "X..X...X", ".X.X.XX.", "X.X.X..X", ".XX..X.X", "X.XX.X..", ".X..XX.X", "X.XX..X."],
]

# Stand-in species: 8 wide x 22 lines, 11 frames each (side R x2, side L x2,
# front x4, back x3), matching the Atari frame order.
def species_fallback(kind):
    heads = {0: ["..XXXX..", "..X..X..", "..XXXX..", "...XX..."],        # mechtron: boxy
             1: ["..XX....", ".XXXX...", "..XX....", "...X...."],        # gollumer
             2: ["..XXXX..", ".XXXXXX.", ".XXXXXX.", "..XXXX.."],        # packer: round
             3: [".X....X.", "..XXXX..", "..X..X..", "..XXXX.."],        # bonzoid: antennae
             4: ["..XXXX..", ".XX..XX.", ".XXXXXX.", ".X.XX.X."],        # spheroid
             5: ["...XX...", "..XXXX..", "...XX...", "..XXXX.."],        # flapper
             6: ["..XX....", "..XXX...", "...XX...", "...XX..."],        # leggite
             7: ["..XXX...", "..XXX...", "...X....", "..XXX..."]}[kind]  # humanoid
    body = ["..XXXX..", ".XXXXXX.", "X.XXXX.X", "X.XXXX.X", "..XXXX..", "..XXXX..", "..X..X.."]
    legs_a = ["..X..X..", "..X..X..", "..X..X..", ".XX..XX."]
    legs_b = [".X....X.", ".X....X.", "X......X", "X......X"]
    legs_s = ["..X.X...", "..X..X..", ".X...X..", "XX...XX."]
    frames = []
    for legs in (legs_s, legs_a):                      # side right
        frames.append(from_art(heads + body + legs))
    for legs in (legs_s, legs_a):                      # side left (mirrored)
        frames.append([r[::-1] for r in from_art(heads + body + legs)])
    for legs in (legs_a, legs_a, legs_b, legs_a):      # front
        frames.append(from_art(heads + body + legs))
    for legs in (legs_a, legs_b, legs_a):              # back
        frames.append(from_art(heads + body + legs))
    return [pad(f, 22) for f in frames]


def map_species_fallback():
    stand = ["........", "...XX...", "...XX...", "..XXXX..", "...XX...", "..X..X..", "..X..X..", "........"]
    step = ["........", "...XX...", "...XX...", "..XXXX..", "...XX...", "...XX...", "..X..X..", "........"]
    return [from_art(stand if f % 2 == 0 else step) for f in range(10)]


# ---------------------------------------------------------------------------
# Atari extraction.
# ---------------------------------------------------------------------------
def load_disk(path):
    raw = open(path, "rb").read()
    crc = zlib.crc32(raw) & 0xFFFFFFFF
    if crc != EXPECTED_CRC:
        print("warning: ATR CRC32 is %08X, expected %08X; addresses may not match" % (crc, EXPECTED_CRC))
    if raw[0:2] != b"\x96\x02":
        sys.exit("not an ATR image")
    body = raw[16:]
    secsize = raw[4] | raw[5] << 8

    def sectors(first, last):
        return body[(first - 1) * secsize:last * secsize]

    main = bytearray(0x10000)
    main[0x1000:0xB000] = sectors(400, 719)
    boot = bytearray(0x10000)
    boot[0x7BEF:0x7BEF + 129 * 128] = sectors(1, 129)
    return main, boot


def atari_palette():
    here = os.path.dirname(os.path.abspath(__file__))
    for p in (os.path.join(here, "..", "..", "..", "tools", "extract", "ntsc.pal"),):
        if os.path.exists(p):
            d = open(p, "rb").read()
            return [tuple(d[i * 3:i * 3 + 3]) for i in range(256)]
    # Crude fallback: grey ramp.
    return [(i & 15) * 17 for i in range(256)]


# The Atari colours the game uses, mapped by eye to the Intellivision
# palette (the NTSC Atari palette is much darker, so plain RGB distance
# sends everything to brown).
ATARI_TO_INTY = {0x00: 0, 0x04: 0, 0x08: 8, 0x0C: 7, 0x0E: 7, 0x24: 11, 0x34: 10, 0x44: 2,
                 0x54: 15, 0x82: 1, 0x84: 1, 0x8A: 13, 0xB4: 4, 0xC4: 5, 0xD4: 5, 0xF4: 14,
                 0x1A: 6, 0xC6: 5, 0x06: 8, 0x86: 13}


def atari_to_inty(c):
    if c in ATARI_TO_INTY:
        return ATARI_TO_INTY[c]
    return ATARI_TO_INTY.get(c & 0xF0 | 4, 8)


def nearest(rgb, allowed=range(16)):
    return min(allowed, key=lambda i: sum((a - b) ** 2 for a, b in zip(rgb, INTY[i])))


class Atari:
    def __init__(self, path):
        self.main, self.boot = load_disk(path)
        self.pal = atari_palette()

    def mapchar(self, c):
        return from_bytes(self.main[0x1C00 + c * 8:0x1C00 + c * 8 + 8])

    def mountains(self):
        return [from_bytes(self.main[a:a + n]) for a, n in ((0x4313, 3), (0x4255, 4), (0x4309, 6), (0x4319, 6))]

    def species(self):
        # server species id -> first frame of its 11 in the parade table.
        # Checked on the Atari's species-select screen: the player graphics
        # there come from these groups. (Frames 89.. are the M.U.L.E., not a
        # species.)
        order = {0: 67, 1: 45, 2: 23, 3: 56, 4: 34, 5: 12, 6: 78, 7: 1}
        out = {}
        for sid, first in order.items():
            frames = []
            for k in range(first, first + 11):
                d = self.boot[0x947C + k * 24 + 2:0x947C + k * 24 + 24]
                frames.append(from_bytes(d))
            out[sid] = frames
        return out

    def map_species(self):
        """The small figures the map uses: 8 species (server order) x 10
        frames x 8 bytes at boot $9215. Byte 0 is a flag, not graphics."""
        out = {}
        for sid in range(8):
            base = 0x9215 + sid * 80
            out[sid] = [from_bytes(b"\0" + self.boot[base + f * 8 + 1:base + f * 8 + 8]) for f in range(10)]
        return out

    def bytes_at(self, a, n):
        return from_bytes(self.main[a:a + n])

    def town(self):
        """40x18 mode-4 template -> 96x144 (cc x lines) grid of Atari colours."""
        cs = self.main[0x1400:0x1800]
        scr = bytearray(self.main[0x2000:0x2000 + 40 * 18])
        for r in range(10, 17):               # the corral interior is drawn by the game
            for c in range(25, 31):
                scr[r * 40 + c] = 0
        top = (0x08, 0x24, 0xB4, 0x0C, 0x00)
        bottom = (0x08, 0xD4, 0x44, 0x0C, 0x00)
        img = []
        for r in range(18):
            regs = top if 1 <= r <= 4 else bottom if 9 <= r <= 15 else top
            for line in range(8):
                row = []
                for x in range(32, 128):           # colour clocks 32..127 = the town box
                    c = scr[r * 40 + x // 4]
                    v = (cs[(c & 0x7F) * 8 + line] >> (6 - 2 * (x % 4))) & 3
                    reg = 4 if (v == 3 and c & 0x80) else v
                    row.append(regs[reg])
                img.append(row)
        return img


# ---------------------------------------------------------------------------
# Card builders.
# ---------------------------------------------------------------------------
def plot_quadrants(icon_rows, dots, owned, mount=None):
    """Build the four 8x8 cards of a plot from 16x32 Atari-line art.

    icon_rows: 16 lines x 8 (the right column interior, rows 8..23)
    Returns dict TL, TR, BL, BR (each 8x8 after halving)."""
    full = blank(16, 32)
    if owned:
        for x in range(16):
            full[0][x] = full[1][x] = 1          # top edge, 2 lines
            full[30][x] = full[31][x] = 1        # bottom edge
        for y in range(32):
            full[y][0] = 1                       # left edge
            full[y][15] = 1                      # right edge
    if icon_rows:
        paste(full, icon_rows, 8, 8)
    if mount:
        for (shape, x, y) in mount:
            paste(full, shape, x, y)
    if dots:
        # Production dots: 1 cc x 2 lines each, two rows of four, bottom left.
        for i in range(dots):
            x = 2 + (i % 4) * 2 - (0 if i < 4 else 1)
            y = 22 + (i // 4) * 4
            full[y][x] = full[y + 1][x] = 1
    h = halve(full)
    return {"TL": crop(h, 0, 0, 8, 8), "TR": crop(h, 8, 0, 8, 8),
            "BL": crop(h, 0, 8, 8, 8), "BR": crop(h, 8, 8, 8, 8)}


# Mountains drawn at Intellivision resolution (one row = two Atari lines, so
# a 3-row peak is the Atari's 6-line mountain). They sit in the plot's
# top-left card, clear of its border (row 0, column 0).
INTY_MOUNTAINS = {
    1: ["........", "........", "....X...", "...XXX..", "..XXXXX.", "........", "........", "........"],
    2: ["........", "...X....", "..XXX...", ".XXXXX..", "......X.", ".....XXX", "....XXXX", "........"],
    3: ["........", "...X....", "..XXX...", ".XXXXX.X", "......XX", "..X..XXX", ".XXX....", "XXXXX..."],
}


def mountain_layout(shapes, n):
    """Where 1..3 peaks sit inside the plot's top-left card (Atari lines)."""
    s = shapes
    if n == 1:
        return [(s[2], 0, 8)]
    if n == 2:
        return [(s[1], 0, 3), (s[3], 3, 9)]
    return [(s[0], 0, 2), (s[2], 3, 6), (s[1], 0, 11)]


def build(atari, out, preview):
    shapes = atari.mountains() if atari else MOUNTAINS_FALLBACK
    icon_names = ["house", "food", "energy", "smithore", "crystite", "mule"]
    # The icons are drawn at the Intellivision's resolution (INTY_ICONS): the
    # Atari's 16-line glyphs don't survive halving. Each icon row is two
    # Atari lines, so it is doubled here and halved again in plot_quadrants.
    icons = {n: [r for r in from_art(v) for _ in (0, 1)] for n, v in INTY_ICONS.items()}

    # ---- map cards ------------------------------------------------------
    out.raw("' ---- map screen cards (GRAM 0..33) ----")
    out.const("MC_TLU", 0)
    out.const("MC_TLO", 3)
    out.const("MC_TR", 7)
    out.const("MC_BL", 13)
    out.const("MC_BR", 22)
    out.const("MC_STORE", 28)
    out.const("MC_RIVER", 32)
    out.const("MC_COUNT", 34)
    out.label("gfx_map")
    for m in (1, 2, 3):
        out.card(from_art(INTY_MOUNTAINS[m]), "TL unowned, %d mountains" % m)
    for m in (0, 1, 2, 3):
        q = plot_quadrants(None, 0, True)["TL"]
        if m:
            q = orb(q, from_art(INTY_MOUNTAINS[m]))
        out.card(q, "TL owned, %d mountains" % m)
    for n in icon_names:
        out.card(plot_quadrants(icons[n], 0, True)["TR"], "TR icon " + n)
    for d in range(9):
        out.card(plot_quadrants(None, d, True)["BL"], "BL owned, %d dots" % d)
    for n in icon_names:
        out.card(plot_quadrants(icons[n], 0, True)["BR"], "BR icon " + n)
    store = from_art(STORE_ART, 16)
    for (x, y) in ((0, 0), (8, 0), (0, 8), (8, 8)):
        out.card(crop(store, x, y, 8, 8), "store")
    out.cards16(from_art(RIVER_ART), "river texture (8x16 MOB at 2x)")

    # ---- time bar -------------------------------------------------------
    out.raw("' ---- time bar: n pixels filled from the bottom (n = 1..8) ----")
    out.label("gfx_bar")
    for n in range(1, 9):
        rows = [[0, 0, 1, 1, 1, 1, 0, 0] if y >= 8 - n else [0] * 8 for y in range(8)]
        out.card(rows, "bar %d" % n)
    # ---- status bars (wide) --------------------------------------------
    out.label("gfx_sbar")
    for n in range(1, 9):
        rows = [[0, 1, 1, 1, 1, 1, 1, 0] if y >= 8 - n else [0] * 8 for y in range(8)]
        out.card(rows, "status bar %d" % n)
    # ---- dotted horizontal lines, one per pixel row ---------------------
    out.label("gfx_hline")
    for n in range(8):
        rows = [[1, 0, 1, 0, 1, 0, 1, 0] if y == n else [0] * 8 for y in range(8)]
        out.card(rows, "dotted line at row %d" % n)
    # Two dotted lines in one card (the auction's sellers' and buyers' lines
    # when they share a card row): card a * 8 + b has rows a and b. Kept in
    # segment 3 ($2100), which has room for data.
    out.raw("    SEGMENT 3")
    out.label("gfx_hline2")
    for n in range(64):
        rows = [[1, 0, 1, 0, 1, 0, 1, 0] if y in (n // 8, n % 8) else [0] * 8 for y in range(8)]
        out.card(rows, "dotted lines at rows %d and %d" % (n // 8, n % 8))
    out.raw("    SEGMENT 1")

    # ---- species --------------------------------------------------------
    out.raw("' ---- species: 8 x 11 frames, each two 8x16 MOBs at 0.5x (4 cards, 16 words) ----")
    out.raw("' frames: 0-1 walk right, 2-3 walk left, 4-7 front, 8-10 back")
    species = atari.species() if atari else {k: species_fallback(k) for k in range(8)}
    out.raw("    SEGMENT 3               ' (room for its 1408 words)")
    out.label("gfx_species")
    for sid in range(8):
        for f, rows in enumerate(species[sid]):
            rows = pad(rows, 32)
            out.cards16(rows[:16], "species %d frame %d" % (sid, f))
            out.cards16(rows[16:32])
    out.raw("    SEGMENT 1")

    # ---- species on the map -------------------------------------------
    # The Atari swaps to small figures on the map: 1 bit = 1 colour clock,
    # 1 line per row, so each frame is one 8x8 card for a MOB at 0.5x.
    out.raw("' ---- species on the map: 8 x 10 frames, one 8x8 card each (MOB at 0.5x) ----")
    out.raw("' frames: 0-1 walk right, 2-3 walk left, 4-5 standing, 6-7 down, 8-9 up")
    # Kept in segment 5 ($4810) with terrain.bas, which reads it.
    small = atari.map_species() if atari else {k: map_species_fallback() for k in range(8)}
    out.raw("    SEGMENT 5")
    out.label("gfx_mapsp")
    for sid in range(8):
        for f, rows in enumerate(small[sid]):
            out.card(pad(rows, 8), "map species %d frame %d" % (sid, f))
    out.raw("    SEGMENT 1")

    # ---- sprites --------------------------------------------------------
    out.raw("' ---- sprites ----")
    out.label("gfx_mule")
    for f in MULE_ART:
        out.card(from_art(f), "M.U.L.E. walk")
    out.label("gfx_wampus")
    out.card(from_art(WAMPUS_ART), "wampus")
    out.label("gfx_cursor")
    out.cards16(from_art(CURSOR_ART), "land grant cursor (8x16 at 1x, double width)")
    out.label("gfx_storeicon")
    out.card(from_art(STOREICON_ART), "store icon")
    out.label("gfx_flag")
    out.card(from_art(FLAG_ART), "plot for sale")
    out.label("gfx_spark")
    out.card(from_art(SPARK_ART), "sunspot spark / radiation dot")
    out.label("gfx_meteor")
    out.card(from_art(METEOR_ART), "meteor")
    out.label("gfx_fire")
    for f in FIRE_ART:
        out.card(from_art(f), "store fire")

    # Ship: nose (8 cc x 10 lines, 0.5x MOB) and body (8 source bits at
    # quad width -> two double-width MOBs, top and bottom halves separate
    # for the two-tone colouring).
    if atari:
        tnose = atari.bytes_at(0x747B, 10)
        pnose = atari.bytes_at(0x73B5, 7)
    else:
        tnose = from_art([".....XXX", "...XXXXX", "..XX..XX", ".XXX..XX", "XX......", "XX......",
                          ".XXX..XX", "..XX..XX", "...XXXXX", ".....XXX"])
        pnose = from_art([".....XXX", "..XX...X", ".XXXX.XX", "XXX.XXX.", "XXXX...X", ".XX.XXX.", ".....XXX"])
    tbody_half = from_art(["....XXX.", "...XXXX.", "..XXXXX.", "XXXXXX..", "XXXXXX..", "..XXXXX.", "...XXXX.", "....XXX."])
    # The pirate hull is half the transport's height. Centre it in the same
    # 8 rows so the shared nose position (fx_draw_ship) lines up with it; the
    # pirate script raises the ship by the same 4 px (PIRATE_Y).
    pbody_half = from_art(["........"] * 2 + ["....XXX.", "XXXXXX..", "XXXXXX..", "....XXX."] + ["........"] * 2)
    out.label("gfx_ship")
    # The nose MOB starts 10 scanlines below the hull's top (fx_draw_ship) and
    # is centred on the hull's middle (scanline 16): the transport's 10 lines
    # at 11-20, the pirate's 7 at 13-19.
    for nm, nose, body, top in (("transport", tnose, tbody_half, 1), ("pirate", pnose, pbody_half, 3)):
        out.cards16(pad(nose, 16, top), nm + " nose")
        wide = widen(body)            # 16 px wide, 8 rows; each row = 4 lines at 2x
        rows = len([r for r in body if any(r)])
        top = [r[:8] for r in wide[:4]] + [[0] * 8] * 4
        top_r = [r[8:] for r in wide[:4]] + [[0] * 8] * 4
        bot = [r[:8] for r in wide[4:8]] + [[0] * 8] * 4
        bot_r = [r[8:] for r in wide[4:8]] + [[0] * 8] * 4
        out.card(top, nm + " body top-left")
        out.card(top_r, nm + " body top-right")
        out.card(bot, nm + " body bottom-left")
        out.card(bot_r, nm + " body bottom-right")
    # Docking frames (the ship sinking into the store), 8 frames 8x16 0.5x.
    if atari:
        seg = atari.main[0x7410:0x7480]
        shapes_dock = []
        cur = []
        for b in seg:
            if b:
                cur.append(b)
            elif cur:
                shapes_dock.append(cur)
                cur = []
        shapes_dock = [from_bytes(s) for s in shapes_dock if len(s) >= 2][:8]
    else:
        shapes_dock = []
    while len(shapes_dock) < 8:
        k = len(shapes_dock)
        w = max(1, 6 - k)
        shapes_dock.append([[1 if abs(x - 3.5) < w / 2 else 0 for x in range(8)] for _ in range(max(2, 8 - k))])
    out.label("gfx_dock")
    for s in shapes_dock:
        out.cards16(pad(s, 16, (16 - len(s)) // 2), "docking frame")

    # Pest (11 lines, two frames), acid-rain cloud (4 frames, 7 rows at 2x
    # double width) and lightning (10 lines).
    if atari:
        pest = [atari.bytes_at(a, 11) for a in (0x778F, 0x779B, 0x77A6, 0x77B2)]
        cloud = [atari.bytes_at(0x798E + 16 * k, 14)[1::2] for k in range(4)]
        bolt = atari.bytes_at(0x7983, 10)
    else:
        pest = [from_art(["....X...", ".XX.X...", ".X..X...", "...XXXX.", ".X.XXX.X", "X.XXX...", "X.XXXX..",
                          "X..X..X.", "....X...", "....X...", "...X...."]),
                from_art(["........", ".X....X.", "X.X..X.X", "X.XXX...", "...XX...", "..XXXXX.", ".X.XX..X",
                          "X..XX...", "..XXXX..", ".X....X.", ".X...X.."])]
        cloud = [from_art(["..XXXX..", ".XXXXXX.", "XXXXXXXX", "XXXXXXXX", "XXXXXXXX", ".XXXXXX.", "..XXXX.."])] * 4
        bolt = from_art(["..X.....", "...X....", "....X...", "..XXXX..", "...X....", "....X...", "..XXXX..",
                         "...X....", "....X...", ".....X.."])
    out.label("gfx_pest")
    for p in pest:
        out.cards16(p, "pest")
    out.label("gfx_cloud")
    for c in cloud:
        out.card(pad(c, 8), "acid rain cloud (2x, double width)")
    out.label("gfx_bolt")
    out.cards16(bolt, "lightning")

    # ---- town -----------------------------------------------------------
    build_town(atari, out, preview)


# Town building positions in 8x8 Intellivision cards of the 12x9 town box.
# (Atari cells of 4 cc x 8 lines: x/2, y/2.)
TOWN_FALLBACK = {
    # name: (x, y, w, h, bg colour, fg colour, sign text), in cards; the doors
    # are where the Atari's are (isr.bas town_door_lo).
    "food": (0, 0, 3, 2, 4, 7, "F"), "energy": (3, 0, 3, 2, 4, 7, "E"),
    "smithore": (6, 0, 3, 2, 11, 7, "S"), "crystite": (9, 0, 3, 2, 11, 7, "C"),
    "assay": (0, 4, 3, 4, 5, 7, "A"), "land": (3, 4, 2, 3, 5, 7, "L"),
    "pub": (5, 4, 3, 3, 2, 7, "P"), "corral": (8, 4, 3, 5, 8, 0, ""),
}


def build_town(atari, out, preview):
    """Emit the town box as MODE 1 (foreground/background) cards.

    Each 8x8 card gets the background colour (any of 16) and foreground colour
    (0..7) that reproduce its pixels best. Identical cards are shared."""
    W, H = 12, 9
    cells = []
    if atari:
        img = atari.town()                       # 144 lines x 96 cc of Atari colours
        pal = atari.pal
        for cy in range(H):
            for cx in range(W):
                px = []
                for y in range(8):
                    line = cy * 16 + y * 2       # every other scanline
                    row = []
                    for x in range(8):
                        a = img[line][cx * 8 + x]
                        b = img[line + 1][cx * 8 + x]
                        # Keep thin features: black lettering on a grey
                        # sign wins over the sign; otherwise colour wins
                        # over the black street.
                        if {a, b} == {0x00, 0x08}:
                            row.append(0x00)
                        else:
                            row.append(b if a == 0 else a)
                    px.append(row)
                cells.append(quantize_card([[INTY[atari_to_inty(c)] for c in r] for r in px]))
    else:
        grid = [[(0, 0, 0)] * 96 for _ in range(72)]
        for name, (x, y, w, h, bg, fg, sign) in TOWN_FALLBACK.items():
            for yy in range(y * 8, (y + h) * 8):
                for xx in range(x * 8, (x + w) * 8):
                    grid[yy][xx] = INTY[bg]
            if sign:
                # a light sign strip in the middle column with the letter
                for yy in range(y * 8 + 2, y * 8 + 7):
                    for xx in range(x * 8 + 5, x * 8 + 11):
                        grid[yy][xx] = INTY[fg]
        for cy in range(H):
            for cx in range(W):
                px = [grid[cy * 8 + y][cx * 8:cx * 8 + 8] for y in range(8)]
                cells.append(quantize_card(px))
    # The outfitters' signs: a grey sign with the good's icon in black, one card
    # under a card of the building's own colour (card columns 1, 4, 7, 10).
    # The buildings start half way down card row 0 (Atari line 8): its top
    # half stays street. Colours: the Atari's $B4 (dark green) and $24 (brown).
    for i, good in enumerate(("crystite", "smithore", "energy", "food")):
        col = 1 + i * 3
        building_bg = (4, 4, 11, 11)[i]
        cells[0 * W + col] = (from_art(["XXXXXXXX"] * 4 + ["........"] * 4), 0, building_bg)
        cells[1 * W + col] = (from_art(INTY_ICONS[good]), 0, 8)
    uniq = []
    screen = []
    for bits, fg, bg in cells:
        if not any(any(r) for r in bits):
            screen.append((None, fg, bg))
            continue
        key = tuple(tuple(r) for r in bits)
        if key not in uniq:
            uniq.append(key)
        screen.append((uniq.index(key), fg, bg))
    if len(uniq) > 40:
        print("warning: town needs %d cards (budget 40); merging nearest" % len(uniq))
        uniq, screen = merge_cards(uniq, screen, 40)
    out.raw("' ---- town (MODE 1): %d cards, then a 12x9 BACKTAB image ----" % len(uniq))
    out.const("TOWN_NCARDS", len(uniq))
    out.label("gfx_town")
    for k in uniq:
        out.card([list(r) for r in k], "town")
    words = []
    for (idx, fg, bg) in screen:
        w = fb_word(idx, fg, bg)
        words.append(w)
    out.data("town_screen", words, "12x9 words: card, fg, bg for MODE 1 (card index relative to GRAM slot 0)")
    if preview:
        save_town_preview(uniq, screen, os.path.join(preview, "town.png"))


def fb_word(card, fg, bg):
    """A MODE 1 BACKTAB word for GRAM card `card` (or GROM space if None)."""
    w = 0
    if card is not None:
        w = 0x0800 | (card << 3)
    w |= fg & 7
    if bg & 1:
        w |= 0x0200
    if bg & 2:
        w |= 0x0400
    # STIC FG/BG mode: bits 9, 10, 13, 12 hold background bits 0, 1, 2, 3
    # (matches BG_* in IntyBASIC's contrib/constants.bas).
    if bg & 4:
        w |= 0x2000
    if bg & 8:
        w |= 0x1000
    return w


def quantize_card(px):
    """8x8 of RGB -> (bits, fg 0..7, bg 0..15) minimising colour error."""
    best = None
    colours = {}
    for r in px:
        for c in r:
            colours[c] = colours.get(c, 0) + 1
    for bg in range(16):
        for fg in range(8):
            err = 0
            bits = []
            for r in px:
                row = []
                for c in r:
                    ef = sum((a - b) ** 2 for a, b in zip(c, INTY[fg]))
                    eb = sum((a - b) ** 2 for a, b in zip(c, INTY[bg]))
                    if ef < eb:
                        row.append(1)
                        err += ef
                    else:
                        row.append(0)
                        err += eb
                bits.append(row)
            if best is None or err < best[0]:
                best = (err, bits, fg, bg)
    _, bits, fg, bg = best
    if not any(any(r) for r in bits):
        fg = 0
    return bits, fg, bg


def merge_cards(uniq, screen, budget):
    uniq = list(uniq)
    remap = {i: i for i in range(len(uniq))}

    def dist(a, b):
        return sum(x != y for ra, rb in zip(a, b) for x, y in zip(ra, rb))

    alive = list(range(len(uniq)))
    while len(alive) > budget:
        best = None
        for i in alive:
            for j in alive:
                if i < j:
                    d = dist(uniq[i], uniq[j])
                    if best is None or d < best[0]:
                        best = (d, i, j)
        _, i, j = best
        alive.remove(j)
        for k, v in remap.items():
            if v == j:
                remap[k] = i
    order = {old: new for new, old in enumerate(alive)}
    new_uniq = [uniq[i] for i in alive]
    new_screen = [(order[remap[idx]] if idx is not None else None, fg, bg) for idx, fg, bg in screen]
    return new_uniq, new_screen


def save_town_preview(uniq, screen, path):
    try:
        from PIL import Image
    except ImportError:
        return
    im = Image.new("RGB", (96, 72))
    for n, (idx, fg, bg) in enumerate(screen):
        cx, cy = n % 12, n // 12
        for y in range(8):
            for x in range(8):
                on = idx is not None and uniq[idx][y][x]
                im.putpixel((cx * 8 + x, cy * 8 + y), INTY[fg] if on else INTY[bg])
    im.resize((96 * 6, 72 * 6), Image.NEAREST).save(path)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--atr")
    ap.add_argument("--preview")
    ap.add_argument("out")
    a = ap.parse_args()
    atari = Atari(a.atr) if a.atr else None
    if a.preview:
        os.makedirs(a.preview, exist_ok=True)
    out = Out()
    out.raw("' gfx.bas -- GENERATED by tools/mkgfx.py (%s). Do not edit or commit." %
            ("converted from the Atari disk" if atari else "built-in stand-in art"))
    out.raw("")
    build(atari, out, a.preview)
    open(a.out, "w").write("\n".join(out.lines) + "\n")
    cpath = a.out[:-4] + "const.bas" if a.out.endswith(".bas") else a.out + ".const"
    open(cpath, "w").write("' %s -- GENERATED by tools/mkgfx.py with %s.\n\n" %
                           (os.path.basename(cpath), os.path.basename(a.out)) + "\n".join(out.consts) + "\n")
    print("%s: %d cards (%s)" % (a.out, out.cards, "Atari" if atari else "stand-in"))


if __name__ == "__main__":
    main()

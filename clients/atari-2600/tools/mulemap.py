#!/usr/bin/env python3
"""mulemap.py -- the M.U.L.E. map as the 2600 draws it: the model.

This is the reference for the cartridge's FN_BLIT_MULEMAP (fn-2600's
vcs_render.c composes exactly these bytes; its host test compares), and for
the layout ROM, which bakes them in.

THE PICTURE. The Atari's map is 9 x 5 plots of 16 colour clocks by 32
scanlines at clocks 8-151. The 2600's playfield pixel is 4 clocks, so a plot
is 4 playfield pixels wide -- and a plot row's 32 lines are 16 ENTRIES of 2
scanlines. The playfield has ONE colour per scanline, so what an entry can
show is decided by which colour that entry is drawn in, the same trick the
Battleship client plays with its three kinds of line. The schedule (ENTRY):

   0-3    the owner's bracket, top: seat 0 outermost ... seat 3 innermost,
          each in its seat's colour
   4-8    the picture, white: the M.U.L.E.'s good as a 4x5 glyph, the
          store, or (production) the units made as bars
   9-11   the ground, dark: mountains (1-3 peaks), the river's dots
   12-15  the owner's bracket, bottom: seat 3 innermost ... seat 0 outermost

The background is the Atari map's grey.

An owned plot is a coloured bracket above and below it -- its inset says
whose as well as its colour does. The cursor is not part of the map: it is
player 0, double width, drawn over it.

TABLES. Six registers -- PF0/PF1/PF2 of the left half, then of the right --
each a plane of the cartridge's text area, entry y at byte MAP0 + y
(y = row * 16 + entry, 0-79). MAP0 = 12 keeps text rows 0-1 and 16-20 free.

usage: mulemap.py [--png out.png] [--asm out.inc] [--test N]
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

ROWS, COLS, NPLOTS = 5, 9, 45
ENTRIES = 16                    # per plot row, 2 scanlines each
NENT = ROWS * ENTRIES           # 80
MAP0 = 12                       # plane byte of entry 0
STORE = 22

T_PLAIN, T_RIVER, T_MOUNTAIN, T_STORE = 0, 1, 2, 3
G_FOOD, G_ENERGY, G_SMITHORE, G_CRYSTITE = 0, 1, 2, 3

# NTSC colours
C_BLACK, C_WHITE, C_LIGHT = 0x00, 0x0E, 0x0A
C_DARK = 0x96        # the river (and, sharing its colour, mountains and the store's ground): blue
C_GROUND = 0x08
C_SEAT = (0x84, 0x54, 0xC4, 0x34)       # the Atari's player colours

# entry -> what it draws, and in which colour
E_BRK_TOP = {0: 0, 1: 1, 2: 2, 3: 3}    # entry -> seat
E_BRK_BOT = {15: 0, 14: 1, 13: 2, 12: 3}
E_PIC = (4, 5, 6, 7, 8)
E_GND = (9, 10, 11)


def entry_colour(e):
    if e in E_BRK_TOP:
        return C_SEAT[E_BRK_TOP[e]]
    if e in E_BRK_BOT:
        return C_SEAT[E_BRK_BOT[e]]
    if e in E_GND:
        return C_DARK
    return C_WHITE


COLTAB = [entry_colour(y % ENTRIES) for y in range(NENT)]


def g(*rows):
    return [int(r.replace(".", "0").replace("#", "1"), 2) for r in rows]


# 4x5 pictures, MSB = the plot's left pixel.
GLYPH = {
    G_FOOD:     g("####", "#...", "###.", "#...", "#..."),
    G_ENERGY:   g("####", "#...", "###.", "#...", "####"),
    G_SMITHORE: g(".###", "#...", ".##.", "...#", "###."),
    G_CRYSTITE: g(".###", "#...", "#...", "#...", ".###"),
}
MOUNT = {
    1: g("....", ".#..", "###."),
    2: g(".#..", "###.", "####"),
    3: g("#.#.", "####", "####"),
}
STOREPIC = g("####", "#..#", "####", "#..#", "####")
STOREGND = g("####", "####", "####")
RIVERGND = g(".#..", "..#.", ".#..")
FULL = 0b1111


def plot_entries(terrain, owner, mule, prod=0, flags=0):
    """16 x 4-bit patterns for one plot."""
    out = [0] * ENTRIES
    t = terrain & 3
    if owner is not None and 0 <= owner < 4:
        for e, s in E_BRK_TOP.items():
            if s == owner:
                out[e] = FULL
        for e, s in E_BRK_BOT.items():
            if s == owner:
                out[e] = FULL
    gnd = None
    if t == T_RIVER:
        gnd = RIVERGND
    elif t == T_STORE:
        gnd = STOREGND
    elif t == T_MOUNTAIN:
        gnd = MOUNT.get(((terrain >> 2) & 3) or 1)
    if gnd:
        for i, e in enumerate(E_GND):
            out[e] = gnd[i]
    pic = None
    if flags & 1 and mule is not None and 0 <= mule < 4:
        # production: a M.U.L.E.'s plot shows the units it made, as bars
        n = min(prod & 15, 8)
        pic = [0, FULL & ~(FULL >> min(n, 4)), 0, FULL & ~(FULL >> max(n - 4, 0)), 0]
    elif mule is not None and 0 <= mule < 4:
        pic = GLYPH[mule]
    elif t == T_STORE:
        pic = STOREPIC
    if pic:
        for i, e in enumerate(E_PIC):
            out[e] = pic[i]
    return out


def pf_pixels(state, flags=0):
    """state -> 80 rows of 40 playfield pixels (0/1)."""
    rows = [[0] * 40 for _ in range(NENT)]
    for p in range(NPLOTS):
        r, c = divmod(p, COLS)
        ents = plot_entries(state["terrain"][p], state["owner"][p], state["mule"][p],
                            state["prod"][p], flags)
        for e in range(ENTRIES):
            v = ents[e]
            for b in range(4):
                if v & (8 >> b):
                    rows[r * ENTRIES + e][2 + 4 * c + b] = 1
    return rows


def pixel_reg(px):
    """playfield pixel 0-39 -> (register 0-5, bit mask)."""
    half, q = divmod(px, 20)
    if q < 4:
        return 3 * half + 0, 1 << (4 + q)
    if q < 12:
        return 3 * half + 1, 1 << (7 - (q - 4))
    return 3 * half + 2, 1 << (q - 12)


def tables(state, flags=0):
    """state -> six 80-byte tables, PF0L PF1L PF2L PF0R PF1R PF2R."""
    t = [[0] * NENT for _ in range(6)]
    for y, row in enumerate(pf_pixels(state, flags)):
        for px, on in enumerate(row):
            if on:
                reg, m = pixel_reg(px)
                t[reg][y] |= m
    return t


def test_state():
    """The MekkoGX test mode's map (clients/mekkogx/src/teststart.c)."""
    terr, owner, mule, prod = [], [None] * NPLOTS, [None] * NPLOTS, [0] * NPLOTS
    for mp in range(NPLOTS):
        c = T_PLAIN
        if mp % 9 == 4:
            c = T_RIVER
        if mp == STORE:
            c = T_STORE
        if mp % 9 == (mp // 9) % 4:
            c = T_MOUNTAIN + ((mp // 9) % 3 + 1) * 4
        if mp % 9 == 5 + (mp // 9) % 4:
            c = T_MOUNTAIN + ((mp // 9 + 1) % 3 + 1) * 4
        terr.append(c)
    owner[10], mule[10], prod[10] = 0, G_FOOD, 5
    owner[12], mule[12], prod[12] = 1, G_ENERGY, 3
    owner[30], mule[30] = 3, G_ENERGY
    owner[31], mule[31], prod[31] = 0, G_SMITHORE, 8
    owner[14] = 2
    owner[5] = 1
    owner[40], mule[40] = 2, G_CRYSTITE
    return {"terrain": terr, "owner": owner, "mule": mule, "prod": prod}


def palette():
    p = os.path.join(HERE, "..", "..", "..", "tools", "extract", "ntsc.pal")
    d = open(p, "rb").read()
    return [tuple(d[i * 3:i * 3 + 3]) for i in range(256)]


def render_png(state, path, scale=3, flags=0):
    from PIL import Image
    pal = palette()
    rows = pf_pixels(state, flags)
    img = Image.new("RGB", (160, NENT * 2))
    for y in range(NENT * 2):
        e = y // 2
        col = pal[COLTAB[e]]
        for x in range(160):
            img.putpixel((x, y), col if rows[e][x // 4] else pal[C_GROUND])
    img = img.resize((160 * scale * 2, NENT * 2 * scale), Image.NEAREST)
    img.save(path)


def write_asm(state, path):
    t = tables(state)
    with open(path, "w") as f:
        f.write("; generated by tools/mulemap.py -- do not edit\n")
        for i in range(6):
            f.write("MT%d:\n" % i)
            for k in range(0, NENT, 16):
                f.write("        DB      %s\n" % ",".join("$%02X" % b for b in t[i][k:k + 16]))
        f.write("MCOLTAB:\n")
        for k in range(0, NENT, 16):
            f.write("        DB      %s\n" % ",".join("$%02X" % b for b in COLTAB[k:k + 16]))


def wire(state):
    """state -> the 225 MuleMap bytes."""
    b = bytearray(225)
    for p in range(NPLOTS):
        b[p] = state["terrain"][p]
        o, m = state["owner"][p], state["mule"][p]
        b[45 + p] = 0xFF if o is None else o
        b[90 + p] = 0xFF if m is None else m
        b[135 + p] = 0xFF
        b[180 + p] = state["prod"][p]
    return bytes(b)


def write_gold(state, path):
    """The host test's golden (fn-2600 firmware/host_test/mule_gold.h)."""
    with open(path, "w") as f:
        f.write("/* mule_gold.h -- GENERATED by fujinet-multiplayer-mule's\n"
                " * clients/atari-2600/tools/mulemap.py --gold. The M.U.L.E. map\n"
                " * FN_BLIT_MULEMAP must compose, byte for byte. */\n")
        f.write("static const uint8_t mule_gold_src[225] = {")
        f.write(",".join("0x%02X" % v for v in wire(state)))
        f.write("};\n")
        for fl in (0, 1):
            t = tables(state, fl)
            f.write("static const uint8_t mule_gold_%s[6][80] = {\n" % ("glyph", "prod")[fl])
            for r in range(6):
                f.write("  {%s},\n" % ",".join("0x%02X" % v for v in t[r]))
            f.write("};\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--png")
    ap.add_argument("--asm")
    ap.add_argument("--gold")
    ap.add_argument("--col")
    ap.add_argument("--prod", action="store_true")
    a = ap.parse_args()
    st = test_state()
    if a.gold:
        write_gold(st, a.gold)
    if a.col:
        with open(a.col, "w") as f:
            f.write("; generated by tools/mulemap.py -- do not edit\n")
            f.write("MCOLTAB:\n")
            for k in range(0, NENT, 16):
                f.write("        DB      %s\n" % ",".join("$%02X" % b for b in COLTAB[k:k + 16]))
    if a.png:
        render_png(st, a.png, flags=1 if a.prod else 0)
    if a.asm:
        write_asm(st, a.asm)


if __name__ == "__main__":
    main()

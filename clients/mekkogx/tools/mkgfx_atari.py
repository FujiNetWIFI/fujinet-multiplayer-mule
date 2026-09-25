#!/usr/bin/env python3
"""Build the Atari client's graphics (gen/atari/gfx.c) from the original disk.

usage: mkgfx_atari.py [--atr "M.U.L.E. (1983)(Electronic Arts)(US).atr"] [--preview dir] out.c

The Atari needs no conversion: the font, the map and town characters, the
town's screen and its colours, the mountains, the store and the sprites are
copied from the original (CRC32 F76D2896) as they are. Those graphics belong
to their copyright holders; the generated file must not be committed.
Without --atr, stand-in art is used so the game still builds (the Makefile
warns): it looks nothing like M.U.L.E.
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import mkgfx_hgr as hgr  # noqa: E402
from mkgfx_hgr import intv  # noqa: E402

# The town's colours per mode 4 row (COLBK, PF0-3), as the original's DLI
# ($2EB9) sets them: rows 0-5, then from row 6.
TOWN_TOP = (0x08, 0x24, 0xB4, 0x08, 0x00)
TOWN_BOTTOM = (0x08, 0xD4, 0x44, 0x08, 0x00)


def pack(rows, w=8):
    """rows of 0/1 (w wide, w <= 8) -> bytes, bit 7 leftmost."""
    out = []
    for r in rows:
        v = 0
        for k in range(8):
            if k < len(r) and r[k]:
                v |= 0x80 >> k
        out.append(v)
    return out


def pad(rows, h):
    rows = [list(r) for r in rows[:h]]
    while len(rows) < h:
        rows.append([0] * 8)
    return rows


def taller(rows, k):
    return [r for r in rows for _ in range(k)]


class Shape:
    def __init__(self, name, frames, h, size=0):
        self.name, self.h, self.size = name, h, size
        self.frames = [pack(pad(f, h)) for f in frames]


def shapes(atari):
    out = []
    sp = atari.species() if atari else {k: intv.species_fallback(k) for k in range(8)}
    out.append(Shape("species", [f for sid in range(8) for f in sp[sid]], 22))
    fig = atari.map_species() if atari else {k: intv.map_species_fallback() for k in range(8)}
    out.append(Shape("figure", [f for sid in range(8) for f in fig[sid]], 8))
    if atari:
        # the original's M.U.L.E.: town frames facing right at $54C0 and
        # $54D8 (24 bytes each; then left, front, down, up), map ones at
        # $55AF (8 each)
        rows = lambda a, n: [hgr.bits(b) for b in atari.main[a:a + n]]
        mule = [rows(a, 24) for a in (0x54C0, 0x54D8)] + [rows(a, 8) for a in (0x55AF, 0x55B7)]
    else:
        mule = [intv.from_art(f) for f in intv.MULE_ART]
        mule = [taller(f, 2) for f in mule] + mule
    out.append(Shape("mule", mule + [[r[::-1] for r in f] for f in mule], 24))
    out.append(Shape("wampus", [taller(intv.from_art(intv.WAMPUS_ART), 2)], 16))
    box = [[1] * 8, [1] * 8] + [[1] + [0] * 6 + [1] for _ in range(28)] + [[1] * 8, [1] * 8]
    out.append(Shape("cursor", [box], 32, 1))
    if atari:
        tnose = atari.bytes_at(0x747B, 10)
        pnose = atari.bytes_at(0x73B5, 7)
    else:
        tnose = intv.from_art([".....XXX", "...XXXXX", "..XX..XX", ".XXX..XX", "XX......", "XX......",
                               ".XXX..XX", "..XX..XX", "...XXXXX", ".....XXX"])
        pnose = intv.from_art([".....XXX", "..XX...X", ".XXXX.XX", "XXX.XXX.", "XXXX...X", ".XX.XXX.", ".....XXX"])
    out.append(Shape("nose", [tnose, [[0] * 8] + pnose], 10))
    tbody = intv.from_art(["....XXX.", "...XXXX.", "..XXXXX.", "XXXXXX..", "XXXXXX..", "..XXXXX.", "...XXXX.", "....XXX."])
    pbody = intv.from_art(["........"] * 2 + ["....XXX.", "XXXXXX..", "XXXXXX..", "....XXX."] + ["........"] * 2)
    out.append(Shape("hull_top", [taller(b[:4], 4) for b in (tbody, pbody)], 16, 3))
    out.append(Shape("hull_bot", [taller(b[4:], 4) for b in (tbody, pbody)], 16, 3))
    docks = []
    if atari:
        seg, cur = atari.main[0x7410:0x7480], []
        for b in seg:
            if b:
                cur.append(b)
            elif cur:
                docks.append(cur)
                cur = []
        docks = [[hgr.bits(b) for b in s] for s in docks if len(s) >= 2][:8]
    while len(docks) < 8:
        k = len(docks)
        w = max(1, 6 - k)
        docks.append([[1 if abs(x - 3.5) < w / 2 else 0 for x in range(8)] for _ in range(max(2, 8 - k))])
    out.append(Shape("dock", [[[0] * 8] * ((16 - len(s)) // 2) + s for s in docks[0:8:2]], 16))
    if atari:
        pest = [[hgr.bits(b) for b in atari.main[a:a + 11]] for a in (0x778F, 0x779B, 0x77A6, 0x77B2)]
        cloud = [[hgr.bits(b) for b in atari.main[0x798E + 16 * k:0x798E + 16 * k + 14][1::2]] for k in range(4)]
        bolt = [hgr.bits(b) for b in atari.main[0x7983:0x7983 + 10]]
    else:
        p = intv.from_art(["....X...", ".XX.X...", ".X..X...", "...XXXX.", ".X.XXX.X", "X.XXX...", "X.XXXX..",
                           "X..X..X.", "....X...", "....X...", "...X...."])
        pest = [p] * 4
        c = intv.from_art(["..XXXX..", ".XXXXXX.", "XXXXXXXX", "XXXXXXXX", "XXXXXXXX", ".XXXXXX.", "..XXXX.."])
        cloud = [c] * 4
        bolt = intv.from_art(["..X.....", "...X....", "....X...", "..XXXX..", "...X....", "....X...", "..XXXX..",
                              "...X....", "....X...", ".....X.."])
    out.append(Shape("pest", pest, 11))
    out.append(Shape("cloud", [taller(c, 4) for c in cloud], 28, 1))
    out.append(Shape("bolt", [bolt], 10))
    out.append(Shape("spark", [[[1], [1]]], 2))
    out.append(Shape("meteor", [taller(intv.from_art(intv.METEOR_ART), 2)], 16))
    return out


def c_bytes(vals, per=16, indent="    "):
    return "\n".join(indent + ",".join("0x%02X" % v for v in vals[i:i + per]) + ","
                     for i in range(0, len(vals), per))


def standin_glyph(ch):
    """A stand-in glyph (5 x 7, one row down, two in) as 8 bytes."""
    rows = hgr.STANDIN_FONT.get(ch, [0] * 7)
    return bytes([0] + [r << 2 for r in rows])


def standin_charsets():
    """No disk: a font from the stand-in glyphs, blank map and town sets."""
    font = bytearray(512)
    for c in range(64):
        font[c * 8:c * 8 + 8] = standin_glyph(chr(32 + c))
    return bytes(font), bytes(512), bytes(1024), bytes(720)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--atr")
    ap.add_argument("--preview")
    ap.add_argument("out")
    a = ap.parse_args()
    atari = hgr.Atari(a.atr) if a.atr else None

    if atari:
        m = atari.main
        font, mapch, townch = bytearray(m[0x1800:0x1A00]), m[0x1C00:0x1E00], m[0x1400:0x1800]
        # The original draws bars with % & ' ( ) * +: give them glyphs back.
        for ch in "%&'()*+":
            c = ord(ch) - 32
            font[c * 8:c * 8 + 8] = standin_glyph(ch)
        town = bytearray(m[0x2000:0x2000 + 720])
        for r in range(10, 17):          # the corral: the game draws its inside
            for c in range(25, 31):
                town[r * 40 + c] = 0
        mtn = [m[a_:a_ + n] for a_, n in ((0x4313, 3), (0x4255, 4), (0x4309, 6), (0x4319, 6))]
        store = m[0x4328:0x4330]
    else:
        font, mapch, townch, town = standin_charsets()
        mtn = [bytes(pack(s)) for s in intv.MOUNTAINS_FALLBACK]
        store = bytes([0xFF, 0x81, 0x81, 0xFF, 0x81, 0x81, 0x81, 0xFF])

    src = ["/* gfx.c -- GENERATED by tools/mkgfx_atari.py (%s). Do not edit or commit. */" %
           ("copied from the Atari disk" if atari else "STAND-IN art: no ATR given"),
           "#include <stdint.h>", '#include "../../src/atari/gfxdata.h"', ""]
    for seg, name, data in (("FONT", "font", font), ("MAPCHARS", "mapchars", mapch)):
        src.append('#pragma rodata-name (push, "%s")' % seg)
        src.append("const uint8_t %s[%d] = {" % (name, len(data)))
        src.append(c_bytes(list(data)))
        src.append("};")
        src.append("#pragma rodata-name (pop)")
    src.append("const uint8_t town_scr[18 * 40] = {")
    src.append(c_bytes(list(town), 40))
    src.append("};")
    src.append("const uint8_t town_pal[18][5] = {")
    for r in range(18):
        p = TOWN_TOP if r <= 5 else TOWN_BOTTOM
        src.append("    { %s }," % ",".join("0x%02X" % v for v in p))
    src.append("};")
    src.append("const uint8_t mtn_shape[4][6] = {")
    for s in mtn:
        s = list(s)
        src.append("    { %s }," % ",".join("0x%02X" % v for v in [0] * (6 - len(s)) + s))
    src.append("};")
    src.append("const uint8_t mtn_h[4] = { %s };" % ",".join(str(len(s)) for s in mtn))
    src.append("const uint8_t store_p3[8] = { %s };" % ",".join("0x%02X" % v for v in store))
    src.append("const uint8_t pics[4][8] = {")
    for n, art in hgr.PICS:
        src.append("    { %s }, /* %s */" % (",".join("0x%02X" % v for v in pack(intv.from_art(art))), n))
    src.append("};")
    shs = shapes(atari)
    # The town's characters (gfxhi.s): the ones its picture doesn't use hold
    # the small sprites (they are loaded there with it).
    # ($7B-$7C too: the corral's M.U.L.E.s; and 1-31, to be safe)
    town_used = {b & 0x7F for b in town} | set(range(32)) | {0x7B, 0x7C}
    free = [c for c in range(128) if c not in town_used]
    runs, start = [], None
    for c in range(129):
        if c < 128 and c in free:
            start = c if start is None else start
        elif start is not None:
            runs.append([start * 8, c * 8])
            start = None
    hi = bytearray(townch)
    placed = {}
    for sh in sorted(shs, key=lambda x: -len(x.frames) * x.h):
        data = [b for f in sh.frames for b in f]
        if sh.name in ("species", "figure"):
            continue
        for r in runs:
            if r[1] - r[0] >= len(data):
                hi[r[0]:r[0] + len(data)] = bytes(data)
                placed[sh.name] = r[0]
                r[0] += len(data)
                break
    hs = ["; gfxhi.s -- GENERATED by tools/mkgfx_atari.py. Do not edit or commit.",
          '        .segment "TOWNCHARS"', "        .export _townchars"]
    for n, off in placed.items():
        hs.append("        .export _sh_%s" % n)
        hs.append("_sh_%s = _townchars + %d" % (n, off))
    hs.append("_townchars:")
    for i in range(0, 1024, 16):
        hs.append("        .byte   " + ",".join("$%02X" % v for v in hi[i:i + 16]))
    open(os.path.join(os.path.dirname(a.out), "gfxhi.s"), "w").write("\n".join(hs) + "\n")
    for s in shs:
        if s.name in placed:
            src.append("extern const uint8_t sh_%s[];" % s.name)
            continue
        data = [b for f in s.frames for b in f]
        src.append("static const uint8_t sh_%s[%d] = {" % (s.name, len(data)))
        src.append(c_bytes(data))
        src.append("};")
    src.append("const ShapeDef atari_shapes[%d] = {" % len(shs))
    for s in shs:
        src.append("    { sh_%s, %d, %d }," % (s.name, s.h, s.size))
    src.append("};")
    open(a.out, "w").write("\n".join(src) + "\n")
    print("%s: %s" % (a.out, "Atari graphics" if atari else "STAND-IN art"))


if __name__ == "__main__":
    main()

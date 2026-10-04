#!/usr/bin/env python3
"""Decode the Intellivision M.U.L.E. graphics straight out of the built ROM.

usage: romgfx.py [--intv clients/intv] [--check] [--preview DIR]

Reads mule.bin + mule.cfg (ROM image and its address map) and mule.sym
(label addresses). IntyBASIC packs a BITMAP card into 4 words: each word
holds two pixel rows, the first row in the low byte.

--check   compare every card against gen/gfx.bas (the source they came from)
--preview write a PNG sheet of every block to DIR
"""
import argparse
import os
import re
import sys

PAL = [(0x00, 0x00, 0x00), (0x00, 0x2D, 0xFF), (0xFF, 0x3D, 0x10), (0xC9, 0xCF, 0xAB),
       (0x38, 0x6B, 0x3F), (0x00, 0xA7, 0x56), (0xFA, 0xEA, 0x50), (0xFF, 0xFC, 0xFF),
       (0xBD, 0xAC, 0xC8), (0x24, 0xB8, 0xFF), (0xFF, 0xB4, 0x1F), (0x54, 0x6E, 0x00),
       (0xFF, 0x4E, 0x57), (0xA4, 0x96, 0xFF), (0x75, 0xCC, 0x80), (0xB5, 0x1A, 0x58)]

BLACK, BLUE, RED, TAN, DKGREEN, GREEN, YELLOW, WHITE = range(8)
GREY, CYAN, ORANGE, BROWN, PINK, LTBLUE, YELGREEN, PURPLE = range(8, 16)

BLOCKS = ["GFX_MAP", "GFX_BAR", "GFX_SBAR", "GFX_HLINE", "GFX_HLINE2", "GFX_SPECIES",
          "GFX_MAPSP", "GFX_MULE", "GFX_WAMPUS", "GFX_CURSOR", "GFX_STOREICON",
          "GFX_FLAG", "GFX_SPARK", "GFX_METEOR", "GFX_FIRE", "GFX_SHIP", "GFX_DOCK",
          "GFX_PEST", "GFX_CLOUD", "GFX_BOLT", "GFX_TOWN", "TOWN_SCREEN"]


class Rom:
    def __init__(self, intv):
        self.intv = intv
        data = open(os.path.join(intv, "mule.bin"), "rb").read()
        self.words = [(data[i] << 8) | data[i + 1] for i in range(0, len(data) - 1, 2)]
        self.map = []                                   # (file_lo, file_hi, addr)
        for ln in open(os.path.join(intv, "mule.cfg")):
            m = re.match(r"\s*\$([0-9A-Fa-f]+)\s*-\s*\$([0-9A-Fa-f]+)\s*=\s*\$([0-9A-Fa-f]+)", ln)
            if m and "RAM" not in ln:
                self.map.append(tuple(int(g, 16) for g in m.groups()))
        self.sym = {}
        for ln in open(os.path.join(intv, "mule.sym")):
            p = ln.split()
            if len(p) == 2 and p[1].startswith("label_"):
                self.sym[p[1][6:]] = int(p[0], 16)
        # every label address, to bound a block by the next label after it
        self.alladdr = sorted(set(self.sym.values()))

    def word(self, addr):
        for lo, hi, base in self.map:
            if base <= addr <= base + (hi - lo):
                return self.words[lo + addr - base]
        raise KeyError("address $%04X not in ROM" % addr)

    def block_words(self, name):
        a = self.sym[name]
        nxt = [x for x in self.alladdr if x > a]
        end = nxt[0] if nxt else a
        # stay inside a single mapped segment
        for lo, hi, base in self.map:
            if base <= a <= base + (hi - lo):
                end = min(end, base + (hi - lo) + 1)
        return [self.word(x) for x in range(a, end)]


def card_rows(w4):
    """4 words -> 8 rows of 8 bits (MSB = leftmost pixel)."""
    rows = []
    for w in w4:
        rows.append(w & 0xFF)
        rows.append((w >> 8) & 0xFF)
    return rows


def load(intv):
    """Return {block: [card,...]} where a card is a list of 8 row bytes,
    plus 'TOWN_SCREEN' as 108 raw BACKTAB words."""
    rom = Rom(intv)
    out = {}
    for b in BLOCKS:
        w = rom.block_words(b)
        if b == "TOWN_SCREEN":
            out[b] = w[:108]
        else:
            out[b] = [card_rows(w[i:i + 4]) for i in range(0, len(w) - 3, 4)]
    return out


def gfxbas_cards(intv):
    """Cards per label as written in gen/gfx.bas (for --check)."""
    out, cur, rows = {}, None, []
    for ln in open(os.path.join(intv, "gen", "gfx.bas")):
        m = re.match(r"^([a-z_0-9]+):", ln)
        if m:
            cur = m.group(1).upper()
            out[cur] = []
            continue
        m = re.match(r'\s*BITMAP\s+"([^"]+)"', ln)
        if m and cur:
            rows.append(int("".join("0" if c in "._ " else "1" for c in m.group(1)), 2))
            if len(rows) == 8:
                out[cur].append(rows)
                rows = []
    return out


def sheet(cards, cols=16, scale=4, fg=WHITE):
    from PIL import Image
    n = len(cards)
    rows = (n + cols - 1) // cols
    im = Image.new("RGB", (cols * 9 * scale, max(1, rows) * 9 * scale), (40, 40, 60))
    px = im.load()
    for i, c in enumerate(cards):
        ox, oy = (i % cols) * 9 * scale, (i // cols) * 9 * scale
        for y, r in enumerate(c):
            for x in range(8):
                col = PAL[fg] if (r >> (7 - x)) & 1 else (0, 0, 0)
                for dy in range(scale):
                    for dx in range(scale):
                        px[ox + x * scale + dx, oy + y * scale + dy] = col
    return im


def main():
    ap = argparse.ArgumentParser()
    here = os.path.dirname(os.path.abspath(__file__))
    ap.add_argument("--intv", default=os.path.join(here, "..", "..", "clients", "intv"))
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--preview")
    a = ap.parse_args()
    g = load(a.intv)
    for b in BLOCKS:
        print("%-14s %d %s" % (b, len(g[b]), "words" if b == "TOWN_SCREEN" else "cards"))
    if a.check:
        src = gfxbas_cards(a.intv)
        bad = 0
        for b in BLOCKS:
            if b == "TOWN_SCREEN":
                continue
            s = src.get(b, [])
            if g[b][:len(s)] != s or len(s) == 0:
                print("MISMATCH", b, len(s), len(g[b]))
                bad += 1
        print("check:", "OK" if not bad else "%d mismatches" % bad)
        sys.exit(1 if bad else 0)
    if a.preview:
        os.makedirs(a.preview, exist_ok=True)
        for b in BLOCKS:
            if b != "TOWN_SCREEN":
                sheet(g[b]).save(os.path.join(a.preview, b.lower() + ".png"))


if __name__ == "__main__":
    main()

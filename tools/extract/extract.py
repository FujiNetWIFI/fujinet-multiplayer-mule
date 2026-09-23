#!/usr/bin/env python3
"""Extract graphics for the web client from the Atari 8-bit M.U.L.E. disk image.

usage: extract.py "M.U.L.E. (1983)(Electronic Arts)(US).atr" [outdir]

The graphics belong to their copyright holders. They are NOT part of this
GPLv3 project and are never committed; each user generates them from their
own disk image (CRC32 F76D2896). Only this extraction code is GPL.

Disk layout (found by disassembling the boot loader at $BA92):
  * sectors 1-129   : boot program loaded at $7BEF (title screen, species parade)
  * sectors 400-719 : main game loaded at $1000-$AFFF, entered through DOSVEC $1009

Data used here (addresses in the loaded images):
  * $1800  1 KB text font (ANTIC mode 2/6, CHBASE $18)
  * $1400  town character set (ANTIC mode 4, CHBASE $14 set by the town DLI at $2EB9)
  * $1C00  map character set (ANTIC mode 6 plot icons, CHBASE $1C set by the map DLI at $2D43)
  * $2000  town screen template, 40 x 18 bytes of ANTIC mode 4
  * $4309/$4319/$4255  mountain player shapes (1, 2 and 3 peaks) drawn by the map DLI
  * boot $947C  species animation frames, 8 x 24 player graphics, 24-byte stride

Colours were read from the colour-register writes in the display list
interrupt handlers and checked against atari800 captures (see capture/).
"""
import json
import os
import sys
import zlib

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
EXPECTED_CRC = 0xF76D2896


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


def palette():
    p = open(os.path.join(HERE, "ntsc.pal"), "rb").read()
    return [tuple(p[i * 3:i * 3 + 3]) for i in range(256)]


def glyph_sheet(cs, count, fg=(255, 255, 255, 255)):
    """1 bpp character set -> 16-wide sheet of 8x8 glyphs, transparent background."""
    im = Image.new("RGBA", (16 * 8, (count // 16) * 8), (0, 0, 0, 0))
    for ch in range(count):
        cx, cy = (ch % 16) * 8, (ch // 16) * 8
        for r in range(8):
            b = cs[ch * 8 + r]
            for x in range(8):
                if b & (0x80 >> x):
                    im.putpixel((cx + x, cy + r), fg)
    return im


# Town colour registers per mode-4 row: (COLBK, PF0, PF1, PF2, PF3).
TOWN_TOP = (0x08, 0x24, 0xB4, 0x0C, 0x00)     # rows 1-4: outfitters
TOWN_BOTTOM = (0x08, 0xD4, 0x44, 0x0C, 0x00)  # rows 9-15: assay, land, pub, corral
TOWN_PLAIN = (0x08, 0x24, 0xB4, 0x0C, 0x00)


def render_town(main, pal):
    cs = main[0x1400:0x1800]
    scr = bytearray(main[0x2000:0x2000 + 40 * 18])
    # Clear the corral interior; the client draws its M.U.L.E.s and price.
    for r in range(10, 17):
        for c in range(25, 31):
            scr[r * 40 + c] = 0
    im = Image.new("RGB", (160, 18 * 8))
    for r in range(18):
        regs = TOWN_TOP if 1 <= r <= 4 else TOWN_BOTTOM if 9 <= r <= 15 else TOWN_PLAIN
        for line in range(8):
            for x in range(160):
                c = scr[r * 40 + x // 4]
                v = (cs[(c & 0x7F) * 8 + line] >> (6 - 2 * (x % 4))) & 3
                reg = 4 if (v == 3 and c & 0x80) else v
                im.putpixel((x, r * 8 + line), pal[regs[reg]])
    # Mode 4 pixels are 2 colour clocks wide: 160 -> 320 wide, keep 8 lines per row.
    return im.resize((320, 144), Image.NEAREST).crop((64, 0, 256, 144))


def render_mountains(main, pal):
    # small (3 lines), medium (4), large (6, two variants)
    order = [main[0x4313:0x4316], main[0x4255:0x4259], main[0x4309:0x430F], main[0x4319:0x431F]]
    im = Image.new("RGBA", (8 * len(order), 6), (0, 0, 0, 0))
    col = pal[0x04] + (255,)
    for k, sh in enumerate(order):
        top = 6 - len(sh)
        for y, b in enumerate(sh):
            for x in range(8):
                if b & (0x80 >> x):
                    im.putpixel((k * 8 + x, top + y), col)
    return im


# Species -> animation frame indices in the boot parade table (11 frames each),
# matched against the species-selection screen.
SPECIES_FRAMES = {
    "mechtron": list(range(12, 23)),
    "gollumer": list(range(45, 56)),
    "packer": list(range(34, 45)),
    "bonzoid": list(range(67, 78)),
    "spheroid": list(range(23, 34)),
    "flapper": list(range(56, 67)),
    "leggite": list(range(89, 100)),
    "humanoid": list(range(1, 12)),
}


def render_species(boot):
    base, n = 0x947C, 100
    im = Image.new("RGBA", (n * 8, 22), (0, 0, 0, 0))
    for k in range(n):
        d = boot[base + k * 24 + 2:base + k * 24 + 24]
        for y, b in enumerate(d):
            for x in range(8):
                if b & (0x80 >> x):
                    im.putpixel((k * 8 + x, y), (255, 255, 255, 255))
    return im


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    out = sys.argv[2] if len(sys.argv) > 2 else os.path.join(HERE, "..", "..", "web", "assets")
    os.makedirs(out, exist_ok=True)
    mainmem, boot = load_disk(sys.argv[1])
    pal = palette()

    glyph_sheet(mainmem[0x1800:0x1C00], 128).save(os.path.join(out, "font.png"))
    glyph_sheet(mainmem[0x1C00:0x2000], 64).save(os.path.join(out, "mapchars.png"))
    render_town(mainmem, pal).save(os.path.join(out, "town.png"))
    render_mountains(mainmem, pal).save(os.path.join(out, "mountains.png"))
    render_species(boot).save(os.path.join(out, "species.png"))

    hexpal = ["#%02x%02x%02x" % c for c in pal]
    manifest = {
        "source": "M.U.L.E. (1983)(Electronic Arts)(US).atr",
        "palette": hexpal,
        "colors": {  # Atari colour values used by the game
            "mapBackground": 0x08, "river": 0xF4, "mountain": 0x04, "text": 0x0C,
            "players": [0x84, 0x54, 0xC4, 0x34], "cursor": 0x0E, "town": 0x08,
            "goods": [0xC6, 0x1A, 0x06, 0x86],
        },
        "font": {"file": "font.png", "glyph": 8, "columns": 16, "encoding": "atari-screen-code"},
        "mapchars": {"file": "mapchars.png", "glyph": 8, "columns": 16,
                     "note": "a plot is 2x4 chars: left column = border, right column = top border, icon (2 chars), bottom border",
                     "left": [0x1D, 0x1E, 0x1E, 0x1F], "rightTop": 0x20, "rightBottom": 0x22,
                     "icons": {"house": [0x11, 0x17], "crystite": [0x12, 0x18], "smithore": [0x13, 0x19],
                               "energy": [0x14, 0x1A], "food": [0x15, 0x1B], "mule": [0x16, 0x1C]}},
        "town": {"file": "town.png", "width": 192, "height": 144,
                 "note": "40x18 mode-4 template at $2000 cropped to the town box; 8x8 cells after 2x horizontal scaling",
                 "buildings": {  # cell rectangles [x, y, w, h] in 8x8 cells of town.png
                     "food": [1, 1, 4, 4], "energy": [7, 1, 4, 4], "smithore": [13, 1, 4, 4], "crystite": [19, 1, 4, 4],
                     "assay": [1, 9, 4, 7], "land": [6, 9, 4, 6], "pub": [11, 9, 4, 5], "corral": [17, 9, 6, 9]}},
        "mountains": {"file": "mountains.png", "frame": [8, 6], "sizes": ["1", "2", "3a", "3b"]},
        "species": {"file": "species.png", "frame": [8, 22], "frames": SPECIES_FRAMES},
    }
    with open(os.path.join(out, "manifest.json"), "w") as f:
        json.dump(manifest, f, indent=1)
    print("wrote assets to", os.path.abspath(out))


if __name__ == "__main__":
    main()

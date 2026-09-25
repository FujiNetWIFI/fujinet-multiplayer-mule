#!/usr/bin/env python3
"""tmsshot.py VRAM.bin OUT.png -- draw a TMS9918A's Graphics II screen (the
layout src/tms9918/gfx.c sets up: patterns 0000h, names 1800h, sprite
attributes 1B00h, colours 2000h, sprite patterns 3800h, 16 x 16 sprites)
from a 16K VRAM dump, as tools/msx/msxsim saves them."""
import sys

PAL = [(0, 0, 0), (0, 0, 0), (33, 200, 66), (94, 220, 120), (84, 85, 237), (125, 118, 252),
       (212, 82, 77), (66, 235, 245), (252, 85, 84), (255, 121, 120), (212, 193, 84),
       (230, 206, 128), (33, 176, 59), (201, 91, 186), (204, 204, 204), (255, 255, 255)]


def render(v, backdrop=1):
    px = [[PAL[backdrop]] * 256 for _ in range(192)]
    for y in range(192):
        third = (y >> 6) * 0x800
        for cx in range(32):
            name = v[0x1800 + (y >> 3) * 32 + cx]
            a = third + name * 8 + (y & 7)
            p, c = v[a], v[0x2000 + a]
            for k in range(8):
                col = c >> 4 if p & (0x80 >> k) else c & 15
                if col:
                    px[y][cx * 8 + k] = PAL[col]
    sprites = []
    for i in range(32):
        y, x, n, c = v[0x1B00 + i * 4:0x1B00 + i * 4 + 4]
        if y == 0xD0:
            break
        sprites.append((y, x, n, c))
    for (y, x, n, c) in reversed(sprites):      # sprite 0 on top
        sy = y + 1 if y < 0xE1 else y - 255
        sx = x - 32 if c & 0x80 else x
        if not c & 15:
            continue
        base = 0x3800 + (n & 0xFC) * 8
        for r in range(16):
            for half in range(2):
                b = v[base + half * 16 + r]
                for k in range(8):
                    if b & (0x80 >> k):
                        xx, yy = sx + half * 8 + k, sy + r
                        if 0 <= xx < 256 and 0 <= yy < 192:
                            px[yy][xx] = PAL[c & 15]
    return px


def main():
    from PIL import Image
    v = open(sys.argv[1], "rb").read()
    px = render(v)
    im = Image.new("RGB", (256, 192))
    im.putdata([p for r in px for p in r])
    im.resize((512, 384), Image.NEAREST).save(sys.argv[2])


if __name__ == "__main__":
    main()

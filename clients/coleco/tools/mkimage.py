#!/usr/bin/env python3
"""mkimage.py RAW LST OUT -- the cartridge image from zmac's output.

zmac wants its ORGs in order, so mule.asm assembles the pages n >= 1 at
0000H.. (each PHASEd to C000H) and the 32K window at 8000H-FFFFH. The
image (APPBANK, fujinet-firmware pico/coleco) is the window, then page 1,
page 2, ...: 32K + (NPAGES - 1) * 8K. A 64K or 128K image would look like a
MegaCart to firmware without APPBANK, so a blank page is added to those.

Checks: the 55 AA header, the mailbox (F800H up, image 7800H-7FFFH) clear
but for the "FUJI" claim this stamps at 7CFCH, and the pages' contents
(nothing assembled past a page's end into the next).
"""
import re
import sys

WINDOW = 0x8000
PAGE = 0x2000
ROM_TOP = 0x7800
CLAIM_OFF = 0x7CFC
CLAIM = b"FUJI"


def symbols(lst):
    text = open(lst, encoding="utf-8", errors="replace").read()
    tail = text[text.rfind("Symbol Table:"):]
    syms = {}
    for name, val in re.findall(r"\b([a-z_][a-z0-9_]*)\s+=?\s*([0-9a-f]{1,5})\+?", tail):
        syms.setdefault(name.upper(), int(val, 16))
    return syms


def main():
    raw_path, lst, out = sys.argv[1:4]
    raw = open(raw_path, "rb").read()
    if len(raw) != 0x10000:
        sys.exit("mkimage: the raw output is %d bytes, not 64K" % len(raw))
    syms = symbols(lst)
    npages = syms["NPAGES"]
    window = bytearray(raw[WINDOW:])
    if window[0:2] != b"\x55\xAA":
        sys.exit("mkimage: no 55 AA header")
    bad = [o for o in range(ROM_TOP, WINDOW) if window[o] != 0]
    if bad:
        sys.exit("mkimage: the mailbox pages hold data at %s" % ", ".join("%04XH" % (o + 0x8000) for o in bad[:8]))
    window[CLAIM_OFF:CLAIM_OFF + 4] = CLAIM
    image = bytes(window)
    for n in range(1, npages):
        image += raw[(n - 1) * PAGE:n * PAGE]
    if (n := len(image)) in (0x10000, 0x20000):
        image += bytes(PAGE)
    if len(image) > 0x20000:
        sys.exit("mkimage: %d bytes, over the cart's 128K" % len(image))
    open(out, "wb").write(image)
    print("mkimage: %s, %d bytes (%d pages)" % (out, len(image), (len(image) - WINDOW) // PAGE + 1))


if __name__ == "__main__":
    main()

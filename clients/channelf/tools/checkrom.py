#!/usr/bin/env python3
"""checkrom.py -- the cartridge image's layout (cart protocol 2, APPBANK).

The image is the fixed 16K (served at $0800-$47FF) and whole 8K pages after
it (one at a time at $4800). Checked:
  - 16K + 1..6 whole pages (the cart refuses anything else: TOOBIG);
  - $55 at $0800 (else the BIOS runs its Hockey) and a JMP at $0802;
  - "FUJI" at $47FC (else the mailbox dies at boot, and with it the pages:
    the cart refuses a banked image without it);
  - each page starts with its id and the id complemented (mule.asm PAGEHDR),
    which a DEBUG build checks after every page select.

usage: checkrom.py build/mule.bin
"""
import sys

FIXED, PAGE, MAX_PAGES = 0x4000, 0x2000, 6


def main():
    path = sys.argv[1]
    img = open(path, "rb").read()
    errs = []
    extra = len(img) - FIXED
    if extra <= 0 or extra % PAGE or extra // PAGE > MAX_PAGES:
        errs.append("%d bytes: not 16K + 1..%d whole 8K pages" % (len(img), MAX_PAGES))
    if img[0] != 0x55:
        errs.append("$%02X at $0800, not the $55 the BIOS looks for" % img[0])
    if img[2] != 0x29:
        errs.append("no JMP at $0802 (found $%02X)" % img[2])
    if img[FIXED - 4:FIXED] != b"FUJI":
        errs.append("no FUJI claim at $47FC (found %r)" % img[FIXED - 4:FIXED])
    npages = max(0, extra // PAGE)
    for n in range(npages):
        a = FIXED + n * PAGE
        if img[a] != n or img[a + 1] != (~n) & 0xFF:
            errs.append("page %d starts %02X %02X, not its id" % (n, img[a], img[a + 1]))
    for e in errs:
        print("checkrom: %s: %s" % (path, e), file=sys.stderr)
    if errs:
        return 1
    print("checkrom: %s: 16K + %d pages, signature, entry, claim and page ids ok"
          % (path, npages))
    return 0


sys.exit(main())

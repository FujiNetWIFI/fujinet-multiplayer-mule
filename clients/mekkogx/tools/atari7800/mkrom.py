#!/usr/bin/env python3
"""mkrom.py -- the 7800 M.U.L.E. image from ld65's: REPL copied into every
switched bank, checked, with the .a78 header.

    tools/atari7800/mkrom.py MAP IMAGE.bin OUT.a78

The image is eight 16K banks (src/atari7800/mule-a78.cfg): banks 0-6 are
$8000-$9FFF REPL and $A000-$BFFF the bank's own, bank 7 is $C000-$FFFF.
ld65 links REPL into bank 0 only; here banks 1-6 get the same bytes, so
REPL's code runs (and MARIA could read it) whichever bank is in.

Then, as fujinet-firmware's checkrom (the 7800 handbook's checkbanks.py)
does, over every code segment in its own bank:
  - no read-modify-write aimed at the mailbox's write pages ($0D00-$0FFF):
    the dummy write is an event of its own;
  - no absolute or zero-page store to the TIA ($00-$1F and its mirrors):
    each rewrites INPTCTRL, which the cart keeps unlocked;
  - no JMP ($xxFF);
  - no store into the bank window but bank.c's to $8000 (a stray one would
    switch banks under the code);
and "FUJI", version 1, kind 5 (SuperGame + RAM), RAM at $FF70; the reset
vector in the fixed bank. A bad image is not written.
"""
import re
import sys

BANK = 0x4000
NBANKS = 8
WR_LO, WR_HI = 0x0D00, 0x0FFF

RMW = {0x0E, 0x1E, 0x2E, 0x3E, 0x4E, 0x5E, 0x6E, 0x7E, 0xCE, 0xDE, 0xEE, 0xFE}
ZP_WRITES = {0x85, 0x86, 0x84, 0x06, 0x26, 0x46, 0x66, 0xC6, 0xE6}
ABS_WRITES = {0x8D, 0x8E, 0x8C, 0x0E, 0x2E, 0x4E, 0x6E, 0xCE, 0xEE, 0x9D, 0x99}
LEN = [1] * 256
for op in (0x69, 0x29, 0xC9, 0xE0, 0xC0, 0x49, 0xA9, 0xA2, 0xA0, 0x09, 0xE9,
           0xA5, 0xA6, 0xA4, 0x85, 0x86, 0x84, 0x65, 0x25, 0x06, 0x24, 0xC5,
           0xC6, 0x45, 0xE6, 0x46, 0x26, 0x66, 0xE5, 0x05, 0x75, 0x35, 0x16,
           0xD5, 0xD6, 0x55, 0xF6, 0x56, 0x36, 0x76, 0xF5, 0x15, 0xB5, 0xB4,
           0x95, 0x94, 0xB6, 0x96, 0x61, 0x21, 0xC1, 0x41, 0xA1, 0x01, 0xE1,
           0x81, 0x71, 0x31, 0xD1, 0x51, 0xB1, 0x11, 0xF1, 0x91, 0xC4, 0xE4,
           0x10, 0x30, 0x50, 0x70, 0x90, 0xB0, 0xD0, 0xF0):
    LEN[op] = 2
for op in (0x6D, 0x2D, 0x0E, 0x2C, 0xCD, 0xEC, 0xCC, 0xCE, 0x4D, 0xEE, 0x4C,
           0x20, 0xAD, 0xAE, 0xAC, 0x4E, 0x0D, 0x2E, 0x6E, 0xED, 0x8D, 0x8E,
           0x8C, 0x7D, 0x3D, 0x1E, 0xDD, 0xDE, 0x5D, 0xFD, 0xFE, 0x5E, 0xBD,
           0xBC, 0x3E, 0x7E, 0x1D, 0x9D, 0x79, 0x39, 0xD9, 0x59, 0xB9, 0xBE,
           0x19, 0xF9, 0x99, 0x6C):
    LEN[op] = 3

# code segments and where they live: (bank, or None = the fixed bank)
CODE_SEGS = {"STARTUP": None, "LOWCODE": None, "ONCE": None, "CODE": None, "REPL": 0,
             "OVERLAY1": 0, "OVERLAY2": 1, "OVERLAY3": 2, "OVERLAY4": 3, "OVERLAY5": 4}


def in_tia(a):
    return (a & 0xFCE0) == 0


def scan(code, base, label, bank_ok):
    bad = []
    pc = 0
    while pc < len(code):
        op = code[pc]
        n = LEN[op]
        if pc + n > len(code):
            break
        if n == 3:
            tgt = code[pc + 1] | (code[pc + 2] << 8)
            if op in RMW and WR_LO <= tgt <= WR_HI:
                bad.append("%s $%04X: a read-modify-write of the mailbox at $%04X" % (label, base + pc, tgt))
            if op in ABS_WRITES and op not in (0x9D, 0x99) and in_tia(tgt):
                bad.append("%s $%04X: a store to $%04X rewrites INPTCTRL" % (label, base + pc, tgt))
            if op in ABS_WRITES and 0x8000 <= tgt <= 0xBFFF and not (tgt == 0x8000 and bank_ok(base + pc)):
                bad.append("%s $%04X: a store to $%04X switches banks" % (label, base + pc, tgt))
            if op == 0x6C and (tgt & 0xFF) == 0xFF:
                bad.append("%s $%04X: JMP ($%04X) hits the page-wrap bug" % (label, base + pc, tgt))
        elif n == 2 and op in ZP_WRITES and code[pc + 1] < 0x20:
            bad.append("%s $%04X: a store to $%02X rewrites INPTCTRL" % (label, base + pc, code[pc + 1]))
        pc += n
    return bad


def main():
    mapfile, path, out = sys.argv[1:4]
    img = bytearray(open(path, "rb").read())
    if len(img) != NBANKS * BANK:
        sys.exit("mkrom: %s is %d bytes, not %d" % (path, len(img), NBANKS * BANK))
    text = open(mapfile).read()
    seglist = re.search(r"Segment list:\s*\n-+\s*\n(.*?)\n\n", text, re.S).group(1)
    segs = {}
    for line in seglist.splitlines():
        p = line.split()
        if len(p) >= 4 and re.fullmatch(r"[0-9A-Fa-f]{6}", p[1]):
            segs[p[0]] = (int(p[1], 16), int(p[2], 16))
    # bank.c's code: the one place a store to $8000 belongs
    mods = re.search(r"Modules list:\s*\n-+\s*\n(.*?)\n\n", text, re.S).group(1)
    bank_rng = None
    cur = None
    for line in mods.splitlines():
        if not line.startswith(" "):
            cur = line.strip()
            continue
        m = re.match(r"\s+(\w+)\s+Offs=([0-9A-Fa-f]+)\s+Size=([0-9A-Fa-f]+)", line)
        if m and cur and (cur == "bank.o:" or cur.endswith("/bank.o:")) and m.group(1) == "CODE":
            start = segs["CODE"][0] + int(m.group(2), 16)
            bank_rng = (start, start + int(m.group(3), 16))
    if not bank_rng:
        sys.exit("mkrom: no bank.o CODE in %s" % mapfile)
    bank_ok = lambda a: bank_rng[0] <= a < bank_rng[1]

    # REPL into banks 1-6
    repl = img[0:0x2000]
    for b in range(1, 7):
        img[b * BANK:b * BANK + 0x2000] = repl

    bad = []
    for name, (s, e) in segs.items():
        if name not in CODE_SEGS:
            continue
        bank = CODE_SEGS[name]
        if bank is None:
            off = 7 * BANK + (s - 0xC000)
        else:
            off = bank * BANK + (s - 0x8000)
        bad += scan(img[off:off + e + 1 - s], s, "%s $%04X-$%04X" % (name, s, e), bank_ok)
    top = img[7 * BANK:]
    mem = lambda a: top[a - 0xC000]
    if bytes(mem(0xFF70 + i) for i in range(4)) != b"FUJI":
        bad.append("no \"FUJI\" claim at $FF70")
    if (mem(0xFF74), mem(0xFF75), mem(0xFF76)) != (1, 5, 1):
        bad.append("the claim is not version 1, kind 5 (SuperGame + RAM), RAM")
    vec = mem(0xFFFC) | (mem(0xFFFD) << 8)
    if vec < 0xC000:
        bad.append("reset vector $%04X is not in the fixed bank" % vec)
    for b in bad:
        print("mkrom: %s: %s" % (path, b), file=sys.stderr)
    if bad:
        sys.exit(1)
    # the usage of each bank's part
    for name in ("REPL", "OVERLAY1", "OVERLAY2", "OVERLAY3", "OVERLAY4", "OVERLAY5", "ART", "STRS"):
        if name in segs:
            s, e = segs[name]
            print("mkrom: %-8s $%04X-$%04X %5d bytes" % (name, s, e, e + 1 - s))
    h = bytearray(128)
    h[0] = 1
    h[1:10] = b"ATARI7800"
    title = b"M.U.L.E."
    h[17:17 + len(title)] = title
    h[49:53] = len(img).to_bytes(4, "big")
    h[53:55] = (0x0006).to_bytes(2, "big")      # SuperGame, RAM at $4000
    h[55] = 1
    h[56] = 1
    h[57] = 0
    h[100:128] = b"ACTUAL CART DATA STARTS HERE"
    open(out, "wb").write(bytes(h) + bytes(img))


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""mkrom.py -- the 5200 M.U.L.E. image from ld65's: REPL copied into every
bank, checked.

    tools/atari5200/mkrom.py MAP IMAGE.bin OUT.a52

The image is four 32K Super Cart banks (src/atari5200/mule-a52.cfg), each
of them the whole cart window $4000-$BFFF. ld65 links REPL (STARTUP up to
$AFFF, and $B800-$BFFF with the claim and the header) into bank 3, the one
the console starts in; here banks 0-2 get the same bytes, so REPL's code
runs whichever bank is in.

Then, as fujinet-lib's atari5200-romstamp.py does, over every code segment
in its own bank:
  - no store and no read-modify-write into the cart window ($4000-$BFFF):
    the edge has no R/W;
  - no indexed read of a hotspot page ($B500-$B7FF) from a base that is not
    page-aligned (a page crossing reads the wrong page first), and none at
    all but the lib's fn_tx / fn_regwr / hand-over (fuji_atari5200_go.o);
  - no read of the bank switch ($BFC0-$BFFF) but bank.o's and the crt0's;
  - no JMP ($xxFF);
and the mailbox and the switch's reads $FF, nothing in the 4 bytes below
$BFC0 (an RTS's dummy read of the next byte), "FUJI" version 1 kind 4 (a
Super Cart) at $BFE0, $02 at $BFE7, $FF at $BFFD, the entry in REPL. A bad
image is not written.
"""
import re
import sys

WIN = 0x4000
BANK = 0x8000
NBANKS = 4
HOT_LO, HOT_HI = 0xB500, 0xB7FF
SW_LO = 0xBFC0

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

STORES = {0x8D, 0x8E, 0x8C, 0x9D, 0x99}
RMW = {0x0E, 0x1E, 0x2E, 0x3E, 0x4E, 0x5E, 0x6E, 0x7E, 0xCE, 0xDE, 0xEE, 0xFE}
IDX_READS = {0x7D, 0x3D, 0xDD, 0x5D, 0xFD, 0xBD, 0xBC, 0x1D,
             0x79, 0x39, 0xD9, 0x59, 0xB9, 0xBE, 0x19, 0xF9}
JMPS = {0x4C, 0x20, 0x6C}

# code segments and their bank (REPL: linked into bank 3)
CODE_SEGS = {"STARTUP": 3, "LOWCODE": 3, "ONCE": 3, "CODE": 3, "HICODE": 3,
             "OVERLAY1": 3, "OVERLAY2": 2, "OVERLAY3": 0, "OVERLAY4": 1, "OVERLAY5": 0}
# who may read the hotspots, and the bank switch
HOT_MODS = ("fuji_atari5200_go.o",)
SW_MODS = ("bank.o", "crt0.o")


def section(text, title):
    return re.search(title + r":\s*\n-+\s*\n(.*?)\n\n", text, re.S).group(1)


def main():
    mapfile, path, out = sys.argv[1:4]
    img = bytearray(open(path, "rb").read())
    if len(img) != NBANKS * BANK:
        sys.exit("mkrom: %s is %d bytes, not %d" % (path, len(img), NBANKS * BANK))
    text = open(mapfile).read()
    segs = {}
    for line in section(text, "Segment list").splitlines():
        p = line.split()
        if len(p) >= 4 and re.fullmatch(r"[0-9A-Fa-f]{6}", p[1]):
            segs[p[0]] = (int(p[1], 16), int(p[2], 16))
    # each module's piece of each code segment: (start, end, module)
    pieces = []
    cur = None
    for line in section(text, "Modules list").splitlines():
        if not line.startswith(" "):
            cur = line.strip().rstrip(":")
            continue
        m = re.match(r"\s+(\w+)\s+Offs=([0-9A-Fa-f]+)\s+Size=([0-9A-Fa-f]+)", line)
        if m and m.group(1) in CODE_SEGS and m.group(1) in segs:
            s = segs[m.group(1)][0] + int(m.group(2), 16)
            pieces.append((s, s + int(m.group(3), 16), cur))

    def module_at(a):
        for s, e, mod in pieces:
            if s <= a < e:
                return mod
        return "?"

    def modname(mod):       # "dir/x.o" or "dir/lib.lib(x.o)" -> "x.o"
        return re.sub(r".*[/(]", "", mod).rstrip(")")

    # REPL into banks 0-2: STARTUP to $AFFF, $B800 to the header's end
    lo = segs["STARTUP"][0]
    src = 3 * BANK
    for b in range(3):
        for a, e in ((lo, 0xB000), (0xB800, 0xC000)):
            img[b * BANK + a - WIN:b * BANK + e - WIN] = img[src + a - WIN:src + e - WIN]

    bad = []
    for name, (s, e) in segs.items():
        if name not in CODE_SEGS:
            continue
        bank = CODE_SEGS[name]
        base = bank * BANK + s - WIN
        code = img[base:base + e + 1 - s]
        pc = 0
        while pc < len(code):
            op = code[pc]
            n = LEN[op]
            if pc + n > len(code):
                break
            if n == 3 and op not in JMPS:
                at = s + pc
                tgt = code[pc + 1] | (code[pc + 2] << 8)
                here = "%s $%04X (%s)" % (name, at, module_at(at))
                if (op in STORES or op in RMW) and WIN <= tgt <= 0xBFFF:
                    bad.append("%s: a store to $%04X: the cart window" % (here, tgt))
                elif HOT_LO <= tgt <= HOT_HI or (op in IDX_READS and HOT_LO <= tgt + 255 and tgt <= HOT_HI):
                    if modname(module_at(at)) not in HOT_MODS:
                        bad.append("%s: a read of the mailbox's hotspots ($%04X)" % (here, tgt))
                    elif op in IDX_READS and tgt & 0xFF:
                        bad.append("%s: an indexed read of $%04X crosses a hotspot page" % (here, tgt))
                elif SW_LO <= tgt <= 0xBFFF or (op in IDX_READS and tgt + 255 >= SW_LO and tgt <= 0xBFFF):
                    if modname(module_at(at)) not in SW_MODS:
                        bad.append("%s: a read of $%04X switches banks" % (here, tgt))
            if op == 0x6C and (code[pc + 1] == 0xFF):
                bad.append("%s $%04X: JMP ($%02X%02X) hits the page-wrap bug" % (name, s + pc, code[pc + 2], code[pc + 1]))
            pc += n

    for b in range(NBANKS):
        bank = img[b * BANK:(b + 1) * BANK]
        mem = lambda a: bank[a - WIN]
        if any(mem(a) != 0xFF for a in range(0xB000, 0xB800)):
            bad.append("bank %d: the mailbox's $B000-$B7FF is not $FF" % b)
        if any(mem(a) != 0xFF for a in range(0xBFBC, 0xBFE0)):
            bad.append("bank %d: $BFBC-$BFDF is not $FF (the bank switch, and the 4 bytes below it)" % b)
        if bytes(mem(0xBFE0 + i) for i in range(4)) != b"FUJI":
            bad.append("bank %d: no \"FUJI\" claim at $BFE0" % b)
        if (mem(0xBFE4), mem(0xBFE5)) != (1, 4):
            bad.append("bank %d: the claim is not version 1, kind 4 (a Super Cart)" % b)
        if mem(0xBFE7) != 0x02:
            bad.append("bank %d: $BFE7 is not $02 (the 2-port BIOS's PAL check)" % b)
        if mem(0xBFFD) != 0xFF:
            bad.append("bank %d: $BFFD is not $FF (the BIOS's title)" % b)
        vec = mem(0xBFFE) | (mem(0xBFFF) << 8)
        if not lo <= vec < 0xB000:
            bad.append("bank %d: entry $%04X is not in REPL" % (b, vec))
        if b < 3 and (bank[lo - WIN:0xB000 - WIN] != img[3 * BANK + lo - WIN:3 * BANK + 0xB000 - WIN]):
            bad.append("bank %d: REPL differs from bank 3's" % b)
    for x in bad:
        print("mkrom: %s: %s" % (path, x), file=sys.stderr)
    if bad:
        sys.exit(1)
    # how full: REPL, and each bank's own part ($4000 up to REPL)
    rl = sum(e + 1 - s for n, (s, e) in segs.items() if lo <= s < 0xB000)
    rh = sum(e + 1 - s for n, (s, e) in segs.items() if 0xB800 <= s < 0xBFBC)
    print("mkrom: REPL     $%04X-$AFFF %5d of %5d, $B800-$BFBB %4d of %4d" % (lo, rl, 0xB000 - lo, rh, 0xBFBC - 0xB800))
    # (up to its last byte that isn't fill: the load/run segments' load
    # addresses aren't in the map)
    for b in range(NBANKS):
        own = img[b * BANK:b * BANK + lo - WIN]
        used = len(own.rstrip(b"\xff"))
        print("mkrom: bank %d   own $4000-$%04X %5d of %5d" % (b, WIN + used - 1, used, lo - WIN))
    open(out, "wb").write(bytes(img))


if __name__ == "__main__":
    main()

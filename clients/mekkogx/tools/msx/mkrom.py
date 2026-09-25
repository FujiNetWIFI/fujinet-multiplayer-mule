#!/usr/bin/env python3
"""Make the MSX's ROM, r2r/msxrom/mule.rom: an ASCII8 megaROM (8K banks),
from what z88dk linked (src/msx/mule.asm has the map).

usage: mkrom.py DIR   (DIR/mule.rom as z88dk made it, mule.map, mule_*.bin)

  banks 0-2   4000h-9FFFh, fixed: the program (z88dk's rom from 4000h)
  banks 3-7   the phases' overlays OV1..OV5, at A000h
  bank 8      the art (ART), bank 9 the strings (STR), at A000h

The last four bytes of every window bank (BFFCh-BFFFh) are the FujiNet's
registers when it runs the ROM; here they are 0, which is what the game
reads as "no FujiNet" when the ROM runs anywhere else (src/msx/plat.c).
The ROM is padded to 128K. A copy is written as mule[ASCII8].rom: the
FujiNet firmware tells a ROM's mapper by that end of its name.
"""
import os
import re
import sys

BANK = 0x2000
WIN = BANK - 4
BANKS = 16


def symbols(path):
    out = {}
    for line in open(path):
        m = re.match(r"(\S+)\s+=\s+\$([0-9A-Fa-f]+)", line)
        if m:
            out[m.group(1)] = int(m.group(2), 16)
    return out


def main():
    d = sys.argv[1]
    rom = open(os.path.join(d, "mule.rom"), "rb").read()
    sym = symbols(os.path.join(d, "mule.map"))
    end = sym["__ROMABLE_END_tail"] + (sym["__DATA_END_tail"] - sym["__DATA_head"])
    errors = []
    if end > 0xA000:
        errors.append("the program ends at %04Xh, in the window (A000h)" % end)
    if sym["__BSS_END_tail"] > 0xE000:
        errors.append("the variables end at %04Xh: little room for the stack" % sym["__BSS_END_tail"])
    out = bytearray(rom[:3 * BANK].ljust(3 * BANK, b"\xff"))
    sizes = []
    for name in ["OV1", "OV2", "OV3", "OV4", "OV5", "ART", "STR"]:
        data = open(os.path.join(d, "mule_%s.bin" % name), "rb").read()
        if len(data) > WIN:
            errors.append("%s is %d bytes: over the window's %d" % (name, len(data), WIN))
        out += data[:WIN].ljust(WIN, b"\xff") + bytes(4)
        sizes.append("%s %d" % (name, len(data)))
    while len(out) < BANKS * BANK:
        out += b"\xff" * WIN + bytes(4)
    if errors:
        for e in errors:
            print("mkrom.py: " + e, file=sys.stderr)
        sys.exit(1)
    open(os.path.join(d, "mule.rom"), "wb").write(out)
    open(os.path.join(d, "mule[ASCII8].rom"), "wb").write(out)
    print("msxrom: program 4000-%04X, variables C000-%04X; %s" %
          (end - 1, sym["__BSS_END_tail"] - 1, ", ".join(sizes)))


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Make the Adam's boot media: r2r/adam/mule.ddp (a tape, 256K) and
r2r/adam/mule.dsk (a disk, 160K), from what z88dk linked.

usage: mkimage.py DIR   (DIR/mule.ddp, mule.map, mule_OV1..5.bin)

z88dk's .ddp has its bootstrap in block 0 and the program in blocks 1..N
(the bootstrap reads them to 0000h). The overlays follow, each from a
block of its own, and the program's ovl_table (src/adam/bank.c, found by
its "OVLT") says where. A disk is the same blocks, 1K each.

Checks the memory map (src/adam/mule.asm): the program must end below
C800h (the bootstrap), the variables below the overlay window, and each
overlay fit the window.
"""
import os
import re
import sys

BLOCK = 1024
DSK_BLOCKS = 160
BOOTSTRAP = 0xC800
STACK_TOP = 0xD390
STACK_MIN = 0x400


def symbols(path):
    out = {}
    for line in open(path):
        m = re.match(r"(\S+)\s+=\s+\$([0-9A-Fa-f]+)", line)
        if m:
            out[m.group(1)] = int(m.group(2), 16)
    return out


def main():
    d = sys.argv[1]
    ddp = bytearray(open(os.path.join(d, "mule.ddp"), "rb").read())
    sym = symbols(os.path.join(d, "mule.map"))
    end = sym["__DATA_END_tail"]
    bss_end = sym["__BSS_END_tail"]
    win = sym["_ovl_window"]
    win_size = sym.get("OVL_SIZE", 0x1800)
    errors = []
    if end > BOOTSTRAP:
        errors.append("the program ends at %04Xh, over the bootstrap (C800h)" % end)
    if bss_end > win:
        errors.append("the variables end at %04Xh, over the overlay window (%04Xh)" % (bss_end, win))
    if win + win_size > STACK_TOP - STACK_MIN:
        errors.append("the overlay window leaves under 1K of stack")

    first = end // BLOCK + 2          # blocks 1..N hold 0000h..end
    table = []
    blk = first
    sizes = []
    for n in range(1, 6):
        data = open(os.path.join(d, "mule_OV%d.bin" % n), "rb").read()
        if len(data) > win_size:
            errors.append("overlay %d is %d bytes: over the window's %d" % (n, len(data), win_size))
        count = (len(data) + BLOCK - 1) // BLOCK
        need = (blk + count) * BLOCK
        if len(ddp) < need:
            errors.append("the tape is full")
            break
        ddp[blk * BLOCK:blk * BLOCK + len(data)] = data
        ddp[blk * BLOCK + len(data):(blk + count) * BLOCK] = bytes(count * BLOCK - len(data))
        table += [blk & 255, blk >> 8, count]
        sizes.append(len(data))
        blk += count
    at = ddp.find(b"OVLT", BLOCK)
    if at < 0:
        errors.append("no OVLT table in the program")
    else:
        ddp[at + 4:at + 4 + len(table)] = bytes(table)
    if blk > DSK_BLOCKS:
        errors.append("%d blocks: over a disk's %d" % (blk, DSK_BLOCKS))
    if errors:
        for e in errors:
            print("mkimage.py: " + e, file=sys.stderr)
        sys.exit(1)
    open(os.path.join(d, "mule.ddp"), "wb").write(ddp)
    open(os.path.join(d, "mule.dsk"), "wb").write(ddp[:DSK_BLOCKS * BLOCK])
    print("adam: program 0000-%04X, variables to %04X, window %04X-%04X, stack %04X-%04X"
          % (end - 1, bss_end - 1, win, win + win_size - 1, win + win_size, STACK_TOP - 1))
    print("adam: overlays %s bytes, blocks %d..%d of %d" %
          (" ".join(str(s) for s in sizes), first, blk - 1, DSK_BLOCKS))


if __name__ == "__main__":
    main()

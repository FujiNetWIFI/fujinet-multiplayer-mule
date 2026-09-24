#!/usr/bin/env python3
"""lwlink-mule.py -- lwlink for CMOC (--lwlink=), with M.U.L.E.'s memory map.

CMOC links with its own script; this swaps in src/coco/mule.link (the
memory map), runs the real lwlink, checks every region's limit, and splits
the DECB output into what the CoCo loads:

  mule.bin      low core (< $8000): LOADM'd by BASIC, has the exec address
  mulehi.bin    high core ($8000-$BFEF): loaded by the game itself
                (src/coco/bank.c) in all-RAM mode
  mule1..5.bin  the overlays, all linked at the window ($E000)

All are DECB .bin files (segments of 00 len addr data, then FF 0000 exec).
"""
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SCRIPT = os.path.join(HERE, "..", "..", "src", "coco", "mule.link")

# Regions: (name, first, end) -- a section starting in one must end in it.
LOW_END = 0x7C00        # the stack is the 1K below $8000 (--initial-s)
REGIONS = [
    ("low",    0x2600, LOW_END),
    ("high",   0x8000, 0xBFF0),     # the CPU vectors are at $BFF0 (SAM)
    ("window", 0xE000, 0xFA00),     # the sprites' save-unders above (gfx.c)
]
WINDOW = 0xE000
OVERLAYS = 5


def region_of(addr):
    for name, a, b in REGIONS:
        if a <= addr < b:
            return name, a, b
    return None, 0, 0


def read_decb(path):
    data = open(path, "rb").read()
    blocks, i, exe = [], 0, None
    while i < len(data):
        kind = data[i]
        n = (data[i + 1] << 8) | data[i + 2]
        a = (data[i + 3] << 8) | data[i + 4]
        if kind == 0xFF:
            exe = a
            break
        blocks.append((a, data[i + 5:i + 5 + n]))
        i += 5 + n
    return blocks, exe


def write_decb(path, blocks, exe):
    with open(path, "wb") as f:
        for a, b in blocks:
            for k in range(0, len(b), 0xFF00):     # length is 16 bits
                part = b[k:k + 0xFF00]
                f.write(bytes([0, len(part) >> 8, len(part) & 255,
                               (a + k) >> 8, (a + k) & 255]))
                f.write(part)
        f.write(bytes([0xFF, 0, 0, exe >> 8, exe & 255]))


def main():
    args = sys.argv[1:]
    out = mapf = None
    new = []
    for a in args:
        if a.startswith("--script="):
            a = "--script=" + SCRIPT
        elif a.startswith("--output="):
            out = a[len("--output="):]
        elif a.startswith("--map="):
            mapf = a[len("--map="):]
        new.append(a)
    if mapf is None:
        mapf = (out or "a.out") + ".map"
        new.append("--map=" + mapf)
    r = subprocess.call(["lwlink"] + new)
    if r != 0 or out is None:
        return r
    # CMOC deletes its map: keep a copy (tools/cocosim reads it)
    if os.path.isdir("build"):
        shutil.copy(mapf, os.path.join("build", "mule-coco.map"))

    # Check the regions from the map.
    secs = {}
    for line in open(mapf):
        m = re.match(r"Section: (\S+) \((.*)\) load at ([0-9A-F]+), length ([0-9A-F]+)", line)
        if not m:
            continue
        name, a, n = m.group(1), int(m.group(3), 16), int(m.group(4), 16)
        s = secs.setdefault(name, [a, a])
        s[0] = min(s[0], a)
        s[1] = max(s[1], a + n)
    bad = 0
    for name, (a, e) in sorted(secs.items(), key=lambda kv: kv[1][0]):
        if e == a:
            continue
        reg, lo, hi = region_of(a)
        if reg is None or e > hi:
            print("lwlink-mule: section %s $%04X-$%04X is outside its region (%s, ends $%04X)"
                  % (name, a, e, reg, hi), file=sys.stderr)
            bad = 1
    if bad:
        return 1

    # Split the DECB file.
    blocks, exe = read_decb(out)
    low, high, ovl = [], [], [[] for _ in range(OVERLAYS + 1)]
    ov = 0
    for a, b in blocks:
        if a == WINDOW:
            ov += 1
            if ov > OVERLAYS:
                print("lwlink-mule: more than %d overlays" % OVERLAYS, file=sys.stderr)
                return 1
        if a < 0x8000:
            low.append((a, b))
        elif WINDOW <= a < 0xFA00:
            if ov == 0:
                print("lwlink-mule: a block at $%04X in the window before overlay 1" % a,
                      file=sys.stderr)
                return 1
            ovl[ov].append((a, b))
        else:
            high.append((a, b))
    base = out[:-4] if out.endswith(".bin") else out
    write_decb(out, low, exe)
    write_decb(base + "hi.bin", high, exe)
    for n in range(1, OVERLAYS + 1):
        write_decb("%s%d.bin" % (base, n), ovl[n], exe)

    # A size table.
    def total(bl):
        return sum(len(b) for _, b in bl)
    print("coco memory: low $2600-$%04X (bss to $%04X, limit $%04X)"
          % (secs.get("rwdata", [0, 0])[1], secs.get("bss", [0, 0])[1], LOW_END))
    print("  low %5d  high %5d (to $%04X, limit $BFF0)"
          % (total(low), total(high), secs.get("hirodata", secs.get("hicode", [0, 0x8000]))[1]))
    print("  overlays " + " ".join("%d:%d" % (n, total(ovl[n])) for n in range(1, OVERLAYS + 1))
          + "  (window %d)" % (0xFA00 - WINDOW))
    return 0


if __name__ == "__main__":
    sys.exit(main())

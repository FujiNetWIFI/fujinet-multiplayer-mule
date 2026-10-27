#!/usr/bin/env python3
"""lwlink-mule.py -- lwlink for CMOC (--lwlink=), with M.U.L.E.'s memory map.

CMOC links with its own script; this swaps in src/coco/mule.link (the
memory map), runs the real lwlink, checks every region's limit, and splits
the DECB output into what the CoCo loads:

  mule.bin      low core (< $8000): LOADM'd by BASIC, has the exec address
  mulehi.bin    high core ($8000 up): loaded by the game itself
                (src/coco/bank.c)
  mule1..5.bin  the overlays, all linked at the window ($E000)

All are DECB .bin files (segments of 00 len addr data, then FF 0000 exec).

The CoCo 3 pass (MULE_LINK=coco3) uses src/coco/mule3.link: high core to
$C000, window to $FE00, and one file per MMU block (src/coco/bank.c):
  mule1.bin  town $E000..  ov1 $F000..     block 4
  mule2.bin  ov3  $E000..  ov2 $EC00..     block 5
  mule3.bin  ov4                           block 6
  mule4.bin  ov5                           block 7
  mule5.bin  gtest (GFXTEST=1 only)        block 15
  mulea.bin  art3 $F800..                  block 3, behind the screen
Each block at $E000 (or $F800) in the output starts a new file.
"""
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
COCO3 = os.environ.get("MULE_LINK") == "coco3"
SCRIPT = os.path.join(HERE, "..", "..", "src", "coco", "mule3.link" if COCO3 else "mule.link")

# Regions: (name, first, end) -- a section starting in one must end in it.
LOW_START = 0x2600      # the CoCo 3: $1F00, above the BASIC loader; its sprite
                        # buffers are below that (gfx3.c)
LOW_END = 0x7C00        # the stack is the 1K below $8000 (--initial-s)
WINDOW = 0xE000
ART = 0xF800
SECT_END = {}           # a section's own limit, where it shares a block
if COCO3:
    LOW_START = 0x1F00
    HIGH_END = 0xC000
    WINDOW_END = 0xFE00
    SECT_END = {"town": 0xF000, "ov3": 0xEC00}
else:
    HIGH_END = 0xBFF0       # the CPU vectors are at $BFF0 (SAM)
    WINDOW_END = 0xFA00     # the sprites' save-unders above (gfx.c)
REGIONS = [
    ("low",    LOW_START, LOW_END),
    ("high",   0x8000, HIGH_END),
    ("window", WINDOW, WINDOW_END),
]
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
        shutil.copy(mapf, os.path.join("build", "mule-coco3.map" if COCO3 else "mule-coco.map"))

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
        hi = SECT_END.get(name, hi)
        if reg is None or e > hi:
            print("lwlink-mule: section %s $%04X-$%04X is outside its region (%s, ends $%04X)"
                  % (name, a, e, reg, hi), file=sys.stderr)
            bad = 1
    if bad:
        return 1

    # Split the DECB file.
    blocks, exe = read_decb(out)
    low, high, art, ovl = [], [], [], [[] for _ in range(OVERLAYS + 1)]
    ov = 0
    dest = None
    for a, b in blocks:
        if a == WINDOW:
            ov += 1
            if ov > OVERLAYS:
                print("lwlink-mule: more than %d overlays" % OVERLAYS, file=sys.stderr)
                return 1
            dest = ovl[ov]
        elif a == ART and COCO3:
            dest = art
        if a < 0x8000:
            low.append((a, b))
        elif WINDOW <= a < WINDOW_END:
            if dest is None:
                print("lwlink-mule: a block at $%04X in the window before overlay 1" % a,
                      file=sys.stderr)
                return 1
            dest.append((a, b))
        else:
            high.append((a, b))
    base = out[:-4] if out.endswith(".bin") else out
    write_decb(out, low, exe)
    write_decb(base + "hi.bin", high, exe)
    for n in range(1, OVERLAYS + 1):
        path = "%s%d.bin" % (base, n)
        if ovl[n] or not COCO3:
            write_decb(path, ovl[n], exe)
        elif os.path.exists(path):
            os.remove(path)          # (GFXTEST=1 builds the fifth)
    if COCO3:
        write_decb(base + "a.bin", art, exe)

    # A size table.
    def total(bl):
        return sum(len(b) for _, b in bl)
    print("%s memory: low $%04X-$%04X (bss to $%04X, limit $%04X)"
          % ("coco3" if COCO3 else "coco", LOW_START, secs.get("rwdata", [0, 0])[1],
             secs.get("bss", [0, 0])[1], LOW_END))
    print("  low %5d  high %5d (to $%04X, limit $%04X)"
          % (total(low), total(high), secs.get("hirodata", secs.get("hicode", [0, 0x8000]))[1],
             HIGH_END))
    if COCO3:
        print("  blocks " + " ".join("%d:%d" % (15 if n == 5 else n + 3, total(ovl[n]))
                                     for n in range(1, OVERLAYS + 1) if ovl[n])
              + "  art 3:%d (1536)" % total(art)
              + "  (8192 each; town < $F000, ov1 < $FE00, ov3 < $EC00, ov2 < $FE00)")
        print("  " + "  ".join("%s $%04X-$%04X" % (name, span[0], span[1])
                               for name, span in sorted(secs.items())
                               if name in ("town", "gtest", "art3", "ov1", "ov2", "ov3", "ov4", "ov5")))
    else:
        print("  overlays " + " ".join("%d:%d" % (n, total(ovl[n])) for n in range(1, OVERLAYS + 1))
              + "  (window %d)" % (WINDOW_END - WINDOW))
    return 0


if __name__ == "__main__":
    sys.exit(main())

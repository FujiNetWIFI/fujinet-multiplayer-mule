#!/usr/bin/env python3
"""The H89 build's overlays: checked, budgeted and packed.

  ovl.py BASE --os cpm|hdos --objs DIR --win ADDR --size BYTES --top ADDR
         [--allow MODULE]... [--z88dk CMD]

BASE is the link's output without its suffix (r2r/h89-cpm/mule): BASE.map,
BASE.com (CP/M) or BASE.abs (HDOS), and the BASE_OVL_<n>.bin z80asm wrote for
each overlay that src/h89/mmap.inc declares.

check  z80asm links a call from one overlay into another without a murmur --
       all are just addresses in the window -- and at run time it lands in
       whatever is loaded. So, from each object's expressions (z88dk-z80nm),
       nothing may refer into an overlay but that overlay itself, and the
       modules named by --allow (main.c: the phase tables, used only after
       bank_overlay). A failure names both ends.

budget Each overlay has to fit the window, and the variables (+ the screen's
       cells; on CP/M a stack) have to fit under --top: the BDOS of a 62K
       CP/M, the one FNDISK needs, or what HDOS leaves with FN: loaded.
       CP/M: the image ends under the window, the variables follow it.
       HDOS: the program follows the window, the variables the program.

pack   What src/h89/bank.c reads, laid out in a table of "MULE" and, for each
       overlay, its first record (16 bits) and its number of records, and then
       the overlays, each from a record:
         CP/M  BASE.ovl, 128-byte records, the table in the first
         HDOS  the end of BASE.abs, 256-byte sectors, the table in the first
               sector past the image HDOS loads

On any failure BASE.com/.abs is removed, so the next make links again.
"""

import argparse
import os
import re
import subprocess
import sys

N = 5
CELLS = 80 * 25         # the H19 layer's buffers after the variables (plat.c)
STACK = 384             # (CP/M: the stack is under the BDOS)


def fail(base, msg):
    for ext in (".com", ".abs"):
        try:
            os.remove(base + ext)
        except OSError:
            pass
    sys.exit("h89 ovl: " + msg)


def read_map(path):
    """name -> address, and (module, name) -> address for the locals."""
    glob, loc = {}, {}
    for line in open(path):
        m = re.match(r"^(\w+)\s*=\s*\$([0-9A-Fa-f]+)\s*;\s*(\w+),\s*(\w+),[^,]*,\s*(\w*)", line)
        if not m:
            continue
        name, val, scope, module = m.group(1), int(m.group(2), 16), m.group(4), m.group(5)
        if scope == "public" or module == "":
            glob[name] = val
        loc[(module, name)] = val
    return glob, loc


def overlay_of_section(sec):
    m = re.match(r"^(?:code|rodata)_ovl_(\d+)$", sec)
    return int(m.group(1)) if m else None


def check(base, objdir, allow, glob, loc, z88dk):
    """Every reference into an overlay comes from inside it (or is allowed)."""
    objs = []
    for root, _, files in os.walk(objdir):
        objs += [os.path.join(root, f) for f in files if f.endswith(".o")]
    bad = []
    for obj in sorted(objs):
        out = subprocess.run(z88dk + ["z88dk-z80nm", "-a", obj], capture_output=True,
                             text=True, check=True).stdout
        module = re.search(r"^\s*Name:\s*(\S+)", out, re.M).group(1)
        if module in allow:
            continue
        part = None
        for line in out.splitlines():
            if line.strip() in ("Symbols:", "Externs:", "Expressions:"):
                part = line.strip()
                continue
            if part != "Expressions:":
                continue
            m = re.match(r"^\s*E (\w+)\s+\$\w+\s+\$\w+\s+\d+:\s*(.*?)\s+\(section (\S+)\)", line)
            if not m:
                continue
            kind, expr, sec = m.groups()
            here = overlay_of_section(sec)
            for sym in re.findall(r"[A-Za-z_]\w*", expr):
                if re.match(r"^__\w+_(head|tail|size)$", sym):
                    continue            # the linker's section bounds
                val = loc.get((module, sym), glob.get(sym))
                if val is None or val < 0x10000:
                    continue
                there = val >> 16
                if there == here or there > N:
                    continue
                bad.append("%s (%s) refers to %s in overlay %d"
                           % (os.path.relpath(obj, objdir), sec, sym, there))
    if bad:
        fail(base, "a reference into an overlay from outside it:\n  " + "\n  ".join(sorted(set(bad))))


def pack_table(sizes, base, first, unit):
    """The table, and the overlays after it from record FIRST + 1."""
    tab, body = b"MULE", b""
    rec = first + 1
    for i, size in enumerate(sizes):
        path = "%s_OVL_%d.bin" % (base, i + 1)
        data = open(path, "rb").read() if os.path.exists(path) else b""
        if len(data) != size:
            fail(base, "%s is %d bytes, the map says %d" % (path, len(data), size))
        n = (len(data) + unit - 1) // unit
        tab += bytes([rec & 255, rec >> 8, n])
        body += data + bytes(n * unit - len(data))
        rec += n
    return tab + bytes(unit - len(tab)) + body


def main():
    ap = argparse.ArgumentParser(usage=__doc__)
    ap.add_argument("base")
    ap.add_argument("--os", choices=["cpm", "hdos"], required=True)
    ap.add_argument("--objs", required=True)
    ap.add_argument("--win", type=lambda v: int(v, 0), required=True)
    ap.add_argument("--size", type=lambda v: int(v, 0), required=True)
    ap.add_argument("--top", type=lambda v: int(v, 0), required=True)
    ap.add_argument("--allow", action="append", default=[])
    ap.add_argument("--z88dk", default="", help="the wrapper that runs z88dk tools (tools/z80/z88dk)")
    a = ap.parse_args()
    base, hdos = a.base, a.os == "hdos"

    glob, loc = read_map(base + ".map")
    check(base, a.objs, set(a.allow), glob, loc, a.z88dk.split())

    # --- budget
    sizes = [(glob["__OVL_%d_END_head" % i] - glob["__OVL_%d_head" % i]) & 0xFFFF
             for i in range(1, N + 1)]
    bss, end = glob["__BSS_head"], glob["__BSS_END_tail"]
    need = end + CELLS + (0 if hdos else STACK)
    if hdos:
        img = open(base + ".abs", "rb").read()
        load, length = img[2] | img[3] << 8, img[4] | img[5] << 8
        # (a second pass over a packed one: the image is what the header says)
        if img[:2] != b"\xff\x00" or len(img) < 8 + length or load + length != bss:
            fail(base, "%s.abs is not a header and the image up to BSS (%04XH, %d bytes, BSS at %04XH)"
                 % (base, load, len(img), bss))
        if load != a.win + a.size:
            fail(base, "the program is at %04XH, not after the window (%04XH)" % (load, a.win + a.size))
        img = img[:8 + length]
        print("h89 ovl: window %04XH-%04XH (%d): %s; program %04XH-%04XH"
              % (a.win, a.win + a.size - 1, a.size,
                 ", ".join("%d %d" % (i + 1, s) for i, s in enumerate(sizes)), load, bss - 1))
    else:
        image = glob["__DATA_END_tail"]
        img = open(base + ".com", "rb").read()
        if len(img) != image - 0x100:
            fail(base, "%s.com is %d bytes for an image of %d" % (base, len(img), image - 0x100))
        print("h89 ovl: image 0100H-%04XH (%d under the window at %04XH), window %d: %s"
              % (image - 1, a.win - image, a.win, a.size,
                 ", ".join("%d %d" % (i + 1, s) for i, s in enumerate(sizes))))
        if image > a.win:
            fail(base, "the image ends at %04XH, past the window (%04XH): raise H89_OVL_WIN" % (image, a.win))
        if bss != a.win + a.size:
            fail(base, "the variables are at %04XH, not after the window (%04XH)" % (bss, a.win + a.size))
    print("h89 ovl: variables %04XH-%04XH; with the screen%s to %04XH: %+d under %04XH"
          % (bss, end - 1, "" if hdos else " and a stack", need, a.top - need, a.top))
    big = [i + 1 for i, s in enumerate(sizes) if s > a.size]
    if big:
        fail(base, "overlay %s past the window: raise H89_OVL_SIZE" % ", ".join(map(str, big)))
    if need > a.top:
        fail(base, "the variables, screen%s end at %04XH, past %04XH"
             % ("" if hdos else " and stack", need, a.top))

    # --- pack
    if hdos:
        img += bytes(-len(img) % 256)
        first = len(img) // 256             # the table's sector
        out = img + pack_table(sizes, base, first, 256)
        open(base + ".abs", "wb").write(out)
        print("h89 ovl: %s.abs %d sectors, HDOS loads %d" % (base, len(out) // 256, first))
    else:
        out = pack_table(sizes, base, 0, 128)
        open(base + ".ovl", "wb").write(out)
        print("h89 ovl: %s.ovl %d records" % (base, len(out) // 128))


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""wlink-mule.py -- wlink for MekkoGX's ow2 toolchain (LD_OW2), with
M.U.L.E.'s overlays.

MekkoGX links every object as one `file {...}` list. This takes the phase
modules out of it and puts each overlay group (bank.h's OV_*) in a section
of its own, loaded on the call by Watcom's dynamic overlay manager, then adds the
medium-model fujinet-lib, the map, the stack, and runs the real wlink
(through tools/dos/ow). It ends with the size of the resident part.

The overlay groups mirror the Apple's (Makefile, A2OBJ):
  1 boot, teststart   2 status   3 grant, events   4 auction   5 develop
with two more that only the PC has, for room on a 128K PCjr: net.c's
appkeys (only boot.c calls them) go with boot, and lobby.c (LEAVE's way
back to the lobby, once) is an overlay of its own.
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
OW = os.path.join(HERE, "ow")
OVERLAYS = [["boot", "teststart", "net"], ["status"], ["grant", "events"],
            ["auction"], ["develop"], ["lobby"]]
FNLIB = os.environ.get("MULE_FNLIB", "build/fnlib-msdos/fujinet-msdos.lib")
MAP = "build/mule-msdos.map"
STACK = 1024                    # ~300 bytes used, FUJINET.SYS calls included
AREA = int(os.environ.get("MULE_OVL_AREA", "4800"))


def main():
    args = sys.argv[1:]
    # `file {a.o b.o ...}`: MekkoGX passes it as words (make splits it)
    try:
        i = args.index("file")
    except ValueError:
        sys.exit("wlink-mule: no file list")
    j = i + 1
    objs = []
    while j < len(args):
        w = args[j]
        objs.extend(x for x in w.strip("{}").split() if x)
        if w.endswith("}"):
            break
        j += 1
    rest = args[:i] + args[j + 1:]

    def base(o):
        return os.path.splitext(os.path.basename(o))[0]

    def ours(o):                # src/X.o or src/msdos/X.o, not a library's
        p = o.replace("\\", "/")
        return "/src/" + base(o) + ".o" in p or "/src/msdos/" + base(o) + ".o" in p

    ov_objs = [[o for o in objs if base(o) in grp and ours(o)] for grp in OVERLAYS]
    flat = {o for g in ov_objs for o in g}
    root = [o for o in objs if o not in flat]

    # a directive file: one directive a line, no quoting to go wrong
    lines = [" ".join(rest), "option map=" + MAP, "option stack=%d" % STACK]
    if not os.environ.get("MULE_NO_OVERLAYS"):
        # the dynamic overlay manager: its area can grow at run time
        # (src/msdos/plat.c gives it what memory there is to spare), so
        # overlays once loaded stay; the code needs -of stack frames
        lines.append("option dynamic")
        # its area: the largest overlay (the default, twice that, doesn't
        # fit a 128K PCjr); wlink raises it, with a warning, if one grows
        lines.append("option area=%d" % AREA)
    lines += ["file " + o for o in root]
    if os.environ.get("MULE_NO_OVERLAYS"):
        lines += ["file " + o for g in ov_objs for o in g]
    else:
        lines.append("begin")
        for g in ov_objs:
            if g:
                lines.append("  section file " + ", ".join(g))
        lines.append("end")
    lines.append("library " + FNLIB)
    lnk = "build/mule-msdos.lnk"
    with open(lnk, "w") as f:
        f.write("\n".join(lines) + "\n")
    cmd = [OW, "wlink", "@" + lnk]
    r = subprocess.run(cmd)
    if r.returncode:
        sys.exit(r.returncode)
    report()


def report():
    """Segment sizes from the map: what stays resident, and each overlay."""
    try:
        text = open(MAP, errors="replace").read()
    except OSError:
        return
    segs = re.findall(r"^(\S+)\s+(\S+)\s+(\S+)\s+([0-9a-fA-F]{4}):([0-9a-fA-F]{4})"
                      r"\s+([0-9a-fA-F]+)\s*$", text, re.M)
    tot = {}
    for name, cls, grp, seg, off, size in segs:
        tot[cls] = tot.get(cls, 0) + int(size, 16)
    if tot:
        print("wlink-mule: segment classes:",
              ", ".join("%s %d" % kv for kv in sorted(tot.items())))
    m = re.search(r"Memory size:\s+([0-9a-fA-F]+)", text)
    if m:
        print("wlink-mule: memory size %d bytes" % int(m.group(1), 16))


if __name__ == "__main__":
    main()

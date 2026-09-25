#!/usr/bin/env python3
"""atarisim.py -- drive the Atari build in libatari800 (asim) from a scenario.

usage: atarisim.py [-xl] DISK.atr MAPFILE SCENARIO OUTDIR

Boots the disk on a 48K Atari 800 (-xl: a 64K 800XL) with the SIO patch,
then plays the scenario (tools/a2sim's language), a command a line; times
are frames:

    test PHASE [ARG [PEVENT]]   offline test mode (src/test.c): phase number,
                                colony event / auction stage / seat, player event
    wait N                      let N frames pass
    snap NAME                   save OUTDIR/NAME.png (and .mem, .regs)
    hold DIRS                   hold directions: any of u d l r (or 'none')
    press                       tap the button
    key CHAR|NUM                type a key (RETURN = 13, ESC = 27)
    peek SYMBOL [N]             print N bytes at a symbol (from the map)
    poke SYMBOL VALUE
    wav FILE | wav off          record the sound
    pokey FILE | pokey off      log the POKEY writes
    asim LINE                   pass a line to asim as it is

The input goes through the game's test hook (inp_hook in input.c); the
addresses come from the linker map. Build asim first (make -C tools/atarisim).
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DIRBITS = {"d": 1, "r": 2, "u": 4, "l": 8}


def symbols(mapfile):
    text = open(mapfile).read()
    part = text.split("Exports list by name:")[1].split("Exports list by value:")[0]
    return {n: int(a, 16) for n, a in re.findall(r"(\S+)\s+([0-9A-F]{6})\s+\S+", part)}


def main():
    args = sys.argv[1:]
    xl = args and args[0] == "-xl"
    if xl:
        args = args[1:]
    disk, mapfile, scen, out = args[:4]
    out = os.path.abspath(out)
    os.makedirs(out, exist_ok=True)
    syms = symbols(mapfile)
    sym = lambda n: syms["_" + n] if "_" + n in syms else syms[n]
    lines, snaps = [], []
    for line in open(scen):
        line = line.split("#")[0].strip()
        if not line:
            continue
        w = line.split()
        c = w[0]
        if c == "test":
            a = [int(x) for x in w[1:]] + [0, 0, 0]
            hook = sym("test_hook")
            lines += ["until %x c3 2000" % (hook + 1),
                      "poke %x %x" % (hook + 1, a[0] + 1), "poke %x %x" % (hook + 2, a[1]),
                      "poke %x %x" % (hook + 3, a[2]), "poke %x 5a" % hook,
                      "until %x a5 600" % hook, "poke %x 1" % sym("inp_hook")]
        elif c == "wait":
            lines.append("wait %s" % w[1])
        elif c == "snap":
            lines.append("shot %s" % w[1])
            snaps.append(w[1])
        elif c == "hold":
            b = 0 if w[1] == "none" else sum(DIRBITS[ch] for ch in w[1])
            lines += ["poke %x 1" % sym("inp_hook"), "poke %x %x" % (sym("inp_hook_dir"), b)]
        elif c == "press":
            lines += ["poke %x 1" % sym("inp_hook"), "poke %x 1" % sym("inp_hook_btn"), "wait 3",
                      "poke %x 0" % sym("inp_hook_btn"), "wait 3"]
        elif c == "key":
            v = int(w[1]) if w[1].isdigit() and len(w[1]) > 1 else ord(w[1][0])
            lines += ["poke %x 1" % sym("inp_hook"), "poke %x %x" % (sym("inp_hook_key"), v), "wait 2"]
        elif c == "peek":
            lines.append("peek %x %x" % (sym(w[1]), int(w[2]) if len(w) > 2 else 1))
        elif c == "poke":
            lines.append("poke %x %x" % (sym(w[1]), int(w[2], 0)))
        elif c in ("wav", "pokey"):
            lines.append(line)
        elif c == "asim":
            lines.append(line[5:])
        else:
            sys.exit("unknown command: " + line)
    script = os.path.join(out, "scenario.txt")
    open(script, "w").write("\n".join(lines) + "\n")
    cmd = [os.path.join(HERE, "asim")] + (["-xl"] if xl else []) + [disk, script, out]
    r = subprocess.run(cmd, capture_output=True, text=True)
    sys.stdout.write(r.stdout)
    err = [l for l in r.stderr.splitlines() if l.strip() and "notified" not in l]
    if err:
        sys.stderr.write("\n".join(err[:20]) + "\n")
    if snaps:
        subprocess.run([sys.executable, os.path.join(HERE, "topng.py")] +
                       [os.path.join(out, s + ".scr") for s in snaps])
        for s in snaps:
            print("SNAP", os.path.join(out, s + ".png"))


if __name__ == "__main__":
    main()

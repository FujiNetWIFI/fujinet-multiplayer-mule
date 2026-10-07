#!/usr/bin/env python3
"""Film the Apple II client's gameplay in MAME (its offline test mode).

usage: clips_a2.py OUTDIR [NAME...]

The same clips as clips_atari.py (the clients share their game code and
test mode), through clients/mekkogx/tools/a2sim: a snapshot every 2 game
frames, OUTDIR/NAME/fNNNN.png (MAME's 560 x 192 colour picture).
"""
import glob
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import clips_atari  # noqa: E402

MX = os.path.join(HERE, "..", "..", "clients", "mekkogx")
SIM = os.path.join(MX, "tools", "a2sim", "a2sim.py")


class Scen(clips_atari.Scen):
    WAIT = 1                # a2sim's snap takes a frame of its own


def run(out, name, sc):
    d = os.path.join(out, name)
    os.makedirs(d, exist_ok=True)
    for f in glob.glob(os.path.join(d, "*.png")):
        os.remove(f)
    p = os.path.join(out, name + ".scen")
    open(p, "w").write(sc.text())
    with open(os.path.join(out, name + ".log"), "w") as log:
        subprocess.run(["python3", SIM, "r2r/apple2/mule.po", "build/mule-apple2.map", os.path.abspath(p),
                        os.path.abspath(d)], cwd=MX, stdout=log, stderr=subprocess.STDOUT, timeout=3600)
    n = len(glob.glob(os.path.join(d, "f*.png")))
    print("%-12s %4d frames" % (name, n))


def main():
    out = os.path.abspath(sys.argv[1])
    want = sys.argv[2:]
    os.makedirs(out, exist_ok=True)
    jobs = [(k, v) for k, v in clips_atari.clips(Scen).items() if k != "gfx" and (not want or k in want)]
    with ThreadPoolExecutor(max_workers=min(12, os.cpu_count() or 4)) as ex:
        list(ex.map(lambda kv: run(out, *kv), jobs))


if __name__ == "__main__":
    main()

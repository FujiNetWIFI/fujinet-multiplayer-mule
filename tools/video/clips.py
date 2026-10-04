#!/usr/bin/env python3
"""Film real gameplay clips from the Intellivision ROM in jzIntv (test mode).

usage: clips.py OUTDIR [NAME...]

Each clip is a clients/intv/tools/intvsim.py scenario. Every frame filmed is
a 704x416 PNG (176x208 scanline space: 160x192 plus an 8 px border) in
OUTDIR/NAME/, numbered in order; render.py plays them back. Filming every 2
frames keeps it fast; playback holds each frame for 2 video frames.
"""
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
INTV = os.path.join(HERE, "..", "..", "clients", "intv")

# Controller bytes for the TEST_HOOK: disc 1 S, 2 E, 4 N, 8 W; button $A0.
S, E, N, W, BTN = "01", "02", "04", "08", "A0"


class Scen:
    """Builds a scenario where every frame after the lead-in is filmed."""

    def __init__(self, test, lead=0):
        self.lines = ["test " + test]
        if lead:
            self.lines.append("wait %d" % lead)
        self.k = 0

    def film(self, frames, inp="0"):
        self.lines.append("set " + inp)
        self.lines.append("film s%02d 2 %d" % (self.k, frames // 2))
        self.k += 1
        return self

    def text(self):
        return "\n".join(self.lines + ["set 0"]) + "\n"


def clips():
    c = {}
    # Month 1 opens with the transport ship landing, then the land grant.
    c["ship"] = Scen("1", 60).film(520)
    c["grant"] = Scen("1", 1040).film(140).film(8, BTN).film(400)
    # A land auction: the plot shown, then bidding by pushing up.
    c["landshow"] = Scen("2 0", 30).film(420)
    c["landbid"] = Scen("2 3", 40).film(60).film(200, N).film(120)
    # Development: corral -> M.U.L.E., food outfitter, out to plot 23, install.
    d = Scen("3 0", 100)
    d.film(40, E).film(40, S).film(30)        # 4 px per 6 frames: x 84 -> 112, into the corral
    d.film(10, E).film(40, N).film(40)        # x -> 120 (food), up into the shop
    d.film(40, E).film(40)                    # out of town on the right, onto plot 23
    d.film(8, BTN).film(160)                  # install
    c["develop"] = d
    c["pevent"] = Scen("3 0 1", 40).film(420)
    c["production"] = Scen("4 0", 30).film(520)
    for ev, name in enumerate(["pest", "pirates", "acid", "quake", "sunspot",
                               "meteor", "radiation", "fire"], 1):
        c["ev_" + name] = Scen("4 %d" % ev, 30).film(560)
    c["goods"] = Scen("5 0", 30).film(470)
    c["declare"] = Scen("5 1", 30).film(120).film(60, N).film(200)
    c["trading"] = Scen("6 3", 30).film(120).film(160, S).film(200)
    c["summary"] = Scen("7", 400).film(420)
    c["species"] = Scen("7 1", 60).film(420)
    c["gameover"] = Scen("8", 700).film(400)
    return c


def run(out, name, sc):
    d = os.path.join(out, name)
    os.makedirs(d, exist_ok=True)
    for f in os.listdir(d):
        os.remove(os.path.join(d, f))
    p = os.path.join(out, name + ".scen")
    open(p, "w").write(sc.text())
    with open(os.path.join(out, name + ".log"), "w") as log:
        subprocess.run(["python3", "tools/intvsim.py", "mule.rom", os.path.abspath(p), os.path.abspath(d)],
                       cwd=INTV, stdout=log, stderr=subprocess.STDOUT, timeout=1800)
    n = len([f for f in os.listdir(d) if f.endswith(".png")])
    print("%-12s %4d frames" % (name, n))


def main():
    out = os.path.abspath(sys.argv[1])
    want = sys.argv[2:]
    os.makedirs(out, exist_ok=True)
    jobs = [(k, v) for k, v in clips().items() if not want or k in want]
    with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as ex:
        list(ex.map(lambda kv: run(out, *kv), jobs))


if __name__ == "__main__":
    main()

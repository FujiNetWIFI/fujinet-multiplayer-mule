#!/usr/bin/env python3
"""Film the Atari client's gameplay in libatari800 (its offline test mode).

usage: clips_atari.py OUTDIR [NAME...]

Each clip is a clients/mekkogx/tools/atarisim scenario: after a lead-in,
a snapshot every 2 frames (fNNNN.scr: 384 x 240 colour indices, read with
palette.bin by render_atari.py). The 'gfx' clip is one snapshot whose RAM
image (gfx.mem) atarigfx.py reads the graphics from.
"""
import glob
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
MX = os.path.join(HERE, "..", "..", "clients", "mekkogx")
SIM = os.path.join(MX, "tools", "atarisim", "atarisim.py")

# the stick, as atarisim's hold takes it; BTN is a press of the trigger
S, E, N, W, BTN, NONE = "d", "r", "u", "l", "btn", "none"


class Scen:
    """Every frame after the lead-in is filmed (a snapshot each 2 frames).
    WAIT is the wait after a snapshot that makes it 2 frames."""
    WAIT = 2
    FPS = 2                 # game frames a snapshot comes to (measured per sim)

    def __init__(self, test, lead=0):
        self.lines = ["test " + test]
        if lead:
            self.lines.append("wait %d" % lead)
        self.n = 0

    def film(self, frames, inp=NONE):
        if inp == BTN:
            self.lines.append("press")          # 6 frames: 3 down, 3 up
            frames -= 6
        else:
            self.lines.append("hold " + inp)
        n = frames // 2 if self.FPS == 2 else int(round(frames / self.FPS))
        for _ in range(max(0, n)):
            self.lines.append("snap f%04d" % self.n)
            self.lines.append("wait %d" % self.WAIT)
            self.n += 1
        return self

    def text(self):
        return "\n".join(self.lines + ["hold none"]) + "\n"


def clips(Scen=Scen):
    c = {}
    g = Scen("3 0", 120)
    g.lines.append("snap gfx")
    c["gfx"] = g
    c["ship"] = Scen("1", 60).film(520)
    c["landbid"] = Scen("2 3", 40).film(60).film(200, N).film(120)
    # Development: corral -> M.U.L.E., food outfitter, out to plot 23, install
    # (4 px a step every 6 frames, as on the Intellivision: same walk code).
    d = Scen("3 0", 100)
    d.film(40, E).film(40, S).film(30)
    d.film(10, E).film(40, N).film(40)
    d.film(40, E).film(40)
    d.film(8, BTN).film(160)
    c["develop"] = d
    c["map"] = Scen("3 0", 120).film(8, W).film(240, W).film(60, N).film(40)
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
    for f in glob.glob(os.path.join(d, "*")):
        os.remove(f)
    p = os.path.join(out, name + ".scen")
    open(p, "w").write(sc.text())
    with open(os.path.join(out, name + ".log"), "w") as log:
        subprocess.run(["python3", SIM, "r2r/atari/mule.atr", "build/mule-atari.map", os.path.abspath(p),
                        os.path.abspath(d)], cwd=MX, stdout=log, stderr=subprocess.STDOUT, timeout=3600)
    # keep the frames (.scr) and palette; the RAM images only for 'gfx'
    for f in glob.glob(os.path.join(d, "f*.mem")) + glob.glob(os.path.join(d, "f*.regs")) + \
            glob.glob(os.path.join(d, "f*.png")):
        os.remove(f)
    n = len(glob.glob(os.path.join(d, "f*.scr")))
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

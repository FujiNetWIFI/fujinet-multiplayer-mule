#!/usr/bin/env python3
"""Film a MekkoGX client's gameplay in its emulator (its offline test mode).

usage: clips_mgx.py TARGET OUTDIR [NAME...]

TARGET is one of the sims below. The clips are clips_atari.py's (the clients
share their game code and test mode); each is a snapshot every 2 game frames
in OUTDIR/NAME/ (fNNNN.png, and fNNNN.vram from the MSX's sim).
"""
import glob
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import clips_atari  # noqa: E402

MX = os.path.abspath(os.path.join(HERE, "..", "..", "clients", "mekkogx"))
OLD_MAME = os.path.expanduser("~/Workspace/mame/.old-mame")    # mame -> mamed (0.263), roms


def old_mame():
    """MAME 0.263 in a tree of its own: the CoCo and the PCjr need it."""
    if not os.path.isdir(OLD_MAME):
        os.makedirs(OLD_MAME)
        os.symlink(os.path.expanduser("~/Workspace/mame/mamed"), os.path.join(OLD_MAME, "mame"))
        os.symlink(os.path.expanduser("~/Workspace/mame/roms"), os.path.join(OLD_MAME, "roms"))
    return OLD_MAME


# sim: command (from clients/mekkogx), environment, extra lead-in frames,
# how many at once, and (FPS) the game frames a "snap + wait 1" comes to
TARGETS = {
    "msdos-cga": (["tools/dossim/dossim.py", "r2r/msdos/mule.exe", "build/mule-msdos.map"],
                  {"DOSSIM_MACHINE": "ibm5150", "DOSSIM_NOFN": "1"}, 200, 8),
    "msdos-pcjr": (["tools/dossim/dossim.py", "r2r/msdos/mule.exe", "build/mule-msdos.map"],
                   {"MAME": "OLD", "DOSSIM_RAMOPT": "-ramsize", "DOSSIM_RAM": "128K", "DOSSIM_NOFN": "1"},
                   440, 8),
    "coco": (["tools/cocosim/cocosim.py", "r2r/coco/mule.dsk", "build/mule-coco.map"],
             {"MAME": "OLD"}, 450, 3),
    "adam": (["tools/adam/adamsim/adamsim.py", "r2r/adam/mule.ddp", "r2r/adam/mule.map"],
             {"ADAMSIM_SECONDS": "1200"}, 2100, 8),
    "msx": (["tools/msx/msxsim/msxsim.py", "r2r/msxrom/mule.rom", "r2r/msxrom/mule.map"], {}, 0, 8),
    "lynx": (["tools/lynxsim/lynxsim.py", "r2r/lynx/mule.lnx", "build/mule-lynx.map"], {}, 0, 8),
    # not MekkoGX, but the same test mode and scenario language
    "astrocade": (["@astrocade", "tools/astrosim.py"], {}, 0, 8),
}
FPS = {
    "msx": 1.45,            # msxsim steps on 0.01 s of emulated time, not frames
}


class Scen(clips_atari.Scen):
    WAIT = 1                # these sims' snap takes a frame of its own
    LEAD = 0

    def __init__(self, test, lead=0):
        super().__init__(test, lead + self.LEAD)


def run(target, out, name, sc):
    cmd, env, lead, _ = TARGETS[target]
    d = os.path.join(out, name)
    os.makedirs(d, exist_ok=True)
    for f in glob.glob(os.path.join(d, "f*")):
        os.remove(f)
    p = os.path.join(out, name + ".scen")
    open(p, "w").write(sc.text())
    e = dict(os.environ)
    for k, v in env.items():
        e[k] = old_mame() if v == "OLD" else v
    cwd = MX
    if cmd[0].startswith("@"):          # a client of its own: clients/<name>
        cwd = os.path.join(MX, "..", cmd[0][1:])
        cmd = cmd[1:]
    with open(os.path.join(out, name + ".log"), "w") as log:
        subprocess.run(["python3"] + cmd + [os.path.abspath(p), os.path.abspath(d)], cwd=cwd, env=e,
                       stdout=log, stderr=subprocess.STDOUT, timeout=7200)
    n = len(glob.glob(os.path.join(d, "f*.png")))
    print("%-12s %4d frames" % (name, n), flush=True)


def main():
    target, out = sys.argv[1], os.path.abspath(sys.argv[2])
    want = sys.argv[3:]
    os.makedirs(out, exist_ok=True)
    cls = type("Scen_" + target.replace("-", "_"), (Scen,),
               {"LEAD": TARGETS[target][2], "FPS": FPS.get(target, 2)})
    jobs = [(k, v) for k, v in clips_atari.clips(cls).items() if k != "gfx" and (not want or k in want)]
    with ThreadPoolExecutor(max_workers=TARGETS[target][3]) as ex:
        list(ex.map(lambda kv: run(target, out, *kv), jobs))


if __name__ == "__main__":
    main()

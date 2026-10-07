#!/usr/bin/env python3
"""Cut the 2600's live game (clips_2600.sh) into the storyboard's clips.

usage: a26clips.py CLIPDIR

CLIPDIR/raw/NNNNN.png are the frames in order, CLIPDIR/film.log says which
screen each was filmed on: "FILM k key BANK:PHASE:ROUND ..." (round 99: any
after the second). Each clip is a window of one screen, linked as
CLIPDIR/NAME/fNNNN.png.

  bank 3 status screens, 4 the map (phase 1 grant, 2 auction, 3 others'
  turns, 4 production), 5 your turn on the map, 6 the town, 7 auctions
  (2 land, 6 trading), 9 goods and declaring, 13 player events, 14 messages
"""
import glob
import os
import re
import sys

CLIPS = {           # name: (screens to try, in order)
    "landbid": ("7:2:2", "7:2:99"),
    "develop": ("6:3:1", "6:3:2"),
    "map": ("4:3:99", "4:3:2"),
    "grant": ("4:1:1",),
    "pevent": ("13:3:99",),
    "production": ("4:4:2", "4:4:1"),
    "ev": ("14:4:99", "4:4:99"),
    "ev2": ("4:4:99",),
    "ev3": ("4:4:1",),
    "goods": ("9:5:1",),
    "declare": ("9:5:2",),
    "trading": ("7:6:1",),
    "summary": ("3:7:2", "3:7:1"),
    "species": ("3:7:1",),
    "gameover": ("3:8:99", "3:7:99"),
}


def main():
    d = os.path.abspath(sys.argv[1])
    frames = {}
    for ln in open(os.path.join(d, "film.log")):
        m = re.match(r"FILM (\d+) key (\S+)", ln)
        if m:
            frames.setdefault(m.group(2), []).append(int(m.group(1)))
    raw = sorted(glob.glob(os.path.join(d, "raw", "*.png")), key=lambda p: int(os.path.basename(p)[:-4]))
    for name, keys in CLIPS.items():
        key = next((k for k in keys if k in frames), None)
        out = os.path.join(d, name)
        os.makedirs(out, exist_ok=True)
        for f in glob.glob(os.path.join(out, "f*.png")):
            os.remove(f)
        if key is None:
            print("%-11s no footage" % name)
            continue
        for i, k in enumerate(frames[key]):
            if k < len(raw):
                os.symlink(raw[k], os.path.join(out, "f%04d.png" % i))
        print("%-11s %-8s %4d frames" % (name, key, len(frames[key])))


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""rowcheck.py -- count the frames of a burst whose text or map rows are
drawn differently from the burst's usual frame (a late band cuts the tops
off, or drops, a row's characters).

    rowcheck.py DIR NAME

Each frame is reduced to its lit pixels per 8-line band (the grid's green
left out); a frame whose bands differ from the most common frame's is
listed. The cursor and the plots change during a burst, so read the list,
don't just count it.
"""
import collections
import glob
import os
import sys

from PIL import Image


def sig(path):
    im = Image.open(path).convert("RGB")
    w, h = im.size
    px = im.load()
    bands = []
    for y0 in range(0, h, 8):
        n = 0
        for y in range(y0, min(h, y0 + 8)):
            for x in range(w):
                r, g, b = px[x, y]
                if r + g + b > 120 and not (g > 2 * r and g > 2 * b):
                    n += 1
        bands.append(n)
    return tuple(bands)


def main(d, name):
    files = sorted(glob.glob(os.path.join(d, name + "_*.png")))
    sigs = [sig(f) for f in files]
    common = collections.Counter(sigs).most_common(1)[0][0]
    odd = [(os.path.basename(f), s) for f, s in zip(files, sigs) if s != common]
    print("%d frames, %d differ" % (len(files), len(odd)))
    for f, s in odd:
        diff = [(8 * k, a - b) for k, (a, b) in enumerate(zip(s, common)) if a != b]
        print("  %s: %s" % (f, " ".join("y%d:%+d" % d for d in diff)))


if __name__ == "__main__":
    main(*sys.argv[1:3])

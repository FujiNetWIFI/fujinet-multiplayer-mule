#!/usr/bin/env python3
"""topng.py NAME.scr ... -- asim screenshots to PNG (the palette.bin asim
writes next to them). The visible 336x224 of the 384x240 frame, doubled."""
import os
import sys
from PIL import Image

for fn in sys.argv[1:]:
    pal = open(os.path.join(os.path.dirname(os.path.abspath(fn)), "palette.bin"), "rb").read()
    d = open(fn, "rb").read()
    im = Image.new("RGB", (336, 224))
    im.putdata([tuple(pal[c * 3:c * 3 + 3]) for y in range(8, 232) for c in d[y * 384 + 24:y * 384 + 360]])
    im.resize((672, 448), Image.NEAREST).save(fn[:-4] + ".png")

#!/usr/bin/env python3
"""mka78.py -- put the 128-byte .a78 header MAME needs on a headerless image.

    tools/atari7800/mka78.py image.bin out.a78 [title] [--type N]

MAME loads a loose 7800 cart only with this header; the FujiNet cart strips
it, and a claimed image's mapper is its claim's (the cart and MAME's model
of it both go by the claim). --type is the header's cart type word (0 plain;
6 SuperGame with RAM at $4000), for other emulators. One joystick per port,
NTSC.
"""
import sys

args = [a for a in sys.argv[1:] if not a.startswith("--type")]
ctype = 0
for i, a in enumerate(sys.argv):
    if a.startswith("--type"):
        ctype = int(a.split("=", 1)[1] if "=" in a else sys.argv[i + 1], 0)
        if "=" not in a:
            args.remove(sys.argv[i + 1])
if len(args) < 2:
    sys.exit("usage: mka78.py image.bin out.a78 [title] [--type N]")
data = open(args[0], "rb").read()
title = (args[2] if len(args) > 2 else "FUJINET").encode()[:32]
h = bytearray(128)
h[0] = 1
h[1:10] = b"ATARI7800"
h[17:17 + len(title)] = title
h[49:53] = len(data).to_bytes(4, "big")
h[53:55] = ctype.to_bytes(2, "big")
h[55] = 1
h[56] = 1
h[57] = 0
h[100:128] = b"ACTUAL CART DATA STARTS HERE"
open(args[1], "wb").write(bytes(h) + data)

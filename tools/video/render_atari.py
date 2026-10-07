#!/usr/bin/env python3
"""Render the Atari 8-bit M.U.L.E. new-player intro video.

usage: render_atari.py BUILD OUT.mp4            the whole video
       render_atari.py BUILD --still T [T...]   PNG stills (BUILD/still_T.png)

BUILD (videos/build/atari) holds music.wav (pokey.py) and clips/
(clips_atari.py, with the gfx snapshot atarigfx.py reads the graphics
from). Text is M.U.L.E.'s own font, from the client's RAM. The scenes are
mgx_scenes.py's, shared with the Apple II.

The canvas is 480 x 270 units of one hi-res pixel by one scanline (half an
Atari colour clock across), each drawn 4 x 4: 1920 x 1080. The 320 x 192
playfield of a clip sits at 1x; a colour clock is 2 units, a player bit 2
units (8 at quad width) by one line.
"""
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import vidlib  # noqa: E402
import mgx_scenes as M  # noqa: E402
from atarigfx import AtariGfx  # noqa: E402

# The game's colours (atari_col, gfx.c), and lighter ones for text on black.
BLACK, WHITE, BLUE, PURPLE, GREEN, ORANGE, RED, YELLOW = 0x00, 0x0F, 0x84, 0x54, 0xC4, 0x34, 0x32, 0x2E
GREY, CYAN, BROWN, TAN, LTBLUE, DKGREEN, PINK = 0x08, 0xA8, 0x24, 0x2A, 0x88, 0xD4, 0x5A
MAPBG, DIM = 0x08, 0x04
AG = None


def lit(c):
    """A colour's hue at a luminance that reads on black."""
    return (c & 0xF0) | 0x0C


def frame(name, k):
    return AG.frame(name, k)


def shape(cv, name, k, x, y, c, s=1, flip=False, wide=1):
    """A player: a bit is a colour clock (2 units), a row a scanline."""
    cv.bits(x, y, AG.frame(name, k), c, sx=2 * s * wide, sy=s, flip=flip)


def ship(cv, x, y, kind=0, s=1):
    """fx.c: the nose 8 colour clocks ahead and 10 lines down of a hull
    drawn at quad width, its top white and its bottom blue (pirate red)."""
    shape(cv, "nose", kind, x - 16 * s, y + 10 * s, WHITE, s)
    shape(cv, "hull_top", kind, x, y, WHITE, s, wide=4)
    shape(cv, "hull_bot", kind, x, y + 16 * s, BLUE if kind == 0 else RED, s, wide=4)


def pic(cv, x, y, k, c, s=1):
    cv.bits(x, y, AG.pics[k], c, sx=2 * s, sy=s)


def cursor(cv, x, y):
    """The land grant cursor: a plot's outline, at double width."""
    cv.bits(x, y, AG.frame("cursor", 0), WHITE, sx=4, sy=1)


def cursor_icon(cv, x, y):
    cv.bits(x, y - 4, AG.frame("cursor", 0)[::2], WHITE, sx=2, sy=1)


def plot_xy(p):
    """A plot's top left in playfield units (gfx.h PLOT_X/PLOT_Y)."""
    return (8 + 16 * (p % 9)) * 2, 16 + 32 * (p // 9)


def init(build):
    global AG
    AG = AtariGfx(os.path.join(build, "clips", "gfx"))
    pal = AG.pal

    def load_frame(path):
        """An asim .scr (384 x 240 colour indices): the playfield is rows
        24-215, columns 32-351."""
        a = np.fromfile(path, dtype=np.uint8).reshape(240, 384)[24:216, 32:352]
        return pal[a]

    vidlib.setup(M.W, M.H, M.SX, M.SY, pal, AG.glyphs(), WHITE, DIM)
    M.configure(
        BUILD=build, PAL=pal,
        BLACK=BLACK, WHITE=WHITE, BLUE=BLUE, PURPLE=PURPLE, GREEN=GREEN, ORANGE=ORANGE, RED=RED,
        YELLOW=YELLOW, GREY=GREY, CYAN=CYAN, TAN=TAN, DKGREEN=DKGREEN, PINK=PINK,
        MAPBG=MAPBG, DIM=DIM, ACC=0x9C, FRAMECOL=DIM + 2, STAR_TINT=LTBLUE,
        NUMCOL=DIM + 4, DOTCOL=DIM + 2, LABELCOL=DIM + 6,
        PLAYER=[BLUE, PURPLE, GREEN, ORANGE],                 # pl_col
        PTEXT=[0x8C, 0x5C, 0xCC, 0x3C],                       # the same hues, lit for black
        GOOD_COL=[0xC6, 0x1A, 0x06, 0x86],                    # the original's goods colours
        lit=lit,
        PFX=80, PFY=4, PFW=320, PFH=192, CW=32, plot_xy=plot_xy,
        load_frame=load_frame, CLIP_GLOB="f*.scr",
        TOWN_BOX=(16, 160, 64, 256), TCW=16,
        frame=frame, shape=shape, ship=ship, pic=pic, cursor=cursor, cursor_icon=cursor_icon,
        TITLE=("ATARI 8-BIT", "400 / 800 / XL / XE"), BUTTON="TRIGGER",
        CONTROLS=[("JOYSTICK", "WALK, BID, MOVE"), ("TRIGGER OR SPACE", "CLAIM, INSTALL"),
                  ("RETURN", "END YOUR TURN"), ("0", "SIT OUT AN AUCTION")],
        CONTROLS_NOTE="NO JOYSTICK? CTRL + ARROW KEYS WALK.",
        OUTRO_IMAGE="ATARI/MULE.ATR IN D1:",
        PLATFORM="atari", ONLINE_FOUR=("intv", "apple2", "msx"),
        CORNERS=("ATARI 8-BIT", "INTELLIVISION", "APPLE II", "COCO"),
        MAPSTILL=("map", 110), TOWN_STILL=("develop", 0), PUB_STILL=("develop", 40),
        START={},
    )


if __name__ == "__main__":
    M.main(init)

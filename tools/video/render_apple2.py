#!/usr/bin/env python3
"""Render the Apple II M.U.L.E. new-player intro video.

usage: render_apple2.py BUILD OUT.mp4            the whole video
       render_apple2.py BUILD --still T [T...]   PNG stills (BUILD/still_T.png)

BUILD (videos/build/apple2) holds music.wav (mockingboard.py) and clips/
(clips_a2.py). The graphics are the client's own (a2gfx.py, from its build);
text is its HGR font (M.U.L.E.'s, in 7-dot cells). The scenes are
mgx_scenes.py's, shared with the Atari.

The canvas is 480 x 270 units of one hi-res dot (1/280 of the line) by one
scanline, drawn 4 x 4: 1920 x 1080. A clip's 280 x 192 picture sits at 1x;
a sprite's colour pixel is 2 dots.
"""
import os
import sys

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import vidlib  # noqa: E402
import mgx_scenes as M  # noqa: E402
from a2gfx import A2Gfx  # noqa: E402

# The HGR colours as MAME draws them, two of its in-between colours, and a
# grey for the video's own frames.
PAL = np.array([(0, 0, 0), (255, 255, 255), (25, 215, 0), (230, 40, 255), (230, 111, 0), (25, 144, 255),
                (64, 99, 0), (64, 28, 247), (90, 90, 90), (150, 150, 150)], dtype=np.uint8)
BLACK, WHITE, GREEN, VIOLET, ORANGE, BLUE, DKGREEN, DKBLUE, DIM, GREY = range(10)
G = None


def frame(name, k):
    return G.frame(name, k)


def shape(cv, name, k, x, y, c, s=1, flip=False, wide=1):
    """A sprite: a bit is a colour pixel (2 dots), a row a scanline."""
    cv.bits(x, y, G.frame(name, k), c, sx=2 * s, sy=s, flip=flip, width=G.width(name))


def ship(cv, x, y, kind=0, s=1):
    """fx.c's ship: the nose ahead of and 10 lines down from the hull (already
    widened in the Apple's shapes), its top white and its bottom blue."""
    shape(cv, "nose", kind, x - 14 * s, y + 10 * s, WHITE, s)
    shape(cv, "hull_top", kind, x, y, WHITE, s)
    shape(cv, "hull_bot", kind, x, y + 16 * s, BLUE if kind == 0 else ORANGE, s)


def pic(cv, x, y, k, c, s=1):
    """hgr_pics (8 x 8; the game draws a row on 2 lines, the video on s)."""
    cv.bits(x, y, G.pics[k], c, sx=2 * s, sy=s)


def cursor(cv, x, y):
    """The land grant cursor: a plot's outline (14 colour pixels, 32 lines)."""
    shape(cv, "cursor", 0, x, y, WHITE)


def cursor_icon(cv, x, y):
    cv.bits(x, y - 4, G.frame("cursor", 0)[::2], WHITE, sx=2, sy=1, width=16)


def plot_xy(p):
    """A plot's top left in dots (gfx.c gfx_plot: byte 2 + 4 * column)."""
    return (2 + 4 * (p % 9)) * 7, 16 + 32 * (p // 9)


def load_frame(path):
    """MAME's 560 x 192 picture: every other column is a dot."""
    return np.array(Image.open(path).convert("RGB"))[:, ::2]


def init(build):
    global G
    G = A2Gfx()
    vidlib.setup(M.W, M.H, M.SX, M.SY, PAL, G.glyphs(), WHITE, DIM)
    M.configure(
        BUILD=build, PAL=PAL,
        BLACK=BLACK, WHITE=WHITE, BLUE=BLUE, PURPLE=VIOLET, GREEN=GREEN, ORANGE=ORANGE, RED=ORANGE,
        YELLOW=ORANGE, GREY=GREY, CYAN=BLUE, TAN=ORANGE, DKGREEN=DKGREEN, PINK=VIOLET,
        MAPBG=BLACK, DIM=DIM, ACC=BLUE, FRAMECOL=DIM, STAR_TINT=BLUE,
        NUMCOL=GREY, DOTCOL=DIM, LABELCOL=GREY,
        PLAYER=[BLUE, VIOLET, GREEN, ORANGE],                 # pl_col, as hcol[] draws them
        PTEXT=[BLUE, VIOLET, GREEN, ORANGE],
        GOOD_COL=[GREEN, ORANGE, WHITE, BLUE],
        lit=lambda c: c,
        PFX=100, PFY=4, PFW=280, PFH=192, CW=28, plot_xy=plot_xy,
        load_frame=load_frame, CLIP_GLOB="f*.png",
        TOWN_BOX=(16, 160, 56, 224), TCW=14,
        frame=frame, shape=shape, ship=ship, pic=pic, cursor=cursor, cursor_icon=cursor_icon,
        TITLE=("APPLE II", "//E   //C   IIGS"), BUTTON="BUTTON",
        CONTROLS=[("ARROWS OR I J K M", "WALK, BID, MOVE"), ("SPACE OR BUTTON", "CLAIM, INSTALL"),
                  ("RETURN", "END YOUR TURN"), ("0", "SIT OUT AN AUCTION")],
        CONTROLS_NOTE="JOYSTICK? CENTRE IT FIRST.",
        OUTRO_IMAGE="BOOT APPLE2/MULE.PO",
        PLATFORM="apple2", ONLINE_FOUR=("intv", "atari", "msx"),
        CORNERS=("APPLE II", "ATARI 8-BIT", "INTELLIVISION", "COCO"),
        MAPSTILL=("map", 110), TOWN_STILL=("develop", 0), PUB_STILL=("develop", 40),
        # the Apple draws the map in as a phase starts: begin after it
        START={"landbid": 30, "pevent": 30, "goods": 20, "declare": 20, "trading": 20,
               "summary": 40, "ev_pirates": 60, "ev_acid": 30, "ev_fire": 120, "ev_pest": 30,
               "ev_meteor": 30},
    )


if __name__ == "__main__":
    M.main(init)

#!/usr/bin/env python3
"""Render the intro video for one of the other MekkoGX clients.

usage: render_mgx.py TARGET BUILD OUT.mp4            the whole video
       render_mgx.py TARGET BUILD --still T [T...]   PNG stills

TARGET: msdos coco adam msx lynx. BUILD (videos/build/<target>) holds
music.wav (chips.py) and clips/ (clips_mgx.py; msdos also pcjr/). The
graphics and font are the client's own (mgxgfx.py, from the tables its
build compiles); the scenes are mgx_scenes.py's.

The canvas is 480 x 270 units drawn 4 x 4 (1920 x 1080). A unit is one of
the machine's pixels, except on the Lynx (160 x 102), drawn at 2 x 2.
"""
import glob
import os
import sys

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import vidlib  # noqa: E402
import mgx_scenes as M  # noqa: E402
from mgxgfx import MgxGfx  # noqa: E402

G = None


def rgb(lst):
    return np.array(lst, dtype=np.uint8)


def png(path):
    return np.array(Image.open(path).convert("RGB"))


def big(a, k):
    return np.repeat(np.repeat(a, k, axis=0), k, axis=1)


# ---------------------------------------------------------------------------
# Drawing hooks, by how a shape's bit maps onto the screen
# ---------------------------------------------------------------------------
def make_hooks(kind, white, blue, red):
    """kind: 'clock' (a bit is 2 units: DOS, CoCo), 'tms' (wide12: 1.5 pixels),
    'lynx' (2 units each way, the data at half height)."""

    def shape(cv, name, k, x, y, c, s=1, flip=False, wide=1):
        if kind == "tms":
            rows, w = G.widened(name, k)
            cv.bits(x, y, rows, c, sx=s, sy=s, flip=flip, width=w)
        elif kind == "lynx":
            cv.bits(x, y, G.frame(name, k), c, sx=2 * s, sy=2 * s, flip=flip, width=G.width(name))
        else:
            cv.bits(x, y, G.frame(name, k), c, sx=2 * s, sy=s, flip=flip, width=G.width(name))

    def ship(cv, x, y, kind_=0, s=1):
        nose = {"tms": 12, "lynx": 16, "clock": 16}[kind] * s
        if G.target == "coco":
            nose = 12 * s
        hh = G.shapes["hull_top"][3] * (2 if kind == "lynx" else 1) * s
        shape(cv, "nose", kind_, x - nose, y + 10 * s, white, s)
        shape(cv, "hull_top", kind_, x, y, white, s)
        shape(cv, "hull_bot", kind_, x, y + hh, blue if kind_ == 0 else red, s)

    def pic(cv, x, y, k, c, s=1):
        sx = {"tms": 2, "lynx": 2, "clock": 2}[kind] * s
        cv.bits(x, y, G.pics[k], c, sx=sx, sy=s)

    def cursor(cv, x, y):
        shape(cv, "cursor", 0, x, y, white)

    def cursor_icon(cv, x, y):
        fr, wb, w, h = G.shapes["cursor"]
        rows = fr[0][::2] if kind != "lynx" else fr[0]
        cv.bits(x, y - 4, rows, white, sx=1 if kind == "tms" else 2, sy=1, width=wb * 8)

    return dict(frame=lambda n, k: G.frame(n, k), shape=shape, ship=ship, pic=pic, cursor=cursor,
                cursor_icon=cursor_icon)


ATARI_PLOT = lambda p: ((8 + 16 * (p % 9)) * 2, 16 + 32 * (p // 9))     # noqa: E731
COMMON = dict(lit=lambda c: c, PFY=4)


def tms_target(name, title, controls, note, button, outro, folder, crop):
    """The Adam and the MSX: a TMS9918A, 256 x 192, the Namco font."""
    pal = rgb([(0, 0, 0), (0, 0, 0), (33, 200, 66), (94, 220, 120), (84, 85, 237), (125, 118, 252),
               (212, 82, 77), (66, 235, 245), (252, 85, 84), (255, 121, 120), (212, 193, 84),
               (230, 206, 128), (33, 176, 59), (201, 91, 186), (204, 204, 204), (255, 255, 255),
               (110, 110, 110)])
    BLACK, WHITE, GREY, LTBLUE, MAGENTA, GREEN, LTRED, LTYELLOW, CYAN, DKBLUE, DIM = \
        1, 15, 14, 5, 13, 2, 9, 11, 7, 4, 16
    return dict(
        tms=True, PAL=pal, BG=BLACK,
        BLACK=BLACK, WHITE=WHITE, BLUE=LTBLUE, PURPLE=MAGENTA, GREEN=GREEN, ORANGE=LTRED, RED=8,
        YELLOW=LTYELLOW, GREY=GREY, CYAN=CYAN, TAN=10, DKGREEN=12, PINK=MAGENTA,
        MAPBG=GREY, DIM=DIM, ACC=CYAN, FRAMECOL=DIM, STAR_TINT=LTBLUE, NUMCOL=GREY, DOTCOL=DIM,
        LABELCOL=GREY, PLAYER=[LTBLUE, MAGENTA, GREEN, LTRED], PTEXT=[LTBLUE, MAGENTA, GREEN, LTRED],
        GOOD_COL=[GREEN, LTYELLOW, WHITE, CYAN],
        PFX=112, PFW=256, PFH=192, CW=24, plot_xy=lambda p: (16 + 24 * (p % 9), 16 + 32 * (p // 9)),
        load_frame=crop, CLIP_GLOB="f*.png", TOWN_BOX=(16, 160, 52, 196), TCW=12,
        hooks="tms", TITLE=title, BUTTON=button, CONTROLS=controls, CONTROLS_NOTE=note,
        OUTRO_IMAGE=outro, PLATFORM=folder)


TARGETS = {
    "adam": lambda: tms_target(
        "adam", ("COLECO ADAM", "WITH A FUJINET"),
        [("CONTROLLER", "WALK, BID, MOVE"), ("FIRE OR SPACE", "CLAIM, INSTALL"),
         ("RETURN OR *", "END YOUR TURN"), ("0", "SIT OUT AN AUCTION")],
        "THE CONTROLLER WALKS BEST.", "FIRE BUTTON", "ADAM/MULE.DDP", "adam",
        lambda p: png(p)[12:204, 12:268]),
    "msx": lambda: tms_target(
        "msx", ("MSX", "MSX1 AND LATER"),
        [("ARROWS OR JOYSTICK", "WALK, BID, MOVE"), ("SPACE OR TRIGGER", "CLAIM, INSTALL"),
         ("RETURN", "END YOUR TURN"), ("0", "SIT OUT AN AUCTION")],
        "F1: SOUND ON AND OFF.", "SPACE BAR", "MSXROM/MULE.ROM", "msx",
        lambda p: png(p)[::2, ::2]),
}


def msdos():
    # CGA palette 1, high intensity, and the map's grey background
    pal = rgb([(0, 0, 0), (85, 255, 255), (255, 85, 255), (255, 255, 255), (85, 85, 85), (60, 60, 60),
               (170, 170, 170)])
    BLACK, CYAN, MAGENTA, WHITE, GREY, DIM, LTGREY = range(7)
    return dict(
        PAL=pal, BG=BLACK, BLACK=BLACK, WHITE=WHITE, BLUE=CYAN, PURPLE=MAGENTA, GREEN=WHITE,
        ORANGE=CYAN, RED=MAGENTA, YELLOW=MAGENTA, GREY=LTGREY, CYAN=CYAN, TAN=WHITE, DKGREEN=GREY,
        PINK=MAGENTA, MAPBG=GREY, DIM=DIM, ACC=CYAN, FRAMECOL=GREY, STAR_TINT=CYAN, NUMCOL=LTGREY,
        DOTCOL=GREY, LABELCOL=LTGREY,
        PLAYER=[CYAN, MAGENTA, WHITE, CYAN], PTEXT=[CYAN, MAGENTA, WHITE, CYAN],
        GOOD_COL=[CYAN, MAGENTA, WHITE, CYAN],
        PFX=80, PFW=320, PFH=192, CW=32, plot_xy=ATARI_PLOT,
        load_frame=lambda p: png(p)[4:196], CLIP_GLOB="f*.png", TOWN_BOX=(16, 160, 64, 256), TCW=16,
        hooks="clock", TITLE=("IBM PC", "CGA  PCJR  TANDY 1000"), BUTTON="SPACE BAR",
        CONTROLS=[("ARROW KEYS", "WALK, BID, MOVE"), ("SPACE", "CLAIM, INSTALL"),
                  ("ENTER", "END YOUR TURN"), ("0", "SIT OUT AN AUCTION")],
        CONTROLS_NOTE="OR USE A JOYSTICK.", OUTRO_IMAGE="MSDOS/MULE.IMG", PLATFORM="msdos-cga")


def coco():
    # the artifact colours as MAME draws them
    pal = rgb([(38, 48, 22), (38, 124, 173), (191, 124, 22), (191, 200, 173), (0, 0, 0), (70, 78, 60),
               (120, 128, 110)])
    PFBLACK, BLUE, ORANGE, WHITE, BLACK, DIM, GREY = range(7)
    return dict(
        PAL=pal, BG=BLACK, BLACK=BLACK, WHITE=WHITE, BLUE=BLUE, PURPLE=ORANGE, GREEN=WHITE,
        ORANGE=BLUE, RED=ORANGE, YELLOW=ORANGE, GREY=GREY, CYAN=BLUE, TAN=WHITE, DKGREEN=DIM,
        PINK=ORANGE, MAPBG=PFBLACK, DIM=DIM, ACC=BLUE, FRAMECOL=DIM, STAR_TINT=BLUE, NUMCOL=GREY,
        DOTCOL=DIM, LABELCOL=GREY,
        PLAYER=[BLUE, ORANGE, WHITE, BLUE], PTEXT=[BLUE, ORANGE, WHITE, BLUE],
        GOOD_COL=[BLUE, ORANGE, WHITE, BLUE],
        PFX=112, PFW=256, PFH=192, CW=24, plot_xy=lambda p: ((2 + 3 * (p % 9)) * 8, 16 + 32 * (p // 9)),
        load_frame=lambda p: png(p)[25:217, 58:314], CLIP_GLOB="f*.png", TOWN_BOX=(16, 160, 52, 196),
        TCW=12, hooks="clock", TITLE=("COLOR COMPUTER", "TRS-80 COCO 1 AND 2"), BUTTON="SPACE BAR",
        CONTROLS=[("ARROW KEYS", "WALK, BID, MOVE"), ("SPACE", "CLAIM, INSTALL"),
                  ("ENTER", "END YOUR TURN"), ("0", "SIT OUT AN AUCTION")],
        CONTROLS_NOTE="OR A JOYSTICK, RIGHT PORT.", OUTRO_IMAGE="COCO/MULE.DSK", PLATFORM="coco")


def lynx():
    src = G.src
    from mgxgfx import carray
    lp = carray(src, "lynx_pal")
    pal = rgb([((lp[16 + i] & 15) * 17, (lp[i] & 15) * 17, (lp[16 + i] >> 4) * 17) for i in range(16)]
              + [(70, 70, 70)])
    # pens = the game's colours (gfx.h C_*)
    BLACK, WHITE, BLUE, PURPLE, GREEN, ORANGE, RED, YELLOW, GREY, CYAN, BROWN, TAN, LTBLUE, DKGREEN, \
        PINK = range(15)
    DIM = 16
    return dict(
        PAL=pal, BG=BLACK, BLACK=BLACK, WHITE=WHITE, BLUE=BLUE, PURPLE=PURPLE, GREEN=GREEN,
        ORANGE=ORANGE, RED=RED, YELLOW=YELLOW, GREY=GREY, CYAN=CYAN, TAN=TAN, DKGREEN=DKGREEN,
        PINK=PURPLE, MAPBG=GREY, DIM=DIM, ACC=LTBLUE, FRAMECOL=DIM, STAR_TINT=LTBLUE, NUMCOL=GREY,
        DOTCOL=DIM, LABELCOL=GREY,
        PLAYER=[BLUE, PURPLE, GREEN, ORANGE], PTEXT=[LTBLUE, PURPLE, GREEN, ORANGE],
        GOOD_COL=[GREEN, YELLOW, WHITE, CYAN],
        PFX=80, PFY=0, PFW=320, PFH=204, CW=32,
        plot_xy=lambda p: (2 * (8 + 16 * (p % 9)), 2 * (10 + 16 * (p // 9))),
        load_frame=lambda p: big(png(p), 2), CLIP_GLOB="f*.png", TOWN_BOX=(20, 164, 64, 256), TCW=16,
        hooks="lynx", TITLE=("ATARI LYNX", "THE HANDHELD"), BUTTON="A BUTTON",
        CONTROLS=[("JOYPAD", "WALK, BID, MOVE"), ("A", "CLAIM, INSTALL"),
                  ("OPTION 1", "END YOUR TURN"), ("OPTION 1 + B", "SIT OUT AN AUCTION")],
        CONTROLS_NOTE="PAUSE: SOUND ON AND OFF.", OUTRO_IMAGE="LYNX/MULE.LNX", PLATFORM="lynx",
        TS=(4, 3), STEP=19)


def astrocade():
    # the one palette the client sets (mule.asm COLSET), as MAME draws it
    pal = rgb([(0, 0, 0), (238, 238, 238), (28, 86, 255), (208, 95, 0), (80, 80, 80), (150, 150, 150)])
    BLACK, WHITE, BLUE, ORANGE, DIM, GREY = range(6)
    return dict(
        PAL=pal, BG=BLACK, BLACK=BLACK, WHITE=WHITE, BLUE=BLUE, PURPLE=ORANGE, GREEN=WHITE,
        ORANGE=BLUE, RED=ORANGE, YELLOW=ORANGE, GREY=GREY, CYAN=BLUE, TAN=WHITE, DKGREEN=DIM,
        PINK=ORANGE, MAPBG=BLACK, DIM=DIM, ACC=BLUE, FRAMECOL=DIM, STAR_TINT=BLUE, NUMCOL=GREY,
        DOTCOL=DIM, LABELCOL=GREY,
        PLAYER=[BLUE, ORANGE, WHITE, BLUE], PTEXT=[BLUE, ORANGE, WHITE, BLUE],
        GOOD_COL=[BLUE, ORANGE, WHITE, BLUE],
        # the picture: 160 x 102 at 2x in MAME's 352 x 240 (from 16, 18); the
        # game uses the top 78 lines (4K of screen RAM)
        PFX=80, PFW=320, PFH=160, CW=32, PH=24, TRH=12,
        plot_xy=lambda p: (2 * (8 + 16 * (p % 9)), 2 * (6 + 12 * (p // 9))),
        load_frame=lambda p: png(p)[18:178, 16:336], CLIP_GLOB="f*.png",
        TOWN_BOX=(12, 120, 64, 256), TCW=16,
        hooks="lynx", TITLE=("ASTROCADE", "BALLY PROFESSIONAL ARCADE"), BUTTON="TRIGGER",
        CONTROLS=[("HAND CONTROLLER", "WALK, BID, MOVE"), ("TRIGGER", "CLAIM, INSTALL"),
                  ("KEYPAD =", "END YOUR TURN"), ("KEYPAD 0", "SIT OUT AN AUCTION")],
        CONTROLS_NOTE="KEYPAD MR: SOUND ON AND OFF.", OUTRO_IMAGE="ASTROCADE/MULE.BIN",
        PLATFORM="astrocade", TS=(4, 3), STEP=19)


def a2600():
    # the NTSC palette (tools/extract/ntsc.pal), indexed by the 2600's colour
    # bytes (src/mdefs.inc): $0E white, $06 grey, $84 blue, $54 purple,
    # $C4 green, $34 red, $1A gold, $08 the map's ground, $96 the river
    pal = np.frombuffer(open(os.path.join(HERE, "..", "extract", "ntsc.pal"), "rb").read(),
                        dtype=np.uint8).reshape(256, 3)
    return dict(
        PAL=pal, BG=0, BLACK=0, WHITE=0x0E, BLUE=0x84, PURPLE=0x54, GREEN=0xC4, ORANGE=0x34,
        RED=0x34, YELLOW=0x2E, GREY=0x08, CYAN=0x9C, TAN=0x2A, DKGREEN=0xD4, PINK=0x5A,
        MAPBG=0x08, DIM=0x04, ACC=0x9C, FRAMECOL=0x06, STAR_TINT=0x88, NUMCOL=0x08, DOTCOL=0x06,
        LABELCOL=0x0A, lit=lambda c: (c & 0xF0) | 0x0C,
        PLAYER=[0x84, 0x54, 0xC4, 0x34], PTEXT=[0x8C, 0x5C, 0xCC, 0x3C],
        GOOD_COL=[0xC6, 0x1A, 0x06, 0x86],
        PFX=80, PFW=320, PFH=192, CW=32, plot_xy=ATARI_PLOT,
        load_frame=lambda p: np.repeat(png(p)[A26_Y0:A26_Y0 + 192, 8:168], 2, axis=1),
        CLIP_GLOB="f*.png", hooks="clock", ship=None,
        TITLE=("ATARI 2600", "THE VIDEO COMPUTER SYSTEM"), BUTTON="FIRE BUTTON",
        CONTROLS=[("JOYSTICK", "WALK, BID, MOVE"), ("FIRE", "CLAIM, INSTALL"),
                  ("SELECT", "END YOUR TURN"), ("FIRE, DECLARING", "SIT OUT AN AUCTION")],
        CONTROLS_NOTE="HOLD RESET TO LEAVE THE TABLE.", OUTRO_IMAGE="ATARI-2600/MULE2600.BIN",
        PLATFORM="atari-2600", TS=(4, 3), STEP=19,
        EVENTS=[("ev3", 20), ("ev3", 80), ("ev", 20), ("ev", 80), ("ev2", 40)],
        **A26_MAP)


A26_Y0 = 15                     # the picture's first line in MAME's 176 x 223
# The live game's map (the 'map' still: the land grant, month 3) and town
# (the 'develop' clip's town), measured on the frames: plots by eye, the
# buildings in pixels of the town picture (TCW = TRH = 1).
A26_MAP = dict(
    MAPSTILL=("map", 110), TOWN_STILL=("develop", 40), PUB_STILL=("develop", 40),
    TERRAIN_PLOTS=(0, 13, 39, 3, 17),
    PLOT_MAP={11: 0, 31: 13, 25: 39, 5: 3, 15: 17},
    OWNED={1, 4, 10, 11, 12, 13, 19, 21, 22, 24, 25, 31, 34, 40},
    CLAIMS=[(20, 0, 2.1), (23, 1, 3.4), (33, 2, 4.5)],
    TOWN_BOX=(20, 166, 64, 256), TCW=1, TRH=1,
    TOWN_RECT={"CRYSTITE": (8, 5, 32, 32), "SMITHORE": (56, 5, 32, 32), "ENERGY": (104, 5, 32, 32),
               "FOOD": (152, 5, 32, 32), "ASSAY": (8, 69, 32, 56), "LAND": (48, 69, 32, 48),
               "PUB": (88, 69, 32, 40), "CORRAL": (136, 69, 48, 72)},
    START={"develop": 20},
)


def a26_letter_icon(cv, g, x, y, k=1):
    """The 2600 shows a plot's M.U.L.E. as a white letter: F E S C."""
    cv.text(x + 2, y + 2, "FESC"[g], 0x0E, 4 * k, 4 * k)


def a26_letter_cell(cv, g, x, y):
    cv.rect(x, y, 64, 64, 0x08)
    cv.frame(x, y, 64, 64, 0x84, 2)
    cv.text(x + 16, y + 14, "FESC"[g], 0x0E, 8, 6)


A26_MAP.update(GOOD_ICON=a26_letter_icon, GOOD_CELL=a26_letter_cell)

GFX_OF = {"msdos": "msdos", "coco": "coco", "adam": "tms", "msx": "tms", "lynx": "lynx",
          "astrocade": "astrocade"}
TARGETS.update({"msdos": msdos, "coco": coco, "lynx": lynx, "astrocade": astrocade, "a2600": a2600})


# ---------------------------------------------------------------------------
# MS-DOS: the PCjr and Tandy 1000's 16 colours, side by side with CGA
# ---------------------------------------------------------------------------
def pcjr_frame(path):
    return png(path)[4:196, ::2]


def sc_pcjr(cv, t, d):
    M.CT(cv, 4, "16 COLOURS", M.YELLOW, 2)
    if t < 7.5:
        town = t >= 3.75
        tt = t - 3.75 if town else t
        if town:
            cga = M.clip("develop").get(0)
            jr = vidlib.Clip(os.path.join(M.BUILD, "pcjr", "develop"), "f*.png", pcjr_frame).get(0)
        else:
            cga = M.still(*M.MAPSTILL)
            jr = vidlib.Clip(os.path.join(M.BUILD, "pcjr", "map"), "f*.png", pcjr_frame).get(110)
        for i, (a, label) in enumerate(((cga, "CGA: 4 COLOURS"), (jr, "PCJR, TANDY 1000: 16"))):
            if i == 1 and tt < 0.6:
                continue
            k = 2.75
            im = Image.fromarray(a)
            im = np.array(im.resize((int(a.shape[1] * k), int(a.shape[0] * k)), Image.NEAREST))
            x = 60 + i * 920
            b = np.zeros((im.shape[0] + 8, im.shape[1] + 8, 3), dtype=np.uint8)
            b[:] = M.PAL[M.GREY]
            b[4:-4, 4:-4] = im
            cv.over(x - 4, 150, b)
            cv.otext(x + im.shape[1] // 2, 150 + im.shape[0] + 24, label, M.WHITE if i == 0 else M.CYAN, 4)
        M.cap(cv, t, "A PCJR OR TANDY 1000 GETS 16-COLOUR GRAPHICS.", y=232)
    else:
        tt = t - 7.5
        c = vidlib.Clip(os.path.join(M.BUILD, "pcjr", "develop"), "f*.png", pcjr_frame)
        cv.frame(M.PFX - 3, 36, M.PFW + 6, M.PFH + 4, M.GREY, 1)
        cv.paste(M.PFX, 38, c.get(int(tt * 30) + 40))
        M.cap(cv, tt, "THE SAME GAME, IN 16 COLOURS.", y=234, c=M.CYAN)


def scenes(target):
    S = list(M.SCENES)
    if target == "msdos":
        # 10 s for the PCjr, from the species, the month, the goods and trading
        d = {M.sc_species: 20, M.sc_month: 10, M.sc_resources: 16, M.sc_trading: 8}
        S = [(d.get(fn, dur), fn) for dur, fn in S]
        i = [fn for _, fn in S].index(M.sc_town)
        S.insert(i + 1, (10, sc_pcjr))
    assert abs(sum(dur for dur, _ in S) - 240) < 0.01, sum(dur for dur, _ in S)
    return S


def init_for(target):
    def init(build):
        global G
        if target == "astrocade":
            from astrogfx import AstroGfx
            G = AstroGfx()
        elif target == "a2600":
            from a26gfx import A26Gfx
            G = A26Gfx()
        else:
            G = MgxGfx(GFX_OF[target])
        cfg = dict(COMMON, **TARGETS[target]())
        hooks = make_hooks(cfg.pop("hooks"), cfg["WHITE"], cfg["BLUE"], cfg["RED"])
        cfg.pop("tms", None)
        ship_hook = cfg.pop("ship", "hook")
        if ship_hook is None:
            hooks["ship"] = None
        cfg.pop("BG", None)
        vidlib.setup(M.W, M.H, M.SX, M.SY, cfg["PAL"], G.glyphs(), cfg["WHITE"], cfg["DIM"], cell=G.cell)
        names = dict(vidlib.PLATFORMS)                      # folder -> label
        me = names.get(cfg["PLATFORM"], cfg["TITLE"][0])
        others = [n for n in ("ATARI 8-BIT", "INTELLIVISION", "APPLE II") if n != me]
        if target == "msdos":
            # screenshots/msdos-cga's summary caught the walk-in: our own clip's
            # finished table instead
            own = os.path.join(build, "own-03-summary.png")
            Image.open(sorted(glob.glob(os.path.join(build, "clips", "summary", "f*.png")))[150]).save(own)
            vidlib.SHOT_OVERRIDE[("msdos-cga", "03-summary")] = own
        conf = dict(MAPSTILL=("map", 110), TOWN_STILL=("develop", 0), PUB_STILL=("develop", 40),
                    START={}, IMG_FRAMES=1.45 if target == "msx" else 2,
                    CORNERS=(me,) + tuple(others[:2]) + ("COCO" if me != "COCO" else "MSX",),
                    ONLINE_FOUR=tuple(f for f in ("intv", "atari", "apple2", "msx")
                                      if f != cfg["PLATFORM"])[:3])
        conf.update(cfg)
        conf.update(hooks)
        M.configure(BUILD=build, SCENES=scenes(target), **conf)
    return init


if __name__ == "__main__":
    target = sys.argv.pop(1)
    M.main(init_for(target))

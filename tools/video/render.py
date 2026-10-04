#!/usr/bin/env python3
"""Render the Intellivision M.U.L.E. new-player intro video.

usage: render.py BUILD OUT.mp4            the whole video
       render.py BUILD --still T [T...]   PNG stills at T seconds (BUILD/still_T.png)

BUILD holds music.wav (chip.py) and clips/ (clips.py). Graphics come from
the ROM (romgfx.py); text is the Intellivision font.

Everything is drawn on a 240 x 270 canvas whose unit is one Intellivision
pixel across by one scanline down, the way the console builds its picture
(a background pixel is 1 x 2, a 0.5x MOB row is 1 x 1). Each unit becomes
8 x 4 video pixels: 1920 x 1080, with the console's pixel aspect.
"""
import glob
import math
import os
import subprocess
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import romgfx  # noqa: E402
from romgfx import (BLACK, BLUE, RED, TAN, DKGREEN, GREEN, YELLOW, WHITE,  # noqa: E402,F401
                    GREY, CYAN, ORANGE, BROWN, PINK, LTBLUE, YELGREEN, PURPLE)

FONT = os.path.expanduser("~/.local/share/fonts/intellivision.otf")
INTV = os.path.join(HERE, "..", "..", "clients", "intv")
W, H, SX, SY = 240, 270, 8, 4
FPS = 60
PAL = np.array(romgfx.PAL, dtype=np.uint8)
PLAYER = [BLUE, PURPLE, GREEN, ORANGE]          # pl_mobcol (screen.bas)
PTEXT = [BLUE, RED, GREEN, TAN]                 # pl_txtcol
COLS = W // 8

G = romgfx.load(INTV)
MC_TLU, MC_TLO, MC_TR, MC_BL, MC_BR, MC_STORE, MC_RIVER = 0, 3, 7, 13, 22, 28, 32
GOOD_COL = [GREEN, YELLOW, GREY, CYAN]
GOOD_NAME = ["FOOD", "ENERGY", "SMITHORE", "CRYSTITE"]
SPECIES = ["MECHTRON", "GOLLUMER", "PACKER", "BONZOID", "SPHEROID", "FLAPPER", "LEGGITE", "HUMANOID"]


# ---------------------------------------------------------------------------
# Font: the Intellivision OTF is an 8 x 8 bitmap face; cut each glyph out.
# ---------------------------------------------------------------------------
def load_font():
    f = ImageFont.truetype(FONT, 8)
    glyphs = {}
    for c in range(32, 127):
        im = Image.new("L", (8, 8), 0)
        d = ImageDraw.Draw(im)
        d.fontmode = "1"
        d.text((0, 0), chr(c), font=f, fill=255)
        glyphs[chr(c)] = np.array(im) > 127
    return glyphs


GLYPH = load_font()


def bitmask(rows, width=8):
    return np.array([[(r >> (width - 1 - x)) & 1 for x in range(width)] for r in rows], dtype=bool)


def card(block, i):
    return G[block][i]


def cards(block, i, n):
    out = []
    for k in range(n):
        out += G[block][i + k]
    return out


# ---------------------------------------------------------------------------
# The canvas
# ---------------------------------------------------------------------------
class Canvas:
    def __init__(self, bg=BLACK):
        self.a = np.zeros((H, W, 3), dtype=np.uint8)
        self.a[:] = PAL[bg]
        self.overlays = []          # (x, y, rgb) in video pixels, drawn last
        self.k = 1.0

    def rect(self, x, y, w, h, c):
        x0, y0, x1, y1 = max(0, x), max(0, y), min(W, x + w), min(H, y + h)
        if x1 > x0 and y1 > y0:
            self.a[y0:y1, x0:x1] = PAL[c]

    def frame(self, x, y, w, h, c, t=1):
        self.rect(x, y, w, t, c)
        self.rect(x, y + h - t, w, t, c)
        self.rect(x, y, t, h, c)
        self.rect(x + w - t, y, t, h, c)

    def mask(self, x, y, m, c, sx=1, sy=1, flip=False, bg=None):
        if flip:
            m = m[:, ::-1]
        m = np.kron(m, np.ones((sy, sx), dtype=bool))
        h, w = m.shape
        x0, y0 = max(0, x), max(0, y)
        x1, y1 = min(W, x + w), min(H, y + h)
        if x1 <= x0 or y1 <= y0:
            return
        sub = m[y0 - y:y1 - y, x0 - x:x1 - x]
        region = self.a[y0:y1, x0:x1]
        if bg is not None:
            region[:] = PAL[bg]
        region[sub] = PAL[c]

    def bits(self, x, y, rows, c, sx=1, sy=2, flip=False, bg=None):
        """Card rows (bytes): a background card is sy=2, a 0.5x MOB sy=1."""
        self.mask(x, y, bitmask(rows), c, sx, sy, flip, bg)

    def text(self, x, y, s, c, sx=1, sy=2):
        for i, ch in enumerate(s):
            g = GLYPH.get(ch)
            if g is not None and g.any():
                self.mask(x + i * 8 * sx, y, g, c, sx, sy)

    def ctext(self, y, s, c, sx=1, sy=2, cx=W // 2):
        self.text(cx - len(s) * 4 * sx, y, s, c, sx, sy)

    def paste(self, x, y, arr):
        h, w = arr.shape[:2]
        x0, y0, x1, y1 = max(0, x), max(0, y), min(W, x + w), min(H, y + h)
        if x1 > x0 and y1 > y0:
            self.a[y0:y1, x0:x1] = arr[y0 - y:y1 - y, x0 - x:x1 - x]

    def dim(self, k):
        self.k = k

    def over(self, x, y, img):
        self.overlays.append((int(x), int(y), img))

    def otext(self, cx, y, s, c, scale=3):
        """Small text straight on the video picture, centred on cx (labels
        too small for the canvas grid), in square pixels of `scale`."""
        if not s:
            return
        m = np.zeros((8, 8 * len(s)), dtype=bool)
        for i, ch in enumerate(s):
            g = GLYPH.get(ch)
            if g is not None:
                m[:, i * 8:i * 8 + 8] = g
        m = np.kron(m, np.ones((scale, scale), dtype=bool))
        img = np.zeros(m.shape + (4,), dtype=np.uint8)
        img[m, :3] = PAL[c]
        img[m, 3] = 255
        self.over(cx - m.shape[1] // 2, y, img)

    def video(self):
        v = np.repeat(np.repeat(self.a, SY, axis=0), SX, axis=1)
        VW, VH = W * SX, H * SY
        for x, y, img in self.overlays:
            h, w = img.shape[:2]
            x0, y0, x1, y1 = max(0, x), max(0, y), min(VW, x + w), min(VH, y + h)
            if x1 <= x0 or y1 <= y0:
                continue
            sub = img[y0 - y:y1 - y, x0 - x:x1 - x]
            if sub.shape[2] == 4:
                m = sub[:, :, 3] > 0
                v[y0:y1, x0:x1][m] = sub[:, :, :3][m]
            else:
                v[y0:y1, x0:x1] = sub
        if self.k < 1.0:
            v = (v.astype(np.float32) * self.k).astype(np.uint8)
        return v


def wrap(s, n=COLS - 2):
    out, line = [], ""
    for w in s.split():
        if line and len(line) + 1 + len(w) > n:
            out.append(line)
            line = w
        else:
            line = (line + " " + w).strip()
    if line:
        out.append(line)
    return out


def typed(s, t, cps=40):
    """The first part of s revealed after t seconds of typing."""
    return s[:max(0, int(t * cps))]


def caption(cv, t, text, y=204, c=WHITE, step=20, cps=45):
    """Centred, word-wrapped, typed-out caption lines."""
    lines = wrap(text)
    done = 0
    for i, ln in enumerate(lines):
        shown = typed(ln, t - done / cps, cps)
        cv.ctext(y + i * step, shown, c)
        done += len(ln)


def blink(t, hz=2.0):
    return (t * hz) % 1.0 < 0.5


# ---------------------------------------------------------------------------
# Sprites from the ROM
# ---------------------------------------------------------------------------
def species_frame(sid, f):
    """8 x 32 rows (0.5x MOBs: one row per scanline)."""
    return cards("GFX_SPECIES", (sid * 11 + f) * 4, 4)


def draw_species(cv, x, y, sid, f, c, s=1):
    cv.bits(x, y, species_frame(sid, f), c, sx=s, sy=s)


def walk_frame(t, base, rate=8):
    return base + int(t * rate) % 2


def draw_mapsp(cv, x, y, sid, f, c, s=1):
    cv.bits(x, y, card("GFX_MAPSP", sid * 10 + f), c, sx=s, sy=s)


def draw_mule(cv, x, y, f, c, s=1, flip=False):
    cv.bits(x, y, card("GFX_MULE", f), c, sx=s, sy=2 * s, flip=flip)


def draw_ship(cv, x, y, kind=0, s=1):
    """fx_draw_ship: a 32 x 16 px body (4 double-width, 4x-tall MOBs, light top
    and dark bottom) with the nose MOB 8 px ahead of it, 10 lines down."""
    sh = G["GFX_SHIP"][kind * 6:kind * 6 + 6]
    top, bot = (LTBLUE, BLUE) if kind == 0 else (PINK, RED)
    cv.bits(x - 8 * s, y + 10 * s, sh[0] + sh[1], top, sx=s, sy=s)
    cv.bits(x, y, sh[2], top, sx=2 * s, sy=4 * s)
    cv.bits(x + 16 * s, y, sh[3], top, sx=2 * s, sy=4 * s)
    cv.bits(x, y + 16 * s, sh[4], bot, sx=2 * s, sy=4 * s)
    cv.bits(x + 16 * s, y + 16 * s, sh[5], bot, sx=2 * s, sy=4 * s)


# ---------------------------------------------------------------------------
# The map (map.bas): 9 x 5 plots of 2 x 2 cards from BACKTAB (1, 1); the
# river is three MOBs at x 76; the store is plot 22.
# ---------------------------------------------------------------------------
def make_map():
    """Terrain like test.bas's: river column 4, a mountain plot either side."""
    terr = []
    for mp in range(45):
        r, c = mp // 9, mp % 9
        t = ("plain", 0)
        if c == 4:
            t = ("river", 0)
        if mp == 22:
            t = ("store", 0)
        if c == r % 4:
            t = ("mount", r % 3 + 1)
        if c == 5 + r % 4:
            t = ("mount", (r + 1) % 3 + 1)
        terr.append(t)
    return terr


MAP = make_map()


def draw_plot(cv, ox, oy, mp, owner=None, good=None, dots=0, white=False, terr=None):
    x = ox + 8 + (mp % 9) * 16
    y = oy + 16 + (mp // 9) * 32
    draw_plot_at(cv, x, y, terr or MAP[mp], owner, good, dots, white)


def draw_plot_at(cv, x, y, terr, owner=None, good=None, dots=0, white=False, s=1):
    kind, mm = terr
    if kind == "store":
        for k in range(4):
            cv.bits(x + (k % 2) * 8 * s, y + (k // 2) * 16 * s, card("GFX_MAP", MC_STORE + k), BLACK, s, 2 * s)
        return
    if owner is None and not white:
        if mm:
            cv.bits(x, y, card("GFX_MAP", MC_TLU + mm - 1), BLACK, s, 2 * s)
        return
    c = WHITE if white else PLAYER[owner]
    mg = 0 if good is None else good + 1
    cv.bits(x, y, card("GFX_MAP", MC_TLO + mm), c, s, 2 * s)
    cv.bits(x + 8 * s, y, card("GFX_MAP", MC_TR + mg), c, s, 2 * s)
    cv.bits(x, y + 16 * s, card("GFX_MAP", MC_BL + min(dots, 8)), c, s, 2 * s)
    cv.bits(x + 8 * s, y + 16 * s, card("GFX_MAP", MC_BR + mg), c, s, 2 * s)


def good_icon(cv, x, y, g, c, s=1):
    """A good's icon: the right half of an owned plot (TR over BR) without
    the plot's border."""
    tr = [b & 0xFE for b in card("GFX_MAP", MC_TR + 1 + g)]
    br = [b & 0xFE for b in card("GFX_MAP", MC_BR + 1 + g)]
    tr[0] = 0
    br[7] = 0
    cv.bits(x, y, tr + br, c, s, 2 * s)


def draw_river(cv, ox, oy, shake=0):
    tex = cards("GFX_MAP", MC_RIVER, 2)
    x = ox + 76 + shake
    cv.bits(x, oy + 16, tex, BLUE, sx=1, sy=4)
    cv.bits(x, oy + 80, tex, BLUE, sx=1, sy=4)
    cv.bits(x, oy + 144, tex[:8], BLUE, sx=1, sy=4)


def draw_map(cv, ox, oy, owners=None, blink_plot=None, t=0.0, river=True):
    """The 160 x 192 map screen at (ox, oy). owners: {plot: (seat, good, dots)}."""
    owners = owners or {}
    cv.rect(ox, oy, 160, 192, GREY)
    if river:
        draw_river(cv, ox, oy)
    for mp in range(45):
        o = owners.get(mp)
        white = blink_plot == mp and blink(t, 3)
        if o:
            draw_plot(cv, ox, oy, mp, o[0], o[1], o[2] if len(o) > 2 else 0, white)
        else:
            draw_plot(cv, ox, oy, mp, white=white)


# ---------------------------------------------------------------------------
# The town (develop.bas town_enter): 12 x 9 MODE 1 cards at BACKTAB (4, 1).
# ---------------------------------------------------------------------------
TOWN_RECT = {"CRYSTITE": (0, 0, 3, 3), "SMITHORE": (3, 0, 3, 3), "ENERGY": (6, 0, 3, 3),
             "FOOD": (9, 0, 3, 3), "ASSAY": (0, 4, 3, 4), "LAND": (3, 4, 2, 4),
             "PUB": (5, 4, 3, 3), "CORRAL": (8, 4, 4, 5)}


def draw_town(cv, ox, oy, s=1, only=None):
    tc = G["GFX_TOWN"]
    for i, w in enumerate(G["TOWN_SCREEN"]):
        r, c = divmod(i, 12)
        if only:
            bx, by, bw, bh = only
            if not (bx <= c < bx + bw and by <= r < by + bh):
                continue
        x, y = ox + c * 8 * s, oy + r * 16 * s
        if not w:
            cv.rect(x, y, 8 * s, 16 * s, BLACK)
            continue
        n = (w >> 3) & 0x3F
        fg = w & 7
        bg = (w >> 9 & 1) | (w >> 9 & 2) | ((w >> 13) & 1) << 2 | ((w >> 12) & 1) << 3
        cv.bits(x, y, tc[n] if n < len(tc) else [0] * 8, fg, sx=s, sy=2 * s, bg=bg)


# ---------------------------------------------------------------------------
# Clips filmed by clips.py: 704 x 416 PNGs, each one 2 game frames.
# ---------------------------------------------------------------------------
# frames caught half way through a screen change (one per transition)
CLIP_DROP = {"develop": (120,)}


class Clip:
    def __init__(self, build, name):
        self.files = sorted(glob.glob(os.path.join(build, "clips", name, "*.png")))
        if not self.files:
            raise SystemExit("no clip %s: run clips.py first" % name)
        self.cache = {}
        self.drop = CLIP_DROP.get(name, ())

    def __len__(self):
        return len(self.files)

    def get(self, i):
        i = max(0, min(len(self.files) - 1, i))
        while i in self.drop:
            i -= 1
        if i not in self.cache:
            if len(self.cache) > 64:
                self.cache.clear()
            a = np.array(Image.open(self.files[i]).convert("RGB"))[::2, ::4]
            self.cache[i] = a[8:200, 8:168]           # the 160 x 192 picture
        return self.cache[i]


CLIPS = {}


def clip(name):
    if name not in CLIPS:
        CLIPS[name] = Clip(BUILD, name)
    return CLIPS[name]


def show_clip(cv, name, t, start=0, x=40, y=4, speed=1.0):
    """Play a clip from image `start` (an image is 2 frames at 60 Hz)."""
    i = start + int(t * 30 * speed)
    cv.frame(x - 2, y - 2, 164, 196, DKGREEN, 1)
    cv.paste(x, y, clip(name).get(i))
    return i


# ---------------------------------------------------------------------------
# Scenes. Each is (seconds, fn(cv, t, d)) where t runs 0..d.
# ---------------------------------------------------------------------------
def stars(cv, t, n=70, seed=7, speed=6.0):
    rng = np.random.default_rng(seed)
    xs = rng.uniform(0, W, n)
    ys = rng.integers(0, H, n)
    ph = rng.uniform(0, 1, n)
    for i in range(n):
        x = int((xs[i] - t * speed * (1 + i % 3)) % W)
        c = WHITE if (t * 1.5 + ph[i]) % 1 < 0.8 else GREY
        cv.rect(x, int(ys[i]), 1, 2, c if i % 4 else LTBLUE)


def sc_title(cv, t, d):
    stars(cv, t)
    # the transport ship crosses right to left, as it does in month 1
    sx = int(260 - t * 75)
    if sx > -80:
        draw_ship(cv, sx, 70, 0, 2)
    if t > 2.6:
        word = "M.U.L.E."
        n = min(len(word), int((t - 2.6) / 0.12) + 1)
        x0 = W // 2 - len(word) * 12
        for i in range(n):
            drop = max(0, 1 - (t - 2.6 - i * 0.12) / 0.25)
            cv.text(x0 + i * 24, 40 - int(drop * 40), word[i], YELLOW, 3, 6)
    if t > 4.2:
        cv.ctext(110, "INTELLIVISION", CYAN, 2, 4)
    if t > 5.2:
        cv.ctext(150, typed("A GUIDE FOR NEW COLONISTS", t - 5.2, 30), WHITE)
    if t > 4.5:
        mx = int(-20 + (t - 4.5) * 70)
        draw_mule(cv, mx, 200, walk_frame(t, 0, 6), WHITE, 2)
    cv.rect(0, 236, W, 2, GREY)


def sc_premise(cv, t, d):
    cv.rect(0, 166, W, 104, GREY)
    # mountains scattered along the ground
    for i, (x, y, m) in enumerate([(12, 176, 2), (60, 222, 1), (206, 172, 3), (176, 230, 2), (100, 240, 1)]):
        cv.bits(x, y, card("GFX_MAP", MC_TLU + m - 1), BLACK, 2, 4)
    # four colonists walk in from the left, one behind the other
    for k in range(4):
        sid = 1 + k * 2
        x = int(-30 - k * 34 + t * 34)
        x = min(x, 160 - k * 36)
        f = walk_frame(t + k * 0.13, 0) if x < 160 - k * 36 else 4 + int(t * 2) % 2
        draw_mapsp(cv, x, 190, sid, f, PLAYER[k], 3)
    lines = [(0.0, "FOUR COLONISTS LAND ON", WHITE), (0.0, "THE PLANET IRATA.", WHITE),
             (2.6, "YOU HAVE 12 MONTHS TO", WHITE), (2.6, "BUILD A COLONY.", WHITE),
             (5.2, "SCORE = MONEY+LAND+GOODS", YELLOW),
             (7.4, "THE RICHEST WINS, BUT", CYAN), (7.4, "THE COLONY MUST SURVIVE.", CYAN)]
    y = 10
    prev = None
    for t0, s, c in lines:
        if prev is not None and t0 != prev:
            y += 6
        prev = t0
        if t >= t0:
            cv.ctext(y, typed(s, t - t0 - (0.5 if s.startswith("THE P") or s.startswith("BUILD")
                                           or s.startswith("THE C") else 0)), c)
        y += 18


def sc_species(cv, t, d):
    std = [0, 1, 2, 3, 4, 6]
    if t < 13:
        cv.ctext(6, "CHOOSE A SPECIES", YELLOW, 1, 2)
        for i, sid in enumerate(std):
            r, c = divmod(i, 3)
            cx = 40 + c * 80
            y = 34 + r * 86
            t0 = 0.3 + i * 0.35
            tx = cx - 8
            x = int(W + 10 - (t - t0) * 160)
            col = PLAYER[i % 4]
            if t < t0:
                continue
            if x > tx:
                draw_species(cv, x, y, sid, walk_frame(t, 2), col, 2)
            else:
                draw_species(cv, tx, y, sid, 4 + int(t * 3 + i) % 4, col, 2)
                cv.ctext(y + 68, SPECIES[sid], PTEXT[i % 4], cx=cx)
        if t > 3.4:
            caption(cv, t - 3.4, "THESE SIX START WITH $1000 AND A NORMAL TURN.", y=212, c=WHITE)
        return
    # spotlights: Flapper then Humanoid
    t2 = t - 13
    sid, col, lines = (5, GREEN, [("FLAPPER", GREEN), ("BEGINNER", WHITE), ("$1600", YELLOW),
                                  ("+30% TIME", CYAN)]) if t2 < 5.5 else \
        (7, ORANGE, [("HUMANOID", ORANGE), ("EXPERT", WHITE), ("$600", YELLOW), ("-30% TIME", PINK)])
    tt = t2 if t2 < 5.5 else t2 - 5.5
    x = int(max(28, W + 10 - tt * 220))
    f = walk_frame(tt, 2) if x > 28 else 4 + int(tt * 6) % 4
    draw_species(cv, x, 40, sid, f, col, 4)
    for i, (s, c) in enumerate(lines):
        if tt > 0.8 + i * 0.4:
            cv.text(96, 48 + i * 36, s, c, 2 if i == 0 else 1, 4 if i == 0 else 2)
    if t2 >= 8.0:
        caption(cv, t2 - 7.5, "COMPUTER PLAYERS ARE MECHTRONS WITH $1200.", y=212, c=TAN)
    elif t2 < 5.5 and tt > 2.2:
        caption(cv, tt - 2.2, "MORE MONEY AND MORE TIME EVERY TURN.", y=212, c=WHITE)
    elif t2 >= 5.5 and tt > 2.2:
        caption(cv, tt - 2.2, "LESS MONEY, LESS TIME. FOR EXPERTS!", y=212, c=WHITE)


def sc_controls(cv, t, d):
    cv.ctext(14, "CONTROLS", YELLOW, 2, 4)
    rows = [("DISC", "WALK, BID, MOVE"), ("SIDE BUTTON", "CLAIM, INSTALL"),
            ("ENTER", "END YOUR TURN"), ("KEYPAD 0", "SIT OUT AN AUCTION")]
    for i, (k, v) in enumerate(rows):
        if t > 0.3 + i * 0.5:
            cv.text(16, 70 + i * 40, k, CYAN)
            cv.text(16, 88 + i * 40, "  " + v, WHITE)
    # a little colonist working the disc
    draw_species(cv, 196, 80, 3, 4 + int(t * 4) % 4, BLUE, 3)


MONTH = [("STATUS SUMMARY", "GFX_MAPSP", 3 * 10 + 4, BLUE, 2),
         ("LAND GRANT", "GFX_CURSOR", 0, BLACK, 2),
         ("LAND AUCTION", "GFX_FLAG", 0, WHITE, 2),
         ("DEVELOPMENT", "GFX_MULE", 0, WHITE, 2),
         ("PRODUCTION", "GFX_MAP", MC_BL + 5, GREEN, 2),
         ("COLONY EVENT", "GFX_METEOR", 0, ORANGE, 2),
         ("GOODS AUCTIONS", "GFX_STOREICON", 0, YELLOW, 2)]


def sc_month(cv, t, d):
    cv.ctext(6, "EVERY MONTH", YELLOW, 2, 4)
    cur = min(len(MONTH) - 1, int(t / 1.2))
    for i, (s, blk, idx, c, sy) in enumerate(MONTH):
        if t < i * 1.2:
            break
        y = 48 + i * 24
        cv.text(28, y, "%d" % (i + 1), GREY)
        bc = c if c != BLACK else WHITE
        if blk == "GFX_CURSOR":
            cv.bits(52, y - 4, card(blk, 0) + card(blk, 1), bc, sx=1, sy=1)
        else:
            cv.bits(52, y, card(blk, idx), bc, sx=1, sy=sy)
        cv.text(72, y, s, YELLOW if i == cur and t < 8.8 else WHITE)
    if t > 8.8:
        caption(cv, t - 7.6, "12 MONTHS, THEN THE SHIP RETURNS.", y=226, c=CYAN)


def sc_map(cv, t, d):
    ox, oy = 40, 4
    draw_map(cv, ox, oy, t=t, blink_plot=None)
    # point out each terrain in turn with the game's white outline
    if 1.2 < t < 2.6:
        draw_plot(cv, ox, oy, 11, white=blink(t, 3))
        caption(cv, 99, "PLAINS", y=206, c=WHITE)
    elif 2.6 <= t < 4.0:
        draw_plot(cv, ox, oy, 31, white=blink(t, 3), terr=MAP[31])
        caption(cv, 99, "THE RIVER", y=206, c=BLUE)
    elif 4.0 <= t:
        for mp in (0, 14):
            draw_plot(cv, ox, oy, mp, white=blink(t, 3))
        caption(cv, 99, "MOUNTAINS: 1, 2 OR 3 PEAKS", y=206, c=WHITE)
    if t <= 1.2:
        caption(cv, t, "THE COLONY: 45 PLOTS", y=206, c=YELLOW)
    caption(cv, max(0, t - 0.6), "THE STORE IS IN THE MIDDLE.", y=232, c=TAN)


TERRAIN_ROWS = [("PLAINS", ("plain", 0), (2, 3, 1)), ("RIVER", ("river", 0), (4, 2, None)),
                ("1 PEAK", ("mount", 1), (1, 1, 2)), ("2 PEAKS", ("mount", 2), (1, 1, 3)),
                ("3 PEAKS", ("mount", 3), (1, 1, 4))]


def sc_terrain(cv, t, d):
    cv.text(8, 16, "WHAT LAND MAKES", YELLOW)
    cols = [144, 180, 216]
    for j in range(3):
        good_icon(cv, cols[j], 12, j, GOOD_COL[j])
    for i, (name, terr, yields) in enumerate(TERRAIN_ROWS):
        if t < 0.3 + i * 0.7:
            break
        y = 44 + i * 34
        cv.rect(8, y, 16, 32, GREY)
        if terr[0] == "river":
            cv.bits(12, y, cards("GFX_MAP", MC_RIVER, 2)[:8], BLUE, sx=1, sy=4)
        else:
            draw_plot_at(cv, 8, y, terr)
        cv.text(32, y + 8, name, WHITE)
        for j, v in enumerate(yields):
            cv.text(cols[j], y + 8, "-" if v is None else str(v), GOOD_COL[j])
    if t > 4.2:
        caption(cv, t - 4.2, "CRYSTITE IS HIDDEN. THE ASSAY OFFICE FINDS IT.", y=222, c=CYAN)


def grant_state(t):
    """The cursor steps plot by plot every 0.2 s; two players claim."""
    owners = {}
    claims = [(14, 0, 2.1), (19, 1, 3.4), (30, 2, 4.5)]
    step = 0.2
    pos = 0
    tt = 0.0
    while tt + step <= t:
        tt += step
        pos += 1
        while pos == 22 or pos in [p for p, s, ct in claims if ct <= tt]:
            pos += 1
        for p, s, ct in claims:
            if ct <= tt:
                owners[p] = (s, None, 0)
    return pos % 45, owners


def sc_grant(cv, t, d):
    ox, oy = 40, 4
    pos, owners = grant_state(t)
    draw_map(cv, ox, oy, owners)
    x = ox + 8 + (pos % 9) * 16
    y = oy + 16 + (pos // 9) * 32
    cur = card("GFX_CURSOR", 0) + card("GFX_CURSOR", 1)
    cv.bits(x, y, cur, BLACK, sx=2, sy=2)
    caption(cv, t, "LAND GRANT: PRESS THE BUTTON TO CLAIM A FREE PLOT.", y=204)


def sc_bid(cv, t, d):
    show_clip(cv, "landbid", t, 0)
    caption(cv, t, "LAND AUCTION: PUSH UP TO BID. HIGHEST BID WINS THE PLOT.", y=204)


RESOURCES = [
    "MORE FOOD MEANS MORE TIME IN YOUR TURN. HALF OF ANY EXTRA SPOILS.",
    "EVERY M.U.L.E. NOT MAKING ENERGY USES 1 A MONTH. RUN SHORT AND PLOTS MAKE NOTHING.",
    "THE STORE BUILDS M.U.L.E.S FROM SMITHORE. NO SMITHORE, NO M.U.L.E.S!",
    "ONLY WORTH MONEY, AND ITS PRICE IS RANDOM. THE BIG GAMBLE.",
]


def sc_resources(cv, t, d):
    k = min(3, int(t / 4.5))
    tt = t - k * 4.5
    cv.ctext(4, "FOUR GOODS", YELLOW)
    for j in range(4):
        c = GOOD_COL[j] if j == k else DKGREEN
        x = 24 + j * 54
        good_icon(cv, x, 22, j, c, 2)
        if j == k:
            cv.frame(x - 4, 18, 24, 72, WHITE)
    cv.ctext(100, GOOD_NAME[k], GOOD_COL[k], 2, 4)
    # a plot whose M.U.L.E. makes this good, its dots counting up
    draw_plot_at(cv, 12, 150, ("plain", 0), 0, k, 1 + int(tt * 2) % 8, s=2)
    lines = wrap(RESOURCES[k], 22)
    done = 0
    for i, ln in enumerate(lines):
        cv.text(56, 150 + i * 20, typed(ln, tt - 0.3 - done / 45, 45), WHITE)
        done += len(ln)


TOWN_TOUR = [(0.0, ["CORRAL"], "CORRAL: BUY A M.U.L.E."),
             (2.8, ["FOOD", "ENERGY", "SMITHORE", "CRYSTITE"],
              "OUTFIT IT: FOOD $25, ENERGY $50, SMITHORE $75, CRYSTITE $100."),
             (6.8, ["ASSAY"], "ASSAY OFFICE: TEST A PLOT FOR CRYSTITE."),
             (8.9, ["LAND"], "LAND OFFICE: PUT A PLOT UP FOR SALE."),
             (10.7, ["PUB"], "PUB: GAMBLE YOUR TIME LEFT FOR CASH. ENDS YOUR TURN.")]


def sc_town(cv, t, d):
    cv.ctext(4, "THE TOWN", YELLOW)
    ox, oy = 72, 26
    draw_town(cv, ox, oy)
    # our colonist in the street
    draw_species(cv, ox + 44, oy + 40, 3, 4, BLUE, 1)
    cur = [s for s in TOWN_TOUR if s[0] <= t][-1]
    for b in cur[1]:
        bx, by, bw, bh = TOWN_RECT[b]
        if blink(t, 3):
            cv.frame(ox + bx * 8 - 1, oy + by * 16 - 2, bw * 8 + 2, bh * 16 + 4, WHITE)
    caption(cv, t - cur[0], cur[2], y=184, step=20)


def sc_develop(cv, t, d):
    i = show_clip(cv, "develop", t, 0)
    if i < 52:
        s = "1. BUY A M.U.L.E. AT THE CORRAL."
    elif i < 100:
        s = "2. TAKE IT INTO AN OUTFITTER."
    elif i < 138:
        s = "3. LEAD IT OUT TO YOUR PLOT."
    else:
        s = "4. PRESS THE BUTTON TO INSTALL IT."
    caption(cv, 99, s, y=204, c=YELLOW)
    caption(cv, 99, "WATCH THE TIME BAR!", y=248, c=BLUE)


def sc_runaway(cv, t, d):
    cv.rect(0, 0, W, H, GREY)
    # a plot of someone else's, and our M.U.L.E. bolting from it
    draw_plot_at(cv, 24, 40, ("plain", 0), 1, None, 0, s=3)
    mx = int(80 + t * 60)
    draw_mule(cv, mx, 80, walk_frame(t, 0, 10), BLACK, 3)
    draw_mapsp(cv, 40, 112, 3, 4, BLUE, 3)
    if blink(t, 4):
        cv.text(mx + 28, 52, "!", RED, 2, 4)
    cv.rect(0, 160, W, 110, BLACK)
    caption(cv, t, "WRONG PLOT OR NO OUTFIT AND YOUR M.U.L.E. RUNS AWAY!", y=176, c=WHITE)


def sc_wampus(cv, t, d):
    if t < 5:
        cv.rect(0, 0, W, 160, GREY)
        for (x, y, m) in [(40, 40, 3), (90, 70, 2), (150, 30, 3), (190, 90, 1), (110, 110, 2)]:
            cv.bits(x, y, card("GFX_MAP", MC_TLU + m - 1), BLACK, sx=2, sy=4)
        # the wampus shows itself on one mountain, then another
        spots = [(156, 34), (46, 44), (116, 112)]
        sx, sy = spots[int(t / 1.4) % 3]
        if (t % 1.4) < 0.9:
            cv.bits(sx, sy, card("GFX_WAMPUS", 0), YELLOW if blink(t, 4) else WHITE, sx=2, sy=4)
        draw_mapsp(cv, int(10 + t * 12), 120, 3, walk_frame(t, 0), BLUE, 2)
        caption(cv, t, "CATCH THE WAMPUS IN THE MOUNTAINS FOR A CASH BOUNTY.", y=176, c=YELLOW)
        cv.ctext(236, "(NOT WHILE LEADING A M.U.L.E.)", TAN)
    else:
        tt = t - 5
        cv.ctext(8, "THE PUB", YELLOW, 2, 4)
        pub = TOWN_RECT["PUB"]
        # draw the pub larger: just its cards, scaled 3x
        cv.rect(0, 36, W, 120, BLACK)
        tmp = Canvas()
        draw_town(tmp, 0, 0, 1, only=pub)
        sub = tmp.a[pub[1] * 16:(pub[1] + pub[3]) * 16, pub[0] * 8:(pub[0] + pub[2]) * 8]
        big = np.repeat(np.repeat(sub, 2, axis=0), 2, axis=1)
        cv.paste(W // 2 - big.shape[1] // 2, 48, big)
        draw_species(cv, W // 2 + 34, 70, 3, 8 + int(tt * 4) % 3, BLUE, 2)
        caption(cv, tt, "DONE EARLY? THE PUB PAYS CASH. THE MORE TIME LEFT, THE MORE YOU WIN.",
                y=176, c=WHITE)


def sc_pevent(cv, t, d):
    show_clip(cv, "pevent", t, 0)
    caption(cv, t, "EACH TURN MAY BRING GOOD OR BAD LUCK. LEADERS NEVER GET GOOD LUCK.", y=204)


def sc_production(cv, t, d):
    if t < 3:
        show_clip(cv, "production", t, 120)
        caption(cv, t, "PRODUCTION: DOTS SHOW WHAT EACH PLOT MADE.", y=204)
        return
    tt = t - 3
    cv.ctext(8, "BONUSES", YELLOW, 2, 4)
    cv.rect(24, 48, 192, 118, GREY)
    # three blue food plots side by side, dots climbing
    n = 2 + (1 if tt > 1.2 else 0) + (1 if tt > 2.4 else 0)
    for k in range(3):
        if k == 0 or tt > 0.6 * k:
            draw_plot_at(cv, 72 + k * 32, 56, ("plain", 0), 0, 0, n, s=2)
    if tt > 1.2:
        cv.text(40, 128, "+1 NEIGHBOR", GREEN)
    if tt > 2.4:
        cv.text(40, 146, "+1 PER 3 PLOTS", GREEN)
    caption(cv, tt, "SAME GOOD NEXT DOOR, OR 3 PLOTS OF IT, MAKES MORE.", y=176)


EVENTS = [("ev_pirates", 40), ("ev_acid", 0), ("ev_meteor", 6), ("ev_fire", 24), ("ev_quake", 0)]


def sc_events(cv, t, d):
    k = min(len(EVENTS) - 1, int(t / 2))
    name, start = EVENTS[k]
    show_clip(cv, name, t - k * 2, start)
    caption(cv, 99, "COLONY EVENTS HIT EVERYONE.", y=204, c=YELLOW)
    caption(cv, 99, "SOME HELP, SOME HURT.", y=226, c=WHITE)


def sc_goods(cv, t, d):
    show_clip(cv, "goods", t, 80)
    caption(cv, t, "AUCTIONS: SMITHORE, CRYSTITE, FOOD, THEN ENERGY.", y=204)


def sc_declare(cv, t, d):
    show_clip(cv, "declare", t, 30)
    caption(cv, t, "PUSH UP TO SELL, DOWN TO BUY. KEEP ENOUGH FOOD AND ENERGY!", y=204)


def sc_trading(cv, t, d):
    show_clip(cv, "trading", t, 20)
    if t < 4:
        caption(cv, t, "BUYERS AND SELLERS WALK TOWARD A PRICE. WHERE THEY MEET, THEY TRADE.", y=204)
    else:
        caption(cv, t - 4, "THE STORE BUYS AT THE BOTTOM AND SELLS AT THE TOP.", y=204)


def sc_win(cv, t, d):
    if t < 5:
        show_clip(cv, "summary", t, 0)
        caption(cv, t, "AFTER 12 MONTHS THE TOP SCORE IS FIRST FOUNDER.", y=204, c=YELLOW)
    else:
        show_clip(cv, "gameover", t - 5, 0)
        caption(cv, t - 5, "BUT RUN OUT OF FOOD OR ENERGY AND THE COLONY FAILS. EVERYONE LOSES.",
                y=204, c=PINK)


def sc_outro(cv, t, d):
    stars(cv, t, seed=11)
    cv.ctext(8, "READY TO PLAY?", YELLOW, 2, 4)
    if t > 0.8:
        cv.ctext(56, typed("ON YOUR FUJINET, MOUNT", t - 0.8), WHITE)
    if t > 1.6:
        cv.ctext(80, "TNFS HOST", GREY)
        cv.ctext(98, typed("APPS.IRATA.ONLINE", t - 1.6, 25), CYAN, 1, 2)
    if t > 2.8:
        cv.ctext(124, "FOLDER", GREY)
        cv.ctext(142, typed("MULE", t - 2.8, 20), CYAN, 2, 4)
    if t > 4.0:
        cv.ctext(170, typed("FROM ANY FUJINET MACHINE", t - 4.0), WHITE)
    if t > 5.0:
        cv.ctext(190, typed("AND JOIN A GAME!", t - 5.0), YELLOW)
    # the cast walks along the bottom
    for k in range(4):
        x = int(-30 - k * 30 + t * 40)
        draw_mapsp(cv, x, 222, 1 + k * 2, walk_frame(t + k * 0.1, 0), PLAYER[k], 3)
    draw_mule(cv, int(-160 + t * 40), 222, walk_frame(t, 0, 6), WHITE, 2)


# ---------------------------------------------------------------------------
# Cross-platform play: real screenshots of the other FujiNet clients
# (screenshots/<target>/), drawn as overlays at video resolution.
# ---------------------------------------------------------------------------
SHOTS = os.path.join(HERE, "..", "..", "screenshots")
PLATFORMS = [("intv", "INTELLIVISION"), ("atari", "ATARI 8-BIT"), ("atari-2600", "ATARI 2600"),
             ("apple2", "APPLE II"), ("coco", "COCO"), ("msdos-cga", "IBM PC/PCJR"),
             ("msx", "MSX"), ("adam", "COLECO ADAM"), ("lynx", "LYNX"), ("astrocade", "ASTROCADE")]
_SHOT = {}


def shot(target, name, bw, bh):
    """A screenshot fitted to a bw x bh box: a TV's 4:3, or the Lynx's own
    shape, scaled with nearest neighbour."""
    key = (target, name, bw, bh)
    if key not in _SHOT:
        im = Image.open(os.path.join(SHOTS, target, name + ".png")).convert("RGB")
        aspect = im.width / im.height if target == "lynx" else 4 / 3
        w, h = bw, int(bw / aspect)
        if h > bh:
            w, h = int(bh * aspect), bh
        _SHOT[key] = np.array(im.resize((w, h), Image.NEAREST))
    return _SHOT[key]


def tile(cv, target, name, label, x, y, bw, bh, pop=1.0, lc=WHITE):
    """A labelled screenshot centred in its box; pop 0..1 grows it in."""
    if pop <= 0:
        return
    img = shot(target, name, bw, bh)
    if pop < 1.0:
        f = max(0.05, pop)
        im = Image.fromarray(img)
        img = np.array(im.resize((max(1, int(im.width * f)), max(1, int(im.height * f))), Image.NEAREST))
    h, w = img.shape[:2]
    cx, cy = x + bw // 2, y + bh // 2
    border = np.zeros((h + 8, w + 8, 3), dtype=np.uint8)
    border[:] = PAL[DKGREEN]
    border[4:-4, 4:-4] = img
    cv.over(cx - w // 2 - 4, cy - h // 2 - 4, border)
    if pop >= 1.0:
        cv.otext(cx, y + bh + 12, label, lc)


def sc_online(cv, t, d):
    cv.ctext(4, "PLAY ONLINE!", YELLOW, 2, 4)
    caption(cv, t - 2.6, "UP TO 4 PLAYERS, EACH ON THEIR OWN MACHINE.", y=226, c=WHITE)
    four = [PLATFORMS[0], PLATFORMS[1], PLATFORMS[3], PLATFORMS[6]]
    bw, bh = 400, 280
    for i, (tg, label) in enumerate(four):
        r, c = divmod(i, 2)
        x = 960 - bw - 30 + c * (bw + 60)
        y = 160 + r * (bh + 80)
        tile(cv, tg, "03-summary", label, x, y, bw, bh, pop=min(1.0, (t - 0.8 - i * 0.5) / 0.25),
             lc=PTEXT[i])


def grid10(cv, t, name, t0=0.0):
    bw, bh = 320, 240
    for i, (tg, label) in enumerate(PLATFORMS):
        r, c = divmod(i, 5)
        x = 60 + c * (bw + 40)
        y = 170 + r * (bh + 90)
        tile(cv, tg, name, label, x, y, bw, bh, pop=min(1.0, (t - t0 - i * 0.3) / 0.2),
             lc=PTEXT[i % 4])


def sc_crossplay(cv, t, d):
    if t < 6:
        cv.ctext(6, "ONE COLONY", YELLOW, 2, 4)
        grid10(cv, t, "15-trading", 0.4)
        caption(cv, t - 3.6, "MANY MACHINES, ONE GAME.", y=232, c=WHITE)
        return
    if t < 12:
        tt = t - 6
        # the server in the middle, four machines in the corners
        cx, cy = 120, 120
        corners = [(8, 16, "INTELLIVISION", 0, 3), (128, 16, "ATARI 8-BIT", 1, 1),
                   (8, 184, "APPLE II", 2, 5), (128, 184, "COCO", 3, 6)]
        for i, (x, y, name, seat, sid) in enumerate(corners):
            if tt < 0.3 + i * 0.4:
                continue
            ax = x + 52
            ay = y + 16
            # dotted link to the server, packets running both ways
            n = 24
            for k in range(n):
                if k % 2 == 0:
                    px = int(ax + (cx - ax) * k / n)
                    py = int(ay + (cy - ay) * k / n)
                    cv.rect(px, py, 1, 2, GREY)
            for ph in (0.0, 0.5):
                f = (tt * 0.8 + ph + i * 0.17) % 1.0
                if ph:
                    f = 1 - f
                px = int(ax + (cx - ax) * f)
                py = int(ay + (cy - ay) * f)
                cv.rect(px - 1, py - 2, 3, 4, PLAYER[seat])
            cv.text(x + 52 - len(name) * 4, y, name, PTEXT[seat])
            draw_mapsp(cv, x + 46, y + 18 if y < 100 else y - 30, sid, 4 + int(tt * 3) % 2,
                       PLAYER[seat], 2)
        cv.rect(cx - 36, cy - 28, 72, 58, BLACK)
        cv.frame(cx - 36, cy - 28, 72, 58, CYAN)
        cv.bits(cx - 4, cy - 24, card("GFX_STOREICON", 0), YELLOW, 1, 2)
        cv.ctext(cy - 4, "IRATA", CYAN)
        cv.ctext(cy + 12, ".ONLINE", CYAN)
        caption(cv, tt - 2.6, "THE COMPUTER TAKES ANY EMPTY SEAT.", y=226, c=WHITE)
        return
    tt = t - 12
    cv.ctext(6, "EVERY MACHINE", YELLOW, 2, 4)
    grid10(cv, tt + 10, "07-land-auction-bidding")
    caption(cv, tt - 0.3, "SAME RULES. SAME TIMING.", y=232, c=WHITE)


SCENES = [
    (9, sc_title), (13, sc_premise), (6, sc_online), (24, sc_species), (6, sc_controls),
    (12, sc_month), (8, sc_map), (10, sc_terrain), (8, sc_grant), (7, sc_bid),
    (18, sc_resources), (13, sc_town), (9, sc_develop), (5, sc_runaway), (10, sc_wampus),
    (7, sc_pevent), (8, sc_production), (10, sc_events), (5, sc_goods), (6, sc_declare),
    (10, sc_trading), (10, sc_win), (16, sc_crossplay), (10, sc_outro),
]
TOTAL = sum(d for d, _ in SCENES)
FADE = 0.12


def frame_at(T):
    t0 = 0.0
    for d, fn in SCENES:
        if T < t0 + d or fn is SCENES[-1][1]:
            t = T - t0
            cv = Canvas()
            fn(cv, t, d)
            # a quick dip to black at each cut (the Atari blanks too); the
            # last scene fades out over its final second
            out = 1.0 if fn is SCENES[-1][1] else FADE
            k = min(1.0, t / FADE, (d - t) / out)
            if k < 1.0:
                cv.dim(max(0.0, k))
            return cv
        t0 += d


def main():
    global BUILD
    BUILD = os.path.abspath(sys.argv[1])
    if "--still" in sys.argv:
        for ts in sys.argv[sys.argv.index("--still") + 1:]:
            T = float(ts)
            Image.fromarray(frame_at(T).video()).save(os.path.join(BUILD, "still_%06.2f.png" % T))
        return
    out = sys.argv[2]
    n = int(round(TOTAL * FPS))
    print("%d scenes, %.1f s, %d frames" % (len(SCENES), TOTAL, n))
    ff = subprocess.Popen(
        ["ffmpeg", "-v", "error", "-y", "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", "%dx%d" % (W * SX, H * SY),
         "-r", str(FPS), "-i", "-", "-i", os.path.join(BUILD, "music.wav"),
         "-map", "0:v", "-map", "1:a", "-c:v", "libx264", "-preset", "slow", "-crf", "16",
         "-tune", "animation", "-pix_fmt", "yuv420p", "-c:a", "aac", "-b:a", "192k",
         "-movflags", "+faststart", "-shortest", out], stdin=subprocess.PIPE)
    for f in range(n):
        ff.stdin.write(frame_at(f / FPS).video().tobytes())
        if f % 600 == 0:
            print("  %3d s" % (f // FPS), flush=True)
    ff.stdin.close()
    ff.wait()
    print("wrote", out)


if __name__ == "__main__":
    BUILD = os.path.join(HERE, "..", "..", "videos", "build")
    main()

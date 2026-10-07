"""The intro video's scenes for the MekkoGX clients (Atari 8-bit, Apple II).

The clients share their game code, test mode and test map, so one storyboard
serves them all; a platform's render script calls configure() with its
colours, playfield geometry, words and drawing hooks, then run():

  colours    BLACK WHITE BLUE PURPLE GREEN ORANGE RED YELLOW GREY CYAN TAN STAR_TINT
             DKGREEN PINK MAPBG DIM ACC NUMCOL DOTCOL LABELCOL FRAMECOL, PLAYER
             PTEXT GOOD_COL, lit(c)
  playfield  PFX PFY (where a clip goes), PFW PFH, CW (a plot's width; 32
             lines tall), plot_xy(p), load_frame(path) -> RGB, TOWN_BOX
             (y0, y1, x0, x1 in a still), TCW (a town cell's width)
  sprites    frame(name, k) -> rows, shape(cv, name, k, x, y, c, s, flip, wide),
             ship(cv, x, y, kind, s), pic(cv, x, y, k, c, s),
             cursor(cv, x, y) (the land grant cursor on a plot),
             cursor_icon(cv, x, y)
  words      TITLE (big, small), BUTTON, CONTROLS [(key, what)], CONTROLS_NOTE,
             OUTRO_IMAGE, PLATFORM (screenshots/ folder), CORNERS (4 names)
  clips      MAPSTILL, TOWN_STILL, PUB_STILL, START {clip: first image}
"""
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import vidlib  # noqa: E402
from vidlib import Canvas, wrap, typed, caption, blink  # noqa: E402,F401

W, H, SX, SY = 480, 270, 4, 4
GOOD_NAME = ["FOOD", "ENERGY", "SMITHORE", "CRYSTITE"]
SPECIES = ["MECHTRON", "GOLLUMER", "PACKER", "BONZOID", "SPHEROID", "FLAPPER", "LEGGITE", "HUMANOID"]
BUILD = None


# Defaults (the Atari's), which configure() may override: the text scale
# (a font cell at TS, headings at twice it) and caption line spacing, the test
# map's plots each scene points at, and the land grant's claims.
TS, STEP = (2, 2), 18
TERRAIN_PLOTS = (2, 4, 25, 5, 15)          # plains, river, 1, 2 and 3 peaks
ICON_PLOT = [10, 12, 20, 34]               # the test map's food, energy, smithore, crystite
OWNED = {10, 12, 14, 20, 30, 34, 22}
CLAIMS = [(19, 0, 2.1), (28, 1, 3.4), (33, 2, 4.5)]
ship = None
IMG_FRAMES = 2                             # game frames between a clip's images
PH, TRH = 32, 16                           # a plot's lines, a town row's lines


def img(n):
    """An image number written for 2 game frames an image, in this clip's."""
    return n if IMG_FRAMES == 2 else int(round(n * 2 / IMG_FRAMES))


def configure(**kw):
    globals().update(kw)


def walk_frame(t, base, rate=8):
    return base + int(t * rate) % 2


# ---------------------------------------------------------------------------
# Text: the font at 2 x 2 (ANTIC mode 7's size), 4 x 4 for headings
# ---------------------------------------------------------------------------
def T(cv, x, y, s, c, z=1):
    cv.text(x, y, s, c, TS[0] * z, TS[1] * z)


def CT(cv, y, s, c, z=1, cx=None):
    cv.ctext(y, s, c, TS[0] * z, TS[1] * z, cx)


def cap(cv, t, s, y=202, c=None, step=None):
    caption(cv, t, s, y=y, c=c, step=STEP if step is None else step, sx=TS[0], sy=TS[1])


# ---------------------------------------------------------------------------
# Sprites (players and missiles): a bit is a colour clock, 2 units wide
# ---------------------------------------------------------------------------
def species(cv, x, y, sid, f, c, s=1):
    shape(cv, "species", sid * 11 + f, x, y, c, s)


def figure(cv, x, y, sid, f, c, s=1):
    shape(cv, "figure", sid * 10 + f, x, y, c, s)


def mule(cv, x, y, f, c, s=1, town=True):
    shape(cv, "mule", (0 if town else 2) + f, x, y, c, s)


# ---------------------------------------------------------------------------
# Clips and stills: snapshots of the client in libatari800 (.scr: 384 x 240
# colour indices; the playfield is rows 24-215, columns 32-351)
# ---------------------------------------------------------------------------
CLIPS = {}


def clip(name):
    if name not in CLIPS:
        CLIPS[name] = vidlib.Clip(os.path.join(BUILD, "clips", name), CLIP_GLOB, load_frame)
    return CLIPS[name]


def show_clip(cv, name, t, start=0, x=None, y=None, speed=1.0):
    x = PFX if x is None else x
    y = PFY if y is None else y
    start = img(START.get(name, start))
    i = start + (int(t * 30 * speed) if IMG_FRAMES == 2 else int(t * 60 / IMG_FRAMES * speed))
    cv.frame(x - 3, y - 2, PFW + 6, PFH + 4, DKGREEN, 1)
    cv.paste(x, y, clip(name).get(i))
    return i


def still(name, i):
    return clip(name).get(img(i))


def plot_cell(p, src=None):
    """A plot from a still, 32 lines tall whatever the machine draws."""
    x, y = plot_xy(p)
    c = still(*(src or MAPSTILL))[y:y + PH, x:x + CW]
    if PH != 32:
        c = c[(np.arange(32) * PH) // 32]
    return c


def big(a, k):
    return np.repeat(np.repeat(a, k, axis=0), k, axis=1)


def outline(cv, x, y, w, h, c, t=2):
    cv.frame(x, y, w, h, c, 1)
    cv.frame(x + 1, y + 1, w - 2, h - 2, c, 1)


# ---------------------------------------------------------------------------
# Scenes
# ---------------------------------------------------------------------------
def stars(cv, t, n=90, seed=7, speed=10.0):
    vidlib.stars(cv, t, (WHITE, DIM, STAR_TINT), n, seed, speed, h=1)


def sc_title(cv, t, d):
    stars(cv, t)
    sx = int(560 - t * 150)
    if sx > -160 and ship:
        ship(cv, sx, 66, 0, 2)
    if t > 2.6:
        word = "M.U.L.E."
        n = min(len(word), int((t - 2.6) / 0.12) + 1)
        x0 = W // 2 - len(word) * 24
        for i in range(n):
            drop = max(0, 1 - (t - 2.6 - i * 0.12) / 0.25)
            T(cv, x0 + i * 48, 36 - int(drop * 40), word[i], YELLOW, 3)
    if t > 4.2:
        CT(cv, 104, TITLE[0], ACC, 2)
        CT(cv, 140, TITLE[1], ACC)
    if t > 5.2:
        CT(cv, 166, typed("A GUIDE FOR NEW COLONISTS", t - 5.2, 30), WHITE)
    if t > 4.5:
        mule(cv, int(-40 + (t - 4.5) * 140), 196, walk_frame(t, 0, 6), WHITE, 2)
    cv.rect(0, 246, W, 1, DIM)


def sc_premise(cv, t, d):
    cv.rect(0, 166, W, 104, MAPBG)
    for x, y, p in [(24, 178, 5), (120, 222, 25), (412, 172, 15), (352, 226, 5), (200, 238, 25)]:
        cv.paste(x, y, plot_cell(mp(p))[:28])
    for k in range(4):
        sid = 1 + k * 2
        x = int(-60 - k * 68 + t * 68)
        x = min(x, 320 - k * 72)
        f = walk_frame(t + k * 0.13, 0) if x < 320 - k * 72 else 4 + int(t * 2) % 2
        figure(cv, x, 196, sid, f, PLAYER[k], 3)
    lines = [(0.0, "FOUR COLONISTS LAND ON", WHITE), (0.0, "THE PLANET IRATA.", WHITE),
             (2.6, "YOU HAVE 12 MONTHS TO", WHITE), (2.6, "BUILD A COLONY.", WHITE),
             (5.2, "SCORE = MONEY+LAND+GOODS", YELLOW),
             (7.4, "THE RICHEST WINS, BUT", ACC), (7.4, "THE COLONY MUST SURVIVE.", ACC)]
    y, prev = 10, None
    for t0, s, c in lines:
        if prev is not None and t0 != prev:
            y += 6
        prev = t0
        if t >= t0:
            late = 0.5 if s.startswith(("THE P", "BUILD", "THE C")) else 0
            CT(cv, y, typed(s, t - t0 - late), c)
        y += 18


PLATS = None


def plats():
    global PLATS
    if PLATS is None:
        PLATS = vidlib.platforms(PLATFORM)
    return PLATS


def sc_online(cv, t, d):
    CT(cv, 4, "PLAY ONLINE!", YELLOW, 2)
    cap(cv, t - 2.6, "UP TO 4 PLAYERS, EACH ON THEIR OWN MACHINE.", y=226)
    p = plats()
    four = [p[0]] + [q for q in p[1:] if q[0] in ONLINE_FOUR][:3]
    vidlib.grid_online(cv, t, four, PTEXT, border=tuple(PAL[DKGREEN]))


def sc_species(cv, t, d):
    std = [0, 1, 2, 3, 4, 6]
    if t < 13:
        CT(cv, 6, "CHOOSE A SPECIES", YELLOW)
        for i, sid in enumerate(std):
            r, c = divmod(i, 3)
            cx = 80 + c * 160
            y = 30 + r * 86
            t0 = 0.3 + i * 0.35
            tx = cx - 24
            x = int(W + 20 - (t - t0) * 320)
            if t < t0:
                continue
            if x > tx:
                species(cv, x, y, sid, walk_frame(t, 2), PLAYER[i % 4], 3)
            else:
                species(cv, tx, y, sid, 4 + int(t * 3 + i) % 4, PLAYER[i % 4], 3)
                CT(cv, y + 70, SPECIES[sid], PTEXT[i % 4], cx=cx)
        if t > 3.4:
            cap(cv, t - 3.4, "THESE SIX START WITH $1000 AND A NORMAL TURN.", y=212)
        return
    t2 = t - 13
    sid, col, lines = (5, GREEN, [("FLAPPER", PTEXT[2]), ("BEGINNER", WHITE), ("$1600", YELLOW),
                                  ("+30% TIME", ACC)]) if t2 < 5.5 else \
        (7, ORANGE, [("HUMANOID", PTEXT[3]), ("EXPERT", WHITE), ("$600", YELLOW), ("-30% TIME", PINK)])
    tt = t2 if t2 < 5.5 else t2 - 5.5
    x = int(max(56, W + 20 - tt * 440))
    f = walk_frame(tt, 2) if x > 56 else 4 + int(tt * 6) % 4
    species(cv, x, 40, sid, f, col, 6)
    for i, (s, c) in enumerate(lines):
        if tt > 0.8 + i * 0.4:
            T(cv, 192, 48 + i * 36, s, c, 2 if i == 0 else 1)
    if t2 >= 8.0:
        cap(cv, t2 - 8.0, "COMPUTER PLAYERS ARE MECHTRONS WITH $1200.", y=212, c=TAN)
    elif t2 < 5.5 and tt > 2.2:
        cap(cv, tt - 2.2, "MORE MONEY AND MORE TIME EVERY TURN.", y=212)
    elif t2 >= 5.5 and tt > 2.2:
        cap(cv, tt - 2.2, "LESS MONEY, LESS TIME. FOR EXPERTS!", y=212)


def sc_controls(cv, t, d):
    CT(cv, 10, "CONTROLS", YELLOW, 2)
    rows = CONTROLS
    for i, (k, v) in enumerate(rows):
        if t > 0.3 + i * 0.5:
            T(cv, 32, 56 + i * 44, k, ACC)
            T(cv, 32, 74 + i * 44, "  " + v, WHITE)
    if t > 2.6:
        cap(cv, t - 2.6, CONTROLS_NOTE, y=238, c=TAN)
    species(cv, 392, 60, 3, 4 + int(t * 4) % 4, BLUE, 4)


MONTH = ["STATUS SUMMARY", "LAND GRANT", "LAND AUCTION", "DEVELOPMENT", "PRODUCTION",
         "COLONY EVENT", "GOODS AUCTIONS"]


def month_icon(cv, i, x, y):
    if i == 0:
        figure(cv, x, y + 2, 3, 4, BLUE, 2)
    elif i == 1:
        cursor_icon(cv, x, y)
    elif i == 2:
        pic(cv, x, y, 1, WHITE, 2)
    elif i == 3:
        mule(cv, x - 4, y, 0, WHITE, 2, town=False)
    elif i == 4:
        cv.paste(x, y - 4, plot_cell(10)[16:32, 0:16])
    elif i == 5:
        shape(cv, "meteor", 0, x, y - 4, ORANGE, 1)
    else:
        pic(cv, x, y, 0, YELLOW, 2)


def sc_month(cv, t, d):
    CT(cv, 6, "EVERY MONTH", YELLOW, 2)
    cur = min(len(MONTH) - 1, int(t / 1.2))
    for i, s in enumerate(MONTH):
        if t < i * 1.2:
            break
        y = 48 + i * 24
        T(cv, 56, y, "%d" % (i + 1), NUMCOL)
        month_icon(cv, i, 104, y)
        T(cv, 144, y, s, YELLOW if i == cur and t < 8.8 else WHITE)
    if t > 8.8:
        cap(cv, t - 8.8, "12 MONTHS, THEN THE SHIP RETURNS.", y=226, c=ACC)


def map_still(cv, src=None):
    cv.frame(PFX - 3, PFY - 2, PFW + 6, PFH + 4, DKGREEN, 1)
    cv.paste(PFX, PFY, still(*(src or MAPSTILL)))


def hl_plot(cv, p, t, c=None):
    if blink(t, 3):
        px, py = plot_xy(p)
        outline(cv, PFX + px, PFY + py, CW, PH, WHITE if c is None else c)


def sc_map(cv, t, d):
    map_still(cv)
    if 1.2 < t < 2.6:
        hl_plot(cv, mp(11), t)
        cap(cv, 99, "PLAINS", y=204)
    elif 2.6 <= t < 4.0:
        hl_plot(cv, mp(31), t)
        cap(cv, 99, "THE RIVER", y=204, c=ACC)
    elif t >= 4.0:
        for p in (25, 5, 15):
            hl_plot(cv, mp(p), t)
        cap(cv, 99, "MOUNTAINS: 1, 2 OR 3 PEAKS", y=204)
    if t <= 1.2:
        cap(cv, t, "THE COLONY: 45 PLOTS", y=204, c=YELLOW)
    cap(cv, max(0, t - 0.6), "THE STORE IS IN THE MIDDLE.", y=230, c=TAN)


TERRAIN_ROWS = [("PLAINS", (2, 3, 1)), ("RIVER", (4, 2, None)), ("1 PEAK", (1, 1, 2)),
                ("2 PEAKS", (1, 1, 3)), ("3 PEAKS", (1, 1, 4))]


GOOD_ICON = GOOD_CELL = None        # hooks for machines that draw goods otherwise
PLOT_MAP = {}                       # test-map plot -> the plot to show instead (a live map)


def mp(p):
    return PLOT_MAP.get(p, p)


def good_icon(cv, g, x, y, k=1):
    """A good's icon: the right half of an owned plot making it."""
    if GOOD_ICON:
        return GOOD_ICON(cv, g, x, y, k)
    cv.paste(x, y, big(plot_cell(ICON_PLOT[g])[2:30, CW // 2:CW - 2], k))


def sc_terrain(cv, t, d):
    T(cv, 16, 8, "WHAT LAND MAKES", YELLOW)
    cols = [300, 364, 428]
    for j in range(3):
        good_icon(cv, j, cols[j] - 4, 0)
    for i, ((name, yields), p) in enumerate(zip(TERRAIN_ROWS, TERRAIN_PLOTS)):
        if t < 0.3 + i * 0.7:
            break
        y = 32 + i * 44
        cv.paste(16, y + 1, big(plot_cell(p)[10:31], 2))
        cv.frame(15, y, 2 * CW + 2, 44, FRAMECOL, 1)
        T(cv, 96, y + 14, name, WHITE)
        for j, v in enumerate(yields):
            T(cv, cols[j], y + 14, "-" if v is None else str(v), lit(GOOD_COL[j]))
    if t > 4.2:
        cap(cv, t - 4.2, "ASSAY OFFICE: FINDS CRYSTITE", y=252, c=ACC)


def grant_state(t):
    owners = {}
    claims = CLAIMS
    step, pos, tt = 0.2, 0, 0.0
    while tt + step <= t:
        tt += step
        pos += 1
        taken = OWNED | {p for p, s, ct in claims if ct <= tt}
        while pos % 45 in taken:
            pos += 1
        for p, s, ct in claims:
            if ct <= tt:
                owners[p] = s
    return pos % 45, owners


def sc_grant(cv, t, d):
    map_still(cv)
    pos, owners = grant_state(t)
    for p, s in owners.items():
        px, py = plot_xy(p)
        outline(cv, PFX + px, PFY + py, CW, PH, PLAYER[s])
    px, py = plot_xy(pos)
    cursor(cv, PFX + px, PFY + py)
    cap(cv, t, "LAND GRANT: PRESS THE %s TO CLAIM A FREE PLOT." % BUTTON, y=202)


def sc_bid(cv, t, d):
    show_clip(cv, "landbid", t, 30)
    cap(cv, t, "LAND AUCTION: PUSH UP TO BID. HIGHEST BID WINS THE PLOT.", y=202)


RESOURCES = [
    "MORE FOOD MEANS MORE TIME IN YOUR TURN. HALF OF ANY EXTRA SPOILS.",
    "EVERY M.U.L.E. NOT MAKING ENERGY USES 1 A MONTH. RUN SHORT AND PLOTS MAKE NOTHING.",
    "THE STORE BUILDS M.U.L.E.S FROM SMITHORE. NO SMITHORE, NO M.U.L.E.S!",
    "ONLY WORTH MONEY, AND ITS PRICE IS RANDOM. THE BIG GAMBLE.",
]


def sc_resources(cv, t, d):
    k = min(3, int(t / 4.5))
    tt = t - k * 4.5
    CT(cv, 4, "FOUR GOODS", YELLOW)
    for j in range(4):
        x = 56 + j * 108
        good_icon(cv, j, x, 24, 2)
        if j != k:
            iw = (CW - 2 - CW // 2) * 2
            cv.a[24:80, x:x + iw] = (cv.a[24:80, x:x + iw] * 0.35).astype(np.uint8)
        else:
            outline(cv, x - 6, 20, 40, 64, WHITE)
    CT(cv, 96, GOOD_NAME[k], lit(GOOD_COL[k]), 2)
    if GOOD_CELL:
        GOOD_CELL(cv, k, 24, 140)
    else:
        cv.paste(24, 140, big(plot_cell(ICON_PLOT[k]), 2))
    lines = wrap(RESOURCES[k], 22)
    done = 0
    for i, ln in enumerate(lines):
        T(cv, 104, 142 + i * 20, typed(ln, tt - 0.3 - done / 45, 45), WHITE)
        done += len(ln)


# The town box in playfield units (gfx.h: x 32..127 colour clocks, y 16..159)
# and its buildings in 8-colour-clock x 16-line cells.
TOWN_RECT = {"CRYSTITE": (0, 0, 3, 3), "SMITHORE": (3, 0, 3, 3), "ENERGY": (6, 0, 3, 3),
             "FOOD": (9, 0, 3, 3), "ASSAY": (0, 4, 3, 4), "LAND": (3, 4, 2, 4),
             "PUB": (5, 4, 3, 3), "CORRAL": (8, 4, 4, 5)}
def town_img(src=None):
    y0, y1, x0, x1 = TOWN_BOX
    return still(*(src or TOWN_STILL))[y0:y1, x0:x1]


TOWN_TOUR = [(0.0, ["CORRAL"], "CORRAL: BUY A M.U.L.E."),
             (2.8, ["FOOD", "ENERGY", "SMITHORE", "CRYSTITE"],
              "OUTFIT IT: FOOD $25, ENERGY $50, SMITHORE $75, CRYSTITE $100."),
             (6.8, ["ASSAY"], "ASSAY OFFICE: TEST A PLOT FOR CRYSTITE."),
             (8.9, ["LAND"], "LAND OFFICE: PUT A PLOT UP FOR SALE."),
             (10.7, ["PUB"], "PUB: GAMBLE YOUR TIME LEFT FOR CASH. ENDS YOUR TURN.")]


def sc_town(cv, t, d):
    CT(cv, 4, "THE TOWN", YELLOW)
    oy = 24
    ti = town_img()
    ox = W // 2 - ti.shape[1] // 2
    cv.rect(ox - 8, oy - 4, ti.shape[1] + 16, ti.shape[0] + 8, MAPBG)
    cv.paste(ox, oy, ti)
    cur = [s for s in TOWN_TOUR if s[0] <= t][-1]
    for b in cur[1]:
        bx, by, bw, bh = TOWN_RECT[b]
        if blink(t, 3):
            outline(cv, ox + bx * TCW - 2, oy + by * TRH - 2, bw * TCW + 4, bh * TRH + 4, WHITE)
    cap(cv, t - cur[0], cur[2], y=184)


def sc_develop(cv, t, d):
    i = show_clip(cv, "develop", t, 0)
    if i < img(52):
        s = "1. BUY A M.U.L.E. AT THE CORRAL."
    elif i < img(100):
        s = "2. TAKE IT INTO AN OUTFITTER."
    elif i < img(138):
        s = "3. LEAD IT OUT TO YOUR PLOT."
    else:
        s = "4. PRESS THE %s TO INSTALL IT." % BUTTON
    cap(cv, 99, s, y=202, c=YELLOW)
    cap(cv, 99, "WATCH THE TIME BAR!", y=246, c=ACC)


def sc_runaway(cv, t, d):
    cv.rect(0, 0, W, 166, MAPBG)
    cv.paste(48, 40, big(plot_cell(12), 3))
    mx = int(170 + t * 120)
    mule(cv, mx, 70, walk_frame(t, 0, 10), WHITE, 4, town=False)
    figure(cv, 80, 112, 3, 4, BLUE, 4)
    if blink(t, 4):
        T(cv, mx + 72, 48, "!", lit(RED), 2)
    cap(cv, t, "WRONG PLOT OR NO OUTFIT AND YOUR M.U.L.E. RUNS AWAY!", y=180)


def sc_wampus(cv, t, d):
    if t < 5:
        cv.rect(0, 0, W, 166, MAPBG)
        for x, y, p in [(70, 36, 15), (190, 70, 5), (310, 26, 15), (380, 96, 25), (230, 116, 5)]:
            cv.paste(x, y, big(plot_cell(mp(p))[:28], 2)[:, :64])
        spots = [(330, 30), (90, 40), (250, 120)]
        sx, sy = spots[int(t / 1.4) % 3]
        if (t % 1.4) < 0.9:
            shape(cv, "wampus", 0, sx, sy, YELLOW if blink(t, 4) else WHITE, 2)
        figure(cv, int(20 + t * 24), 124, 3, walk_frame(t, 0), BLUE, 3)
        cap(cv, t, "CATCH THE WAMPUS IN THE MOUNTAINS FOR A CASH BOUNTY.", y=178, c=YELLOW)
        CT(cv, 236, "(NOT WHILE LEADING A M.U.L.E.)", TAN)
    else:
        tt = t - 5
        CT(cv, 8, "THE PUB", YELLOW, 2)
        bx, by, bw, bh = TOWN_RECT["PUB"]
        pub = town_img(PUB_STILL)[by * TRH:(by + bh) * TRH, bx * TCW:(bx + bw) * TCW]
        b = big(pub, 2)
        cv.paste(W // 2 - b.shape[1] // 2, 48, b)
        species(cv, W // 2 + 64, 76, 3, 8 + int(tt * 4) % 3, BLUE, 2)
        cap(cv, tt, "DONE EARLY? THE PUB PAYS CASH. THE MORE TIME LEFT, THE MORE YOU WIN.", y=176)


def sc_pevent(cv, t, d):
    show_clip(cv, "pevent", t, 30)
    cap(cv, t, "EACH TURN MAY BRING GOOD OR BAD LUCK. LEADERS NEVER GET GOOD LUCK.", y=202)


def sc_production(cv, t, d):
    if t < 3:
        show_clip(cv, "production", t, 120)
        cap(cv, t, "PRODUCTION: DOTS SHOW WHAT EACH PLOT MADE.", y=202)
        return
    tt = t - 3
    CT(cv, 8, "BONUSES", YELLOW, 2)
    cv.rect(48, 48, 384, 118, MAPBG)
    cell = big(plot_cell(10), 2)
    for k in range(3):
        if k == 0 or tt > 0.6 * k:
            cv.paste(W // 2 - 3 * CW + k * 2 * CW, 52, cell)
    if tt > 1.2:
        T(cv, 80, 124, "+1 NEIGHBOR", lit(GREEN))
    if tt > 2.4:
        T(cv, 80, 144, "+1 PER 3 PLOTS", lit(GREEN))
    cap(cv, tt, "SAME GOOD NEXT DOOR, OR 3 PLOTS OF IT, MAKES MORE.", y=180)


EVENTS = [("ev_pirates", 40), ("ev_acid", 0), ("ev_fire", 20), ("ev_pest", 0), ("ev_meteor", 0)]


def sc_events(cv, t, d):
    k = min(len(EVENTS) - 1, int(t / 2))
    name, start = EVENTS[k]
    show_clip(cv, name, t - k * 2, start)
    cap(cv, 99, "COLONY EVENTS HIT EVERYONE.", y=202, c=YELLOW)
    cap(cv, 99, "SOME HELP, SOME HURT.", y=222)


def sc_goods(cv, t, d):
    show_clip(cv, "goods", t, 60)
    cap(cv, t, "AUCTIONS: SMITHORE, CRYSTITE, FOOD, THEN ENERGY.", y=202)


def sc_declare(cv, t, d):
    show_clip(cv, "declare", t, 40)
    cap(cv, t, "PUSH UP TO SELL, DOWN TO BUY. KEEP ENOUGH FOOD AND ENERGY!", y=202)


def sc_trading(cv, t, d):
    show_clip(cv, "trading", t, 40)
    if t < 4:
        cap(cv, t, "BUYERS AND SELLERS WALK TOWARD A PRICE. WHERE THEY MEET, THEY TRADE.", y=202)
    else:
        cap(cv, t - 4, "THE STORE BUYS AT THE BOTTOM AND SELLS AT THE TOP.", y=202)


def sc_win(cv, t, d):
    if t < 5:
        show_clip(cv, "summary", t, 0)
        cap(cv, t, "AFTER 12 MONTHS THE TOP SCORE IS FIRST FOUNDER.", y=202, c=YELLOW)
    else:
        show_clip(cv, "gameover", t - 5, 0)
        cap(cv, t - 5, "THE COLONY IS RATED ON EVERYONE'S TOTAL. RUN OUT OF FOOD OR ENERGY AND IT FAILS.",
            y=202, c=PINK)


def sc_crossplay(cv, t, d):
    p = plats()
    border = tuple(PAL[DKGREEN])
    if t < 6:
        CT(cv, 6, "ONE COLONY", YELLOW, 2)
        vidlib.grid10(cv, t, "15-trading", p, PTEXT, 0.4, border)
        cap(cv, t - 3.6, "MANY MACHINES, ONE GAME.", y=232)
        return
    if t < 12:
        tt = t - 6
        cx, cy = 240, 120
        corners = [(16, 16, CORNERS[0], 0, 3), (256, 16, CORNERS[1], 1, 1),
                   (16, 184, CORNERS[2], 2, 5), (256, 184, CORNERS[3], 3, 6)]
        for i, (x, y, name, seat, sid) in enumerate(corners):
            if tt < 0.3 + i * 0.4:
                continue
            ax, ay = x + 104, y + 16
            n = 30
            for k in range(0, n, 2):
                cv.rect(int(ax + (cx - ax) * k / n), int(ay + (cy - ay) * k / n), 2, 2, DOTCOL)
            for ph in (0.0, 0.5):
                f = (tt * 0.8 + ph + i * 0.17) % 1.0
                f = 1 - f if ph else f
                cv.rect(int(ax + (cx - ax) * f) - 2, int(ay + (cy - ay) * f) - 2, 6, 4, PLAYER[seat])
            CT(cv, y, name, PTEXT[seat], cx=x + 104)
            figure(cv, x + 92, y + 20 if y < 100 else y - 30, sid, 4 + int(tt * 3) % 2, PLAYER[seat], 3)
        cv.rect(cx - 72, cy - 28, 144, 58, BLACK)
        cv.frame(cx - 72, cy - 28, 144, 58, CYAN, 2)
        pic(cv, cx - 8, cy - 24, 0, YELLOW, 2)
        CT(cv, cy - 4, "IRATA", lit(CYAN))
        CT(cv, cy - 4 + vidlib.CELLH * TS[1], ".ONLINE", lit(CYAN))
        cap(cv, tt - 2.6, "THE COMPUTER TAKES ANY EMPTY SEAT.", y=226)
        return
    tt = t - 12
    CT(cv, 6, "EVERY MACHINE", YELLOW, 2)
    vidlib.grid10(cv, tt + 10, "07-land-auction-bidding", p, PTEXT, 0.0, border)
    cap(cv, tt - 0.3, "SAME RULES. SAME TIMING.", y=232)


def sc_outro(cv, t, d):
    stars(cv, t, seed=11)
    CT(cv, 8, "READY TO PLAY?", YELLOW, 2)
    if t > 0.8:
        CT(cv, 52, typed("ON YOUR FUJINET, MOUNT", t - 0.8), WHITE)
    if t > 1.6:
        CT(cv, 76, "TNFS HOST", LABELCOL)
        CT(cv, 94, typed("APPS.IRATA.ONLINE", t - 1.6, 25), ACC)
    if t > 2.8:
        CT(cv, 118, "FOLDER", LABELCOL)
        CT(cv, 136, typed("MULE", t - 2.8, 20), ACC, 2)
    dy = 2 * vidlib.CELLH * TS[1] - 32          # the big MULE's extra height
    if t > 2.8:
        CT(cv, 168 + dy, typed(OUTRO_IMAGE, t - 3.2, 30), ACC)
    if t > 4.6:
        CT(cv, 192 + dy, typed("FROM ANY FUJINET MACHINE", t - 4.6), WHITE)
    if t > 5.6:
        CT(cv, 210 + dy, typed("AND JOIN A GAME!", t - 5.6), YELLOW)
    for k in range(4):
        x = int(-60 - k * 60 + t * 80)
        figure(cv, x, 238, 1 + k * 2, walk_frame(t + k * 0.1, 0), PLAYER[k], 3)
    mule(cv, int(-320 + t * 80), 238, walk_frame(t, 0, 6), WHITE, 3, town=False)


SCENES = [
    (9, sc_title), (13, sc_premise), (6, sc_online), (24, sc_species), (6, sc_controls),
    (12, sc_month), (8, sc_map), (10, sc_terrain), (8, sc_grant), (7, sc_bid),
    (18, sc_resources), (13, sc_town), (9, sc_develop), (5, sc_runaway), (10, sc_wampus),
    (7, sc_pevent), (8, sc_production), (10, sc_events), (5, sc_goods), (6, sc_declare),
    (10, sc_trading), (10, sc_win), (16, sc_crossplay), (10, sc_outro),
]



def main(init):
    """init(build) configures the platform; then stills or the video."""
    init(os.path.abspath(sys.argv[1]))
    vidlib.run(SCENES, BUILD, sys.argv[2:])

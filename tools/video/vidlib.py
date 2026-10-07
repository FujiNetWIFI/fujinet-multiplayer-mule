"""The platform-neutral half of the intro videos: a pixel canvas in the
console's own units, typed captions, clip playback, the cross-platform
screenshot tiles and the render driver (stills or an ffmpeg pipe).

A platform's render script calls setup() once with its canvas size, scale,
palette (index -> RGB) and glyphs (char -> 8 x 8 bool array), then builds its
scenes on Canvas and hands them to run().
"""
import glob
import os
import subprocess
import sys

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
FPS = 60
FADE = 0.12

# set by setup()
W = H = SX = SY = 0
PAL = None
GLYPH = {}
COLS = 0
WHITE = GREY = 0
CELLW, CELLH = 8, 8


def setup(w, h, sx, sy, pal, glyphs, white, grey, cell=(8, 8)):
    """cell: a glyph's cell (glyphs are CELLH x CELLW bool arrays)."""
    global W, H, SX, SY, PAL, GLYPH, COLS, WHITE, GREY, CELLW, CELLH
    W, H, SX, SY = w, h, sx, sy
    PAL = np.asarray(pal, dtype=np.uint8)
    GLYPH = glyphs
    CELLW, CELLH = cell
    COLS = w // CELLW
    WHITE, GREY = white, grey


def bitmask(rows, width=8):
    return np.array([[(r >> (width - 1 - x)) & 1 for x in range(width)] for r in rows], dtype=bool)


# ---------------------------------------------------------------------------
# The canvas
# ---------------------------------------------------------------------------
class Canvas:
    def __init__(self, bg=0):
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

    def bits(self, x, y, rows, c, sx=1, sy=2, flip=False, bg=None, width=8):
        """Rows of bits, MSB leftmost: each bit sx wide and sy tall."""
        self.mask(x, y, bitmask(rows, width), c, sx, sy, flip, bg)

    def text(self, x, y, s, c, sx=1, sy=2):
        for i, ch in enumerate(s):
            g = GLYPH.get(ch)
            if g is not None and g.any():
                self.mask(x + i * CELLW * sx, y, g, c, sx, sy)

    def ctext(self, y, s, c, sx=1, sy=2, cx=None):
        cx = W // 2 if cx is None else cx
        self.text(cx - len(s) * CELLW * sx // 2, y, s, c, sx, sy)

    def paste(self, x, y, arr):
        h, w = arr.shape[:2]
        x0, y0, x1, y1 = max(0, x), max(0, y), min(W, x + w), min(H, y + h)
        if x1 > x0 and y1 > y0:
            self.a[y0:y1, x0:x1] = arr[y0 - y:y1 - y, x0 - x:x1 - x]

    def dim(self, k):
        self.k = k

    def over(self, x, y, img):
        self.overlays.append((int(x), int(y), img))

    def otext(self, cx, y, s, c, scale=None):
        """Small text straight on the video picture, centred on cx (labels
        too small for the canvas grid), in square pixels of `scale`
        (default: 24 video pixels a character)."""
        if not s:
            return
        scale = 24 // CELLW if scale is None else scale
        m = np.zeros((CELLH, CELLW * len(s)), dtype=bool)
        for i, ch in enumerate(s):
            g = GLYPH.get(ch)
            if g is not None:
                m[:, i * CELLW:(i + 1) * CELLW] = g
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


# ---------------------------------------------------------------------------
# Captions
# ---------------------------------------------------------------------------
def wrap(s, n=None):
    n = COLS - 2 if n is None else n
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


def caption(cv, t, text, y=204, c=None, step=20, cps=45, sx=1, sy=2):
    """Centred, word-wrapped, typed-out caption lines."""
    c = WHITE if c is None else c
    lines = wrap(text, COLS // sx - 2)
    done = 0
    for i, ln in enumerate(lines):
        shown = typed(ln, t - done / cps, cps)
        cv.ctext(y + i * step, shown, c, sx, sy)
        done += len(ln)


def blink(t, hz=2.0):
    return (t * hz) % 1.0 < 0.5


def stars(cv, t, colours, n=70, seed=7, speed=6.0, h=2):
    """A twinkling, drifting star field; colours = (bright, dim, tint)."""
    bright, dim, tint = colours
    rng = np.random.default_rng(seed)
    xs = rng.uniform(0, W, n)
    ys = rng.integers(0, H, n)
    ph = rng.uniform(0, 1, n)
    for i in range(n):
        x = int((xs[i] - t * speed * (1 + i % 3)) % W)
        c = bright if (t * 1.5 + ph[i]) % 1 < 0.8 else dim
        cv.rect(x, int(ys[i]), max(1, W // 240), h, c if i % 4 else tint)


# ---------------------------------------------------------------------------
# Clips: numbered frames, each 2 game frames, read by a platform loader
# ---------------------------------------------------------------------------
class Clip:
    def __init__(self, folder, pattern, loader, drop=()):
        self.files = sorted(glob.glob(os.path.join(folder, pattern)))
        if not self.files:
            raise SystemExit("no clip in %s: film it first" % folder)
        self.loader = loader
        self.cache = {}
        self.drop = drop

    def __len__(self):
        return len(self.files)

    def get(self, i):
        i = max(0, min(len(self.files) - 1, i))
        while i in self.drop:
            i -= 1
        if i not in self.cache:
            if len(self.cache) > 64:
                self.cache.clear()
            self.cache[i] = self.loader(self.files[i])
        return self.cache[i]


# ---------------------------------------------------------------------------
# Cross-platform play: real screenshots of the FujiNet clients
# (screenshots/<target>/), drawn as overlays at video resolution.
# ---------------------------------------------------------------------------
SHOTS = os.path.join(HERE, "..", "..", "screenshots")
PLATFORMS = [("intv", "INTELLIVISION"), ("atari", "ATARI 8-BIT"), ("atari-2600", "ATARI 2600"),
             ("apple2", "APPLE II"), ("coco", "COCO"), ("msdos-cga", "IBM PC/PCJR"),
             ("msx", "MSX"), ("adam", "COLECO ADAM"), ("lynx", "LYNX"), ("astrocade", "ASTROCADE")]
_SHOT = {}


def platforms(first):
    """PLATFORMS with `first` (a screenshots/ folder) moved to the front."""
    return [p for p in PLATFORMS if p[0] == first] + [p for p in PLATFORMS if p[0] != first]


SHOT_OVERRIDE = {}          # (target, name) -> a picture to use instead


def shot(target, name, bw, bh):
    """A screenshot fitted to a bw x bh box: a TV's 4:3, or the Lynx's own
    shape, scaled with nearest neighbour."""
    key = (target, name, bw, bh)
    if key not in _SHOT:
        path = SHOT_OVERRIDE.get((target, name), os.path.join(SHOTS, target, name + ".png"))
        im = Image.open(path).convert("RGB")
        aspect = im.width / im.height if target == "lynx" else 4 / 3
        w, h = bw, int(bw / aspect)
        if h > bh:
            w, h = int(bh * aspect), bh
        _SHOT[key] = np.array(im.resize((w, h), Image.NEAREST))
    return _SHOT[key]


def tile(cv, target, name, label, x, y, bw, bh, pop=1.0, lc=None, border=(0x38, 0x6B, 0x3F)):
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
    b = np.zeros((h + 8, w + 8, 3), dtype=np.uint8)
    b[:] = border
    b[4:-4, 4:-4] = img
    cv.over(cx - w // 2 - 4, cy - h // 2 - 4, b)
    if pop >= 1.0:
        cv.otext(cx, y + bh + 12, label, WHITE if lc is None else lc)


def grid_online(cv, t, plats, labelcols, name="03-summary", border=(0x38, 0x6B, 0x3F)):
    """Four machines' copies of one screen in a 2 x 2 grid, popping in."""
    bw, bh = 400, 280
    for i, (tg, label) in enumerate(plats[:4]):
        r, c = divmod(i, 2)
        x = 960 - bw - 30 + c * (bw + 60)
        y = 160 + r * (bh + 80)
        tile(cv, tg, name, label, x, y, bw, bh, pop=min(1.0, (t - 0.8 - i * 0.5) / 0.25),
             lc=labelcols[i % len(labelcols)], border=border)


def grid10(cv, t, name, plats, labelcols, t0=0.0, border=(0x38, 0x6B, 0x3F)):
    """All ten machines' copies of one screen, 5 x 2, popping in in turn."""
    bw, bh = 320, 240
    for i, (tg, label) in enumerate(plats):
        r, c = divmod(i, 5)
        x = 60 + c * (bw + 40)
        y = 170 + r * (bh + 90)
        tile(cv, tg, name, label, x, y, bw, bh, pop=min(1.0, (t - t0 - i * 0.3) / 0.2),
             lc=labelcols[i % len(labelcols)], border=border)


# ---------------------------------------------------------------------------
# The driver
# ---------------------------------------------------------------------------
def frame_at(scenes, T, bg=0):
    t0 = 0.0
    last = scenes[-1][1]
    for d, fn in scenes:
        if T < t0 + d or fn is last:
            t = T - t0
            cv = Canvas(bg)
            fn(cv, t, d)
            # a quick dip to black at each cut; the last scene fades out over
            # its final second
            out = 1.0 if fn is last else FADE
            k = min(1.0, t / FADE, (d - t) / out)
            if k < 1.0:
                cv.dim(max(0.0, k))
            return cv
        t0 += d


def run(scenes, build, argv, music="music.wav", bg=0):
    """argv: OUT.mp4, or --still T [T...] (PNG stills into build)."""
    if "--still" in argv:
        for ts in argv[argv.index("--still") + 1:]:
            T = float(ts)
            Image.fromarray(frame_at(scenes, T, bg).video()).save(
                os.path.join(build, "still_%06.2f.png" % T))
        return
    out = argv[0]
    total = sum(d for d, _ in scenes)
    n = int(round(total * FPS))
    print("%d scenes, %.1f s, %d frames" % (len(scenes), total, n))
    ff = subprocess.Popen(
        ["ffmpeg", "-v", "error", "-y", "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", "%dx%d" % (W * SX, H * SY),
         "-r", str(FPS), "-i", "-", "-i", os.path.join(build, music),
         "-map", "0:v", "-map", "1:a", "-c:v", "libx264", "-preset", "slow", "-crf", "16",
         "-tune", "animation", "-pix_fmt", "yuv420p", "-c:a", "aac", "-b:a", "192k",
         "-movflags", "+faststart", "-shortest", out], stdin=subprocess.PIPE)
    for f in range(n):
        ff.stdin.write(frame_at(scenes, f / FPS, bg).video().tobytes())
        if f % 600 == 0:
            print("  %3d s" % (f // FPS), flush=True)
    ff.stdin.close()
    ff.wait()
    print("wrote", out)
    sys.stdout.flush()

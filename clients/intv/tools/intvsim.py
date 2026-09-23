#!/usr/bin/env python3
"""intvsim.py -- run the game headless in jzIntv and take real screenshots.

Drives jzIntv's debugger (-d) over a pipe: a breakpoint on frame_isr makes
every 'r' exactly one frame. Screenshots are rendered here from BACKTAB,
GRAM, the GROM and IntyBASIC's MOB shadow (_mobs), in the screen mode the
game says it is in (scr_mode / scr_bg), so they show what a TV would.

Input goes through the TEST_HOOK in input.bas ($8FF0 = $A5, $8FF1 = the
decoded controller byte: disc 1 S, 2 E, 4 N, 8 W; top button $A0).

usage: intvsim.py mule.rom scenario.txt outdir
Scenario lines ('#' comments):
    wait N                  run N frames
    snap NAME               save outdir/NAME.png and print the screen text
    hold HEX N              hold controller byte HEX for N frames, then let go
    press HEX               hold for 4 frames then let go (a button tap)
    until TEXT [MAX]        run until the screen shows TEXT (max frames, 3600)
    untilvar NAME VAL [MAX] run until IntyBASIC variable NAME (low byte) is VAL
    untilnot TEXT [MAX]     run until it doesn't
    peek ADDR N             print N words of memory from ADDR (hex)
    vars NAME...            print IntyBASIC variables
    trace N NAME...         print them every frame for N frames
    set HEX                 set the controller byte (stays until set 0)
    film PREFIX EVERY COUNT a snapshot every EVERY frames, COUNT times
    text                    print the screen text
    test PHASE [N [EV]]     (first line only) start the offline test mode
Environment: JZINTV_DIR, FUJINET (host:port, default localhost:9997).
"""
import os
import re
import subprocess
import sys

JZ = os.environ.get("JZINTV_DIR", os.path.expanduser("~/Workspace/jzintv-20200712-src"))
JZINTV = os.environ.get("JZINTV", os.path.join(JZ, "bin", "linux", "jzintv"))
FUJINET = os.environ.get("FUJINET", "localhost:9997")
HOOK = 0x8FF0

PAL = [(0x00, 0x00, 0x00), (0x00, 0x2D, 0xFF), (0xFF, 0x3D, 0x10), (0xC9, 0xCF, 0xAB),
       (0x38, 0x6B, 0x3F), (0x00, 0xA7, 0x56), (0xFA, 0xEA, 0x50), (0xFF, 0xFC, 0xFF),
       (0xBD, 0xAC, 0xC8), (0x24, 0xB8, 0xFF), (0xFF, 0xB4, 0x1F), (0x54, 0x6E, 0x00),
       (0xFF, 0x4E, 0x57), (0xA4, 0x96, 0xFF), (0x75, 0xCC, 0x80), (0xB5, 0x1A, 0x58)]


class Jz:
    def __init__(self, rom, sym):
        env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
        self.p = subprocess.Popen(
            [JZINTV, "-d", "-e", os.path.join(JZ, "rom", "exec.bin"), "-g", os.path.join(JZ, "rom", "grom.bin"),
             "--fujinet=" + FUJINET, "-r1", rom],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env, bufsize=0)
        self.buf = b""
        self.read_prompt()
        self.cmd("l " + sym)
        self.syms = {}
        for ln in open(sym):
            a, _, n = ln.strip().partition(" ")
            if n and re.match(r"^[0-9A-Fa-f]+$", a):
                self.syms[n] = int(a, 16)
        self.cmd("b label_FRAME_ISR")
        self.grom = open(os.path.join(JZ, "rom", "grom.bin"), "rb").read()
        self.frames = 0

    def read_prompt(self):
        while True:
            i = self.buf.find(b"\n> ")
            if i < 0 and self.buf.startswith(b"> "):
                i = -1
                out, self.buf = b"", self.buf[2:]
                return out.decode("latin1")
            if i >= 0:
                out, self.buf = self.buf[:i + 1], self.buf[i + 3:]
                return out.decode("latin1")
            ch = os.read(self.p.stdout.fileno(), 65536)
            if not ch:
                raise SystemExit("jzIntv exited:\n" + self.buf.decode("latin1")[-2000:])
            self.buf += ch

    def cmd(self, c):
        self.p.stdin.write((c + "\n").encode())
        self.p.stdin.flush()
        return self.read_prompt()

    def run(self, n=1):
        for _ in range(n):
            self.cmd("r")
            self.frames += 1

    def mem(self, addr, n):
        out = self.cmd("m %x %x" % (addr, n))
        words = {}
        for ln in out.splitlines():
            m = re.match(r"^([0-9A-F]{4}):((?:\s+[0-9A-F]{4}\*?){1,8})", ln)
            if m:
                a = int(m.group(1), 16)
                for i, t in enumerate(re.findall(r"[0-9A-F]{4}", m.group(2))):
                    words[a + i] = int(t, 16)
        return [words.get(addr + i, 0) for i in range(n)]

    def poke(self, addr, val):
        self.cmd("e %x %x" % (addr, val))

    def var(self, name):
        n = name.upper()
        return self.mem(self.syms.get("var_" + n, self.syms.get("var_&" + n)), 1)[0]

    def screen_text(self, bt=None):
        bt = bt or self.mem(0x200, 240)
        mode = self.var("scr_mode") & 1
        rows = []
        for r in range(12):
            s = ""
            for c in range(20):
                w = bt[r * 20 + c]
                if w & 0x800:
                    s += "#"
                else:
                    card = (w >> 3) & (0x3F if mode else 0xFF)
                    s += chr(card + 32) if card < 95 else "?"
            rows.append(s)
        return rows

    def render(self, path):
        from PIL import Image
        bt = self.mem(0x200, 240)
        gram = [w & 0xFF for w in self.mem(0x3800, 512)]
        mobs = self.mem(self.syms["_mobs"], 24)
        mode = self.var("scr_mode") & 1
        bg0 = self.var("scr_bg") & 15
        img = Image.new("RGB", (176, 208), PAL[bg0])   # 8 px border around 160 x 192 scanlines
        px = img.load()

        def card_bits(gram_card, n):
            if gram_card:
                return gram[(n & 63) * 8:(n & 63) * 8 + 8]
            return self.grom[n * 8:n * 8 + 8]

        fgmask = [[False] * 192 for _ in range(160)]
        for r in range(12):
            for c in range(20):
                w = bt[r * 20 + c]
                g = bool(w & 0x800)
                if mode:
                    n = (w >> 3) & 0x3F
                    fg = w & 7
                    bg = (w >> 9 & 1) | (w >> 9 & 2) | ((w >> 13) & 1) << 2 | ((w >> 12) & 1) << 3
                else:
                    n = (w >> 3) & (0x3F if g else 0xFF)
                    fg = (w & 7) | ((w >> 9) & 8 if g else 0)
                    bg = bg0
                bits = card_bits(g, n)
                for y in range(8):
                    for x in range(8):
                        on = (bits[y] >> (7 - x)) & 1
                        col = PAL[fg] if on else PAL[bg]
                        for s in (0, 1):
                            px[8 + c * 8 + x, 8 + (r * 8 + y) * 2 + s] = col
                        if on:
                            fgmask[c * 8 + x][(r * 8 + y) * 2] = True
                            fgmask[c * 8 + x][(r * 8 + y) * 2 + 1] = True
        # MOBs, 7 first so 0 ends on top.
        for i in range(7, -1, -1):
            X, Y, A = mobs[i], mobs[8 + i], mobs[16 + i]
            if not X & 0x200:
                continue
            x0 = (X & 0xFF) - 8
            wide = 2 if X & 0x400 else 1
            y0 = ((Y & 0x7F) - 8) * 2
            tall = 16 if Y & 0x80 else 8
            ysz = [1, 2, 4, 8][(Y >> 8) & 3]
            xflip, yflip = Y & 0x400, Y & 0x800
            col = PAL[(A & 7) | ((A >> 9) & 8)]
            g = bool(A & 0x800)
            n = (A >> 3) & 0xFF
            if tall == 16:
                n &= ~1
            behind = A & 0x2000
            rows = []
            for k in range(tall // 8):
                rows += card_bits(g, n + k)
            if yflip:
                rows = rows[::-1]
            for ry, b in enumerate(rows):
                for rx in range(8):
                    on = (b >> (7 - rx)) & 1
                    if not on:
                        continue
                    xx = 7 - rx if xflip else rx
                    for dx in range(wide):
                        for dy in range(ysz):
                            sx, sy = x0 + xx * wide + dx, y0 + ry * ysz + dy
                            if 0 <= sx < 160 and 0 <= sy < 192:
                                if behind and fgmask[sx][sy]:
                                    continue
                                px[8 + sx, 8 + sy] = col
        img.resize((176 * 4, 208 * 2), Image.NEAREST).save(path)


def main():
    rom, scen, outdir = sys.argv[1], sys.argv[2], sys.argv[3]
    os.makedirs(outdir, exist_ok=True)
    jz = Jz(rom, rom[:-4] + ".sym")
    lines = open(scen).read().splitlines() if scen != "-" else sys.stdin.read().splitlines()
    # "test PHASE [EVENT]" as the first line starts test.bas's offline mode.
    first = lines[0].split("#", 1)[0].split() if lines else []
    if first and first[0] == "test":
        jz.poke(HOOK + 2, 0x5A)
        jz.poke(HOOK + 3, int(first[1]) if len(first) > 1 else 3)
        jz.poke(HOOK + 4, int(first[2]) if len(first) > 2 else 0)
        jz.poke(HOOK + 5, int(first[3]) if len(first) > 3 else 0)
        lines = lines[1:]
    jz.poke(HOOK, 0xA5)
    jz.poke(HOOK + 1, 0)
    jz.run(2)
    for ln in lines:
        ln = ln.split("#", 1)[0].strip()
        if not ln:
            continue
        op, _, arg = ln.partition(" ")
        if op == "wait":
            jz.run(int(arg))
        elif op == "snap":
            jz.render(os.path.join(outdir, arg + ".png"))
            print("--- %s (frame %d) ---" % (arg, jz.frames))
            for r, t in enumerate(jz.screen_text()):
                print("%2d|%s|" % (r, t))
        elif op == "text":
            for r, t in enumerate(jz.screen_text()):
                print("%2d|%s|" % (r, t))
        elif op in ("hold", "press"):
            parts = arg.split()
            jz.poke(HOOK, 0xA5)
            jz.poke(HOOK + 1, int(parts[0], 16))
            jz.run(int(parts[1]) if op == "hold" else 4)
            jz.poke(HOOK + 1, 0)
            jz.run(2)
        elif op in ("until", "untilnot"):
            parts = arg.rsplit(" ", 1)
            mx = 3600
            text = arg
            if len(parts) == 2 and parts[1].isdigit():
                text, mx = parts[0], int(parts[1])
            n = 0
            while n < mx:
                jz.run(10)
                n += 10
                found = any(text in t for t in jz.screen_text())
                if found == (op == "until"):
                    break
            print("%s %r: %d frames (%s)" % (op, text, n, "ok" if n < mx else "TIMEOUT"))
        elif op == "untilvar":
            # untilvar NAME VALUE [MAX]: run until an IntyBASIC variable equals VALUE
            parts = arg.split()
            mx = int(parts[2]) if len(parts) > 2 else 3600
            want = int(parts[1], 0)
            n = 0
            while n < mx and (jz.var(parts[0]) & 0xFF) != want:
                jz.run(10)
                n += 10
            print("untilvar %s=%d: %d frames (%s)" % (parts[0], want, n, "ok" if n < mx else "TIMEOUT"))
        elif op == "film":
            # film PREFIX EVERY COUNT: a snapshot every EVERY frames, COUNT times
            pre, every, count = arg.split()
            for i in range(int(count)):
                jz.run(int(every))
                jz.render(os.path.join(outdir, "%s_%03d.png" % (pre, i)))
                print("film %s_%03d frame %d phase %d" % (pre, i, jz.frames, jz.var("phase") & 255))
        elif op == "vars":
            print("vars", " ".join("%s=%d" % (v, jz.var(v) & 0xFFFF) for v in arg.split()))
        elif op == "trace":
            # trace N VAR...: print the variables every frame for N frames
            parts = arg.split()
            for _ in range(int(parts[0])):
                jz.run(1)
                print("f%d" % jz.frames, " ".join("%s=%d" % (v, jz.var(v) & 0xFFFF) for v in parts[1:]))
        elif op == "set":
            # set HEX N: hold controller byte HEX without releasing (0 releases)
            jz.poke(HOOK + 1, int(arg, 16))
        elif op == "poke":
            # poke HEXADDR VAL [VAL...]: write consecutive locations
            a, *vals = arg.split()
            for k, v in enumerate(vals):
                jz.poke(int(a, 16) + k, int(v))
        elif op == "pokevar":
            # pokevar NAME IDX VAL: write a variable or array element (8- or
            # 16-bit; leave off the '#', which starts a comment here).
            n, i, v = arg.split()
            n = n.upper()
            addr = None
            for s in ("var_" + n, "array_" + n, "var_&" + n, "array_&" + n):
                addr = jz.syms.get(s, addr) if addr is None else addr
            jz.poke(addr + int(i), int(v))
        elif op == "peek":
            a, _, n = arg.partition(" ")
            print(arg, " ".join("%04X" % w for w in jz.mem(int(a, 16), int(n or 1))))
        else:
            raise SystemExit("unknown: " + ln)
    jz.p.stdin.write(b"q\n")
    jz.p.stdin.flush()


if __name__ == "__main__":
    main()

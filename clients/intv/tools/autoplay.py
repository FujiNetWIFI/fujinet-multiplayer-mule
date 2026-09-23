#!/usr/bin/env python3
"""autoplay.py -- play M.U.L.E. headless against a real server, for testing.

usage: autoplay.py mule.rom outdir [minutes]

Uses intvsim's jzIntv driver. Every few frames it looks at the game's own
variables and acts like a (simple-minded) player: readies up, presses on the
summaries, claims the first free plot the cursor reaches, and in its
development turn buys a M.U.L.E., outfits it for food and installs it on a
plot it owns. It saves a screenshot every 5 seconds and whenever the phase
changes, and prints a log of what it did.
"""
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import intvsim  # noqa: E402

HOOK = intvsim.HOOK
PHASES = ["lobby", "grant", "landauction", "develop", "production", "declare", "trading", "summary", "gameover"]


class Player:
    def __init__(self, jz, outdir):
        self.jz = jz
        self.out = outdir
        self.shot = 0
        self.last_phase = -1
        self.claimed_round = -1
        self.pressed = set()
        self.plan = []
        self.last_press = -1000

    def log(self, *a):
        print("[f%6d]" % self.jz.frames, *a, flush=True)

    def snap(self, tag):
        name = "%04d_%s" % (self.shot, tag)
        self.shot += 1
        self.jz.render(os.path.join(self.out, name + ".png"))

    def hold(self, val, frames):
        self.jz.poke(HOOK, 0xA5)          # the game clears the hook at boot
        self.jz.poke(HOOK + 1, val)
        self.jz.run(frames)
        self.jz.poke(HOOK + 1, 0)
        self.jz.run(2)

    def v(self, name):
        return self.jz.var(name) & 0xFF

    def step(self):
        ph = self.v("phase")
        rnd = self.v("round")
        if ph != self.last_phase:
            self.log("phase", PHASES[ph] if ph < 9 else ph, "round", rnd)
            self.last_phase = ph
            self.snap(PHASES[ph] if ph < 9 else "p%d" % ph)
        if ph == 0:
            seat = self.v("my_seat")
            if seat < 4 and self.jz.frames - self.last_press > 120:
                flags = self.jz.mem(0x8000 + 16 + seat * 29 + 11, 1)[0]
                if not flags & 2:
                    self.hold(0xA0, 4)
                    self.last_press = self.jz.frames
                    self.log("lobby: ready")
        elif ph == 7:
            if self.v("su_drawn") and not self.v("su_pressed") and self.jz.frames - self.last_press > 60:
                self.last_press = self.jz.frames
                self.hold(0xA0, 4)
                self.log("summary: pressed")
        elif ph == 1:
            cur = self.v("g_cur")
            if (self.claimed_round != rnd and cur < 45 and self.v("gr_claimed") == 0
                    and self.v("g_show") and not self.v("gr_depart")):
                self.hold(0xA0, 4)
                self.claimed_round = rnd
                self.log("grant: claim at", cur)
        elif ph == 3:
            st = self.v("dv_state")
            if st == 1:
                self.hold(0xA0, 4)
                self.log("develop: start turn")
                self.plan = ["corral", "shop", "leave", "plot"]
            elif st == 2 and self.plan and self.v("anim") == 1 and not self.v("w_hold"):
                self.do_plan()
        elif ph == 6:
            if self.v("au_stage") == 3 and ("trade", rnd, self.v("au_good")) not in self.pressed:
                self.pressed.add(("trade", rnd, self.v("au_good")))
                self.hold(0x04, 60)
                self.log("trading: pushed up")

    def walk_x(self, target, maxf=400):
        self.jz.poke(HOOK, 0xA5)
        n = 0
        while n < maxf:
            x = self.v("w_x")
            if x == target or self.v("w_hold"):
                break
            self.jz.poke(HOOK + 1, 0x02 if x < target else 0x08)
            self.jz.run(1)
            n += 1
        self.jz.poke(HOOK + 1, 0)
        self.jz.run(1)

    def do_plan(self):
        what = self.plan.pop(0)
        if what == "corral":
            self.walk_x(116)
            self.hold(0x01, 60)
            self.jz.run(60)
            self.log("develop: corral -> mule", self.v("w_mule"))
            self.snap("corral")
        elif what == "shop":
            self.walk_x(48)
            self.hold(0x04, 60)
            self.jz.run(60)
            self.log("develop: food shop")
            self.snap("shop")
        elif what == "leave":
            self.walk_x(0, 200)
            self.jz.run(60)
            self.log("develop: left town, screen", self.v("scr_now"))
        elif what == "plot":
            # Find a plot we own and walk onto it.
            st = 0x8000
            seat = self.v("my_seat")
            owned = [p for p in range(45)
                     if (self.jz.mem(st + 177 + p, 1)[0] & 255) == seat and p != 22]
            if not owned:
                self.log("develop: no plot to install on")
                return
            p = owned[0]
            tx, ty = (p % 9) * 16 + 16, (p // 9) * 16 + 16
            self.jz.poke(HOOK, 0xA5)
            for _ in range(900):
                x, y = self.v("w_x"), self.v("w_y")
                ty2 = ty + 6                   # puts the middle of the small figure (y + 2) on the plot
                if abs(x - tx) <= 2 and abs(y - ty2) <= 2:
                    break
                dx = 0x02 if x < tx else (0x08 if x > tx + 2 else 0)
                dy = 0x01 if y < ty2 else (0x04 if y > ty2 + 2 else 0)
                self.jz.poke(HOOK + 1, dx | dy)
                self.jz.run(1)
            self.jz.poke(HOOK + 1, 0)
            self.jz.run(2)
            self.hold(0xA0, 4)
            self.jz.run(60)
            self.log("develop: install on", p, "mule now", self.v("w_mule"))
            self.snap("install")


def main():
    rom, outdir = sys.argv[1], sys.argv[2]
    minutes = float(sys.argv[3]) if len(sys.argv) > 3 else 10
    os.makedirs(outdir, exist_ok=True)
    jz = intvsim.Jz(rom, rom[:-4] + ".sym")
    jz.poke(HOOK, 0xA5)
    jz.poke(HOOK + 1, 0)
    jz.run(2)
    p = Player(jz, outdir)
    end = time.time() + minutes * 60
    last = 0
    while time.time() < end:
        jz.run(10)
        p.step()
        if jz.frames - last >= 300:
            last = jz.frames
            p.snap("t")
    jz.p.stdin.write(b"q\n")
    jz.p.stdin.flush()


if __name__ == "__main__":
    main()

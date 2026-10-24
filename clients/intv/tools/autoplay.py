#!/usr/bin/env python3
"""autoplay.py -- play M.U.L.E. headless against a real server, for testing
and filmed games.

usage: autoplay.py mule.rom outdir [minutes]

Uses intvsim's jzIntv driver. Every frame it looks at the game's own
variables and holds the controller as a player would (the same play as the
C clients' make AUTOPLAY=1, src/autoplay.c): ready once four players sit,
on with the summary a moment after its table is up, claim the first plot of
the terrain this seat farms, walk the development turn (corral, outfitter,
out of town on the plot's side, down or up to its row and along to it,
install, back to the pub), declare by what next month needs, and push the
line in trading until the surplus is sold or the shortfall bought. It saves
a screenshot every 5 seconds and whenever the phase changes, and prints a
log of what it did.
"""
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import intvsim  # noqa: E402

HOOK = intvsim.HOOK
PHASES = ["lobby", "grant", "landauction", "develop", "production", "declare", "trading", "summary", "gameover"]

# the controller byte (input.bas TEST_HOOK): disc S, E, N, W; the top buttons
DOWN, RIGHT, UP, LEFT, BUTTON = 0x01, 0x02, 0x04, 0x08, 0xA0

# MuleState (../../mule_proto.h) at $8000, a byte an address
ST = 0x8000
PLAYERS, PL_SIZE = 16, 29               # name 0, flags 11, money 12, goods 14
TERRAIN, OWNER, MULE = 132, 177, 222
STORE_MULES, MULE_PRICE = 385, 386
AU_START, AU_STORE_SELL = 406, 416
CARRYING = 456
PF_AI, PF_READY = 1, 2
RIVER, MOUNTAIN, PLAIN = 1, 2, 0
FOOD, ENERGY, SMITHORE = 0, 1, 2
SELLER, BUYER = 2, 1
STAGE_SHOW, STAGE_DECLARE, STAGE_LEAD, STAGE_ACTIVE = 0, 1, 2, 3

LAND = [RIVER, MOUNTAIN, PLAIN, PLAIN]  # the terrain each seat farms
DOOR = [120, 96, 72, 48]                # the outfitters, by good (isr.bas)

# the game's 8-bit variables live in $0100-$01EF, the 16-bit ones from $0300
V8 = ("ACT ANIM W_HOLD W_TOWN W_IN W_X W_Y W_MULE W_EVENT G_SHOW G_CUR AU_KIND AU_STAGE AU_GOOD "
      "AU_STEPV SCR_NOW ST_OK MY_SEAT PHASE GR_CLAIMED GR_DEPART ROUND DV_STATE SU_DRAWN SU_PRESSED").split()


class Player:
    def __init__(self, jz, log=None):
        self.jz = jz
        self.log = log or (lambda *a: print("[f%6d]" % jz.frames, *a, flush=True))
        a = jz.syms
        self.addr = {n: a["var_" + n] for n in V8}
        self.dv_left = a["var_&DV_LEFT"]
        self.au_pos = a["array_&AU_POS"]
        self.au_role = a["array_AU_ROLE"]
        self.ctl = 0            # the byte the hook holds
        self.btn = 0            # frames of a button press still to hold
        self.ph = -1
        self.ph_f = 0           # the frame the phase began
        self.last_press = -1000
        self.cur_f = 0          # the frame the grant cursor last moved
        self.last_cur = 255
        self.cart = None        # MuleState, refreshed every 15 frames
        self.cart_f = -1000
        self.dv = "off"
        self.dv_f = 0
        self.inside = False
        self.v = {}

    # ---- reading the game ------------------------------------------------
    def read(self):
        w = self.jz.mem(0x100, 0xF0)
        self.v = {n: w[a - 0x100] & 0xFF for n, a in self.addr.items()}
        w = self.jz.mem(0x300, 0x40)
        self.v["DV_LEFT"] = w[self.dv_left - 0x300] & 0xFFFF
        self.v["AU_POS"] = [w[self.au_pos - 0x300 + k] & 0xFFFF for k in range(4)]
        self.v["AU_ROLE"] = [x & 0xFF for x in self.jz.mem(self.au_role, 4)]
        if self.jz.frames - self.cart_f >= 15:
            self.cart = [x & 0xFF for x in self.jz.mem(ST, 470)]
            self.cart_f = self.jz.frames
        return self.v

    def b(self, off):
        return self.cart[off]

    def w(self, off):
        return self.cart[off] | self.cart[off + 1] << 8

    def pl(self, k, off):
        return PLAYERS + k * PL_SIZE + off

    def need(self, seat, good, rnd):
        if good == FOOD:
            return 0 if rnd > 12 else 5 if rnd >= 8 else 4 if rnd >= 4 else 3
        if good == ENERGY:
            return sum(1 for p in range(45) if self.b(OWNER + p) == seat and self.b(MULE + p) < 4
                       and self.b(MULE + p) != ENERGY)
        return 0

    # ---- the controller --------------------------------------------------
    def press(self, every=45):
        if self.btn or self.jz.frames - self.last_press < every:
            return
        self.btn = 4
        self.last_press = self.jz.frames

    def step(self):
        """Once a frame: read the game, then hold what a player would."""
        v = self.read()
        d = 0
        if v["ST_OK"] and v["MY_SEAT"] < 4:
            if v["PHASE"] != self.ph:
                self.ph = v["PHASE"]
                self.ph_f = self.jz.frames
                self.log("phase", PHASES[self.ph] if self.ph < 9 else self.ph, "round", v["ROUND"])
            d = self.decide(v) or 0
        if self.btn:
            self.btn -= 1
            d = BUTTON if self.btn >= 2 else 0      # 2 frames up after it (INPUT_SETTLE)
        if d != self.ctl:
            self.jz.poke(HOOK, 0xA5)
            self.jz.poke(HOOK + 1, d)
            self.ctl = d

    def decide(self, v):
        ph, seat, since = v["PHASE"], v["MY_SEAT"], self.jz.frames - self.ph_f
        if ph == 0:
            humans = sum(1 for k in range(4) if self.b(self.pl(k, 0)) and not self.b(self.pl(k, 11)) & PF_AI)
            if humans == 4 and not self.b(self.pl(seat, 11)) & PF_READY:
                self.press()
        elif ph == 7:
            if v["SU_DRAWN"] and not v["SU_PRESSED"] and since > 560:
                self.press()
        elif ph == 1:
            cur = v["G_CUR"]
            if cur != self.last_cur:
                self.last_cur, self.cur_f = cur, self.jz.frames
            if (not v["GR_CLAIMED"] and not v["GR_DEPART"] and v["G_SHOW"] and cur < 45
                    and self.jz.frames - self.cur_f >= 3):
                t = self.b(TERRAIN + cur) & 3
                if t == LAND[seat] or (cur >= 18 and t == PLAIN) or cur >= 36:
                    self.press(20)
        elif ph in (2, 5, 6):
            return self.auction(v, seat, since)
        elif ph == 3:
            return self.develop(v, seat)
        return 0

    def auction(self, v, me, since):
        stage, pos, role = v["AU_STAGE"], v["AU_POS"][me], v["AU_ROLE"][me]
        if v["AU_KIND"] == 1:
            if stage == STAGE_SHOW:
                if since > 90:
                    self.press()
                return 0
            if role != BUYER or stage < STAGE_LEAD:
                return 0
            n = sum(1 for p in range(45) if self.b(OWNER + p) == me)
            start = self.w(AU_START)
            if n < 6 and self.w(self.pl(me, 12)) > start + 300 and pos < start + (me + 2) * 4 * max(1, v["AU_STEPV"]):
                return UP
            return 0
        good = v["AU_GOOD"] & 3
        have = self.w(self.pl(me, 14) + 2 * good)
        need = self.need(me, good, v["ROUND"] + 1)
        if stage == STAGE_DECLARE:
            if since < 40:
                return 0
            want = SELLER if have > need else BUYER if need > have else 0
            if want == SELLER and role != SELLER:
                return UP
            if want == BUYER and role != BUYER:
                return DOWN
            return 0
        if stage not in (STAGE_LEAD, STAGE_ACTIVE):
            return 0
        if role == SELLER and have > need:
            return DOWN
        if role == BUYER and need > have and pos < self.w(AU_STORE_SELL):
            return UP
        return 0

    # ---- the development turn (src/develop.c ap_develop) -----------------
    def develop(self, v, seat):
        st = v["DV_STATE"]
        if st == 1:                             # "press the button to start"
            if self.dv != "prompt":
                self.dv, self.dv_f = "prompt", self.jz.frames
            elif self.jz.frames - self.dv_f > 60:
                self.press()
            return 0
        if st != 2:
            self.dv = "off"
            return 0
        if v["W_IN"] or v["W_EVENT"] == 5:
            self.inside = True                  # into a building: the server has it
        if v["W_HOLD"] or v["W_IN"] or v["W_EVENT"] or v["ANIM"] != 1:
            return 0
        if self.inside and self.jz.frames != self.cart_f:
            self.cart_f = -1000         # back out of a building: what it did, now
            self.read()
        left = v["DV_LEFT"]
        if self.dv in ("prompt", "off"):
            self.dv, self.dv_f = "wait", self.jz.frames
        if self.dv == "wait":
            if self.jz.frames - self.dv_f < 40:
                return 0
            self.plan(seat, left)
        x, y, town, mule = v["W_X"], v["W_Y"], v["W_TOWN"], v["W_MULE"]
        if self.dv == "corral":
            if mule:
                self.dv, self.inside = "shop", False
                self.log("develop: M.U.L.E. bought")
            elif self.inside:
                self.dv = "pub"
            else:
                return self.door(x, 116, DOWN)
        if self.dv == "shop":
            c = self.b(CARRYING)
            if c == self.good:
                self.dv = "out"
                self.log("develop: outfitted for", ["food", "energy", "smithore", "crystite"][self.good])
            elif self.inside:
                self.dv = "pub"
            else:
                return self.door(x, DOOR[self.good], UP)
        if self.dv == "out":
            if not town:
                self.dv = "plot"
            else:
                return self.side
        if self.dv == "plot":
            if not mule:
                self.log("develop: installed on", self.plot)
                self.dv = "home"
            elif y != self.ty:
                return DOWN if y < self.ty else UP
            elif x + 1 < self.tx:
                return RIGHT
            elif x > self.tx + 1:
                return LEFT
            else:
                self.press()
                return 0
        if self.dv == "home":
            if town:
                self.dv = "pub"
            elif y != 52:
                return DOWN if y < 52 else UP
            else:
                return RIGHT if x < 88 else LEFT
        if self.dv == "pub":
            if not town:
                self.dv = "home"
                return 0
            return self.door(x, 88, DOWN)
        return 0

    def plan(self, seat, left):
        """The turn begins: our first empty plot, and what it grows."""
        self.dv, self.inside = "pub", False
        mine = [p for p in range(45) if self.b(OWNER + p) == seat and p != 22 and self.b(MULE + p) >= 4]
        if not mine or left < 1024 or not self.b(STORE_MULES):
            self.log("develop: to the pub (land %d, time %d, mules %d)" % (len(mine), left, self.b(STORE_MULES)))
            return
        p = mine[0]
        t = self.b(TERRAIN + p) & 3
        self.good = FOOD if t == RIVER else SMITHORE if t == MOUNTAIN else FOOD if seat & 1 else ENERGY
        if self.w(self.pl(seat, 12)) < self.w(MULE_PRICE) + 25 * (self.good + 1):
            self.log("develop: to the pub (money)")
            return
        self.plot = p
        self.side = LEFT if p % 9 <= 4 else RIGHT
        self.tx, self.ty = (p % 9) * 16 + 16, (p // 9) * 16 + 20
        self.dv = "corral"
        self.log("develop: plot", p, "for", ["food", "energy", "smithore", "crystite"][self.good])

    @staticmethod
    def door(x, at, push):
        return RIGHT if x < at else LEFT if x > at else push


def main():
    rom, outdir = sys.argv[1], sys.argv[2]
    minutes = float(sys.argv[3]) if len(sys.argv) > 3 else 10
    os.makedirs(outdir, exist_ok=True)
    jz = intvsim.Jz(rom, rom[:-4] + ".sym")
    jz.poke(HOOK, 0xA5)
    jz.poke(HOOK + 1, 0)
    jz.run(2)
    p = Player(jz)
    end = time.time() + minutes * 60
    last, ph, shot = 0, -1, 0
    while time.time() < end:
        jz.run(1)
        p.step()
        if jz.frames - last >= 300 or p.ph != ph:
            last, ph = jz.frames, p.ph
            jz.render(os.path.join(outdir, "%04d_%s.png" % (shot, PHASES[ph] if 0 <= ph < 9 else "t")))
            shot += 1
    jz.p.stdin.write(b"q\n")
    jz.p.stdin.flush()


if __name__ == "__main__":
    main()

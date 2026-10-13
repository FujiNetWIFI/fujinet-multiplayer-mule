#!/usr/bin/env python3
"""h89sim.py -- drive the H89 build in fujinet-go-h89-desktop's headless H89.

usage: h89sim.py DISK MAPFILE SCENARIO OUTDIR

Boots Heath CP/M 2.2.04 (64K) from H17 drive 1 with DISK (r2r/h89-cpm/
mule.h8d) in drive 2, runs B:MULE, waits for the title, then plays the
scenario, one command a line; times are in frames of the game's clock
(frame_count). The same scenario language as tools/msx/msxsim (and
adamsim, a2sim, cocosim):

    test PHASE [ARG [PEVENT]]   offline test mode (src/test.c)
    wait N                      let N frames pass
    snap NAME                   OUTDIR/NAME.png (the H19's picture, once the
                                screen is all sent) and NAME.txt (its text)
    hold DIRS                   hold directions: any of u d l r (or 'none')
    press                       tap the button
    key CHAR|NUM                a key (RETURN = 13, ESC = 27)
    peek SYMBOL [N]             print N bytes at a symbol (from the map)
    poke SYMBOL VALUE
    type TEXT                   type on the H19's keyboard (\\r escapes)
    keyname NAME                press a key the rig names: esc, up, f1...
    sleep MS                    let the machine run MS milliseconds (once
                                the game has quit, its clock has stopped)
    profile N                   sample the PC for N frames: the functions
                                it was in most, from the map
    pcprof N                    the same from the PCs a profiling build's
                                clock interrupt keeps (H89_PROF: fair, where
                                the debugger's samples aren't); no warp

The machine is h89_session_run (with its in-process FujiNet) driven step
by step through its --stdin. H89SIM_RIG is fujinet-go-h89-desktop
(~/Workspace/fujinet-go-h89-desktop), H89SIM_BOOT the CP/M boot disk (its
tools/disks/boot/cpm-2.2.04-64k.h8d). Offline it runs with --warp (as
fast as it goes once the card is quiet). H89SIM_FUJINET=1 is a live game:
no warp, and the FujiNet's SD gets the appkeys first -- H89SIM_NAME (the
player's name, key 0) and H89SIM_TABLE (the table's URL, key 10, e.g.
http://127.0.0.1:9900/?table=irata).
"""
import os
import re
import subprocess
import sys
import time

RIG = os.environ.get("H89SIM_RIG", os.path.expanduser("~/Workspace/fujinet-go-h89-desktop"))
BOOT = os.environ.get("H89SIM_BOOT", os.path.join(RIG, "tools/disks/boot/cpm-2.2.04-64k.h8d"))
LIVE = os.environ.get("H89SIM_FUJINET") == "1"
DIRBITS = {"d": 1, "r": 2, "u": 4, "l": 8}


def symbols(path):
    syms = {}
    for line in open(path):
        m = re.match(r"(\S+)\s+= \$([0-9A-F]+) ;", line)
        if m:
            syms.setdefault(m.group(1), int(m.group(2), 16))
    return syms


class H89:
    def __init__(self, home, disk):
        cmd = [os.path.join(RIG, "build/core/h89_session_run"), "--roms", os.path.join(RIG, "tools/roms"),
               "--home", home, "--timeout", "120"]
        if not LIVE and not os.environ.get("H89SIM_NOWARP"):
            cmd.append("--warp")         # (a machine option: before the steps)
        cmd += ["--disk", "h17:1=" + os.path.join(home, "boot.h8d"), "--disk", "h17:2=" + disk, "--stdin"]
        self.p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT, text=True, bufsize=1)
        self.log = open(os.path.join(home, "rig.log"), "w")

    def step(self, s):
        """Run one step; its output lines (FujiNet's log prefix taken off)."""
        self.p.stdin.write(s + "\n")
        self.p.stdin.flush()
        out = []
        while True:
            line = self.p.stdout.readline()
            if not line:
                raise SystemExit("h89sim: the rig went away during: " + s)
            self.log.write(line)
            line = line.rstrip("\n")
            if line.startswith("[fujinet] "):
                line = line[10:]
            if line in ("@@ok", "@@fail"):
                if line == "@@fail":
                    print("h89sim: step failed: " + s, file=sys.stderr)
                return out
            out.append(line)

    def mem(self, addr, n):
        out = self.step("debug mem 0x%X %d" % (addr, n))
        b = []
        for line in out:
            m = re.match(r"^[0-9A-F]{4}\s+((?:[0-9A-F]{2} )+)", line + " ")
            if m:
                b += [int(x, 16) for x in m.group(1).split()]
        return b[:n]

    def poke(self, addr, *vals):
        self.step("debug poke 0x%X %s" % (addr, " ".join("%d" % v for v in vals)))

    def close(self):
        try:
            self.p.stdin.close()
            self.p.wait(timeout=20)
        except Exception:
            self.p.kill()


def main():
    disk, mapfile, scen, out = sys.argv[1:5]
    out = os.path.abspath(out)
    os.makedirs(out, exist_ok=True)
    home = os.path.join(out, "home")
    os.makedirs(home, exist_ok=True)
    with open(BOOT, "rb") as f, open(os.path.join(home, "boot.h8d"), "wb") as g:
        g.write(f.read())
    if LIVE:
        sd = os.path.join(home, "data/fujinet/SD/FujiNet")
        os.makedirs(sd, exist_ok=True)
        for key, val in (("00010100.key", os.environ.get("H89SIM_NAME", "H89")),
                         ("0001010a.key", os.environ.get("H89SIM_TABLE", ""))):
            if val:
                open(os.path.join(sd, key), "w").write(val)
    syms = symbols(mapfile)

    def sym(n):
        """A symbol (C names without their _), +offset, or a number."""
        base, _, off = n.partition("+")
        if re.match(r"^(0x)?[0-9A-Fa-f]+$", base) and not base.isalpha():
            a = int(base, 0)
        else:
            a = syms["_" + base] if "_" + base in syms else syms[base]
        return a + (int(off, 0) if off else 0)
    m = H89(home, os.path.abspath(disk))

    def frames():
        b = m.mem(sym("frame_count"), 2)
        return b[0] | b[1] << 8

    def wait_frames(n):
        t = (frames() + n) & 0xFFFF
        while ((frames() - t) & 0xFFFF) >= 0x8000:
            m.step("wait 50")

    try:
        m.step("until H:")
        m.step("type B\\r")
        m.step("until A>")
        m.step("wait 500")
        m.step("prompt A>")
        m.step("type B:MULE\\r")
        lines = [l.split("#")[0].strip() for l in open(scen)]
        lines = [l for l in lines if l]
        if lines and lines[0].split()[0] == "test":
            a = [int(x) for x in lines[0].split()[1:]] + [0, 0, 0]
            th = sym("test_hook")
            t0 = time.time()
            while m.mem(th, 2) != [0x3C, 0xC3]:
                if time.time() - t0 > 240:
                    raise SystemExit("h89sim: no title")
                m.step("wait 20")
            m.poke(th + 1, a[0] + 1, a[1], a[2])
            m.poke(th, 0x5A)
            t0 = time.time()
            while m.mem(th, 1) != [0xA5]:
                if time.time() - t0 > 20:
                    raise SystemExit("h89sim: test mode didn't start")
                m.step("wait 20")
            lines = lines[1:]
        for line in lines:
            w = line.split()
            c = w[0]
            if c == "wait":
                wait_frames(int(w[1]))
            elif c == "snap":
                t0 = time.time()
                while m.mem(sym("h89_idle"), 1) != [1] and time.time() - t0 < 10:
                    m.step("wait 50")
                ppm = os.path.join(out, w[1] + ".ppm")
                m.step("ppm " + ppm)
                # (FujiNet's log shares the rig's output: its lines are left out)
                txt = "\n".join(l for l in m.step("screen") if not l.startswith("----")
                                and not re.match(r"\d\d:\d\d:\d\d\.\d+ > |CF: dev:", l)) + "\n"
                # (the rig writes the graphics characters' UTF-8 as \xHH)
                txt = re.sub(r"(?:\\x[0-9A-Fa-f]{2})+",
                             lambda g: bytes.fromhex(g.group(0).replace("\\x", "")).decode("utf-8", "replace"), txt)
                open(os.path.join(out, w[1] + ".txt"), "w").write(txt)
                try:
                    from PIL import Image
                    Image.open(ppm).save(os.path.join(out, w[1] + ".png"))
                    os.remove(ppm)
                except Exception as e:
                    print("h89sim: no PNG: %s" % e, file=sys.stderr)
                print("SNAP %s frame %d" % (w[1], frames()), file=sys.stderr)
            elif c == "hold":
                bits = 0 if w[1] == "none" else sum(DIRBITS[ch] for ch in w[1])
                m.poke(sym("inp_hook"), 1)
                m.poke(sym("inp_hook_dir"), bits)
            elif c == "press":
                m.poke(sym("inp_hook"), 1)
                m.poke(sym("inp_hook_btn"), 1)
                wait_frames(3)
                m.poke(sym("inp_hook_btn"), 0)
            elif c == "key":
                k = int(w[1]) if w[1].isdigit() and len(w[1]) > 1 else ord(w[1][0])
                m.poke(sym("inp_hook"), 1)
                m.poke(sym("inp_hook_key"), k)
                wait_frames(2)
            elif c == "peek":
                n = int(w[2]) if len(w) > 2 else 1
                print("PEEK %s %s" % (w[1], " ".join("%02X" % b for b in m.mem(sym(w[1]), n))),
                      file=sys.stderr)
            elif c == "poke":
                m.poke(sym(w[1]), int(w[2], 0))
            elif c == "profile":
                funcs = sorted((a, n) for n, a in syms.items() if n.startswith("_") or n.startswith("l_"))
                addrs = [a for a, n in funcs]
                import bisect
                import collections
                hist = collections.Counter()
                raw = collections.Counter()
                t = (frames() + int(w[1])) & 0xFFFF
                while ((frames() - t) & 0xFFFF) >= 0x8000:
                    def fn(a):
                        k = bisect.bisect_right(addrs, a) - 1
                        return (re.sub(r"^l_(.*)_\d+$", r"_\1", funcs[k][1]) if k >= 0 and a < 0xE000
                                else "(cp/m %04X)" % (a & 0xFF00))
                    for line in m.step("debug regs"):
                        g = re.match(r"PC=([0-9A-F]{4}) SP=([0-9A-F]{4})", line)
                        if g:
                            pc, sp = int(g.group(1), 16), int(g.group(2), 16)
                            raw[pc] += 1
                            name = fn(pc)
                            if os.environ.get("H89SIM_CALLERS") and pc < 0xE000:
                                st = m.mem(sp, 8)
                                callers = [fn(st[i] | st[i + 1] << 8) for i in (0, 2, 4, 6)]
                                callers = [c for c in callers if not c.startswith("(cp/m") and c != name]
                                if callers:
                                    name += " <- " + callers[0]
                            hist[name] += 1
                n = sum(hist.values())
                print("PROFILE %d samples" % n, file=sys.stderr)
                for name, v in hist.most_common(25):
                    print("  %5.1f%% %s" % (100.0 * v / n, name), file=sys.stderr)
                print("  PCs: " + " ".join("%04X:%d" % (a, v) for a, v in raw.most_common(12)), file=sys.stderr)
            elif c == "pcprof":
                import bisect
                import collections
                funcs = sorted((a, n) for n, a in syms.items() if n.startswith("_") or n.startswith("l_"))
                addrs = [a for a, n in funcs]
                hist = collections.Counter()
                raw = collections.Counter()
                ring, at_sym = sym("h89_prof"), sym("h89_prof_at")
                prev = m.mem(at_sym, 1)[0]
                t = (frames() + int(w[1])) & 0xFFFF
                while ((frames() - t) & 0xFFFF) >= 0x8000:
                    m.step("wait 150")
                    at = m.mem(at_sym, 1)[0]
                    b = []
                    for k in range(0, 512, 128):
                        b += m.mem(ring + k, 128)
                    i = prev
                    while i != at:
                        pc = b[2 * i] | b[2 * i + 1] << 8
                        k = bisect.bisect_right(addrs, pc) - 1
                        name = (re.sub(r"^l_(.*)_\d+$", r"_\1", funcs[k][1]) if k >= 0 and pc < 0xD000
                                else "(cp/m %04X)" % (pc & 0xFF00))
                        hist[name] += 1
                        raw[pc] += 1
                        i = (i + 1) & 255
                    prev = at
                n = sum(hist.values()) or 1
                print("PCPROF %d samples" % n, file=sys.stderr)
                for name, v in hist.most_common(30):
                    print("  %5.1f%% %s" % (100.0 * v / n, name), file=sys.stderr)
                print("  PCs: " + " ".join("%04X:%d" % (a, v) for a, v in raw.most_common(16)), file=sys.stderr)
            elif c == "stallpcs":
                # the PCs of the worst stall (a profiling build), oldest first
                import bisect
                funcs = sorted((a, n) for n, a in syms.items() if n.startswith("_") or n.startswith("l_"))
                addrs = [a for a, n in funcs]
                at = m.mem(sym("h89_stall_at"), 1)[0]
                b = []
                for k in range(0, 512, 128):
                    b += m.mem(sym("h89_stall_pcs") + k, 128)
                seq = []
                for j in range(256):
                    i = (at + j) & 255
                    pc = b[2 * i] | b[2 * i + 1] << 8
                    k = bisect.bisect_right(addrs, pc) - 1
                    seq.append(re.sub(r"^l_(.*)_\d+$", r"_\1", funcs[k][1]) if k >= 0 and pc < 0xD000 else "cp/m")
                out_s, prev, cnt = [], None, 0
                for f in seq + [None]:
                    if f == prev:
                        cnt += 1
                        continue
                    if prev:
                        out_s.append("%s x%d" % (prev, cnt))
                    prev, cnt = f, 1
                print("STALL PCS (2 ms each): " + ", ".join(out_s), file=sys.stderr)
            elif c == "sleep":
                m.step("wait " + w[1])
            elif c == "keyname":
                m.step("key " + w[1])
            elif c == "type":
                m.step("type " + line.split(None, 1)[1])
            elif c == "test":
                raise SystemExit("h89sim: test must be the first step")
            else:
                raise SystemExit("h89sim: unknown command: " + line)
    finally:
        m.close()


if __name__ == "__main__":
    main()

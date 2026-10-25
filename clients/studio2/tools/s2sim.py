#!/usr/bin/env python3
"""s2sim.py -- run the Studio II client headless in MAME from a scenario.

    s2sim.py SCENARIO OUTDIR [IMAGE]

MAME's studio2 with the FujiNet cart device (fujinet-firmware pico/studio2
emu/apply.sh, protocol 2), serving IMAGE ($FUJINET_BOOT) from power-on.
SCENARIO is a file of steps, one per line (# comments); `-` reads stdin:

    wait N              N frames
    hold KEYS N         hold KEYS for N frames, then let go for 6
    press KEYS          hold 6 frames, let go 6
    down KEYS / up KEYS press and keep holding / let go (no wait)
                        (KEYS: 0-9 on pad A, b0-b9 on pad B, clear; joined by +)
    snap NAME           OUTDIR/NAME.png: the picture the 1861 shows, at a
                        TV's aspect (a pixel ten wide, a line three tall)
    burst NAME N        a snap every frame for N frames, NAME_000 ...
    text                the text cells (TEXT lines; '@' an app glyph, '%' mosaic)
    sprites             the sprites shown (SPR lines)
    peek ADDR [LEN]     console RAM ($0800-$09FF; ADDR may be a symbol from
                        the listing, NAME or NAME+n)
    speek ADDR [LEN]    the image's own bytes (SNAP, PREV, cart RAM: SPEEK)
    poke ADDR V ...     write console RAM
    status OFF [LEN]    the cart's status page (FN_R_* - $400)
    until ADDR V [N]    wait (at most N frames, default 3600) until ADDR == V
    watch ADDR N [LEN]  for N frames, print ADDR (LEN bytes) when it changes
    test PH [ARG [PEV]] the offline test mode: at the title, phase PH, its
                        argument (event, stage, seat) and player event
    short               the DMA bursts that came up short so far (SHORT)

S2SIM_FUJINET=host:port: the fujinet-pc to use (default: none, offline --
never the everyday one on 9995). S2SIM_WAV=FILE records the sound.
S2SIM_SECONDS: MAME is killed (SIGKILL) past it (default 600).
S2SIM_THROTTLE=1: real time (a live server's clock is wall time).
S2SIM_MAMEBIN: the binary in ~/Workspace/mame-studio2 (default studio2-p2).
Prints its lines prefixed (SNAP PEEK TEXT SPR WATCH UNTIL STATUS DONE).
"""
import glob
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
MAME = os.path.expanduser(os.environ.get("MAME", "~/Workspace/mame-studio2"))
MAMEBIN = os.environ.get("S2SIM_MAMEBIN", "studio2-p2")
ROMS = os.path.expanduser(os.environ.get("ROMS", "~/Workspace/mame/roms"))

LUA = r"""
local steps = { %(steps)s }
local testhk = %(testhk)d
local dev = manager.machine.devices[":fujinet"]
local ram = manager.machine.memory.shares[":ram"]
local ports = manager.machine.ioport.ports
local items = {}
local function item(name)
  if not items[name] then items[name] = emu.item(dev.items["0/" .. name]) end
  return items[name]
end
local function rd(a) return ram:read_u8(a & 0x1FF) end
local function wr(a, v) ram:write_u8(a & 0x1FF, v) end
local frames, i, waitleft = 0, 1, 0
local held, holdleft, relleft = nil, 0, 0
local untila, untilv, untilleft = 0, 0, 0
local burstname, burstk, burstleft = "", 0, 0
local watcha, watchleft, watchlen, watchlast = 0, 0, 1, ""
local teststage, testargs = 0, nil

local function setkeys(list, v)
  for _, k in ipairs(list) do
    local f = ports[k[1]].fields[k[2]]
    if v ~= 0 then f:set_value(1) else f:clear_value() end
  end
end
local function snap(name)
  local front = item("m_front"):read(0)
  local buf = item("m_text.disp.buf")
  local t = {}
  for k = 0, 1023 do t[#t + 1] = string.format("%%02X", buf:read(front * 1024 + k)) end
  print("SNAP " .. name .. " " .. frames .. " " .. table.concat(t))
end
local function text()
  local ch = item("m_text.chars")
  for r = 0, 9 do
    local s = ""
    for c = 0, 15 do
      local v = ch:read(r * 16 + c)
      if v >= 0x20 and v < 0x7F then s = s .. string.char(v)
      elseif v == 0xA0 then s = s .. "%%"
      elseif v >= 0x80 then s = s .. "@"
      else s = s .. "#" end
    end
    print(string.format("TEXT %%d |%%s|", r, s))
  end
end
local function sprites()
  local m = item("m_spr")
  for n = 0, 15 do
    if m:read(n * 5 + 4) ~= 0 then
      local function s8(v) return v >= 128 and v - 256 or v end
      print(string.format("SPR %%d id %%d x %%d y %%d mode %%d", n, m:read(n * 5),
            s8(m:read(n * 5 + 1)), s8(m:read(n * 5 + 2)), m:read(n * 5 + 3)))
    end
  end
end
local function hex(a, n)
  local t = {}
  for k = 0, n - 1 do t[#t + 1] = string.format("%%02X", rd(a + k)) end
  return table.concat(t, " ")
end
local function shex(a, n)
  local live = item("m_dbg_live"):read(0)
  local buf = item("m_buf[" .. live .. "]")
  local t = {}
  for k = 0, n - 1 do
    local b = a + k
    local pg = item("m_dbg_pg"):read(b >> 8)
    t[#t + 1] = pg == 0 and "--" or string.format("%%02X", buf:read((pg - 1) * 256 + (b & 0xFF)))
  end
  return table.concat(t, " ")
end

_G.S2SIM_SUB = emu.add_machine_frame_notifier(function()
  frames = frames + 1
  if burstleft > 0 then
    snap(string.format("%%s_%%03d", burstname, burstk))
    burstk = burstk + 1
    burstleft = burstleft - 1
  end
  if watchleft > 0 then
    local v = hex(watcha, watchlen)
    if v ~= watchlast then print(string.format("WATCH %%d %%04X %%s", frames, watcha, v)); watchlast = v end
    watchleft = watchleft - 1
    if watchleft > 0 then return end
  end
  if teststage > 0 then
    if teststage == 1 then
      if rd(testhk) == 0x3C and rd(testhk + 1) == 0xC3 then
        wr(testhk + 1, testargs[1] + 1)
        wr(testhk + 2, testargs[2])
        wr(testhk + 3, testargs[3])
        wr(testhk, 0x5A)
        teststage = 2
      end
      return
    end
    if rd(testhk) == 0xA5 then
      print("TEST " .. frames); teststage = 0
    end
    return
  end
  if holdleft > 0 then
    holdleft = holdleft - 1
    if holdleft == 0 then setkeys(held, 0); relleft = 6 end
    return
  end
  if relleft > 0 then relleft = relleft - 1; return end
  if waitleft > 0 then waitleft = waitleft - 1; return end
  if untilleft > 0 then
    if rd(untila) == untilv then
      print(string.format("UNTIL %%04X %%02X %%d", untila, untilv, frames)); untilleft = 0
    else
      untilleft = untilleft - 1
      if untilleft == 0 then print(string.format("UNTIL %%04X timeout (%%02X)", untila, rd(untila))) end
      return
    end
  end
  while i <= #steps do
    local s = steps[i]; i = i + 1
    local op = s[1]
    if op == "wait" then waitleft = s[2]; return
    elseif op == "hold" then held = s[2]; setkeys(held, 1); holdleft = s[3]; return
    elseif op == "down" then setkeys(s[2], 1)
    elseif op == "up" then setkeys(s[2], 0)
    elseif op == "snap" then snap(s[2])
    elseif op == "burst" then burstname = s[2]; burstk = 0; burstleft = s[3]; return
    elseif op == "text" then text()
    elseif op == "sprites" then sprites()
    elseif op == "peek" then print(string.format("PEEK %%04X: %%s", s[2], hex(s[2], s[3])))
    elseif op == "speek" then print(string.format("SPEEK %%04X: %%s", s[2], shex(s[2], s[3])))
    elseif op == "poke" then for k = 3, #s do wr(s[2] + k - 3, s[k]) end
    elseif op == "status" then
      local t = {}
      for k = 0, s[3] - 1 do t[#t + 1] = string.format("%%02X", item("m_arena"):read(0x400 + s[2] + k)) end
      print(string.format("STATUS %%02X: %%s", s[2], table.concat(t, " ")))
    elseif op == "until" then untila, untilv, untilleft = s[2], s[3], s[4]; return
    elseif op == "watch" then watcha, watchleft, watchlen, watchlast = s[2], s[3], s[4], ""; return
    elseif op == "test" then testargs = { s[2], s[3], s[4] }; teststage = 1; return
    elseif op == "short" then print("SHORT " .. item("m_short_lines"):read(0))
    end
  end
  print("DONE " .. frames)
  manager.machine:exit()
end)
"""

SYMS = {}


def load_syms(lst):
    for line in open(lst, errors="replace"):
        for m in re.finditer(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*:\s*([0-9A-F]+) [C-]", line):
            SYMS.setdefault(m.group(1), int(m.group(2), 16))


def num(t):
    """A number ($hex, 0x.., decimal) or a symbol from the listing (NAME or NAME+n)."""
    base, _, add = t.partition("+")
    if base and (base[0].isalpha() or base[0] == "_") and not base.lower().startswith("0x"):
        if base not in SYMS:
            sys.exit("s2sim: no symbol %s" % base)
        return SYMS[base] + (int(add, 0) if add else 0)
    return int(t.replace("$", "0x"), 0)


def keys(spec):
    out = []
    for k in spec.split("+"):
        if k == "clear":
            out.append('{":CLEAR","Clear"}')
        elif re.fullmatch(r"[0-9]", k):
            out.append('{":A","A %s"}' % k)
        elif re.fullmatch(r"b[0-9]", k):
            out.append('{":B","B %s"}' % k[1])
        else:
            sys.exit("s2sim: no key %r" % k)
    return "{" + ",".join(out) + "}"


def render(hexs, path):
    """The 64x128 picture as a PNG at a TV's aspect: a pixel 10 wide, 3 tall."""
    from PIL import Image
    data = bytes.fromhex(hexs)
    im = Image.new("L", (640, 384))
    px = im.load()
    for y in range(128):
        for x in range(64):
            on = (data[y * 8 + x // 8] >> (7 - x % 8)) & 1
            if on:
                for dx in range(10):
                    for dy in range(3):
                        px[x * 10 + dx, y * 3 + dy] = 255
    im.save(path)


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    scen, outdir = sys.argv[1], sys.argv[2]
    image = sys.argv[3] if len(sys.argv) > 3 else os.path.join(HERE, "..", "build", "mule.st2")
    image = os.path.abspath(image)
    for lst in sorted(glob.glob(os.path.join(os.path.dirname(image), "mule.b*.lst"))):
        load_syms(lst)
    lines = (sys.stdin if scen == "-" else open(scen)).read().splitlines()
    steps = []
    for line in lines:
        line = line.split("#")[0].strip()
        if not line:
            continue
        w = line.split()
        op = w[0]
        if op == "wait":
            steps.append('{"wait",%d}' % int(w[1]))
        elif op == "hold":
            steps.append('{"hold",%s,%d}' % (keys(w[1]), int(w[2])))
        elif op in ("down", "up"):
            steps.append('{"%s",%s}' % (op, keys(w[1])))
        elif op == "press":
            steps.append('{"hold",%s,6}' % keys(w[1]))
        elif op == "snap":
            steps.append('{"snap","%s"}' % w[1])
        elif op == "burst":
            steps.append('{"burst","%s",%d}' % (w[1], int(w[2])))
        elif op in ("text", "sprites", "short"):
            steps.append('{"%s"}' % op)
        elif op in ("peek", "speek"):
            steps.append('{"%s",%d,%d}' % (op, num(w[1]), int(w[2]) if len(w) > 2 else 1))
        elif op == "status":
            steps.append('{"status",%d,%d}' % (num(w[1]), int(w[2]) if len(w) > 2 else 1))
        elif op == "poke":
            steps.append('{"poke",%d,%s}' % (num(w[1]), ",".join(str(num(x)) for x in w[2:])))
        elif op == "until":
            steps.append('{"until",%d,%d,%d}' % (num(w[1]), num(w[2]), int(w[3]) if len(w) > 3 else 3600))
        elif op == "watch":
            steps.append('{"watch",%d,%d,%d}' % (num(w[1]), int(w[2]), int(w[3]) if len(w) > 3 else 1))
        elif op == "test":
            a = [int(x) for x in w[1:]] + [0, 0]
            steps.append('{"test",%d,%d,%d}' % (a[0], a[1], a[2]))
        else:
            sys.exit("s2sim: no step %r" % op)
    os.makedirs(outdir, exist_ok=True)
    lua = LUA % {"steps": ",".join(steps), "testhk": SYMS.get("TESTHK", 0x0810)}
    luafile = os.path.abspath(os.path.join(outdir, "s2sim.lua"))
    open(luafile, "w").write(lua)
    env = dict(os.environ)
    env["FUJINET"] = "1"
    env["FUJINET_BOOT"] = image
    env["FUJINET_TCP"] = os.environ.get("S2SIM_FUJINET", "127.0.0.1:9")
    env["SDL_VIDEODRIVER"] = "dummy"
    throttle = [] if os.environ.get("S2SIM_THROTTLE") else ["-nothrottle"]
    out = os.path.abspath(outdir)
    cmd = ["./" + MAMEBIN, "studio2", "-rompath", ROMS, "-autoboot_script", luafile] + throttle + [
        "-video", "none", "-skip_gameinfo",
        "-cfg_directory", os.path.join(out, "cfg"), "-nvram_directory", os.path.join(out, "nvram"),
        "-snapshot_directory", out]
    wav = os.environ.get("S2SIM_WAV")
    cmd += ["-wavwrite", os.path.abspath(wav)] if wav else ["-sound", "none"]
    secs = int(os.environ.get("S2SIM_SECONDS", "600"))
    r = subprocess.run(["timeout", "-s", "KILL", str(secs)] + cmd, env=env, cwd=MAME,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    for line in r.stdout.splitlines():
        if line.startswith("SNAP "):
            _, name, frame, hexs = line.split(" ", 3)
            render(hexs, os.path.join(outdir, name + ".png"))
            print("SNAP %s %s" % (name, frame))
        else:
            print(line)


if __name__ == "__main__":
    main()

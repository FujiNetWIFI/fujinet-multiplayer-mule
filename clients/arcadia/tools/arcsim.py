#!/usr/bin/env python3
"""arcsim.py -- run the Arcadia client headless in MAME from a scenario.

    arcsim.py SCENARIO OUTDIR [IMAGE]

MAME's arcadia with the FujiNet cart device (`-cartslot fujinet`, grafted by
fujinet-firmware's pico/arcadia/emu/apply.sh). SCENARIO is a file of steps,
one per line (# comments); `-` reads it from stdin:

    wait N              N frames
    hold KEYS N         hold KEYS for N frames, then let go for 6
    press KEYS          hold 6 frames, let go 6
    down KEYS / up KEYS press and keep holding / let go (no wait)
                        (KEYS: up down left right fire 0-9 ent clr start opt
                        sel, joined by +)
    snap NAME           a screenshot, OUTDIR/NAME.png
    burst NAME N        a screenshot every frame for N frames, NAME_000 ...
    text                the screen as text (TEXT lines; '#' a block-graphics
                        row's cells, '@' a user glyph)
    peek ADDR [LEN]     print memory (any address the CPU sees; ADDR may be a
                        symbol from the listings, NAME or NAME+n: the boot
                        page's first, then the other pages')
    poke ADDR V ...     write memory (console RAM)
    until ADDR V [N]    wait (at most N frames, default 3600) until ADDR == V
    watch ADDR N [LEN]  for N frames, print ADDR (LEN bytes) when it changes
    test PH [ARG [PEV]] the offline test mode: at the title, phase PH, its
                        argument (event, stage, seat) and player event
    pc                  the PC
    reset               the console's RESET (a soft reset; MAME runs this
                        script again after it: the scenario starts over)

ARCSIM_FUJINET=host:port: the fujinet-pc to use (default: none, offline --
never the everyday one on 9995). ARCSIM_WAV=FILE records the sound.
ARCSIM_SECONDS: MAME is killed (SIGKILL) past it (default 600).
ARCSIM_THROTTLE=1: real time (a live server's clock is wall time). Prints its
lines prefixed (SNAP PEEK TEXT WATCH UNTIL PC DONE) for a caller to grep.
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
MAME = os.path.expanduser(os.environ.get("MAME", "~/Workspace/mame"))
TESTHK = None   # read from the listings: the title page's test hook

# port, mask
KEYS = {
    "left": (":joysticks", 0x01), "right": (":joysticks", 0x02),
    "down": (":joysticks", 0x04), "up": (":joysticks", 0x08),
    "1": (":controller1_col1", 0x08), "4": (":controller1_col1", 0x04),
    "7": (":controller1_col1", 0x02), "clr": (":controller1_col1", 0x01),
    "fire": (":controller1_col2", 0x08), "2": (":controller1_col2", 0x08),
    "5": (":controller1_col2", 0x04), "8": (":controller1_col2", 0x02),
    "0": (":controller1_col2", 0x01),
    "3": (":controller1_col3", 0x08), "6": (":controller1_col3", 0x04),
    "9": (":controller1_col3", 0x02), "ent": (":controller1_col3", 0x01),
    "start": (":panel", 0x01), "opt": (":panel", 0x02), "sel": (":panel", 0x04),
}

LUA = r"""
local steps = { %(steps)s }
local out = "%(out)s"
local testhk = %(testhk)d
local cpu = manager.machine.devices[":maincpu"]
local mem = cpu.spaces["program"]
local ports = manager.machine.ioport.ports
local frames, i, waitleft = 0, 1, 0
local held, holdleft, relleft = nil, 0, 0
local untila, untilv, untilleft = 0, 0, 0
local burstname, burstk, burstleft = "", 0, 0
local watcha, watchleft, watchlen, watchlast = 0, 0, 1, ""
local teststage, testargs = 0, nil

local function field(port, mask)
  local p = ports[port]
  if p == nil then return nil end
  for _, f in pairs(p.fields) do
    if f.mask == mask then return f end
  end
  return nil
end
local function setkeys(list, v)
  for _, k in ipairs(list) do
    local f = field(k[1], k[2])
    if f then f:set_value(v) end
  end
end
function snap(name)
  manager.machine.video:snapshot()
  print("SNAP " .. name .. " " .. frames)
end
local GL = " /\\#^|_|::::<><>0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ.,+$@@@@@@@@"
local function text()
  for r = 0, 25 do
    local base = r < 13 and (0x1800 + r * 16) or (0x1A00 + (r - 13) * 16)
    local s, gfx = "", false
    for c = 0, 15 do
      local v = mem:read_u8(base + c)
      if v == 0xC0 then gfx = true end
      if v == 0x40 then gfx = false end
      if gfx then s = s .. "#" else
        local g = v & 0x3F
        s = s .. string.sub(GL, g + 1, g + 1)
      end
    end
    print(string.format("TEXT %%2d |%%s|", r, s))
  end
end
local function hex(a, n)
  local t = {}
  for k = 0, n - 1 do t[#t + 1] = string.format("%%02X", mem:read_u8(a + k)) end
  return table.concat(t, " ")
end

_G.ARCSIM_SUB = emu.add_machine_frame_notifier(function()
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
      if mem:read_u8(testhk) == 0x3C and mem:read_u8(testhk + 1) == 0xC3 then
        mem:write_u8(testhk + 1, testargs[1] + 1)
        mem:write_u8(testhk + 2, testargs[2])
        mem:write_u8(testhk + 3, testargs[3])
        mem:write_u8(testhk, 0x5A)
        teststage = 2
      end
      return
    end
    if mem:read_u8(testhk) == 0xA5 then
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
    if mem:read_u8(untila) == untilv then
      print(string.format("UNTIL %%04X %%02X %%d", untila, untilv, frames)); untilleft = 0
    else
      untilleft = untilleft - 1
      if untilleft == 0 then print(string.format("UNTIL %%04X timeout (%%02X)", untila, mem:read_u8(untila))) end
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
    elseif op == "peek" then print(string.format("PEEK %%04X: %%s", s[2], hex(s[2], s[3])))
    elseif op == "poke" then for k = 3, #s do mem:write_u8(s[2] + k - 3, s[k]) end
    elseif op == "until" then untila, untilv, untilleft = s[2], s[3], s[4]; return
    elseif op == "watch" then watcha, watchleft, watchlen, watchlast = s[2], s[3], s[4], ""; return
    elseif op == "test" then testargs = { s[2], s[3], s[4] }; teststage = 1; return
    elseif op == "pc" then print(string.format("PC %%04X", cpu.state["PC"].value))
    elseif op == "reset" then print("RESET " .. frames); manager.machine:soft_reset()
    end
  end
  print("DONE " .. frames)
  manager.machine:exit()
end)
"""


SYMS = {}


def load_syms(image, page=0):
    import re
    lst = os.path.join(os.path.dirname(image), "mule_p%d.lst" % page)
    for line in open(lst, errors="replace"):
        for m in re.finditer(r"\b([A-Z_][A-Z0-9_]*)\s*:\s*([0-9A-F]+) [C-]", line):
            SYMS.setdefault(m.group(1), int(m.group(2), 16))


def num(t):
    """A number ($hex, 0x.., decimal) or a symbol from the listing (NAME or NAME+n)."""
    base, _, add = t.partition("+")
    if base and (base[0].isalpha() or base[0] == "_") and base.upper() == base and base not in ("0X",):
        if base not in SYMS:
            sys.exit("arcsim: no symbol %s" % base)
        return SYMS[base] + (int(add, 0) if add else 0)
    return int(t.replace("$", "0x"), 0)


def keys(spec):
    out = []
    for k in spec.split("+"):
        if k not in KEYS:
            sys.exit("arcsim: no key %r" % k)
        out.append('{"%s",%d}' % KEYS[k])
    return "{" + ",".join(out) + "}"


def testhook(image):
    """TESTHK from the listings (the title page's: build/mule_p*.lst)."""
    import glob
    import re
    for lst in sorted(glob.glob(os.path.join(os.path.dirname(image), "mule_p*.lst"))):
        for line in open(lst, errors="replace"):
            m = re.search(r"\bTESTHK\s*:\s*([0-9A-F]+)\b", line)
            if m:
                return int(m.group(1), 16)
    sys.exit("arcsim: no TESTHK in the listings")


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    scen, outdir = sys.argv[1], sys.argv[2]
    image = sys.argv[3] if len(sys.argv) > 3 else os.path.join(HERE, "..", "build", "mule.bin")
    image = os.path.abspath(image)
    load_syms(image)                    # the boot page's first, then each page's own
    import glob
    for lst in sorted(glob.glob(os.path.join(os.path.dirname(image), "mule_p*.lst"))):
        load_syms(image, int(lst.rsplit("_p", 1)[1].split(".")[0]))
    lines = (sys.stdin if scen == "-" else open(scen)).read().splitlines()
    steps = []
    for line in lines:
        line = line.split("#")[0].strip()
        if not line:
            continue
        w = line.split()
        op = w[0]
        if op in ("wait",):
            steps.append('{"wait",%d}' % int(w[1]))
        elif op == "hold":
            steps.append('{"hold",%s,%d}' % (keys(w[1]), int(w[2])))
        elif op in ("down", "up"):
            steps.append('{"%s",%s}' % (op, keys(w[1])))
        elif op == "press":
            steps.append('{"hold",%s,6}' % keys(w[1]))
        elif op in ("snap",):
            steps.append('{"snap","%s"}' % w[1])
        elif op == "burst":
            steps.append('{"burst","%s",%d}' % (w[1], int(w[2])))
        elif op in ("text", "pc", "reset"):
            steps.append('{"%s"}' % op)
        elif op == "peek":
            steps.append('{"peek",%d,%d}' % (num(w[1]), int(w[2]) if len(w) > 2 else 1))
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
            sys.exit("arcsim: no step %r" % op)
    os.makedirs(outdir, exist_ok=True)
    lua = LUA % {"steps": ",".join(steps), "out": outdir, "testhk": testhook(image)}
    luafile = os.path.join(outdir, "arcsim.lua")
    open(luafile, "w").write(lua)
    env = dict(os.environ)
    env["FUJINET_TCP"] = os.environ.get("ARCSIM_FUJINET", "127.0.0.1:9")
    throttle = [] if os.environ.get("ARCSIM_THROTTLE") else ["-nothrottle"]
    cmd = [os.path.join(MAME, "mame"), "arcadia", "-cartslot", "fujinet", "-cart", image,
           "-autoboot_script", luafile] + throttle + ["-video", "none",
           "-snapshot_directory", outdir, "-snapname", "%i",
           "-cfg_directory", os.path.join(outdir, "cfg"), "-nvram_directory", os.path.join(outdir, "nvram"),
           "-skip_gameinfo", "-nomouse"]
    wav = os.environ.get("ARCSIM_WAV")
    if wav:
        cmd += ["-wavwrite", wav]
    else:
        cmd += ["-sound", "none"]
    secs = int(os.environ.get("ARCSIM_SECONDS", "600"))
    try:
        r = subprocess.run(["timeout", "-s", "KILL", str(secs)] + cmd, env=env,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        sys.stdout.write(r.stdout)
    except KeyboardInterrupt:
        pass
    # MAME numbers its snapshots 0000.png, ... in order: give them their names
    names = [l.split()[1] for l in r.stdout.splitlines() if l.startswith("SNAP ")]
    shots = sorted(f for f in os.listdir(outdir) if f[:4].isdigit() and f.endswith(".png"))
    for f, n in zip(shots, names):
        os.replace(os.path.join(outdir, f), os.path.join(outdir, n + ".png"))


if __name__ == "__main__":
    main()

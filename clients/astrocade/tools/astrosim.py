#!/usr/bin/env python3
"""astrosim.py -- drive the Astrocade build in MAME from a scenario.

usage: astrosim.py SCENARIO OUTDIR [IMAGE [LISTING]]

Boots MAME's astrocde with the FujiNet cartridge device, presses keypad 1
at the on-board menu, waits for the game's title, then plays the scenario,
one command a line; times are in frames of the game's clock (FRAMES).
The scenario language is the other sims' (clients/mekkogx/tools/cocosim):

    test PHASE [ARG [PEVENT]]   offline test mode (test.inc): phase number,
                                colony event / auction stage / seat, player event
    wait N                      let N frames pass
    snap NAME                   save OUTDIR/NAME.png
    hold DIRS                   hold directions: any of u d l r (or 'none')
    press                       tap the button
    key CHAR|NUM                a key (13 end/=, 27 leave, 8 back)
    peek SYMBOL [N]             print N bytes at a symbol (from the listing)
    poke SYMBOL VALUE
    dump FROM TO FILE           save memory (hex addresses) to OUTDIR/FILE
    pad KEYPADn MASK            tap a real keypad key (e.g. KEYPAD3 0x10 = '1')
    pc N                        print the CPU's PC for N (MAME) frames
    trace FILE | trace off      log every instruction to OUTDIR/FILE (slow)
    show N SYMBOL[+OFF]...      for N frames (under 32768), print FRAMES and a byte at each
                                whenever one of them changes (SHOW)

The input goes through the game's test hook (INPHK in input.inc); symbols
come from the zmac listing's symbol table (build/mule.lst).

Environment:
    ASTROSIM_FUJINET=host:port  a fujinet-pc's BoIP listener (live run,
                                throttled: the server's clock is real time);
                                default 127.0.0.1:9, nothing there
    ASTROSIM_SECONDS=N          give up after N emulated seconds (600)
    ASTROSIM_VERBOSE=1          MAME's output too
"""
import os
import re
import subprocess
import sys

MAME = os.environ.get("MAME", os.path.expanduser("~/Workspace/mame"))
HERE = os.path.dirname(os.path.abspath(__file__))
TOP = os.path.dirname(HERE)


def symbols(lst):
    text = open(lst, encoding="utf-8", errors="replace").read()
    tail = text[text.rfind("Symbol Table:"):]
    syms = {}
    for name, val in re.findall(r"\b([a-z_][a-z0-9_]*)\s+=?\s*([0-9a-f]{1,5})\+?", tail):
        syms.setdefault(name.upper(), int(val, 16))
    return syms


DIRBITS = {"d": 1, "r": 2, "u": 4, "l": 8}


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    scen, out = sys.argv[1:3]
    image = sys.argv[3] if len(sys.argv) > 3 else os.path.join(TOP, "build/mule.bin")
    lst = sys.argv[4] if len(sys.argv) > 4 else os.path.join(TOP, "build/mule.lst")
    out = os.path.abspath(out)
    os.makedirs(out, exist_ok=True)
    syms = symbols(lst)
    def sym(n):                     # NAME, or NAME+OFFSET
        n, *off = n.split("+")
        return syms[n.upper()] + sum(int(x, 0) for x in off)
    steps = []
    for line in open(scen):
        line = line.split("#")[0].strip()
        if not line:
            continue
        w = line.split()
        c = w[0]
        if c == "test":
            args = [int(x) for x in w[1:]] + [0, 0, 0]
            steps.append(("test", sym("TESTHK"), args[0] + 1, args[1], args[2]))
        elif c == "wait":
            steps.append(("wait", int(w[1])))
        elif c == "snap":
            steps.append(("snap", w[1]))
        elif c == "hold":
            bits = 0 if w[1] == "none" else sum(DIRBITS[ch] for ch in w[1])
            steps.append(("poke", sym("INPHK"), 1))
            steps.append(("poke", sym("INPHKD"), bits))
        elif c == "press":
            steps.append(("poke", sym("INPHK"), 1))
            steps.append(("poke", sym("INPHKB"), 1))
            steps.append(("wait", 3))
            steps.append(("poke", sym("INPHKB"), 0))
        elif c == "key":
            k = int(w[1]) if w[1].isdigit() and len(w[1]) > 1 else ord(w[1][0])
            steps.append(("poke", sym("INPHK"), 1))
            steps.append(("poke", sym("INPHKK"), k))
            steps.append(("wait", 2))
        elif c == "show":
            steps.append(("show", int(w[1]), [sym(x) for x in w[2:]]))
        elif c == "peek":
            steps.append(("peek", w[1], sym(w[1]), int(w[2]) if len(w) > 2 else 1))
        elif c == "poke":
            steps.append(("poke", sym(w[1]), int(w[2], 0)))
        elif c == "dump":
            steps.append(("dump", int(w[1], 16), int(w[2], 16), w[3]))
        elif c == "pad":
            steps.append(("pad", w[1], int(w[2], 0)))
        elif c == "trace":
            steps.append(("trace", w[1]))
        elif c == "pc":
            steps.append(("pc", int(w[1]) if len(w) > 1 else 1))
        else:
            sys.exit("unknown command: " + line)

    lua = ["local steps = {"]
    for s in steps:
        if s[0] == "poke":
            lua.append('{"poke", %d, %d},' % (s[1], s[2]))
        elif s[0] == "wait":
            lua.append('{"wait", %d},' % s[1])
        elif s[0] == "snap":
            lua.append('{"snap", "%s"},' % s[1])
        elif s[0] == "peek":
            lua.append('{"peek", "%s", %d, %d},' % (s[1], s[2], s[3]))
        elif s[0] == "dump":
            lua.append('{"dump", %d, %d, "%s"},' % (s[1], s[2], os.path.join(out, s[3])))
        elif s[0] == "pad":
            lua.append('{"pad", "%s", %d},' % (s[1], s[2]))
        elif s[0] == "trace":
            f = "off" if s[1] == "off" else os.path.join(out, s[1])
            lua.append('{"trace", "%s"},' % f)
        elif s[0] == "test":
            lua.append('{"test", %d, %d, %d, %d},' % s[1:])
        elif s[0] == "pc":
            lua.append('{"pc", %d},' % s[1])
        elif s[0] == "show":
            lua.append('{"show", %d, {%s}},' % (s[1], ", ".join(str(a) for a in s[2])))
    lua.append("}")
    lua.append("""
local fc = %d
local out = "%s"
local mem = manager.machine.devices[":maincpu"].spaces["program"]
local function port(suffix)
  for tag, p in pairs(manager.machine.ioport.ports) do
    if tag:sub(-#suffix) == suffix then return p end
  end
end
local started = false
local i = 1
local wait_until = nil
local frames = 0
local asked = false
local untap = {}
local pcdump = 0
local show_last = ""
emu.register_frame_done(function()
  frames = frames + 1
  for k, u in pairs(untap) do
    if frames >= u[1] then u[2]:field(u[3]):clear_value(); untap[k] = nil end
  end
  if frames == 150 then local p = port("KEYPAD3"); p:field(0x10):set_value(1)
    untap[#untap + 1] = {frames + 8, p, 0x10} end
  if frames > 60 * %d then manager.machine:exit() end
  local now = mem:read_u16(fc)
  if pcdump > 0 then
    local st = manager.machine.devices[":maincpu"].state
    print(string.format("PC %%04X SP %%04X page %%d frames %%d", st["PC"].value, st["SP"].value, mem:read_u8(%d), now))
    pcdump = pcdump - 1
    return
  end
  if not started then
    local s = steps[1]
    if s ~= nil and s[1] == "test" then
      local h0 = mem:read_u8(s[2])
      if h0 == 0xA5 and asked then started = true; i = 2
      elseif not asked and h0 == 0x3C and mem:read_u8(s[2] + 1) == 0xC3 then
        mem:write_u8(s[2] + 1, s[3]); mem:write_u8(s[2] + 2, s[4])
        mem:write_u8(s[2] + 3, s[5]); mem:write_u8(s[2], 0x5A)
        asked = true
        return
      else return end
    elseif frames > 200 then started = true
    else return end
  end
  while i <= #steps do
    local s = steps[i]
    if s[1] == "wait" then
      if wait_until == nil then wait_until = (now + s[2]) & 0xFFFF end
      if ((now - wait_until) & 0xFFFF) < 0x8000 then wait_until = nil; i = i + 1 else return end
    elseif s[1] == "poke" then
      mem:write_u8(s[2], s[3]); i = i + 1
    elseif s[1] == "snap" then
      manager.machine.video:snapshot()
      print("SNAP " .. s[2] .. " game frame " .. now .. " mame frame " .. frames)
      local f = io.open(out .. "/snaps.txt", "a"); f:write(s[2] .. "\\n"); f:close()
      i = i + 1
      return
    elseif s[1] == "dump" then
      local f = io.open(s[4], "wb")
      for a = s[2], s[3] do f:write(string.char(mem:read_u8(a))) end
      f:close(); i = i + 1
    elseif s[1] == "pad" then
      local p = port(s[2]); p:field(s[3]):set_value(1)
      untap[#untap + 1] = {frames + 8, p, s[3]}
      i = i + 1
      return
    elseif s[1] == "trace" then
      if s[2] == "off" then manager.machine.debugger:command("trace off")
      else manager.machine.debugger:command("trace " .. s[2] .. ",0") end
      i = i + 1
    elseif s[1] == "pc" then
      pcdump = s[2]; i = i + 1; return
    elseif s[1] == "show" then
      if wait_until == nil then wait_until = (now + s[2]) & 0xFFFF; show_last = "" end
      local t = {}
      for _, a in ipairs(s[3]) do t[#t + 1] = string.format("%%d", mem:read_u8(a)) end
      local v = table.concat(t, " ")
      if v ~= show_last then print("SHOW " .. now .. " " .. v); show_last = v end
      if ((now - wait_until) & 0xFFFF) < 0x8000 then wait_until = nil; i = i + 1 else return end
    elseif s[1] == "peek" then
      local t = {}
      for k = 0, s[4] - 1 do t[#t + 1] = string.format("%%02X", mem:read_u8(s[3] + k)) end
      print("PEEK " .. s[2] .. " " .. table.concat(t, " ") .. "  (game frame " .. now .. ")")
      i = i + 1
    end
  end
  manager.machine:exit()
end, "astrosim")
""" % (sym("FRAMES"), out, int(os.environ.get("ASTROSIM_SECONDS", "600")), sym("CURPG")))
    luafile = os.path.join(out, "scenario.lua")
    open(luafile, "w").write("\n".join(lua))
    snapdir = os.path.join(out, "raw")
    subprocess.run(["rm", "-rf", snapdir, os.path.join(out, "snaps.txt")])
    cfgdir = os.path.join(out, "cfg")
    os.makedirs(cfgdir, exist_ok=True)
    live = os.environ.get("ASTROSIM_FUJINET")
    env = dict(os.environ, FUJINET_TCP=live or "127.0.0.1:9")
    cmd = [os.path.join(MAME, "mame"), "astrocde", "-rompath", os.path.join(MAME, "roms"),
           "-cartslot", "fujinet", "-cart", os.path.abspath(image),
           "-video", "none", "-sound", "none", "-snapshot_directory", snapdir,
           "-cfg_directory", cfgdir, "-nvram_directory", os.path.join(out, "nvram"),
           "-autoboot_script", luafile, "-skip_gameinfo"]
    if not live:
        cmd.append("-nothrottle")
    if any(x[0] == "trace" for x in steps):
        cmd[2:2] = ["-debug", "-debugger", "none"]
    r = subprocess.run(cmd, capture_output=True, text=True, env=env, cwd=out,
                       timeout=int(os.environ.get("ASTROSIM_SECONDS", "600")) + 300)
    if os.environ.get("ASTROSIM_VERBOSE"):
        print(r.stdout, r.stderr)
    for line in r.stdout.splitlines():
        if line.startswith(("PEEK", "SNAP", "PC ", "SHOW")):
            print(line)
    names = []
    if os.path.exists(os.path.join(out, "snaps.txt")):
        names = open(os.path.join(out, "snaps.txt")).read().split()
    d = os.path.join(snapdir, "astrocde")
    shots = sorted(os.listdir(d)) if os.path.isdir(d) else []
    for n, f in zip(names, shots):
        os.replace(os.path.join(d, f), os.path.join(out, n + ".png"))
        print(os.path.join(out, n + ".png"))


if __name__ == "__main__":
    main()

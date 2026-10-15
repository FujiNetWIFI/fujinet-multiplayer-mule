#!/usr/bin/env python3
"""chfsim.py -- drive the Channel F build in MAME from a scenario.

usage: chfsim.py SCENARIO OUTDIR [IMAGE [LISTING]]

Boots MAME's channelf with the FujiNet cartridge device and plays the
scenario, one command a line. Times are MAME frames (the console's own,
60.53 a second). The language is the other sims' (astrocade's astrosim.py):

    test PHASE [ARG [PEVENT]]   offline test mode (test.inc): phase number,
                                colony event / auction stage / seat, player event
    wait N                      let N frames pass
    snap NAME                   save OUTDIR/NAME.png
    hold DIRS                   hold directions: any of u d l r (or 'none')
    press                       tap the button
    key CHAR|NUM                a key (13 end, 27 leave, 8 back, '0' sit out)
    peek SYMBOL|ADDR [N]        print N bytes
    poke SYMBOL|ADDR VALUE
    dump FROM TO FILE           save memory (hex addresses) to OUTDIR/FILE
    vram FILE                   save the 128 x 64 screen (a byte a pixel)
    watch ADDR [N [LEN]]        print the PC of the next N writes to ADDR..+LEN
    vcheck                      VRAM against the shadow; 'untagged differ' are
                                the real errors (tagged pixels show a sprite)
    tick                        print the cart's tick and the game's FRAMES
    page                        print the page mapped
    pc N                        print the CPU's PC for N frames
    trace FILE | trace off      log every instruction to OUTDIR/FILE (slow)
    traceon ADDR FILE [N [V]]   the same from the first write (of V) to ADDR, N frames

Input goes through the game's test hook (INPHK in input.inc); symbols come
from the listing's symbol table (build/mule.lst).

Environment:
    CHFSIM_FUJINET=host:port    a fujinet-pc's BoIP listener (live run,
                                throttled: the server's clock is real time);
                                default 127.0.0.1:9, nothing there
    CHFSIM_SECONDS=N            give up after N emulated seconds (600)
    CHFSIM_VERBOSE=1            MAME's output too
"""
import os
import re
import subprocess
import sys

MAME = os.environ.get("MAME", os.path.expanduser("~/Workspace/mame"))
HERE = os.path.dirname(os.path.abspath(__file__))
TOP = os.path.dirname(HERE)


def symbols(lst):
    """AS's listing: 'NAME : VALUE C |' entries in the symbol table."""
    text = open(lst, encoding="latin-1").read()
    syms = {}
    for name, val in re.findall(r"([A-Z_][A-Z0-9_]*)\s*:\s*([0-9A-F]+)\s+\S\s*\|", text):
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

    def sym(n):
        """A number, a symbol, or a sum of them (ST+389)."""
        total = 0
        for part in n.split("+"):
            try:
                total += int(part, 0)
            except ValueError:
                total += syms[part.upper()]
        return total

    steps = []
    for line in open(scen):
        line = line.split("#")[0].strip()
        if not line:
            continue
        w = line.split()
        c = w[0]
        if c == "test":
            args = [int(x) for x in w[1:]] + [0, 0, 0]
            steps.append('{"test", %d, %d, %d, %d}' % (sym("TESTHK"), args[0] + 1, args[1], args[2]))
        elif c == "wait":
            steps.append('{"wait", %d}' % int(w[1]))
        elif c == "snap":
            steps.append('{"snap", "%s"}' % w[1])
        elif c == "hold":
            bits = 0 if w[1] == "none" else sum(DIRBITS[ch] for ch in w[1])
            steps.append('{"poke", %d, 1}' % sym("INPHK"))
            steps.append('{"poke", %d, %d}' % (sym("INPHKD"), bits))
        elif c == "press":
            steps.append('{"poke", %d, 1}' % sym("INPHK"))
            steps.append('{"poke", %d, 1}' % sym("INPHKB"))
            steps.append('{"wait", 3}')
            steps.append('{"poke", %d, 0}' % sym("INPHKB"))
        elif c == "key":
            k = int(w[1]) if w[1].isdigit() and len(w[1]) > 1 else ord(w[1][0])
            steps.append('{"poke", %d, 1}' % sym("INPHK"))
            steps.append('{"poke", %d, %d}' % (sym("INPHKK"), k))
            steps.append('{"wait", 2}')
        elif c == "peek":
            steps.append('{"peek", "%s", %d, %d}' % (w[1], sym(w[1]), int(w[2]) if len(w) > 2 else 1))
        elif c == "poke":
            steps.append('{"poke", %d, %d}' % (sym(w[1]), int(w[2], 0)))
        elif c == "dump":
            steps.append('{"dump", %d, %d, "%s"}' % (int(w[1], 16), int(w[2], 16), os.path.join(out, w[3])))
        elif c == "vram":
            steps.append('{"vram", "%s"}' % os.path.join(out, w[1]))
        elif c == "watch":
            steps.append('{"watch", %d, %d, %d}' % (sym(w[1]), int(w[2]) if len(w) > 2 else 20,
                                                   int(w[3], 0) if len(w) > 3 else 1))
        elif c == "traceon":
            steps.append('{"traceon", %d, "%s", %d, %d}' % (sym(w[1]), os.path.join(out, w[2]), int(w[3]) if len(w) > 3 else 10,
                                                           int(w[4], 0) if len(w) > 4 else -1))
        elif c == "vcheck":
            steps.append('{"vcheck"}')
        elif c == "tick":
            steps.append('{"tick"}')
        elif c == "page":
            steps.append('{"page"}')
        elif c == "trace":
            f = "off" if w[1] == "off" else os.path.join(out, w[1])
            steps.append('{"trace", "%s"}' % f)
        elif c == "pc":
            steps.append('{"pc", %d}' % (int(w[1]) if len(w) > 1 else 1))
        else:
            sys.exit("unknown command: " + line)

    lua = "local steps = {\n" + ",\n".join(steps) + "\n}\n" + r"""
local out = "%(out)s"
local FRAMES, CURPG, SHADOW = %(frames)d, %(curpg)d, 0x8000
local mem = manager.machine.devices[":maincpu"].spaces["program"]
local vram = manager.machine.memory.regions[":vram"]
local i, frames, wait_until, pcdump, asked, started = 1, 0, nil, 0, false, false
traceoff = nil
_G._chfsim = emu.add_machine_frame_notifier(function()
  frames = frames + 1
  if traceoff and frames >= traceoff then manager.machine.debugger:command("trace off"); traceoff = nil end
  if frames > 60 * %(secs)d then manager.machine:exit() end
  if pcdump > 0 then
    local st = manager.machine.devices[":maincpu"].state
    print(string.format("PC %%04X DC0 %%04X page %%d", st["PC0"].value, st["DC0"].value, mem:readv_u8(CURPG)))
    pcdump = pcdump - 1
    return
  end
  if not started then
    local s = steps[1]
    if s ~= nil and s[1] == "test" then
      local h0 = mem:readv_u8(s[2])
      if h0 == 0xA5 and asked then started = true; i = 2
      elseif not asked and h0 == 0x3C and mem:readv_u8(s[2] + 1) == 0xC3 then
        mem:write_u8(s[2] + 1, s[3]); mem:write_u8(s[2] + 2, s[4])
        mem:write_u8(s[2] + 3, s[5]); mem:write_u8(s[2], 0x5A)
        asked = true
        return
      else
        if frames > 3000 then print("CHFSIM: the title never asked for the test hook"); manager.machine:exit() end
        return
      end
    else started = true end
  end
  while i <= #steps do
    local s = steps[i]
    if s[1] == "wait" then
      if wait_until == nil then wait_until = frames + s[2] end
      if frames >= wait_until then wait_until = nil; i = i + 1 else return end
    elseif s[1] == "poke" then
      mem:write_u8(s[2], s[3]); i = i + 1
    elseif s[1] == "snap" then
      manager.machine.video:snapshot()
      print("SNAP " .. s[2] .. " frame " .. frames)
      local f = io.open(out .. "/snaps.txt", "a"); f:write(s[2] .. "\n"); f:close()
      i = i + 1
      return
    elseif s[1] == "dump" then
      local f = io.open(s[4], "wb")
      for a = s[2], s[3] do f:write(string.char(mem:readv_u8(a))) end
      f:close(); i = i + 1
    elseif s[1] == "vram" then
      local f = io.open(s[2], "wb")
      for a = 0, 128 * 64 - 1 do f:write(string.char(vram:read_u8(a) & 3)) end
      f:close(); i = i + 1
    elseif s[1] == "watch" then
      local left = s[3]
      _G["_chfwatch" .. s[2]] = mem:install_write_tap(s[2], s[2] + s[4] - 1, "chfwatch" .. s[2], function(off, data, mask)
        if left > 0 then
          left = left - 1
          local st = manager.machine.devices[":maincpu"].state
          local ks, regs = "", nil
          for k, v in pairs(manager.machine.memory.shares) do if k:find("regs") then regs = v end end
          if regs then
            local sp = regs:read_u8(59)
            for r = 0, 8 do ks = ks .. string.format("r%%d=%%02X ", r, regs:read_u8(r)) end
            ks = ks .. string.format("ksp=%%02X ", sp)
            for r = 16, sp - 2, 2 do ks = ks .. string.format("%%02X%%02X ", regs:read_u8(r), regs:read_u8(r + 1)) end
          end
          print(string.format("WATCH %%04X <- %%02X  PC %%04X PC1 %%04X K %%04X page %%d frame %%d  kstack %%s", off, data, st["PC0"].value,
                st["PC1"].value, st["K"] and st["K"].value or 0, mem:readv_u8(CURPG), frames, ks))
        end
        return data
      end)
      i = i + 1
    elseif s[1] == "traceon" then
      local armed = true
      _G["_chftron" .. s[2]] = mem:install_write_tap(s[2], s[2], "chftron" .. s[2], function(off, data, mask)
        if armed and (s[5] < 0 or data == s[5]) then
          armed = false
          manager.machine.debugger:command("trace " .. s[3] .. ",0")
          traceoff = frames + s[4]
          print(string.format("TRACEON %%04X <- %%02X frame %%d", off, data, frames))
        end
        return data
      end)
      i = i + 1
    elseif s[1] == "vcheck" then
      local bad, tags, ubad, first = 0, 0, 0, nil
      for y = 0, 63 do for x = 0, 127 do
        local v, sh = vram:read_u8(y * 128 + x) & 3, mem:readv_u8(SHADOW + y * 128 + x)
        if sh > 3 then tags = tags + 1 end
        if v ~= (sh & 3) then
          bad = bad + 1
          if sh < 4 then ubad = ubad + 1 end
          if first == nil and sh < 4 then first = string.format("(%%d,%%d) vram %%d shadow %%d", x, y, v, sh) end
        end
      end end
      print("VCHECK " .. bad .. " differ, " .. tags .. " tagged, " .. ubad .. " untagged differ" ..
            (first and (", first " .. first) or "") .. "  (frame " .. frames .. ")")
      i = i + 1
    elseif s[1] == "tick" then
      print(string.format("TICK cart %%d FRAMES %%d mame %%d", mem:readv_u8(0xFC0E) + 256 * mem:readv_u8(0xFC0F),
            mem:readv_u8(FRAMES) + 256 * mem:readv_u8(FRAMES + 1), frames))
      i = i + 1
    elseif s[1] == "page" then
      print("PAGE " .. mem:readv_u8(CURPG)); i = i + 1
    elseif s[1] == "trace" then
      if s[2] == "off" then manager.machine.debugger:command("trace off")
      else manager.machine.debugger:command("trace " .. s[2] .. ",0") end
      i = i + 1
    elseif s[1] == "pc" then
      pcdump = s[2]; i = i + 1; return
    elseif s[1] == "peek" then
      local t = {}
      for k = 0, s[4] - 1 do t[#t + 1] = string.format("%%02X", mem:readv_u8(s[3] + k)) end
      print("PEEK " .. s[2] .. " " .. table.concat(t, " ") .. "  (frame " .. frames .. ")")
      i = i + 1
    end
  end
  if i > #steps then i = i + 1; if i > #steps + 2 then manager.machine:exit() end end
end)
""" % {"out": out, "frames": sym("FRAMES"), "curpg": sym("CURPG"),
       "secs": int(os.environ.get("CHFSIM_SECONDS", "600"))}
    luafile = os.path.join(out, "scenario.lua")
    open(luafile, "w").write(lua)
    snapdir = os.path.join(out, "raw")
    subprocess.run(["rm", "-rf", snapdir, os.path.join(out, "snaps.txt")])
    cfgdir = os.path.join(out, "cfg")
    os.makedirs(cfgdir, exist_ok=True)
    live = os.environ.get("CHFSIM_FUJINET")
    env = dict(os.environ, FUJINET_TCP=live or "127.0.0.1:9",
               SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    cmd = ["./mame", "channelf", "-bios", "sl31253", "-cartslot", "fujinet",
           "-cart", os.path.abspath(image), "-video", "none", "-sound", "none",
           "-snapshot_directory", snapdir, "-cfg_directory", cfgdir,
           "-nvram_directory", os.path.join(out, "nvram"),
           "-autoboot_script", luafile, "-skip_gameinfo"]
    if not live:
        cmd.append("-nothrottle")
    if any('"trace"' in x or '"traceon"' in x for x in steps):
        cmd[2:2] = ["-debug", "-debugger", "none"]
    r = subprocess.run(cmd, capture_output=True, text=True, env=env, cwd=MAME,
                       timeout=int(os.environ.get("CHFSIM_SECONDS", "600")) + 300)
    if os.environ.get("CHFSIM_VERBOSE"):
        print(r.stdout, r.stderr)
    for line in r.stdout.splitlines():
        if line.startswith(("PEEK", "SNAP", "PC ", "VCHECK", "TICK", "PAGE", "CHFSIM", "WATCH", "TRACEON")):
            print(line)
    names = []
    if os.path.exists(os.path.join(out, "snaps.txt")):
        names = open(os.path.join(out, "snaps.txt")).read().split()
    d = os.path.join(snapdir, "channelf")
    shots = sorted(os.listdir(d)) if os.path.isdir(d) else []
    for n, f in zip(names, shots):
        os.replace(os.path.join(d, f), os.path.join(out, n + ".png"))
        print(os.path.join(out, n + ".png"))


if __name__ == "__main__":
    main()

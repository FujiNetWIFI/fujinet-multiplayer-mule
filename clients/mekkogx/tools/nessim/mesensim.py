#!/usr/bin/env python3
"""mesensim.py -- drive the NES build in MesenCE from a nessim scenario.

usage: mesensim.py CART.nes MAPFILE SCENARIO OUTDIR [SETTING=VALUE ...]

nessim.py's twin for what MAME doesn't model: MesenCE (the FujiNet Go NES
core) emulates the 2C02's OAM corruption -- by default the pre-render
line's copy of OAM row 0 over the row OAM2ADDR was left at when rendering
went off mid-screen (DisableOamAddrBug off), and with
Nes.EnablePpuOamRowCorruption=true the rows the switch itself corrupts.
It runs the image headless (Mesen --testrunner, as fast as it goes, the
user's MesenCE settings, saved back never) and plays the scenario, the
offline test mode only (no FujiNet cartridge here):

    test PHASE [ARG [PEVENT]]   offline test mode (src/test.c)
    wait N                      let N frames of the game's clock pass
    snap NAME                   save OUTDIR/NAME.png
    hold DIRS                   hold directions: any of u d l r (or 'none')
    press                       tap the button
    poke SYMBOL[+OFFSET] VALUE

Every frame the picture shows, the sprites the PPU holds at the first
scanline interrupt (after the pre-render line, before the next DMA) are
compared with the page the game sent (oam_show): OUTDIR/oam.txt lists the
frames whose rows differ, and the summary line says how many. The exit
status is 1 if any did.

Extra arguments are MesenCE settings (Nes.EnablePpuOamRowCorruption=true).
"""
import os
import re
import subprocess
import sys

MESEN = os.environ.get("MESEN", os.path.expanduser(
    "~/Workspace/MesenCE/bin/linux-x64/Release/Mesen"))
DIRBITS = {"d": 1, "r": 2, "u": 4, "l": 8}


def symbols(mapfile):
    syms = {}
    text = open(mapfile).read()
    part = text.split("Exports list by name:")[1].split("Exports list by value:")[0]
    for name, addr in re.findall(r"(\S+)\s+([0-9A-F]{6})\s+\S+", part):
        syms[name] = int(addr, 16)
    return syms


def main():
    if len(sys.argv) < 5:
        sys.exit(__doc__)
    cart, mapfile, scen, out = sys.argv[1:5]
    settings = sys.argv[5:]
    out = os.path.abspath(out)
    os.makedirs(out, exist_ok=True)
    syms = symbols(mapfile)
    def sym(n):                     # NAME, or NAME+OFFSET[+OFFSET...]
        n, *off = n.split("+")
        return (syms["_" + n] if "_" + n in syms else syms[n]) + sum(int(x, 0) for x in off)
    steps = []
    for line in open(scen):
        line = line.split("#")[0].strip()
        if not line:
            continue
        w = line.split()
        c = w[0]
        if c == "test":
            a = [int(x) for x in w[1:]] + [0, 0, 0]
            steps.append('{"test", %d, %d, %d, %d}' % (sym("test_hook"), a[0] + 1, a[1], a[2]))
        elif c == "wait":
            steps.append('{"wait", %d}' % int(w[1]))
        elif c == "snap":
            steps.append('{"snap", "%s"}' % w[1])
        elif c == "hold":
            bits = 0 if w[1] == "none" else sum(DIRBITS[ch] for ch in w[1])
            steps.append('{"poke", %d, 1}' % sym("inp_hook"))
            steps.append('{"poke", %d, %d}' % (sym("inp_hook_dir"), bits))
        elif c == "press":
            steps.append('{"poke", %d, 1}' % sym("inp_hook"))
            steps.append('{"poke", %d, 1}' % sym("inp_hook_btn"))
            steps.append('{"wait", 3}')
            steps.append('{"poke", %d, 0}' % sym("inp_hook_btn"))
        elif c == "poke":
            steps.append('{"poke", %d, %d}' % (sym(w[1]), int(w[2], 0)))
        else:
            sys.exit("unknown command: " + line)

    lua = """
local steps = { %s }
local out = "%s"
local FC, SHOW = %d, %d
local function rd(a) return emu.read(a, emu.memType.nesDebug, false) end
local function wr(a, v) emu.write(a, v, emu.memType.nesDebug) end
local log = io.open(out .. "/oam.txt", "w")
local started, asked, i, wait_until, frames = false, false, 1, nil, 0
local checked, bad, rows, irq_done = 0, 0, 0, true
local function finish(code)
  log:write(string.format("SUMMARY checked %%d frames, %%d with rows differing (%%d rows)\\n",
                          checked, bad, rows))
  log:close()
  emu.stop(code)
end
-- the first scanline interrupt of a frame: the PPU's OAM against the page
emu.addEventCallback(function()
  if irq_done or not started then return end
  irq_done = true
  local page = rd(SHOW) * 256
  local diff = {}
  for r = 0, 31 do
    for b = 0, 7 do
      local a, v = rd(page + r * 8 + b), emu.read(r * 8 + b, emu.memType.nesSpriteRam, false)
      if b %% 4 == 2 then a = a & 0xE3 end   -- (attribute bits 2-4 aren't there)
      if a ~= v then diff[#diff + 1] = r; break end
    end
  end
  checked = checked + 1
  if #diff > 0 then
    bad = bad + 1
    rows = rows + #diff
    log:write(string.format("frame %%d rows %%s\\n", rd(FC) + 256 * rd(FC + 1), table.concat(diff, ",")))
  end
end, emu.eventType.irq)
emu.addEventCallback(function()
  irq_done = false
  frames = frames + 1
  if frames > 60 * 600 then finish(2) return end
  local now = rd(FC) + 256 * rd(FC + 1)
  if not started then
    local s = steps[1]
    if s ~= nil and s[1] == "test" then
      local h0 = rd(s[2])
      if h0 == 0xA5 and asked then started = true; i = 2
      elseif not asked and h0 == 0x3C and rd(s[2] + 1) == 0xC3 then
        wr(s[2] + 1, s[3]); wr(s[2] + 2, s[4]); wr(s[2] + 3, s[5]); wr(s[2], 0x5A)
        asked = true
        return
      else return end
    elseif frames > 60 * 25 then started = true
    else return end
  end
  while i <= #steps do
    local s = steps[i]
    if s[1] == "wait" then
      if wait_until == nil then wait_until = (now + s[2]) & 0xFFFF end
      if ((now - wait_until) & 0xFFFF) < 0x8000 then wait_until = nil; i = i + 1 else return end
    elseif s[1] == "poke" then
      wr(s[2], s[3]); i = i + 1
    elseif s[1] == "snap" then
      local f = io.open(out .. "/" .. s[2] .. ".png", "wb")
      f:write(emu.takeScreenshot()); f:close()
      i = i + 1
      return
    else
      i = i + 1
    end
  end
  finish(bad > 0 and 1 or 0)
end, emu.eventType.startFrame)
""" % (",\n  ".join(steps), out, sym("frame_count"), sym("oam_show"))
    luafile = os.path.join(out, "scenario.lua")
    open(luafile, "w").write(lua)
    cmd = [MESEN, "--testrunner", os.path.abspath(cart), luafile, "--timeout=600",
           "--Debug.ScriptWindow.AllowIoOsAccess=true"] + ["--" + s for s in settings]
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=700)
    res = os.path.join(out, "oam.txt")
    summary = [l for l in open(res)] if os.path.exists(res) else []
    for l in summary:
        if l.startswith("SUMMARY"):
            print(l.rstrip())
    if r.returncode not in (0, 1):
        print("mesen exit", r.returncode, r.stdout[-2000:], r.stderr[-2000:])
    sys.exit(r.returncode)


if __name__ == "__main__":
    main()

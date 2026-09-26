#!/usr/bin/env python3
"""lynxsim.py -- drive the Lynx build in MAME from a scenario.

usage: lynxsim.py CART.lnx MAPFILE SCENARIO OUTDIR

Runs the cart in MAME's Lynx (no window, as fast as it goes; MAME needs
lynx.zip, the boot ROM, in its roms/; its own settings go to OUTDIR/cfg),
waits for the game's title (frame_count starts counting), then plays the
scenario, one command a line; times are in frames of the game's clock:

    test PHASE [ARG [PEVENT]]   offline test mode (src/test.c): phase number,
                                colony event / auction stage / seat, player event
    wait N                      let N frames pass
    snap NAME                   save OUTDIR/NAME.png
    hold DIRS                   hold directions: any of u d l r (or 'none')
    press                       tap the button
    key CHAR|NUM                type a key (RETURN = 13, ESC = 27)
    peek SYMBOL [N]             print N bytes at a symbol (from the map)
    pc N                        print the CPU's PC for N frames, then quit
    trace FILE | trace off      log every instruction to OUTDIR/FILE (slow)
    dump FROM TO FILE           save memory (hex addresses) to OUTDIR/FILE
    type TEXT                   type on the real keyboard (\\n RETURN, \\s space)
    poke SYMBOL VALUE

The input goes through the game's test hook (inp_hook in input.c), the
addresses come from the linker map. LYNXWAV=out.wav records the sound.
MAME's Lynx has no ComLynx to the outside: only the offline test mode runs
here (tools/a2sim's scenario language, same commands).
"""
import os
import re
import subprocess
import sys

MAME = os.environ.get("MAME", os.path.expanduser("~/Workspace/mame"))


def symbols(mapfile):
    syms = {}
    text = open(mapfile).read()
    part = text.split("Exports list by name:")[1].split("Exports list by value:")[0]
    for name, addr in re.findall(r"(\S+)\s+([0-9A-F]{6})\s+\S+", part):
        syms[name] = int(addr, 16)
    return syms


DIRBITS = {"d": 1, "r": 2, "u": 4, "l": 8}


def main():
    disk, mapfile, scen, out = sys.argv[1:5]
    out = os.path.abspath(out)
    os.makedirs(out, exist_ok=True)
    syms = symbols(mapfile)
    sym = lambda n: syms["_" + n] if "_" + n in syms else syms[n]
    steps = []
    for line in open(scen):
        line = line.split("#")[0].strip()
        if not line:
            continue
        w = line.split()
        c = w[0]
        if c == "test":
            args = [int(x) for x in w[1:]] + [0, 0, 0]
            steps.append(("test", sym("test_hook"), args[0] + 1, args[1], args[2]))
        elif c == "wait":
            steps.append(("wait", int(w[1])))
        elif c == "snap":
            steps.append(("snap", w[1]))
        elif c == "hold":
            bits = 0 if w[1] == "none" else sum(DIRBITS[ch] for ch in w[1])
            steps.append(("poke", sym("inp_hook"), 1))
            steps.append(("poke", sym("inp_hook_dir"), bits))
        elif c == "press":
            steps.append(("poke", sym("inp_hook"), 1))
            steps.append(("poke", sym("inp_hook_btn"), 1))
            steps.append(("wait", 3))
            steps.append(("poke", sym("inp_hook_btn"), 0))
        elif c == "key":
            k = int(w[1]) if w[1].isdigit() and len(w[1]) > 1 else ord(w[1][0])
            steps.append(("poke", sym("inp_hook"), 1))
            steps.append(("poke", sym("inp_hook_key"), k))
            steps.append(("wait", 2))
        elif c == "peek":
            steps.append(("peek", w[1], sym(w[1]), int(w[2]) if len(w) > 2 else 1))
        elif c == "dump":
            steps.append(("dump", int(w[1], 16), int(w[2], 16), w[3]))
        elif c == "trace":
            steps.append(("trace", w[1]))
        elif c == "type":
            steps.append(("type", line.split(None, 1)[1].replace("\\n", "\n").replace("\\s", " ")))
        elif c == "pc":
            steps.append(("pc", int(w[1]) if len(w) > 1 else 1))
        elif c == "poke":
            steps.append(("poke", sym(w[1]), int(w[2], 0)))
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
        elif s[0] == "pc":
            lua.append('{"pc", %d},' % s[1])
        elif s[0] == "dump":
            lua.append('{"dump", %d, %d, "%s"},' % (s[1], s[2], os.path.join(out, s[3])))
        elif s[0] == "type":
            lua.append('{"type", %s},' % ('"' + s[1].replace('"', '\\"').replace("\n", "\\n") + '"'))
        elif s[0] == "trace":
            f = "off" if s[1] == "off" else os.path.join(out, s[1])
            lua.append('{"trace", "%s"},' % f)
        elif s[0] == "test":
            lua.append('{"test", %d, %d, %d, %d},' % s[1:])
    lua.append("}")
    lua.append("""
local fc = %d
local out = "%s"
local mem = manager.machine.devices[":maincpu"].spaces["program"]
local started = false
local i = 1
local wait_until = nil
local last_fc = 0
local frames = 0
local pcdump = 0
local asked = false
emu.register_frame_done(function()
  frames = frames + 1
  if frames > 60 * 600 then manager.machine:exit() end
  if pcdump > 0 then
    print(string.format("PC %%04X", manager.machine.devices[":maincpu"].state["PC"].value))
    pcdump = pcdump - 1
    if pcdump == 0 then manager.machine:exit() end
    return
  end
  local now = mem:read_u16(fc)
  if not started then
    -- until the game has started: the first step may be the test hook,
    -- poked every frame until the game answers $A5
    local s = steps[1]
    if s ~= nil and s[1] == "test" then
      local h0 = mem:read_u8(s[2])
      if h0 == 0xA5 and asked then started = true; i = 2
      elseif not asked and h0 == 0x3C and mem:read_u8(s[2] + 1) == 0xC3 then
        -- the title is up (never before: the image is still loading)
        mem:write_u8(s[2] + 1, s[3]); mem:write_u8(s[2] + 2, s[4])
        mem:write_u8(s[2] + 3, s[5]); mem:write_u8(s[2], 0x5A)
        asked = true
        return
      else return end
    elseif frames > 60 * 25 then started = true
    else return end
  end
  while i <= #steps do
    local s = steps[i]
    if s[1] == "wait" then
      if wait_until == nil then wait_until = now + s[2] end
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
    elseif s[1] == "type" then
      manager.machine.natkeyboard:post(s[2]); i = i + 1
    elseif s[1] == "trace" then
      if s[2] == "off" then manager.machine.debugger:command("trace off")
      else manager.machine.debugger:command("trace " .. s[2] .. ",0") end
      i = i + 1
    elseif s[1] == "pc" then
      pcdump = s[2]; i = i + 1; return
    elseif s[1] == "peek" then
      local t = {}
      for k = 0, s[4] - 1 do t[#t + 1] = string.format("%%02X", mem:read_u8(s[3] + k)) end
      print("PEEK " .. s[2] .. " " .. table.concat(t, " ") .. "  (mame frame " .. frames .. ")")
      i = i + 1
    end
  end
  manager.machine:exit()
end, "lynxsim")
""" % (sym("frame_count"), out))
    luafile = os.path.join(out, "scenario.lua")
    open(luafile, "w").write("\n".join(lua))
    snapdir = os.path.join(out, "raw")
    subprocess.run(["rm", "-rf", snapdir, os.path.join(out, "snaps.txt")])
    cfg = os.path.join(out, "cfg")
    cmd = ["./mame", "lynx", "-rompath", "roms", "-cart", os.path.abspath(disk),
           "-video", "none", "-sound", "none", "-nothrottle", "-snapshot_directory", snapdir,
           "-cfg_directory", cfg, "-nvram_directory", cfg,
           "-autoboot_script", luafile, "-skip_gameinfo"]
    if any(x[0] == "trace" for x in steps):
        cmd[2:2] = ["-debug", "-debugger", "none"]
    if os.environ.get("LYNXWAV"):       # record the sound: LYNXWAV=out.wav
        i = cmd.index("-sound")
        cmd[i + 1] = "auto"
        cmd += ["-wavwrite", os.path.abspath(os.environ["LYNXWAV"])]
    r = subprocess.run(cmd, cwd=MAME, capture_output=True, text=True, timeout=900)
    for line in r.stdout.splitlines():
        if line.startswith(("PEEK", "SNAP", "PC ")):
            print(line)
    if r.returncode:
        print(r.stdout[-2000:], r.stderr[-2000:])
    names = []
    if os.path.exists(os.path.join(out, "snaps.txt")):
        names = open(os.path.join(out, "snaps.txt")).read().split()
    shots = sorted(os.listdir(os.path.join(snapdir, "lynx"))) if os.path.isdir(os.path.join(snapdir, "lynx")) else []
    for n, f in zip(names, shots):
        os.replace(os.path.join(snapdir, "lynx", f), os.path.join(out, n + ".png"))
        print(os.path.join(out, n + ".png"))


if __name__ == "__main__":
    main()

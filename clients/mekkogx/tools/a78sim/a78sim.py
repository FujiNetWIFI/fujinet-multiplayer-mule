#!/usr/bin/env python3
"""a78sim.py -- drive the Atari 7800 build in MAME from a scenario.

usage: a78sim.py CART.a78 MAPFILE SCENARIO OUTDIR

Runs the image in MAME's 7800 on the FujiNet 7800 cartridge (MAME with
fujinet-firmware's pico/atari-7800/emu grafted in, ~/Workspace/mame-a7800:
its "fujinet" cart slot; no window, as fast as it goes; MAME's own
settings go to OUTDIR/cfg). A claimed image takes CONFIG's place, so the
console's BIOS hands straight over to it. Then it waits for the game's
title (frame_count starts counting) and plays the scenario, one command a
line; times are in frames of the game's clock:

    test PHASE [ARG [PEVENT]]   offline test mode (src/test.c): phase number,
                                colony event / auction stage / seat, player event
    wait N                      let N frames pass
    snap NAME                   save OUTDIR/NAME.png
    hold DIRS                   hold directions: any of u d l r (or 'none')
    press                       tap the button
    key CHAR|NUM                type a key (RETURN = 13, ESC = 27)
    peek SYMBOL [N]             print N bytes at a symbol (from the map)
    zones                       each zone's DMA (maria.s mt_dma, in MARIA
                                cycles) and the sprite pieces shed
    pc N                        print the CPU's PC for N frames, then quit
    trace FILE | trace off      log every instruction to OUTDIR/FILE (slow)
    dump FROM TO FILE           save memory (hex addresses) to OUTDIR/FILE
    poke SYMBOL[+OFFSET] VALUE
    watch N SYMBOL[+OFFSET]...  for N frames (under 32768), print the game frame and a byte at
                                each symbol whenever one of them changes (WATCH)
    pad DIRS|1|2|pause|select|reset|none   the real stick, buttons and switches
                                (MAME's ports), not the test hook
    wcount N SYMBOL             for N frames, count the stores to SYMBOL (inp_dir:
                                the main loop's passes): WCOUNT stores frames

The input goes through the game's test hook (inp_hook in input.c) but for
pad, the addresses come from the linker map. A78WAV=out.wav records the
sound. Only the offline test mode runs here: the cartridge is pointed at a
port nothing listens on (FUJINET_TCP), so no fujinet-pc is needed or
disturbed. For a live game, FUJINET_TCP=host:port names a private
fujinet-pc's BoIP and A78SIM_THROTTLE=1 runs MAME in real time.
A78SIM_SYSTEM=a7800p is a PAL console.
"""
import os
import re
import subprocess
import sys

MAME = os.environ.get("MAME", os.path.expanduser("~/Workspace/mame-a7800"))
ROMS = os.environ.get("ROMS", os.path.expanduser("~/Workspace/mame/roms"))
SECS = int(os.environ.get("A78SIM_SECONDS", "600"))   # a run's limit, in the console's seconds


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
        elif c == "zones":
            steps.append(("peek", "mt_dma", sym("mt_dma"), 24))
            steps.append(("peek", "mt_shed", sym("mt_shed"), 1))
        elif c == "pad":
            steps.append(("pad", w[1]))
        elif c == "wcount":
            steps.append(("wcount", int(w[1]), sym(w[2])))
        elif c == "watch":
            steps.append(("watch", int(w[1]), [sym(x) for x in w[2:]]))
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
        elif s[0] == "pad":
            lua.append('{"pad", "%s"},' % s[1])
        elif s[0] == "wcount":
            lua.append('{"wcount", %d, %d},' % (s[1], s[2]))
        elif s[0] == "watch":
            lua.append('{"watch", %d, {%s}},' % (s[1], ", ".join(str(a) for a in s[2])))
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
local watch_until = nil
local watch_last = ""
-- MAME's 7800 ports: the stick (P1 Up/Down/Left/Right), the buttons, the
-- console's switches
local ports = manager.machine.ioport.ports
local function field(name)
  for _, p in pairs(ports) do
    for fname, f in pairs(p.fields) do
      if fname == name then return f end
    end
  end
  return nil
end
local padnames = { u = "P1 Up", d = "P1 Down", l = "P1 Left", r = "P1 Right",
                   ["1"] = "P1 Button 1", ["2"] = "P1 Button 2",
                   pause = "Pause", select = "Select", reset = "Reset" }
local padheld = {}
local function pad(spec)
  for _, f in pairs(padheld) do f:set_value(0) end
  padheld = {}
  if spec == "none" then return end
  local keys = {}
  if padnames[spec] ~= nil then keys = { spec }
  else for ch in spec:gmatch(".") do keys[#keys + 1] = ch end end
  for _, k in ipairs(keys) do
    local f = field(padnames[k])
    if f ~= nil then f:set_value(1); padheld[#padheld + 1] = f
    else print("PAD no field " .. padnames[k]) end
  end
end
if _G.__a78sim then return end
_G.__a78sim = emu.add_machine_frame_notifier(function()
  frames = frames + 1
  if frames > 60 * %d then manager.machine:exit() end
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
      else manager.machine.debugger:command("trace " .. s[2] .. ",0,noloop") end
      i = i + 1
    elseif s[1] == "pc" then
      pcdump = s[2]; i = i + 1; return
    elseif s[1] == "pad" then
      pad(s[2]); i = i + 1
    elseif s[1] == "wcount" then
      if watch_until == nil then
        watch_until = now + s[2]
        _G.__wc = 0
        _G.__wtap = mem:install_write_tap(s[3], s[3], "wc", function() _G.__wc = _G.__wc + 1 end)
      end
      if ((now - watch_until) & 0xFFFF) < 0x8000 then
        print(string.format("WCOUNT %%d stores %%d frames", _G.__wc, s[2]))
        _G.__wtap:remove(); _G.__wtap = nil
        watch_until = nil; i = i + 1
      else return end
    elseif s[1] == "watch" then
      if watch_until == nil then watch_until = now + s[2]; watch_last = "" end
      local t = {}
      for _, a in ipairs(s[3]) do t[#t + 1] = string.format("%%d", mem:read_u8(a)) end
      local v = table.concat(t, " ")
      if v ~= watch_last then print("WATCH " .. now .. " " .. v); watch_last = v end
      if ((now - watch_until) & 0xFFFF) < 0x8000 then watch_until = nil; i = i + 1 else return end
    elseif s[1] == "peek" then
      local t = {}
      for k = 0, s[4] - 1 do t[#t + 1] = string.format("%%02X", mem:read_u8(s[3] + k)) end
      print("PEEK " .. s[2] .. " " .. table.concat(t, " ") .. "  (mame frame " .. frames .. ")")
      i = i + 1
    end
  end
  manager.machine:exit()
end)
""" % (sym("frame_count"), out, SECS))
    luafile = os.path.join(out, "scenario.lua")
    open(luafile, "w").write("\n".join(lua))
    snapdir = os.path.join(out, "raw")
    subprocess.run(["rm", "-rf", snapdir, os.path.join(out, "snaps.txt")])
    cfg = os.path.join(out, "cfg")
    system = os.environ.get("A78SIM_SYSTEM", "a7800")
    cmd = ["./a7800", system, "-rompath", ROMS, "-cartslot", "fujinet", "-cart", os.path.abspath(disk),
           "-video", "none", "-sound", "none", "-nothrottle", "-snapshot_directory", snapdir,
           "-cfg_directory", cfg, "-nvram_directory", cfg,
           "-autoboot_script", luafile, "-skip_gameinfo"]
    if os.environ.get("A78SIM_THROTTLE"):  # real time (a live server's clock)
        cmd.remove("-nothrottle")
    if any(x[0] == "trace" for x in steps):
        cmd[2:2] = ["-debug", "-debugger", "none"]
    if os.environ.get("A78WAV"):        # record the sound: A78WAV=out.wav
        i = cmd.index("-sound")
        cmd[i + 1] = "auto"
        cmd += ["-wavwrite", os.path.abspath(os.environ["A78WAV"])]
    env = dict(os.environ)
    env.setdefault("FUJINET_TCP", "127.0.0.1:1")
    env.setdefault("SDL_VIDEODRIVER", "dummy")
    r = subprocess.run(cmd, cwd=MAME, capture_output=True, text=True, timeout=SECS + 300, env=env)
    for line in r.stdout.splitlines():
        if line.startswith(("PEEK", "SNAP", "PC ", "PAD", "WATCH", "WCOUNT", "[LUA")):
            print(line)
    if r.returncode:
        print(r.stdout[-2000:], r.stderr[-2000:])
    names = []
    if os.path.exists(os.path.join(out, "snaps.txt")):
        names = open(os.path.join(out, "snaps.txt")).read().split()
    sd = os.path.join(snapdir, system)
    shots = sorted(os.listdir(sd)) if os.path.isdir(sd) else []
    for n, f in zip(names, shots):
        os.replace(os.path.join(sd, f), os.path.join(out, n + ".png"))
        print(os.path.join(out, n + ".png"))


if __name__ == "__main__":
    main()

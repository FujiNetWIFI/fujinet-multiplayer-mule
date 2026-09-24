#!/usr/bin/env python3
"""dossim.py -- drive the MS-DOS build in MAME from a scenario.

usage: dossim.py EXE MAPFILE SCENARIO OUTDIR

Boots a DOS disk (DOSSIM_BOOT, default the PCjr's fn-dos21.img) in MAME's
IBM PCjr, or with DOSSIM_MACHINE=ibm5150 a PC with CGA (DOSSIM_RAM: 128K on
the PCjr, MAME's default on the PC), with the game and an AUTOEXEC.BAT that runs it copied on; no
window, as fast as it goes. Waits for the game's title, then plays the
scenario, one command a line; times are in frames of the game's clock
(adapted from tools/cocosim/cocosim.py; same scenario language):

    test PHASE [ARG [PEVENT]]   offline test mode (src/test.c): phase number,
                                colony event / auction stage / seat, player event
    wait N                      let N frames pass
    mwait N                     let N of MAME's frames pass (after the game exits)
    khold KEY | khold           hold a key of MAME's keyboard (its field name,
                                e.g. "KP 6 (Right)"), or let go
    snap NAME                   save OUTDIR/NAME.png
    hold DIRS                   hold directions: any of u d l r (or 'none')
    press                       tap the button
    key CHAR|NUM                type a key (ENTER = 13, ESC = 27)
    peek SYMBOL [N]             print N bytes at a symbol (from the map)
    stack                       how much of the stack was never used
    pc N                        print the CPU's CS:IP for N frames, then quit
                                (DOSSIM_EARLYPC=F: every 10th frame from MAME
                                frame F, before the game is found)
    trace FILE | trace off      log every instruction to OUTDIR/FILE (slow)
    type TEXT                   type on the real keyboard (\\n ENTER, \\s space)
    poke SYMBOL VALUE

Symbols are DGROUP offsets from the wlink map (build/mule-msdos.map); DGROUP
is wherever DOS loaded the game, so it is taken from the CPU's DS once the
title has written 3C C3 into test_hook. DOSSIM_NOFN=1 boots without
CONFIG.SYS (no FUJINET.SYS: the game answers INT F5h itself).

Against a FujiNet: DOSSIM_FUJINET=127.0.0.1:PORT connects the serial port
(a null modem) to a fujinet-pc (RS232 build) listening for BoIP there; the
boot disk's CONFIG.SYS must load FUJINET.SYS for that port and speed.
"""
import os
import re
import shutil
import subprocess
import sys

MAME = os.environ.get("MAME", os.path.expanduser("~/Workspace/mame"))
BOOT = os.environ.get("DOSSIM_BOOT", os.path.expanduser(
    "~/Vintage/IBM 5160 PCXT/fn-dos21.img"))


def symbols(mapfile):
    """DGROUP offsets: the map's `seg:off  _name` lines in the data group."""
    text = open(mapfile).read()
    syms = {}
    for seg, off, name in re.findall(r"^([0-9a-f]{4}):([0-9a-f]{4})[+*]?\s+(_\w+)\s*$", text, re.M):
        syms.setdefault(name[1:], int(off, 16))
    return syms


DIRBITS = {"d": 1, "r": 2, "u": 4, "l": 8}


def make_disk(exe, out):
    disk = os.path.join(out, "boot.img")
    shutil.copy(BOOT, disk)
    subprocess.run(["mdel", "-i", disk, "::AUTOEXEC.BAT"], capture_output=True)
    if os.environ.get("DOSSIM_NOFN"):
        subprocess.run(["mdel", "-i", disk, "::CONFIG.SYS"], capture_output=True)
    bat = os.path.join(out, "autoexec.bat")
    open(bat, "wb").write(b"@ECHO OFF\r\nMULE\r\n")
    subprocess.run(["mcopy", "-o", "-i", disk, bat, "::AUTOEXEC.BAT"], check=True)
    subprocess.run(["mcopy", "-o", "-i", disk, exe, "::MULE.EXE"], check=True)
    return disk


def main():
    exe, mapfile, scen, out = sys.argv[1:5]
    out = os.path.abspath(out)
    os.makedirs(out, exist_ok=True)
    syms = symbols(mapfile)
    sym = lambda n: syms[n]
    steps = []
    for line in open(scen):
        line = line.split("#")[0].strip()
        if not line:
            continue
        w = line.split()
        c = w[0]
        if c == "test":
            args = [int(x) for x in w[1:]] + [0, 0, 0]
            steps.append(("test", args[0] + 1, args[1], args[2]))
        elif c == "wait":
            steps.append(("wait", int(w[1])))
        elif c == "khold":
            steps.append(("khold", line.split(None, 1)[1] if len(w) > 1 else ""))
        elif c == "mwait":
            steps.append(("mwait", int(w[1])))
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
        elif c == "trace":
            steps.append(("trace", w[1]))
        elif c == "stack":
            steps.append(("stack", sym("_STACKLOW")))
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
        elif s[0] == "khold":
            lua.append('{"khold", "%s"},' % s[1])
        elif s[0] == "mwait":
            lua.append('{"mwait", %d},' % s[1])
        elif s[0] == "snap":
            lua.append('{"snap", "%s"},' % s[1])
        elif s[0] == "peek":
            lua.append('{"peek", "%s", %d, %d},' % (s[1], s[2], s[3]))
        elif s[0] == "pc":
            lua.append('{"pc", %d},' % s[1])
        elif s[0] == "type":
            lua.append('{"type", %s},' % ('"' + s[1].replace('"', '\\"').replace("\n", "\\n") + '"'))
        elif s[0] == "stack":
            lua.append('{"stack", %d},' % s[1])
        elif s[0] == "trace":
            f = "off" if s[1] == "off" else os.path.join(out, s[1])
            lua.append('{"trace", "%s"},' % f)
        elif s[0] == "test":
            lua.append('{"test", %d, %d, %d},' % s[1:])
    lua.append("}")
    lua.append("""
local fc_off = %d
local hook_off = %d
local out = "%s"
local cpu = manager.machine.devices[":maincpu"]
local mem = cpu.spaces["program"]
local ds = nil
local started = false
local i = 1
local wait_until = nil
local frames = 0
local pcdump = 0
local asked = false
local held = nil
local early = %d
local function a(off) return ds * 16 + off end
emu.register_frame_done(function()
  frames = frames + 1
  if frames > 60 * %d then
    manager.machine.video:snapshot()
    local f = io.open(out .. "/snaps.txt", "a"); f:write("timeout\\n"); f:close()
    print("TIMEOUT"); manager.machine:exit()
  end
  if pcdump > 0 then
    print(string.format("PC %%04X:%%04X DS %%04X", cpu.state["CS"].value, cpu.state["IP"].value, cpu.state["DS"].value))
    pcdump = pcdump - 1
    if pcdump == 0 then manager.machine:exit() end
    return
  end
  if early > 0 and frames >= early and frames < early + 3000 and frames %% 10 == 0 then
    print(string.format("EPC %%04X:%%04X DS %%04X F5 %%04X:%%04X", cpu.state["CS"].value, cpu.state["IP"].value, cpu.state["DS"].value, mem:read_u16(0x3D6), mem:read_u16(0x3D4)))
  end
  if ds == nil then
    -- the title writes 3C C3 into test_hook: DS then is DGROUP
    local d = cpu.state["DS"].value
    if mem:read_u8(d * 16 + hook_off) == 0x3C and mem:read_u8(d * 16 + hook_off + 1) == 0xC3 then
      ds = d
      print(string.format("DGROUP %%04X (mame frame %%d)", ds, frames))
    else return end
  end
  local now = mem:read_u16(a(fc_off))
  if not started then
    local s = steps[1]
    if s ~= nil and s[1] == "test" then
      local h0 = mem:read_u8(a(hook_off))
      if h0 == 0xA5 and asked then started = true; i = 2
      elseif not asked and h0 == 0x3C then
        mem:write_u8(a(hook_off) + 1, s[2]); mem:write_u8(a(hook_off) + 2, s[3])
        mem:write_u8(a(hook_off) + 3, s[4]); mem:write_u8(a(hook_off), 0x5A)
        asked = true
        return
      else return end
    else started = true end
  end
  while i <= #steps do
    local s = steps[i]
    if s[1] == "khold" then
      if held then held:set_value(0); held = nil end
      if s[2] ~= "" then
        for tag, port in pairs(manager.machine.ioport.ports) do
          local f = port.fields[s[2]]
          if f then held = f end
        end
        if held then held:set_value(1) else print("NOKEY " .. s[2]) end
      end
      i = i + 1
    elseif s[1] == "mwait" then
      if wait_until == nil then wait_until = frames + s[2] end
      if frames >= wait_until then wait_until = nil; i = i + 1 else return end
    elseif s[1] == "wait" then
      if wait_until == nil then wait_until = now + s[2] end
      if ((now - wait_until) & 0xFFFF) < 0x8000 then wait_until = nil; i = i + 1 else return end
    elseif s[1] == "poke" then
      mem:write_u8(a(s[2]), s[3]); i = i + 1
    elseif s[1] == "snap" then
      manager.machine.video:snapshot()
      print("SNAP " .. s[2] .. " game frame " .. now .. " mame frame " .. frames)
      local f = io.open(out .. "/snaps.txt", "a"); f:write(s[2] .. "\\n"); f:close()
      i = i + 1
      return
    elseif s[1] == "type" then
      manager.machine.natkeyboard:post(s[2]); i = i + 1
    elseif s[1] == "trace" then
      if s[2] == "off" then manager.machine.debugger:command("trace off")
      else manager.machine.debugger:command("trace " .. s[2] .. ",0") end
      i = i + 1
    elseif s[1] == "pc" then
      pcdump = s[2]; i = i + 1; return
    elseif s[1] == "stack" then
      local lo = mem:read_u16(a(s[2]))
      local n = 0
      while mem:read_u8(a(lo) + n) == 0xA5 do n = n + 1 end
      print("STACK never used: " .. n .. " bytes (mame frame " .. frames .. ")")
      i = i + 1
    elseif s[1] == "peek" then
      local t = {}
      for k = 0, s[4] - 1 do t[#t + 1] = string.format("%%02X", mem:read_u8(a(s[3]) + k)) end
      print("PEEK " .. s[2] .. " " .. table.concat(t, " ") .. "  (mame frame " .. frames .. ")")
      i = i + 1
    end
  end
  manager.machine:exit()
end, "dossim")
""" % (sym("frame_count"), sym("test_hook"), out, int(os.environ.get("DOSSIM_EARLYPC", "0")),
       int(os.environ.get("DOSSIM_SECONDS", "600"))))
    luafile = os.path.join(out, "scenario.lua")
    open(luafile, "w").write("\n".join(lua))
    snapdir = os.path.join(out, "raw")
    subprocess.run(["rm", "-rf", snapdir, os.path.join(out, "snaps.txt")])
    cfgdir = os.path.join(out, "cfg")
    os.makedirs(cfgdir, exist_ok=True)
    machine = os.environ.get("DOSSIM_MACHINE", "ibmpcjr")
    disk = make_disk(os.path.abspath(exe), out)
    flop = "-flop" if machine == "ibmpcjr" else "-flop1"
    # (the 5150's memory switches stay at MAME's default: 128K there is a
    # POST error 201; a CGA's screen isn't in main memory anyway)
    ram = os.environ.get("DOSSIM_RAM", "128K" if machine == "ibmpcjr" else "")
    cmd = ["./mame", machine, "-rompath", "roms", flop, disk] + \
          (["-ramsize", ram] if ram else []) + [
           "-video", "none", "-sound", "none", "-nothrottle", "-snapshot_directory", snapdir,
           "-cfg_directory", cfgdir, "-nvram_directory", os.path.join(out, "nvram"),
           "-autoboot_script", luafile, "-skip_gameinfo"]
    cmd += os.environ.get("DOSSIM_MAMEARGS", "").split()   # e.g. "-isa1 ega"
    if os.environ.get("DOSSIM_FUJINET"):   # host:port of a fujinet-pc's BoIP
        slot = "-serport" if machine == "ibmpcjr" else "-com1"
        cmd += [slot, "null_modem", "-bitb", "socket." + os.environ["DOSSIM_FUJINET"]]
        cmd.remove("-nothrottle")       # the server's clock is real time
    if os.environ.get("DOSSIM_REALTIME"):
        cmd.remove("-nothrottle")
    if any(x[0] == "trace" for x in steps):
        cmd[2:2] = ["-debug", "-debugger", "none"]
    if os.environ.get("DOSWAV"):        # record the sound: DOSWAV=out.wav
        k = cmd.index("-sound")
        cmd[k + 1] = "auto"
        cmd += ["-wavwrite", os.path.abspath(os.environ["DOSWAV"])]
    r = subprocess.run(cmd, cwd=MAME, capture_output=True, text=True,
                       timeout=int(os.environ.get("DOSSIM_SECONDS", "600")) + 300)
    if os.environ.get("DOSSIM_VERBOSE"):
        print(r.stdout, r.stderr)
    for line in r.stdout.splitlines():
        if line.startswith(("PEEK", "SNAP", "PC ", "EPC", "DGROUP", "TIMEOUT", "STACK", "NOKEY")):
            print(line)
    names = []
    if os.path.exists(os.path.join(out, "snaps.txt")):
        names = open(os.path.join(out, "snaps.txt")).read().split()
    sd = os.path.join(snapdir, machine)
    shots = sorted(os.listdir(sd)) if os.path.isdir(sd) else []
    for n, f in zip(names, shots):
        os.replace(os.path.join(sd, f), os.path.join(out, n + ".png"))
        print(os.path.join(out, n + ".png"))


if __name__ == "__main__":
    main()

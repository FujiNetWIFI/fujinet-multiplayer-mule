#!/usr/bin/env python3
"""nessim.py -- drive the NES build in MAME from a scenario.

usage: nessim.py CART.nes MAPFILE SCENARIO OUTDIR

Runs the image in MAME's NES on the FujiNet NES cartridge (MAME with
fujinet-firmware's pico/nes/emu grafted in: its "fujinet" slot; no window,
as fast as it goes; MAME's own settings go to OUTDIR/cfg). The cartridge
puts the image straight into its SRAMs, as if CONFIG had booted it. Then
it waits for the game's title (frame_count starts counting) and plays the
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
    ppuwatch N                  for N frames, where the CPU writes the PPU and
                                MMC3's pattern banks (PPUW lines): the line
                                rendering comes on at, PPU writes while it
                                shows (should be none), each switch's lines,
                                and the interrupts' drains: cycles taken
                                beyond 16 a unit of budget spent (crt0.s)
    trace FILE | trace off      log every instruction to OUTDIR/FILE (slow; loops
                                not folded, so it can be counted)
    dump FROM TO FILE           save memory (hex addresses) to OUTDIR/FILE
    type TEXT                   type on the real keyboard (\\n RETURN, \\s space)
    poke SYMBOL[+OFFSET] VALUE
    watch N SYMBOL[+OFFSET]...  for N frames (under 32768), print the game frame and a byte at
                                each symbol whenever one of them changes (WATCH)

The input goes through the game's test hook (inp_hook in input.c), the
addresses come from the linker map. NESWAV=out.wav records the sound.
Only the offline test mode runs here (tools/a2sim's scenario language,
same commands): the cartridge is pointed at a port nothing listens on
(FUJINET_TCP), so no fujinet-pc is needed or disturbed. For a live game,
FUJINET_TCP=host:port names a private fujinet-pc's BoIP and
NESSIM_THROTTLE=1 runs MAME in real time.
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
        elif c == "ppuwatch":
            steps.append(("ppuwatch", int(w[1])))
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
        elif s[0] == "ppuwatch":
            lua.append('{"ppuwatch", %d},' % s[1])
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
-- ppuwatch: the debugger logs the cycle count of the NMI's first
-- instruction, of the drain's and oam_send's (the sprites' DMA, after the
-- NMI's drain) and of every write to PPUMASK and MMC3's bank registers; a
-- write's line is 241 (the NMI's) + its cycles since / 113.667 (+10 for
-- the NMI's entry), past 261 the next frame's
local cpu = manager.machine.devices[":maincpu"]
local dbg = manager.machine.debugger
local tap = nil
local ws = nil
local rend, trend, sel, nmi_c, seen = 0, 0, 0, nil, 0
local function bump(t, k) t[k] = (t[k] or 0) + 1 end
local function watch_on()
  ws = {on = {}, late = 0, bad = 0, chr = {}, nmis = 0, drains = 0, over = -99999, most = 0}
  seen = #dbg.consolelog
  cpu.debug:bpset(mem:read_u16(0xFFFA), "1", 'printf "PW N %%d",totalcycles; g')
  -- the drain's budget is crt0.s's zero page byte 4, in_drain byte 7
  cpu.debug:bpset(%d, "1", 'printf "PW D %%d %%d",totalcycles,a; g')
  cpu.debug:bpset(%d, "1", 'printf "PW S %%d %%d",totalcycles,b@4; g')
  cpu.debug:wpset(mem, "w", 7, 1, "1", 'printf "PW I %%d %%d %%d",totalcycles,wpdata,b@4; g')
  cpu.debug:wpset(mem, "w", 0x2001, 1, "1", 'printf "PW M %%d %%d %%d",totalcycles,wpdata,b@4; g')
  cpu.debug:wpset(mem, "w", 0x8000, 2, "1", 'printf "PW B %%d %%d %%d",totalcycles,wpaddr,wpdata; g')
  if tap == nil then
    tap = mem:install_write_tap(0x2000, 0x2007, "ppuw", function(off, data, mask)
      if ws == nil then return end
      local r = off & 7
      if r == 1 then trend = data & 0x18
      elseif r >= 5 and trend ~= 0 then ws.bad = ws.bad + 1 end
    end)
  end
end
local function line_of(c)
  local d = c - nmi_c + 10
  local l = 241 + d // 113.6667
  local dot = math.floor((d %% 113.6667) * 3)
  if l >= 262 then l = l - 262 end
  return math.floor(l), dot
end
local nmi_drain, irq_drain = nil, nil
local function drained(c0, c1, used)
  local d = c1 - c0
  ws.drains = ws.drains + 1
  if d > ws.most then ws.most = d end
  if d - 16 * used > ws.over then ws.over = d - 16 * used end
end
local function watch_read()
  local log = dbg.consolelog
  local n = #log
  for k = seen + 1, n do
    local w = {}
    for x in string.gmatch(log[k] or "", "%%S+") do w[#w + 1] = x end
    if w[1] == "PW" then
      local c = tonumber(w[3])
      if w[2] == "N" then nmi_c = c; ws.nmis = ws.nmis + 1
      elseif w[2] == "D" then
        -- the NMI's drain: done 12 cycles before oam_send (rts, jsr)
        if tonumber(w[4]) == 90 then nmi_drain = c else nmi_drain = nil end
      elseif w[2] == "S" then
        if nmi_drain ~= nil then drained(nmi_drain, c - 12, 90 - tonumber(w[4])); nmi_drain = nil end
      elseif w[2] == "I" then
        -- the IRQ's: between in_drain's writes (and 16 cycles of its own)
        if tonumber(w[4]) == 1 then irq_drain = c
        elseif irq_drain ~= nil then drained(irq_drain + 16, c, 247 - tonumber(w[5])); irq_drain = nil end  -- (247: crt0.s IRQ_BUDGET)
      elseif nmi_c ~= nil and w[2] == "M" then
        local v = tonumber(w[4])
        if (v & 0x18) ~= 0 and rend == 0 then
          local l, dot = line_of(c)
          bump(ws.on, l)
          -- on by the pre-render line's dot 280 (v is copied from t then)
          if not ((l >= 241 and l < 261) or (l == 261 and dot < 280)) then ws.late = ws.late + 1 end
        end
        rend = v & 0x18
      elseif nmi_c ~= nil and w[2] == "B" then
        local a, v = tonumber(w[4]), tonumber(w[5])
        if a == 0x8000 then sel = v & 7
        elseif sel <= 1 and rend ~= 0 then
          local l = line_of(c)
          local key = string.format("R%%d=%%2d", sel, v)
          local e = ws.chr[key]
          if e == nil then ws.chr[key] = {l, l, 1}
          else e[1] = math.min(e[1], l); e[2] = math.max(e[2], l); e[3] = e[3] + 1 end
        end
      end
    end
  end
  seen = n
end
local function watch_report()
  watch_read()
  cpu.debug:bpclear()
  cpu.debug:wpclear()
  local t = {}
  for k, n in pairs(ws.on) do t[#t + 1] = {k, n} end
  table.sort(t, function(x, y) return x[1] < y[1] end)
  local u = {}
  for _, e in ipairs(t) do u[#u + 1] = string.format("%%d:%%d", e[1], e[2]) end
  print("PPUW frames " .. ws.nmis .. "  rendering on at line " .. table.concat(u, " ") .. "  late " .. ws.late)
  print("PPUW writes-while-showing " .. ws.bad)
  print("PPUW drains " .. ws.drains .. "  longest " .. ws.most .. " cycles  most beyond 16 a unit " .. ws.over)
  t = {}
  for k, e in pairs(ws.chr) do t[#t + 1] = string.format("%%s lines %%3d-%%3d x%%d", k, e[1], e[2], e[3]) end
  table.sort(t)
  for _, l in ipairs(t) do print("PPUW switch " .. l) end
  ws = nil
end
local watch_until = nil
local watch_last = ""
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
      else manager.machine.debugger:command("trace " .. s[2] .. ",0,noloop") end
      i = i + 1
    elseif s[1] == "pc" then
      pcdump = s[2]; i = i + 1; return
    elseif s[1] == "ppuwatch" then
      if watch_until == nil then watch_on(); watch_until = now + s[2] end
      watch_read()
      if ((now - watch_until) & 0xFFFF) < 0x8000 then
        watch_report(); watch_until = nil; i = i + 1
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
end, "nessim")
""" % (sym("frame_count"), out, syms.get("_ppu_drain", 0), syms.get("oam_send", 0)))
    luafile = os.path.join(out, "scenario.lua")
    open(luafile, "w").write("\n".join(lua))
    snapdir = os.path.join(out, "raw")
    subprocess.run(["rm", "-rf", snapdir, os.path.join(out, "snaps.txt")])
    cfg = os.path.join(out, "cfg")
    cmd = ["./mame", "nes", "-nes_slot", "fujinet", "-cart", os.path.abspath(disk),
           "-video", "none", "-sound", "none", "-nothrottle", "-snapshot_directory", snapdir,
           "-cfg_directory", cfg, "-nvram_directory", cfg,
           "-autoboot_script", luafile, "-skip_gameinfo"]
    if os.environ.get("NESSIM_THROTTLE"):  # real time (a live server's clock)
        cmd.remove("-nothrottle")
    if any(x[0] in ("trace", "ppuwatch") for x in steps):
        cmd[2:2] = ["-debug", "-debugger", "none"]
    if os.environ.get("NESWAV"):        # record the sound: NESWAV=out.wav
        i = cmd.index("-sound")
        cmd[i + 1] = "auto"
        cmd += ["-wavwrite", os.path.abspath(os.environ["NESWAV"])]
    env = dict(os.environ)
    env.setdefault("FUJINET_TCP", "127.0.0.1:1")
    r = subprocess.run(cmd, cwd=MAME, capture_output=True, text=True, timeout=900, env=env)
    for line in r.stdout.splitlines():
        if line.startswith(("PEEK", "SNAP", "PC ", "PPUW", "WATCH")):
            print(line)
    if r.returncode:
        print(r.stdout[-2000:], r.stderr[-2000:])
    names = []
    if os.path.exists(os.path.join(out, "snaps.txt")):
        names = open(os.path.join(out, "snaps.txt")).read().split()
    shots = sorted(os.listdir(os.path.join(snapdir, "nes"))) if os.path.isdir(os.path.join(snapdir, "nes")) else []
    for n, f in zip(names, shots):
        os.replace(os.path.join(snapdir, "nes", f), os.path.join(out, n + ".png"))
        print(os.path.join(out, n + ".png"))


if __name__ == "__main__":
    main()

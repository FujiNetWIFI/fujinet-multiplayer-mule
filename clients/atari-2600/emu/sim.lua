-- sim.lua -- run a scenario against the client (the astrocade's astrosim
-- verbs, for MAME's a2600 with the FujiNet cart).
--
-- M26_SCEN names a file of commands, one a line, run in order:
--   wait N                 N frames
--   bank B [T]             until the live bank is B (T frames at most, 3600)
--   zp ADDR VAL [T]        until zero-page ADDR (hex) holds VAL (hex)
--   press NAME [F]         press an input for F frames (6), then release
--                          NAME: fire up down left right reset select
--   hold NAME F            the same, F frames
--   snap                   a snapshot (build/snap)
--   text                   print the 21 text rows, decoded from the planes
--   peek ADDR [N]          print N bytes (hex) from ADDR (hex)
--   walkto X Y [T]         steer the walker (WX $C7, WY $C8, decimal
--                          units) until it stands there; T frames at most
--   quit
-- Every bank change and every change of HPHASE is logged as it happens.
package.path = (os.getenv("M26_EMU") or ".") .. "/?.lua;" .. package.path
local font = require("vcsfont")

local sp = manager.machine.devices[":maincpu"].spaces["program"]
local function rd(a) return sp:readv_u8(a) end
local BANK, PLANES, HPHASE = 0x1F0E, 0x1800, 0xA8

local NAMES = {
    fire = { ":joyport1:joy:JOY", "P1 Button 1" },
    up = { ":joyport1:joy:JOY", "P1 Up" },
    down = { ":joyport1:joy:JOY", "P1 Down" },
    left = { ":joyport1:joy:JOY", "P1 Left" },
    right = { ":joyport1:joy:JOY", "P1 Right" },
    reset = { ":SWB", "Reset Game" },
    select = { ":SWB", "Select Game" },
}

local cmds = {}
for line in io.lines(os.getenv("M26_SCEN")) do
    line = line:gsub("#.*", "")
    local w = {}
    for t in line:gmatch("%S+") do w[#w + 1] = t end
    if #w > 0 then cmds[#cmds + 1] = w end
end

local function read_row(row)
    local s = ""
    for col = 0, 11 do
        local plane, left, key = col // 2, (col % 2) == 0, 0
        for l = 0, 4 do
            local b = rd(PLANES + plane * 128 + row * 6 + l)
            local ink = left and ((b >> 5) & 7) or ((b >> 1) & 7)
            key = (key << 3) | ink
        end
        s = s .. (font.glyph[key] or "?")
    end
    return s
end

local n, pc, waitn, held, holdn = 0, 1, 0, nil, 0
local lastbank, lastph = -1, -1
local function field(name)
    local f = NAMES[name]
    return manager.machine.ioport.ports[f[1]].fields[f[2]]
end

_G._sim = emu.add_machine_frame_notifier(function()
    n = n + 1
    local b, ph = rd(BANK), rd(HPHASE)
    if b ~= lastbank then
        print(string.format("[%6d] bank %d", n, b)); lastbank = b
    end
    if ph ~= lastph then
        print(string.format("[%6d] phase %d", n, ph)); lastph = ph
    end
    if held then
        holdn = holdn - 1
        if holdn <= 0 then field(held):set_value(0); held = nil; waitn = 4 end
        return
    end
    if waitn > 0 then waitn = waitn - 1; return end
    local c = cmds[pc]
    if not c then return end
    local op = c[1]
    if op == "wait" then
        waitn = tonumber(c[2]); pc = pc + 1
    elseif op == "bank" or op == "zp" then
        c.t = (c.t or tonumber(op == "bank" and c[3] or c[4]) or 3600) - 1
        local ok = (op == "bank") and (b == tonumber(c[2]))
                   or (op == "zp" and rd(tonumber(c[2], 16)) == tonumber(c[3], 16))
        if ok then pc = pc + 1
        elseif c.t <= 0 then
            print(string.format("[%6d] TIMEOUT: %s", n, table.concat(c, " "))); pc = pc + 1
        end
    elseif op == "press" or op == "hold" then
        held = c[2]; holdn = tonumber(c[3]) or 6
        field(held):set_value(1); pc = pc + 1
    elseif op == "snap" then
        manager.machine.video:snapshot(); pc = pc + 1
        print(string.format("[%6d] snap", n))
    elseif op == "text" then
        for r = 0, 20 do
            local s = read_row(r)
            if s:match("%S") then print(string.format("  %2d |%s|", r, s)) end
        end
        pc = pc + 1
    elseif op == "peek" then
        local a, k, out = tonumber(c[2], 16), tonumber(c[3] or "1"), {}
        for i = 0, k - 1 do out[#out + 1] = string.format("%02X", rd(a + i)) end
        print(string.format("[%6d] %04X: %s", n, a, table.concat(out, " ")))
        pc = pc + 1
    elseif op == "walkto" then
        local tx, ty = tonumber(c[2]), tonumber(c[3])
        local wx, wy = rd(0xC7), rd(0xC8)
        c.t = (c.t or tonumber(c[4]) or 1800) - 1
        local f = manager.machine.ioport.ports[":joyport1:joy:JOY"].fields
        f["P1 Left"]:set_value(wx > tx and 1 or 0)
        f["P1 Right"]:set_value(wx < tx and 1 or 0)
        f["P1 Up"]:set_value((wx == tx and wy > ty) and 1 or 0)
        f["P1 Down"]:set_value((wx == tx and wy < ty) and 1 or 0)
        if (wx == tx and wy == ty) or c.t <= 0 then
            for _, k in ipairs({ "P1 Left", "P1 Right", "P1 Up", "P1 Down" }) do
                f[k]:set_value(0)
            end
            if c.t <= 0 then
                print(string.format("[%6d] TIMEOUT: walkto %d %d at %d %d", n, tx, ty, wx, wy))
            end
            pc = pc + 1
            waitn = 4
        end
    elseif op == "quit" then
        manager.machine:exit()
    else
        print("sim: unknown command " .. op); pc = pc + 1
    end
end)

-- M26_BANKLOG=1: every bank switch as it happens, with the PC and MENT.
if os.getenv("M26_BANKLOG") then
    local cpu = manager.machine.devices[":maincpu"]
    local cnt = 0
    _G._banktap = sp:install_write_tap(0x1D80, 0x1D8F, "banks", function(off, data, mask)
        cnt = cnt + 1
        if cnt <= tonumber(os.getenv("M26_BANKLOG")) then
            print(string.format("[%6d] switch -> %d  pc=%04X ment=%d nstate=%d nreq=%d",
                                n, off - 0x1D80, cpu.state["PC"].value, rd(0x8D), rd(0x9A), rd(0x9B)))
        end
    end)
end

-- M26_WATCH=hexaddr: every write to that address, with the PC and the bank.
if os.getenv("M26_WATCH") then
    local cpu = manager.machine.devices[":maincpu"]
    local wa = tonumber(os.getenv("M26_WATCH"), 16)
    local cnt = 0
    _G._watch = sp:install_write_tap(wa, wa, "watch", function(off, data, mask)
        cnt = cnt + 1
        if cnt <= 60 then
            print(string.format("[%6d] write %04X=%02X pc=%04X bank=%d", n, off, data,
                                cpu.state["PC"].value, rd(0x1F0E)))
        end
    end)
end

-- M26_FRAMES=1: VSYNC to VSYNC; every frame that is not 262 lines, with the
-- bank it began and ended in (emu/frames.lua's measure).
if os.getenv("M26_FRAMES") then
    local LINE = (1 / 59.92) / 262
    local last, lastb, bad, tot = nil, 0, 0, 0
    _G._frm = sp:install_write_tap(0x00, 0x00, "vsync", function(off, data, mask)
        if (data & 0x02) == 0 then return end
        local t = manager.machine.time:as_double()
        local b = rd(0x1F0E)
        if last then
            tot = tot + 1
            local lines = math.floor((t - last) / LINE + 0.5)
            if lines ~= 262 then
                bad = bad + 1
                if bad <= 30 then
                    print(string.format("[%6d] FRAME %d lines, bank %d -> %d", n, lines, lastb, b))
                end
            end
        end
        last, lastb = t, b
    end)
    _G._frmstop = emu.add_machine_stop_notifier(function()
        print(string.format("FRAMES: %d of %d were not 262 lines", bad, tot))
    end)
end

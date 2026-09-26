-- auto.lua -- play by itself, for soak runs: whatever screen is up, press
-- what a player would, and log what happens.
--
-- It watches the live bank ($1F0E) and the client's own cells, never frame
-- counts: a network round trip takes as long as it takes. Logged: every
-- phase and round, every cold restart (the boot bank coming back after the
-- start is a crash), every frame that is not 262 lines, and a summary of the
-- banks visited at the end.
--
--   M26_AUTO_SEED=n    the random walk's seed

local sp = manager.machine.devices[":maincpu"].spaces["program"]
local function rd(a) return sp:readv_u8(a) end
local BANK, HPHASE, HROUND = 0x1F0E, 0xA8, 0xA9
local LRDY, SPRESS, DVST, GSTG = 0xB6, 0xB2, 0xD2, 0xB2

local NAMES = {
    fire = { ":joyport1:joy:JOY", "P1 Button 1" },
    up = { ":joyport1:joy:JOY", "P1 Up" },
    down = { ":joyport1:joy:JOY", "P1 Down" },
    left = { ":joyport1:joy:JOY", "P1 Left" },
    right = { ":joyport1:joy:JOY", "P1 Right" },
    select = { ":SWB", "Select Game" },
}
local function field(name)
    local f = NAMES[name]
    return manager.machine.ioport.ports[f[1]].fields[f[2]]
end

math.randomseed(tonumber(os.getenv("M26_AUTO_SEED") or "1"))
local n, held, holdn, cool = 0, nil, 0, 0
local lastbank, lastph, lastrd = -1, -1, -1
local visits, colds, turnframes = {}, 0, 0
local started = false

local function press(name, frames)
    held = name; holdn = frames or 6
    field(name):set_value(1)
end

_G._auto = emu.add_machine_frame_notifier(function()
    n = n + 1
    local b, ph, rn = rd(BANK), rd(HPHASE), rd(HROUND)
    visits[b] = (visits[b] or 0) + 1
    if b ~= lastbank then
        if b == 0 and started then
            colds = colds + 1
            print(string.format("[%7d] COLD RESTART (from bank %d)", n, lastbank))
        end
        lastbank = b
    end
    if ph ~= lastph or rn ~= lastrd then
        print(string.format("[%7d] round %d phase %d (bank %d)", n, rn, ph, b))
        lastph, lastrd = ph, rn
        if ph == 8 then print(string.format("[%7d] GAME OVER", n)) end
    end
    if held then
        holdn = holdn - 1
        if holdn <= 0 then field(held):set_value(0); held = nil; cool = 8 end
        return
    end
    if cool > 0 then cool = cool - 1; return end
    if b == 0 then press("fire"); cool = 60; return end
    started = true
    if b == 8 then press("fire"); cool = 120; return end
    if b == 1 then
        if rd(LRDY) == 0 then press("fire"); cool = 90 end
        return
    end
    if b == 3 then
        if rd(SPRESS) == 0 then press("fire"); cool = 60 end
        return
    end
    if b == 4 then
        if math.random(60) == 1 then press("fire") end
        return
    end
    if b == 5 or b == 6 then
        if rd(DVST) == 1 then press("fire"); cool = 60; turnframes = 0; return end
        turnframes = turnframes + 1
        if turnframes > 1500 then press("select"); cool = 120; turnframes = 0; return end
        local dirs = { "up", "down", "left", "right", "fire" }
        press(dirs[math.random(#dirs)], math.random(10, 60))
        return
    end
    if b == 9 then
        if math.random(40) == 1 then
            local d = { "up", "down", "fire" }
            press(d[math.random(3)])
        end
        return
    end
    if b == 7 then
        local d = { "up", "down" }
        press(d[math.random(2)], math.random(10, 80))
        return
    end
end)

-- every frame that is not 262 lines
local LINE = (1 / 59.92) / 262
local last, lastb, bad, tot = nil, 0, 0, 0
local badby = {}
_G._autofrm = sp:install_write_tap(0x00, 0x00, "vsync", function(off, data, mask)
    if (data & 0x02) == 0 then return end
    local t = manager.machine.time:as_double()
    local bk = rd(BANK)
    if last then
        tot = tot + 1
        local lines = math.floor((t - last) / LINE + 0.5)
        if lines ~= 262 then
            bad = bad + 1
            local k = string.format("%d->%d", lastb, bk)
            badby[k] = (badby[k] or 0) + 1
        end
    end
    last, lastb = t, bk
end)

_G._autostop = emu.add_machine_stop_notifier(function()
    local keys = {}
    for k in pairs(visits) do keys[#keys + 1] = k end
    table.sort(keys)
    local out = {}
    for _, k in ipairs(keys) do out[#out + 1] = string.format("%d:%d", k, visits[k]) end
    print("VISITS " .. table.concat(out, " "))
    out = {}
    for k, v in pairs(badby) do out[#out + 1] = k .. "=" .. v end
    print(string.format("FRAMES: %d of %d not 262 lines (%s)", bad, tot, table.concat(out, " ")))
    print(string.format("COLD RESTARTS: %d", colds))
end)

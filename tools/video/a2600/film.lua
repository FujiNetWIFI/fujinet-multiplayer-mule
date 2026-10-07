-- film.lua -- the 2600 has no offline test mode: play a live game with
-- emu/auto.lua and film the first FILM frames (a snapshot every 2) after the
-- first visit to each (bank, phase) of each round we want. Copied to
-- clients/atari-2600/emu/ and run like a2600-shots.lua (tools/video/clips_2600.sh).
-- Each snapshot is logged "FILM k key bank:phase round r".
dofile(os.getenv("M26_EMU") .. "/auto.lua")

local sp = manager.machine.devices[":maincpu"].spaces["program"]
local function rd(a) return sp:readv_u8(a) end
local BANK, HPHASE, HROUND = 0x1F0E, 0xA8, 0xA9
local FILM = tonumber(os.getenv("FILM") or "720")
local seen, left, key, k, f = {}, 0, "", 0, 0

_G._film = emu.add_machine_frame_notifier(function()
    f = f + 1
    local b, ph, rn = rd(BANK), rd(HPHASE), rd(HROUND)
    local kk = b .. ":" .. ph .. ":" .. (rn <= 2 and rn or 99)
    if not seen[kk] then
        seen[kk] = true
        left, key = FILM, kk
    end
    if left > 0 then
        if left % 2 == 0 then
            manager.machine.video:snapshot()
            print(string.format("FILM %d key %s bank %d phase %d round %d frame %d", k, key, b, ph, rn, f))
            k = k + 1
        end
        left = left - 1
    end
end)

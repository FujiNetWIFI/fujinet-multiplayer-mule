-- a2600-shots.lua -- the 2600 has no offline test mode: play a live game
-- with emu/auto.lua and take a snapshot shortly after the first visit to
-- each (bank, phase) pair, and twice more later in the same screen.
-- Copy to clients/atari-2600/emu/ and run `test/rig.sh auto`-style via
-- run.sh (see screenshots/make-shots.sh a2600). Snapshots land in
-- build/snap/a2600/NNNN.png, in order; each is logged "SHOT k bank phase round".
dofile(os.getenv("M26_EMU") .. "/auto.lua")

local sp = manager.machine.devices[":maincpu"].spaces["program"]
local function rd(a) return sp:readv_u8(a) end
local BANK, HPHASE, HROUND = 0x1F0E, 0xA8, 0xA9
local seen, due, k, f = {}, {}, 0, 0

_G._shots = emu.add_machine_frame_notifier(function()
    f = f + 1
    local b, ph, rn = rd(BANK), rd(HPHASE), rd(HROUND)
    local key = b .. ":" .. ph
    if not seen[key] then
        seen[key] = true
        due[#due + 1] = { f + 90, key, rn }
        due[#due + 1] = { f + 400, key, rn }
    end
    for i = #due, 1, -1 do
        if f >= due[i][1] then
            manager.machine.video:snapshot()
            print(string.format("SHOT %d bank:phase %s round %d (now bank %d phase %d)",
                                k, due[i][2], due[i][3], b, ph))
            k = k + 1
            table.remove(due, i)
        end
    end
end)

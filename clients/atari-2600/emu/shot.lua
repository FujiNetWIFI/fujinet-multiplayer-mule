-- shot.lua -- a snapshot after SHOT_FRAMES frames, then exit.
local target = tonumber(os.getenv("SHOT_FRAMES") or "120")
local n = 0
_G._shot = emu.add_machine_frame_notifier(function()
    n = n + 1
    if n == target then
        manager.machine.video:snapshot()
    elseif n == target + 15 then
        manager.machine:exit()
    end
end)

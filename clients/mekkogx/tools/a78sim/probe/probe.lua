-- probe.lua -- once the probe's frame count (FB) has run 60 frames, count
-- the idle loop (FC-FE) over 60 more (the CPU left over), snapshot, exit.
if _G.__probe then return end
_G.__probe = { f0 = nil, c0 = nil }
local P = _G.__probe
local mem = manager.machine.devices[":maincpu"].spaces["program"]
local function count()
  return mem:read_u8(0xFC) | (mem:read_u8(0xFD) << 8) | (mem:read_u8(0xFE) << 16)
end
local last, run = nil, 0
P.sub = emu.add_machine_frame_notifier(function()
  local f = mem:read_u8(0xFB)
  if last ~= nil and f ~= last then run = run + 1 elseif last ~= nil then run = 0 end
  last = f
  if run == 60 then P.c0 = count() end
  if run == 120 then
    print(string.format("IDLE %.1f loops per frame", (count() - P.c0) / 60))
    manager.machine.screens[":screen"]:snapshot()
    manager.machine:exit()
  end
end)

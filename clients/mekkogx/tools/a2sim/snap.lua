-- snap.lua: run the machine, take snapshots at given frames, then quit.
-- env A2SNAP="600,900" (frame numbers), A2OUT=dir
local frames = {}
for f in string.gmatch(os.getenv("A2SNAP") or "600", "%d+") do table.insert(frames, tonumber(f)) end
local n = 0
local last = frames[#frames]
emu.register_frame_done(function()
  n = n + 1
  for _, f in ipairs(frames) do
    if n == f then manager.machine.video:snapshot() end
  end
  if n >= last then manager.machine:exit() end
end, "snap")

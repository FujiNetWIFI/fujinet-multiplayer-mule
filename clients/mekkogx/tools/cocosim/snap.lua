-- snap.lua: type LOADM"MULE":EXEC once BASIC is up, take snapshots at the
-- given frames, then quit. env A2SNAP="600,900" (frame numbers)
local frames = {}
for f in string.gmatch(os.getenv("A2SNAP") or "600", "%d+") do table.insert(frames, tonumber(f)) end
local n = 0
local last = frames[#frames]
emu.register_frame_done(function()
  n = n + 1
  if n == 90 and os.getenv("COCO_TYPE") ~= "0" then manager.machine.natkeyboard:post('LOADM"MULE":EXEC\n') end
  for _, f in ipairs(frames) do
    if n == f then manager.machine.video:snapshot() end
  end
  if n >= last then manager.machine:exit() end
end, "snap")

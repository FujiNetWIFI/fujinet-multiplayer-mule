local n = 0
emu.register_frame_done(function()
  n = n + 1
  if n == 600 then
    local mem = manager.machine.devices[":maincpu"].spaces["program"]
    local f = io.open(os.getenv("A2DUMP"), "wb")
    for a = 0, 0xBFFF do f:write(string.char(mem:read_u8(a))) end
    f:close()
    manager.machine:exit()
  end
end, "dump")

-- standings.lua -- flip the left difficulty switch mid-game and snapshot.
-- It is a LEVEL, not an edge: the screen is the standings for as long as it
-- is up, which is why the client reads it out of INCUR and not out of the
-- newly-pressed byte.
dofile(os.getenv("DRIVE_LUA"))
local sp = manager.machine.devices[":maincpu"].spaces["program"]
local f
for _, p in pairs(manager.machine.ioport.ports) do
    for n, fl in pairs(p.fields) do
        if n == "Left Diff. Switch" then f = fl end
    end
end
local n = 0
emu.register_frame_done(function()
    n = n + 1
    if n == 1500 then print("-- difficulty UP"); f:set_value(1) end
    if n == 1800 then manager.machine.video:snapshot() end
    if n == 1830 then manager.machine:exit() end
end)

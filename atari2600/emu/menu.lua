-- menu.lua -- open the SELECT menu mid-game, page to the help, and snapshot.
dofile(os.getenv("DRIVE_LUA"))
local sp = manager.machine.devices[":maincpu"].spaces["program"]
local F = {}
for _, p in pairs(manager.machine.ioport.ports) do
    for n, fl in pairs(p.fields) do F[n] = fl end
end
local n = 0
local WHEN = tonumber(os.getenv("MENU_AT") or "1500")
emu.register_frame_done(function()
    n = n + 1
    -- SELECT is only seen by a bank that scans input.
    if n == WHEN and sp:readv_u8(0x1F0E) ~= 1 then n = n - 1; return end
    if n == WHEN then F["Select Game"]:set_value(1) end
    if n == WHEN + 6 then F["Select Game"]:set_value(0) end
    if n == WHEN + 60 then manager.machine.video:snapshot() end
    if n == WHEN + 90 then manager.machine:exit() end
end)

-- selflip.lua -- tap SELECT every two seconds, so a frame-length run covers
-- BOTH shapes of the kernel: the dice class, whose band is two scanlines
-- longer than the two text rows it replaces, and the plain 21-row one.
local sel
for _, p in pairs(manager.machine.ioport.ports) do
    for name, f in pairs(p.fields) do
        if name:lower():find("select") then sel = f end
    end
end
local n = 0
emu.register_frame_done(function()
    n = n + 1
    if sel then sel:set_value((n % 120) < 4 and 1 or 0) end
end)

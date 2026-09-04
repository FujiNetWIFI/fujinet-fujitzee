-- demoshot.lua: launch the cart from the OS menu and snapshot the
-- static DEMO screen (build with DEMO=1).
--   mame ... -autoboot_script emu/demoshot.lua -video none -sound none \
--        -seconds_to_run 14
local function port_by_suffix(suffix)
    for tag, port in pairs(manager.machine.ioport.ports) do
        if tag:sub(-#suffix) == suffix then return port end
    end
    return nil
end

local phase = 0
local shot = false
emu.register_frame(function()
    local t = manager.machine.time.seconds
    if phase == 0 and t >= 3 then
        local p = port_by_suffix("KEYPAD3")
        if p then p:field(0x10):set_value(1) end
        phase = 1
    elseif phase == 1 and t >= 5 then
        local p = port_by_suffix("KEYPAD3")
        if p then p:field(0x10):clear_value() end
        phase = 2
    end
    if not shot and t >= 10 then
        manager.machine.video:snapshot()
        shot = true
    elseif t >= 12 then
        manager.machine:exit()
    end
end)

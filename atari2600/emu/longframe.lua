-- longframe.lua -- which bank is mapped when a frame runs long?
-- The VSYNC write is the program's own frame boundary; the bank byte the
-- cartridge publishes at $1F0E says who was doing the work.
if os.getenv("DRIVE_LUA") then dofile(os.getenv("DRIVE_LUA")) end
local sp = manager.machine.devices[":maincpu"].spaces["program"]
local FRAME, LINE = 1/59.92, (1/59.92)/262
local last, tally = nil, {}
_G._lf = sp:install_write_tap(0x00, 0x00, "vsync", function(off, data, mask)
    if data ~= 2 then return end
    local t = manager.machine.time:as_double()
    if last then
        local lines = math.floor((t - last) / LINE + 0.5)
        if lines ~= 262 then
            local b = sp:readv_u8(0x1F0E)
            local e = sp:readv_u8(0xA5)
            local k = string.format("bank %d ent %02X -> %d lines", b, e, lines)
            tally[k] = (tally[k] or 0) + 1
        end
    end
    last = t
end)
emu.register_stop(function()
    local ks = {}
    for k in pairs(tally) do ks[#ks+1] = k end
    table.sort(ks)
    for _, k in ipairs(ks) do print(string.format("LONG %-34s x%d", k, tally[k])) end
end)

-- stack.lua -- the stack must stay above STKLOW.
--
-- This client moved the boundary: both 2600 siblings put variables in $80-$BF
-- and gave the stack 64 bytes, but the five dice pointer PAIRS and the
-- sixteen-bit score shadow do not fit in what that leaves. 46 bytes is still
-- twenty-three nested calls -- and "still" is a claim, so it is measured.
if os.getenv("DRIVE_LUA") then dofile(os.getenv("DRIVE_LUA")) end
local cpu = manager.machine.devices[":maincpu"]
local STKLOW = tonumber(os.getenv("STKLOW") or "0xD3")
local low = 0xFF
emu.register_frame_done(function()
    local sp = cpu.state["SP"].value & 0xFF
    if sp < low then low = sp end
end)
emu.register_stop(function()
    print(string.format("STACK low-water $01%02X (%d bytes used); floor $01%02X",
        low, 0xFF - low, STKLOW))
    if low < STKLOW then
        print("STACK FAILED: reached below the floor")
    else
        print("STACK ok")
    end
end)

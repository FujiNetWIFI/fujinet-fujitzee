-- curbar.lua -- the dice cursor bar, on the screen, under the right die.
--
-- MAME'S FRAME BOUNDARY IS NOT THE PROGRAM'S VSYNC. Sampling zero page at
-- frame_done can catch the client already a vblank into the next frame, with
-- the cursor moved on -- so an early cut of this read FZDCUR=4, snapshotted a
-- bar drawn under die 0, and looked exactly like a positioning bug. It was
-- the measurement.
--
-- So: wait until the cursor has been STILL for a while, and only then look.
dofile(os.getenv("DRIVE_LUA"))
local sp = manager.machine.devices[":maincpu"].spaces["program"]
local function r(a) return sp:readv_u8(a) end
local shot, stable, last = false, 0, -1
emu.register_frame_done(function()
    if shot then return end
    local d = r(0xB3)
    if d == last then stable = stable + 1 else stable, last = 0, d end
    if stable > 20 and r(0xD1) == 2 and r(0x1F0E) == 1 and r(0xA7) == 1 then
        print(string.format("-- bar on, cursor steady on die %d for %d frames",
            d, stable))
        manager.machine.video:snapshot()
        shot = true
    end
end)

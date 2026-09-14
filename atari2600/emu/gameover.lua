-- gameover.lua -- play until the server calls it, then snapshot.
--
-- Round 99 is the whole of the game-over screen: the server's prompt is the
-- only thing on the wire that names the winner, and it gets four rows.
dofile(os.getenv("DRIVE_LUA"))
local sp = manager.machine.devices[":maincpu"].spaces["program"]
local shot = false
emu.register_frame_done(function()
    if shot then return end
    if sp:readv_u8(0xAE) == 99 and sp:readv_u8(0x1F0E) == 1 then
        local p = ""
        for i = 0, 40 do
            local c = sp:readv_u8(0x1B00 + 22 + i)
            if c == 0 then break end
            p = p .. string.char(c)
        end
        print("-- GAME OVER, prompt: " .. p)
        manager.machine.video:snapshot()
        shot = true
    end
end)

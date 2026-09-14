-- reveal.lua -- catch the moment a bot's chosen row is held up.
--
-- The wire has no "last move" field, so the client works it out by diffing
-- the seat's filled rows against a two-byte shadow. This waits for FZREVSL to
-- stop being $FF and snapshots while the hold is still running.
dofile(os.getenv("DRIVE_LUA"))
local sp = manager.machine.devices[":maincpu"].spaces["program"]
local armed, shot = false, false
emu.register_frame_done(function()
    if shot then return end
    -- A reveal is a highlighted row on a card that is NOT ours on a turn that
    -- is NOT ours. FZREVSL is disarmed by the time the picture is drawn --
    -- the shadow staging re-arms it with a meaningless answer, so it is not
    -- what to watch.
    local hl  = sp:readv_u8(0x9E)   -- FZHL, the highlighted row
    local my  = sp:readv_u8(0xB0)
    local view = sp:readv_u8(0xB1)
    if hl ~= 0xFF and my == 0 and view ~= 0 and sp:readv_u8(0x1F0E) == 1 then
        if not armed then
            armed = true
            print(string.format("-- reveal: row %d held up on seat %d's card",
                hl, view))
        end
        manager.machine.video:snapshot()
        shot = true
    end
end)

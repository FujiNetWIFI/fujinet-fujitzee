-- holdkeep.lua -- a die you have held must STAY held until you roll.
--
-- keepRoll is an ECHO of the last roll that actually went out, so restaging
-- FZKEEP from it between rolls throws away every hold made since. The symptom
-- is a die that sits there held and then deselects itself at the next poll.
--
-- Purely observational, so it does not argue with the play loop for the
-- controls: watch FZKEEP, and call it a violation when a bit goes from held
-- (0) to re-roll (1) on YOUR turn without a roll having gone out and without
-- the turn having changed. Those are the only two things allowed to move it.
dofile(os.getenv("DRIVE_LUA"))
local sp = manager.machine.devices[":maincpu"].spaces["program"]
local function r(a) return sp:readv_u8(a) end
local keep, rl, act, my = -1, -1, -1, -1
local holds, bad = 0, 0
emu.register_frame_done(function()
    local k, l, a, m = r(0xB5), r(0xB6), r(0xAF), r(0xB0)
    if keep ~= -1 and k ~= keep then
        if os.getenv("HK_TRACE") then
            print(string.format("   keep %02X -> %02X  rl %d->%d act %d->%d my %d",
                keep, k, rl, l, act, a, m))
        end
        -- bits that went from held to re-roll
        local freed = k & (~keep) & 0x1F
        if freed ~= 0 and m == 1 and a == act and l == rl then
            bad = bad + 1
            print(string.format(
                "!! DESELECTED: keep %02X -> %02X with no roll (rl=%d act=%d)",
                keep, k, l, a))
        end
        if (keep & (~k) & 0x1F) ~= 0 and m == 1 then holds = holds + 1 end
    end
    keep, rl, act, my = k, l, a, m
end)
emu.register_stop(function()
    print(string.format("HOLDKEEP %d holds made, %d spurious deselects",
        holds, bad))
end)

-- dicecheck.lua -- does a roll actually rattle, and only the dice that moved?
--
-- The animation is a countdown the vblank hook steps, not a blocking loop:
-- there is no frame to block in on this console, because the display loop IS
-- the program. So what is watched is FZANM going high and the pointers
-- changing underneath it.
dofile(os.getenv("DRIVE_LUA"))
local sp = manager.machine.devices[":maincpu"].spaces["program"]
local function r(a) return sp:readv_u8(a) end
local DP = {0xC6, 0xC8, 0xCA, 0xCC, 0xCE}
local prev, changed, rolls, wasanm = {}, {}, 0, false
for i = 1, 5 do prev[i] = -1; changed[i] = 0 end
emu.register_frame_done(function()
    local anm = r(0xC0)
    if anm ~= 0 and not wasanm then
        rolls = rolls + 1
        for i = 1, 5 do changed[i] = 0 end
    end
    if anm ~= 0 then
        for i = 1, 5 do
            local v = r(DP[i])
            if prev[i] ~= -1 and v ~= prev[i] then changed[i] = changed[i] + 1 end
            prev[i] = v
        end
    elseif wasanm then
        -- The wire's keepRoll names exactly the dice this roll was made with.
        local keep = ""
        for i = 0, 4 do
            local c = r(0x1B00 + 74 + i)
            keep = keep .. string.char(c == 0 and 46 or c)
        end
        local s = ""
        for i = 1, 5 do s = s .. changed[i] .. " " end
        print(string.format("ANIM roll %d: keepRoll=%s changes per die: %s",
            rolls, keep, s))
    end
    wasanm = anm ~= 0
end)

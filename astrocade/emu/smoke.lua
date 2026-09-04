-- smoke.lua: headless smoke test of the whole flow, against a LOCAL
-- fujitzee server (table row 2 there is "AI Room - 2 bots"; verify with
--   curl 'http://127.0.0.1:8080/tables'
-- and adjust the join digit below if the order changes).
-- Press keypad 1 at the OS menu to launch, pull the trigger to accept
-- the default name, keypad 2 to join the 2-bot table, one trigger to
-- ready up -- then HANDS OFF while the server's start countdown runs
-- (another trigger would un-ready and cancel it). From t=30 a trigger
-- every ~4 s plays legally blind: the dice cursor rests on the ROLL
-- tile so each press re-rolls everything, and when the rolls run out
-- the client jumps the cursor to the best open score row, so the next
-- press scores it. Snapshots mid-game and near the end (a full 13-round
-- game outlasts the run; the mid-game shot is the proof of play).
--   mame ... -autoboot_script emu/smoke.lua -video none -sound none \
--        -seconds_to_run 100
local function port_by_suffix(suffix)
    for tag, port in pairs(manager.machine.ioport.ports) do
        if tag:sub(-#suffix) == suffix then return port end
    end
    return nil
end

local function tap(name, field)
    local p = port_by_suffix(name)
    if p then p:field(field):set_value(1) end
end
local function untap(name, field)
    local p = port_by_suffix(name)
    if p then p:field(field):clear_value() end
end

local phase = 0
local shots = 0
local trig = false
emu.register_frame(function()
    local t = manager.machine.time.seconds
    if phase == 0 and t >= 3 then
        emu.print_info("smoke.lua: keypad 1 (launch)")
        tap("KEYPAD3", 0x10)
        phase = 1
    elseif phase == 1 and t >= 5 then
        untap("KEYPAD3", 0x10)
        phase = 2
    elseif phase == 2 and t >= 8 then
        emu.print_info("smoke.lua: trigger (accept name)")
        tap("ctrl1:joy:HANDLE", 0x10)
        phase = 3
    elseif phase == 3 and t >= 9 then
        untap("ctrl1:joy:HANDLE", 0x10)
        phase = 4
    elseif phase == 4 and t >= 13 then
        emu.print_info("smoke.lua: keypad 2 (join the 2-bot table)")
        tap("KEYPAD2", 0x10)
        phase = 5
    elseif phase == 5 and t >= 14 then
        untap("KEYPAD2", 0x10)
        phase = 6
    elseif phase == 6 and t >= 18 then
        emu.print_info("smoke.lua: trigger (ready) -- then hands off")
        tap("ctrl1:joy:HANDLE", 0x10)
        phase = 7
    elseif phase == 7 and t >= 19 then
        untap("ctrl1:joy:HANDLE", 0x10)
        phase = 8
    elseif phase == 8 and t >= 30 then
        -- play: a trigger every ~4 s rolls (cursor on ROLL) and scores
        -- (PICKBEST parks the cursor once the rolls run out)
        local m = t % 4
        if m < 0.5 and not trig then
            tap("ctrl1:joy:HANDLE", 0x10)
            trig = true
        elseif m >= 0.5 and trig then
            untap("ctrl1:joy:HANDLE", 0x10)
            trig = false
        end
    end
    if shots == 0 and t >= 60 then
        emu.print_info("smoke.lua: mid-game snapshot")
        manager.machine.video:snapshot()
        shots = 1
    elseif shots == 1 and t >= 94 then
        emu.print_info("smoke.lua: final snapshot")
        manager.machine.video:snapshot()
        shots = 2
    elseif t >= 96 then
        manager.machine:exit()
    end
end)

-- drive.lua -- play a game against the live server, headless.
--
-- EVERY WAIT IS ON THE CLIENT'S OWN STATE -- the live bank at $1F0E and the
-- zero-page cells -- and never on a frame number, so a slow socket round trip
-- cannot flake the run. Inputs are edge-detected by the client, so every press
-- is released again with frames in between.
local m = manager.machine.devices[":maincpu"].spaces["program"]
local io_ = manager.machine.ioport.ports
local function r(a) return m:readv_u8(a) end

local Z = {bank=0x1F0E, ent=0xA5, err=0x9A, step=0x9B, rnd=0xAE, pc=0xAD,
           act=0xAF, cls=0xA7, poll=0xA6, myturn=0xB0, rleft=0xB6,
           keep=0xB5, dcur=0xB3, uimd=0xB2, scur=0xB4, anm=0xC0,
           view=0xB1, revsl=0xBF, hl=0x9E, redrw=0xC5}

local frames, script, si = 0, {}, 1
local held = {}

local FIELDS = {}
for _, p in pairs(io_) do
    for name, f in pairs(p.fields) do FIELDS[name] = f end
end
local function press(name, on)
    local f = FIELDS[name]
    if f then f:set_value(on and 1 or 0) end
end

-- A press is three frames down and three up: the client edge-detects, and a
-- press that spans a bank switch would otherwise be seen twice or not at all.
local function tap(name)
    return {kind="tap", name=name, n=0}
end
local function waitfor(what, fn, limit)
    return {kind="wait", what=what, fn=fn, limit=limit or 3600, n=0}
end
local function say(msg) return {kind="say", msg=msg} end

-- The exact ioport field names, from emu/ports.lua. Not a substring search:
-- "P1 Left" and "P2 Left" both contain "left", and a driver that pressed the
-- second controller would look exactly like a client that ignores input.
local KEY = {
    fire = "P1 Button 1",
    up   = "P1 Up",    down  = "P1 Down",
    left = "P1 Left",  right = "P1 Right",
    sel  = "Select Game", rst = "Reset Game",
    diff = "Left Diff. Switch",
}
for k, n in pairs(KEY) do
    if not FIELDS[n] then error("drive: no ioport field " .. n) end
end

script = {
    say("waiting for the table list"),
    waitfor("lobby", function() return r(Z.bank) == 0 and r(Z.ent) == 1 end),
    say("joining"),
    tap("fire"),
    waitfor("a state", function() return r(Z.rnd) ~= 0 or r(Z.pc) ~= 0 end),
    say("ready up"),
    tap("fire"),
    waitfor("play", function() return r(Z.rnd) >= 1 and r(Z.rnd) <= 13 end, 7200),
    say("in play"),
    {kind="play"},
}

-- DRIVE_NOPLAY=1 stops after the game starts and hands the controls over, so
-- a test that wants to press things itself is not arguing with the play loop.
if os.getenv("DRIVE_NOPLAY") then script[#script] = nil end

-- Playing a turn: roll while there are rolls left and the cursor is on the
-- ROLL tile, then cross to the card and take whatever PICKBEST parked on.
-- Nothing here chooses a row: the client's own auto-park is what is being
-- tested, and a driver that picked its own would not be testing it.
local play = {rolls=0, scores=0, held=0, turn=-1, step=0}
local function playstep()
    local b = r(Z.bank)
    if not (b == 0 or b == 1 or b == 3 or b == 4) then return nil end
    if r(Z.myturn) == 0 then return nil end
    if r(Z.anm) ~= 0 then return nil end          -- the dice are still rattling

    if r(Z.rleft) ~= 0 then
        if r(Z.uimd) ~= 0 then return nil end
        -- On the SECOND roll of a turn, walk into the tray and keep a die
        -- before rolling again. That is the part a player actually does, and
        -- it is what exercises the cursor bar, the hold markers and the art.
        if r(Z.rleft) == 2 and play.step < 3 then
            if play.step == 0 then play.step = 1; return "left" end
            if play.step == 1 then
                play.step = 2; play.held = play.held + 1; return "fire"
            end
            -- THEN SIT ON IT. A hold has to survive a POLL, and a driver that
            -- rolled again three frames later would never find out: the echo
            -- it resyncs from is only stale between one roll and the next.
            play.linger = (play.linger or 0) + 1
            if play.linger < 150 then return nil end
            play.linger, play.step = 0, 3
            return "right"
        end
        if r(Z.dcur) == 5 then
            play.rolls = play.rolls + 1
            return "fire"
        end
        return "right"
    end
    play.step, play.linger = 0, 0
    if r(Z.uimd) == 0 then return "up" end        -- cross to the card
    play.scores = play.scores + 1
    return "fire"
end

-- Seat 0's scores[0], which in the lobby IS the ready flag. Read out of the
-- reply window, where it still lies: nothing is buffered.
local function ready0()
    return r(0x1B00 + 95 + 10) + 256 * r(0x1B00 + 95 + 11)
end

local function dump(tag)
    print(string.format("%s bank=%d ent=%02X cls=%d rnd=%d pc=%d act=%02X "
        .. "my=%d rl=%d keep=%02X dcur=%d uimd=%d scur=%d view=%d rev=%02X "
        .. "err=%02X/%02X poll=%d",
        tag, r(Z.bank), r(Z.ent), r(Z.cls), r(Z.rnd), r(Z.pc), r(Z.act),
        r(Z.myturn), r(Z.rleft), r(Z.keep), r(Z.dcur), r(Z.uimd), r(Z.scur),
        r(Z.view), r(Z.revsl), r(Z.err), r(Z.step), r(Z.poll))
        .. string.format(" rdy0=%04X req2=%02X dp=%02X%02X,%02X%02X,%02X%02X,%02X%02X,%02X%02X",
            ready0(), r(0xC4),
            r(0xC7), r(0xC6), r(0xC9), r(0xC8), r(0xCB), r(0xCA),
            r(0xCD), r(0xCC), r(0xCF), r(0xCE)))
end

local SNAP_AT = tonumber(os.getenv("SNAP_AT") or "0")
local WIRE_AT = tonumber(os.getenv("WIRE_AT") or "0")

-- The reply window, raw. Nothing here is buffered, so this is exactly what
-- every renderer on the screen is reading.
local function wiredump()
    local function str(off, len)
        local s = ""
        for i = 0, len - 1 do
            local c = r(0x1B00 + off + i)
            s = s .. (c >= 32 and c < 127 and string.char(c) or ".")
        end
        return s
    end
    print("WIRE pc=" .. r(0x1B00) .. ' srv="' .. str(1, 21) .. '"')
    print('WIRE prompt="' .. str(22, 41) .. '"')
    print(string.format("WIRE rnd=%d rl=%d act=%d mv=%d view=%d",
        r(0x1B00+63), r(0x1B00+64), r(0x1B00+65), r(0x1B00+66), r(0x1B00+67)))
    print('WIRE dice="' .. str(68, 6) .. '" keep="' .. str(74, 6) .. '"')
    local v = ""
    for i = 0, 14 do v = v .. string.format("%d,", r(0x1B00+80+i)) end
    print("WIRE valid=" .. v)
    for i = 0, math.min(r(0x1B00), 6) - 1 do
        local b = 95 + 42 * i
        print(string.format('WIRE seat %d name="%s" sc0=%04X', i,
            str(b, 9), r(0x1B00+b+10) + 256*r(0x1B00+b+11)))
    end
end

emu.register_frame_done(function()
    frames = frames + 1
    -- MAME REMEMBERS SWITCH POSITIONS BETWEEN RUNS, in its own cfg, and
    -- re-applies them after the machine starts -- so this has to be done on a
    -- frame and not at load time. A run that left the left difficulty up made
    -- the next one boot straight into the standings screen, where there is no
    -- card and no turn, which looks exactly like a client that never joined.
    if frames < 4 then FIELDS["Left Diff. Switch"]:set_value(0) end
    if SNAP_AT > 0 and frames == SNAP_AT then
        manager.machine.video:snapshot()
    end
    if WIRE_AT > 0 and frames == WIRE_AT then wiredump() end
    if frames % 300 == 0 then dump(string.format("%5d", frames)) end
    local s = script[si]
    if not s then return end
    if s.kind == "say" then
        print("-- " .. s.msg); dump("   "); si = si + 1; return
    end
    if s.kind == "tap" then
        -- ONLY THE BANKS THAT SCAN INPUT SEE A PRESS: the lobby, the game,
        -- the menu and the keyboard. The network and composer banks' hooks do
        -- their own work and nothing else, so a press made while one of them
        -- is mapped is simply not there when it is looked for. A player holds
        -- a button for longer than a recompose takes; a driver taps for six
        -- frames and would miss it every time.
        local b = r(Z.bank)
        if s.n == 0 and not (b == 0 or b == 1 or b == 3 or b == 4) then
            return
        end
        s.n = s.n + 1
        if s.n == 1 then
            print(string.format("   press %s (bank %d)", s.name, r(Z.bank)))
            press(KEY[s.name], true)
        elseif s.n == 6 then press(KEY[s.name], false)
        elseif s.n > 6 then
            -- Wait for the RELEASE to be seen, not merely made. The client
            -- edge-detects, and only the input-scanning banks update what it
            -- compares against, so a release that falls entirely inside a
            -- compose never happened as far as the next press is concerned.
            local b = r(Z.bank)
            if b == 0 or b == 1 or b == 3 or b == 4 then
                s.seen = (s.seen or 0) + 1
                if s.seen >= 10 then si = si + 1 end
            end
        end
        return
    end
    if s.kind == "play" then
        s.n = (s.n or 0) + 1
        if s.hold and s.hold > 0 then
            s.hold = s.hold - 1
            if s.hold == 3 then press(KEY[s.key], false) end
            return
        end
        local want = playstep()
        if want then
            print(string.format("   play %s (rl=%d dcur=%d mcur=%d keep=%02X "
                .. "rolls=%d holds=%d scores=%d rnd=%d)", want, r(Z.rleft),
                r(Z.dcur), r(0xD1), r(Z.keep), play.rolls, play.held,
                play.scores, r(Z.rnd)))
            s.key = want
            s.hold = 24
            press(KEY[want], true)
        end
        return
    end
    if s.kind == "wait" then
        s.n = s.n + 1
        if s.fn() then
            print(string.format("-- %s after %d frames", s.what, s.n))
            si = si + 1
        elseif s.n > s.limit then
            print("!! TIMEOUT waiting for " .. s.what)
            dump("   ")
            manager.machine:exit()
        end
        return
    end
end)

-- drive.lua -- play the client through the hand controller, headlessly.
--
--   DRIVE="fire,hash,fire" SCREEN_AT=12 ./run.sh --drive
--
-- Actions: up down left right fire, k0..k9, star, hash, wait.
-- SCREEN_EXPECT is both the trigger and the verdict: the screen is dumped the
-- first moment that text appears, plus DRIVE_AFTER seconds, and SCREEN_AT is
-- the deadline after which it dumps anyway and fails. Waiting on a CONDITION
-- rather than a clock is what makes a test of "my turn" reproducible against a
-- live server whose bots take as long as they take.
--
-- Two things here are not incidental:
--
--   Presses are timed off machine.time:as_double(), NOT machine.time.seconds.
--   The latter is the whole-seconds FIELD, so comparing against it quantises
--   every interval to a full second -- which holds a 0.3 s press for a whole
--   one and lets auto-repeat walk the cursor several rows per "press".
--
--   The frame notifier lives in _G. add_machine_frame_notifier returns an RAII
--   token that unsubscribes at the next garbage collection if you drop it, so
--   a callback scheduled a couple of seconds out fires and one scheduled ten
--   seconds out silently never does.

local SCRIPT  = os.getenv("DRIVE") or "fire"
local AT      = tonumber(os.getenv("SCREEN_AT") or "10")
local EXPECT  = os.getenv("SCREEN_EXPECT")
local HOLD    = tonumber(os.getenv("DRIVE_HOLD") or "0.30")
local GAP     = tonumber(os.getenv("DRIVE_GAP") or "0.60")
local SETTLE  = tonumber(os.getenv("DRIVE_SETTLE") or "3.0")
local TRACE   = os.getenv("DRIVE_TRACE")   -- print row 0 once a second
local AFTER   = tonumber(os.getenv("DRIVE_AFTER") or "0")

local PNT, COLS, ROWS = 0x1800, 32, 24
local TILEMARK = string.byte("*")

local JOY = { up = 0x01, right = 0x02, down = 0x04, left = 0x08, fire = 0x40 }
local PAD = { k0 = 0x0001, k1 = 0x0002, k2 = 0x0004, k3 = 0x0008,
              k4 = 0x0010, k5 = 0x0020, k6 = 0x0040, k7 = 0x0080,
              k8 = 0x0100, k9 = 0x0200, hash = 0x0400, star = 0x0800 }

local function find_port(suffix)
    for tag, port in pairs(manager.machine.ioport.ports) do
        if tag:sub(-#suffix) == suffix then return port end
    end
    error("drive.lua: no input port ending in " .. suffix)
end

local function press(action, on)
    if action == "wait" then return end
    local bit, port
    if JOY[action] then
        bit, port = JOY[action], find_port("STD_JOY1")
    elseif PAD[action] then
        bit, port = PAD[action], find_port("STD_KEYPAD1")
    else
        error("drive.lua: unknown action '" .. action .. "'")
    end
    local f = port:field(bit)
    if f == nil then
        error(string.format("drive.lua: no field %#x on that port", bit))
    end
    if on then f:set_value(1) else f:clear_value() end
end

-- Fold Fujitzee's banked Graphics II codes back to ascii; see emu/screen.lua.
local function read_screen()
    local vdp = manager.machine.devices[":tms9928a"]
    if vdp == nil then return nil, "no :tms9928a device" end
    local vram = vdp.spaces["vram"] or vdp.spaces["videoram"] or vdp.spaces["data"]
    if vram == nil then return nil, "no VRAM space" end
    local lines, marks = {}, {}
    for row = 0, ROWS - 1 do
        local chars, accent, green = {}, false, false
        for col = 0, COLS - 1 do
            local b = vram:read_u8(PNT + row * COLS + col)
            if b >= 0xF0 then b = b - 0xC0; green = true
            elseif b >= 0xB0 then b = b - 0x90; accent = true
            elseif b >= 0x70 then b = b - 0x50
            else b = TILEMARK end
            if b < 32 or b > 126 then b = 32 end
            chars[#chars + 1] = string.char(b)
        end
        lines[#lines + 1] = table.concat(chars)
        marks[#marks + 1] = green and "$" or (accent and "#" or "|")
    end
    return lines, nil, marks
end

local script = {}
for a in SCRIPT:gmatch("[^,%s]+") do script[#script + 1] = a end

if _G.drive == nil then
    _G.drive = { step = 0, t_next = SETTLE, down = false, done = false }
end
local d = _G.drive

_G.drive_sub = emu.add_machine_frame_notifier(function ()
    local now = manager.machine.time:as_double()

    -- One line a second of the server's prompt and the header fields it turns
    -- on, so a single run shows where the turns actually fall instead of
    -- needing one run per guess at a sample time. The reply window is only
    -- stable between transactions, so an occasional line catches a CLOSE or
    -- STATUS reply and reads round 0 -- that is the trace, not the client.
    if TRACE then
        local t = math.floor(now)
        if t ~= _G.trace_t then
            _G.trace_t = t
            local lines = read_screen()
            if lines then
                local m = manager.machine.devices[":maincpu"].spaces["program"]
                emu.print_info(string.format(
                    "t=%3d | %-26s | rnd=%2d act=%3d rolls=%d move=%3d keep=%s",
                    t, lines[1]:sub(1, 26),
                    m:readv_u8(0xF800 + 63), m:readv_u8(0xF800 + 65),
                    m:readv_u8(0xF800 + 64), m:readv_u8(0xF800 + 66),
                    (function ()
                        local o = {}
                        for i = 0, 4 do
                            o[#o+1] = string.char(math.max(46,
                                m:readv_u8(0xF800 + 74 + i)))
                        end
                        return table.concat(o)
                    end)()))
            end
        end
    end

    -- Stepping the script and sampling the screen are independent: the sample
    -- must land at SCREEN_AT even mid-script, because the interesting moment
    -- is usually in the middle of the presses, not after the last one.
    if not d.done then
        if now >= d.t_next then
            if d.down then
                press(script[d.step], false)
                d.down = false
                d.t_next = now + GAP
                if d.step >= #script then d.done = true end
            else
                d.step = d.step + 1
                if d.step > #script then
                    d.done = true
                else
                    press(script[d.step], true)
                    d.down = true
                    d.t_next = now + HOLD
                end
            end
        end
    end

    if d.fired then return end

    -- Arm on the condition, fire DRIVE_AFTER seconds later; or give up at the
    -- deadline and dump whatever is there, which is the failure evidence.
    if d.armed == nil then
        if EXPECT then
            local lines = read_screen()
            if lines and table.concat(lines, "\n"):find(EXPECT, 1, true) then
                d.armed = now + AFTER
                d.matched = true
            elseif now >= AT then
                d.armed = now
            else
                return
            end
        elseif now >= AT then
            d.armed = now
        else
            return
        end
    end
    if now < d.armed then return end
    d.fired = true

    local lines, err, marks = read_screen()
    if lines == nil then
        emu.print_info("drive: FAIL " .. err)
        manager.machine:exit()
        return
    end
    emu.print_info("+--------------------------------+")
    for i, l in ipairs(lines) do
        emu.print_info(marks[i] .. l .. marks[i])
    end
    emu.print_info("+--------------------------------+")

    -- A PNG of the real thing. The name table is not the screen: a VDP left
    -- in the wrong mode, or a charset that only half uploaded, reads back
    -- perfectly and displays nothing. Both happened here.
    if os.getenv("SNAPOUT") then
        local ok, err = pcall(function ()
            manager.machine.screens[":screen"]:snapshot(os.getenv("SNAPOUT"))
        end)
        emu.print_info("snapshot: " .. (ok and os.getenv("SNAPOUT")
                                           or ("FAILED " .. tostring(err))))
    end

    local cpu = manager.machine.devices[":maincpu"]
    emu.print_info(string.format("drive: PC=%04X ACKSEQ=%02X ERR=%02X RXLEN=%d",
        cpu.state["PC"].value,
        cpu.spaces["program"]:readv_u8(0xFC00),
        cpu.spaces["program"]:readv_u8(0xFC02),
        cpu.spaces["program"]:readv_u8(0xFC04)
            + cpu.spaces["program"]:readv_u8(0xFC05) * 256))

    if EXPECT then
        if d.matched then
            emu.print_info("drive: PASS")
        else
            emu.print_info("drive: FAIL never saw " .. string.format("%q", EXPECT))
        end
    end
    manager.machine:exit()
end)

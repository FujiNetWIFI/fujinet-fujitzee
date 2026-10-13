-- smoke.lua -- read the Atari 5200 screen out of RAM and give a verdict. The
-- 5200 counterpart of support/atari7800/smoke.lua.
--
-- The screen (src/atari5200/antic.s) is 40x26 ANTIC mode 4 codes at $3000,
-- in the 8-bit client's charset: codes 0-32 are " " to "@", 97-122 are the
-- letters (printed in lower case), $80 picks the other colour, and the rest
-- is game art, which reads as "#". EXPECT is matched ignoring case.
--
--   FZ_SCREEN  the screen's address (default 0x3000)
--   FZ_AT      emulated seconds to settle before sampling; measured from the
--               end of FZ_SCRIPT when there is one
--   FZ_SETTLE  emulated seconds before the first press (default 8)
--   FZ_SCRIPT  comma-separated controller actions to play first, e.g.
--               "fire,wait10,fire" -- up down left right (the stick) fire
--               (top) two (lower) start pause reset star hash k0-k9, and
--               waitN to idle N emulated seconds (default 1)
--   FZ_EXPECT  substring that must appear on screen; sets the exit verdict
--   FZ_QUIET   only print the verdict, not the screen
--   FZ_SNAP    path to also save a PNG screenshot of the sampled frame; a
--               "snap" step in FZ_SCRIPT saves one mid-script, numbered by step

local AT      = tonumber(os.getenv("FZ_AT") or "8")
local EXPECT  = os.getenv("FZ_EXPECT")
if EXPECT == "" then EXPECT = nil end   -- make/env hand through an empty string
local QUIET   = os.getenv("FZ_QUIET")
local SCRIPT  = os.getenv("FZ_SCRIPT")
local SCREEN  = tonumber(os.getenv("FZ_SCREEN") or "0x3000")

local COLS, ROWS = 40, 26

local function decode(c)
    c = c & 0x7F
    if c <= 32 then return string.char(c + 32) end
    if c >= 97 and c <= 122 then return string.char(c) end
    return "#"
end

local function read_screen()
    local mem = manager.machine.devices[":maincpu"].spaces["program"]
    local lines = {}
    for row = 0, ROWS - 1 do
        local chars = {}
        for col = 0, COLS - 1 do
            chars[#chars + 1] = decode(mem:read_u8(SCREEN + row * COLS + col))
        end
        lines[#lines + 1] = table.concat(chars)
    end
    return lines
end

-- A button is held; the stick is an analog value, centred on release.
local BTN = { fire = { ":djoy_b", "P1 Button 1" }, two = { ":djoy_b", "P1 Button 2" },
              hash = { ":keypad.0", "[#]" }, star = { ":keypad.0", "[*]" },
              k0 = { ":keypad.0", "[0]" },
              reset = { ":keypad.1", "Reset" }, k7 = { ":keypad.1", "[7]" },
              k8 = { ":keypad.1", "[8]" }, k9 = { ":keypad.1", "[9]" },
              pause = { ":keypad.2", "Pause" }, k4 = { ":keypad.2", "[4]" },
              k5 = { ":keypad.2", "[5]" }, k6 = { ":keypad.2", "[6]" },
              start = { ":keypad.3", "Start" }, k1 = { ":keypad.3", "[1]" },
              k2 = { ":keypad.3", "[2]" }, k3 = { ":keypad.3", "[3]" } }
local STICK = { up = { ":analog_1", "AD Stick Y", 0x00 }, down = { ":analog_1", "AD Stick Y", 0xE4 },
                left = { ":analog_0", "AD Stick X", 0x00 }, right = { ":analog_0", "AD Stick X", 0xE4 } }

-- SETTLE has to outlast the client's start-up round trips -- the appkey reads
-- and the first table fetch. The keypad needs a press held for its scan and
-- a gap before the next; the stick, several frames of the POT shadows.
local HOLD, GAP = 0.30, 0.45
local SETTLE = tonumber(os.getenv("FZ_SETTLE") or "8")

local function is_wait(action)
    return action:match("^wait(%d*)$") ~= nil
end

local function wait_secs(action)
    local n = action:match("^wait(%d*)$")
    return (n == "" or n == nil) and 1 or tonumber(n)
end

local function field(port, name)
    local f = manager.machine.ioport.ports[port].fields[name]
    if f == nil then error("smoke.lua: no field " .. name) end
    return f
end

local function press(action, on)
    local s = STICK[action]
    if s then
        local f = field(s[1], s[2])
        if on then f:set_value(s[3]) else f:clear_value() end
        return
    end
    local b = BTN[action]
    if b == nil then error("smoke.lua: unknown action '" .. action .. "'") end
    field(b[1], b[2]):set_value(on and 1 or 0)
end

local script = {}
if SCRIPT then
    for a in SCRIPT:gmatch("[^,%s]+") do script[#script + 1] = a end
end

if _G.fz_state == nil then
    _G.fz_state = { fired = false, step = 1, down = false, t_next = SETTLE }
end
local st = _G.fz_state

-- The subscription has to stay in a live global: add_machine_frame_notifier
-- hands back an RAII token, and letting it fall out of scope unsubscribes at
-- the next Lua collection.
_G.fz_sub = emu.add_machine_frame_notifier(function ()
    if st.fired then return end
    local now = manager.machine.time:as_double()

    if st.step <= #script then
        if now < st.t_next then return end
        if script[st.step] == "snap" then
            local base = (os.getenv("FZ_SNAP") or "snap.png"):gsub("%.png$", "")
            local name = base .. "-" .. tostring(st.step) .. ".png"
            manager.machine.screens[":screen"]:snapshot(name)
            emu.print_info("fz: snap " .. name)
            st.step = st.step + 1
            if st.step > #script then st.t_sample = now + AT end
            return
        end
        if is_wait(script[st.step]) then
            st.t_next = now + wait_secs(script[st.step])
            st.step = st.step + 1
            if st.step > #script then st.t_sample = st.t_next + AT end
            return
        end
        if st.down then
            press(script[st.step], false)
            st.down = false
            st.step = st.step + 1
            st.t_next = now + GAP
            if st.step > #script then st.t_sample = now + AT end
        else
            press(script[st.step], true)
            st.down = true
            st.t_next = now + HOLD
        end
        return
    end

    if now < (st.t_sample or AT) then return end
    st.fired = true

    local lines = read_screen()
    if not QUIET then
        print("+" .. string.rep("-", COLS) .. "+")
        for _, l in ipairs(lines) do print("|" .. l .. "|") end
        print("+" .. string.rep("-", COLS) .. "+")
    end

    -- "The screen looks right" is not proof on its own: the screen keeps
    -- whatever was last drawn. The PC and the mailbox status page say
    -- whether the client is still running and what the last transaction said.
    -- (The status page only: a read of the hotspot pages is an event.)
    do
        local cpu = manager.machine.devices[":maincpu"]
        local mem = cpu.spaces["program"]
        print(string.format("fz: PC=%04X ACKSEQ=%02X STATUS=%02X ERR=%02X",
            cpu.state["PC"].value,
            mem:read_u8(0xB400), mem:read_u8(0xB401), mem:read_u8(0xB402)))
    end

    local snap = os.getenv("FZ_SNAP")
    if snap and snap ~= "" then
        local serr = manager.machine.screens[":screen"]:snapshot(snap)
        emu.print_info("fz: snapshot " .. snap .. (serr and (" FAILED: " .. tostring(serr)) or ""))
    end

    if EXPECT then
        local joined = table.concat(lines, "\n"):lower()
        if joined:find(EXPECT:lower(), 1, true) then
            print("fz: PASS")
        else
            print("fz: FAIL expected " .. string.format("%q", EXPECT))
        end
    end
    manager.machine:exit()
end)

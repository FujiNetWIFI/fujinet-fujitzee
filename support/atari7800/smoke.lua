-- smoke.lua -- read the Atari 7800 screen out of the engine's map and give a
-- verdict. The 7800 counterpart of support/nes/smoke.lua.
--
-- The map (src/atari7800/maria.s) is 32x24 bytes at the start of the cart's
-- RAM, each tile * 2. The tiles are not ASCII: src/atari7800/mkchr.py packs
-- the MS-DOS art, so a tile is decoded through the tile -> character table
-- the same script writes (support/atari7800/tilemap.lua). Game art reads as
-- "#".
--
--   FZ_TILEMAP path to support/atari7800/tilemap.lua (required)
--   FZ_MAP     the map's address (default 0x4000, the VIDEO segment)
--   FZ_AT      emulated seconds to settle before sampling; measured from the
--               end of FZ_SCRIPT when there is one
--   FZ_SETTLE  emulated seconds before the first press (default 8)
--   FZ_SCRIPT  comma-separated joystick actions to play first, e.g.
--               "fire,wait10,fire" -- up down left right fire two select
--               pause reset, a chord like select+two, and
--               waitN to idle N emulated seconds (default 1)
--   FZ_EXPECT  substring that must appear on screen; sets the exit verdict
--   FZ_QUIET   only print the verdict, not the screen
--   FZ_SNAP    path to also save a PNG screenshot of the sampled frame; a
--               "snap" step in FZ_SCRIPT saves one mid-script, numbered by step

local AT      = tonumber(os.getenv("FZ_AT") or "8")
local EXPECT  = os.getenv("FZ_EXPECT")
if EXPECT == "" then EXPECT = nil end   -- make/env hand through an empty string
local QUIET   = os.getenv("FZ_QUIET")
local TILEMAP = os.getenv("FZ_TILEMAP")
local SCRIPT  = os.getenv("FZ_SCRIPT")
local MAP     = tonumber(os.getenv("FZ_MAP") or "0x4000")

local COLS, ROWS = 32, 24

local tilemap
local function load_tilemap()
    if TILEMAP == nil then return "FZ_TILEMAP is not set" end
    local chunk, err = loadfile(TILEMAP)
    if chunk == nil then return err end
    tilemap = chunk()
    return nil
end

local function read_screen()
    local mem = manager.machine.devices[":maincpu"].spaces["program"]
    local lines = {}
    for row = 0, ROWS - 1 do
        local chars = {}
        for col = 0, COLS - 1 do
            local t = mem:read_u8(MAP + row * COLS + col) >> 1
            chars[#chars + 1] = tilemap[t] or "?"
        end
        lines[#lines + 1] = table.concat(chars)
    end
    return lines
end

local BTN = { up = { ":JOYSTICKS", "P1 Up" }, down = { ":JOYSTICKS", "P1 Down" },
              left = { ":JOYSTICKS", "P1 Left" }, right = { ":JOYSTICKS", "P1 Right" },
              fire = { ":BUTTONS", "P1 Button 1" }, two = { ":BUTTONS", "P1 Button 2" },
              select = { ":CONSOLE", "Select" }, pause = { ":CONSOLE", "Pause" },
              reset = { ":CONSOLE", "Reset" } }

-- SETTLE has to outlast the client's start-up round trips -- the appkey reads
-- and the first table fetch. A press delivered while it is still inside one
-- of those is seen by the edge detector and dropped by whatever loop is not
-- yet running.
local HOLD, GAP = 0.30, 0.45
local SETTLE = tonumber(os.getenv("FZ_SETTLE") or "8")

local function is_wait(action)
    return action:match("^wait(%d*)$") ~= nil
end

local function wait_secs(action)
    local n = action:match("^wait(%d*)$")
    return (n == "" or n == nil) and 1 or tonumber(n)
end

-- "select+two" holds both together: a chord.
local function press(action, on)
    for b in action:gmatch("[^+]+") do
        local name = BTN[b]
        if name == nil then error("smoke.lua: unknown action '" .. b .. "'") end
        local f = manager.machine.ioport.ports[name[1]].fields[name[2]]
        if f == nil then error("smoke.lua: no field " .. name[2]) end
        f:set_value(on and 1 or 0)
    end
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

    local err = load_tilemap()
    if err then
        emu.print_info("fz: FAIL " .. err)
        manager.machine:exit()
        return
    end

    local lines = read_screen()
    if not QUIET then
        print("+--------------------------------+")
        for _, l in ipairs(lines) do print("|" .. l .. "|") end
        print("+--------------------------------+")
    end

    -- "The screen looks right" is not proof on its own: the map keeps
    -- whatever was last drawn. The PC and the mailbox status page say
    -- whether the client is still running and what the last transaction said.
    do
        local cpu = manager.machine.devices[":maincpu"]
        local mem = cpu.spaces["program"]
        print(string.format("fz: PC=%04X ACKSEQ=%02X STATUS=%02X ERR=%02X",
            cpu.state["PC"].value,
            mem:read_u8(0x0C00), mem:read_u8(0x0C01), mem:read_u8(0x0C02)))
    end

    local snap = os.getenv("FZ_SNAP")
    if snap and snap ~= "" then
        local serr = manager.machine.screens[":screen"]:snapshot(snap)
        emu.print_info("fz: snapshot " .. snap .. (serr and (" FAILED: " .. tostring(serr)) or ""))
    end

    if EXPECT then
        local joined = table.concat(lines, "\n")
        if joined:find(EXPECT, 1, true) then
            print("fz: PASS")
        else
            print("fz: FAIL expected " .. string.format("%q", EXPECT))
        end
    end
    manager.machine:exit()
end)

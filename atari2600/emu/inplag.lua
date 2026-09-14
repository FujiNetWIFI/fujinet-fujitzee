-- inplag2.lua -- does a press get LATCHED within a frame, whatever bank is
-- mapped?
--
-- Measures the plumbing, not the game: the cursor is also being driven by the
-- play loop, so timing the cursor times an argument between two drivers. What
-- matters is FZILAT -- DFRAME's once-a-frame scan OR'd into a latch -- because
-- that is the thing that used to stop happening for twenty-four frames after
-- every poll.
dofile(os.getenv("DRIVE_LUA"))
local sp = manager.machine.devices[":maincpu"].spaces["program"]
local F = {}
for _, p in pairs(manager.machine.ioport.ports) do
    for n, f in pairs(p.fields) do F[n] = f end
end
local function r(a) return sp:readv_u8(a) end
local function scans(b) return b == 0 or b == 1 or b == 3 or b == 4 end

local n, phase, t0, tries, worst = 0, "idle", 0, 0, 0
local incompose, results = 0, {}
emu.register_frame_done(function()
    n = n + 1
    local b = r(0x1F0E)
    if phase == "idle" then
        if n > 900 and not scans(b) then
            phase, t0 = "press", n
            incompose = incompose + 1
            F["P1 Up"]:set_value(1)
        end
    elseif phase == "press" then
        if (r(0xD2) % 2) == 1 then             -- FZILAT, IN_UP
            results[#results+1] = n - t0
            if n - t0 > worst then worst = n - t0 end
            F["P1 Up"]:set_value(0)
            phase, t0, tries = "cool", n, tries + 1
        elseif n - t0 > 120 then
            results[#results+1] = -1
            F["P1 Up"]:set_value(0)
            phase, t0, tries = "cool", n, tries + 1
        end
    elseif phase == "cool" then
        if n - t0 > 60 then phase = (tries >= 8) and "done" or "idle" end
    end
end)
emu.register_stop(function()
    local s = ""
    for _, d in ipairs(results) do s = s .. (d < 0 and "MISSED " or (d .. " ")) end
    print("LATCH delay (frames) for presses begun mid-compose: " .. s)
    print(string.format("WORST %d frames over %d presses", worst, incompose))
end)

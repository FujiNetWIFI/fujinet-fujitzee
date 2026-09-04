# Fujitzee for the Bally Astrocade

A standalone Z80 assembly client in the battleship/astrocade mold: the
shared C core cannot fit this machine, so the game is written to it. It
talks to the Fujitzee server through the FujiNet Astrocade cartridge —
the RP2040 mailbox cart from `fujinet-firmware/pico/astrocade` — and its
gameplay logic is transcribed from the Intellivision port (`intv/`), the
freshest client written directly against the binary wire format.

Built and tested against a local server in MAME through a real
fujinet-pc BoIP listener, through a complete 13-round bot game.

## Building and running

    ./build.sh                # build/fujitzee.bin, exactly 8192 bytes
    ./run.sh                  # MAME with the fujinet cart device
    make smoke                # headless end-to-end test (see below)

Environment:

  * `ENDPOINT=` — game server, default
    `https://fujitzee.carr-designs.com/`. Regenerated into
    `build/endpoint.inc` on every build.
  * `DEMO=1` — assemble the static mock play screen (`demo.inc`)
    instead of the game, for tuning the layout by screenshot.
  * `ZMAC=` — assembler override. Otherwise zmac is taken from `PATH`,
    then `~/Workspace/zmac-1.3/zmac`, then `$FUJI_PICO/tools/zmac/zmac`.
  * `FUJI_PICO=` — the cartridge bring-up tree, only consulted as the
    last place to look for zmac.

The port is self-contained the way the battleship client is:
`tools/checkrom.py` is vendored here and `fujilib.inc` / `HVGLIB.H` live
in the port, so a build needs no particular branch of the firmware tree
checked out. Those are copies of the bring-up's `testrom/` files — keep
them in step. `assets/font.inc` is the card-game ports' committed 4x6
font, vendored byte-for-byte.

`run.sh` expects the MAME tree with the fujinet cart device grafted in
(`pico/astrocade/emu/apply.sh`) at `MAME_DIR` (default `~/Workspace/mame`)
and a fujinet-pc BoIP listener at `FUJINET_TCP` (default 127.0.0.1:9995).
At the on-screen menu, keypad **1** starts the game.

## The cartridge budget

The cart serves an 8K window; the mailbox owns 1B00H up, so code and
data end at 1AFFH — 6,912 bytes, enforced by `checkrom.py` and itemised
by `tools/checksize.py` on every build (the `MB_*` labels in
`fujitzee.asm` are its module fences). `build.sh` stamps the `FUJI`
claim signature at 1CFCH, so when this image is booted over the network
the cart keeps the mailbox alive for it.

RAM is screen RAM, full stop. 90 visible lines use 4000H–4E0FH and
everything above is the game's — see the map in `fujitzee.asm`.
Interrupts stay off for the program's whole life (fujilib's contract:
with I = 0, refresh strays land in OS ROM and never hit the hotspots).

## The screen

The server owns the whole game — every endpoint returns the same state
blob, `validScores[]` carries the exact points each open row would earn,
and the only arithmetic in the client is the running total (`card.inc`'s
RUNTOT, because the wire's grand total is empty until game over). So the
screen is one 40x15 text grid over the family's byte-aligned 4x6
renderer, plus the dice:

```
row 0     viewed player           RD nn/13
rows 1-8  ONE..SIX,UP,BON | SET3..CNT,FUJI,TOT | standings panel
          (the intv two-column card)           | name  total  TURN
row 9     YOUR TURN - ROLL OR SCORE (red)      | < > VIEW hint (row 8)
rows 10-11  ROLL tile + five 12x12 dice
row 12    HLD markers under held dice / the dice cursor
row 14    server prompt                          clock (3 digits, red)
```

The Intellivision's 20 columns forced a seat strip and a hold-to-view
standings overlay; this machine's 40 columns replace both with an
always-visible standings panel — every seat's name, live running total,
and a red TURN tag. One scorecard shows at a time: yours on your turn,
the roller's otherwise (auto-followed), pageable with left/right while
you wait.

A die is 3 bytes x 12 lines at 2bpp, drawn from three 3-bit pip masks
per face (18 bytes of data for all six faces, `dice.inc`) — no bitmap
assets at all. A roll landing rattles only the dice the wire's keepRoll
echo says were re-rolled, so the animation is right for everyone's
rolls, not just yours. Value cells: filled scores white, an open row's
potential red (a zero shows only under the cursor), closed rows a black
dash, the cursor's cell inverse.

Four palette colors: felt green (the texasHoldEm table, 0A0H), black,
red, white.

## The Intellivision lessons carried over

* **The turn edge.** `activePlayer` stays 0 for every poll of your
  turn, so `PRVACT` advances at the bottom of every successful poll —
  lobby included — and the your-turn cue fires BEFORE the input slice.
* **The roll edge is a decrease.** A fresh turn resets `rollsLeft`
  upward, so only a decrease means a roll landed — that is what fires
  the dice animation, and it is muted across every screen boundary by
  the FFH sentinel in `PRVRLS`.
* **The ready toggle lives in the input path**, sampled every ~20 ms in
  `GLWAIT` and edge-detected, never in the once-per-poll renderer.
* **Validate before believing.** The reply window is never cleared;
  `VALID8` gates on the header, `playerCount <= 12`, the round on its
  enum (0–13 or 99), and `95 + 42*playerCount` bytes. And
  `validScores[]` is all zeros in the lobby — only play-class code may
  read it.
* **The clock bails.** The turn input counts down locally from the
  wire's moveTime and returns at zero — the server has already
  force-scored, and only the next poll learns where the game went.
* **A rejected press earns only a short slice.** An accepted key
  restarts the full ~5 s input slice (an aiming player is never
  interrupted), but a trigger bounced off a dead ROLL tile grants ~1 s
  — otherwise mashed rejects could starve the poll loop forever.
* **Rejoins land anywhere.** The server holds your seat, so your turn
  can arrive mid-turn with the rolls already spent: the my-turn edge
  restages the wire's keep mask when dice exist and drops straight into
  the score rows when `rollsLeft` is 0.

## Controls

    stick / keypad arrows   move the cursor / lists / the name wheel
    trigger                 select / ready up / hold a die / roll / score
    up (dice mode)          into the score rows; down from the bottom
                            returns to the dice while rolls remain
    left/right (waiting)    page the viewed scorecard
    keypad 0                poll now
    CE                      leave the table (name screen from the list)
    .                       how to play

On your turn the cursor starts on the ROLL tile; trigger rolls
everything. Trigger on a die holds it (HLD). After the last roll the
cursor jumps to the highest-paying open row, so trigger-trigger-trigger
plays a legal (if artless) game — which is exactly what the smoke test
does.

## Testing

`make smoke` runs MAME headless with `emu/smoke.lua` against a LOCAL
server: launch, accept the default name, join "AI Room - 2 bots" by
digit 2, ONE trigger to ready — then hands off while the server's start
countdown runs (a second trigger would un-ready and cancel it; the
countdown only advances on `/state` polls) — then a trigger every ~4 s
rolls and scores blind. Snapshots mid-game and at the end land in
`build/astrocde/`. `FUJINET_DEBUG=1` (default) logs every mailbox
transaction; a `/state` for an N-seat game reads back `95 + 42*N`
bytes. `emu/demoshot.lua` snapshots the `DEMO=1` mock screen.

Against a local server:

    (cd .../servers/fujinet-game-system/fujitzee/server && go run .) &
    ENDPOINT=http://127.0.0.1:8080/ ./build.sh
    make smoke

## Bank switching

Firmware protocol v2 supports banked carts: `fujilib.inc` now carries the
`FNBKSEL`/`FNBKMAX` equates (one read maps a 4K image page into
2000H-2FFFH with the mailbox fully live; the high half never moves). This
client still fits the single 8K window and does not use them -- see
`firmware/include/fuji_mailbox.h` in fujinet-firmware for the scheme.

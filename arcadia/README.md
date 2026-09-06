# Fujitzee for the Emerson Arcadia 2001

A 2650 assembly client for the [Fujitzee server](https://fujitzee.carr-designs.com/),
running over the FujiNet cartridge mailbox from the Arcadia bring-up at
`fujinet-firmware/pico/arcadia` (protocol v1: everything rides reads, and
A14 is not on the connector, so there is no banking). The game logic is
the astrocade port's, itself transcribed from the Intellivision client --
the freshest one written directly against the binary wire format -- and
the hardware half is battleship/arcadia's: binary `?bin=1`, zero-copy
rendering straight out of the reply window, opaque overdraw per poll, a
round/playerCount watcher instead of dirty rectangles.

## The screen

This client runs the 2637's **NORMAL colour mode**, which no other
FujiNet Arcadia client uses. The two card games take multicolor's two
inks over two backgrounds; Fujitzee wants more hues than backgrounds --
a filled score, a potential, the round number and a closed row all have
to differ -- and normal mode gives **four foreground colours over one
global background**. From `arcadia_v.cpp`'s `draw_char()`:

    fg = (($19F9>>3)&1) | ((cell>>5)&6)        bg = $19F9 & 7

so the cell's top two bits supply fg bits 2 and 1 while `$19F9` bit 3
supplies bit 0, shared by all four. With bit 3 clear, over a blue page
(`$19F8 = $40`, `$19F9 = $86`):

| attr | ink | used for |
|------|-----|----------|
| `$00` | white | labels, filled scores, ordinary text |
| `$40` | cyan | potential scores, the cursor'd ROLL tile, a ready seat |
| `$80` | magenta | the round, YOUR TURN, the clock, the active seat, a held die |
| `$C0` | blue | closed rows, a dead ROLL tile |

That last row is the page's own colour, so those cells are **invisible**
rather than dim -- the Intellivision port's `COL_DASH` trick exactly
(it drew black on a blue page for the same effect). A closed row simply
reads as absent. The corollary is a rule: **nothing that must be read
may be drawn in `ADIM`**. Two lines learned it the hard way, the
standings overflow line and the lobby's spectator tag.

The single bit that picks between this palette and the warm one is
`INKLSB` in `disp.inc`; setting it to `$08` gives yellow / green / red /
black instead, with the same four roles and `ADIM` merely dim rather
than invisible.

Two consequences of normal mode itself. There is no second background,
so no inverse cell: the scorecard cursor is a **sprite**, and a sprite
carries its own 3-bit colour -- here yellow, a fifth hue the screen inks
do not have, which is the whole reason the cursor can be seen against
white text. And `$18FD` bit 7, the multicolor enable, must stay
**clear** -- `sound.inc`'s pitch writes preserve it clear where the
other Arcadia ports' preserve it set. Setting it would repaint the whole
screen in someone else's palette.

The `$C0` hazard is handled more broadly here than in the card games. A
cell of exactly `$C0` -- a space with attrs 11 -- flips the rest of its
row into block graphics, and `$40` flips it back; black-on-blue *is* an
attribute this port paints with, so `$C0` spaces would otherwise be
common. But a blank cell shows only the global background, so every
attribute renders one identically: `DPUTC` forces the attribute to `$00`
whenever the glyph is 0, which defuses both codes at once and costs
nothing.

## Layout (16 x 26)

```
row  0    <viewed name>    R n/13      name in the seat's colour, round magenta
rows 1-8  the scorecard                labels col 0 / 8, values col 4 / 12
row  9    YOUR TURN                    magenta, blank while you wait
rows10-12 five dice, die i at col 1+i*3
row 13    hold markers, two magenta blocks under a die that will be kept
row 14    ROLL  X2
rows15-20 standings: name and live running total, one row per seat
row 21    +N MORE                      only when the seats overflow
rows22-24 the server's prompt, word-wrapped over three rows of 16
row 25    the move clock, three magenta cells
```

The scorecard is the Intellivision port's compact two-column form, which
happens to be exactly sixteen columns wide -- so it lands here without
being redesigned. Slots 0-7 (ONE..SIX, the upper total, the bonus) run
down the left and 8-15 (the seven lower rows, and the running total) down
the right. Both geometries are arithmetic, not a table: row is
`(slot & 7) + 1`, the label column is `slot & 8` and the value column
`(slot & 8) + 4`, which is also how the cursor sprite finds its row.

The intv port's 20 columns forced a seat strip and a hold-to-view
standings overlay. Twenty-six rows buy the real thing outright: every
seat's name, live running total, and the row drawn in that seat's colour
-- magenta for whoever is up, cyan for the card you are looking at. That
is this machine's answer to the intv's per-seat palette; six distinct
name colours do not exist here, but the three that carry meaning do.

One card shows at a time: the roller's, auto-followed on the turn edge,
pageable with left/right while you wait -- served entirely from the
cached reply, since every seat's record arrived in the same poll.

## The dice

A die is **2 cells wide by 3 tall** -- 16 x 24 pixels -- drawn as a
solid body in the die's state colour with the pips **punched through**
to the blue page. That inversion is what makes six faces cost three UDCs
instead of twenty-four: the body is the built-in full-cell block (`$03`)
and a pip is just a block with a hole in it. It also means an ordinary
die comes out white with blue pips -- which is what a die is supposed to
look like. The left and right pip
columns land at the same cell-relative position, so one glyph serves
both; only the centre pip straddles the cell boundary and needs its two
halves.

```
  1        2        3        4        5        6
 S S      P S      P S      P P      P P      P P
 L R      S S      L R      S S      L R      P P
 S S      S P      S P      P P      P P      P P
```

The die's colour is its state -- the Intellivision scheme (white /
yellow-kept / green-cursor) recast in the four inks this machine gives
us: **white** ordinary, **magenta** held, **cyan** under the cursor. The
cursor overrides, so the hold markers on the row beneath carry the held
state independently and a die that is both still reads.

A roll landing rattles only the dice the wire's `keepRoll` echo says were
re-rolled, so the animation is right for everyone's rolls, not just
yours.

## UDCs and the sprite

Eight UDCs at `$1980`, one bank loaded once at boot. The first four *are*
the sprite images, so slot `$38` holds the cursor:

* `$38` cursor chevron (= sprite 0's image, yellow)
* `$39` pip · `$3A` centre pip, left half · `$3B` centre pip, right half
* `$3C-$3F` spare

`assets/udc.inc` is generated from ASCII art by `tools/mkudc.py`
(`make assets`). Everything else comes from the character ROM: `$03` is
the die body, `$06` the closed-row dash, `$01` the `/` in "R n/13" and in
the lobby's "cur / max". `DXLAT` never emits a UDC code, so a stray `-`
or `:` in a server string cannot render as half a die.

## Watching a bot score

The wire has no "last move" field: every endpoint returns the same state
blob, and nothing in it says which row a player just took. So when
another seat finishes a turn the client works it out by comparing their
filled rows against what they were last poll, then shows the answer --
their card, the row they took held in magenta, a falling two-tone cue
where ours rise, and about a second and a half to read it before play
moves on.

A full shadow of sixteen 16-bit scores is 32 bytes and this machine has
84 in total, so the shadow is **one bit per slot**: `PRVSCM`, two bytes,
restaged at the bottom of every poll for whoever is up. `SCSCAN` walks
the seat's score words once, building the new mask and reporting the
lowest slot that is filled now and was not filled then.

Three details make it right rather than nearly right:

* **Slots 6, 7 and 15 can never be the answer.** The upper total, the
  bonus and the grand total fill as a side effect of the row actually
  chosen -- slot 6 the moment *any* upper row is taken -- so without
  excluding them slot 6 would win every time, being the lowest.
* **The highlight is a colour, not an animation.** `SCELL` paints any
  cell matching `REVSLT` in `AHOT`, so the reveal is just "point the
  view at them, redraw, wait", and `REVSLT` returning to `$FF` disarms
  it. That is a quarter of the code a blink loop cost.
* **A full clear parks the shadow at `$FFFF`.** After `GSTATIC` -- a
  class change, or a seat count change where the shadow would belong to
  a different player -- no bit can have turned on, so nothing is
  revealed from a comparison that was never valid.

`REVEAL` runs from `GAMELP` rather than inside `GEDGE`, for the return
stack: `CARDRW` is five frames deep on its own and a call from `GEDGE`
would have put the chain at seven of the 2650's eight. It therefore
takes the turn edge itself, which is fine -- `GEDGE` has not advanced
`PRVACT` yet.

## Controls

```
disc up/down     the score rows; up from a die crosses to them,
                 down from a die drops to the ROLL tile
disc left/right  the dice; the other card column in SCORE mode;
                 page the viewed scorecard while you wait
FIRE / 2 / Enter select: ready up, hold a die, roll, take a row
Clear            leave the table
keypad 0         poll now
```

FIRE and keypad `2` are electrically one wire, so that bit is always
"select". On your turn the cursor starts on the ROLL tile, because
rolling is always the first move; a trigger rolls everything. After the
last roll the cursor jumps to the highest-paying open row, so
trigger-trigger-trigger-trigger plays a legal (if artless) game.

## Code layout

Block 1 (CPU `$0000-$0FFF`, 2650 page 0) holds everything that touches
the screen -- page-0 RAM and the UVI live there. Block 2
(`$2000-$2AFF`, page 1) shares its page with the mailbox, so a register
arm or a TX append is a direct indexed load. Each side reaches the
other's RAM through `DWBE` pointer words.

Block 1 is the tight one, so anything screen-independent has been pushed
across: the string literals, the ASCII and keypad translate tables, the
die-face map, `sound.inc` -- **and both 13-line screens**, `nament.inc`
and `lobby.inc`. That last move is what made the port fit, and it is
nearly free: an indirect operand carries the full 15 bits and costs the
same three bytes as the absolute form it replaces, so the whole price is
the pointer words at the foot of `mailbox.inc`. The two literals that
cannot move are `DEFNAM` and `WHEEL`, both read with direct indexed
loads.

`net.inc` carries the family's five hard-won rules verbatim (CLOSE opens
the *next* request; capture RXLEN immediately after READ; STATUS settles
on two agreeing readings; STATUS byte 3 must be 1; validate the length
`playerCount` implies before believing a byte). `MCOMMIT` derives every
sequence number from the cart's persisted `ACKSEQ`, never a local
counter, because a console RESET restarts this program and not the cart.

`GMAXLEN` is 599 and spans three of the cart's four 256-byte reply
slices, so `state.inc` needs the 16-bit `RXSEEK`/`PLSEEK` rather than a
flat 8-bit cursor.

RAM is the whole design constraint: **84 bytes** in 26-line mode (zone A
`$18D0-$18EF`, the four bytes at `$18F8-$18FB`, zone B `$1AD0-$1AFF`),
and every byte is spoken for -- the ledger is in `fujitzee.asm`,
including five deliberate aliases, each a pair of cells whose lifetimes
cannot overlap. The bot reveal's two-byte shadow is only there because
`RUNTOT` was changed to accumulate straight into `V_NUM`, where `NUMFLD`
already reads, which retired a scratch pair and a copy at each of its
two call sites. Nothing is buffered: names and the
server prompt render straight out of the reply window (`RXPRNT`),
numbers straight into cells (`NUMFLD`), and the only copies kept are the
two names that ride out in a URL.

## Two things that cost a day

**R2 is both the screen cursor and every seek routine's field argument.**
`RUNTOT` and `RDSCOR` clobber it, so every caller reads the wire FIRST
and positions afterwards. Getting that backwards is what put the whole
standings panel's totals on the dice row.

**On this CPU every load sets the condition code -- `LODI` and `STRZ`
included** (MAME implements `STRZ,n` as `M_LOD`). So the Z80 habit of
comparing, moving a value into place, and then branching is silently
wrong here:

```asm
        COMI,R0 8
        LODI,R2 8       ; <- sets CC; the compare above is now lost
        BCTR,LT SKD0    ; <- tests the LODI, and never branches
```

Three sites had it, transcribed from the astrocade port where `LD`
leaves the flags alone: the two column-limit selects in `SKPDN`/`SKPUP`
and the column jump in `TISCOL`. Branch on a compare BEFORE anything
else touches CC, and if a value has to be fetched first, fetch it and
re-test explicitly. The inverse of the same trap is a branch sense read
backwards -- `BCFR,EQ` on a freshly loaded flag branches when it is
NONZERO -- which is what kept the client out of `TURNIN` entirely and
left the server force-scoring every turn.

## The Intellivision lessons carried over

* **The turn edge.** `activePlayer` stays 0 for every poll of your turn,
  so `PRVACT` advances at the bottom of every successful poll -- lobby
  included -- and the your-turn cue fires BEFORE the input slice.
* **The roll edge is a decrease.** A fresh turn resets `rollsLeft`
  upward, so only a decrease means a roll landed; that is what fires the
  animation, and the `$FF` sentinel in `PRVRLS` mutes it across every
  screen boundary.
* **The ready toggle lives in the input path**, sampled every frame in
  `GLWAIT` and edge-detected, never in the once-per-poll renderer.
* **Validate before believing.** The reply window is never cleared, so
  `VALID8` gates on the header length first, then `playerCount <= 12`,
  the round on its enum (0-13 or 99), and `95 + 42*playerCount` bytes.
  And `validScores[]` is all zeros in the lobby -- only play-class code
  may read it.
* **The clock bails.** The turn input counts down locally from the
  wire's `moveTime` and returns at zero; the server has already
  force-scored, and only the next poll learns where the game went.
* **A rejected press earns only a short slice.** An accepted key
  restarts the full ~4 s slice, so a player still choosing is never
  interrupted; a trigger bounced off a dead ROLL tile grants ~1 s, or
  mashed rejects could starve the poll loop forever.
* **Rejoins land anywhere.** The server holds your seat, so your turn
  can arrive mid-turn with the rolls already spent: the my-turn edge
  restages the wire's keep mask when dice exist and drops straight into
  the score rows when `rollsLeft` is 0.

## Building and running

```sh
./build.sh                  # build/fujitzee.bin, exactly 8192 bytes
./run.sh                    # MAME arcadia with the fujinet cart device
make assets                 # regenerate assets/udc.inc from the ASCII art
```

Prerequisites: Macroassembler AS (`asl`/`p2bin` on `PATH`, or `~/asl`,
else the Windows binaries under wine), a MAME tree with the cart device
grafted in (`fujinet-firmware/pico/arcadia/emu/apply.sh`), and a
`fujinet-pc` BoIP listener.

Environment: `ENDPOINT` (default `https://fujitzee.carr-designs.com/`,
regenerated into `build/endpoint.inc` on every build so a stale value
cannot survive), `FUJI_FIRMWARE`, `MAME_DIR`, `FUJINET_TCP` (default
`127.0.0.1:9995`), `FUJINET_DEBUG=1` to log every mailbox transaction.

Four static gates run on **every** build and are not optional:
`mkimage.py` rejects anything in the `$1000-$1FFF` console-RAM hole or
the `$2B00+` mailbox pages; `checkrom.py` enforces the 8192-byte layout,
the `$1F`/`$17` header and the `FUJI` claim at `0x1CFC` (without it the
mailbox goes dead the moment the image boots); `checkdepth.py` bounds
the call graph to the 2650's 8-entry return stack; `checksize.py`
reports the per-module budgets.

`checkdepth.py` under-counts through label-dense bodies -- it reports 5
here, starting inside `CARDRW`. The hand-audited worst chain is 6 of the
8 RAS entries:

    GAMELP > RENDER > CARDRW > SCELL > RDSCOR > RXNEXT > MSLICE

Current budgets: block 1 3918 of 4096, block 2 2634 of 2816. Block 1 is
the one to watch -- `checksize.py` starts grumbling below 200 bytes
spare, and the next thing to push across is whichever screen-independent
routine is largest at the time.

Against a local server:

```sh
(cd <servers>/fujinet-game-system/fujitzee/server && go run .) &
ENDPOINT=http://127.0.0.1:8080/ ./build.sh
FUJINET_DEBUG=1 ./run.sh
```

A `/state` for an N-seat game reads back exactly `95 + 42*N` bytes --
that one number confirms the wire format, `VALID8` and the slice walk at
the same time.

## Not here

No `DEMO=1` mock screen and no MAME Lua harnesses: this port is verified
by the static gates above plus manual play. If it grows a maintenance
tail, `emu/smoke.lua` from the sibling ports is the thing to add back,
and it exists to avoid two specific traps -- MAME's
`machine.time.seconds` returns the integer second, so a script written
in fractional seconds collapses a press and its release into one frame;
and `/ready` toggles against a wall-clock countdown, so a script that
keeps pressing FIRE in the lobby cancels the start every time.

# Fujitzee for the Fairchild Channel F

A 3850 assembly client for the [Fujitzee server](https://fujitzee.carr-designs.com/),
running over the FujiNet cartridge mailbox from the Channel F bring-up at
`fujinet-firmware/pico/channelf`. The game logic is the Intellivision
client's -- the freshest one written directly against the binary wire format --
by way of the ColecoVision and Arcadia ports; the hardware half is
`fujinet-battleship/channelf`'s, copied rather than rewritten.

## Why this is the easy console and the hard screen

Astrocade, Arcadia and ColecoVision all push both mailbox directions through
the READ path, because none of those cartridge edges carries a write strobe.
That is why the Arcadia Fujitzee client lives in **84 bytes of RAM** and walks
its reply in 256-byte slices. A Channel F cart is not a ROM; it is a peer on
the F8 bus that services `ROMC 05` stores. So the cart hands the console 16K of
ROM at `$0800`, **30K of read/write RAM at `$8000`**, and the whole 599-byte
reply flat at `$F800`. Nothing here is rationed and there is no slice walk at
all.

What is scarce is pixels. VRAM is 128x64 at 2bpp, **write-only** through I/O
ports, and one plotted pixel costs about 80us. Over the ~95x58 safe area a 4x6
cell gives **23 columns by 9 ROWS** -- tighter than the Intellivision's 20x12
and far tighter than the Arcadia's 16x26, both of which had room to spread this
game out. The Intellivision needed a seat strip and a standings overlay to fit
15 categories into 20 columns; this screen has three fewer rows than that.

## The scorecard is the wire's own order

Sixteen slots, four columns of five cells -- a two-character label and a
three-character value -- with a one-cell gap between them. That is
`4*5 + 3 = 23`, the screen exactly:

```
1S 2S 3S 4S        slots  0  1  2  3    ones .. fours
5S 6S UP BN        slots  4  5  6  7    fives, sixes, upper total, bonus
3K 4K FH SR        slots  8  9 10 11    3-kind, 4-kind, house, small run
LR CH FZ TT        slots 12 13 14 15    large run, chance, fujitzee, total
```

Laid out that way the grid **is the wire's own order, read across**, so both
coordinates are arithmetic rather than a lookup: the row is `(slot>>2)+1` and
the label column is `(slot&3)*6`, which is also how the cursor finds its cell.
The two-character labels are the price of the fourth column, and they are
affordable because the row ORDER is the conventional one -- a player who knows
the game reads the grid, not the abbreviations.

Three of the sixteen are never selectable: the upper total, the bonus and the
grand total are filled by the server as a side effect of the row actually
chosen, so the cursor steps straight over them. Stepping again in the SAME
direction is what makes that work in every case -- right from slot 5 walks 6, 7
and wraps to 4, and down from 11 walks 15 and wraps to 3. The grand total is
computed HERE, by summing slots 6-14, because the server only fills the wire's
own slot 15 at game over.

## The screen

```
row 0     BOB        R6/13  250     name, round, move clock
rows 1-4  the scorecard, 4 cols x 4 rows
rows 5-7  five dice, 16 x 18 px each
row 8     ROLL X2      YOUR TURN    roll tile + the server's prompt
```

The palette is **banded per scanline** (columns 125 and 126 of each row pick
it), which is this machine's one real trick:

| rows | palette | role |
|---|---|---|
| 0-9, 52-63 | 0 | BLACK ground, **WHITE** ink -- header and footer |
| 10-51 | 2 | LTGRAY ground; BLUE / RED / GREEN -- the card and the dice |

Palette 0 is the only place this machine HAS white, and it has exactly one ink:
values 1, 2 and 3 all show white. So the header and the footer carry **no
colour distinctions at all** and their text has to say what it means. That is
also why the cursor on the ROLL tile is a caret in the gap row beneath it
rather than a colour, and why the standings overlay marks the row you are
looking at with the cursor gutter instead of an ink.

The card band is where the colour lives, and value 0 is the LTGRAY ground
itself:

| value | ink | used for |
|---|---|---|
| `CVAL0` | ltgray = the ground | a closed row: it reads as **absent** |
| `CVAL1` | blue | labels and filled scores |
| `CVAL2` | red | potential scores, a held die, a row a bot just took |
| `CVAL3` | green | whatever the cursor is on |

A closed row painted in the ground colour is **invisible rather than dim** --
the Intellivision's `COL_DASH` trick on a machine with no black to spend. The
corollary is the rule the Arcadia port learned twice: **nothing that must be
read may be drawn in `CVAL0`**. The cursor therefore overrides the closed
colour, or it would vanish the moment it stepped onto a shut row.

## The dice, and why there is no art file

A die is a solid body in its state colour with the pips **punched through** to
the ground -- the Arcadia port's inversion, which is also simply what a die
looks like. Sixteen pixels wide on a nineteen-pixel pitch is 95 across for five
of them, the safe width exactly.

The Arcadia port pre-composited its faces into UDCs and battleship
pre-composites its reticle into a second bank of cells, both because their
blitters carry the colour IN the image. `DFILL` takes a colour argument, so
here a face is **nine bits** -- one per pip position in a 3x3 grid -- and the
six of them are **twelve bytes of table**. The three state colours then cost
nothing, where a pre-composited set would have been six faces times three banks
of 72-byte cells. There is no `mkart.py` in this port and `fujidisp.inc`'s
`DCELL` goes unused.

```
columns at x+1, x+6, x+11   rows at y+1, y+7, y+13   each pip 4x4
```

A die's colour is its state: **blue** ordinary, **red** held, **green** under
the cursor. The cursor overrides the hold, which is why `DIEHLD` has to be
independent of it. On your own turn `DIEHLD` reads the **locally staged** mask,
not the wire: the wire's `keepRoll` is an echo of what was last actually
re-rolled and does not move until the next roll goes out, so reading it to
decide what to draw made a die you had just held look exactly like one you had
not. For every other seat the echo is all there is -- and it is the right
thing, because it is what they kept.

## Watching a bot score

The wire has no "last move" field: every endpoint returns the same blob and
nothing in it says which row a player just filled. So the client works it out,
by walking that seat's sixteen score words into a one-bit-per-slot mask and
diffing it against the mask stored last time -- then points the card at them
and holds their row in red for about a second and a half.

**The shadow is per player**, so it is STAGED for whoever is up at the bottom
of every poll and DIFFED only on the turn edge. While seat P is up, every poll
stores P's mask; when the turn moves on, P's card differs from that mask by
exactly the row they took. An earlier version restaged it only when something
had happened, which meant comparing two different players' cards.

**Slots 6, 7 and 15 can never be the answer**: slot 6 fills the moment any
upper row is taken, so a lowest-set-bit search that does not exclude them
returns 6 every single time.

When the hold expires the view is handed back to whoever is up now. The reveal
borrowed it, and leaving it there meant a player spent their own turn looking
at a bot's card.

## Controls

```
stick up/down/left/right  walk the 4x4 card; left/right walks the dice;
                          up from the dice crosses to the card, down from the
                          card's bottom row drops back to the ROLL tile;
                          left/right pages the viewed card while you wait
plunger (push down)       select: ready up, hold a die, roll, take a row
HOLD                      the standings overlay
MODE                      leave the table
TIME                      poll now
```

On your turn the cursor starts on the ROLL tile, because rolling is always the
first move. After the last roll it crosses to the card by itself and parks on
the highest-paying open row, so trigger-trigger-trigger-trigger plays a legal
if artless game.

## Code layout

```
fujitzee.asm  the RAM ledger, the cart header, ENTRY, MAIN/GOTO, the MB_ fences
fujidisp.inc  DPALR DPLOT DSTR DHEXS DCLRR DCELL DFILL DELAYMS   (copied)
ui.inc        the 23x9 furniture: title, seven list rows, footer  (copied)
strings.inc   the row builder and DEC5                            (copied)
sound.inc     the console's three notes                     (copied, trimmed)
input.inc     controllers and console buttons, edge-triggered     (copied)
fujilib.inc   the mailbox: FNCHK FNBEG FNGO FNPB FNFIX FNACK ...  (copied)
net.inc       the N: round trip and its five rules                (copied)
fujinet.inc   the wire format, as EQUs
url.inc       BLDURL, streamed straight into the TX page
card.inc      the 4x4 scorecard, the running total, the bot-score shadow
dice.inc      the dice band and the roll animation
nament.inc    the username and room appkeys, and the caret keyboard
lobby.inc     /tables
turn.inc      the bounded turn-input slice
stand.inc     the standings overlay
game.inc      the poll loop, VALID8, the edges, the renderer
strdata.inc   every literal that reaches the screen
```

Everything marked "copied" comes from `fujinet-battleship/channelf` unchanged,
which is where to send a fix. Battleship's is the better of the two shipped
Channel F clients at every point of difference -- its `fujidisp.inc` has
`DPALR` (a palette over a row RANGE) and an opaque `DCELL` where 5 Card Stud
has `DPAL` and a 1bpp `DSTAMP`, and its `net.inc` has the settle delay at
`LI 11` rather than `LI 100`, which matters because a `DELAYMS` unit is about
9 ms and 100 asks for the best part of a second per reading.

Current budget: **9,211 of 16,380 bytes**, so there is room. The 16K is not a
target to fill but a requirement: the image must be exactly that, with the
`"FUJI"` claim at `$47FC`, or the cartridge boots it with the mailbox dead and
the arena at `$8000` reverts to open bus.

## The invariants carried from the earlier consoles

1. **The turn edge.** `activePlayer` stays 0 for EVERY poll of your turn, so
   the shadows advance at the bottom of every successful poll -- lobby included
   -- and the your-turn cue fires BEFORE the input slice.
2. **The roll edge is a DECREASE.** A fresh turn resets `rollsLeft` upward.
   `$FF` in `VPRVRLS` mutes the animation across a screen boundary.
3. **The ready toggle lives in the input path**, sampled every pass and
   edge-detected. A toggle read once per poll feels dead.
4. **`validScores[]` is all zeros in the lobby** -- a nil slice, not a row of
   -1 -- so only play-class code may read it, or an open board paints as a wall
   of zeros.
5. **The clock bails at zero.** Count down locally and return; the server has
   already force-scored and only the next poll learns where the game went.
6. **Rejoins land anywhere.** Your turn can arrive with the rolls already
   spent, so the my-turn edge restages the wire's mask when dice exist and
   drops straight into the score rows when `rollsLeft` is 0. This is not a
   corner case -- it is what every seat taken at a table already in progress
   looks like.
7. **A staged action rides the next request** in place of `/state`, and its
   reply IS the next state. There is never a second round trip -- which is also
   why `VPEND` holding 0 must mean "nothing staged" and not `RQTABLE`. Request
   0 is the table list, and a cleared cell read as a request id refetches it
   forever.
8. **Validate before believing.** The reply window is never cleared, so a short
   read leaves the tail of a previous, bigger reply exactly where the fixed
   offsets are about to look. A `/state` for an N-seat game reads back exactly
   `95 + 42*N` bytes -- one number that confirms the wire format and `VALID8`
   together.
9. **A rejected press earns only a short slice.** An accepted one restarts the
   full four seconds, so a player still choosing is never interrupted; a
   trigger bounced off a dead ROLL tile grants about a second, or mashed
   rejects could starve the poll loop.
10. **A full clear only on a class edge.** A clear is most of a second of
    plotted pixels; per poll it would blank the screen every cycle. Everything
    else updates cells in place, and a cursor move repaints the two cells that
    changed.
11. **Something has to listen during the blind window.** Input is read only
    inside an input slice, and a poll plus the repaint after it is about a
    second of the four-second cycle with nothing listening at all -- so a press
    that begins AND ends inside it is never seen. This is not theoretical: it
    swallowed two deliberate button presses during testing before it was
    noticed, and from the player's seat a button that does nothing is a broken
    button. `INLATCH` therefore samples at the seams of the render and at both
    ends of the fetch and remembers ONE press; `INDRAIN` hands it to the slice
    before the slice scans. The first press wins -- a latch that overwrote
    itself would deliver whatever the player did last, not what they asked for
    first.

## F8 traps this port paid for

**Compare by ADDING THE ONE'S COMPLEMENT, not by subtracting the two's.**
`a + ~b` carries out exactly when `a > b`, and it is right for every pair. The
obvious version -- negate and add, then read the borrow -- is not: negating
zero gives zero, the add then produces no carry, and the comparison comes out
backwards for precisely the operand that matters. It cost three separate bugs
here, all of them silent:

* the roll edge never fired, because `rollsLeft` reaching **0** IS the edge it
  was looking for -- so the cursor never crossed to the card and sat on a dead
  ROLL tile refusing every trigger until the server force-scored;
* `TIBEST` picked nothing at all, because it seeds "best" at **0** and every
  candidate compared against the seed read as no better -- leaving the cursor
  wherever it started, usually on a closed row;
* an **empty** table list drew a row out of whatever the previous reply left
  behind, because the count it was bounded by was 0.

The rest are the family's:

* **`PI` and `JMP` clobber A** (`m_a = m_dbus`). Nothing returns in A across a
  call; `LEAVE` carries it by hand through r10.
* **Draw routines write r0 and r2-r8.** Loop counters, arguments and error
  codes live in RAM. A data counter, by contrast, DOES survive a call.
* **`ENTER`/`LEAVE` give nine levels** on the BIOS K stack at r40-r58, and
  `GOTO` must rewind r59 to 40 -- every state is entered by `JMP` and left
  through `GOTO`, so the `ENTER` at the top of a state never meets its `LEAVE`.
* **Relative branches reach +-127**, so several loops need a `JMP` trampoline.
* **`DELAYMS` units are about 9 ms, not 1.**
* **`CI n` computes `n - A`**, so `BP` means `A <= n` and `BM` means `A > n`.
  Reading those backwards is an off-by-one that only shows up at the boundary.

## Building and running

```sh
./build.sh                  # build/fujitzee.bin, exactly 16384 bytes
./run.sh                    # MAME channelf with the fujinet cart device
make assets                 # regenerate src/font.inc
```

Prerequisites: Macroassembler AS (`asl`/`p2bin`, default `~/asl`), a MAME tree
with the cart device grafted in (`fujinet-firmware/pico/channelf/emu/apply.sh`),
and a `fujinet-pc` BoIP listener.

Environment: `ENDPOINT` (default `https://fujitzee.carr-designs.com/`,
regenerated into `src/endpoint.inc` on every build so a stale value cannot
survive), `MAME_DIR`, `FUJINET_TCP` (default `127.0.0.1:9995`).

`-l 0xff` fills the gaps, matching what MAME's stock Videocart answers past its
ROM. `p2bin`'s `-f` FILTERS records out -- silently producing an empty image --
and `-s` appends a checksum byte; neither is wanted. `tools/checkrom.py
--claim` then enforces the 16384 bytes, the `$55` at `$0800` and the `"FUJI"`
at `$3FFC`, and `tools/checksize.py` reports the per-module budgets off the
`MB_*` fences.

`run.sh` is **throttled**, and that is not a performance choice: the server's
start countdown and its move clock are wall clock, so a run at 250x emulated
speed sits at "starting in 2" forever because barely any real time passes.

Against a local server:

```sh
(cd <servers>/fujinet-game-system/fujitzee/server && go run .) &
ENDPOINT=http://127.0.0.1:8080/ ./build.sh
./run.sh
```

## Not here

No `emu/` Lua harnesses and no `DEMO=1` mock screen -- the Arcadia port made
the same call. This one is verified by the static gates above plus play against
the live server. If it grows a maintenance tail, `emu/smoke.lua` from
`fujinet-battleship/channelf` is the thing to add back, and it exists to avoid
two specific traps: MAME's `machine.time.seconds` returns the INTEGER second,
so a script written in fractional seconds collapses a press and its release
into one frame and the key is never seen; and a state variable changes BEFORE
the screen it belongs to is drawn, and a screen here is one to two seconds of
plotted pixels, so every step has to wait for the state to be STABLE rather
than merely present.

The lobby appkey is read for a username and for a room handoff, and written
back on both. A name accepted as the default is not written -- only a typed one
is, which is the same behaviour as the battleship port.

## Known gaps

* **No paging at game over.** The stick pages the viewed scorecard while you
  wait during PLAY, but the game-over screen shows one card and the standings
  overlay. Every seat's final total is one button away, so this has not been
  worth the state it would add.
* **The ROLL tile reads `ROLL X0` when the rolls are spent** rather than
  disappearing. The footer runs palette 0 and has no dim ink to grey it out
  with, and a tile that says why a trigger is being refused seemed better than
  one that vanishes.
* **Opening and closing the standings each cost a full screen clear**, about
  0.7 s apiece. Repainting only the 23x9 grid would be faster, but the dice
  band and the gaps between the card's columns are not covered by the routines
  that would do it, and leftovers from the overlay would show through.
* **A staged action is consumed even if the request fails.** The server
  force-scores on its own clock, so a dropped move costs a turn rather than
  wedging the client; the sibling ports behave the same way.
* **The press latch does not cover the middle of a network wait.** `INLATCH`
  runs at the render's seams and at both ends of the fetch, but `NFETCH` itself
  is one synchronous call and nothing samples inside it. A press made entirely
  during the STATUS settle can still be missed. Covering that means sampling
  inside `net.inc`, which is shared with the battleship port verbatim and is
  not worth forking for it.

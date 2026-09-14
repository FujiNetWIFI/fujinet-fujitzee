# Fujitzee for the ColecoVision

A Z80 assembly client for the [Fujitzee server](https://fujitzee.carr-designs.com/),
running over the FujiNet cartridge mailbox from the ColecoVision bring-up at
`fujinet-firmware/pico/coleco`. Fourth in the standalone cart-client family after
the Intellivision, the Astrocade and the Arcadia 2001; the game logic is the
astrocade port's, itself transcribed from the Intellivision client, and the
platform half is new — TMS9918 **Graphics II** driven as a 32x24 character map
through **OS7**, the console's own BIOS.

Graphics come from the **MS-DOS port's charset**, including its NAMCO arcade
font, converted to Graphics II pattern/colour pairs by `tools/mkcharset.py`.

## Why standalone, and why assembly

The shared mekkogx C core cannot run here. Its static footprint alone is about
2.1 KB — `Game` 599, `GameState` ~337, `url`/`tempBuffer`/`serverEndpoint` 338,
plus a 768-byte name-table shadow — against **1024 bytes of RAM in total**. So
this port does what the rest of the family does: **nothing is buffered.** The
cartridge paints the whole 1K reply at `$F800` as directly addressable ROM,
`/state` maxes out at `95 + 42*12 = 599` bytes, and names, prompts, scores and
dice all render straight out of the window.

Once the shared core is out, C buys nothing, and `astrocade/` is already this
game in Z80.

## The screen

Graphics II gives a foreground and a background colour per 8-pixel row of each
**pattern** — not per cell. Two cells showing the same code in the same screen
third are always the same colour. So the font is uploaded three times, at three
code ranges, in three inks, and drawing coloured text is picking a base:

| codes | | |
|---|---|---|
| `$00-$59` | 90 art tiles | dice in four colour banks, the ROLL tile, the fujiTZEE logo, chrome, icons |
| `$70-$AF` | font, **white** on dark blue | `code = ascii + $50` |
| `$B0-$EF` | font, **cyan** — the accent | `code = ascii + $90` |
| `$F0-$FF` | digits, **light green** | `code = ascii + $C0` |

All three screen thirds get identical uploads, so a code means the same thing
everywhere and nothing has to know which third it is drawing into.

The pay-off is the VRAM budget: **drawing a cell is one name-table byte**, and
nothing touches the colour table after `DINIT`. A score cell is 3 bytes, the
timer 2, a die 9, the prompt one row of 32. Only a wholesale repaint is large,
and those blank the display first (`DHIDE`/`DSHOW`). The NMI handler writes no
VRAM at all.

Colours: **dark blue** table, **white** text and dice bodies, **cyan** accent
(labels, prompts, a bot's just-taken score), **light green** potential scores,
**yellow** held dice, gold chrome.

### Layout (32 x 24)

```
row  0     the server's prompt, and "R n/13" in the last six cells
row  1     seat initials; the turn mark on the active seat's column
row  2     thick rule
rows 3-19  the scorecard. Labels at cols 2-6, six seat columns at
           8, 12, 16, 20, 24, 28 -- three cells each, right-aligned.
           Slot 14 is labelled by the fujiTZEE LOGO, as on the MS-DOS board
row 20     thick rule
rows 21-23 the clock at col 5, the ROLL tile at 8, five dice at 12/16/20/24/28
```

Both geometries are arithmetic, not tables: seat *i*'s column is
`SCORESX + 6 + i*4`, slot *j*'s row is `SCOREY[j]`.

The prompt is on row 0 rather than the bottom because 24 rows leave no spare
line under a three-row dice panel — the MS-DOS board only has one because it is
25 tall.

## Watching a bot score

The wire has no "last move" field: every endpoint returns the same blob and
nothing in it says which row a seat just took. So the client works it out by
comparing each seat's filled rows against what they were last poll, then points
at the answer — the row in the accent, a falling two-tone cue where your own
rises, and about a second to read it before play moves on.

A full shadow of six seats × sixteen 16-bit scores is 192 bytes; **one bit per
slot is twelve**. Three details make it right rather than nearly right:

* **Slots 6, 7 and 15 can never be the answer.** The upper total, the bonus and
  the grand total fill as a side effect of the row actually chosen — slot 6 the
  moment *any* upper row is taken — so without excluding them slot 6 would win
  every time, being the lowest.
* **Only the seat that just finished a turn is looked at.** This client can lag
  several polls behind, and animating every score that appeared since would be a
  slideshow.
* **A full clear parks the shadow at `$FFFF`.** After a class change or a seat
  count change the shadow may belong to a different player, so no bit can
  legitimately have turned on and nothing may be revealed from a comparison that
  was never valid.

Your own rows are excluded: a row you chose needs no reveal.

## Holding dice

Held dice are **yellow**, with the MS-DOS clamp art, and they change *the
moment you press the trigger* -- not when the next roll lands.

That distinction is the whole point: the wire's `keepRoll` is an echo of what
was last re-rolled, so it only moves when a roll goes out. Reading it to decide
what to draw meant a die you had just held looked exactly like one you had not,
right up until you rolled -- so the one action whose feedback matters most had
none. `DIEKEEP` reads the locally staged mask on your own turn and falls back
to the wire's echo for everybody else's, where there is no local intent to show
and the echo is the right answer.

The rolls left are spelled out in the bottom-left corner, five columns by three
rows that nothing else uses. The ROLL tile already encodes the same number as a
glyph, but only on your own turn and only if you know to read it.

## Potential scores

`validScores[15]` carries the exact points every open row would earn for the
dice on the table — `-1` closed, `0` open but scoreless, `>0` the score. Drawn
right-aligned in the active seat's column, in the **light green** bank, once the
roll animation stops. A potential of exactly **0 is hidden except on the row
under the cursor**, so an open board is not a wall of zeros before you have
decided; the cursor **skips closed rows**; and on select every *other* potential
is wiped before the request goes out.

`validScores[]` is all zeros in the lobby. Only rounds 1-13 may read it.

## The clock

Seeded from the wire's `moveTime` on every entry to the turn loop and counted in
**vblanks** — this console has a real 60 Hz interrupt, so a second is exactly
sixty frames and the clock cannot drift against the server's. It appears only in
the last fifteen seconds, repaints only when the whole-second value changes,
ticks audibly as it goes, and is wiped the moment you roll.

At zero it **bails** rather than waiting: the server has already force-scored,
and only the next poll can discover where the game went.

## Controls

```
joystick         the score rows and the dice; up from a die crosses to the
                 scorecard, down from the bottom drops back to the ROLL tile
either FIRE      select: ready up, hold a die, roll, take a row
keypad *         back / leave the table / backspace in the name editor
keypad #         the in-game menu / accept in the name editor
keypad 0         poll now
keypad 1-9       jump to a table in the list
```

**Cancel is `*` and nothing else** — backspace must never discard an edit,
because reaching for it repeatedly is exactly when a reflexive extra press
happens. On your turn the cursor starts on the ROLL tile, and after the last
roll it jumps to the highest-paying open row, so trigger-trigger-trigger-trigger
plays a legal if artless game.

There is no keyboard, so names are typed off an on-screen grid (`nament.inc`).

## Code and RAM

`fujitzee.asm` carries the cartridge header, the RAM ledger and the module
includes; `tools/checksize.py` prints the per-module budget on every build.

RAM is 1K at `$6000`, mirrored every 1K through `$7FFF`. This client uses the
`$7000` mirror, OS7's own convention, and **never `$7800-$7FFF`**: A15 is not on
the cartridge connector, so a RAM access there puts the same bits on A0-A14 as a
cartridge read of `$FC00-$FFFF` and reads as a hotspot touch. OS7 keeps
`$73B8-$73FF`; the stack tops out below it. About 160 bytes of headroom sit in
the sprite tables the header has to declare and this client never uses.

## Interrupts, and the one rule

Unlike every sibling port, **this client runs with interrupts on**. The
ColecoVision's vblank is wired to `/NMI`: `DI` will not stop it, so the astrocade
trick of running with interrupts off for the program's whole life is not
available — and it is not wanted, because POLLER, the frame counter and the
timer manager all ride it.

The NMI *will* land in the middle of mailbox transactions. That is harmless
because of the one rule the handler keeps: **it never reads `$F800` or above.**
An armed register waits, and the TX stream is append-only, so an interrupted
transaction simply resumes. `tools/checkrom.py` enforces the image half of the
same rule.

## Things that cost real time

* **`SCOREY[15]` is row 21, which during play is the dice panel's top row.**
  Drawing the grand total there on every poll blanked the top third of the ROLL
  tile and all five dice, and `DICEHUD` painted them straight back a moment
  later: a flicker once per poll, right where the eye is. The shared engine
  stops at slot 15 until the game is over for exactly this reason
  (`gamelogic.c:278`), and by then the dice are gone.
* **Reveal only after the first full paint.** `CHKNEW` blanks the screen and
  redraws the statics on a class change; turning the display back on *there*
  showed a board with no dice, no scores and no prompt for the third of a
  second it took to fill them in. The display now stays blanked until `DSHOWN`,
  after the first complete render. Same rule for the menu, the name editor and
  the table list: blank, draw, then reveal.
* **Write each cell once.** `DCURSOR` used to clear the whole cursor-bar row
  and then draw the bar, leaving a gap the VDP was free to display in between.
  It is one pass now.
* **The MS-DOS kept-die tiles carry a drop shadow; recoloured, it reads as
  damage.** Those tiles (`0x20-0x2B`) are the normal die shapes with a flat
  clamped top and a two-pixel BLACK shadow along the die's outer edge, which
  that port draws for depth over its own background. Remapped to yellow the
  shadow stops looking like a shadow and starts looking like a chipped corner
  -- a die that appears corrupted without anything having gone wrong. The held
  die is built from the NORMAL tiles instead, recoloured, with the clamp bars
  laid over its top and bottom edges: same rounded corners as the white dice,
  same pips, no shadow.
* **The "cannot roll" tile was a chrome cross.** The MS-DOS dice table puts
  glyph `0x50` -- a thick grid cross -- in the ROLL tile's count cell when no
  rolls remain. Converted here it becomes a cyan-and-black blot sitting inside
  a white tile, which reads as corruption rather than as a state. The count is
  simply blank now, and the ROLLS counter beside it says the same thing in
  words.
* **The VDP address setup is not atomic against the NMI.** An address is two
  bytes clocked through a flip-flop inside the VDP, and reading the status
  register -- the one thing the vblank handler must do, because it is what
  clears the interrupt -- also RESETS that flip-flop. An NMI landing between
  the two bytes leaves the second taken as a first and the write lands at an
  address it was never meant for: one cell, anywhere on screen, holding a code
  meant for another. Every VDP access now goes through `VWRITE`/`VFILL`/`VWREG`,
  which set `VDPBUSY`; the handler defers its status read while that is set and
  the wrapper makes the read as soon as the sequence finishes. Nothing is lost,
  because the VDP holds its interrupt line asserted until that read happens and
  the NMI is edge-triggered.

  Honestly: this one is reasoned, not reproduced. MAME does model the flip-flop
  (`tms9928a_device::register_read` clears `m_latch`), and a canary watching 158
  static cells over 5,000 frames caught nothing even with the guard removed --
  so if a corrupted rule was something else, this did not fix it. The guard is
  correct and costs a byte of RAM and a compare, so it stays either way.

  It also has to be TRANSPARENT to every register. The first version set the
  busy flag with `LD A,1`, and `FILL_VRAM` takes its fill value in A -- so
  every fill painted VRAM with the flag instead of the colour, and the charset
  came out entirely in ink 1. A watcher on the three font colour banks caught
  72 corruptions in 8,377 frames; with `VDPLOCK` and `VDPFREE` preserving AF it
  catches none. Guards that clobber what they guard are worse than no guard.
* **A row index is arithmetic, so an out-of-range one is a valid address.**
  `DVADDR` turns row 79 into an address inside the COLOUR table quite happily,
  which is how a slot index that walked off the end of `SCOREY` -- into the
  label strings, where 'O' is 79 -- would repaint the charset's colours instead
  of drawing a cell. `DPUTC` refuses any row at or past `HEIGHT`, and the two
  render loops test less-than against their limit rather than equality, so an
  overshoot ends the loop instead of running forever.
* **The fujiTZEE logo is six MS-DOS tiles and the label column is five cells.**
  Slot 14 is labelled by the logo itself, so drawn as-is it poked into the
  first player's column divider. The sixth tile is that port's score-box left
  border -- `drawFujitzee()` blanks it away before use -- and stripping it
  leaves exactly forty inked columns, so the image is shifted flush left and
  repacked into five tiles with nothing lost. The generator publishes
  `LOGOCNT` so the two draw sites cannot drift from the asset.
* **The MS-DOS charset keeps ICONS in two punctuation slots.** Index `0x0A` is
  the little-person marker and `0x03` is half the connection symbol -- so every
  footer that names a keypad key ("`*` BACK", "`#` OK") drew a person and a
  lightning bolt. On a console whose only named keys are `*` and `#`, that is
  the one thing those footers exist to say. Both are synthesized now, at the
  font's own 2px weight, alongside `-`, `/` and `:`.
* **OS7's `RAND_GEN` has a fixed point at zero, and RAM powers up there.** It
  stirs `RANDNUM` in place, so a client that never seeds it gets the same
  "random" number for the life of the session -- which made the roll animation
  redraw five identical dice eleven times and look like nothing happening at
  all. `START` seeds it, the splash stirs it while it waits for the trigger
  (how long somebody takes to reach for it is the only real entropy this
  machine has), and `RANDFC` re-seeds if it ever reaches zero again.
* **A cue that goes quiet must not take the animation with it.** `SNDROLL` used
  to be blocking, and it was the *only* thing pacing `ROLLANM` -- so muting the
  sound ran the whole roll in microseconds, invisibly. It is non-blocking now
  and the animation owns its own frame count.
* **`B` is the register number in `WRITE_REGISTER`, not part of the value.**
  `LD BC,00C0H` to blank the display writes **R0**, and R0 bit 1 is M3 -- so it
  put the VDP straight back into Graphics I one instruction after selecting
  Graphics II, where R4 = `$03` points the pattern generator at `$1800`, the
  name table. The console came up a flat blue while every name-table byte was
  still perfectly correct. R1 bit 6 is also the display enable, so `$C0` never
  blanked anything either; blanking is `$A0`.
* **OS7's `WRITE_VRAM` does not honour a full 16-bit count.** Handed 720 it
  copies 464 -- one 256-byte page short -- and returns as though it had copied
  all of it; handed 512 it copies 512. `PUT_VRAM` has the documented version of
  the same disease. The charset's 720-byte tile block is the first thing
  uploaded, so codes `$3A-$59` -- the logo, the icons and *every piece of board
  chrome* -- silently never arrived. `DIWR` now copies in 128-byte chunks,
  which also keeps each burst inside the frame budget.
* **The name table is not the screen.** Both of the above read back perfectly
  out of VRAM: the codes were in their right places and the display showed
  nothing. Every harness here can now capture a PNG (`SNAPOUT=`, `make shot`),
  and a change to the charset or the VDP setup is not verified until somebody
  has looked at one.
* **OS7's `WRITE_VRAM` returns with BC at zero**, because it counts down with
  it. Five routines here run `DJNZ` loops around `DPUTC`; without a `PUSH BC`
  inside it, every one of them has its counter reset to zero each pass, so
  `DJNZ` decrements to 255 and never terminates. The logo blit did exactly that
  on the first run: it wrote the same 256-cell window forever and the screen
  filled with the ROM's own `$FF` padding.
* **`MODE_1` lays out the Mode 2 tables but does not unmask them.** It leaves
  R3 = `$80` and R4 = `$00`, and in Graphics II those are *masks*: all three
  screen thirds alias onto the first 2K of patterns and the colour table
  collapses to 64 bytes. `DINIT` writes `R3 = $FF` and `R4 = $03` after setting
  M3. Nothing may call `MODE_1` again afterwards either — it clears R1 bit 5,
  the vblank interrupt enable, and the client stays alive and goes deaf.
* **The Namco font has no lowercase.** Its letterforms ARE the capitals, which
  is why the MS-DOS port keeps them in its *lowercase* slots and uses ascii case
  as a colour escape. Every string the server sends is lowercased, so `DFOLD`
  sits on the path of every wire string that reaches the screen; without it a
  table list renders as punctuation ("ai room" comes out as "!) 2//-").
* **A byte-sized scratch cell held a 16-bit value.** `SHMASK` kept its filled-row
  mask in `V_TMP` and quietly wrote the high half over `V_TMP2` — which held the
  seat it was building the mask *for*. Every seat got seat 0's mask, so the bot
  reveal re-fired on the same cell every poll. Word-sized scratch has word-sized
  storage now, and the ledger says so.
* **`CP $FF` leaves a borrow behind.** `RXSET` returned "not a real score" for
  every value below `$FF00` because the compare that ruled out the sentinels set
  carry and nothing cleared it. Every score read as unset, so the whole card was
  blank and every seat in the lobby read as a spectator.
* **`RXPLR` builds its 42-byte stride in BC**, so `RXSCOR` — which takes the
  slot in C — has to save it. And `RXVALID` reads through DE, so anything
  keeping a running value in D or E across it loses it: that is what put
  `PICKBEST`'s cursor on an arbitrary row.
* **MAME left unthrottled runs at 1500-3000%.** The game server is a real host
  taking real seconds to move its bots, so an unthrottled `SCREEN_AT=40` samples
  about two real seconds in and every turn still belongs to whoever had it at
  boot. `run.sh` throttles by default; `NOTHROTTLE=1` is for tests that never
  leave the console.
* **`zmac` trims its raw output to the lowest address actually emitted.** An
  `INCLUDE` placed before the `ORG` — `build/endpoint.inc` was — puts data at
  address 0 and shifts the whole image down, taking every mailbox address with
  it. `build.sh` checks for the cartridge header magic at offset 0 rather than
  assuming it.

## Building and running

```sh
./build.sh                  # build/fujitzee.bin, exactly 32768 bytes
./run.sh                    # MAME coleco with the fujinet cart device
make smoke                  # headless: does the splash come up?
make assets                 # regenerate assets/charset.inc
```

Prerequisites: `zmac` (found on `PATH`, or the copy vendored in the astrocade
bring-up, built on demand), a MAME tree with the ColecoVision FujiNet cart
device grafted in (`fujinet-firmware/pico/coleco/emu/apply.sh`), and a
`fujinet-pc` BoIP listener.

Environment: `ENDPOINT` (default `https://fujitzee.carr-designs.com/`,
regenerated into `build/endpoint.inc` on every build so a stale value cannot
survive), `ZMAC`, `FUJI_PICO`, `MAME_DIR`, `FUJINET_TCP` (default
`127.0.0.1:9995`), `NOTHROTTLE`.

Against a local server:

```sh
(cd <servers>/fujinet-game-system/fujitzee/server && go run .) &
ENDPOINT=http://127.0.0.1:8080/ ./build.sh
./run.sh
```

Two gates run on every build and are not optional: the bring-up's
`checkrom.py` enforces the 32768-byte layout, the header magic, the `FUJI` claim
at `0x7CFC` (without it the mailbox goes dead the moment the image boots) and
that nothing of ours reaches `0x7800-0x7FFF`; `tools/checksize.py` reports the
per-module budget.

### Headless harnesses

`emu/screen.lua` dumps the screen and gives a one-line verdict;
`emu/drive.lua` plays the hand controller first. Both fold the banked Graphics
II codes back to ascii, and mark each row `$` when it carries light-green cells,
`#` for cyan — colour is the thing most worth asserting here and it is invisible
in the glyphs alone.

```sh
SCREEN_AT=3 NOTHROTTLE=1 ./run.sh --headless "PRESS FIRE"
DRIVE="fire,hash,fire,fire" DRIVE_AFTER=4 SCREEN_AT=70 \
    ./run.sh --drive "YOUR TURN"
DRIVE_TRACE=1 DRIVE="fire,hash,fire,fire" SCREEN_AT=60 ./run.sh --drive
```

`SCREEN_EXPECT` is both the trigger and the verdict: the screen is dumped the
first moment that text appears, plus `DRIVE_AFTER` seconds, and `SCREEN_AT` is
the deadline after which it dumps anyway and fails. Waiting on a *condition*
rather than a clock is what makes "my turn" reproducible against a live server
whose bots take as long as they take. `DRIVE_TRACE=1` prints the prompt and the
header fields once a second, which is how you find out where the turns fall.

## Known gaps

* **A one-cell gap sometimes appears in the row-2 rule, at column 24.** Traced
  live, that cell holds the correct rule tile; the artifact is intermittent and
  has not been reproduced on demand. Cosmetic, and it survives the next
  wholesale repaint.
* **The lobby appkey is not read.** The name always starts from the built-in
  default rather than the FujiNet lobby's username (creator 1 / app 1 / key 0),
  and preferences are not persisted. The editor itself works.
* **No sound verification.** The cues are written and the chip is silenced
  first, but the headless harness runs with `-sound none`; only the
  silence-at-power-on rule is enforced by construction. The roll animation no
  longer depends on the sound for its timing, which was the one place that
  mattered.
* **PAL is not detected.** 60 Hz is assumed throughout.

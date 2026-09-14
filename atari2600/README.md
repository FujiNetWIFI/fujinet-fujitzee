# Fujitzee for the Atari 2600

A 6502 client for the [Fujitzee server](https://fujitzee.carr-designs.com/),
running over the FujiNet cartridge mailbox from the 2600 bring-up at
`fujinet-firmware/pico/atari-2600`, in the `?bin=1` wire format the
Intellivision, Astrocade, Arcadia, ColecoVision and Channel F ports share.

**A whole scorecard, five dice and a table of players on a console with 128
bytes of RAM**, of which the stack owns the top, no framebuffer, and a picture
the CPU draws a scanline at a time.

```sh
make                # build/fujitzee.bin, 32768 bytes
make layout         # the screen with no network at all
make shot           # ...snapshotted and read back, cell by cell
make run            # in a window
make drive          # headless: a real game against the live server
make frames         # every frame across that game must be 262 lines
make longframe      # ...and when one is not, which bank was mapped
make standings      # the left difficulty switch
make menu           # the SELECT menu
make reveal         # a bot's chosen row, held up on their card
make stack          # the stack stays above its floor
make inplag         # a press begun mid-compose is latched within a frame
make curbar         # the dice cursor bar, under the die the stick is on
make holdkeep       # a die you hold stays held until you roll
make dicecheck      # a roll rattles only the dice that actually moved
make gameover       # all thirteen rounds, and what the server calls it
```

`build.sh` needs Macroassembler AS and the firmware tree; `run.sh` needs a
MAME with `pico/atari-2600/emu/apply.sh` applied and a fujinet-pc listening.

## The screen

```
 R 7/13   T45     round, whose clock, and how long is left
>ALICE            the viewed player, '>' if they are the one to move
 ONE        3     the fifteen card rows: an 8-column label and a
 TWO        6     3-column right-aligned value
 ...
 UP  59 B  0      the two server-computed upper rows share a row
 ...
 TOTAL    236     summed from slots 6-14, never read from slot 15
 [..][..][..]     the five dice, from the client's own ROM art
  ==      ==      hold markers under the ones that will be kept
 >ROLL     X2     the tile a turn starts on, and the rolls left
```

**One category per row, and that is not a layout preference.** The kernel
writes `COLUP0` and `COLUP1` from the same value on a seam line that has four
cycles in hand, so a text row has exactly one ink colour. Every other port in
this family uses a two-column card -- which fits sixteen columns exactly --
and none of them could paint one category green without painting its
neighbour green too.

| colour | meaning |
|--------|---------|
| white | an ordinary row; a filled score |
| **green** | an open row with a potential: what this roll would score there |
| yellow | the cursor row, or the row a bot has just taken |
| blue | a closed row -- the page's own colour, so it reads as absent |

The corollary is a rule: **nothing that must be read may be drawn in the page
colour.** A zero potential is blank rather than "0", and shows only under the
cursor: an open card would otherwise be a wall of zeros, and the one number a
player actually wants would be the hardest thing on the screen to find.

The standings are a **class, not an overlay** -- twenty-one rows cannot hold a
scorecard and a seat list at once, which is what the left difficulty switch is
for. Game over is another: the server's prompt is the only thing on the wire
that names the winner, and it gets four rows of its own.

## The dice

Five dice, eight colour clocks each, drawn by the client's own kernel from art
in its own ROM. Nothing was added to the cartridge for this: Battleship grew
four playfield transforms and 5 Card Stud grew a card transform, and this port
grew none.

**It needs no repositioning at all.** The text kernel's six 8-clock objects
become five when `NUSIZ1` goes from three copies to two, and the five land at
pixels 52-91 -- exactly text columns 0-9, so the hold markers on the row below
line up for free. Leave `NUSIZ1` at three and P1 draws a sixth object at pixel
92 showing whatever `GRP1` last held: die 3 reappears at the right-hand end of
the tray, and nothing about it looks like a bug.

**Per-die colour does not exist on this console.** In that span `COLUP0`
paints dice 0, 2 and 4 and `COLUP1` dice 1 and 3. So the three states are
carried without it:

- **held** is in the ART. An ordinary die is a solid body with the pips
  punched through to the blue page -- which is what a die actually looks like
  -- and a held one is a bar above and below with the pips drawn in ink
  between them. A bracket cannot be confused with a solid die at any size.
  There are hold markers on the row beneath as well, because the tray is eight
  pixels wide and one signal is not enough.
- **the cursor** is MISSILE 0, on two lines below the tray where the sprites
  are blanked and `COLUP0` is therefore free. Not the ball: the ball takes
  `COLUPF`, and `COLUPF` is the playfield border, which is what gives every
  seam line until cycle 32 to write `COLUBK` instead of 22.67.
- **rolling** cycles the faces for eight flickers, and **only the dice the
  wire's `keepRoll` says moved**, so an opponent's roll rattles correctly and
  not just yours.

A die's whole state is the LOW BYTE of its kernel pointer -- `held*72 +
face*12` -- so the held art costs no extra RAM, no extra cycles and no per-die
test anywhere in the kernel.

## Controls

| | lobby | your turn | waiting |
|---|---|---|---|
| stick | -- | left/right the dice, up to the card, up/down the rows | -- |
| FIRE | ready up (a toggle) | hold a die / roll / take a row | -- |
| SELECT | the menu | the menu | the menu |
| RESET | poll now | poll now | poll now |
| left difficulty | the standings, for as long as it is up | | |

The stick auto-repeats after a third of a second, then fifteen a second --
fifteen card rows end to end is a second. Every press is seen in the frame it
is made, whatever the client happens to be doing: see the input latch below.

RESET is a SWITCH on this console -- a bit in a RIOT register the program
reads -- and it restarts nothing. What it means is the client's to choose, and
"poll now" is worth more than anything else: a player who thinks the game has
stalled wants the screen to catch up.

The cursor starts on ROLL, because rolling is always the first move, and after
the last roll it parks on the highest-paying open row -- so fire, fire, fire,
fire plays a legal, if artless, game.

## Banks

**Fifteen 2K banks and the fixed half, 32768 bytes; eight of them are real.**
A bank is 2048 bytes whatever the image is, so the image is not what fixed
anything -- the composer had to be split three ways, and eight banks is not
one of the four sizes MAME's cart slot accepts.

| | bank | |
|---|---|---|
| 0 | `fzlobby` | the cold start and the table list |
| 1 | `fzgame` | the picture, the tray, the cursors, the clock, the cues |
| 2 | `fznet` | one request, with the picture up, and back |
| 3 | `fzmenu` | the SELECT menu and the help |
| 4 | `fzname` | the keyboard and the shared appkey username |
| 5 | `fzcard` | the name row and the fifteen card rows, a few a frame |
| 6 | `fzchrome` | the status row, the prompt, the panels |
| 7 | `fzedge` | what the reply CHANGED |

The top 176 bytes of every drawing bank are reserved for the dice art and the
per-row background table, and the code is required at assembly time to stop
before them.

## Traps paid for here

- **The art's ORG runs backwards.** The first cut put `FZART` at `$1600` with
  the `ORG` after the includes, and the code had already run past it: the art
  assembled ON TOP OF the code, and the listing reported a bank that fitted,
  because it did. Hence the reserved page and the `IF * > FZART` guard.
- **`DPTRn`'s high byte survives a bank switch and `DINIT` does not run
  again**, so the art has to be at the same address in every bank that draws
  -- four of them. `tools/checkart.py` is that check, and it was itself wrong
  for a while: it repeated `FZART` instead of reading it, so when the reserved
  page moved it went on comparing 144 bytes of something else, in four banks
  at once, and passed.
- **`DFACE` must add the art's base, not just the offset.** It did not, and
  nothing showed it while the art happened to be page-aligned -- the offset
  and the low byte were the same number. Moving the art off a page boundary
  turned every die into a pointer thirty bytes below it and the tray came up
  empty.
- **`HMOVE` does not move anything at the moment it is strobed.** It drives
  the objects from the HM registers through the following hblank, so an
  `HMCLR` three cycles later stops the motion half done: P0 kept 8 of its
  9-clock gap instead of closing it to 8, which put every P1 object one clock
  right, merged dice 1 with 2 and 3 with 4, and stretched the text block by a
  pixel. There is a `WSYNC` between them now.
- **The dice writes have WINDOWS, not deadlines.** A write to `GRP1` replaces
  what P0 is DISPLAYING, so it must land after the copy it would overwrite has
  been drawn as well as before the one it is for. Bunching all six by cycle 43
  gave a tray of dice 2, 3, 4, 3, 4 -- five dice, in the right places, all the
  right shapes, three of them the wrong ones. Moving the fourth write to cycle
  39 produced BYTE-FOR-BYTE the same picture, which is what finally made the
  point: reading the pointers back out of memory is what proved they had never
  been wrong.
- **`FNENDW` counts its bounded wait down in Y.** The prompt's word-wrap kept
  its cursor into the string there and stored it afterwards: the first row
  wrapped correctly and every row after it started from a count of 128, which
  reads as a prompt that is one line long.
- **A compose margin has to cover the worst row, not the average one.** A
  category row is about eleven timer units; the TOTAL row calls `RUNTOT`,
  walks nine score words through a sixteen-bit pointer and costs nearer
  twenty-eight. One margin for both ran the frame eight lines long, once per
  poll, for the whole of a game. The margin is asked per row now.
- **A bank entered from a hook must finish that frame.** The card bank called
  `DLOOP` instead of `DFRAME2` and threw a 40-line frame at the set every time
  a reply landed -- `DFRAME` waited on a timer armed for the vblank, not the
  overscan.
- **`SCSCAN` arms `FZREVSL` as a side effect**, and the shadow it is about to
  replace still belongs to the seat whose turn just ended. Staging it before
  anything read the answer overwrote the real reveal: a bot that took ONE was
  correctly found, correctly drawn, and then reported as having taken CHANCE.
- **INPUT HAS TO BE SCANNED IN EVERY BANK, not just the ones that act on it.**
  This was the single worst bug in the port and it was entirely invisible from
  the code: the network bank and the three composer banks are mapped for up to
  TWENTY-FOUR CONSECUTIVE FRAMES after every poll, and while one of them was
  mapped nothing updated what the edge detector compares against. A press made
  in that window was not there when it was looked for, and the first thing
  that moved the cursor was the auto-repeat, half a second later -- which is
  what "I had to hold the direction down for a while for it to move once"
  means. Resyncing the latch when the game bank came back made it worse, not
  better: a press begun during a compose was then guaranteed to be swallowed.
  `DFRAME` now scans once a frame in every bank and ORs the result into a
  latch, and whichever hook cares drains it. `make inplag` measures it:
  presses begun mid-compose are latched in ONE frame.
- **A compose budget that can stall is a HANG, not a slowdown.** Adding that
  per-frame scan took about two timer units out of the vblank before the card
  bank's hook ever ran, and the TOTAL row's margin -- deliberately close to
  the whole vblank, because the row is nearly a whole vblank's work -- stopped
  being reachable. The row was never composed, the bank never handed over, and
  the client sat in the composer for the rest of the session with a frozen
  picture. The first row of every frame is now composed unconditionally, and
  the margin decides only whether a second one fits.
- **Deciding and placing are two things.** The dice cursor bar was never
  enabled at all -- only the layout ROM ever wrote `FZMCUR`, so in the real
  game missile 0 was permanently off and there was no way to see which die the
  stick was on. Moving the decision into `DMPOS`, where the position is
  chosen, fixed that; bailing out before the strobe when the bar was hidden
  broke it again, because the object was then enabled on a frame it had never
  been given a position on.
- **MAME's frame boundary is not the program's VSYNC.** Sampling zero page at
  `frame_done` can catch the client already a vblank into the next frame. An
  early test read the cursor as die 4, snapshotted a bar drawn under die 0,
  and looked exactly like a positioning bug -- for several rounds of chasing
  it. Wait for the value to be STILL before believing a snapshot of it.
- **`VALID8` returned the length that proved the reply, not zero.** A
  145-byte table listing came back as error `$91`.
- **`validScores[]` is fifteen zeros in the lobby** and zero is a legal
  potential, so an unguarded read paints the whole card green on the ready
  screen.
- **`keepRoll`'s `'1'` means RE-ROLL**, and it is an ECHO of the last roll
  that actually went out -- it does not move until the next one does. So on
  your own turn the BUTTON owns the mask and the wire must not be allowed to
  overwrite it. Restaging from the echo on every poll, which is what the first
  cut did while its own comment said otherwise, throws away every hold made
  since: the dice you picked sit there held and then deselect themselves a
  second and a half later, when the next poll lands. It is resynced only when
  a roll has just gone out, when the turn has changed, or when it is not your
  turn at all.
- **A test that does the right thing too quickly proves nothing.** The driver
  held a die and rolled again three frames later, so the hold never had to
  survive a poll and `make holdkeep` passed against code that was plainly
  broken. It sits on the hold for 150 frames now -- longer than the 90-frame
  poll -- and the gate was re-run against the broken version to watch it fail
  before it was believed.
- **The lobby's ready toggle is sampled every frame**, not once per poll: the
  lobby is a panel the composer draws when a reply lands, so a press made
  between two polls would simply be dropped.
- **MAME remembers switch positions between runs.** A test that left the left
  difficulty up made the next run boot straight into the standings screen,
  where there is no card and no turn -- which looks exactly like a client that
  never joined.
- Everything the two sibling 2600 ports learned still applies: RAM is not
  cleared by a reset and the cold stub forces a cold entry; the cartridge's
  four path buffers survive one too; SEQ comes from the cart's own persisted
  ACKSEQ and never from a counter in RAM; the trampoline resets the stack;
  never read `$1D00-$1EFF`; `p2bin` truncates an over-long bank in silence.

## Not here

- **Real hardware.** The firmware builds and its host tests pass; bus timing
  is the one thing emulation cannot settle.
- **PAL.** The line counts are NTSC.
- **More than seven tables in the lobby, or nine seats on a panel.**
  `FNWRPL` and `FNRPLA` index the reply window with a single byte, so table
  7's record starts one byte past what they can reach; and a nine-seat state
  is 473 bytes, which is the last one that fits in a single 512-byte slice
  with no paging at all. A bigger table still reads and still validates, and
  says `+N MORE`.
- **Lower case.** The font has none, and the wire is lowercased.

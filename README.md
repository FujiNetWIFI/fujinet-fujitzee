This is a WIP cross platform game client for the Fujitzee server.

Fujitzee is a clone of the popular Yahtzee dice game.

### Supported Platforms
* **Atari**
* **Apple II**
* **CoCo** *(WIP)*
* **Coleco Adam**
* **ColecoVision** *(cartridge, over the FujiNet cart mailbox)*
* **Fairchild Channel F** *(cartridge, over the FujiNet cart mailbox)*
* **Magnavox Odyssey 2** *(cartridge, over the FujiNet cart mailbox)*
* **NES** *(cartridge, over the FujiNet cart mailbox)*
* **Atari 7800** *(cartridge, over the FujiNet cart mailbox)*
* **Atari 5200** *(cartridge, over the FujiNet cart mailbox)*
* **Sega Master System** *(cartridge, over the FujiNet cart mailbox)*
* **Heathkit H89** *(HDOS and CP/M, on the H19 terminal, over the H89 FujiNet card)*
* **C64** *(planned)*
* *Please contribute to add more!*

### To Test/Run
1. Start Fujinet-PC for your appropriate platform
2. Tweak the emulator start commands in makefile / Makefile.coco, and for CoCo, the cp command that copies the dsk to the Fujinet-PC SD directory
3. Build/run per below

### To build: *CoCo*

*NOTE:* The latest fujinet-lib release (4.7.3) does not work with CoCo, so the latest fujinet-lib needs to cloned and built to work.

`make -f Makefile.coco`


### To build: *Coleco Adam*

There is no published Adam fujinet-lib release yet, so build the sibling
checkout first, then build the game inside the z88dk container (run from the
directory that holds both checkouts so `../fujinet-lib/build` resolves):

```
defoogi make -C fujinet-lib TARGETS=adam
defoogi make -C fujinet-fujitzee adam
```

The bootable tape image lands at `r2r/adam/fujitzee.ddp` (and is copied to
`~/tnfs/` if that directory exists).

### To build: *ColecoVision*

A standalone Z80 client in the `coleco/` directory, not part of the mekkogx
build: the console has 1K of RAM and the shared C core needs about 2.1 KB of
statics, so this one reads the server's reply in place out of the cartridge
mailbox window instead of buffering it. It talks to the FujiNet cartridge from
`fujinet-firmware/pico/coleco`.

```
cd coleco && ./build.sh          # -> build/fujitzee.bin, exactly 32768 bytes
cd coleco && ./run.sh            # MAME, against a fujinet-pc BoIP listener
```

Needs `zmac` and, to run, a MAME tree with the FujiNet cart device grafted in.
See `coleco/README.md` for the design, the colour scheme and the harnesses.

### To build: *Fairchild Channel F*

A standalone F8 (3850) client in the `channelf/` directory, not part of the
mekkogx build. This is the roomy console of the cartridge family and the
cramped screen: the cart services write cycles, so it hands the console 30K of
RAM and the whole reply flat at `$F800` -- but the display is 23 columns by
nine ROWS, against the Intellivision's 20x12. The scorecard is folded into four
columns of four, which is the wire's own slot order read across. It talks to
the FujiNet cartridge from `fujinet-firmware/pico/channelf`.

```
cd channelf && ./build.sh        # -> build/fujitzee.bin, exactly 16384 bytes
cd channelf && ./run.sh          # MAME, against a fujinet-pc BoIP listener
```

Needs Macroassembler AS (`asl`/`p2bin`) and, to run, a MAME tree with the
FujiNet cart device grafted in. See `channelf/README.md` for the layout, the
banded palette and the F8 traps.

### To build: *Magnavox Odyssey 2*

A standalone 8048 client in the `o2/` directory, not part of the mekkogx
build: the console has 64 bytes of RAM inside the CPU and 128 outside it, so
the shared C core is out of the question. It is a protocol 2 (APPBANK)
image -- the cart serves a fixed 1K and eight 2K pages -- built on the
display engine and tooling of the other O2 FujiNet games (Texas Hold'em's,
5 Card Stud's), with the Channel F port's game logic. The whole card is one
screen, the dice down its right edge. It talks to the FujiNet cartridge
from `fujinet-firmware/pico/o2` (APPBANK: PR #2177).

```
cd o2 && make                    # -> build/fujitzee.bin, 17408 bytes
cd o2 && make run                # MAME, against a fujinet-pc BoIP listener
cd o2 && make test               # MAME against a private server (test/rig.sh)
```

Needs Macroassembler AS (`asl`/`p2bin`) and, to run, a MAME tree with the
FujiNet cart device grafted in. See `o2/README.md` for the screen, the
controls, the pages and the tests.

### To build: *NES*

The shared C core, built by mekkogx with cc65: the FujiNet NES cartridge
serves 8K of RAM at `$6000`, which holds the core's statics comfortably. The
platform layer is `src/nes/`: the MS-DOS client's CGA art converted tile for
tile (`src/nes/mkchr.py`), drawn on the Adam's 32-column grid with the MS-DOS
25 rows, the Adam's three-voice sound on the 2A03, and an on-screen keyboard
for the player's name. One controller means one local player.

```
make nes                         # -> r2r/nes/fujitzee.nes (NROM-256, "FUJI" claim stamped)
make nes-smoke EXPECT="..."      # headless MAME, screen read out of the nametable
make nes-play                    # MAME in a window
```

Needs cc65 and the `add-nes` branch of `fujinet-lib-experimental` checked out
(and built for nes) at `~/Workspace/fujinet-lib-experimental`, or pass
`FUJINET_LIB=<path>`. To run, a MAME tree with the cartridge device grafted in
by `fujinet-firmware/pico/nes/emu/apply.sh`, and fujinet-pc listening on
`127.0.0.1:9995`.

Buttons: **A** is the trigger, **B** help, **START** the menu, **SELECT**
refreshes the table list, **SELECT+B** toggles sound and **SELECT+START**
quits.

### To build: *Atari 7800*

The shared C core, built by mekkogx with cc65: the FujiNet 7800 cartridge
runs a 32K client at `$8000` and serves it 16K of RAM at `$4000`. The
platform layer is `src/atari7800/`: the MS-DOS client's art converted for
the MARIA tile engine CONFIG also uses (`maria.s`, `src/atari7800/mkchr.py`
then `mktiles.py`), drawn on the Adam's 32x24 grid, the NES client's sound
cues on the cartridge's POKEY and its on-screen keyboard for the player's
name. One joystick means one local player.

```
make atari7800                   # -> r2r/atari7800/fujitzee.bin (for the cart) and .a78 (for MAME)
make atari7800-smoke EXPECT="..." # headless MAME, screen read out of the engine's map
make atari7800-play              # MAME in a window
```

Needs cc65 and the `add-atari7800` branch of `fujinet-lib-experimental`
checked out (and built for atari7800) at `~/Workspace/fnlib-7800`, or pass
`FUJINET_LIB=<path>`. To run, a MAME tree with the cartridge device grafted
in by `fujinet-firmware/pico/atari-7800/emu/apply.sh`, the 7800 BIOS, and
fujinet-pc listening on `127.0.0.1:9995` (`FUJINET_TCP=` to change it).

Controls: **FIRE** (button 1) is the trigger, button **2** help, **PAUSE**
the menu, **SELECT** refreshes the table list, **SELECT+2** toggles sound and
**RESET** quits.

### To build: *Atari 5200*

The shared C core, built by mekkogx with cc65: the FujiNet 5200 cartridge
runs a 32K client at `$4000` around its mailbox, and the console's 16K of
RAM holds the game's state, the screen and the player/missile graphics. The
platform layer is `src/atari5200/`: the Atari 8-bit client's screen on the
same ANTIC and GTIA (mode 4 at 40x26 in `support/atari/charset.fnt`, the
player highlight and board wall in player/missile graphics), its sound cues
on the same POKEY, and the 7800 client's on-screen keyboard for the player's
name. One controller means one local player.

```
make atari5200                   # -> r2r/atari5200/fujitzee.bin (for the cart and for MAME)
make atari5200-smoke EXPECT="..." # headless MAME, screen read out of the console's RAM
make atari5200-play              # MAME in a window
```

Needs cc65 and the `add-atari5200` branch of `fujinet-lib-experimental`
checked out (and built for atari5200) at `~/Workspace/fnlib-5200`, or pass
`FUJINET_LIB=<path>`. To run, a MAME tree with the cartridge device grafted
in by `fujinet-firmware/pico/atari-5200/emu/apply.sh`, the 5200 BIOS, and
fujinet-pc listening on `127.0.0.1:9995` (`FUJINET_TCP=` to change it).

Controls: the stick (or keypad **2 4 6 8**) moves, top **FIRE** (or **#**)
is the trigger, lower fire (or **1**) help, **PAUSE** (or **\***) the menu,
**START** refreshes the table list, **0** toggles sound and **RESET** quits
to the lobby, or to CONFIG when the lobby cannot be loaded. On the name
screen, lower fire (or **\***) deletes and **START** accepts.

### To build: *Sega Master System*

The shared C core, built by mekkogx with z88dk (`+sms`, sccz80): the console's
8K of RAM holds the core's statics and a shadow of the screen. The platform
layer is `src/sms/`, whose hardware half (VDP access, the pad latch, the
lobby boot) is fujinet-battleship's. The art is the MS-DOS client's, made into
Mode 4 tiles by `src/sms/mktiles.py`, with the MS-DOS palette exactly and the
active player's column blacked out per cell as MS-DOS does it, on the Adam's
32x24 layout. The highlighted dice and the "cannot roll" mark, which the
MS-DOS sheet lost, come back from the Atari original. Sound is the Adam's
SN76489 set, unchanged: the Master System has the same chip at the same
clock. An on-screen keyboard types the player's name; one pad means one local
player.

```
make sms                         # -> r2r/sms/fujitzee.sms (48K, "FUJI" claim stamped)
make sms-smoke EXPECT="..."      # headless MAME, screen read out of the name table
make sms-play                    # MAME in a window
```

The image is 48K: the code fills most of the first 32K, which must end below
the claim at `$7FDC`, and the 4bpp art is the third bank, kept in slot 2
below the cartridge's mailbox arena at `$B000`. The build checks both, and
that the cartridge will pick the Sega mapper for it (`support/sms/mapcheck.py`).

Needs z88dk (from `~/Workspace/z88dk` when `zcc` is not on `PATH`) and the
`add-sms` branch of `fujinet-lib-experimental` checked out (and built for sms)
at `~/Workspace/fnlib-sms`, or pass `FUJINET_LIB=<path>`. To run, a MAME tree
with the cartridge device grafted in by `fujinet-firmware/pico/sms/emu/apply.sh`,
and fujinet-pc listening on `127.0.0.1:9995`.

Buttons: **1** is the trigger, **Pause** the menu, **2** (tapped) refreshes
the table list, **2+Up** help, **2+Down** toggles sound and **2+1** quits.

### To build: *Heathkit H89*

The shared C core, built by mekkogx with z88dk twice from one tree: `+hdos`
(`FUJITZEE.ABS`) and `+cpm` (`FUJITZEE.COM`), both from `src/h89/` through
`PLATFORM_COMBOS`, guarded by `__H89__`. They talk to the H89 FujiNet card
(the 82C55 at 0xD0) through fujinet-lib's `h89-support` branch.

The game is drawn on the H19 terminal in 80 columns, with nothing but the
terminal's own character set: the line drawing and edge bar characters, the
bullet and the checkerboard, and reverse video, which is the alternate text
style. The score sheet is the 40 column one widened, with the score names in
full and six player columns eight wide. The active player's column header is
a reverse video tab, and the potential scores are reversed. The dice are 7x3,
which is square on the H19's tall cells: a free die is a solid block with
dark pips, and a kept one is hollow. The bordered screens hang their boxes
from a frame between four corner dice, and the 25th line is the status line.
The console runs at 9600 baud, so `src/h89/console.c` keeps a shadow of the
screen and sends only the cells that change. That code comes from
fujinet-5cardstud and fujinet-battleship.

```
make h89-hdos    # -> r2r/h89-hdos/fujitzee.h8d: MOUNT SY1: then RUN SY1:FUJITZEE.ABS
make h89-cpm     # -> r2r/h89-cpm/fujitzee.h8d:  B:FUJITZEE
make h89-smoke [H89_OS=hdos] [EXPECT="..."] [STEPS="..."] [SHOT=x.ppm]
```

Each `.h8d` is a 100K H17 data disk. The HDOS build loads at z88dk's
`$4480` and needs a 64K H89, where HDOS 2.0 keeps itself above `$EDB0`. The
build fails if the program would reach that. The CP/M build runs in the
64K CP/M 2.2.04 that FujiNet Go H89 ships.

Needs z88dk (from `~/Workspace/z88dk` when `zcc` is not on `PATH`) and
fujinet-lib's `h89-support` branch checked out (with `r2r/h89-cpm` and
`r2r/h89-hdos` built) at `~/Workspace/fujinet-lib`. `make h89-smoke` runs the
game headless in FujiNet Go H89 (`~/Workspace/fujinet-go-h89-desktop`, built)
against the live server; see `support/h89/smoke.sh`.

Keys: the keypad arrows (or `, . - =`) move, **SPACE** or **RETURN** selects,
**ESC** (or f1-f5) opens the menu, and **H** toggles help on your turn. There
is no H89 lobby: **Q** at the table list returns to HDOS or CP/M.

### To build: *Atari | Apple ][ | C64*

1. Set the appropriate target(s) to build in the **makefile**.
2. running `make` or `make clean` will download large dependencies like fujinet-lib/apple ii disk files.

### Typical make command

I normally run the following, which *cleans*, downloads dependencies, *builds*, prints the program size, and runs in the emulator.

`make clean test`

# Server / Api details

## Endianness
The server defaults to little-endian values for 16 bit values. To request big-endian from the server, define QUERY_SUFFIX as follows in src/[platform]/vars.h:

```c
#define QUERY_SUFFIX "&be=1"
```


Please visit the server page for general Api information:

https://github.com/FujiNetWIFI/servers/tree/main/fujinet-game-system/fujitzee/server#readme

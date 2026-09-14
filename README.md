This is a WIP cross platform game client for the Fujitzee server.

Fujitzee is a clone of the popular Yahtzee dice game.

### Supported Platforms
* **Atari**
* **Apple II**
* **CoCo** *(WIP)*
* **Coleco Adam**
* **ColecoVision** *(cartridge, over the FujiNet cart mailbox)*
* **Fairchild Channel F** *(cartridge, over the FujiNet cart mailbox)*
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

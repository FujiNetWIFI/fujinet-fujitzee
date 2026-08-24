This is a WIP cross platform game client for the Fujitzee server.

Fujitzee is a clone of the popular Yahtzee dice game.

### Supported Platforms
* **Atari**
* **Apple II**
* **CoCo** *(WIP)*
* **Coleco Adam**
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

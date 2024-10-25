This is a WIP cross platform game client for the Fujitzee server.

Fujitzee is a clone of the popular Yahtzee dice game.

### Supported Platforms
* **Atari**
* **Apple II**
* **CoCo** *(WIP)*
* **C64** *(planned)*
* *Please contribute to add more!*


### To build: *CoCo*
`make -f Makefile.coco`

### To build: *Atari | Apple ][ | C64*

1. Set the appropriate target(s) to build in the **makefile**.
2. running `make` or `make clean` will download large dependencies like fujinet-lib/apple ii disk files.

### Typical make command

I normally run the following, which *cleans*, downloads dependencies, *builds*, prints the program size, and runs in the emulator.

`make clean test`

# Server / Api details

Please visit the server page for more information:

https://github.com/FujiNetWIFI/servers/tree/main/fujinet-game-system/fujitzee/server#readme

This is a WIP cross platform game client for the Fujitzee server.

Fujitzee is a clone of the popular Yahtzee dice game.

### Supported Platforms
* **Atari**
* **Apple II**
* **CoCo** *(WIP)*
* **C64** *(planned)*
* *Please contribute to add more!*

### To Test/Run
1. Start Fujinet-PC for your appropriate platform
2. Tweak the emulator start commands in makefile / Makefile.coco, and for CoCo, the cp command that copies the dsk to the Fujinet-PC SD directory
3. Build/run per below

### To build: *CoCo*

*NOTE:* The latest fujinet-lib release (4.7.3) does not work with CoCo, so the latest fujinet-lib needs to cloned and built to work.

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

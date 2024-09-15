This is a WIP cross platform game client for the Fujitzee server.

Fujitzee is a clone of the popular Yahtzee dice game.

### Supported Platforms
* **Atari**
* **Apple II** *(WIP)*
* **C64** *(planned)*
* *Please contribute to add more!*

### To build

1. Set the appropriate target(s) to build in the **makefile**.
2. run `make` with no parameters once to download dependencies like fujinet-lib, or as needed when dependency versions change. You will need to do this for each platform.

### Typical make command

I normally run the following, which *cleans*, *builds*, outputs the executable *size*, and runs in the emulator.

`make clean test size`

Note that for Apple II, Fujinet-PC runsas part of the makescript before the emulator starts.

# Server / Api details

Please visit the server page for more information:

https://github.com/FujiNetWIFI/servers/tree/main/fujinet-game-system/fujitzee/server#readme

#ifdef _CMOC_VERSION_

#include<peekpoke.h>
#include<stdlib.h>
#include<stdint.h>
#include <fujinet-fuji.h>
#include"../platform-specific/graphics.h"
#include<coco.h>

void resetTimer() {
  setTimer(0);
}

uint16_t getTime() {
  return getTimer();
}

void quit() {
  resetGraphics();

  // Tell FujiNet to serve the game lobby on the next boot, then reset the
  // CoCo so the FujiNet cartridge boots straight into the lobby.
  fuji_set_boot_mode(2);
  coldStart();
}

void housekeeping() {
  // Not needed on CoCo
}

uint8_t getJiffiesPerSecond() {
  return 60;
}

#endif

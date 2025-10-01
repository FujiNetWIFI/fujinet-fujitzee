#ifdef _CMOC_VERSION_

#include<peekpoke.h>
#include<stdlib.h>
#include<stdint.h>
#include"../fujinet-fuji.h"
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
  memset(0x200,0,0x200);
  memset(0x600,0,0x1200);
  exit(0);
}

void housekeeping() {
  // Not needed on CoCo
}

uint8_t getJiffiesPerSecond() {
  return 60;
}

#endif
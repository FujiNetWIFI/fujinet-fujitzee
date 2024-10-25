#ifdef _CMOC_VERSION_

#include<peekpoke.h>
#include<stdlib.h>
#include<stdint.h>
#include"../fujinet-fuji.h"
#include"../platform-specific/graphics.h"

uint16_t jiffieTimer;

void resetTimer() {
  jiffieTimer=0;
}

int getTime() {
  jiffieTimer+=10;
  return jiffieTimer;
}

void quit() {
  resetGraphics();
  exit(0);
}


void housekeeping() {
  // Not applicable on CoCo
}

uint8_t getJiffiesPerSecond() {
  return 60;
}

#endif
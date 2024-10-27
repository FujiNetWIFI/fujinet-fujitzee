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

uint16_t getTime() {
  jiffieTimer+=10;
  return jiffieTimer;
}

void quit() {
  resetGraphics();
  exit(0);
}
#define disableInterrupts() asm("ORCC",  "#$50")

void housekeeping() {
  // Disable interrupts when not needed to avoid some flcker
  // on the hires screen that overlaps with disk routines
  disableInterrupts();
}

uint8_t getJiffiesPerSecond() {
  return 60;
}

#endif
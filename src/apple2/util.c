#ifdef __APPLE2__

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
  // Possible to revert screen to boot normals
  resetGraphics();
  exit(0);
}


void housekeeping() {
  // Not applicable on Apple
}

#endif /* __APPLE2__ */
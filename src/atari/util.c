#ifdef __ATARI__

#include<peekpoke.h>
#include<stdlib.h>
#include<stdint.h>
#include<atari.h>
#include<string.h>
#include "../misc.h"
#include"../fujinet-fuji.h"
#include"../platform-specific/graphics.h"


void resetTimer() {
 POKEW(0x13,0);
}

int getTime() {
  return (PEEK(0x13)*256)+PEEK(0x14);
}

void quit() {
  resetScreen();
  resetGraphics();
  fuji_set_boot_config(1);
  exit(0);
}

void housekeeping() {
  // Clear attract mode
  OS.atract=0;
}
#endif /* __ATARI__ */
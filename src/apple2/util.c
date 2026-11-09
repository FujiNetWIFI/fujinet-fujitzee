#ifdef __APPLE2__

#include<peekpoke.h>
#include<stdlib.h>
#include<stdint.h>
#include <fujinet-fuji.h>
#include "../platform-specific/graphics.h"

uint16_t jiffieTimer;

void resetTimer() {
  jiffieTimer=0;
}

uint16_t getTime() {
  jiffieTimer+=10;
  return jiffieTimer;
}

void quit() {
  joy_uninstall();
  resetGraphics();
  fuji_set_boot_mode(FUJI_BOOT_MODE_LOBBY);
  // ROM in, then spoil the reset vector's checksum so the reset is a cold boot
  __asm__ ("bit $c082");
  __asm__ ("dec $03f4");
  __asm__ ("jmp ($fffc)");
}


void housekeeping() {
  // Not applicable on Apple
}

uint8_t getJiffiesPerSecond() {
  return 60;
}

#endif /* __APPLE2__ */

#ifdef __ATARI__

#include<peekpoke.h>
#include<stdint.h>
#include<atari.h>
#include<string.h>
#include "../misc.h"
#include <fujinet-fuji.h>
#include "../platform-specific/graphics.h"


void resetTimer() {
 POKEW(0x13,0);
}

uint16_t getTime() {
  return (PEEK(0x13)*256)+PEEK(0x14);
}

// Cold start into the lobby: exit(0) would leave an XL in SELF TEST
void quit() {
  resetScreen(false);
  resetGraphics();
  fuji_set_boot_mode(FUJI_BOOT_MODE_LOBBY);
  asm("jmp $E477"); // COLDSV
}

void housekeeping() {
  // Clear attract mode
  OS.atract=0;
}

uint8_t getJiffiesPerSecond() {
  return PEEK(0xD014)==1 ? 50 : 60;
}
#endif /* __ATARI__ */

#if _CMOC_VERSION_

#include <stdbool.h>
#include "../../support/coco/bcontrol/Keyboard.h"

bool reverse, initKeyboard=false;
char ss[2] = {0,0};
unsigned char lastKey=0;
Keyboard keyboard;


unsigned char kbhit (void) { 
  if (!initKeyboard) {
    initKeyboard=true;
    Keyboard_init(&keyboard);
  }

  return lastKey=Keyboard_poll(&keyboard);
}

char cgetc (void) {
  if (!lastKey) {
    kbhit();
  }
  return (char)lastKey;
}

#endif

#if _CMOC_VERSION_

#include <stdbool.h>
#include "../../support/coco/bcontrol/Keyboard.h"

bool initKeyboard=false;
char lastKey=0;
Keyboard keyboard;


unsigned char kbhit (void) { 
  if (!initKeyboard) {
    initKeyboard=true;
    Keyboard_init(&keyboard);
  }

  return lastKey=(char)Keyboard_poll(&keyboard);
}

char cgetc (void) {
  static char key;
  while (!lastKey) {
    kbhit();
  }
  key = lastKey;
  lastKey=0;
  return key;
}

#endif

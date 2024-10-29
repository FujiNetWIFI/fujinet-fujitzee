#if _CMOC_VERSION_

#include <stdbool.h>
#include <coco.h>

char lastKey=0;

unsigned char kbhit (void) { 
  return lastKey=(char)inkey();
}

char cgetc (void) {
  char key=lastKey;
  
  lastKey=0;

  while (!key) {
    key=(char)inkey();
  }
  
  return key;
}

#endif

#ifdef _CMOC_VERSION_
/*
  Platform specific sound functions
*/

#include <stdint.h>
#include <stdlib.h>
#include <coco.h>
#include "../misc.h"
#include "../platform-specific/sound.h"

//#define CLICK  __asm__ ("sta $c030")

uint16_t ii;

void tone(uint16_t period, uint8_t dur, uint8_t wait) {
  // sound((byte)(100-period/2), dur/10);
  //  while (wait--)
  //   for (ii=0; ii<40; ii++) ;
}

// Keeping this here in case I need it
// void toneFinder() {
//   clearCommonInput();
//   while (input.key != KEY_RETURN || i<2) {
//     while (!kbhit());
//     input.key = cgetc();
//     if (input.key == KEY_DOWN_ARROW)
//       i-=1;
//     if (input.key == KEY_UP_ARROW)
//       i+=1;
//       printf("%d ",i);
//     tone(i,5,0);
//   }
// }

void initSound() {}

void soundJoinGame() {
  tone(36,50,50);
  tone(44,50,50);
  tone(36,50,0);
}

void soundFujitzee() {
  tone(146,20,0);
  tone(109,28,0);
  tone(86,35,0);

  tone(109,6,0);
  tone(72,42,100);

  tone(86,30,0);

  tone(109,6,0);
  tone(72,120,250);
}

void soundMyTurn() {
  tone(36,50,60);
  tone(36,80,0);
}

void soundGameDone() {
  tone(60,70,130);
  tone(45,230,200);
  tone(39,90,130);
  tone(35,250,0);
}


void soundRollDice() {
 tone(50+(rand() % 20)*5,3,0);
}

void soundRollButton() {
  tone(45,20,10);
  tone(36,20,0);
}

void soundCursor() {
  tone(47,10,0);
}

void soundScoreCursor() {
  tone(42,10,0);
}

void soundKeep() {
  tone(88,10,0);
  tone(84,10,0);
  tone(77,10,0);
}

void soundRelease() {
  for (i=105;i<=112;i++)
    tone(i,2,0);
}

void soundTick() {
 tone(80,2,0);
}

void soundScore() {
 tone(42,10,0);
 tone(29,30,0);
 tone(27,40,0);
}

// Not applicable to Apple
void soundStop() {}
void disableKeySounds() {}
void enableKeySounds() {}

#endif
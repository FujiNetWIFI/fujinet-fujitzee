#ifdef _CMOC_VERSION_
/*
  Platform specific sound functions
*/

#include <stdint.h>
#include <stdlib.h>
#include <coco.h>
#include "../misc.h"
#include "../platform-specific/sound.h"

uint16_t ii;

void tone(uint8_t period, uint8_t dur, uint8_t wait) {
  if (!prefs.disableSound) {
    sound(period, dur);
  }
  
  while (wait--)
    for (ii=0; ii<60; ii++) ;
}

// // Keeping this here in case I need it
// void toneFinder() {
//   clearCommonInput();
//   while (input.key != KEY_RETURN || i<2) {
//     while (!kbhit());
//     input.key = cgetc();
//     switch (input.key) {
//       case KEY_DOWN_ARROW: i--; break;
//       case KEY_UP_ARROW: i++; break;
//       case KEY_RIGHT_ARROW: i+=20; break;
//       case KEY_LEFT_ARROW: i-=20; break;
//       case KEY_ESCAPE: return;
//     }

//     printf("%d ",i);
//     tone(i,1,0);
//   }
// }

void initSound() {}

void soundJoinGame() {
  tone(40,1,50);
  tone(2,1,50);
  tone(40,1,0);
}

void soundFujitzee() {
  tone(0,1,20);
  tone(70,1,20);
  tone(110,1,20);

  tone(132,2,50);
  
  tone(110,1,20);
  tone(132,3,0);
}

void soundMyTurn() {
  tone(40,1,60);
  tone(40,2,0);
}

void soundGameDone() {
  tone(0,2,20);
  tone(70,3,100);
  tone(90,3,20);
  tone(110,5,20);
}


void soundRollDice() {
 tone(100+(rand() % 20)*7,0,0);
}

void soundRollButton() {
  tone(2,1,10);
  tone(40,1,10);
}

void soundCursor() {
  static int i;
  tone(0,0,0);
  tone(0,0,0);
  tone(0,0,0);
}

void soundScoreCursor() {
  tone(30,0,0);
  tone(30,0,0);
  tone(30,0,0);
}

void soundKeep() {
  static uint8_t i;
  for(i=0;i<10;i++)
    tone((i*13+8)%100,0,0);
}

void soundRelease() {
   tone(10,0,1);
   tone(10,0,2);
   tone(10,0,3);
   tone(10,0,0);
}

void soundTick() {
  tone(0,0,0);
}

void soundScore() {
 tone(80,1,5);
 tone(90,1,0);
}

// Not applicable to CoCo
void soundStop() {}
void disableKeySounds() {}
void enableKeySounds() {}

#endif
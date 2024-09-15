#ifdef __APPLE2__
/*
  Platform specific sound functions
*/

#include <stdint.h>
#include <stdlib.h>
#include <apple2.h>
#include "../misc.h"
#include "../platform-specific/sound.h"

#define CLICK  __asm__ ("sta $c030")

uint16_t ii;

void tone(uint16_t period, uint8_t dur, uint8_t wait) {
  while (dur--) {
    for (ii=0; ii<period; ii++) ;
    CLICK;
  }

  while (wait--)
    for (ii=0; ii<40; ii++) ;
}

// Keeping this here in case I need it
void toneFinder() {
  clearCommonInput();
  while (input.key != KEY_RETURN || i<2) {
    while (!kbhit());
    input.key = cgetc();
    if (input.key == KEY_DOWN_ARROW)
      i-=1;
    if (input.key == KEY_UP_ARROW)
      i+=1;
      cprintf("%i ",i);
    tone(i,50,0);
  }
}

void initSound() {
 
}

void soundJoinGame() {
  tone(36,50,50);
  tone(44,50,50);
  tone(36,50,0);
}

void soundFujitzee() {
  tone(83,70,20);
  tone(79,70,30);
  tone(65,70,20);
  tone(61,70,50);
}

void soundMyTurn() {
  tone(36,50,60);
  tone(36,80,0);
  
}

void soundGameDone() {
  tone(83,20,20);
  tone(79,50,30);
  tone(65,20,20);
  tone(61,40,50);
}


void soundRollDice() {
 // _sound(0, 150+ (rand() % 20)*5,8,8);
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
  for (i=88;i>=78;i--)
    tone(i,3,0);
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

#endif /* __APPLE2__ */
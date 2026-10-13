#ifdef BUILD_ATARI5200
/*
  Platform specific sound functions - Atari 5200

  The Atari 8-bit client's cues (src/atari/sound.c) on the same POKEY at
  $E800. cc65's _sound() is the 8-bit library's, so its stores are done here,
  less its SKCTL write: SKCTL stays as the crt0 set it, scanning the keypad,
  which also keeps POKEY out of initialisation so the voices run.
*/

#include <string.h>
#include <stdlib.h>

#include "../misc.h"
#include "../platform-specific/sound.h"

#define POKEY(r)   (*(volatile uint8_t *)(0xE800 + (r)))
#define AUDF(v)    POKEY((v) * 2)
#define AUDC(v)    POKEY((v) * 2 + 1)
#define AUDCTL     POKEY(0x8)

void initSound() {
  // The 64 kHz clock the 8-bit client's AUDF values assume
  AUDCTL = 0;
}

void sound(unsigned char voice, unsigned char frequency, unsigned char distortion, unsigned char volume) {
  if (prefs.disableSound)
    return;
  AUDF(voice) = frequency;
  AUDC(voice) = (distortion << 4) | volume;
}


void note(uint8_t n, uint8_t n2, uint8_t n3, uint8_t d, uint8_t f, uint8_t p) {
  static uint8_t i;
  if (prefs.disableSound)
    return;

  sound(0,n,10,8);
  if (n2)
    sound(1,n2,10,6);
  if (n3)
    sound(2,n3,10,4);


  pause(d);

  for (i=7;i<255;i--) {
    sound(0,n,10,i);
    if (n2 && i>1)
    sound(1,n2,10,i-2);
    if (n3 && i>3)
    sound(2,n3,10,i-4);
    pause(f);
  }
  pause(p);
}


void soundJoinGame() {
  static uint8_t j;
  for(j=0;j<2;j++) {
    note(81,0,0,0,1,0);
    if (j==0)
      note(96,0,0,0,1,0);
  }
}

void soundFujitzee() {
  note(76,153,0,5,0,0);
  note(57,230,0,5,0,0);
  note(45,182,0,5,0,0);
  note(37,153,0,5,1,2);
  note(45,182,0,5,0,0);
  note(37,153,0,6,2,0);
}

void soundMyTurn() {
  static uint8_t i,j;
   for (j=0;j<1;j++) {
    sound(0,81,10,5);
    pause(2);
    for (i=7;i<255;i--) {
      sound(0,81,10,i);
      waitvsync();
    }
    waitvsync();
  }
 }


void soundGameDone() {
  note(128,204, 64, 6, 2,0);
  note(96, 153, 193, 25, 2,3);
  note(85, 144, 172, 6, 2,0);
  note(76, 128, 153, 30, 3, 0);
}

void soundRollDice() {
  sound(0, 150+ (rand() % 20)*5,8,8);
}

void soundRollButton() {
  sound(0,96,10,5);
   pause(2);
   sound(0,81,10,4);
   pause(2);
   soundStop();

}

void soundCursor() {
   sound(0,102,10,7);
   pause(1);
   soundStop();
}

void soundScoreCursor() {
  sound(0,91,10,7);
  pause(1);
  soundStop();
}

void soundKeep() {
  static uint8_t i,j;
  j=0;
  for(i=200;i>150;i-=10) {
    sound(0,i,10,3+j++);
    waitvsync();
  }
  soundStop();
}

void soundRelease() {
  static uint8_t i;
 for(i=6;i<255;i--) {
    sound(0,255-i*5,10,i);
    waitvsync();
  }
}

void soundTick() {
  sound(0, 200,8,7);
  waitvsync();
  soundStop();
}

void soundStop() {
  sound(0,0,0,0);
}

// The keypad has no click to silence.
void disableKeySounds() {
}

void enableKeySounds() {
}

void soundScore() {
  static uint8_t i,j;
  j=0;
  for(i=80;i>50;i-=10) {
    sound(0,i,10,4+j++);
    waitvsync();
  }
  soundStop();
}

#endif /* BUILD_ATARI5200 */

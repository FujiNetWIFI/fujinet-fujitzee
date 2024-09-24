#ifdef __ATARI__
/*
  Platform specific sound functions
*/

#include <atari.h>
#include <string.h>
#include <peekpoke.h>
#include <stdlib.h>

#include "../misc.h"
#include "../platform-specific/sound.h"

// Set to control delay of played note
static uint8_t delay;

void initSound() {
  // Silence SIO noise
  OS.soundr = 0;
  disableKeySounds();
}

void sound(unsigned char voice, unsigned char frequency, unsigned char distortion, unsigned char volume) {
  if (prefs.disableSound)
    return;
  _sound(voice, frequency, distortion, volume);
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
  // note(76,153,0,0,1,0);
  // note(57,230,0,0,1,0);
  // note(45,182,0,0,1,0);
  // note(37,153,0,3,1,3);
  // note(45,182,0,0,1,0);
  // note(37,153,0,1,2,0);
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

void disableKeySounds() {
  OS.noclik = 255;
}

void enableKeySounds() {
  OS.noclik = 0;
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

#endif /* __ATARI__ */
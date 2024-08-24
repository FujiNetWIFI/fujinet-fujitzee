/*
  Platform specific sound functions 
*/
#ifndef SOUND_H
#define SOUND_H

#include <stdbool.h>
#include <stdint.h>

void initSound();

void disableKeySounds();
void enableKeySounds();

/* Will bring back sound toggle if there is a platform with no controllable sound volume
bool toggleSound();
void setSound();
*/

void soundStop();
void soundJoinGame();
void soundMyTurn();
void soundFujitzee();
void soundGameDone();
void soundRollDice();
void soundTick();
void soundPlayerJoin();
void soundPlayerLeft();

void soundCursor();
void soundScoreCursor();
void soundKeep();
void soundRelease();
void soundScore();



#endif /* SOUND_H */
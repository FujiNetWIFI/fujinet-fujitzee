#ifndef GAMELOGIC_H
#define GAMELOGIC_H

#include "platform-specific/graphics.h"

void processStateChange();
void processInput();
void handleAnimation();


void clearRenderState();

void centerText(unsigned char y, char * text);
void centerTextAlt(unsigned char y, char * text);
void centerTextWide(unsigned char y, char * text);
void centerStatusText(char * text);

bool resetInputField();
bool inputFieldCycle(uint8_t x, uint8_t y, uint8_t max, char* buffer);

void renderBoardNamesMessages();

void waitOnPlayerMove();


void progressAnim(unsigned char y);


#endif /*GAMELOGIC_H*/

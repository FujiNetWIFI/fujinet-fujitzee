/*
 Platform specific graphics commands
*/

#ifndef GRAPHICS_H
#define GRAPHICS_H

#define PARTIAL_LEFT 1
#define PARTIAL_RIGHT 2
#define FULL_CARD 3

#include <stdint.h>
#include <stdbool.h>
#include "../misc.h"


extern uint8_t scoreY[];
extern char* scores[];

// Call to clear the screen to an empty table
void resetScreen(bool forBorderScreen);

void drawText(unsigned char x, unsigned char y, char* s);
void drawTextAlt(unsigned char x, unsigned char y, char* s);
void drawTextVert(unsigned char x, unsigned char y, char* s);

void clearBelowBoard();

void drawChar(unsigned char x, unsigned char y, char c, unsigned char alt);

void drawIcon(unsigned char x, unsigned char y, unsigned char icon);

void drawFujzee(unsigned char x, unsigned char y);
void drawDie(unsigned char x, unsigned char y, unsigned char s, bool isSelected, bool isHighlighted);

void drawClock(unsigned char x, unsigned char y);
void drawBlank(unsigned char x, unsigned char y);
void drawSpace(unsigned char x, unsigned char y, unsigned char w);
void drawLine(unsigned char x, unsigned char y, unsigned char w);
void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h);
void drawBorder();
void drawBoard();
void drawDiceCursor(unsigned char x);
void hideDiceCursor(unsigned char x);

void saveScreenBuffer();
void restoreScreenBuffer();

void setHighlight(int8_t player, bool isThisPlayer, uint8_t flash );

void initGraphics();
void resetGraphics();
void waitvsync();

uint8_t cycleNextColor();
void setColorMode();
extern unsigned char colorMode;

#endif /* GRAPHICS_H */
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
void resetScreen();

void drawText(unsigned char x, unsigned char y, char* s);
void drawTextAlt(unsigned char x, unsigned char y, char* s);
void drawTextcursorPos(unsigned char x, unsigned char y);
void drawTextVert(unsigned char x, unsigned char y, char* s);
bool isEmpty(unsigned char x, unsigned char y);

void drawChar(unsigned char x, unsigned char y, char c);
void drawCharAlt(unsigned char, unsigned char y, char c);

void drawFujzee(unsigned char x, unsigned char y);
void drawDie(unsigned char x, unsigned char y, unsigned char s, bool isSelected, bool isHighlighted);
void drawMark(unsigned char x, unsigned char y);
void drawAltMark(unsigned char x, unsigned char y);
void drawClock(unsigned char x, unsigned char y);
void drawSpec(unsigned char x, unsigned char y);
void drawCursor(unsigned char x, unsigned char y);
void drawBlank(unsigned char x, unsigned char y);
void drawSpace(unsigned char x, unsigned char y, unsigned char w);
void drawLine(unsigned char x, unsigned char y, unsigned char w);
void hideLine(unsigned char x, unsigned char y, unsigned char w);
void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h);
void drawBorder();
void drawBoard();
void drawcursorPos(unsigned char x, unsigned char y);
void hidecursorPos(unsigned char x, unsigned char y);

void saveScreen();
void restoreScreen();

void initGraphics();
void resetGraphics();
void waitvsync();

uint8_t cycleNextColor();
void setColorMode();
extern unsigned char colorMode;

#endif /* GRAPHICS_H */
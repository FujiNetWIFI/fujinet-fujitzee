#ifdef __ATARI__

/*
  Graphics functionality
*/


#include <peekpoke.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <atari.h>
#include "../platform-specific/graphics.h"

extern unsigned char charset[];

// Set low enough that the program will run with or without BASIC enabled
#define CHARSET_LOC 0x9000
#define SCREEN_LOC ((uint8_t*)0x9400)
#define SCREEN_BAK 0x8B00

// Stack grows down from as low as 0x9C20

#define xypos(x,y) (SCREEN_LOC + x + (y)*WIDTH)

unsigned char colorMode=0, oldChbas=0;

// 26 lines
void DisplayList =
{
    DL_BLK8,DL_BLK8, // 2 Blanks Lines
    DL_LMS(DL_CHR40x8x4),SCREEN_LOC, // 1 Line
    DL_CHR40x8x4,DL_CHR40x8x4,DL_CHR40x8x4,DL_CHR40x8x4,DL_CHR40x8x4, // 5 Lines
    DL_CHR40x8x4,DL_CHR40x8x4,DL_CHR40x8x4,DL_CHR40x8x4,DL_CHR40x8x4, // 5 Lines
    DL_CHR40x8x4,DL_CHR40x8x4,DL_CHR40x8x4,DL_CHR40x8x4,DL_CHR40x8x4, // 5 Lines
    DL_CHR40x8x4,DL_CHR40x8x4,DL_CHR40x8x4,DL_CHR40x8x4,DL_CHR40x8x4, // 5 Lines
    DL_CHR40x8x4,DL_CHR40x8x4,DL_CHR40x8x4,DL_CHR40x8x4,DL_CHR40x8x4, // 5 Lines
    DL_JVB  /* JVB Destination will be updated at the real display list location*/
};

// Color scheme: Shadow, Highlights, White, Alt White, Background
const unsigned char colors[] = {
 0x00, 0x2A, 0x0E, 0xCC,  0x84, // NTSC
 0x00, 0xFA, 0x0E, 0xBC,  0x74 // PAL
};

const unsigned char diceChars[] = {
  // Normal dice
  0x41, 0x40, 0x42, 0x40, 0x45, 0x40, 0x43, 0x40, 0x44, // 1
  0x41, 0x40, 0x47, 0x40, 0x40, 0x40, 0x48, 0x40, 0x44, // 2
  0x41, 0x40, 0x47, 0x40, 0x45, 0x40, 0x48, 0x40, 0x44, // 3
  0x46, 0x40, 0x47, 0x40, 0x40, 0x40, 0x48, 0x40, 0x49, // 4
  0x46, 0x40, 0x47, 0x40, 0x45, 0x40, 0x48, 0x40, 0x49, // 5
  0x46, 0x40, 0x47, 0x4A, 0x40, 0x4B, 0x48, 0x40, 0x49, // 6

  // Kept dice
  0x21, 0x4C, 0x22, 0x20, 0x25, 0x20, 0x23, 0x4D, 0x24, // 1
  0x21, 0x4C, 0x27, 0x20, 0x20, 0x20, 0x28, 0x4D, 0x24, // 2
  0x21, 0x4C, 0x27, 0x20, 0x25, 0x20, 0x28, 0x4D, 0x24, // 3
  0x26, 0x4C, 0x27, 0x20, 0x20, 0x20, 0x28, 0x4D, 0x29, // 4
  0x26, 0x4C, 0x27, 0x20, 0x25, 0x20, 0x28, 0x4D, 0x29, // 5
  0x26, 0x4C, 0x27, 0x2A, 0x20, 0x2B, 0x28, 0x4D, 0x29, // 6

  // "Roll" button
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 13 - Empty space
  0x41, 0x40, 0x42, 0x2C, 0x2D, 0x2E, 0x43, 0x30, 0x44, // 14 - 1 Roll left
  0x41, 0x40, 0x42, 0x2C, 0x2D, 0x2E, 0x43, 0x2F, 0x44, // 15 - 2 Rolls left
  
  // "Roll" button pushed
  0xC1, 0xC0, 0xC2, 0xAC, 0xAD, 0xAE, 0xC3, 0xB0, 0xC4, // 16 - 1 Roll left
  0xC1, 0xC0, 0xC2, 0xAC, 0xAD, 0xAE, 0xC3, 0xAF, 0xC4,  // 17 - 2 Rolls left
  
  // Alternate color
  0xC1, 0xC0, 0xC2, 0xC0, 0xC5, 0xC0, 0xC3, 0xC0, 0xC4, // 1
  0xC1, 0xC0, 0xC7, 0xC0, 0xC0, 0xC0, 0xC8, 0xC0, 0xC4, // 2
  0xC1, 0xC0, 0xC7, 0xC0, 0xC5, 0xC0, 0xC8, 0xC0, 0xC4, // 3
  0xC6, 0xC0, 0xC7, 0xC0, 0xC0, 0xC0, 0xC8, 0xC0, 0xC9, // 4
  0xC6, 0xC0, 0xC7, 0xC0, 0xC5, 0xC0, 0xC8, 0xC0, 0xC9, // 5
  0xC6, 0xC0, 0xC7, 0xCA, 0xC0, 0xCB, 0xC8, 0xC0, 0xC9, // 6

  // Kept Alternate dice
  0xA1, 0xCC, 0xA2, 0xA0, 0xA5, 0xA0, 0xA3, 0xCD, 0xA4, // 1
  0xA1, 0xCC, 0xA7, 0xA0, 0xA0, 0xA0, 0xA8, 0xCD, 0xA4, // 2
  0xA1, 0xCC, 0xA7, 0xA0, 0xA5, 0xA0, 0xA8, 0xCD, 0xA4, // 3
  0xA6, 0xCC, 0xA7, 0xA0, 0xA0, 0xA0, 0xA8, 0xCD, 0xA9, // 4
  0xA6, 0xCC, 0xA7, 0xA0, 0xA5, 0xA0, 0xA8, 0xCD, 0xA9, // 5
  0xA6, 0xCC, 0xA7, 0xAA, 0xA0, 0xAB, 0xA8, 0xCD, 0xA9 // 6

};

unsigned char cycleNextColor() {
  return 0;
}

void setColorMode(unsigned char mode) {
 memcpy(&OS.color0, &colors[PEEK(53268)==1 ? 5 : 0], 5);
}


void initGraphics() {
  resetScreen();
  
  // Set the displaylist end JVB instruction to point to the start of the display list
  POKEW(PEEKW(0x230)+31, OS.sdlst);

  // Overwrite current Display list with custom
  waitvsync();
  memcpy(OS.sdlst,&DisplayList,sizeof(DisplayList));

  // Load custom charset
  memcpy((void*)CHARSET_LOC,&charset,1024);
  oldChbas = OS.chbas;
  OS.chbas = CHARSET_LOC/256;

  // Stub
  setColorMode(0);
}

// 19345
void saveScreen() {
  memcpy(SCREEN_BAK, SCREEN_LOC, WIDTH*HEIGHT);
}

void restoreScreen() {
  waitvsync();
  memcpy(SCREEN_LOC, SCREEN_BAK, WIDTH*HEIGHT);
}

void drawText(unsigned char x, unsigned char y, char* s) {
  static unsigned char c;
  static unsigned char* pos;

  pos = xypos(x,y);

  while(c=*s++) {
    if (c<65 && c>=32) c-=32;
    *pos++ = c;
  }  
}

void drawChar(unsigned char x, unsigned char y, char c) {
  if (c<65 && c>=32) c-=32;
  POKE(xypos(x,y), c);
}

void drawCharAlt(unsigned char x, unsigned char y, char c) {
  if (c<65 && c>=32) c-=32;
  if (c<65 || c> 90)
    c+=128;
  else      
    c+=32;

  POKE(xypos(x,y), c);
}

void drawTextAlt(unsigned char x, unsigned char y, char* s) {
  static unsigned char c;
  static unsigned char* pos;

  pos = xypos(x,y);

  while(c=*s++) {
    if (c<65 && c>=32) c-=32;
    if (c<65 || c> 90)
      c+=128;
    else      
      c+=32;

    *pos++ = c;
  }  
}

void drawTextVert(unsigned char x, unsigned char y, char* s) {
  static unsigned char c;
  static unsigned char* pos;

  pos = xypos(x,y);

  while(c=*s++) {
    if (c<65 && c>=32) c-=32;
    *pos = c;
    pos+=40;
  }  
}

void resetScreen() { 
  waitvsync();
  // Clear screen memory
  memset((void*)SCREEN_LOC,0,WIDTH*HEIGHT);
}

void drawDie(unsigned char x, unsigned char y, unsigned char s, bool isSelected, bool isHighlighted) {
  static unsigned char *source, *dest;
  
  source=diceChars + (s-1)*9 ; // Locate the diceChar index for this die number
  
  // Change the dice color
  if (isSelected)
    source+=54;

  // Change the dice color
  if (isHighlighted)
    source+=153;

  dest = xypos(x,y);
  
  memcpy(dest, source, 3);
  memcpy(dest+40, source+3, 3);
  memcpy(dest+80, source+6, 3);
}


void drawMark(unsigned char x, unsigned char y) {
  POKE(xypos(x,y),0x1D); // 0x21
}

void drawAltMark(unsigned char x, unsigned char y) {
  POKE(xypos(x,y),0x1C);
}

void drawClock(unsigned char x, unsigned char y) {
  POKE(xypos(x,y),0x37);
}

void drawSpec(unsigned char x, unsigned char y) {
  POKE(xypos(x,y),0xDC);
}

void drawBlank(unsigned char x, unsigned char y) {
  POKE(xypos(x,y),0);
}

void drawSpace(unsigned char x, unsigned char y, unsigned char w) {
  memset(xypos(x,y),0,w);
}

void drawTextcursorPos(unsigned char x, unsigned char y) {
  POKE(xypos(x,y),0xD9);
}

void drawCursor(unsigned char x, unsigned char y, unsigned char i) {
  POKE(xypos(x,y),i+0xBE);
}

/// @brief Returns true if the screen location is empty
bool isEmpty(unsigned char x, unsigned char y) {
  return PEEK(xypos(x,y))==0;
}

void drawBoard() {
  static uint8_t y,x,c;
  static unsigned char *dest;

  resetScreen();

  // Thin horz ines
  //memset(xypos(0,7),84,40);
  memset(xypos(10,7),84,30);
  memset(xypos(11,10),84,29);
  memset(xypos(0,20),84,10);

  // Thick horz lines
  memset(xypos(11,18),82,29);
  memset(xypos(11,20),82,29);

  // Main scores box
  drawBox(10,0,5,19);

  // Vertical linesz
  c=91;
  dest = xypos(20,0);
  for (y=0;y<21;y++) {
    for (x=0;x<20;x+=4) {
      *(dest+x)=c;
    }
    c = (y==19) ? 88 : 124; 
    dest+=40;
  }

  // Cross areas
  dest = xypos(10,7);
  for (x=0;x<7;x++) {
    POKE(dest,83);
    POKE(dest+40*3,83);
    POKE(dest+40*11,86);
    if (x)
      dest+=4;
    else
      dest+=6;
  }

  POKE(xypos(16,20),88);

  // Score names
  for(y = 0; y<16; y++) {
    drawTextAlt(11,scoreY[y],scores[y]);
  }
  
  // Fujitzee score!
  drawFujzee(11,17);

}

void drawFujzee(unsigned char x, unsigned char y) {
  memcpy(xypos(x,y),&"89:;<",5); // Fujzee
}

void drawLine(unsigned char x, unsigned char y, unsigned char w) {
  memset(xypos(x,y),82,w);
}


void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h) {
  static unsigned char i;
  static unsigned char* pos;

  pos = xypos(x,y);

  // Top row
  *pos=81;
  memset(pos+1,82,w);
  *(pos+w+1)=85;

  // Sides
  for(i=0;i<h;i++) {
    pos+=40;
    *pos=*(pos+w+1)=124;
  }
  
  // Bottom row
  *(pos+=40)=87;
  memset(pos+1,82,w);
  *(pos+w+1)=90;
}

void drawDiceCursor(unsigned char x, unsigned char y) {
  static unsigned char i;
  static unsigned char* pos;

  pos = xypos(x-1,y-1);

  // Top row
  *pos=177;
  memset(pos+1,182,3);
  *(pos+4)=178;

  // Sides
  for(i=0;i<3;i++) {
    pos+=40;
    *pos=*(pos+4)=181;
  }
  
  // Bottom row
  *(pos+=40)=179;
  memset(pos+1,182,3);
  *(pos+4)=180;
}

void hideDiceCursor(unsigned char x, unsigned char y) {
static unsigned char i;
  static unsigned char* pos;

  pos = xypos(x-1,y-1);

  // Top row
  memset(pos,0,5);

  // Sides
  for(i=0;i<3;i++) {
    pos+=40;
    *pos=*(pos+4)=0;
  }
  
  // Bottom row
  memset(pos+40,0,5);
}

void resetGraphics() {
  OS.color4=2;
  OS.chbas = oldChbas;
  waitvsync();
}



#endif /* __C64__ */
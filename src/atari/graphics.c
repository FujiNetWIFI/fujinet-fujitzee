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

// I'm using the space XL/XE allocates to BASIC for player/missles, charset, and screen buffer/backup
// On graphics initialization, I turn off BASIC, freeing up this space.

// There are two startup scenarios for XL/XE that must be considered:
// 1. If BASIC is disabled, the cc65 Stack will grow down from $BC1F, so to leave 512 bytes for stack,
//    so, I'm allowing $A000-$BA1F for charset, screen/buf, and pmgraphics.

// 2. If BASIC is enabled, the stack grows down from $9C1F. So, again allowing for 512 byte stack,
//    $9A1F is as high as we want to allow the program to reach without needing custom cc65 configs/segments

// Bottom line, program+vars needs to stay below $9A1F

#define CHARSET_LOC 0xB000
#define SCREEN_LOC ((uint8_t*)0xB400)
#define SCREEN_BAK 0xAB00
#define PM_BASE 0xA000


#define xypos(x,y) (SCREEN_LOC + x + (y)*WIDTH)

unsigned char colorMode=0, oldChbas=0, highlightX=0, prevHighlightX=0, missleLineVisible=0, prevMissleLineVisible=0, colIndex=0;

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

// Color scheme: 
// Player/Missle: Current player higlight, other player highlight, Shadow, Highlight
// Shadow, Highlights, White, Alt White, Background
const unsigned char colors[] = {
 0xA2, 0x82, 0x00, 0x2A, 0x00,  0x00, 0x2A, 0x0E, 0xCC,  0x84, // NTSC
 0x92, 0x72, 0x00, 0xFA, 0x00,  0x00, 0xFA, 0x0E, 0xBC,  0x74 // PAL
};

uint8_t own_player;

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


  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 13 - Empty space
    
  // "Roll" button
  0x41, 0x40, 0x42, 0x2C, 0x2D, 0x2E, 0x43, 0x30, 0x44, // 14 - 1 Roll left
  0x41, 0x40, 0x42, 0x2C, 0x2D, 0x2E, 0x43, 0x2F, 0x44, // 15 - 2 Rolls left
  0x41, 0x40, 0x42, 0x2C, 0x2D, 0x2E, 0x43, 0x50, 0x44, // 16 - Cannot roll
  
  // "Roll" button pushed
  0xC1, 0xC0, 0xC2, 0xAC, 0xAD, 0xAE, 0xC3, 0xB0, 0xC4, // 17 - 1 Roll left
  0xC1, 0xC0, 0xC2, 0xAC, 0xAD, 0xAE, 0xC3, 0xAF, 0xC4, // 18 - 2 Rolls left
  0xC1, 0xC0, 0xC2, 0xAC, 0xAD, 0xAE, 0xC3, 0xD0, 0xC4, // 19 - Cannot roll
  

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

uint8_t cursorUpper[] = {177,182,182,182,178,0,0,0,0,0};
uint8_t cursorLower[] = {179,182,182,182,180,0,0,0,0,0};

unsigned char cycleNextColor() {
  return 0;
}

void setColorMode(unsigned char mode) {
  colorMode = mode; 
  memcpy(&OS.pcolr0, &colors[colIndex+1], 9);
}


void initGraphics() {
  // Check if BASIC is enabled
  if (PEEKW(0x2E5)<0xBC1F && !(PEEK(0xD301)&2) ) {
    waitvsync();
    
    // Disable BASIC by setting bit 1 of 0xD301
    POKE(0xD301, PEEK(0xD301)|2);

    // Disable BASIC at shadow location
    POKE(0x3F8, 1);

    // Clear pmg memory
    memset(PM_BASE,0, 0x800);
  }

  resetScreen(false);
  if (PEEK(53268)==1) {
    colIndex=10;
  }
  
  // Set the displaylist end JVB instruction to point to the start of the display list
  POKEW(PEEKW(0x230)+31, OS.sdlst);

  // Overwrite current Display list with custom
  waitvsync();
  memcpy(OS.sdlst,&DisplayList,sizeof(DisplayList));

  // Load custom charset
  memcpy((void*)CHARSET_LOC,&charset,1024);
  oldChbas = OS.chbas;
  OS.chbas = CHARSET_LOC/256;

  // Initialize player+missle graphics, single line
  OS.sdmctl = OS.sdmctl | (12 + 16);
  POKE(0xD407, PM_BASE/256);

  // Turn on P+M
  POKE(0xD01D, 3);
  
  // Players behind playfield
  OS.gprior = 4; 
   
  // Vertical selector - player 0
  memset(PM_BASE+1024+28,0xff,158);
  POKE(0xd000,0); // horiz loc
  POKE(0xd008,1); //  Double width
  
  // Missle side wall
  memset(PM_BASE+768+29,0x18,156);

  // Side wall upper and lower rounder corners
  POKE(PM_BASE+768+26,0x20);
  POKEW(PM_BASE+768+27,0x3030);

  POKEW(PM_BASE+768+185,0x3030);
  POKE(PM_BASE+768+187,0x24);
  POKEW(PM_BASE+768+188,0x080c);
  
  POKEW(0xd005,0); // missle 1+2
 
  // Stub
  setColorMode(0);
}

void setHighlight(int8_t player, bool isThisPlayer, uint8_t flash ) {
  highlightX=player;
  POKE(0xd000,player>-1 ? player*16+112 : 0);
  if (isThisPlayer) {
    OS.pcolr0 = colors[colIndex] + flash; 
  } else {
    setColorMode(colorMode);
  }
}

bool saveScreenBuffer() {
  memcpy(SCREEN_BAK, SCREEN_LOC, WIDTH*HEIGHT);
  prevMissleLineVisible = missleLineVisible;
  prevHighlightX = highlightX;
}

void restoreScreenBuffer() {
  waitvsync();
  memcpy(SCREEN_LOC, SCREEN_BAK, WIDTH*HEIGHT);
  if (prevHighlightX) {
    setHighlight(prevHighlightX, state.localPlayerIsActive, 0);
  }
  
  if (prevMissleLineVisible) {
    POKEW(0xd005, 0xd0d0); // Right side missile location
    missleLineVisible=1;
  }
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

void drawChar(unsigned char x, unsigned char y, char c, unsigned char alt) {
  if (c<65 && c>=32) c-=32;
  if (alt) {
    if (c<65 || c> 90)
      c+=128;
    else      
      c+=32;
  }
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

void resetScreen(bool forBorderScreen) { 
  static uint8_t i;
  waitvsync();
  setHighlight(-1,0,0);
  POKEW(0xd005,0);  // Right side missile location
  missleLineVisible=false;
  // Clear screen memory
  if (!forBorderScreen) {
    memset((void*)SCREEN_LOC,0,WIDTH*HEIGHT);
  } else {
    memset(xypos(0,3),0,40*(HEIGHT-6));
    for(i=0;i<3;i++) {
      memset(xypos(3,i),0,WIDTH-6);
      memset(xypos(3,HEIGHT-i),0,WIDTH-6);
    }
  }
}

void drawDie(unsigned char x, unsigned char y, unsigned char s, bool isSelected, bool isHighlighted) {
  static unsigned char *source, *dest;
  if (!s || s>16)
    return;

  // Locate the diceChar index for this die number
  source=diceChars + (s-1)*9 ; 
  
  // Is die being kept?
  if (isSelected)
    source+=54;

  // Change the dice color
  if (isHighlighted) {
    if (s<14)
      source+=171;
    else
      source+=27;
  }

  // Draw the dice to the screen
  dest = xypos(x,y);
  memcpy(dest, source, 3);
  memcpy(dest+40, source+3, 3);
  memcpy(dest+80, source+6, 3);
}

void drawIcon(unsigned char x, unsigned char y, unsigned char icon) {
  POKE(xypos(x,y),icon);
}

void drawBlank(unsigned char x, unsigned char y) {
  POKE(xypos(x,y),0);
}

void drawSpace(unsigned char x, unsigned char y, unsigned char w) {
  memset(xypos(x,y),0,w);
}

void drawClock(unsigned char x, unsigned char y) {
  POKE(xypos(x,y),0x37);
}

void drawConnectionIcon(unsigned char x, unsigned char y) {
  POKEW(xypos(x,y), 0x0403);
}

void clearBelowBoard() {
  memset(xypos(0,HEIGHT-5),0,200);
}

void drawBoard() { 
  static uint8_t y,x,c;
  static unsigned char *dest;

  // Thin horz ines
  memset(xypos(10,9),84,30);
  memset(xypos(11,12),84,29);
  memset(xypos(0,5),84,10);

  // Thick horz lines
  memset(xypos(17,0),82,23);
  memset(xypos(11,2),82,29);
  memset(xypos(11,20),82,29);

  // Main scores box
  drawBox(10,2,5,17);
 
  // Vertical lines
  c=91;
  dest = xypos(16,0);
  for (y=0;y<20;y++) {
    for (x=0;x<24;x+=4) {
      *(dest+x)=c;
    }
    c=124;
    dest+=40;
  }

  // Cross sections
  dest = xypos(10,9);
  for (x=0;x<7;x++) {
    if (x) {
      POKE(dest-40*7,86);
      POKE(dest+40*11,88);
    }
    *dest=*(dest+40*3)=83;
    
    if (x)
      dest+=4;
    else
      dest+=6;
  }
  
  POKE(xypos(16,0),81);

  // Set 2 missles that are the rightmost vertical line
  POKEW(0xd005, 0xd0d0); // Right side missile location
  missleLineVisible=1;

  // Score names (16 for end game score)
  for(y = 0; y<14; y++) {
    drawTextAlt(11,scoreY[y],scores[y]);
  }
  
  // Fujitzee score text
  drawFujitzee(11,scoreY[14]);
  POKE(xypos(10,scoreY[14]),5);
}

void drawFujitzee(unsigned char x, unsigned char y) {
  memcpy(xypos(x-1,y),&"89:;<=",6); // "/|\TZEE"
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

void drawDiceCursorInternal(unsigned char x, unsigned char offset) {
  static unsigned char i;
  static unsigned char* pos;
  
  pos = xypos(x-1,HEIGHT-5);

  // Top
  memcpy(pos, cursorUpper+offset, 5);
  
  // Sides
  for(i=0;i<3;i++) {
    pos+=40;
    *pos=*(pos+4)=offset? 0 :181;
  }

  // Bottom
  memcpy(pos+40, cursorLower+offset, 5);
}

void drawDiceCursor(unsigned char x) {
  drawDiceCursorInternal(x,0);
}

void hideDiceCursor(unsigned char x) {
  drawDiceCursorInternal(x,5);
}

void resetGraphics() {
  OS.color4=2;
  OS.chbas = oldChbas;
  waitvsync();
}

#endif /* __ATARI__ */
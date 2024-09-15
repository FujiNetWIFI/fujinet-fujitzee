#ifdef __APPLE2__

/*
  Graphics functionality
*/


#include "hires.h"
#include "text.h"
#include <peekpoke.h>
#include <string.h>
#include "../platform-specific/graphics.h"
#include "../platform-specific/sound.h"
#include "../misc.h"

extern unsigned char charset[];

#define OFFSET_Y 4
unsigned char colorMode=0, oldChbas=0, highlightX=0, prevHighlightX=0, missleLineVisible=0, prevMissleLineVisible=0, colIndex=0;

uint8_t own_player;

const uint16_t diceRop[] = {
  ROP_CPY, ROP_CPY, ROP_AND(0b11010101), ROP_AND(0b10101010), ROP_AND(0b11010101)
};

const unsigned char diceChars[] = {
  // Normal dice
  0x01, 0x00, 0x02, 0x00, 0x05, 0x00, 0x03, 0x00, 0x04, // 1
  0x01, 0x00, 0x07, 0x00, 0x00, 0x00, 0x08, 0x00, 0x04, // 2
  0x01, 0x00, 0x07, 0x00, 0x05, 0x00, 0x08, 0x00, 0x04, // 3
  0x06, 0x00, 0x07, 0x00, 0x00, 0x00, 0x08, 0x00, 0x09, // 4
  0x06, 0x00, 0x07, 0x00, 0x05, 0x00, 0x08, 0x00, 0x09, // 5
  0x06, 0x00, 0x07, 0x0A, 0x00, 0x0B, 0x08, 0x00, 0x09, // 6

  // Kept dice
  0x11, 0x0C, 0x12, 0x00, 0x05, 0x00, 0x13, 0x0D, 0x14, // 1
  0x11, 0x0C, 0x17, 0x00, 0x00, 0x00, 0x18, 0x0D, 0x14, // 2
  0x11, 0x0C, 0x17, 0x00, 0x05, 0x00, 0x18, 0x0D, 0x14, // 3
  0x16, 0x0C, 0x17, 0x00, 0x00, 0x00, 0x18, 0x0D, 0x19, // 4
  0x16, 0x0C, 0x17, 0x00, 0x05, 0x00, 0x18, 0x0D, 0x19, // 5
  0x16, 0x0C, 0x17, 0x0A, 0x00, 0x0B, 0x18, 0x0D, 0x19, // 6

  // "Roll" button
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 13 - Empty space
  0x01, 0x00, 0x02, 0x0E, 0x0F, 0x10, 0x03, 0x1A, 0x04, // 14 - 1 Roll left
  0x01, 0x00, 0x02, 0x0E, 0x0F, 0x10, 0x03, 0x15, 0x04, // 15 - 2 Rolls left
  
  // "Roll" button pushed
  0x01, 0x00, 0x02, 0x0E, 0x0F, 0x10, 0x03, 0x1A, 0x04, // 16 - 1 Roll left
  0x01, 0x00, 0x02, 0x0E, 0x0F, 0x10, 0x03, 0x15, 0x04, // 17 - 2 Rolls left
  
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
  ++colorMode;
  if (colorMode>1)
    colorMode=0;
  return colorMode;
}

void setColorMode(unsigned char mode) {
  colorMode = mode; 
}

void initGraphics() {
   hires_Init();
}

void setHighlight(int8_t player, bool isThisPlayer, uint8_t flash ) {
  highlightX=player;
  // POKE(0xd000,player>-1 ? player*16+112 : 0);
  // if (isThisPlayer) {
  //   OS.pcolr0 = colors[colIndex] + flash; 
  // } else {
  //   setColorMode(colorMode);
  // }
}

void saveScreen() {
  memcpy(0x1000,0x2000,0x1000);
  memcpy(0x4000,0x3000,0x1000);
  // memcpy(SCREEN_BAK, SCREEN_LOC, WIDTH*HEIGHT);
  // prevMissleLineVisible = missleLineVisible;
  prevHighlightX = highlightX;
}

void restoreScreen() {
  memcpy(0x2000,0x1000,0x1000);
  memcpy(0x3000,0x4000,0x1000);
  
  // waitvsync();
  // memcpy(SCREEN_LOC, SCREEN_BAK, WIDTH*HEIGHT);
  // if (prevHighlightX) {
  //   setHighlight(prevHighlightX, state.localPlayerIsActive, 0);
  // }
  
  // if (prevMissleLineVisible) {
  //   POKEW(0xd005, 0xd0d0); // Right side missile location
  //   missleLineVisible=1;
  // }
}

void drawText(unsigned char x, unsigned char y, char* s) {
  static unsigned char c;
  // static unsigned char* pos;

  // pos = xypos(x,y);

  // while(c=*s++) {
  //   if (c<65 && c>=32) c-=32;
  //   *pos++ = c;
  // }  
  if (y==HEIGHT-1) {
    y=182;
  } else {
    y=y*8-OFFSET_Y;
  }

  while(*s) {
    c=*s++;
    if (c>=97 && c<=122) c=c-32;
    hires_putc(x++,y,ROP_CPY,c);
  }  
}

void drawChar(unsigned char x, unsigned char y, char c) {
  if (c>=97 && c<=122) c=c-32;
  hires_putc(x,y*8-OFFSET_Y,ROP_CPY,c);
}

void drawCharAlt(unsigned char x, unsigned char y, char c) {
  drawChar(x,y,c);
  // if (c<65 && c>=32) c-=32;
  // if (c<65 || c> 90)
  //   c+=128;
  // else      
  //   c+=32;

  // POKE(xypos(x,y), c);
}

void drawTextAlt(unsigned char x, unsigned char y, char* s) {
  drawText(x,y,s);
  // static unsigned char c;
  // static unsigned char* pos;

  // pos = xypos(x,y);

  // while(c=*s++) {
  //   if (c<65 && c>=32) c-=32;
  //   if (c<65 || c> 90)
  //     c+=128;
  //   else      
  //     c+=32;

  //   *pos++ = c;
  // }  
}

void drawTextVert(unsigned char x, unsigned char y, char* s) {
  static unsigned char c;
  y=y*8-OFFSET_Y;
  while(c=*s++) {
    //if (c<65 && c>=32) c-=32;
    //*pos = c;
    if (c>=97 && c<=122) c=c-32;
    hires_putc(x,y+=8,ROP_CPY,c);
  }  
}

void resetScreen() { 
  setHighlight(-1,0,0);

  // Clear screen memory
  hires_Mask(0,0,40,192,0xa900);
}

void drawDie(unsigned char x, unsigned char y, unsigned char s, bool isSelected, bool isHighlighted) {
  static unsigned char i,j,diceRopIndex;
  static unsigned char *source;
  diceRopIndex=0;
  
  if (s==16 || s==17) {
    s=s-2;
    diceRopIndex=3;
  }
  // Locate the diceChar index for this die number
  source=diceChars + (s-1)*9 ; 
  
  // Is die being kept?
  if (isSelected)
    source+=54;

  // Change the dice color
  if (isHighlighted)
   diceRopIndex=2;
  
  
    //source+=153;

  // Draw the dice to the screen
  y=y*8; //-OFFSET_Y;

  // If drawing the bottom dice, offset them lower to make room
  if (y==160) {
    y+=6;
  }

  for (i=0;i<3;i++) {
    for (j=0;j<3;j++) {
      hires_Draw(x+j,y,1,8,diceRop[diceRopIndex+(j!=1)],&charset[*(source++)<<3]);
    }
    y+=8;
  }
}


void drawMark(unsigned char x, unsigned char y) {
  //POKE(xypos(x,y),0x1D); // 0x21
  hires_putc(x,y*8-OFFSET_Y,ROP_CPY, 0x22);
}

void drawAltMark(unsigned char x, unsigned char y) {
  //POKE(xypos(x,y),0x1C);
  hires_putc(x,y*8-OFFSET_Y,ROP_INV, 0x22); 
}

void drawClock(unsigned char x, unsigned char y) {
  //POKE(xypos(x,y),0x37);
  hires_putcc(x,y*8-OFFSET_Y,ROP_CPY, 0x2526);
}

void drawSpec(unsigned char x, unsigned char y) {
  //POKE(xypos(x,y),0xDC);
  hires_putc(x,y*8-OFFSET_Y,ROP_CPY, 0x28);
}

void drawBlank(unsigned char x, unsigned char y) {
  //POKE(xypos(x,y),0);
  hires_putc(x,y*8-OFFSET_Y,ROP_CPY, 0x20);
}

void drawSpace(unsigned char x, unsigned char y, unsigned char w) {
  hires_Mask(x,y*8-OFFSET_Y,w,8,0xa900);
}

void drawTextcursorPos(unsigned char x, unsigned char y) {
  //POKE(xypos(x,y),0xD9);
  hires_putc(x,y*8-OFFSET_Y,ROP_CPY, 0x22);
}
 
void drawCursor(unsigned char x, unsigned char y, unsigned char i) {
  //POKE(xypos(x,y),i+0xBE);
  hires_putc(x,y*8-OFFSET_Y,ROP_CPY, 0x29+i);
}
 
/// @brief Returns true if the screen location is empty
bool isEmpty(unsigned char x, unsigned char y) {
  return true; //PEEK(xypos(x,y))==0;
}

void clearBelowBoard() {
  //memset(xypos(0,HEIGHT-5),0,200);
  hires_Mask(0,161,40,31,0xa900);
}

void drawBoard() {
  static uint8_t y,x,c;
  static unsigned char *dest;

  // // Vertical lines
  for (x=20;x<40;x+=4) {
    hires_Mask(x,0,1,160,0xa988);
  }

  // // Main scores box
  drawBox(16,0,23,1);
  drawBox(10,2,5,17);

  
 
 
  // // Thin horz ines
  hires_Mask(11,9*8,29,1, 0xa9ff); 
  hires_Mask(11,12*8,29,1, 0xa9ff);
  hires_Mask(0,5*8,10,1, 0xa9ff);  
  

  // // Thick horz lines
  // memset(xypos(17,0),82,23);
  // memset(xypos(11,2),82,29);
  // memset(xypos(11,20),82,29);
  
  //hires_Mask(17,0,23,2, 0xa9ff); 
  hires_Mask(11,2*8,29,2, 0xa9ff); 
  hires_Mask(11,20*8-1,29,2, 0xa9ff); 
  


  // c=91;
  // dest = xypos(16,0);
  // for (y=0;y<20;y++) {
  //   for (x=0;x<24;x+=4) {
  //     *(dest+x)=c;
  //   }
  //   c=124;
  //   dest+=40;
  // }

  // // Cross sections
  // dest = xypos(10,9);
  // for (x=0;x<7;x++) {
  //   if (x) {
  //     POKE(dest-40*7,86);
  //     POKE(dest+40*11,88);
  //   }
  //   *dest=*(dest+40*3)=83;
    
  //   if (x)
  //     dest+=4;
  //   else
  //     dest+=6;
  // }
  
  // POKE(xypos(16,0),81);

  // // Set 2 missles that are the rightmost vertical line
  // POKEW(0xd005, 0xd0d0); // Right side missile location
  // missleLineVisible=1;

  // // Score names (16 for end game score)
  for(y = 0; y<14; y++) {
    drawTextAlt(11,scoreY[y],scores[y]);
  }
  
  // // Fujitzee score text
  // drawFujzee(10,scoreY[14]);
  drawText(11,scoreY[14],"fuji!");
}

void drawFujzee(unsigned char x, unsigned char y) {
  //memcpy(xypos(x,y),&"89:;<=",6); // "/|\TZEE"
}


void drawLine(unsigned char x, unsigned char y, unsigned char w) {
  //memset(xypos(x,y),82,w);
  if (y==HEIGHT) {
    y=191;
  } else {
    y=y*8-OFFSET_Y+1;
  }
  hires_Mask(x,y,w,1, 0xa9ff); 
}

// 42653
void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h) {
  y=y*8-OFFSET_Y+1;

  // Top Corners
  hires_putc(x,y+3,ROP_CPY, 0x3b);
  hires_putc(x+w+1,y+3,ROP_CPY, 0x3c);
  
  // Accents if height > 1
  // if (h>1) {
  //   hires_putc(x+1,y+8,ROP_CPY, 1);
  // }

  // Top/bottom lines
  hires_Mask(x+1,y+3,w,2, 0xa9ff); 
  hires_Mask(x+1,y+(h+1)*8+2,w,2, 0xa9ff); 
  
  // Sides
  for(i=0;i<h;++i) {
    y+=8;
    hires_putc(x,y,ROP_CPY, 0x3f);
    hires_putc(x+w+1,y,ROP_CPY, 0x3f);
  }
  
    // Accents if height > 1
  // if (h>1) {
  //   hires_putc(x+w,y,ROP_CPY, 4);
  // }

  y+=7;
  // Bottom Corners
  hires_putc(x,y,ROP_CPY, 0x3d);hires_putc(x+w+1,y,ROP_CPY, 0x3e);
}

void drawDiceCursor(unsigned char x) {
  // Top / Bottom
  hires_Mask(x,190-26,3,1, 0xa955); 
  hires_Mask(x,191,3,1, 0xa955);

  hires_Mask(x+1,190-26,1,1, 0xa92A); 
  hires_Mask(x+1,191,1,1, 0xa92A);
  
  // Sides
  hires_Mask(x-1,190-26,1,28,0xa920);
  hires_Mask(x+3,190-26,1,28,0xa902);
}

void hideDiceCursor(unsigned char x) {
  //drawDiceCursorInternal(x,5);
  hires_Mask(x,190-26,3,1, 0xa900); 
  hires_Mask(x,191,3,1, 0xa900); 
  
  // Sides
  hires_Mask(x-1,190-26,1,28,0xa900);
  hires_Mask(x+3,190-26,1,28,0xa900);
}

void resetGraphics() {}

void waitvsync() {
  static uint16_t i;
  // Aproximate a jiffy for the timer countdown
  for ( i=0;i<630;i++);
}

#endif /* __APPLE2__ */
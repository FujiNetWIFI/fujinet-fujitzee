#ifdef _CMOC_VERSION_

/*
  Graphics functionality
*/


#include "hires.h"
#include <peekpoke.h>
#include <string.h> 
#include "../platform-specific/graphics.h"
#include "../platform-specific/sound.h"
#include "../misc.h" 
#include <coco.h>
   
#define enableInterrupts()  asm("ANDCC", "#$AF")

extern unsigned char charset[];
#define OFFSET_Y 4

uint8_t colorMode=0, oldChbas=0, colIndex=0;
int8_t highlightX=-1;
uint8_t own_player;
bool inBorderScreen=false;

#define ROP_CPY 0xff

// Mode 4
#define ROP_BLUE 0b01010101
#define ROP_ORANGE 0b10101010

uint8_t diceChars[] = {
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
  0x01, 0x00, 0x02, 0x0E, 0x0F, 0x10, 0x03, 0x1F, 0x04  // 16 - Cannot Roll

};


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
  //enableInterrupts();
  
  initCoCoSupport();
  
  //if (isCoCo3) {  }

  pmode(4, SCREEN);
  pcls(0);
  screen(1,1);
} 

//uint8_t highlight[] = {0x23,0x5b,0x3f,0x5b}; 
//uint8_t highlight[] = {0b100,0b1010,0b11100,0b1010}; 
uint16_t highlight[] = {0x80|0b100, 0|0b1010, 0x80|0b11100, 0|0b1010}; 

void setHighlight(int8_t player, bool isThisPlayer, uint8_t flash ) {
  // static uint8_t i;
  // if (state.drawBoard) {
  //   highlightX=player;
  //   return;
  // }
  // if (flash || highlightX==player)  
  //   return;   
  
  // for(i=0;i<2;i++) {
  //   if (highlightX>=0) {

  //     hires_Mask(SCORES_X+5+highlightX*4,4,1,19*8+1,highlight[i+(highlightX==0)*2]);
  //     hires_Mask(SCORES_X+9+highlightX*4,4,1,19*8+1,highlight[i+(highlightX==5)*2]);

  //     if (i==0) {
  //       hires_Mask(SCORES_X,2*8,29,2, 0xff); 
  //       hires_Mask(SCORES_X,9*8,29,1, 0xff); 
  //       hires_Mask(SCORES_X,12*8,29,1, 0xff);
  //     }
  //   }

  //   highlightX=player;
  // }
}

void saveScreenBuffer() {
  // memcpy((void*)0x840,(void*)0x2000,0x17C0);
  // memcpy((void*)0x4000,(void*)0x37C0,0x840);
}

void restoreScreenBuffer() {
  // memcpy((void*)0x2000,(void*)0x840,0x17C0);
  // memcpy((void*)0x37C0,(void*)0x4000,0x840);
}

void drawText(unsigned char x, unsigned char y, char* s) {
  static unsigned char c;
  y=y*8-OFFSET_Y; 
  if (y==8*(HEIGHT-1)-OFFSET_Y) {
    y=182;
  } 

  while(c=*s++) {
    if (c>=97 && c<=122) c-=32; 
    hires_putc(x++,y,ROP_CPY,c);
  }  
}

void drawChar(unsigned char x, unsigned char y, char c, unsigned char alt) {
  if (c>=97 && c<=122) c-=32;
  hires_putc(x,y*8-OFFSET_Y,alt ? ROP_BLUE : ROP_CPY,c);
}

void drawTextAlt(unsigned char x, unsigned char y, char* s) {
  static unsigned char c, mustAlt;
  static uint16_t rop;

  mustAlt = state.inGame && x>SCORES_X+5 && y<21;
 
  y=y*8-OFFSET_Y; 
  if (y==8*(HEIGHT-1)-OFFSET_Y) {
    y=182;
  } 

  while(c=*s++) {
    if (mustAlt) {
      rop = ROP_BLUE; 
    } else if (y!=21*8-OFFSET_Y  && (c<65 || c> 90)) {
      rop = ROP_BLUE; 
    } else {
      rop=ROP_CPY;
    }
    
    if (c>=97 && c<=122) c-=32; 
    // Shift lowercase
      //if (c>=0x60 && c<0x80)
        //c-=0x20;
    hires_putc(x++,y,rop,c);
  }  
}

void drawTextVert(unsigned char x, unsigned char y, char* s) {
  static unsigned char c;
  y=y*8-OFFSET_Y;
  while(c=*s++) {
    if (c>=97 && c<=122) c-=32;
    hires_putc(x,y+=8,ROP_CPY,c);
  }  
}

void resetScreen(bool forBorderScreen) { 
  if (!forBorderScreen) {
    // Clear entire screen
    pcls(0);
    //hires_Mask(0,0,40,192,0);
  } else {
    // Moving from one border screen to next - just clear non border area
     hires_Mask(3,0,WIDTH-6,24,0);
     hires_Mask(0,24,WIDTH,168,0);
     //hires_Mask(0,24,WIDTH,144,0);     
     //hires_Mask(3,168,WIDTH-6,24,0);
  }
}

void drawDie(unsigned char x, unsigned char y, unsigned char s, bool isSelected, bool isHighlighted) {
  static unsigned char i,j,rop;
  static const unsigned char *source;

  // Don't draw die if invalid index passed in
  if (!s || s>16 || y==HEIGHT-3)
    return;
    
  // Change the dice color
  rop=ROP_CPY;
  if (isHighlighted || y==0 || y==HEIGHT-3) {
   rop=ROP_BLUE;
  }
  
  // Locate the diceChar index for this die number
  source=diceChars + (s-1)*9 ; 
  
  // Is die being kept?
  if (isSelected)
    source+=54;

  // Draw the dice to the screen
  y*=8;

  // If drawing the bottom dice, offset them lower to make room
  if (y==160) {
    y=165;
  }

  
  //if (x==0) x++; else if (x==WIDTH-3) x--;
  //if (y==0) y+=4;

  for (i=0;i<3;i++) {
    for (j=0;j<3;j++) {
      hires_Draw(x+j,y,1,8,rop,&charset[*(source++)<<3]);
    }
    y+=8;
  }
}

void drawIcon(unsigned char x, unsigned char y, unsigned char icon) {
  hires_putc(x,y==HEIGHT-1 ? 182 : y*8-OFFSET_Y,ROP_CPY, icon);
}

void drawClock(unsigned char x, unsigned char y) {
  hires_putcc(x,y*8-OFFSET_Y,ROP_CPY, 0x2526);
}

void clearBelowBoard() {
  hires_Mask(0,161,40,31,0);
}

void drawBlank(unsigned char x, unsigned char y) {
  hires_putc(x,y*8-OFFSET_Y,ROP_CPY, 0x20);
}

void drawSpace(unsigned char x, unsigned char y, unsigned char w) {
  hires_Mask(x,y*8-OFFSET_Y,w,8,0);
}

void drawBoard() {
  static uint8_t y,x;

  // Vertical lines
  for (x=SCORES_X+9;x<40;x+=4) {
    hires_Mask(x,0,1,160,0x84);
  }

  // Main scores box
  drawBox(SCORES_X+5,0,23,19);
  drawBox(SCORES_X-1,2,5,17); 
 
  // Fix overlapping box corners 
  drawChar(SCORES_X-1,2, 0x24, 0);

  // // Thin horz ines
  hires_Mask(SCORES_X,9*8,29,1, 0xff); 
  hires_Mask(SCORES_X,12*8,29,1, 0xff);
  hires_Mask(0,5*8,SCORES_X-1,1, 0xff);  
  

  // // Thick horz lines
  hires_Mask(SCORES_X,2*8,29,2, 0xff); 
  hires_Mask(SCORES_X,20*8-1,29,2, 0xff); 
  

  // // Score names (16 for end game score)
  for(y = 0; y<14; y++) { 
    drawTextAlt(SCORES_X,scoreY[y],scores[y]);
  } 
  
  // // Fujitzee score text
  drawFujzee(SCORES_X,scoreY[14]);
}

void drawFujzee(unsigned char x, unsigned char y) {
  y=y*8-OFFSET_Y+1;
  hires_putcc(x,y,ROP_CPY, 0x1b1c); 
  hires_putcc(x+2,y,ROP_CPY, 0x1d1e);  
  hires_putc(x+4,y,ROP_CPY, 0x1e);
}


void drawLine(unsigned char x, unsigned char y, unsigned char w) {
  if (y==HEIGHT) {
    y=191;
  } else {
    y=y*8-OFFSET_Y+1;
  }
  hires_Mask(x,y,w,2, ROP_ORANGE); 
}

// 42653
void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h) {
  y=y*8-OFFSET_Y+1;

  // Top Corners
  hires_putc(x,y+3,ROP_ORANGE, 0x3b);
  hires_putc(x+w+1,y+3,ROP_ORANGE, 0x3c);
  
  // Accents if height > 1
  // if (h>1) {
  //   hires_putc(x+1,y+8,ROP_CPY, 1);
  // }
  
  // Top/bottom lines
  hires_Mask(x+1,y+3,w,2, ROP_ORANGE); 
  hires_Mask(x+1,y+(h+1)*8+2,w,2, ROP_ORANGE); 
  
  // Sides
  for(i=0;i<h;++i) {
    y+=8;
    hires_putc(x,y,ROP_ORANGE, 0x3f);
    hires_putc(x+w+1,y,ROP_ORANGE, 0x3f);
  }

  // Bottom Corners
  hires_putc(x,y+7,ROP_ORANGE, 0x3d);
  hires_putc(x+w+1,y+7,ROP_ORANGE, 0x3e);
}

void drawDiceCursor(unsigned char x) {
  // Top / Bottom
  hires_Mask(x,190-28,3,2, 0x55); 
  hires_Mask(x,190,3,2, 0x55);

  hires_Mask(x+1,190-28,1,2, 0x2A); 
  hires_Mask(x+1,190,1,2, 0x2A);
  
  // Sides
  hires_Mask(x-1,190-27,1,28,0x20);
  hires_Mask(x+3,190-27,1,28,0x02);
}

void hideDiceCursor(unsigned char x) {
  //drawDiceCursorInternal(x,5);
  hires_Mask(x,190-28,3,2, 0); 
  hires_Mask(x,190,3,2, 0); 
  
  // Sides
  hires_Mask(x-1,190-27,1,28,0);
  hires_Mask(x+3,190-27,1,28,0);
}

void resetGraphics() {
  screen(0,0);
}

void waitvsync() {
  static uint16_t i;
  // Aproximate a jiffy for the timer countdown
  for ( i=0;i<705;i++);
}

#endif
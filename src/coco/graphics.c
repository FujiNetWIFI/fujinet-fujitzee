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
   
extern unsigned char charset[];
#define OFFSET_Y 4

#define ROP_CPY 0xff

// Mode 4
#define ROP_BLUE 0b01010101
#define ROP_ORANGE 0b10101010
#define BOX_SIDE 0b111100

int8_t highlightX=-1;
bool inBorderScreen=false;
uint8_t box_color = ROP_ORANGE;

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
  // No color modes
  return 0;
}

void setColorMode(unsigned char mode) {
 // No-op
}

// Potentially use interrupt in future
// 0x985 - of note for disk
//#define enableInterrupts()  asm("ANDCC", "#$AF")
//#define disableInterrupts() asm("ORCC",  "#$50")
// typedef interrupt void (*ISR)(void);
// interrupt asm void irqISR(void)
// {
//     asm
//     {
// //_dskcon_irqService IMPORT
//         ldb     $FF03
//         bpl     @done           // do nothing if 63.5 us interrupt
//         ldb     $FF02           // 60 Hz interrupt. Reset PIA0, port B interrupt flag.
// @done //      lbsr    _dskcon_irqService
//         rti
//     }
// }
// void setISR(void *vector, ISR newRoutine)
// {
//     byte *isr = * (byte **) vector;
//     *isr = 0x7E;  // JMP extended
//     * (ISR *) (isr + 1) = newRoutine;
// }

void initGraphics() {
  //disableInterrupts();
  //setISR(0xFFF8, irqISR);
  //enableInterrupts(); 
  
  initCoCoSupport();
   
  //if (isCoCo3) {  }

  pmode(4, SCREEN);
  pcls(0);
  screen(1,1);
} 

// Red / Blue vertical lines
uint16_t highlight[] = {0b100000, 0b1010000};

void setHighlight(int8_t player, bool isThisPlayer, uint8_t flash ) {
  static uint8_t i;
  if (state.drawBoard) {
    highlightX=player;
    return;
  }
  if (flash)
    return;   

  waitvsync();

  // Two passes. First pass hides the previous highlight. Second pass draws the new highlight.
  for(i=0;i<2;i++) {
    if (highlightX>=0 ) {
      if (i==0) {
        if (highlightX!=player) {
          
          drawBox(SCORES_X+5+highlightX*4,0,3,19);

          if (highlightX>0)
            hires_Mask(SCORES_X+5+highlightX*4,2,1,19*8+5,highlight[i]);

          if (highlightX<5)        
            hires_Mask(SCORES_X+9+highlightX*4,2,1,19*8+5,highlight[i]);

          // Redraw horizontal board lines
          // Thin horz ines
          hires_Mask(SCORES_X,9*8,29,1, 0xff & ROP_ORANGE); 
          hires_Mask(SCORES_X,12*8,29,1, 0xff & ROP_ORANGE);
          
          // Thick horz lines
          hires_Mask(SCORES_X+6,0, 23,2, 0xff & ROP_ORANGE); 
          hires_Mask(SCORES_X,2*8,29,2, 0xff & ROP_ORANGE); 
          hires_Mask(SCORES_X,20*8-1,29,2, 0xff & ROP_ORANGE); 
        }
      } else {
        box_color=ROP_BLUE;
        drawBox(SCORES_X+5+highlightX*4,0,3,19);
        box_color=ROP_ORANGE;
      }
    }

    highlightX=player;
  }
}

bool saveScreenBuffer() {
  // No room on CoCo 32K for second page
  return false;
}

void restoreScreenBuffer() {
  // No-op on CoCo
}

void drawText(unsigned char x, unsigned char y, char* s) {
  static unsigned char c;
  if (y==255)
    y=scoreY[14]*8-OFFSET_Y+1;
  else {
    y=y*8-OFFSET_Y; 
    if (y==8*(HEIGHT-1)-OFFSET_Y) {
      y=182;
    } 
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
  } else {
    // Moving from one border screen to next - just clear non border area
     hires_Mask(3,0,WIDTH-6,24,0);
     hires_Mask(0,24,WIDTH,168,0);
  }
}

void drawDie(unsigned char x, unsigned char y, unsigned char s, bool isKept, bool isHighlighted) {
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
  if (isKept)
    source+=54;

  // Draw the dice to the screen
  y*=8;

  // If drawing the bottom dice, offset them lower to make room
  if (y==160) {
    y=165;
  }

  for (i=0;i<3;i++) {
    for (j=0;j<3;j++) {
      hires_Draw(x+j,y,1,8,rop,&charset[(uint16_t)*(source++)<<3]);
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
  for (x=SCORES_X+9;x<32;x+=4) {
    hires_Mask(x,0,1,160,0x20);
  }

  // Main scores box
  drawBox(SCORES_X+5,0,23,19);
  drawBox(SCORES_X-1,2,5,17); 

  // Thin horz ines
  hires_Mask(SCORES_X,9*8,29,1, 0xff & ROP_ORANGE); 
  hires_Mask(SCORES_X,12*8,29,1, 0xff & ROP_ORANGE);
  
  // Thick horz lines
  hires_Mask(SCORES_X,2*8,29,2, 0xff & ROP_ORANGE); 
  hires_Mask(SCORES_X,20*8-1,29,2, 0xff & ROP_ORANGE); 
  
  // Score names (16 for end game score)
  for(y = 0; y<14; y++) { 
    drawTextAlt(SCORES_X,scoreY[y],scores[y]);
  } 
  
  // Fujitzee score text
  drawFujitzee(SCORES_X,255);
}

void drawFujitzee(unsigned char x, unsigned char y) {
  char fujitzee[] = {0x1b, 0x1c, 0x1d, 0x1e, 0x1e, 0};
  drawText(x,y,&fujitzee);
}


void drawLine(unsigned char x, unsigned char y, unsigned char w) {
  if (y==HEIGHT) {
    y=191;
  } else {
    y=y*8-OFFSET_Y+1;
  }
  hires_Mask(x,y,w,2, ROP_ORANGE); 
}

void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h) {
  y=y*8-OFFSET_Y+1;

  // Top Corners
  hires_putc(x,y+3,box_color, 0x3b);
  hires_putc(x+w+1,y+3,box_color, 0x3c);
  
  // Top/bottom lines
  hires_Mask(x+1,y+3,w,2, box_color); 
  hires_Mask(x+1,y+(h+1)*8+2,w,2, box_color); 
  
  // Sides
  for(i=0;i<h;++i) {
    y+=8;
    hires_putc(x,y,box_color, 0x3f);
    hires_putc(x+w+1,y,box_color,0x3f);
  }

  // Bottom Corners
  hires_putc(x,y+7,box_color, 0x3d);
  hires_putc(x+w+1,y+7,box_color, 0x3e);
}

void drawDiceCursor(unsigned char x) {
  if (x==ROLL_X-1)
    x++;

  // Sides
  hires_Mask(x-1,190-27,1,28,0x14);
  hires_Mask(x+3,190-27,1,28,0x14);

  // Top / Bottom
  hires_Mask(x-1,190-28,5,2, ROP_BLUE); 
  hires_Mask(x-1,190,5,2, ROP_BLUE);

  // Corners
  hires_Mask(x-1,190-28,1,2, 0x0f & ROP_BLUE); 
  hires_Mask(x-1,190,1,2, 0x0f & ROP_BLUE);
  hires_Mask(x+3,190-28,1,2, 0xf0 & ROP_BLUE); 
  hires_Mask(x+3,190,1,2, 0xf0 & ROP_BLUE);
}

void hideDiceCursor(unsigned char x) {
  if (x==ROLL_X-1)
    x++;
  hires_Mask(x-1,190-28,5,2, 0); 
  hires_Mask(x-1,190,5,2, 0); 
  
  // Sides
  hires_Mask(x-1,190-27,1,28,0);
  hires_Mask(x+3,190-27,1,28,0);
}

void resetGraphics() {
  pmode(0, 0x400);
  pcls(0x60);
  screen(0,0);
  
  // Future - mount and start Lobby
}

void waitvsync() {
  static uint16_t i;
  i=getTimer();
  while (i==getTimer());
}

#endif
#ifdef _CMOC_VERSION_

#include <string.h>
#include <stdint.h>
#include <conio.h>
#include "../platform-specific/sound.h"
#include "hires.h"

static char sprite[8];
extern uint8_t charset[];

/*-----------------------------------------------------------------------*/
void hires_putc(uint8_t x, uint8_t y, uint8_t rop, uint8_t c)
{
  //static uint8_t i;
  hires_Draw(x,y,1,8,rop,&charset[(uint16_t)c<<3]);
  //memcpy(sprite, &charset[c<<3],8);
    
  //for(i=0;i<8;i++)
    //sprite[i]=sprite[i]&rop;
  //   x=0;
  //   y=0;
  // for(i=0;i<92;i++) {
    
  //   if (x==32) {
  //     x=0;
  //     y+=8;
  //   }
  //   hires_Draw(x,y,1,8,0xff,&charset[(uint16_t)i<<3]);
  //   x++;
  // }
}

/*-----------------------------------------------------------------------*/
void hires_putcc(uint8_t x, uint8_t y,uint8_t rop, uint16_t cc)
{
  hires_putc(x,y,rop,(uint8_t)cc>>8);
  hires_putc(++x,y,rop,(uint8_t)cc);
}

void hires_Mask(uint8_t x, uint8_t y, uint8_t xlen, uint8_t ylen, uint8_t c)
{
  uint8_t *pos = SCREEN+(uint16_t)y*32+x;
  ylen++;
  while (--ylen) {
    memset(pos,c,xlen);
    pos+=32;
  }
}

void hires_Draw(uint8_t x, uint8_t y, uint8_t xlen, uint8_t ylen, uint8_t rop, char *src)
{
  uint8_t *pos = SCREEN+(uint16_t)y*32+x;
  ylen++;
  while (--ylen) {
    *pos=*(src++)&rop;
    pos+=32;
  }
}

#endif
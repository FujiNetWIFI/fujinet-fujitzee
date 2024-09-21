#ifdef __APPLE2__

/**
 * @brief Text routines
 * @author Oliver Schmidt, Eric Carr
 */
#include <string.h>
#include <conio.h>
#include "hires.h"
#include "../platform-specific/sound.h"

extern unsigned char charset[];
static uint8_t sprite[8];
/*-----------------------------------------------------------------------*/
void hires_putc(unsigned char x, unsigned char y, unsigned rop, unsigned char c)
{
  static uint8_t i;

memcpy(sprite, &charset[c<<3],8);
  
 if (rop == ROP_COLORS) {
   rop = ROP_AND(0b11010101);
   if (!(x%2)) 
     for(i=0;i<8;i++)
       sprite[i]+=128;
 }

  hires_Draw(x,y,1,8,rop,sprite);
  //hires_Draw(x,y,1,8,rop,&charset[c<<3]);

}

/*-----------------------------------------------------------------------*/
void hires_putcc(unsigned char x, unsigned char y,unsigned rop, unsigned cc)
{
  hires_putc(x,y,rop,cc>>8);
  hires_putc(++x,y,rop,cc);
}

#endif /* __APPLE2__ */
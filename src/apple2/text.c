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

/*-----------------------------------------------------------------------*/
void hires_putc(unsigned char x, unsigned char y, unsigned rop, unsigned char c)
{
  if (y>191) {
    soundFujitzee();
    cgetc();
  }
    hires_Draw(x,y,1,8,rop,&charset[c<<3]);
}

/*-----------------------------------------------------------------------*/
void hires_putcc(unsigned char x, unsigned char y,unsigned rop, unsigned cc)
{
  hires_putc(x,y,rop,cc>>8);
  hires_putc(++x,y,rop,cc);
}

#endif /* __APPLE2__ */
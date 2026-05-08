#ifdef __WATCOMC__

#include <i86.h>
#include <dos.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include "../misc.h"
#include "../platform-specific/graphics.h"

extern unsigned char prevVideoMode;

static clock_t startClock;

void resetTimer()
{
    startClock = clock();
}

uint16_t getTime()
{
    return (uint16_t)((clock() - startClock) * 60 / CLOCKS_PER_SEC);
}

void quit()
{
    union REGS r;
    r.h.ah = 0x00;
    r.h.al = prevVideoMode;
    int86(0x10, &r, &r);
    exit(0);
}

void housekeeping()
{
}

uint8_t getJiffiesPerSecond()
{
    return 60;
}

#endif /* __WATCOMC__ */

#ifdef __ADAM__
/*
  Platform specific utilities - Coleco Adam

  The VDP vblank is wired to NMI on the Adam; the CRT's NMI handler calls
  jiffyTick() (installed via add_raster_int in initGraphics), which drives
  getTime()/waitvsync().
*/

#include "../misc.h"
#include "../platform-specific/util.h"
#include <eos.h>

/* The server sends the packed wire layout straight into clientState.game,
 * so sccz80 must not pad Game. Fails the build if it ever does (in which
 * case define FUJITZEE_PACK_STRUCTS - see misc.h). */
typedef char game_wire_size_check[(sizeof(Game) == 599) ? 1 : -1];

volatile uint16_t jiffyCount = 0;
volatile uint8_t vsyncFlag = 0;

static uint16_t timerBase = 0;

void jiffyTick(void)
{
    // jiffyCount advances in waitvsync() (which counts frames via either
    // this flag or the polled VDP status bit) - don't double count here.
    vsyncFlag = 1;
}

void resetTimer()
{
    timerBase = jiffyCount;
}

uint16_t getTime()
{
    return jiffyCount - timerBase;
}

uint8_t getJiffiesPerSecond()
{
    return 60;
}

void housekeeping()
{
    // Not needed on Adam
}

void quit()
{
    resetGraphics();
    eos_exit_to_smartwriter();
}

#endif /* __ADAM__ */

#ifdef __H89__

/**
 * @brief   Utility Functions - Heathkit H89
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 *
 * The H89's clock interrupts every 2 ms, and both HDOS and the CP/M BIOS
 * count the ticks in a word (console.h's TICKS). waitvsync() paces frames at
 * 60 a second, as the shared code expects of pause(): 25/3 ticks, so 8, 8,
 * then 9. The move clock counts in 50ths of a second (TICKS/10), which needs
 * no long arithmetic.
 */

#include <stdint.h>
#include <stdlib.h>
#include "console.h"
#include "../misc.h"
#include "../platform-specific/graphics.h"
#include "../platform-specific/util.h"

/* The server sends the packed wire layout straight into clientState.game,
 * so sccz80 must not pad Game. Fails the build if it ever does (in which
 * case define FUJITZEE_PACK_STRUCTS - see misc.h). */
typedef char game_wire_size_check[(sizeof(Game) == 599) ? 1 : -1];

static uint16_t lastFrame, timerBase;
static uint8_t framePhase;

void waitvsync()
{
    static uint8_t len;

    // Send what was drawn, then wait out the rest of the frame. Drawing over
    // the 9600 baud console can take longer than a frame; the game just runs
    // at the pace of the terminal then.
    flush();
    len = (framePhase == 2) ? 9 : 8;
    while ((uint16_t)(TICKS - lastFrame) < len)
        ;
    lastFrame = TICKS;
    framePhase = (framePhase == 2) ? 0 : framePhase + 1;
}

void resetTimer()
{
    timerBase = TICKS;
}

uint16_t getTime()
{
    // The tick word wraps after 131 s, longer than any move clock
    return (uint16_t)(TICKS - timerBase) / 10;
}

uint8_t getJiffiesPerSecond()
{
    return 50;
}

void housekeeping()
{
}

void quit()
{
    // There is no H89 lobby to load: back to HDOS or CP/M, with the
    // terminal as it was
    resetGraphics();
    exit(0);
}

#endif /* __H89__ */

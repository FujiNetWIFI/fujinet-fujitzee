#ifdef BUILD_ATARI7800

/**
 * @brief   Utility Functions - Atari 7800
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 */

#include <stdbool.h>
#include <stdint.h>
#include "maria.h"
#include "../misc.h"
#include "../platform-specific/graphics.h"
#include "../platform-specific/util.h"

/* The server sends the packed wire layout straight into clientState.game,
 * so cc65 must not pad Game. Fails the build if it ever does. */
typedef char game_wire_size_check[(sizeof(Game) == 599) ? 1 : -1];

/* There is no frame interrupt: the engine counts the vertical blanks it
   waits for, and the game timer counts them here. */
static uint16_t frames, timerBase;

void waitvsync(void)
{
    mt_sync();
    frames++;
}

void resetTimer(void)
{
    timerBase = frames;
}

uint16_t getTime(void)
{
    return frames - timerBase;
}

uint8_t getJiffiesPerSecond()
{
    return mt_pal ? 50 : 60;
}

void housekeeping()
{
}

// quit() is in fujinet.c, with the rest of the cartridge calls.

#endif /* BUILD_ATARI7800 */

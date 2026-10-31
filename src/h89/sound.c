#ifdef __H89__

/**
 * @brief   Sound - Heathkit H89
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 *
 * The only sound an H89 makes is the H19's bell, so it marks the moments
 * that need the player's attention -- sitting down at a table, their turn, a
 * Fujitzee, the end of the game -- and the rest stay quiet.
 */

#include <stdint.h>
#include <stdbool.h>
#include "console.h"
#include "../misc.h"
#include "../platform-specific/sound.h"

static void bell(void)
{
    if (!prefs.disableSound)
        termBell();
}

void initSound() {}
void disableKeySounds() {}
void enableKeySounds() {}
void soundStop() {}

void soundJoinGame()
{
    bell();
}

void soundMyTurn()
{
    bell();
}

void soundFujitzee()
{
    bell();
    pause(10);
    bell();
}

void soundGameDone()
{
    bell();
}

void soundRollDice() {}
void soundRollButton() {}
void soundTick() {}
void soundCursor() {}
void soundScoreCursor() {}
void soundKeep() {}
void soundRelease() {}
void soundScore() {}

#endif /* __H89__ */

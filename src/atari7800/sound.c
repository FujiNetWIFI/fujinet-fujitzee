#ifdef BUILD_ATARI7800

/**
 * @brief   Atari 7800 Sound Routines (the cartridge's POKEY)
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 *
 * The cue set is the NES port's (src/nes/sound.c), whose sound() keeps the
 * Atari client's POKEY interface: (voice, AUDF, distortion, volume). The
 * FujiNet 7800 cartridge has a POKEY at $0450 with the 8-bit Atari's 64 kHz
 * clock, so those values go to it unchanged, as src/atari/sound.c sends them
 * through cc65's _sound(). Never the TIA: its registers are INPTCTRL until
 * something locks it.
 */

#include <stdint.h>
#include <stdlib.h>
#include "../misc.h"
#include "../platform-specific/sound.h"

#define POKEY(r)   (*(volatile uint8_t *)(0x0450 + (r)))
#define AUDF(v)    POKEY((v) * 2)
#define AUDC(v)    POKEY((v) * 2 + 1)
#define AUDCTL     POKEY(0x8)
#define SKCTL      POKEY(0xF)

void soundStop()
{
    AUDC(0) = 0;
    AUDC(1) = 0;
    AUDC(2) = 0;
    AUDC(3) = 0;
}

static void sound(uint8_t voice, uint8_t frequency, uint8_t distortion, uint8_t volume)
{
    if (prefs.disableSound)
        return;

    AUDF(voice) = frequency;
    AUDC(voice) = (uint8_t)((distortion << 4) | volume);
}

static void note(uint8_t n, uint8_t n2, uint8_t n3, uint8_t d, uint8_t f, uint8_t p)
{
    static uint8_t i;

    if (prefs.disableSound)
        return;

    sound(0, n, 10, 8);
    if (n2)
        sound(1, n2, 10, 6);
    if (n3)
        sound(2, n3, 10, 4);

    pause(d);

    for (i = 7; i < 255; i--)
    {
        sound(0, n, 10, i);
        if (n2 && i > 1)
            sound(1, n2, 10, i - 2);
        if (n3 && i > 3)
            sound(2, n3, 10, i - 4);
        pause(f);
    }
    soundStop();
    pause(p);
}

void initSound()
{
    SKCTL = 0;
    SKCTL = 3;                  /* out of initialisation: the counters run */
    AUDCTL = 0;
    soundStop();
}

void soundJoinGame()
{
    static uint8_t j;
    for (j = 0; j < 2; j++)
    {
        note(81, 0, 0, 0, 1, 0);
        if (j == 0)
            note(96, 0, 0, 0, 1, 0);
    }
}

void soundFujitzee()
{
    note(76, 153, 0, 5, 0, 0);
    note(57, 230, 0, 5, 0, 0);
    note(45, 182, 0, 5, 0, 0);
    note(37, 153, 0, 5, 1, 2);
    note(45, 182, 0, 5, 0, 0);
    note(37, 153, 0, 6, 2, 0);
}

void soundMyTurn()
{
    static uint8_t i;

    sound(0, 81, 10, 5);
    pause(2);
    for (i = 7; i < 255; i--)
    {
        sound(0, 81, 10, i);
        waitvsync();
    }
    waitvsync();
    soundStop();
}

void soundGameDone()
{
    note(128, 204, 64, 6, 2, 0);
    note(96, 153, 193, 25, 2, 3);
    note(85, 144, 172, 6, 2, 0);
    note(76, 128, 153, 30, 3, 0);
}

void soundRollDice()
{
    // Distortion 8 is POKEY's noise; retriggered every roll frame
    sound(0, 150 + (rand() % 20) * 5, 8, 8);
}

void soundRollButton()
{
    sound(0, 96, 10, 5);
    pause(2);
    sound(0, 81, 10, 4);
    pause(2);
    soundStop();
}

void soundCursor()
{
    sound(0, 102, 10, 7);
    pause(1);
    soundStop();
}

void soundScoreCursor()
{
    sound(0, 91, 10, 7);
    pause(1);
    soundStop();
}

void soundKeep()
{
    static uint8_t i, j;
    j = 0;
    for (i = 200; i > 150; i -= 10)
    {
        sound(0, i, 10, 3 + j++);
        waitvsync();
    }
    soundStop();
}

void soundRelease()
{
    static uint8_t i;
    for (i = 6; i < 255; i--)
    {
        sound(0, 255 - i * 5, 10, i);
        waitvsync();
    }
    soundStop();
}

void soundTick()
{
    sound(0, 200, 8, 7);
    waitvsync();
    soundStop();
}

void soundScore()
{
    static uint8_t i, j;
    j = 0;
    for (i = 80; i > 50; i -= 10)
    {
        sound(0, i, 10, 4 + j++);
        waitvsync();
    }
    soundStop();
}

void disableKeySounds()
{
}

void enableKeySounds()
{
}

#endif /* BUILD_ATARI7800 */

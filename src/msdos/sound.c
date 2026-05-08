#ifdef __WATCOMC__

#include <dos.h>
#include <stdint.h>
#include "../misc.h"
#include "../platform-specific/sound.h"

#define PIT_CONTROL_PORT     0x43
#define PIT_CHANNEL2_PORT    0x42
#define SPEAKER_CONTROL_PORT 0x61
#define PIT_FREQUENCY        1193180UL

_WCIRTLINK extern unsigned inp(unsigned __port);
_WCIRTLINK extern unsigned outp(unsigned __port, unsigned __value);

static void wait_frames(unsigned int frames)
{
    while (frames--) {
        while (!(inp(0x3DA) & 0x08));
        while (inp(0x3DA) & 0x08);
    }
}

static void beep(unsigned int frequency, unsigned int frames, unsigned int wait)
{
    unsigned int divisor;
    unsigned char tmp;

    if (prefs.disableSound) {
        wait_frames(frames + wait);
        return;
    }

    divisor = (unsigned int)(PIT_FREQUENCY / frequency);
    outp(PIT_CONTROL_PORT, 0xB6);
    outp(PIT_CHANNEL2_PORT, divisor & 0xFF);
    outp(PIT_CHANNEL2_PORT, (divisor >> 8) & 0xFF);
    tmp = (unsigned char)inp(SPEAKER_CONTROL_PORT);
    outp(SPEAKER_CONTROL_PORT, tmp | 0x03);

    wait_frames(frames);

    tmp = (unsigned char)inp(SPEAKER_CONTROL_PORT);
    outp(SPEAKER_CONTROL_PORT, tmp & ~0x03);

    wait_frames(wait);
}

void initSound()        { }
void disableKeySounds() { }
void enableKeySounds()  { }

void soundStop()
{
    unsigned char tmp = (unsigned char)inp(SPEAKER_CONTROL_PORT);
    outp(SPEAKER_CONTROL_PORT, tmp & ~0x03);
}

/* Sound frequencies translated from the Atari POKEY divisor values used
 * in src/atari/sound.c via Hz = 63920 / (2 * (n + 1)). PC speaker is a
 * single-voice square-wave generator, so the secondary/tertiary chord
 * notes from the Atari `note(n, n2, n3, ...)` calls are dropped — only
 * the primary note is played. Beep durations approximate the Atari total
 * (d + 8*(f+1) + p) in vsync frames. */

void soundJoinGame()
{
    /* atari: note(81); note(96); note(81); */
    beep(390, 8, 0);
    beep(329, 8, 0);
    beep(390, 8, 0);
}

void soundMyTurn()
{
    /* atari: 390 Hz with ~11-frame decay */
    beep(390, 11, 0);
}

void soundFujitzee()
{
    /* atari: ascending then resolving fanfare. Total ~115 frames ≈ 1.9s */
    beep(415, 13, 0);
    beep(551, 13, 0);
    beep(695, 13, 0);
    beep(841, 23, 0);
    beep(695, 13, 0);
    beep(841, 30, 0);
}

void soundGameDone()
{
    /* atari: 4-chord triumphant resolve. Total ~174 frames ≈ 2.9s */
    beep(248, 30, 0);
    beep(329, 52, 0);
    beep(372, 30, 0);
    beep(415, 62, 0);
}

void soundRollDice()
{
    /* atari: sound(0, 150 + (rand()%20)*5, 8, 8) — random clatter */
    unsigned char n = 150 + (rand() % 20) * 5;
    beep(63920UL / (2UL * (n + 1)), 1, 0);
}

void soundRollButton()
{
    /* atari: note 96 then 81 */
    beep(329, 2, 0);
    beep(390, 2, 0);
}

void soundCursor()       { beep(310, 1, 0); }   /* atari n=102 */
void soundScoreCursor()  { beep(347, 1, 0); }   /* atari n=91  */
void soundTick()         { beep(159, 1, 0); }   /* atari n=200 */

void soundKeep()
{
    /* atari: ascending sweep i=200..160 step -10 */
    beep(159, 1, 0);   /* n=200 */
    beep(167, 1, 0);   /* n=190 */
    beep(176, 1, 0);   /* n=180 */
    beep(187, 1, 0);   /* n=170 */
    beep(198, 1, 0);   /* n=160 */
}

void soundRelease()
{
    /* atari: descending decay 225..255 */
    beep(141, 1, 0);
    beep(138, 1, 0);
    beep(135, 1, 0);
    beep(132, 1, 0);
    beep(130, 1, 0);
    beep(127, 1, 0);
    beep(125, 1, 0);
}

void soundScore()
{
    /* atari: ascending sweep i=80..60 step -10 */
    beep(395, 1, 0);   /* n=80 */
    beep(450, 1, 0);   /* n=70 */
    beep(524, 1, 0);   /* n=60 */
    beep(627, 1, 0);   /* n=50 */
}

#endif /* __WATCOMC__ */

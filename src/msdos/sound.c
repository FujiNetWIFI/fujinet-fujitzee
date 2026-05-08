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

void soundJoinGame()    { beep(440, 3, 1); beep(660, 3, 0); }
void soundMyTurn()      { beep(660, 4, 2); beep(660, 4, 0); }
void soundFujitzee()    { beep(523, 4, 0); beep(659, 4, 0); beep(784, 6, 0); beep(1047, 8, 0); }
void soundGameDone()    { beep(311, 5, 0); beep(415, 5, 0); beep(523, 10, 0); }
void soundRollDice()    { beep(150 + (rand() % 100), 1, 0); }
void soundRollButton()  { beep(660, 2, 0); beep(440, 2, 0); }
void soundTick()        { beep(200, 1, 0); }
void soundCursor()      { beep(440, 1, 0); }
void soundScoreCursor() { beep(370, 1, 0); }
void soundKeep()        { beep(523, 2, 0); beep(659, 2, 0); }
void soundRelease()     { beep(659, 2, 0); beep(523, 2, 0); }
void soundScore()       { beep(523, 2, 0); beep(659, 2, 0); beep(784, 2, 0); }

#endif /* __WATCOMC__ */

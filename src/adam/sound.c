#ifdef __ADAM__
/*
  Platform specific sound functions - Coleco Adam SN76489 (port 0xFF)

  The sound set is transposed from src/atari/sound.c. sound() keeps the
  POKEY-style (voice, frequency, distortion, volume) interface and converts:
    - frequency: POKEY 64kHz divisor f -> SN tone period N = 7*(f+1)/2
      (63921/(2*(f+1)) Hz == 3579545/(32*N) Hz)
    - volume 0-15 -> SN attenuation 15-vol
    - distortion 10 = pure tone on the requested voice; anything else runs
      the noise channel clocked by tone generator 2 so pitch sweeps carry
      over (mode 0xE7 = white noise, rate from tone 2)
*/

#include "../misc.h"
#include "../platform-specific/sound.h"
#include <stdlib.h>

#define SN_PORT 0xFF

static uint8_t noiseMode = 0;

static void snw(uint8_t b)
{
    outp(SN_PORT, b);
}

static void tone(uint8_t ch, uint16_t n)
{
    snw(0x80 | (ch << 5) | (n & 0x0F));
    snw((n >> 4) & 0x3F);
}

static void vol(uint8_t ch, uint8_t atten)
{
    snw(0x90 | (ch << 5) | atten);
}

void soundStop()
{
    snw(0x9F);
    snw(0xBF);
    snw(0xDF);
    snw(0xFF);
    noiseMode = 0;
}

static void sound(uint8_t voice, uint8_t frequency, uint8_t distortion, uint8_t volume)
{
    static uint16_t n;

    if (prefs.disableSound)
        return;

    if (!volume)
    {
        soundStop();
        return;
    }

    n = ((uint16_t)frequency + 1) * 7 / 2;
    if (n > 1023)
        n = 1023;

    if (distortion == 10)
    {
        tone(voice, n);
        vol(voice, 15 - volume);
    }
    else
    {
        // Noise pitched by tone generator 2
        tone(2, n);
        if (noiseMode != 0xE7)
        {
            noiseMode = 0xE7;
            snw(noiseMode);
        }
        vol(3, 15 - volume);
    }
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
    // Distortion 8 takes the noise path; retriggered every roll frame
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

#endif /* __ADAM__ */

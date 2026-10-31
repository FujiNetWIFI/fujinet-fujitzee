#ifdef BUILD_ATARI5200

/**
 * @brief Atari 5200 input routines
 * @author Thomas Cherryhomes
 * @license gpl v.3
 */

#include <joystick.h>
#include <fujinet-atari5200.h>
#include "vars.h"
#include "../platform-specific/input.h"
#include "../platform-specific/graphics.h"
#include "../misc.h"

/*
  Controller 1, read as CONFIG does (fujinet-config atari5200/fujiin.c): the
  analog stick from the VBI's POT shadows, the fire buttons from GTIA and
  POKEY, the keypad from the lib's IRQ latch.

  Only the stick travels in readJoystick(). The buttons and keys are
  synthesized as key codes behind kbhit()/cgetc(), as on the 7800, so a held
  button reports exactly once:

    top fire, #       RETURN (the trigger)   lower fire, 1   'h' help
    PAUSE, *          ESCAPE (menu, back)    START           'r' refresh
    RESET             'q' quit               0               's' sound
    2 4 6 8           the arrows

  While the on-screen keyboard is open (osk.c) it owns the controller, and
  kbhit() reports what it types instead.
*/

#define REG(a)     (*(volatile unsigned char *)(a))
#define PADDL0     REG(0x11)    /* stick X, 0-228, 114 at rest */
#define PADDL1     REG(0x12)    /* stick Y */
#define TRIG0      REG(0xC010)  /* top fire: 0 while held */
#define SKSTAT     REG(0xE80F)  /* bit 3 clear: the lower fire button */

/* Past these the stick is pushed; between them it is at rest. */
#define STICK_LO   0x40
#define STICK_HI   0xA4

extern unsigned char oskActive;   /* osk.c */
unsigned char oskPoll(void);      /* osk.c */

static unsigned char lastButtons;
static unsigned char pendingKey;

unsigned char readStick(void)
{
    unsigned char x = PADDL0, y = PADDL1, v = 0;

    if (y < STICK_LO)
        v |= JOY_UP_MASK;
    else if (y > STICK_HI)
        v |= JOY_DOWN_MASK;
    if (x < STICK_LO)
        v |= JOY_LEFT_MASK;
    else if (x > STICK_HI)
        v |= JOY_RIGHT_MASK;
    return v;
}

unsigned char readButtons(void)
{
    unsigned char b = 0;

    if (TRIG0 == 0)
        b |= BTN_FIRE;
    if (!(SKSTAT & 0x08))
        b |= BTN_2;
    return b;
}

unsigned char readKeypad(void)
{
    unsigned char k = fuji_a5200_key;

    if (k)
        fuji_a5200_key = 0;
    return k;
}

unsigned char readJoystick()
{
    return oskActive ? 0 : readStick();
}

static unsigned char poll(void)
{
    unsigned char buttons = readButtons();
    unsigned char pressed = buttons & ~lastButtons;

    lastButtons = buttons;

    if (pressed & BTN_FIRE)
        return KEY_RETURN;
    if (pressed & BTN_2)
        return 'h';

    switch (readKeypad())
    {
    case FUJI_A5200_KEY_HASH:     return KEY_RETURN;
    case FUJI_A5200_KEY_DIGIT(1): return 'h';
    case FUJI_A5200_KEY_STAR:
    case FUJI_A5200_KEY_PAUSE:    return KEY_ESCAPE;
    case FUJI_A5200_KEY_START:    return 'r';
    case FUJI_A5200_KEY_RESET:    return 'q';
    case FUJI_A5200_KEY_DIGIT(0): return 's';
    case FUJI_A5200_KEY_DIGIT(2): return KEY_UP_ARROW;
    case FUJI_A5200_KEY_DIGIT(8): return KEY_DOWN_ARROW;
    case FUJI_A5200_KEY_DIGIT(4): return KEY_LEFT_ARROW;
    case FUJI_A5200_KEY_DIGIT(6): return KEY_RIGHT_ARROW;
    }
    return 0;
}

unsigned char kbhit(void)
{
    if (!pendingKey)
    {
        if (oskActive)
        {
            pendingKey = oskPoll();
            // Whatever the grid used up is not a press once it closes.
            lastButtons = readButtons();
        }
        else
            pendingKey = poll();
    }

    return pendingKey != 0;
}

char cgetc(void)
{
    unsigned char k;

    while (!kbhit())
        waitvsync();

    k = pendingKey;
    pendingKey = 0;
    return k;
}

#endif /* BUILD_ATARI5200 */

#ifdef BUILD_ATARI7800

/**
 * @brief Atari 7800 input routines
 * @author Thomas Cherryhomes
 * @license gpl v.3
 */

#include <joystick.h>
#include "vars.h"
#include "../platform-specific/input.h"
#include "../platform-specific/graphics.h"
#include "../misc.h"

/*
  The left joystick and the console switches, read from the TIA and RIOT as
  CONFIG does (fujinet-config atari7800/fujiin.c): only RIOT is ever written.
  A ProLine joystick is read in two-button mode; a 2600 joystick's one button
  arrives through INPT4 as button 1.

  Only the stick travels in readJoystick(). The buttons and switches are
  synthesized as key codes behind kbhit()/cgetc() -- the surface the keyboard
  platforms present -- so a held button reports exactly once. Button 1 is
  RETURN, which the shared readCommonInput() maps to input.trigger and which
  also satisfies the screens that block on cgetc() ("press any key").

  SELECT is a shift key, as on the NES client, because the shared screens
  want more letter shortcuts than there are buttons (vars.h's *_TEXT labels
  say which):

    FIRE (1)     RETURN    2            'h' help     PAUSE        ESCAPE
    RESET        'q' quit  SELECT+2     's' sound
    SELECT alone 'r' refresh, reported when it is let go without having
                 shifted anything -- the only way to tell it from a chord.

  While the on-screen keyboard is open (osk.c) it owns the joystick, and
  kbhit() reports what it types instead.
*/

#define REG(a)     (*(volatile unsigned char *)(a))
#define INPT0      REG(0x08)    /* button 2, two-button mode: bit 7 set */
#define INPT1      REG(0x09)    /* button 1, two-button mode: bit 7 set */
#define INPT4      REG(0x0C)    /* a 2600 joystick's button: bit 7 clear */
#define SWCHA      REG(0x280)   /* bits 7-4: right, left, down, up; low */
#define SWCHB      REG(0x282)   /* bit 0 RESET, 1 SELECT, 3 PAUSE; low */
#define CTLSWB     REG(0x283)

#define PB2        0x04         /* left port's two-button mode, driven low */

#define DIRS (JOY_UP_MASK | JOY_DOWN_MASK | JOY_LEFT_MASK | JOY_RIGHT_MASK)

extern unsigned char oskActive;   /* osk.c */
unsigned char oskPoll(void);      /* osk.c */

static unsigned char lastButtons;
static unsigned char shifted;
static unsigned char pendingKey;

void initJoystick(void)
{
    CTLSWB = PB2;
    SWCHB = 0;
}

unsigned char readStick(void)
{
    return (unsigned char)~SWCHA & DIRS;
}

unsigned char readButtons(void)
{
    unsigned char b = 0, sw = (unsigned char)~SWCHB;

    if ((INPT1 & 0x80) || !(INPT4 & 0x80))
        b |= BTN_FIRE;
    if (INPT0 & 0x80)
        b |= BTN_2;
    if (sw & 0x01)
        b |= BTN_RESET;
    if (sw & 0x02)
        b |= BTN_SELECT;
    if (sw & 0x08)
        b |= BTN_PAUSE;
    return b;
}

unsigned char readJoystick()
{
    return oskActive ? 0 : readStick();
}

static unsigned char poll(void)
{
    unsigned char buttons = readButtons();
    unsigned char pressed = buttons & ~lastButtons;
    unsigned char released = lastButtons & ~buttons;
    unsigned char sel = buttons & BTN_SELECT;

    lastButtons = buttons;

    if (pressed & BTN_SELECT)
        shifted = 0;

    if (pressed & BTN_FIRE)
    {
        shifted = 1;
        return KEY_RETURN;
    }
    if (pressed & BTN_2)
    {
        shifted = 1;
        return sel ? 's' : 'h';
    }
    if (pressed & BTN_PAUSE)
    {
        shifted = 1;
        return KEY_ESCAPE;
    }
    if (pressed & BTN_RESET)
    {
        shifted = 1;
        return 'q';
    }
    if ((released & BTN_SELECT) && !shifted)
        return 'r';

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
            shifted = 1;
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

#endif /* BUILD_ATARI7800 */

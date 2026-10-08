#ifdef BUILD_SMS

/**
 * @brief Sega Master System input routines
 * @author Thomas Cherryhomes
 * @license gpl v.3
 */

// vars.h first: its joystick.h defines JOY_UP() and friends, which
// <arch/sms.h> then leaves alone (it defines them only if they are missing).
#include "vars.h"
#include <arch/sms.h>
#include <interrupt.h>
#include "../platform-specific/input.h"
#include "../platform-specific/graphics.h"

/*
  A Master System pad is a d-pad and buttons 1 and 2; Pause is on the console
  and arrives as an NMI that toggles crt0's pause_flag. The reader is
  fujinet-battleship's src/sms/input.c.

  Only the d-pad travels in readJoystick(). The buttons are synthesized as key
  codes behind kbhit()/cgetc() -- the surface the keyboard platforms present --
  the way the NES port reports its buttons, so a held button reports exactly
  once. Button 1 is KEY_RETURN, which the shared readCommonInput() maps to
  input.trigger and which also satisfies the screens that block on cgetc().

  Button 2 is a shift key, because the shared screens want more letter
  shortcuts than there are buttons (vars.h's *_TEXT labels say which):

    1          RETURN    Pause       ESCAPE
    2+1        'q'       2+Up        'h'
    2+Down     's'
    2 alone    'r', reported when it is let go without having shifted
               anything -- that is the only way to tell it from a chord.

  While 2 is held the d-pad does not move anything, and a direction used in a
  chord stays out of readJoystick() until it is let go, so finishing a chord
  never nudges the cursor.

  Pause (crt0's NMI) and both buttons (pad.asm, on the frame interrupt) are
  latched, so a tap is not lost to a network call. While the on-screen
  keyboard is open (osk.c) it owns the pad, and kbhit() reports what it types
  instead.
*/

#define DIRS JOY_DIRS_MASK
#define B1 JOY_BTN_1_MASK
#define B2 JOY_BTN_2_MASK

extern void padIrq(void);               /* pad.asm */
extern volatile unsigned char padSeen;

extern unsigned char oskActive;         /* osk.c */
unsigned char oskPoll(void);            /* osk.c */

static unsigned char lastPad;
static unsigned char lastPause;
static unsigned char shifted;
static unsigned char chordDirs;
static unsigned char pendingKey;
static unsigned char installed;

static unsigned char readPad(void)
{
    return (unsigned char)~IO_DC & (DIRS | B1 | B2);
}

static void install(void)
{
    // The handler list is read by the interrupt; add the entry with it off.
#asm
    di
#endasm
    add_raster_int(padIrq);
#asm
    ei
#endasm
    installed = 1;
    lastPad = readPad();
    lastPause = pause_flag;
    padSeen = 0;
}

unsigned char readJoystick()
{
    static unsigned char pad, dirs;

    if (oskActive)
        return 0;

    pad = readPad();
    dirs = pad & DIRS;

    chordDirs &= dirs;
    if (pad & B2)
        return 0;
    return dirs & ~chordDirs;
}

static unsigned char seen;

/* Take what the frame interrupt has latched, and clear it, as one step. */
static void takeLatched(void)
{
#asm
    di
#endasm
    seen = padSeen;
    padSeen = 0;
#asm
    ei
#endasm
}

static unsigned char poll(void)
{
    static unsigned char live, held, pressed, key;

    if (!installed)
        install();

    takeLatched();
    live = readPad();
    held = live | seen;                 // down now, or at some frame since
    pressed = held & ~lastPad;
    key = 0;

    if (pressed & B2)
        shifted = 0;                    // a new hold of 2 starts clean

    if (pause_flag != lastPause)
    {
        lastPause = pause_flag;
        key = KEY_ESCAPE;
    }
    else if (pressed & B1)
    {
        if (held & B2)
        {
            shifted = 1;
            key = 'q';
        }
        else
            key = KEY_RETURN;
    }
    else if ((held & B2) && (pressed & DIRS))
    {
        // 2+Left/Right is no shortcut here, but it is still a chord: it
        // must not move the cursor, nor turn 2's release into a refresh.
        shifted = 1;
        chordDirs |= pressed & DIRS;
        if (pressed & JOY_UP_MASK)
            key = 'h';
        else if (pressed & JOY_DOWN_MASK)
            key = 's';
    }
    else if (((lastPad | seen) & B2) && !(live & B2))
    {
        // 2 let go (or tapped and let go between two polls)
        if (!shifted)
            key = 'r';
        shifted = 0;
    }

    lastPad = live;
    return key;
}

/* Forget anything held or latched: whatever the on-screen keyboard used up
   is not a press once it hands the pad back. */
static void resync(void)
{
    if (!installed)
        install();
    takeLatched();
    lastPad = readPad();
    lastPause = pause_flag;
    shifted = 1;
}

unsigned char kbhit(void)
{
    if (!pendingKey)
    {
        if (oskActive)
        {
            pendingKey = oskPoll();
            resync();
        }
        else
            pendingKey = poll();
    }

    return pendingKey != 0;
}

unsigned char cgetc(void)
{
    static unsigned char k;

    while (!kbhit())
        waitvsync();

    k = pendingKey;
    pendingKey = 0;
    return k;
}

#endif /* BUILD_SMS */

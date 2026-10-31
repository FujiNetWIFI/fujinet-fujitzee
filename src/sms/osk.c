#ifdef BUILD_SMS

/**
 * @brief   Sega Master System on-screen keyboard
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 */

// vars.h first: its joystick.h masks are the ones the pad is read with.
#include "vars.h"
#include <arch/sms.h>
#include "../platform-specific/graphics.h"
#include "../platform-specific/sound.h"

/*
  A Master System pad has no keys at all, so the player's name is typed on
  screen. This is src/nes/osk.c: a KEY SOURCE rather than an editor. While it
  is open, input.c's kbhit() asks oskPoll() for a key, and the shared
  inputFieldCycle() does the editing, the cursor chip and the minimum-length
  check exactly as it does on a keyboard machine.

  1 types the cell under the cursor, 2 backspaces and Pause accepts (as does
  the OK cell). The cursor position IS the character, so there is no shift
  key and no paging. There is no cancel: the name screen only opens when a
  name is required.

  24 rows leave no room for the NES's blank line between grid rows: the grid
  is rows 18-21, under the "at least 2 letters" hint at row 16, and the
  button legend is the status row.
*/

#define OSK_COLS 10
#define OSK_ROWS 4
#define OSK_PITCH 3 /* marker column + two label columns */
#define OSK_X 1
#define OSK_Y 18

/* The last row is one short: its tenth cell does not exist. */
#define OSK_LAST_ROW_COLS 9

#define CELL_SPACE (OSK_COLS * 3 + 6) /* index 36 */
#define CELL_DEL (CELL_SPACE + 1)
#define CELL_OK (CELL_SPACE + 2)

static const char cells[] =
    "ABCDEFGHIJ"
    "KLMNOPQRST"
    "UVWXYZ0123"
    "456789";

#define DIRS JOY_DIRS_MASK
#define B1 JOY_BTN_1_MASK
#define B2 JOY_BTN_2_MASK

#define REPEAT_FIRST 18 /* frames before a held stick repeats */
#define REPEAT_NEXT 5

// crt0's frame counter (src/sms/util.c reads it the same way)
extern volatile unsigned int timer;

unsigned char oskActive;

static unsigned char curX, curY, hold, lastDir, lastButtons, lastPause, lastFrame;

static unsigned char readPad(void)
{
    return (unsigned char)~IO_DC & (DIRS | B1 | B2);
}

static unsigned char cellCols(unsigned char row)
{
    return row == OSK_ROWS - 1 ? OSK_LAST_ROW_COLS : OSK_COLS;
}

static void drawCell(unsigned char col, unsigned char row, unsigned char marked)
{
    static unsigned char idx, x, y;
    static char glyph[3];

    idx = row * OSK_COLS + col;
    x = OSK_X + col * OSK_PITCH;
    y = OSK_Y + row;

    if (marked)
        drawIcon(x, y, ICON_MARK);
    else
        drawBlank(x, y);

    if (idx == CELL_SPACE)
        drawTextAlt(x + 1, y, "sp");
    else if (idx == CELL_DEL)
        drawTextAlt(x + 1, y, "dl");
    else if (idx == CELL_OK)
        drawTextAlt(x + 1, y, "ok");
    else
    {
        glyph[0] = cells[idx];
        glyph[1] = ' ';
        glyph[2] = 0;
        drawText(x + 1, y, glyph);
    }
}

void oskOpen(void)
{
    static unsigned char row, col, pad;

    curX = curY = 0;
    for (row = 0; row < OSK_ROWS; row++)
        for (col = 0; col < cellCols(row); col++)
            drawCell(col, row, row == curY && col == curX);

    drawTextAlt(WIDTH / 2 - 12, HEIGHT - 1, "1:TYPE 2:DELETE PAUSE:OK");

    // Whatever is already held (the 1 that opened this screen) is not a press.
    pad = readPad();
    lastButtons = pad & ~DIRS;
    lastDir = pad & DIRS;
    lastPause = pause_flag;
    hold = REPEAT_FIRST;
    lastFrame = (unsigned char)timer;
    oskActive = 1;
}

void oskClose(void)
{
    static unsigned char row;

    oskActive = 0;
    for (row = 0; row < OSK_ROWS; row++)
        drawSpace(0, OSK_Y + row, WIDTH);
    drawSpace(0, HEIGHT - 1, WIDTH);
}

/* One key for inputFieldCycle(), or 0. inputFieldCycle() spins on kbhit()
   without waiting for vblank, so the pad is read once a frame here: that is
   what auto-repeat and the button edges are counted in. */
unsigned char oskPoll(void)
{
    static unsigned char pad, dir, move, pressed, idx;

    if ((unsigned char)timer == lastFrame)
        return 0;
    lastFrame = (unsigned char)timer;

    pad = readPad();
    dir = pad & DIRS;
    pressed = pad & ~DIRS & ~lastButtons;
    lastButtons = pad & ~DIRS;

    move = 0;
    if (dir != lastDir)
    {
        lastDir = dir;
        hold = REPEAT_FIRST;
        move = dir;
    }
    else if (dir && --hold == 0)
    {
        hold = REPEAT_NEXT;
        move = dir;
    }

    if (move)
    {
        drawCell(curX, curY, 0);

        if ((move & JOY_LEFT_MASK) && curX)
            curX--;
        else if ((move & JOY_RIGHT_MASK) && curX + 1 < cellCols(curY))
            curX++;
        else if ((move & JOY_UP_MASK) && curY)
            curY--;
        else if ((move & JOY_DOWN_MASK) && curY + 1 < OSK_ROWS)
            curY++;

        if (curX >= cellCols(curY))
            curX = cellCols(curY) - 1;

        drawCell(curX, curY, 1);
        soundCursor();
    }

    /* 1 takes whatever the cursor is sitting on; 2 and Pause are shortcuts
       for the backspace and OK cells. */
    if (pause_flag != lastPause)
    {
        lastPause = pause_flag;
        idx = CELL_OK;
    }
    else if (pressed & B2)
        idx = CELL_DEL;
    else if (pressed & B1)
        idx = curY * OSK_COLS + curX;
    else
        return 0;

    if (idx == CELL_OK)
        return KEY_RETURN;
    if (idx == CELL_DEL)
        return KEY_BACKSPACE;

    soundCursor();
    return idx == CELL_SPACE ? KEY_SPACEBAR : cells[idx];
}

#endif /* BUILD_SMS */

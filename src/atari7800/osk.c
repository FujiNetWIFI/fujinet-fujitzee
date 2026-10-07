#ifdef BUILD_ATARI7800

/**
 * @brief   Atari 7800 on-screen keyboard
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 */

#include <joystick.h>
#include "vars.h"
#include "../platform-specific/graphics.h"
#include "../platform-specific/sound.h"

/*
  The NES client's key source (src/nes/osk.c) on a 13 x 3 grid, not an
  editor: while it is open, input.c's kbhit() asks oskPoll() for a key, and
  the shared inputFieldCycle() does the editing, the cursor chip and the
  minimum-length check exactly as it does on a keyboard machine.

  FIRE types the cell under the cursor, button 2 backspaces and PAUSE accepts
  (as does the OK cell). The cursor position IS the character, so there is no
  shift key and no paging. There is no cancel: the name screen only opens
  when a name is required.
*/

#define OSK_COLS 13
#define OSK_ROWS 3
#define OSK_X 2
#define OSK_Y 18       /* a row clear of the "at least 2 letters" hint at row 16 */
#define OSK_ROW_STEP 2 /* a blank line between rows, or the grid reads as columns */
#define OSK_LEGEND_Y 13 /* under the name box: 24 rows leave no gap above HEIGHT-1 */

/* A cell is a marker column and its character; the last row's SP, DL and OK
   take a column more each. */
#define CELL_SPACE (OSK_COLS * 2 + 10) /* index 36 */
#define CELL_DEL (CELL_SPACE + 1)
#define CELL_OK (CELL_SPACE + 2)

static const char cells[] =
    "ABCDEFGHIJKLM"
    "NOPQRSTUVWXYZ"
    "0123456789";

#define REPEAT_FIRST 18 /* frames before a held stick repeats */
#define REPEAT_NEXT 5

unsigned char oskActive;

static unsigned char curX, curY, hold, lastDir, lastButtons;

static void drawCell(unsigned char col, unsigned char row, unsigned char marked)
{
    unsigned char idx = row * OSK_COLS + col;
    unsigned char x = OSK_X + col * 2;
    unsigned char y = OSK_Y + row * OSK_ROW_STEP;
    char glyph[2];

    if (idx > CELL_SPACE)
        x += idx - CELL_SPACE;

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
        glyph[1] = 0;
        drawText(x + 1, y, glyph);
    }
}

void oskOpen(void)
{
    unsigned char row, col;

    curX = curY = 0;
    for (row = 0; row < OSK_ROWS; row++)
        for (col = 0; col < OSK_COLS; col++)
            drawCell(col, row, row == curY && col == curX);

    drawTextAlt(WIDTH / 2 - 13, OSK_LEGEND_Y, "FIRE:TYPE 2:DELETE PAUSE:OK");

    // Whatever is already held (the FIRE that opened this screen) is not a
    // press.
    lastButtons = readButtons();
    lastDir = readStick();
    hold = REPEAT_FIRST;
    oskActive = 1;
}

void oskClose(void)
{
    unsigned char row;

    oskActive = 0;
    for (row = 0; row < OSK_ROWS; row++)
        drawSpace(0, OSK_Y + row * OSK_ROW_STEP, WIDTH);
    drawSpace(0, OSK_LEGEND_Y, WIDTH);
}

/* One key for inputFieldCycle(), or 0. inputFieldCycle() spins on kbhit(),
   so this waits for the next frame first: auto-repeat and the button edges
   are counted in frames. */
unsigned char oskPoll(void)
{
    unsigned char dir, move, buttons, pressed, idx;

    waitvsync();

    dir = readStick();
    buttons = readButtons();
    pressed = buttons & ~lastButtons;
    lastButtons = buttons;

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
        else if ((move & JOY_RIGHT_MASK) && curX + 1 < OSK_COLS)
            curX++;
        else if ((move & JOY_UP_MASK) && curY)
            curY--;
        else if ((move & JOY_DOWN_MASK) && curY + 1 < OSK_ROWS)
            curY++;

        drawCell(curX, curY, 1);
        soundCursor();
    }

    /* FIRE takes whatever the cursor is sitting on; button 2 and PAUSE are
       shortcuts for the backspace and OK cells. */
    if (pressed & BTN_2)
        idx = CELL_DEL;
    else if (pressed & BTN_PAUSE)
        idx = CELL_OK;
    else if (pressed & BTN_FIRE)
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

#endif /* BUILD_ATARI7800 */

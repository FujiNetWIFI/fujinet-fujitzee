#ifdef BUILD_ATARI7800

/*
  Graphics functionality - Atari 7800 MARIA

  The screen is the MARIA text and tile engine (maria.s, see maria.h): 32x24
  cells, each a tile and one of two palettes, redrawn by MARIA from the map
  every line, so a cell costs one store. The art is the MS-DOS client's CGA
  sheet, converted by src/atari7800/mkchr.py, and the drawing is the NES
  client's (src/nes/graphics.c) on the Adam's 24 rows.

  Palette 0 is the CGA set less blue, which is the background; palette 1 is
  the MS-DOS MASK_ALT, so a cyan glyph is the white one in palette 1. Two
  things the MS-DOS client does pixel by pixel are done with tiles instead,
  as on the Adam:
    - the active player's column: its two dividers turn white (hlTo[]).
    - the dice cursor: a checkerboard bar in the gap column either side of
      the die.
*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "vars.h"
#include "tiles.h"
#include "maria.h"
#include "../misc.h"

/* MARIA colours, hue << 4 | luminance. A PAL console's hues sit one higher. */
#define C_BLACK    0x00
#define C_WHITE    0x0F
#define C_BLUE     0x82
#define C_CYAN     0xA8
#define PAL_HUE    0x10

#define BOARD_ROWS 21   /* the board's rows: the outline runs down all of them */
#define CURSOR_ROW (HEIGHT - 4)

unsigned char colorMode = 0;

/* What saveScreenBuffer() keeps: the engine's map (tile * 2) and palettes. */
static unsigned char savedMap[MT_ROWS * MT_COLS];
static unsigned char savedAttr[MT_ROWS * MT_COLS];

/* Set by drawBoard: the logo's first cell carries the score-box border, and
   clearBelowBoard() puts the bottom rule back. */
static bool boardDrawn, savedBoardDrawn;

static int8_t highlightX = -1, savedHighlightX = -1;
static unsigned char hlCol;

static bool cursorActive, savedCursorActive;
static unsigned char cursorX, savedCursorX;
static unsigned char cursorSave[6], savedCursorSave[6];

/* Atari-derived dice/roll-button tile layout: the MS-DOS client's table,
   whose glyph numbers sheetTile[] maps. */
static const unsigned char diceChars[] = {
    /* Normal (white) dice */
    0x41,0x40,0x42, 0x40,0x45,0x40, 0x43,0x40,0x44,
    0x41,0x40,0x47, 0x40,0x40,0x40, 0x48,0x40,0x44,
    0x41,0x40,0x47, 0x40,0x45,0x40, 0x48,0x40,0x44,
    0x46,0x40,0x47, 0x40,0x40,0x40, 0x48,0x40,0x49,
    0x46,0x40,0x47, 0x40,0x45,0x40, 0x48,0x40,0x49,
    0x46,0x40,0x47, 0x4A,0x40,0x4B, 0x48,0x40,0x49,

    /* Kept (selected/locked) dice */
    0x21,0x4C,0x22, 0x20,0x25,0x20, 0x23,0x4D,0x24,
    0x21,0x4C,0x27, 0x20,0x20,0x20, 0x28,0x4D,0x24,
    0x21,0x4C,0x27, 0x20,0x25,0x20, 0x28,0x4D,0x24,
    0x26,0x4C,0x27, 0x20,0x20,0x20, 0x28,0x4D,0x29,
    0x26,0x4C,0x27, 0x20,0x25,0x20, 0x28,0x4D,0x29,
    0x26,0x4C,0x27, 0x2A,0x20,0x2B, 0x28,0x4D,0x29,

    /* 13 - empty space (Roll button background) */
    0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00,

    /* 14 - "Roll" button: 1 roll left */
    0x41,0x40,0x42, 0x2C,0x2D,0x2E, 0x43,0x30,0x44,
    /* 15 - "Roll" button: 2 rolls left */
    0x41,0x40,0x42, 0x2C,0x2D,0x2E, 0x43,0x2F,0x44,
    /* 16 - "Roll" button: cannot roll */
    0x41,0x40,0x42, 0x2C,0x2D,0x2E, 0x43,0x50,0x44,

    /* 17-19 - Roll button pushed (alt color) */
    0xC1,0xC0,0xC2, 0xAC,0xAD,0xAE, 0xC3,0xB0,0xC4,
    0xC1,0xC0,0xC2, 0xAC,0xAD,0xAE, 0xC3,0xAF,0xC4,
    0xC1,0xC0,0xC2, 0xAC,0xAD,0xAE, 0xC3,0xD0,0xC4,

    /* Highlighted dice (alt color) */
    0xC1,0xC0,0xC2, 0xC0,0xC5,0xC0, 0xC3,0xC0,0xC4,
    0xC1,0xC0,0xC7, 0xC0,0xC0,0xC0, 0xC8,0xC0,0xC4,
    0xC1,0xC0,0xC7, 0xC0,0xC5,0xC0, 0xC8,0xC0,0xC4,
    0xC6,0xC0,0xC7, 0xC0,0xC0,0xC0, 0xC8,0xC0,0xC9,
    0xC6,0xC0,0xC7, 0xC0,0xC5,0xC0, 0xC8,0xC0,0xC9,
    0xC6,0xC0,0xC7, 0xCA,0xC0,0xCB, 0xC8,0xC0,0xC9,

    /* Highlighted kept dice (alt color) */
    0xA1,0xCC,0xA2, 0xA0,0xA5,0xA0, 0xA3,0xCD,0xA4,
    0xA1,0xCC,0xA7, 0xA0,0xA0,0xA0, 0xA8,0xCD,0xA4,
    0xA1,0xCC,0xA7, 0xA0,0xA5,0xA0, 0xA8,0xCD,0xA4,
    0xA6,0xCC,0xA7, 0xA0,0xA0,0xA0, 0xA8,0xCD,0xA9,
    0xA6,0xCC,0xA7, 0xA0,0xA5,0xA0, 0xA8,0xCD,0xA9,
    0xA6,0xCC,0xA7, 0xAA,0xA0,0xAB, 0xA8,0xCD,0xA9
};

/* A board piece in one of the outline's columns, in white; else t. */
static unsigned char outlined(unsigned char t)
{
    unsigned char i;

    for (i = 0; i < HL_PIECES; i++)
        if (hlFrom[i] == t)
            return hlTo[i];
    return t;
}

static unsigned char plain(unsigned char t)
{
    unsigned char i;

    for (i = 0; i < HL_PIECES; i++)
        if (hlTo[i] == t)
            return hlFrom[i];
    return t;
}

/**
 * @brief put tile t (tiles.h numbering: 0x80 is palette 1) at column x, row y
 */
static void put(unsigned char x, unsigned char y, unsigned char t)
{
    unsigned char pal;

    if (x >= WIDTH || y >= HEIGHT)
        return;

    if (highlightX >= 0 && y < BOARD_ROWS && (x == hlCol || x == hlCol + 4))
        t = outlined(t);

    pal = t >> 7;
    mt_at(x, y);
    mt_put(t & 0x7F);
    if (mt_attr[y * MT_COLS + x] != pal)
    {
        mt_at(x, y);
        mt_setpal(pal, 1);
    }
}

/* The tile at (x, y), with its palette bit. */
static unsigned char get(unsigned char x, unsigned char y)
{
    return mt_get(x, y) | (mt_attr[y * MT_COLS + x] << 7);
}

/**
 * @brief plot MS-DOS glyph g, as MS-DOS plot_glyph() with MASK_WHITE
 */
static void glyph(unsigned char x, unsigned char y, unsigned char g)
{
    put(x, y, sheetTile[g]);
}

/* ---- Active player column: white dividers ---- */

/* Player p's column is cells SCORES_X+6+p*4 .. +2, between the dividers at
   SCORES_X+5+p*4 and four to the right of it. */
static void outlineColumn(bool on)
{
    unsigned char y, i, x, t;

    for (i = 0; i < 2; i++)
    {
        x = hlCol + i * 4;
        for (y = 0; y < BOARD_ROWS; y++)
        {
            t = mt_get(x, y);
            t = on ? outlined(t) : plain(t);
            if (t != mt_get(x, y))
            {
                mt_at(x, y);
                mt_put(t);
            }
        }
    }
}

void setHighlight(int8_t player, bool isThisPlayer, uint8_t flash)
{
    (void)isThisPlayer;
    (void)flash;

    if (highlightX == player)
        return;

    if (highlightX >= 0)
        outlineColumn(false);
    highlightX = player;
    if (player >= 0)
    {
        hlCol = SCORES_X + 5 + player * 4;
        outlineColumn(true);
    }
}

/* ---- Dice cursor: bars in the gap columns either side of the die ---- */

void drawDiceCursor(unsigned char x)
{
    unsigned char r;

    // The roll button sits one column right of where gamelogic.c aims.
    if (x == ROLL_X - 1)
        x++;

    if (cursorActive)
        hideDiceCursor(cursorX);
    cursorX = x;

    for (r = 0; r < 3; r++)
    {
        cursorSave[r] = get(x - 1, CURSOR_ROW + r);
        cursorSave[r + 3] = get(x + 3, CURSOR_ROW + r);
        put(x - 1, CURSOR_ROW + r, T_CURSOR_L);
        put(x + 3, CURSOR_ROW + r, T_CURSOR_R);
    }
    cursorActive = true;
}

void hideDiceCursor(unsigned char x)
{
    unsigned char r;

    (void)x;
    if (!cursorActive)
        return;

    for (r = 0; r < 3; r++)
    {
        put(cursorX - 1, CURSOR_ROW + r, cursorSave[r]);
        put(cursorX + 3, CURSOR_ROW + r, cursorSave[r + 3]);
    }
    cursorActive = false;
}

/* ---- Screen ---- */

uint8_t cycleNextColor()
{
    return 0;
}

void setColorMode()
{
}

void initGraphics()
{
    unsigned char hue;

    initJoystick();
    mt_init(T_BLANK);
    hue = mt_pal ? PAL_HUE : 0;
    mt_background((unsigned char)(C_BLUE + hue));
    mt_palette(0, C_BLACK, (unsigned char)(C_CYAN + hue), C_WHITE);
    mt_palette(1, (unsigned char)(C_BLUE + hue), (unsigned char)(C_CYAN + hue),
               (unsigned char)(C_CYAN + hue));
}

void resetGraphics()
{
}

void resetScreen(bool forBorderScreen)
{
    unsigned char y;

    cursorActive = false;
    highlightX = -1;
    boardDrawn = false;

    if (!forBorderScreen)
    {
        mt_clear(T_BLANK);
        return;
    }

    // Moving between bordered screens: clear everything except the top
    // corner dice, the only ones drawDie() draws on 24 rows.
    for (y = 0; y < HEIGHT; y++)
    {
        if (y < 3)
        {
            mt_at(3, y);
            mt_fill(T_BLANK, WIDTH - 6);
            mt_at(3, y);
            mt_setpal(0, WIDTH - 6);
        }
        else
        {
            mt_at(0, y);
            mt_fill(T_BLANK, WIDTH);
            mt_at(0, y);
            mt_setpal(0, WIDTH);
        }
    }
}

bool saveScreenBuffer()
{
    memcpy(savedMap, mt_map, sizeof savedMap);
    memcpy(savedAttr, mt_attr, sizeof savedAttr);
    savedHighlightX = highlightX;
    savedBoardDrawn = boardDrawn;
    savedCursorActive = cursorActive;
    savedCursorX = cursorX;
    memcpy(savedCursorSave, cursorSave, sizeof cursorSave);
    return true;
}

void restoreScreenBuffer()
{
    unsigned char x, y;
    unsigned int i = 0;

    // The saved cells already carry their outline and the dice cursor.
    highlightX = -1;
    for (y = 0; y < HEIGHT; y++)
        for (x = 0; x < WIDTH; x++, i++)
            put(x, y, (savedMap[i] >> 1) | (savedAttr[i] << 7));
    highlightX = savedHighlightX;
    if (highlightX >= 0)
        hlCol = SCORES_X + 5 + highlightX * 4;
    boardDrawn = savedBoardDrawn;
    cursorActive = savedCursorActive;
    cursorX = savedCursorX;
    memcpy(cursorSave, savedCursorSave, sizeof cursorSave);
}

/**
 * @brief Text, clipped at the right edge. No wrap: the shared code centres
 * some strings wider than 32 columns, which start at x = 255 - a wrap would
 * carry them onto the row below.
 *
 * As MS-DOS drawText: ASCII 32-64 are glyphs 0-32. Upper case, which MS-DOS
 * leaves on its art glyphs, folds onto the lower-case letters.
 */
void drawText(unsigned char x, unsigned char y, char *s)
{
    unsigned char c;

    if (x >= WIDTH)
        x = 0;
    while ((c = (unsigned char)*s++) && x < WIDTH)
    {
        if (c >= 32 && c < 65)
            c -= 32;
        else if (c >= 'A' && c <= 'Z')
            c += 32;
        glyph(x++, y, c);
    }
}

/**
 * @brief As MS-DOS drawTextAlt: upper case is drawn as the plain (white)
 * lower-case letter; lower case, digits and punctuation in cyan.
 */
void drawTextAlt(unsigned char x, unsigned char y, char *s)
{
    unsigned char c;

    if (x >= WIDTH)
        x = 0;
    while ((c = (unsigned char)*s++) && x < WIDTH)
    {
        if (c >= 32 && c < 65)
            c -= 32;
        if (c >= 'A' && c <= 'Z')
            glyph(x++, y, c + 32);
        else
            put(x++, y, c < 128 ? altTile[c] : sheetTile[c]);
    }
}

void drawChar(unsigned char x, unsigned char y, char c, unsigned char alt)
{
    unsigned char ch = (unsigned char)c;

    if (ch >= 32 && ch < 65)
        ch -= 32;
    if (ch >= 'A' && ch <= 'Z')
        ch += 32;
    put(x, y, alt && ch < 128 ? altTile[ch] : sheetTile[ch]);
}

void drawIcon(unsigned char x, unsigned char y, unsigned char icon)
{
    glyph(x, y, icon);
}

void drawBlank(unsigned char x, unsigned char y)
{
    put(x, y, T_BLANK);
}

void drawSpace(unsigned char x, unsigned char y, unsigned char w)
{
    while (w--)
        put(x++, y, T_BLANK);
}

void drawClock(unsigned char x, unsigned char y)
{
    glyph(x, y, 0x37);
}

void drawConnectionIcon(unsigned char x, unsigned char y)
{
    glyph(x, y, 0x03);
    glyph(x + 1, y, 0x04);
}

/**
 * @brief The six-cell "fujiTZEE" logo, 0x38-0x3D, starting one cell left of
 * x. Its first cell carries the score-name box's left border; away from the
 * board that border is left off, as MS-DOS erases it.
 */
void drawFujitzee(unsigned char x, unsigned char y)
{
    unsigned char i;

    put(x - 1, y, boardDrawn ? sheetTile[0x38] : T_LOGO_NOBAR);
    for (i = 1; i < 6; i++)
        glyph(x - 1 + i, y, 0x38 + i);
}

void drawLine(unsigned char x, unsigned char y, unsigned char w)
{
    while (w--)
        glyph(x++, y, 0x52);
}

void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h)
{
    unsigned char i;

    glyph(x, y, 0x51);
    for (i = 0; i < w; i++)
        glyph(x + 1 + i, y, 0x52);
    glyph(x + w + 1, y, 0x55);

    for (i = 0; i < h; i++)
    {
        glyph(x, y + 1 + i, 0x7C);
        glyph(x + w + 1, y + 1 + i, 0x7C);
    }

    glyph(x, y + h + 1, 0x57);
    for (i = 0; i < w; i++)
        glyph(x + 1 + i, y + h + 1, 0x52);
    glyph(x + w + 1, y + h + 1, 0x5A);
}

void drawDie(unsigned char x, unsigned char y, unsigned char s, bool isSelected, bool isHighlighted)
{
    const unsigned char *src;
    unsigned char r, c;

    // Invalid index, or the bottom border-screen corners the Adam and CoCo
    // also skip: the status line at HEIGHT-1 runs under those columns.
    if (!s || s > 16 || y == HEIGHT - 3)
        return;

    src = diceChars + (s - 1) * 9;
    if (isSelected)
        src += 54;
    if (isHighlighted)
        src += (s < 14) ? 171 : 27;

    for (r = 0; r < 3; r++)
        for (c = 0; c < 3; c++)
            glyph(x + c, y + r, *src++);
}

/*
  The MS-DOS drawBoard, shifted left by BX = 10-SCORES_X cells: the score-name
  box's left edge goes from column 9 to 1 and the right-hand edge from 39 to
  31, the last column of the 32.
*/
#define BX (10 - SCORES_X)

/* The board's bottom rule. It shares row 20 with the top of the dice. */
static void drawBottomRule(void)
{
    unsigned char x;

    glyph(9 - BX, 20, 0x57);
    for (x = 10 - BX; x < 39 - BX; x++)
        glyph(x, 20, 0x52);
    for (x = 15 - BX; x < 39 - BX; x += 4)
        glyph(x, 20, 0x58);
    glyph(39 - BX, 20, 0x5A);
}

void drawBoard()
{
    unsigned char x, y;

    /* Thin horizontal rules (0x54). */
    for (x = 9 - BX; x < 40 - BX; x++)
        glyph(x, 9, 0x54);
    for (x = 10 - BX; x < 40 - BX; x++)
        glyph(x, 12, 0x54);

    /* Thick horizontal rules (0x52). */
    for (x = 16 - BX; x < 40 - BX; x++)
        glyph(x, 0, 0x52);
    for (x = 10 - BX; x < 40 - BX; x++)
        glyph(x, 2, 0x52);

    /* Score-name box. */
    drawBox(9 - BX, 2, 5, 17);

    /* Player column dividers; row 0 uses the upper tee. */
    for (y = 0; y < 20; y++)
        for (x = 15 - BX; x <= 39 - BX; x += 4)
            glyph(x, y, y == 0 ? 0x5B : 0x7C);

    /* Crosses and tees where the rules meet the dividers. */
    glyph(9 - BX, 9, 0x53);
    glyph(9 - BX, 12, 0x53);
    for (x = 15 - BX; x <= 39 - BX; x += 4)
    {
        glyph(x, 2, 0x50);
        glyph(x, 9, 0x4F);
        glyph(x, 12, 0x4F);
    }

    /* Corners and right-hand tees. */
    glyph(15 - BX, 0, 0x51);
    glyph(39 - BX, 0, 0x55);
    glyph(39 - BX, 2, 0x60);
    glyph(39 - BX, 9, 0x5F);
    glyph(39 - BX, 12, 0x5F);
    drawBottomRule();

    /* Score-row labels. */
    for (y = 0; y < 14; y++)
        drawTextAlt(SCORES_X, scoreY[y], scores[y]);

    boardDrawn = true;
    drawFujitzee(SCORES_X, scoreY[14]);
}

void clearBelowBoard()
{
    unsigned char y;

    hideDiceCursor(0);
    for (y = HEIGHT - 4; y < HEIGHT; y++)
        drawSpace(0, y, WIDTH);
    if (boardDrawn)
        drawBottomRule();
}

#endif /* BUILD_ATARI7800 */

#ifdef BUILD_SMS

/*
  Graphics functionality - Sega Master System VDP, Mode 4

  The art is the MS-DOS client's CGA sheet, converted tile for tile by
  src/sms/mktiles.py into 4bpp tiles that live in the image's third bank and
  are uploaded once. The drawing is the NES port's (src/nes/graphics.c), the
  MS-DOS client's on the Adam's 32 column grid: the MS-DOS board shifted eight
  cells left lands exactly on the Adam board's columns.

  Mode 4 shows 24 rows, one fewer than MS-DOS and the NES, so the layout is
  the Adam's: the board ends on row 20 and the dice stand on rows 20-22, under
  the six player columns. They cover the board's bottom rule only where they
  stand, and any blank written to row 20 while the board is up becomes that
  rule again (put()), so clearing the dice or the roll button restores it
  without anyone having to know.

  Two things the MS-DOS client does pixel by pixel are done by the VDP:
    - the active player's column turns its blue background black: each name
      table entry picks one of two palettes, and palette 1 is palette 0 with
      blue made black. The handful of blue pixels in those cells that MS-DOS
      leaves blue are drawn in a second blue that palette 1 keeps
      (mktiles.py), so the column comes out exactly as the MS-DOS one does.
    - the dice cursor, a checkerboard frame one pixel outside the die, is
      twelve sprites, so drawing and hiding it never touches the name table.
      The frame reaches into the Fujitzee row above the dice and the status
      row below them; text cells carry the priority bit, so the frame passes
      behind the digits and letters and shows only through their background.

  The write path is fujinet-battleship's src/sms/graphics.c: a RAM shadow of
  the name table, written to VRAM a cell or a run at a time with the display
  on (vdp.asm paces the writes), and also what saveScreenBuffer() keeps.
*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "vars.h"
#include "tiles.h"
#include "../misc.h"

#define NAME_TABLE  0x3800
#define SAT         0x3F00
#define SAT_XN      0x3F80   /* the X/tile pairs */
#define VRAM_WRITE  0x4000
#define CRAM_WRITE  0xC000

#define CELLS       (WIDTH * HEIGHT)

/* Name table entry bits above the tile number */
#define PAL1        0x0800
#define PRIO        0x1000

/* The board's bottom rule, and the dice's top row */
#define EDGE_Y      (HEIGHT - 4)

/* First interior cell of player p's score column */
#define HX(p)       (SCORES_X + 6 + ((p) << 2))

#define xyoff(x, y) ((uint16_t)(y) * WIDTH + (x))

/* Clearing the shadow is a memset to 0. */
#if T_BLANK != 0
#error "mktiles.py must make the blank tile 0"
#endif

extern void vdp_addr(unsigned int cmd) __z88dk_fastcall;    /* vdp.asm */
extern void vdp_word(unsigned int w) __z88dk_fastcall;
extern void vdp_byte(unsigned char b) __z88dk_fastcall;
extern void vdp_blit(const unsigned int *cells) __z88dk_fastcall;
extern void vdp_runs(const unsigned char *runs) __z88dk_fastcall;
extern void mapperInit(void);
extern unsigned int blitCount;

unsigned char colorMode = 0;

/* What each cell shows, as a name table entry, and the copy
   saveScreenBuffer() keeps. */
static unsigned int screen[CELLS];
static unsigned int screenBak[CELLS];

/* While set, cells only go to the shadow; the caller blits them. */
static bool batch;

/* Set by drawBoard: row 20 is the board's bottom rule, and the logo's first
   cell carries the score-box border. */
static bool boardDrawn, savedBoardDrawn;

static int8_t highlightX = -1;
static int8_t savedHighlightX = -1;

static bool cursorActive, savedCursorActive;
static unsigned char cursorX, savedCursorX;

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
    /* 16 - "Roll" button: cannot roll. The Atari's X, which mktiles.py puts
       in the MS-DOS sheet's empty 0xCF: MS-DOS's 0x50 is a board cross. */
    0x41,0x40,0x42, 0x2C,0x2D,0x2E, 0x43,0xCF,0x44,

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

static void vdp_reg(unsigned char r, unsigned char v)
{
    vdp_addr(((unsigned int)(0x80 | r) << 8) | v);
}

/// @brief Copy a shadow range to the name table
static void blit(uint16_t off, uint16_t count)
{
    if (batch || off >= CELLS)
        return;
    if (count > CELLS - off)
        count = CELLS - off;
    if (count == 0)
        return;
    vdp_addr(VRAM_WRITE | NAME_TABLE | (off << 1));
    blitCount = count;
    vdp_blit(screen + off);
}

/* ---- Row 20 and the active column ---- */

/*
  Row 20 of the board: the score-name box's corner at 1, the rule, a tee
  under each player column divider (7, 11 .. 27) and the corner at 31. The
  rule is mktiles.py's T_RULE_BOTTOM, whose lower half stays blue under the
  active column -- MS-DOS stops the highlight at the rule.
*/
static unsigned int edgeTile(unsigned char x)
{
    if (x == 0)
        return T_BLANK;
    if (x == 1)
        return sheetTile[0x57];
    if (x == WIDTH - 1)
        return sheetTile[0x5A];
    if (x >= SCORES_X + 5 && ((x - (SCORES_X + 5)) & 3) == 0)
        return T_TEE_BOTTOM;
    return T_RULE_BOTTOM;
}

static bool isEdge(unsigned int e)
{
    return (unsigned char)e == T_RULE_BOTTOM || (unsigned char)e == T_TEE_BOTTOM;
}

/* The active column is cells HX-1 (its left divider, whose right half is
   inside the column) to HX+2, rows 0-19, and the rule on row 20. */
static bool inColumn(unsigned char x)
{
    return highlightX >= 0 && x + 1 >= HX(highlightX) && x <= HX(highlightX) + 2;
}

/**
 * @brief put entry e at column x, row y. Every cell goes through here, so
 * the bottom rule and the active column hold through any redraw.
 */
static void put(unsigned char x, unsigned char y, unsigned int e)
{
    static uint16_t off;

    if (x >= WIDTH || y >= HEIGHT)
        return;

    if (y == EDGE_Y && boardDrawn && (unsigned char)e == T_BLANK)
        e = edgeTile(x);
    if (y <= EDGE_Y && inColumn(x) && (y < EDGE_Y || isEdge(e)))
        e |= PAL1;

    off = xyoff(x, y);
    if (screen[off] == e)
        return;
    screen[off] = e;
    if (!batch)
    {
        vdp_addr(VRAM_WRITE | NAME_TABLE | (off << 1));
        vdp_word(e);
    }
}

/**
 * @brief plot MS-DOS glyph g, as MS-DOS plot_glyph() with MASK_WHITE
 */
static void glyph(unsigned char x, unsigned char y, unsigned char g)
{
    put(x, y, sheetTile[g]);
}

/* A run of cells is composed in the shadow and sent as one stream. */
static bool runBatch;

static void runBegin(void)
{
    runBatch = batch;
    batch = true;
}

static void runEnd(unsigned char x, unsigned char y, unsigned char n)
{
    batch = runBatch;
    if (x < WIDTH && y < HEIGHT)
        blit(xyoff(x, y), n);
}

static void column(int8_t player, bool on)
{
    static unsigned char x, y;
    static uint16_t off;
    static unsigned int e;

    if (player < 0)
        return;

    for (y = 0; y <= EDGE_Y; y++)
    {
        off = xyoff(HX(player) - 1, y);
        for (x = 0; x < 4; x++)
        {
            e = screen[off + x];
            if (y == EDGE_Y && !isEdge(e))
                continue;
            screen[off + x] = on ? (e | PAL1) : (e & ~PAL1);
        }
        blit(off, 4);
    }
}

void setHighlight(int8_t player, bool isThisPlayer, uint8_t flash)
{
    (void)isThisPlayer;
    (void)flash;

    if (highlightX != player)
    {
        column(highlightX, false);
        highlightX = player;
    }
    column(player, true);
}

/* ---- Dice cursor: sprites ---- */

static void placeCursor(unsigned char x)
{
    static unsigned char i;
    static const unsigned char *s;

    // Y: the line above the sprite's first one; $D0 ends the list
    vdp_addr(VRAM_WRITE | SAT);
    s = cursorSprite;
    for (i = 0; i < CURSOR_SPRITES; i++)
    {
        vdp_byte((EDGE_Y << 3) - 4 + (s[1] << 3) - 1);
        s += 3;
    }
    vdp_byte(0xD0);

    vdp_addr(VRAM_WRITE | SAT_XN);
    s = cursorSprite;
    for (i = 0; i < CURSOR_SPRITES; i++)
    {
        vdp_byte((x << 3) - 4 + (s[0] << 3));
        vdp_byte(s[2]);
        s += 3;
    }
}

static void clearSprites(void)
{
    vdp_addr(VRAM_WRITE | SAT);
    vdp_byte(0xD0);
}

void drawDiceCursor(unsigned char x)
{
    // The core asks for the roll button one cell left of where the Adam
    // layout puts it (ROLL_X).
    if (x == ROLL_X - 1)
        x++;
    cursorX = x;
    cursorActive = true;
    placeCursor(x);
}

void hideDiceCursor(unsigned char x)
{
    (void)x;
    if (!cursorActive)
        return;
    cursorActive = false;
    clearSprites();
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
    static uint8_t i;

    vdp_reg(1, 0x80); // display and frame interrupt off
    vdp_reg(7, 0xF4); // border: palette 1 colour 4, the blue that stays blue

    // The art is in bank 2; the Sega mapper must have it in slot 2.
    mapperInit();

    vdp_addr(CRAM_WRITE);
    for (i = 0; i < 32; i++)
        vdp_byte(smsPalette[i]);

    vdp_runs(smsTiles);
    clearSprites();

    memset(screen, 0, sizeof screen);
    blit(0, CELLS);

    // Display on, frame interrupt on: crt0's handler counts frames in timer,
    // which waitvsync() and getTime() read (src/sms/util.c).
    vdp_reg(1, 0xE0);
}

void resetGraphics()
{
}

void resetScreen(bool forBorderScreen)
{
    static unsigned char y;

    hideDiceCursor(0);
    highlightX = -1;
    boardDrawn = false;

    if (!forBorderScreen)
        memset(screen, 0, sizeof screen);
    else
    {
        // Moving between bordered screens: clear everything except the top
        // corner dice, as the Adam does. drawDie() never draws the bottom
        // pair, whose rows the status line runs through.
        for (y = 0; y < 3; y++)
            memset(screen + xyoff(3, y), 0, (WIDTH - 6) * sizeof(unsigned int));
        memset(screen + xyoff(0, 3), 0, (HEIGHT - 3) * WIDTH * sizeof(unsigned int));
    }
    blit(0, CELLS);
}

bool saveScreenBuffer()
{
    memcpy(screenBak, screen, sizeof screen);
    savedHighlightX = highlightX;
    savedBoardDrawn = boardDrawn;
    savedCursorActive = cursorActive;
    savedCursorX = cursorX;
    return true;
}

void restoreScreenBuffer()
{
    memcpy(screen, screenBak, sizeof screen);
    highlightX = savedHighlightX;
    boardDrawn = savedBoardDrawn;
    cursorActive = savedCursorActive;
    cursorX = savedCursorX;
    blit(0, CELLS);
    if (cursorActive)
        placeCursor(cursorX);
    else
        clearSprites();
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
    static unsigned char c, x0;

    if (x >= WIDTH)
        x = 0;
    x0 = x;
    runBegin();
    while ((c = (unsigned char)*s++) && x < WIDTH)
    {
        if (c >= 32 && c < 65)
            c -= 32;
        else if (c >= 'A' && c <= 'Z')
            c += 32;
        put(x++, y, sheetTile[c] | PRIO);
    }
    runEnd(x0, y, x - x0);
}

/**
 * @brief As MS-DOS drawTextAlt: upper case is drawn as the plain (white)
 * lower-case letter; lower case, digits and punctuation in cyan.
 */
void drawTextAlt(unsigned char x, unsigned char y, char *s)
{
    static unsigned char c, x0;

    if (x >= WIDTH)
        x = 0;
    x0 = x;
    runBegin();
    while ((c = (unsigned char)*s++) && x < WIDTH)
    {
        if (c >= 32 && c < 65)
            c -= 32;
        if (c >= 'A' && c <= 'Z')
            put(x++, y, sheetTile[c + 32] | PRIO);
        else
            put(x++, y, (c < 128 ? altTile[c] : sheetTile[c]) | PRIO);
    }
    runEnd(x0, y, x - x0);
}

void drawChar(unsigned char x, unsigned char y, char c, unsigned char alt)
{
    static unsigned char ch;

    ch = (unsigned char)c;
    if (ch >= 32 && ch < 65)
        ch -= 32;
    if (ch >= 'A' && ch <= 'Z')
        ch += 32;
    put(x, y, (alt && ch < 128 ? altTile[ch] : sheetTile[ch]) | PRIO);
}

void drawIcon(unsigned char x, unsigned char y, unsigned char icon)
{
    put(x, y, sheetTile[icon] | PRIO);
}

void drawBlank(unsigned char x, unsigned char y)
{
    put(x, y, T_BLANK);
}

void drawSpace(unsigned char x, unsigned char y, unsigned char w)
{
    static unsigned char x0, n;

    x0 = x;
    n = w;
    runBegin();
    while (w--)
        put(x++, y, T_BLANK);
    runEnd(x0, y, n);
}

void drawClock(unsigned char x, unsigned char y)
{
    put(x, y, sheetTile[0x37] | PRIO);
}

void drawConnectionIcon(unsigned char x, unsigned char y)
{
    put(x, y, sheetTile[0x03] | PRIO);
    put(x + 1, y, sheetTile[0x04] | PRIO);
}

/**
 * @brief The six-cell "fujiTZEE" logo, 0x38-0x3D, starting one cell left of
 * x. Its first cell carries the score-name box's left border; away from the
 * board that border is left off, as MS-DOS erases it.
 */
void drawFujitzee(unsigned char x, unsigned char y)
{
    static unsigned char i;

    runBegin();
    put(x - 1, y, boardDrawn ? sheetTile[0x38] : T_LOGO_NOBAR);
    for (i = 1; i < 6; i++)
        glyph(x - 1 + i, y, 0x38 + i);
    runEnd(x - 1, y, 6);
}

void drawLine(unsigned char x, unsigned char y, unsigned char w)
{
    static unsigned char x0, n;

    x0 = x;
    n = w;
    runBegin();
    while (w--)
        glyph(x++, y, 0x52);
    runEnd(x0, y, n);
}

void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h)
{
    static unsigned char i;

    drawLine(x + 1, y, w);
    glyph(x, y, 0x51);
    glyph(x + w + 1, y, 0x55);

    for (i = 0; i < h; i++)
    {
        glyph(x, y + 1 + i, 0x7C);
        glyph(x + w + 1, y + 1 + i, 0x7C);
    }

    drawLine(x + 1, y + h + 1, w);
    glyph(x, y + h + 1, 0x57);
    glyph(x + w + 1, y + h + 1, 0x5A);
}

void drawDie(unsigned char x, unsigned char y, unsigned char s, bool isSelected, bool isHighlighted)
{
    static const unsigned char *src;
    static unsigned char r, c;

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
    {
        runBegin();
        for (c = 0; c < 3; c++)
            glyph(x + c, y + r, *src++);
        runEnd(x, y + r, 3);
    }
}

/*
  The MS-DOS drawBoard, shifted left by BX = 10-SCORES_X cells: the score-name
  box's left edge goes from column 9 to 1 and the right-hand edge from 39 to
  31, the last column of the 32. Its bottom rule is row 20, which the dice
  share (edgeTile()).
*/
#define BX (10 - SCORES_X)

void drawBoard()
{
    static unsigned char x, y;

    batch = true;
    boardDrawn = true;

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
    for (y = 0; y < EDGE_Y; y++)
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

    /* The bottom rule, row 20. */
    for (x = 0; x < WIDTH; x++)
        put(x, EDGE_Y, edgeTile(x));

    /* Score-row labels. */
    for (y = 0; y < 14; y++)
        drawTextAlt(SCORES_X, scoreY[y], scores[y]);

    drawFujitzee(SCORES_X, scoreY[14]);

    batch = false;
    blit(0, CELLS);
}

void clearBelowBoard()
{
    static unsigned char y;

    hideDiceCursor(0);
    batch = true;
    for (y = EDGE_Y; y < HEIGHT; y++)
        drawSpace(0, y, WIDTH);
    batch = false;
    blit(xyoff(0, EDGE_Y), (HEIGHT - EDGE_Y) * WIDTH);
}

#endif /* BUILD_SMS */

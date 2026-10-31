#ifdef __H89__

/**
 * @brief   Graphics - Heathkit H89 (H19 terminal, 80x24 + 25th line)
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 *
 * Everything is drawn with the H19's own characters: its graphics set (line
 * drawing, edge bars, the bullet, the checkerboard, arrows) and reverse
 * video, which is the alternate text style here. Text is shown in capitals,
 * as the game's fonts do on the other platforms.
 *
 * The board (drawBoard) is the 40 column one widened to 80 -- see
 * src/h89/vars.h for the columns. Lines are drawn by direction (line()), so
 * where two meet -- the board's rules and dividers, a box laid over the
 * frame of the bordered screens -- the cell becomes the right corner, T or
 * cross by itself.
 *
 * Dice are 7x3, which is square on the H19's tall cells: a free die is a
 * solid reverse video block with dark pips, a kept one is hollow -- bright
 * pips between edge bars. The roll button is the same size, and the dice
 * cursor is a bracket in the gap columns either side. The active player's
 * column header is a reverse video tab, kept through every redraw of the
 * initial in it by cell(), which all drawing goes through.
 *
 * Drawing goes into console.c's back buffer, and goes to the terminal only
 * at waitvsync() (src/h89/util.c), when input is read (src/h89/input.c) and
 * before every FujiNet call (src/h89/fujinet.c) -- at 9600 baud, only what
 * changed.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "console.h"
#include "../platform-specific/graphics.h"

// Line directions, for line()
#define LN_U 1
#define LN_D 2
#define LN_L 4
#define LN_R 8

// The board: the score name box's left border, the player column dividers
// (the first is the name box's right border), and the rules
#define NAMES_LEFT   11
#define DIV_FIRST    23
#define DIV_STEP     9
#define DIV_LAST     77
#define COL_W        8  // a player column's inside
#define RULE_TOP     0
#define RULE_HEAD    2
#define RULE_UPPER   9
#define RULE_LOWER   12
#define RULE_BOTTOM  20
#define INITIAL_ROW  1

#define DIE_W 7
#define DIE_H 3

// The bordered screens: a die in each corner, the frame between them
#define CORNER_RIGHT (WIDTH - DIE_W)
#define CORNER_BOTTOM (HEIGHT - 1 - DIE_H)

unsigned char colorMode = 0;

// The score names, in scoreY[] order (the FUJITZEE row is drawFujitzee's)
static const char *const scoreName[14] = {
    "ONES", "TWOS", "THREES", "FOURS", "FIVES", "SIXES", "TOTAL", "BONUS",
    "SET OF 3", "SET OF 4", "FULL HOUSE", "SHORT RUN", "LONG RUN", "COUNT"};

// Pips on a 3x3 grid, bit r*3+c, shown in columns 1, 3 and 5
static const uint16_t pipMask[6] = {
    0x010, // 1: centre
    0x044, // 2: top right, bottom left
    0x054, // 3
    0x145, // 4: the corners
    0x155, // 5
    0x16D, // 6: two columns of three
};

// The line drawing character for each combination of directions
static const uint8_t lineGlyph[16] = {
    ' ',         G_VLINE,     G_VLINE,    G_VLINE,
    G_HLINE,     G_CORNER_LR, G_CORNER_UR, G_TEE_LEFT,
    G_HLINE,     G_CORNER_LL, G_CORNER_UL, G_TEE_RIGHT,
    G_HLINE,     G_TEE_UP,    G_TEE_DOWN, G_CROSS};

// The active player's column (-1 none), whether its scores are flashed, and
// whether the board is up at all
static int8_t tab = -1;
static bool flashed, boardShown;

// The dice cursor
static bool cursorShown;
static uint8_t cursorX;

// What saveScreenBuffer() keeps beside the screen
static int8_t savedTab;
static bool savedBoardShown, savedCursorShown;
static uint8_t savedCursorX;

/*
 * Cells
 */

static uint8_t tabLeft(int8_t p)
{
    return DIV_FIRST + 1 + DIV_STEP * (uint8_t)p;
}

/// @brief Set a cell, in reverse if it is in the active player's tab
static void cell(uint8_t x, uint8_t y, uint8_t code)
{
    static uint8_t l;

    if (y == INITIAL_ROW && tab >= 0 && boardShown)
    {
        l = tabLeft(tab);
        if (x >= l && x < l + COL_W)
            code |= REV;
    }
    put(x, y, code);
}

// The directions of each graphics character (screen codes 0x00-0x1F), 0 for
// those that are not line drawing
static const uint8_t glyphBits[32] = {
    LN_U | LN_D,               // G_VLINE
    LN_L | LN_R,               // G_HLINE
    LN_U | LN_D | LN_L | LN_R, // G_CROSS
    LN_D | LN_L,               // G_CORNER_UR
    LN_U | LN_L,               // G_CORNER_LR
    LN_U | LN_R,               // G_CORNER_LL
    LN_D | LN_R,               // G_CORNER_UL
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    LN_D | LN_L | LN_R,        // G_TEE_DOWN
    LN_U | LN_D | LN_L,        // G_TEE_LEFT
    LN_U | LN_L | LN_R,        // G_TEE_UP
    LN_U | LN_D | LN_R,        // G_TEE_RIGHT
    0, 0, 0, 0, 0, 0, 0, 0, 0};

/// @brief Draw a piece of line, joining any line already in the cell
static void line(uint8_t x, uint8_t y, uint8_t bits)
{
    static uint8_t c;

    c = peek(x, y);
    if (c < 0x20)
        bits |= glyphBits[c];
    put(x, y, lineGlyph[bits]);
}

/*
  Lines are drawn as a set of straight segments at once, a row at a time:
  every segment through a cell adds its directions, so corners and junctions
  come out whole. (A line end stored on its own would be a plain line, and
  read back as one.) Only what was on screen before -- the frame under a box
  -- is joined cell by cell.
*/
#define SEG_MAX 8

typedef struct
{
    uint8_t from, to, at; // across: x from-to on row at; down: y from-to in column at
} Seg;

static Seg acrossSeg[SEG_MAX], downSeg[SEG_MAX];
static uint8_t acrossCount, downCount;
static uint8_t rowBits[WIDTH];
static uint8_t segLeft = WIDTH - 1, segRight; // the columns the segments span

static void segSpan(uint8_t x0, uint8_t x1)
{
    if (x0 < segLeft)
        segLeft = x0;
    if (x1 > segRight)
        segRight = x1;
}

static void across(uint8_t x0, uint8_t x1, uint8_t y)
{
    if (x1 >= WIDTH)
        x1 = WIDTH - 1;
    acrossSeg[acrossCount].from = x0;
    acrossSeg[acrossCount].to = x1;
    acrossSeg[acrossCount].at = y;
    acrossCount++;
    segSpan(x0, x1);
}

static void down(uint8_t x, uint8_t y0, uint8_t y1)
{
    if (x >= WIDTH)
        return;
    downSeg[downCount].from = y0;
    downSeg[downCount].to = y1;
    downSeg[downCount].at = x;
    downCount++;
    segSpan(x, x);
}

/// @brief Draw the segments given since the last call, rows y0-y1
static void drawSegs(uint8_t y0, uint8_t y1)
{
    static uint8_t x, y, i, n;
    static Seg *g;

    n = segRight - segLeft + 1;
    for (y = y0; y <= y1; y++)
    {
        memset(rowBits + segLeft, 0, n);
        for (i = 0; i < acrossCount; i++)
        {
            g = &acrossSeg[i];
            if (g->at != y)
                continue;
            for (x = g->from; x <= g->to; x++)
                rowBits[x] |= (x > g->from ? LN_L : 0) | (x < g->to ? LN_R : 0);
        }
        for (i = 0; i < downCount; i++)
        {
            g = &downSeg[i];
            if (y < g->from || y > g->to)
                continue;
            rowBits[g->at] |= (y > g->from ? LN_U : 0) | (y < g->to ? LN_D : 0);
        }
        for (x = segLeft; x <= segRight; x++)
            if (rowBits[x])
                line(x, y, rowBits[x]);
    }
    acrossCount = downCount = 0;
    segLeft = WIDTH - 1;
    segRight = 0;
}

/*
 * Text
 */

/// @brief The screen code for a character of text: capitals, and a space
/// for anything the terminal can't show
static uint8_t textCode(uint8_t c)
{
    if (c >= 'a' && c <= 'z')
        return c - 32;
    if (c < 0x20 || c > 0x7E)
        return ' ';
    return c;
}

/// @brief Is a letter or digit of alternate text in the alternate style?
/// Capitals are shown plain, the rest in reverse video.
static bool altStyle(uint8_t c)
{
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

void drawText(unsigned char x, unsigned char y, char *s)
{
    static uint8_t c;

    while ((c = (uint8_t)*s++) && x < WIDTH)
        cell(x++, y, textCode(c));
}

void drawTextAlt(unsigned char x, unsigned char y, char *s)
{
    static uint8_t c;
    static bool rev, prevRev;

    // A space is reversed only between two reversed letters or digits, so
    // headings become one bar, words keep their gaps and padding stays
    // blank; punctuation goes with what it follows ("1.", "SET," but
    // "FILLED.")
    prevRev = false;
    while ((c = (uint8_t)*s++) && x < WIDTH)
    {
        if (c == ' ')
            rev = prevRev && altStyle((uint8_t)*s);
        else if ((c >= 'A' && c <= 'Z') || altStyle(c))
            rev = altStyle(c);
        else
            rev = prevRev;
        cell(x++, y, textCode(c) | (rev ? REV : 0));
        prevRev = rev;
    }
}

void drawTextRev(unsigned char x, unsigned char y, char *s)
{
    static uint8_t c;

    while ((c = (uint8_t)*s++) && x < WIDTH)
        cell(x++, y, textCode(c) | REV);
}

void drawChar(unsigned char x, unsigned char y, char c, unsigned char alt)
{
    // The initials over the player columns are plain: the tab marks the
    // active one
    if (y == INITIAL_ROW && boardShown)
        alt = 0;
    cell(x, y, textCode((uint8_t)c) | (alt ? REV : 0));
}

void drawIcon(unsigned char x, unsigned char y, unsigned char icon)
{
    cell(x, y, icon);
}

void drawBlank(unsigned char x, unsigned char y)
{
    cell(x, y, ' ');
}

void drawSpace(unsigned char x, unsigned char y, unsigned char w)
{
    while (w--)
        cell(x++, y, ' ');
}

void drawLine(unsigned char x, unsigned char y, unsigned char w)
{
    while (w--)
        line(x++, y, LN_L | LN_R);
}

void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h)
{
    static uint8_t r;

    // The in-game help clears the columns it covers but not the board's top
    // and bottom rules: make those plain there, rather than T pieces
    // leading nowhere. (Hiding the help restores the whole screen.)
    if (boardShown && x == HELP_X - 1 && y == 1)
    {
        fill(HELP_CLEAR_X, RULE_TOP, HELP_CLEAR_W, G_HLINE);
        fill(HELP_CLEAR_X, RULE_BOTTOM, HELP_CLEAR_W, G_HLINE);
        put(HELP_CLEAR_X - 1, RULE_HEAD, G_TEE_LEFT);
        put(HELP_CLEAR_X - 1, RULE_UPPER, G_TEE_LEFT);
        put(HELP_CLEAR_X - 1, RULE_LOWER, G_TEE_LEFT);
        put(DIV_LAST, RULE_HEAD, G_VLINE);
        put(DIV_LAST, RULE_UPPER, G_VLINE);
        put(DIV_LAST, RULE_LOWER, G_VLINE);
    }

    across(x, x + w + 1, y);
    across(x, x + w + 1, y + h + 1);
    down(x, y, y + h + 1);
    down(x + w + 1, y, y + h + 1);
    drawSegs(y, y + h + 1);
    for (r = 1; r <= h; r++)
        fill(x + 1, y + r, w, ' ');
}

void drawClock(unsigned char x, unsigned char y)
{
    // An hourglass
    cell(x, y, REV | G_X);
}

void drawConnectionIcon(unsigned char x, unsigned char y)
{
    cell(x, y, REV | G_X);
    cell(x + 1, y, REV | '?');
}

void drawFujitzee(unsigned char x, unsigned char y)
{
    // On the board, the last score name; anywhere else (the help screen) it
    // stands in front of " - 5 OF A KIND" drawn at x, so it ends before it
    if (x != SCORES_X)
        x -= 3;
    drawTextAlt(x, y, "fujiTZEE");
}

/*
 * Dice
 */

/// @brief A 7x3 die (1-6), roll button (14-15 for 1-2 rolls left, 16 when
/// all the dice are kept) or blank (13)
static void dieAt(uint8_t x, uint8_t y, uint8_t s, bool isSelected, bool isHighlighted)
{
    static uint8_t r, c, code, flip, rowPips, bit;
    static uint16_t mask;

    flip = isHighlighted ? REV : 0;
    if (s <= 6)
        mask = pipMask[s - 1];

    for (r = 0; r < DIE_H; r++)
    {
        // This row's three pip places, in bits 1, 3 and 5 for columns 1, 3, 5
        rowPips = (uint8_t)((mask & 7) << 1);
        rowPips = (rowPips & 2) | ((rowPips & 4) << 1) | ((rowPips & 8) << 2);
        mask >>= 3;
        bit = 1;
        for (c = 0; c < DIE_W; c++, bit <<= 1)
        {
            if (s <= 6)
            {
                if (rowPips & bit)
                    code = G_BULLET;
                else if (isSelected && c == 0)
                    code = G_BAR_LEFT;
                else if (isSelected && c == DIE_W - 1)
                    code = G_BAR_RIGHT;
                else
                    code = ' ';
                // Free dice are solid; kept ones hollow
                if (!isSelected)
                    code |= REV;
            }
            else if (s == 13)
                code = ' ';
            else if (r == 1)
            {
                // " ROLL! " -- plain on a checkerboard when nothing is left
                // to roll
                code = (uint8_t)" ROLL! "[c] | (s == 16 ? 0 : REV);
            }
            else if (s == 16)
                code = G_CHECKER;
            else if (r == 2 && ((s == 14 && c == 3) || (s == 15 && (c == 2 || c == 4))))
                code = REV | G_BULLET; // a dot for each roll left
            else
                code = REV | ' ';

            cell(x + c, y + r, code ^ flip);
        }
    }
}

/// @brief The frame between the bordered screens' corner dice
static void drawFrameLines(void)
{
    across(DIE_W, CORNER_RIGHT - 1, 0);
    across(DIE_W, CORNER_RIGHT - 1, CORNER_BOTTOM + DIE_H - 1);
    down(0, DIE_H, CORNER_BOTTOM - 1);
    down(WIDTH - 1, DIE_H, CORNER_BOTTOM - 1);
    drawSegs(0, CORNER_BOTTOM + DIE_H - 1);
}

/// @brief The bordered screens' corner dice and the frame between them
static void drawFrame(void)
{
    dieAt(0, 0, 1, 0, 0);
    dieAt(CORNER_RIGHT, 0, 2, 0, 0);
    dieAt(0, CORNER_BOTTOM, 3, 0, 0);
    dieAt(CORNER_RIGHT, CORNER_BOTTOM, 4, 0, 0);
    drawFrameLines();
}

void drawBorder()
{
    drawFrame();
}

void drawDie(unsigned char x, unsigned char y, unsigned char s, bool isSelected, bool isHighlighted)
{
    // The shared code's flash loop passes the string terminator after the
    // fifth die
    if (!s || s > 16)
        return;

    // The bordered screens' 3x3 corner dice (at WIDTH-3 and HEIGHT-3) are
    // 7x3 here, the bottom ones above the status line, with a frame between
    // them (drawn with the first, the top left one)
    if ((x == 0 || x == WIDTH - 3) && (y == 0 || y == HEIGHT - 3))
    {
        dieAt(x ? CORNER_RIGHT : 0, y ? CORNER_BOTTOM : 0, s, 0, 0);
        if (!x && !y)
            drawFrameLines();
        return;
    }

    dieAt(x, y, s, isSelected, isHighlighted);
}

static void cursorCells(uint8_t x, uint8_t l, uint8_t m, uint8_t r)
{
    static uint8_t y;

    y = HEIGHT - 4;
    put(x - 1, y, l);
    put(x - 1, y + 1, m);
    put(x - 1, y + 2, r);
}

void drawDiceCursor(unsigned char x)
{
    hideDiceCursor(x);

    // [ ] in the gap columns either side of the die
    cursorCells(x, G_CORNER_UL, G_VLINE, G_CORNER_LL);
    cursorCells(x + DIE_W + 1, G_CORNER_UR, G_VLINE, G_CORNER_LR);
    cursorX = x;
    cursorShown = true;
}

void hideDiceCursor(unsigned char x)
{
    // Where it was drawn, whatever the shared code asks
    (void)x;
    if (!cursorShown)
        return;
    cursorCells(cursorX, ' ', ' ', ' ');
    cursorCells(cursorX + DIE_W + 1, ' ', ' ', ' ');
    cursorShown = false;
}

/*
 * Board
 */

/// @brief Turn the active player's tab on or off
static void paintTab(int8_t p, bool on)
{
    static uint8_t x, l, c;

    l = tabLeft(p);
    for (x = l; x < l + COL_W; x++)
    {
        c = peek(x, INITIAL_ROW);
        put(x, INITIAL_ROW, on ? (c | REV) : (c & ~REV));
    }
}

/// @brief Reverse (or put back) a player's score column
static void flashColumn(int8_t p)
{
    static uint8_t x, y, l;

    l = tabLeft(p);
    for (y = RULE_HEAD + 1; y < RULE_BOTTOM; y++)
    {
        if (y == RULE_UPPER || y == RULE_LOWER)
            continue;
        for (x = l; x < l + COL_W; x++)
            put(x, y, peek(x, y) ^ REV);
    }
}

void drawBoard()
{
    static uint8_t x, i;

    // The rules across, the name box and dividers down through them, and a
    // rule under the round, over the players
    across(DIV_FIRST, DIV_LAST, RULE_TOP);
    across(NAMES_LEFT, DIV_LAST, RULE_HEAD);
    across(NAMES_LEFT, DIV_LAST, RULE_UPPER);
    across(NAMES_LEFT, DIV_LAST, RULE_LOWER);
    across(NAMES_LEFT, DIV_LAST, RULE_BOTTOM);
    across(0, 9, 5);
    down(NAMES_LEFT, RULE_HEAD, RULE_BOTTOM);
    for (x = DIV_FIRST; x <= DIV_LAST; x += DIV_STEP)
        down(x, RULE_TOP, RULE_BOTTOM);
    drawSegs(RULE_TOP, RULE_BOTTOM);

    for (i = 0; i < 14; i++)
        drawText(SCORES_X, scoreY[i], (char *)scoreName[i]);
    drawFujitzee(SCORES_X, scoreY[14]);

    boardShown = true;
    if (tab >= 0)
        paintTab(tab, true);
}

void setHighlight(int8_t player, bool isThisPlayer, uint8_t flash)
{
    (void)isThisPlayer;

    if (flashed)
    {
        flashColumn(tab);
        flashed = false;
    }

    // Six columns on the board
    if (player > 5)
        player = -1;

    if (player != tab)
    {
        if (tab >= 0 && boardShown)
            paintTab(tab, false);
        tab = player;
        if (tab >= 0 && boardShown)
            paintTab(tab, true);
    }

    // Your turn: the whole column, for a moment
    if (flash == 2 && tab >= 0 && boardShown)
    {
        flashColumn(tab);
        flashed = true;
    }
}

void clearBelowBoard()
{
    blankRows(HEIGHT - 4, 4);
    cursorShown = false;
}

/*
 * Screen
 */

void resetScreen(bool forBorderScreen)
{
    blankRows(0, HEIGHT);
    tab = -1;
    flashed = boardShown = cursorShown = false;

    if (forBorderScreen)
        drawFrame();
}

bool saveScreenBuffer()
{
    // The tab is in the screen; a flash is always over by the time the
    // shared code saves it
    conSave();
    savedTab = tab;
    savedBoardShown = boardShown;
    savedCursorShown = cursorShown;
    savedCursorX = cursorX;
    return true;
}

void restoreScreenBuffer()
{
    conRestore();
    tab = savedTab;
    boardShown = savedBoardShown;
    cursorShown = savedCursorShown;
    cursorX = savedCursorX;
    flashed = false;
}

void initGraphics()
{
    termInit();
    resetScreen(false);
}

void resetGraphics()
{
    termExit();
}

uint8_t cycleNextColor()
{
    return 0;
}

void setColorMode()
{
}

#endif /* __H89__ */

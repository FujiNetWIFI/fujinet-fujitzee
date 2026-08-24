#ifdef __ADAM__
/*
  Graphics functionality - Coleco Adam / TMS9918A

  The VDP runs in Graphics II (mode 2) but is driven as a 32x24 character
  map: the name table at 0x1800 holds one glyph index per cell, and the
  pattern/color tables carry the charset (identical copies in all three
  screen thirds), giving per-glyph per-row colors at 1-byte-per-cell draw
  cost. A 768-byte RAM shadow of the name table backs saveScreenBuffer()
  and lets draw functions batch their VRAM writes.

  Layout and glyph indices follow the CoCo 1/2 client (src/coco/graphics.c,
  the same 32x24 grid); the CoCo's pixel-level hires_Mask line drawing maps
  onto the chrome glyphs 0x5E-0x68 synthesized by
  support/adam/make_charset.py. Adding 0x80 to a glyph selects the
  light-blue accent bank - the Adam's ROP_BLUE.
*/

#include "../misc.h"
#include <string.h>
#include <video/tms99x8.h>
#include <interrupt.h>

extern const uint8_t charsetPatterns[];
extern const uint8_t charsetColors[];
extern const uint8_t altColors[];

// From util.c - 60Hz NMI hook
extern void jiffyTick(void);
extern volatile uint8_t vsyncFlag;
extern volatile uint16_t jiffyCount;

#define NT_BASE 0x1800
#define CT_BASE 0x2000

#define CHARSET_GLYPHS 0x69
#define ALT_GLYPH_BASE 0x80

#define TILE_BLANK 0x20

// Box glyphs (CoCo charset)
#define TILE_BOX_TL 0x3B
#define TILE_BOX_TR 0x3C
#define TILE_BOX_BL 0x3D
#define TILE_BOX_BR 0x3E
#define TILE_BOX_SIDE 0x3F

// Synthesized board chrome (see make_charset.py)
#define TILE_VLINE 0x5E
#define TILE_HTHICK 0x5F
#define TILE_HTHIN 0x60
#define TILE_VXTHICK 0x61
#define TILE_VXTHIN 0x62
#define TILE_SXTHICK 0x63
#define TILE_SXTHIN 0x64
#define TILE_HBOTTOM 0x65
#define TILE_TBOTTOM 0x66
#define TILE_CURSOR_L 0x67
#define TILE_CURSOR_R 0x68

static uint8_t screen[768];
static uint8_t screenBak[768];

static int8_t highlightX = -1;
static uint8_t cursorX;
static bool cursorActive = false;
static uint8_t cursorSave[6];

#define xyoff(x, y) ((uint16_t)(y)*WIDTH + (x))

// Dice / roll-button tile layout - identical to the CoCo 1/2 table
static const uint8_t diceChars[] = {
  // Normal dice
  0x01, 0x00, 0x02, 0x00, 0x05, 0x00, 0x03, 0x00, 0x04, // 1
  0x01, 0x00, 0x07, 0x00, 0x00, 0x00, 0x08, 0x00, 0x04, // 2
  0x01, 0x00, 0x07, 0x00, 0x05, 0x00, 0x08, 0x00, 0x04, // 3
  0x06, 0x00, 0x07, 0x00, 0x00, 0x00, 0x08, 0x00, 0x09, // 4
  0x06, 0x00, 0x07, 0x00, 0x05, 0x00, 0x08, 0x00, 0x09, // 5
  0x06, 0x00, 0x07, 0x0A, 0x00, 0x0B, 0x08, 0x00, 0x09, // 6

  // Kept dice
  0x11, 0x0C, 0x12, 0x00, 0x05, 0x00, 0x13, 0x0D, 0x14, // 1
  0x11, 0x0C, 0x17, 0x00, 0x00, 0x00, 0x18, 0x0D, 0x14, // 2
  0x11, 0x0C, 0x17, 0x00, 0x05, 0x00, 0x18, 0x0D, 0x14, // 3
  0x16, 0x0C, 0x17, 0x00, 0x00, 0x00, 0x18, 0x0D, 0x19, // 4
  0x16, 0x0C, 0x17, 0x00, 0x05, 0x00, 0x18, 0x0D, 0x19, // 5
  0x16, 0x0C, 0x17, 0x0A, 0x00, 0x0B, 0x18, 0x0D, 0x19, // 6

  // "Roll" button
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 13 - Empty space
  0x01, 0x00, 0x02, 0x0E, 0x0F, 0x10, 0x03, 0x1A, 0x04, // 14 - 1 Roll left
  0x01, 0x00, 0x02, 0x0E, 0x0F, 0x10, 0x03, 0x15, 0x04, // 15 - 2 Rolls left
  0x01, 0x00, 0x02, 0x0E, 0x0F, 0x10, 0x03, 0x1F, 0x04  // 16 - Cannot Roll
};

/// @brief Copy a shadow-buffer range to the name table.
/// count 0 must never reach LDIRVM (it decrements before testing and would
/// spray 64K over VRAM); out-of-range writes would trash the sprite/color
/// tables that live above the name table.
static void blit(uint16_t off, uint16_t count)
{
    if (off >= 768)
        return;
    if (count > 768 - off)
        count = 768 - off;
    if (count == 0)
        return;
    vdp_vwrite((void *)(screen + off), NT_BASE + off, count);
}

/// @brief Set a single cell in shadow + VRAM
static void cell(uint16_t off, uint8_t c)
{
    if (off >= 768)
        return;
    screen[off] = c;
    vdp_vpoke(NT_BASE + off, c);
}

uint8_t cycleNextColor()
{
    return 0;
}

void setColorMode()
{
}

void waitvsync()
{
    // The VDP F flag (status bit 7) sets on every vblank regardless of the
    // interrupt enable, and reading status clears it - so polling works even
    // where the VDP interrupt is not wired through to the Z80 NMI (as in the
    // fujinet-go-adam emulator core). The flag is sticky, so a stale F from
    // an earlier vblank must be consumed FIRST or this returns immediately
    // and the whole game free-runs at full Z80 speed. When the NMI does fire
    // (real hardware), the CRT handler consumes the status read and the
    // installed tick sets vsyncFlag instead - both paths pace to one frame.
    vsyncFlag = 0;
    vdp_get_status(0); // clear any stale frame flag
    while (!vsyncFlag)
    {
        if (vdp_get_status(0) & 0x80)
            break;
    }
    ++jiffyCount;
}

void initGraphics()
{
    static uint16_t base;
    static uint8_t t;

    vdp_color(VDP_INK_WHITE, VDP_INK_BLACK, VDP_INK_BLACK);
    vdp_set_mode(mode_2);

    // Load charset patterns + colors into all three screen thirds. The
    // accent bank at 0x80 reuses the same patterns with altColors.
    for (t = 0; t < 3; t++)
    {
        base = (uint16_t)t << 11;
        vdp_vwrite((void *)charsetPatterns, base, CHARSET_GLYPHS * 8);
        vdp_vwrite((void *)charsetColors, CT_BASE + base, CHARSET_GLYPHS * 8);
        vdp_vwrite((void *)charsetPatterns, base + ALT_GLYPH_BASE * 8, CHARSET_GLYPHS * 8);
        vdp_vwrite((void *)altColors, CT_BASE + base + ALT_GLYPH_BASE * 8, CHARSET_GLYPHS * 8);
    }

    resetScreen(false);

    // 60Hz tick for getTime()/waitvsync()
    add_raster_int(jiffyTick);
}

void resetGraphics()
{
}

void resetScreen(bool forBorderScreen)
{
    static uint8_t y;

    cursorActive = false;

    if (!forBorderScreen)
    {
        memset(screen, TILE_BLANK, sizeof(screen));
        vdp_vfill(NT_BASE, TILE_BLANK, 768);
    }
    else
    {
        // Moving between bordered screens: clear everything except the top
        // corner dice (cells 0-2 and 29-31, rows 0-2), like the CoCo
        for (y = 0; y < 3; y++)
            memset(screen + xyoff(3, y), TILE_BLANK, WIDTH - 6);
        memset(screen + xyoff(0, 3), TILE_BLANK, (uint16_t)(HEIGHT - 3) * WIDTH);
        blit(0, 768);
    }
}

bool saveScreenBuffer()
{
    memcpy(screenBak, screen, sizeof(screen));
    return true;
}

void restoreScreenBuffer()
{
    memcpy(screen, screenBak, sizeof(screen));
    blit(0, 768);
}

void drawText(unsigned char x, unsigned char y, char *s)
{
    static uint16_t off;
    static uint8_t c, n;

    if (y >= HEIGHT)
        y = HEIGHT - 1;
    if (x >= WIDTH) // centered text wider than the screen (34-char prompts)
        x = 0;

    off = xyoff(x, y);
    n = 0;
    while ((c = *s++) && x + n < WIDTH)
    {
        if (c >= 97 && c <= 122)
            c -= 32;
        screen[off + n++] = c;
    }
    blit(off, n);
}

// Uppercase letters render plain white; lowercase, digits and punctuation
// render in the accent color. Row 21 (the end-game score row) always
// renders white, and in-game score-grid text is forced to accent - the
// same selection rules as the CoCo 1/2 client.
void drawTextAlt(unsigned char x, unsigned char y, char *s)
{
    static uint16_t off;
    static uint8_t c, n, mustAlt, alt;

    if (y >= HEIGHT)
        y = HEIGHT - 1;
    if (x >= WIDTH)
        x = 0;

    mustAlt = state.inGame && x > SCORES_X + 5 && y < 21;

    off = xyoff(x, y);
    n = 0;
    while ((c = *s++) && x + n < WIDTH)
    {
        alt = mustAlt || (y != 21 && (c < 65 || c > 90));
        if (c >= 97 && c <= 122)
            c -= 32;
        if (alt && c >= 0x20 && c <= 0x5A)
            c += 0x80;
        screen[off + n++] = c;
    }
    blit(off, n);
}

void drawChar(unsigned char x, unsigned char y, char c, unsigned char alt)
{
    static uint8_t ch;
    ch = (uint8_t)c;
    if (ch >= 97 && ch <= 122)
        ch -= 32;
    if (alt && ch >= 0x20 && ch <= 0x5A)
        ch += 0x80;
    cell(xyoff(x, y), ch);
}

void drawIcon(unsigned char x, unsigned char y, unsigned char icon)
{
    cell(xyoff(x, y), icon);
}

void drawBlank(unsigned char x, unsigned char y)
{
    cell(xyoff(x, y), TILE_BLANK);
}

void drawSpace(unsigned char x, unsigned char y, unsigned char w)
{
    static uint16_t off;

    if (y >= HEIGHT)
        y = HEIGHT - 1;
    if (x >= WIDTH)
        x = 0;
    if (w > WIDTH - x)
        w = WIDTH - x;

    off = xyoff(x, y);
    memset(screen + off, TILE_BLANK, w);
    blit(off, w);
}

void drawLine(unsigned char x, unsigned char y, unsigned char w)
{
    static uint16_t off;

    if (y >= HEIGHT)
        y = HEIGHT - 1;
    if (x >= WIDTH)
        x = 0;
    if (w > WIDTH - x)
        w = WIDTH - x;

    off = xyoff(x, y);
    memset(screen + off, TILE_HTHICK, w);
    blit(off, w);
}

void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h)
{
    static uint8_t r;
    static uint16_t off;

    off = xyoff(x, y);
    screen[off] = TILE_BOX_TL;
    memset(screen + off + 1, TILE_HTHICK, w);
    screen[off + w + 1] = TILE_BOX_TR;
    blit(off, w + 2);

    for (r = 0; r < h; r++)
    {
        off += WIDTH;
        screen[off] = TILE_BOX_SIDE;
        screen[off + w + 1] = TILE_BOX_SIDE;
        blit(off, w + 2);
    }

    off += WIDTH;
    screen[off] = TILE_BOX_BL;
    memset(screen + off + 1, TILE_HBOTTOM, w);
    screen[off + w + 1] = TILE_BOX_BR;
    blit(off, w + 2);
}

void drawDie(unsigned char x, unsigned char y, unsigned char s, bool isSelected, bool isHighlighted)
{
    static const uint8_t *src;
    static uint8_t r, c, acc;
    static uint16_t off;

    // Invalid index, or the bottom border-screen corners the CoCo also
    // skips (the status line at HEIGHT-1 runs under that column)
    if (!s || s > 16 || y == HEIGHT - 3)
        return;

    src = diceChars + (s - 1) * 9;
    if (isSelected)
        src += 54;

    // Highlighted (and top border) dice render from the accent bank
    acc = (isHighlighted || y == 0) ? 0x80 : 0;

    for (r = 0; r < 3; r++)
    {
        off = xyoff(x, y + r);
        for (c = 0; c < 3; c++)
            screen[off + c] = *src++ + acc;
        blit(off, 3);
    }
}

void drawFujitzee(unsigned char x, unsigned char y)
{
    static uint16_t off;

    if (y >= HEIGHT)
        y = HEIGHT - 1;
    off = xyoff(x, y);
    screen[off] = 0x1B;
    screen[off + 1] = 0x1C;
    screen[off + 2] = 0x1D;
    screen[off + 3] = 0x1E;
    screen[off + 4] = 0x1E;
    blit(off, 5);
}

void drawClock(unsigned char x, unsigned char y)
{
    cell(xyoff(x, y), 0x25);
    cell(xyoff(x + 1, y), 0x26);
}

void drawConnectionIcon(unsigned char x, unsigned char y)
{
    cell(xyoff(x, y), 0x5C);
    cell(xyoff(x + 1, y), 0x5D);
}

/// @brief true if x is one of the interior score-column dividers
static uint8_t isVert(uint8_t x)
{
    return x >= SCORES_X + 9 && x < 31 && ((x - SCORES_X - 9) & 3) == 0;
}

/// @brief Redraw the board's bottom edge across row 20 (x=SCORES_X..30);
/// corners at 1 and 31 belong to the label/main boxes.
static void drawBoardBottomEdge()
{
    static uint8_t x, g;
    static uint16_t off;

    off = xyoff(0, 20);
    screen[off + 1] = TILE_BOX_BL;
    for (x = SCORES_X; x < 31; x++)
    {
        g = (x == SCORES_X + 5) ? TILE_TBOTTOM : TILE_HBOTTOM;
        // Keep the active column's outline blue through redraws
        if (highlightX >= 0 &&
            (x == SCORES_X + 5 + highlightX * 4 || x == SCORES_X + 9 + highlightX * 4))
            g += 0x80;
        screen[off + x] = g;
    }
    screen[off + 31] = TILE_BOX_BR;
}

void drawBoard()
{
    static uint8_t x, y;
    static uint16_t off;

    // Vertical score-column dividers (rows 1-19; row 0 gets cross glyphs)
    for (x = SCORES_X + 9; x < 31; x += 4)
        for (y = 1; y < 20; y++)
            screen[xyoff(x, y)] = TILE_VLINE;

    // Main scores box and label box
    drawBox(SCORES_X + 5, 0, 23, 19);
    drawBox(SCORES_X - 1, 2, 5, 17);

    // Dividers crossing the main box's top edge
    for (x = SCORES_X + 9; x < 31; x += 4)
        screen[xyoff(x, 0)] = TILE_VXTHICK;

    // Thick rule under the header row
    off = xyoff(0, 2);
    for (x = SCORES_X; x < 31; x++)
        screen[off + x] = (x == SCORES_X + 5) ? TILE_SXTHICK
                        : isVert(x)           ? TILE_VXTHICK
                                              : TILE_HTHICK;

    // Thin rules between the scoring sections
    for (y = 9; y <= 12; y += 3)
    {
        off = xyoff(0, y);
        for (x = SCORES_X; x < 31; x++)
            screen[off + x] = (x == SCORES_X + 5) ? TILE_SXTHIN
                            : isVert(x)           ? TILE_VXTHIN
                                                  : TILE_HTHIN;
    }

    // Bottom edge (main + label box bottoms merge into one rule)
    drawBoardBottomEdge();

    // Score-row labels
    for (y = 0; y < 14; y++)
        drawTextAlt(SCORES_X, scoreY[y], scores[y]);

    drawFujitzee(SCORES_X, scoreY[14]);

    blit(0, 768);
}

void clearBelowBoard()
{
    memset(screen + xyoff(0, 20), TILE_BLANK, 4 * WIDTH);
    drawBoardBottomEdge();
    blit(xyoff(0, 20), 4 * WIDTH);
    cursorActive = false;
}

/// @brief Move the active player's column outline in or out of the accent
/// bank. Idempotent (bitwise), so redraw paths can re-apply it safely.
static void translateColumn(uint8_t player, uint8_t on)
{
    static uint8_t y, i, x;
    static uint16_t off;

    for (i = 0; i < 2; i++)
    {
        x = SCORES_X + 5 + player * 4 + i * 4;
        off = x;
        for (y = 0; y <= 20; y++)
        {
            cell(off, on ? (screen[off] | 0x80) : (screen[off] & 0x7F));
            off += WIDTH;
        }
    }
}

void setHighlight(int8_t player, bool isThisPlayer, uint8_t flash)
{
    (void)isThisPlayer;

    if (state.drawBoard)
    {
        highlightX = player;
        return;
    }
    if (flash)
        return;

    waitvsync();

    if (highlightX >= 0 && highlightX != player)
        translateColumn(highlightX, 0);

    highlightX = player;

    if (player >= 0)
        translateColumn(player, 1);
}

// ---- Dice cursor: accent bars in the gap columns flanking the die ----

void drawDiceCursor(unsigned char x)
{
    static uint8_t r;
    static uint16_t off;

    if (x == ROLL_X - 1)
        x++;

    if (cursorActive)
        hideDiceCursor(cursorX);
    cursorX = x;

    off = xyoff(x - 1, HEIGHT - 4);
    for (r = 0; r < 3; r++)
    {
        cursorSave[r] = screen[off];
        cursorSave[r + 3] = screen[off + 4];
        cell(off, TILE_CURSOR_L);
        cell(off + 4, TILE_CURSOR_R);
        off += WIDTH;
    }
    cursorActive = true;
}

void hideDiceCursor(unsigned char x)
{
    static uint8_t r;
    static uint16_t off;

    (void)x;
    if (!cursorActive)
        return;

    off = xyoff(cursorX - 1, HEIGHT - 4);
    for (r = 0; r < 3; r++)
    {
        cell(off, cursorSave[r]);
        cell(off + 4, cursorSave[r + 3]);
        off += WIDTH;
    }
    cursorActive = false;
}

#endif /* __ADAM__ */

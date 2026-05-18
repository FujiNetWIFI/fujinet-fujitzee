#ifdef _CMOC_VERSION_

/*
  Graphics functionality
*/


#include "hires.h"
#include <peekpoke.h>
#include <string.h>
#include "../platform-specific/graphics.h"
#include "../platform-specific/sound.h"
#include "../misc.h"
#include <coco.h>

#ifdef COCO3

/*===========================================================================
  CoCo 3 graphics: 320x200x16, 40x25 character grid. Layout follows the
  MS-DOS port; the 4-color font (charset_coco3.h) is recolored at render
  time through text_palettes[] so the screen can use all 16 colors.
===========================================================================*/

// Hardware palette ($FFB0). Approximates the Atari port's color scheme.
// Index meaning is referenced by text_palettes[] and the COL_* defines.
const unsigned char palette[16] = {
     0,  // 0  black
    63,  // 1  white
     9,  // 2  table blue (background)
    27,  // 3  cyan
    52,  // 4  gold (grid lines)
    36,  // 5  red (highlight)
    10,  // 6  dark blue-green (highlighted column background)
    56,  // 7  light gray
     4,  // 8  dark red
    11,  // 9  light blue
    18,  // 10 green
    54,  // 11 yellow
     7,  // 12 dark gray
    45,  // 13 pink
     2,  // 14 dark green
    63,  // 15 white
};

// Maps the 4 source colors of the 2bpp font {bg, secondary, black, fg}
// onto hardware palette indices, one row per PAL_* selector.
const unsigned char text_palettes[PAL_COUNT][4] = {
    /* PAL_NORMAL */ { 2, 4, 0, 1 },  // fg white
    /* PAL_ALT    */ { 2, 4, 0, 3 },  // fg cyan
    /* PAL_HILITE */ { 2, 4, 0, 5 },  // fg red
    /* PAL_DIM    */ { 2, 4, 0, 7 },  // fg gray
};

#define BG_FILL         0x22  // packed 4bpp byte: both pixels = COL_BG (COL_BG/COL_BG_HILITE in hires.h)

// The 32K screen lives in MMU blocks 52-55, swapped in at $8000 by task 1.
// Blocks 56-59 are the program's low half, present in both tasks.
static const unsigned char task1MMUBlocks[8] = {
    56, 57, 58, 59,   // low half (do not change)
    52, 53, 54, 55,   // graphics half
};

static unsigned char paletteBackup[16];
static uint16_t oldGime;
static int8_t highlightX = -1;

// First cell of the active score column (or -1). Read by hires.c to keep
// the highlight intact as score values are redrawn over it.
int8_t hiliteCol = -1;

unsigned char colorMode = 0;

// Dice / roll-button tile layout, ported from the MS-DOS port. Highlighted
// and pushed variants are produced by palette selection, not extra glyphs.
static const unsigned char diceChars[] = {
    /* Normal dice (s 1-6) */
    0x41,0x40,0x42, 0x40,0x45,0x40, 0x43,0x40,0x44,
    0x41,0x40,0x47, 0x40,0x40,0x40, 0x48,0x40,0x44,
    0x41,0x40,0x47, 0x40,0x45,0x40, 0x48,0x40,0x44,
    0x46,0x40,0x47, 0x40,0x40,0x40, 0x48,0x40,0x49,
    0x46,0x40,0x47, 0x40,0x45,0x40, 0x48,0x40,0x49,
    0x46,0x40,0x47, 0x4A,0x40,0x4B, 0x48,0x40,0x49,

    /* Kept dice (s 1-6 with isSelected) */
    0x21,0x4C,0x22, 0x20,0x25,0x20, 0x23,0x4D,0x24,
    0x21,0x4C,0x27, 0x20,0x20,0x20, 0x28,0x4D,0x24,
    0x21,0x4C,0x27, 0x20,0x25,0x20, 0x28,0x4D,0x24,
    0x26,0x4C,0x27, 0x20,0x20,0x20, 0x28,0x4D,0x29,
    0x26,0x4C,0x27, 0x20,0x25,0x20, 0x28,0x4D,0x29,
    0x26,0x4C,0x27, 0x2A,0x20,0x2B, 0x28,0x4D,0x29,

    /* 13 - empty space */
    0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00,

    /* 14-16 - "Roll" button: 1 left / 2 left / cannot roll */
    0x41,0x40,0x42, 0x2C,0x2D,0x2E, 0x43,0x30,0x44,
    0x41,0x40,0x42, 0x2C,0x2D,0x2E, 0x43,0x2F,0x44,
    0x41,0x40,0x42, 0x2C,0x2D,0x2E, 0x43,0x50,0x44,
};

void waitvsync()
{
    asm { sync }
}

unsigned char cycleNextColor() { return 0; }
void setColorMode() { }

void initGraphics()
{
    initCoCoSupport();
    buildExpandTables();

    disableInterrupts();

    // Set up task 1's MMU map (graphics blocks).
    memcpy((void *)0xFFA8, task1MMUBlocks, sizeof(task1MMUBlocks));

    memcpy(paletteBackup, (void *)0xFFB0, 16);
    memcpy((void *)0xFFB0, palette, 16);

    asm { sync } // wait for v-sync before switching graphics mode

    *(unsigned char *)0xFF90 = 0x4C; // reset CoCo 2 compatible bit
    *(unsigned char *)0xFF98 = 0x80; // graphics mode

    // GIME mode: 200 scan lines, 160 bytes/row, 2 pixels/byte -> 320x200x16
    *(unsigned char *)0xFF99 = 0x3E;
    *(unsigned char *)0xFF9A = 0;    // black border

    // Screen lives in MMU block 52: physical $68000 / 8 = $D000.
    oldGime = *(uint16_t *)0xFF9D;
    *(uint16_t *)0xFF9D = 0xD000;

    enableInterrupts();

    resetScreen(false);
}

void resetGraphics()
{
    task0();
    *(uint16_t *)0xFF9D = oldGime;
    memcpy((void *)0xFFB0, paletteBackup, 16);
    width(32);
}

void resetScreen(bool forBorderScreen)
{
    if (!forBorderScreen) {
        highlightX = -1;
        hiliteCol = -1;
        hires_Mask(0, 0, WIDTH, HEIGHT, BG_FILL);
    } else {
        // Border screen: clear everything except the 3x3 dice corners, so
        // stale title/status text in the edge rows does not bleed through.
        hires_Mask(0, 3,          WIDTH,     HEIGHT - 6, BG_FILL); // interior
        hires_Mask(3, 0,          WIDTH - 6, 3,          BG_FILL); // top
        hires_Mask(3, HEIGHT - 3, WIDTH - 6, 3,          BG_FILL); // bottom
    }
}

void drawText(unsigned char x, unsigned char y, char *s)
{
    unsigned char c;
    while ((c = (unsigned char)*s++)) {
        if (c >= 32 && c < 65) c -= 32;
        hires_putc(x++, y, PAL_NORMAL, c);
    }
}

// Uppercase letters render plain white; lowercase, digits and punctuation
// render in the alt color.
void drawTextAlt(unsigned char x, unsigned char y, char *s)
{
    unsigned char c, pal;
    while ((c = (unsigned char)*s++)) {
        pal = PAL_ALT;
        if (c >= 32 && c < 65) c -= 32;
        if (c >= 'A' && c <= 'Z') { c += 32; pal = PAL_NORMAL; }
        hires_putc(x++, y, pal, c);
    }
}

void drawChar(unsigned char x, unsigned char y, char c, unsigned char alt)
{
    unsigned char ch = (unsigned char)c;
    if (ch >= 32 && ch < 65) ch -= 32;
    if (alt && ch >= 'A' && ch <= 'Z') ch += 32;
    hires_putc(x, y, alt ? PAL_ALT : PAL_NORMAL, ch);
}

void drawIcon(unsigned char x, unsigned char y, unsigned char icon)
{
    hires_putc(x, y, PAL_NORMAL, icon);
}

void drawBlank(unsigned char x, unsigned char y)
{
    hires_putc(x, y, PAL_NORMAL, 0x00);
}

void drawSpace(unsigned char x, unsigned char y, unsigned char w)
{
    hires_Mask(x, y, w, 1, BG_FILL);
}

void drawClock(unsigned char x, unsigned char y)
{
    hires_putc(x, y, PAL_NORMAL, 0x37);
}

void drawConnectionIcon(unsigned char x, unsigned char y)
{
    hires_putc(x,   y, PAL_NORMAL, 0x03);
    hires_putc(x+1, y, PAL_NORMAL, 0x04);
}

void drawFujitzee(unsigned char x, unsigned char y)
{
    hires_putc(x-1, y, PAL_NORMAL, 0x38);
    hires_putc(x,   y, PAL_NORMAL, 0x39);
    hires_putc(x+1, y, PAL_NORMAL, 0x3A);
    hires_putc(x+2, y, PAL_NORMAL, 0x3B);
    hires_putc(x+3, y, PAL_NORMAL, 0x3C);
    hires_putc(x+4, y, PAL_NORMAL, 0x3D);

    // Glyph 0x38 carries the score-box left border for the in-game
    // placement. For a standalone logo (x != SCORES_X) erase that border
    // bar - the left 4 pixels - keeping the logo's bottom tail intact.
    if (x != SCORES_X) {
        unsigned char r;
        unsigned char *pos = (unsigned char *)SCREEN
                           + (uint16_t)y * 1280 + (uint16_t)(x - 1) * 4;
        BEGIN_GFX
        for (r = 0; r < 8; r++) {
            pos[0] = BG_FILL;
            pos[1] = BG_FILL;
            pos += 160;
        }
        END_GFX
    }
}

void drawLine(unsigned char x, unsigned char y, unsigned char w)
{
    while (w--) hires_putc(x++, y, PAL_NORMAL, 0x52);
}

void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h)
{
    unsigned char i;
    hires_putc(x, y, PAL_NORMAL, 0x51);
    for (i = 0; i < w; i++) hires_putc(x+1+i, y, PAL_NORMAL, 0x52);
    hires_putc(x+w+1, y, PAL_NORMAL, 0x55);

    for (i = 0; i < h; i++) {
        hires_putc(x,     y+1+i, PAL_NORMAL, 0x7C);
        hires_putc(x+w+1, y+1+i, PAL_NORMAL, 0x7C);
    }

    hires_putc(x, y+h+1, PAL_NORMAL, 0x57);
    for (i = 0; i < w; i++) hires_putc(x+1+i, y+h+1, PAL_NORMAL, 0x52);
    hires_putc(x+w+1, y+h+1, PAL_NORMAL, 0x5A);
}

void drawDie(unsigned char x, unsigned char y, unsigned char s, bool isSelected, bool isHighlighted)
{
    const unsigned char *src;
    unsigned char r, c, pal;
    if (!s || s > 16) return;

    src = diceChars + (s - 1) * 9;
    if (isSelected) src += 54;
    // Highlighted dice flash red; the highlighted Roll button uses cyan.
    pal = isHighlighted ? (s >= 14 ? PAL_ALT : PAL_HILITE) : PAL_NORMAL;

    for (r = 0; r < 3; r++)
        for (c = 0; c < 3; c++)
            hires_putc(x + c, y + r, pal, *src++);
}

void drawBoard()
{
    unsigned char x, y;

    // Thin horizontal rules.
    for (x = 9;  x < 40; x++) hires_putc(x, 9,  PAL_NORMAL, 0x54);
    for (x = 10; x < 40; x++) hires_putc(x, 12, PAL_NORMAL, 0x54);
    for (x = 0;  x <  9; x++) hires_putc(x, 5,  PAL_NORMAL, 0x54);

    // Thick horizontal rules.
    for (x = 16; x < 40; x++) hires_putc(x, 0,  PAL_NORMAL, 0x52);
    for (x = 10; x < 40; x++) hires_putc(x, 2,  PAL_NORMAL, 0x52);
    for (x = 10; x < 40; x++) hires_putc(x, 20, PAL_NORMAL, 0x52);

    drawBox(9, 2, 5, 17);

    // Vertical dividers.
    for (y = 0; y < 20; y++)
        for (x = 15; x <= 39; x += 4)
            hires_putc(x, y, PAL_NORMAL, (y == 0) ? 0x5B : 0x7C);

    // Cross / T tiles at the rule intersections.
    hires_putc(9, 9,  PAL_NORMAL, 0x53);
    hires_putc(9, 12, PAL_NORMAL, 0x53);
    for (x = 15; x <= 39; x += 4) {
        hires_putc(x, 2,  PAL_NORMAL, 0x50);
        hires_putc(x, 9,  PAL_NORMAL, 0x4F);
        hires_putc(x, 12, PAL_NORMAL, 0x4F);
        hires_putc(x, 20, PAL_NORMAL, 0x58);
    }

    hires_putc(15, 0, PAL_NORMAL, 0x51);

    // Right corners / tees for the rightmost vertical.
    hires_putc(39, 0,  PAL_NORMAL, 0x55);
    hires_putc(39, 2,  PAL_NORMAL, 0x60);
    hires_putc(39, 9,  PAL_NORMAL, 0x5F);
    hires_putc(39, 12, PAL_NORMAL, 0x5F);
    hires_putc(39, 20, PAL_NORMAL, 0x5A);

    // Score-row labels.
    for (y = 0; y < 14; y++)
        drawTextAlt(SCORES_X, scoreY[y], scores[y]);

    drawFujitzee(SCORES_X, scoreY[14]);
}

void clearBelowBoard()
{
    hires_Mask(0, HEIGHT - 4, WIDTH, 4, BG_FILL);
}

// Active-column highlight: remap background pixels in the active player's
// three interior cells (rows 0..20) between COL_BG and COL_BG_HILITE.
static void hilite_column(unsigned char player, bool on)
{
    unsigned char cx = SCORES_X + 6 + player * 4;
    unsigned char from = on ? COL_BG : COL_BG_HILITE;
    unsigned char to   = on ? COL_BG_HILITE : COL_BG;
    unsigned char fromHi = from << 4, toHi = to << 4;
    uint16_t row;

    // Tint only between the box rules (cells 1..19) so it does not bleed
    // over the top/bottom borders.
    BEGIN_GFX
    for (row = 8; row < 20 * 8; row++) {
        unsigned char *pos = (unsigned char *)SCREEN + row * 160 + (uint16_t)cx * 4;
        unsigned char j, b, hi, lo;
        for (j = 0; j < 12; j++) {
            b = pos[j];
            hi = b & 0xF0;
            lo = b & 0x0F;
            if (hi == fromHi) hi = toHi;
            if (lo == from)   lo = to;
            pos[j] = hi | lo;
        }
    }
    END_GFX
}

void setHighlight(int8_t player, bool isThisPlayer, uint8_t flash)
{
    (void)isThisPlayer; (void)flash;
    if (highlightX != player) {
        if (highlightX >= 0) {
            hiliteCol = -1;
            hilite_column(highlightX, false);
        }
        highlightX = player;
    }
    if (player >= 0) {
        hiliteCol = SCORES_X + 6 + player * 4;
        hilite_column(player, true);
    }
}

// ---- Dice cursor: a chunky frame saved/restored cell by cell ----
static const int8_t  curDX[16] = { -1,0,1,2,3,  -1,0,1,2,3,  -1,-1,-1,  3,3,3 };
static const unsigned char curDY[16] = { 0,0,0,0,0,   4,4,4,4,4,   1, 2, 3,  1,2,3 };
static unsigned char cursorSave[16][32];
static unsigned char cursorX;
static bool cursorActive = false;

// Both helpers assume task 1 is already mapped in.
static void cell_io(unsigned char cx, unsigned char cy, unsigned char *buf, bool save)
{
    unsigned char *pos = (unsigned char *)SCREEN + (uint16_t)cy * 1280 + (uint16_t)cx * 4;
    unsigned char r, j;
    for (r = 0; r < 8; r++) {
        for (j = 0; j < 4; j++) {
            if (save) *buf++ = pos[j];
            else      pos[j] = *buf++;
        }
        pos += 160;
    }
}

static void set_pixel(uint16_t px, uint16_t py, unsigned char c)
{
    unsigned char *p = (unsigned char *)SCREEN + py * 160 + (px >> 1);
    if (px & 1) *p = (*p & 0xF0) | c;
    else        *p = (*p & 0x0F) | (c << 4);
}

// Gold/cyan checkerboard color for a pixel: gold (4) and cyan (3) alternate
// per pixel so the 2px-thick frame reads as a checkered border.
#define CKR(px, py) ((((px) + (py)) & 1) ? 4 : 3)

// A 2px-thick gold/cyan checkered frame around the 24x24 die at cells
// (x..x+2, 21..23), with a 1px gap to the die and 1px-rounded corners (the
// outer row/col is inset one pixel at each end). All touched pixels lie
// within the 16 perimeter cells saved by drawDiceCursor.
static void draw_cursor_frame(unsigned char x)
{
    uint16_t L = (uint16_t)x * 8 - 3, R = (uint16_t)x * 8 + 26;
    uint16_t T = 165, B = 194;
    uint16_t p;
    // Top edge: outer row rounded, inner row full.
    for (p = L + 1; p <= R - 1; p++) set_pixel(p, T,     CKR(p, T));
    for (p = L;     p <= R;     p++) set_pixel(p, T + 1, CKR(p, T + 1));
    // Bottom edge.
    for (p = L;     p <= R;     p++) set_pixel(p, B - 1, CKR(p, B - 1));
    for (p = L + 1; p <= R - 1; p++) set_pixel(p, B,     CKR(p, B));
    // Left edge.
    for (p = T + 1; p <= B - 1; p++) set_pixel(L,     p, CKR(L, p));
    for (p = T;     p <= B;     p++) set_pixel(L + 1, p, CKR(L + 1, p));
    // Right edge.
    for (p = T + 1; p <= B - 1; p++) set_pixel(R,     p, CKR(R, p));
    for (p = T;     p <= B;     p++) set_pixel(R - 1, p, CKR(R - 1, p));
}

void drawDiceCursor(unsigned char x)
{
    unsigned char i;
    if (cursorActive) hideDiceCursor(cursorX);
    cursorX = x;
    BEGIN_GFX
    for (i = 0; i < 16; i++)
        cell_io(x + curDX[i], 20 + curDY[i], cursorSave[i], true);
    draw_cursor_frame(x);
    END_GFX
    cursorActive = true;
}

void hideDiceCursor(unsigned char x)
{
    unsigned char i, cx, cy;
    (void)x;
    if (!cursorActive) return;
    BEGIN_GFX
    for (i = 0; i < 16; i++) {
        cx = cursorX + curDX[i];
        cy = 20 + curDY[i];
        cell_io(cx, cy, cursorSave[i], false);
    }
    END_GFX
    cursorActive = false;
}

bool saveScreenBuffer()
{
    return false;
}

void restoreScreenBuffer()
{
}

#else /* CoCo 1/2 */

extern unsigned char charset[];
#define OFFSET_Y 4

#define ROP_CPY 0xff

// Mode 4
#define ROP_BLUE 0b01010101
#define ROP_ORANGE 0b10101010
#define BOX_SIDE 0b111100

int8_t highlightX=-1;
bool inBorderScreen=false;
uint8_t box_color = ROP_ORANGE;

uint8_t diceChars[] = {
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


unsigned char cycleNextColor() {
  // No color modes
  return 0;
}

void setColorMode(unsigned char mode) {
 // No-op
}

// Potentially use interrupt in future
// 0x985 - of note for disk
//#define enableInterrupts()  asm("ANDCC", "#$AF")
//#define disableInterrupts() asm("ORCC",  "#$50")
// typedef interrupt void (*ISR)(void);
// interrupt asm void irqISR(void)
// {
//     asm
//     {
// //_dskcon_irqService IMPORT
//         ldb     $FF03
//         bpl     @done           // do nothing if 63.5 us interrupt
//         ldb     $FF02           // 60 Hz interrupt. Reset PIA0, port B interrupt flag.
// @done //      lbsr    _dskcon_irqService
//         rti
//     }
// }
// void setISR(void *vector, ISR newRoutine)
// {
//     byte *isr = * (byte **) vector;
//     *isr = 0x7E;  // JMP extended
//     * (ISR *) (isr + 1) = newRoutine;
// }

void initGraphics() {
  //disableInterrupts();
  //setISR(0xFFF8, irqISR);
  //enableInterrupts();

  initCoCoSupport();

  //if (isCoCo3) {  }

  pmode(4, SCREEN);
  pcls(0);
  screen(1,1);
}

// Red / Blue vertical lines
uint16_t highlight[] = {0b100000, 0b1010000};

void setHighlight(int8_t player, bool isThisPlayer, uint8_t flash ) {
  static uint8_t i;
  if (state.drawBoard) {
    highlightX=player;
    return;
  }
  if (flash)
    return;

  waitvsync();

  // Two passes. First pass hides the previous highlight. Second pass draws the new highlight.
  for(i=0;i<2;i++) {
    if (highlightX>=0 ) {
      if (i==0) {
        if (highlightX!=player) {

          drawBox(SCORES_X+5+highlightX*4,0,3,19);

          if (highlightX>0)
            hires_Mask(SCORES_X+5+highlightX*4,2,1,19*8+5,highlight[i]);

          if (highlightX<5)
            hires_Mask(SCORES_X+9+highlightX*4,2,1,19*8+5,highlight[i]);

          // Redraw horizontal board lines
          // Thin horz ines
          hires_Mask(SCORES_X,9*8,29,1, 0xff & ROP_ORANGE);
          hires_Mask(SCORES_X,12*8,29,1, 0xff & ROP_ORANGE);

          // Thick horz lines
          hires_Mask(SCORES_X+6,0, 23,2, 0xff & ROP_ORANGE);
          hires_Mask(SCORES_X,2*8,29,2, 0xff & ROP_ORANGE);
          hires_Mask(SCORES_X,20*8-1,29,2, 0xff & ROP_ORANGE);
        }
      } else {
        box_color=ROP_BLUE;
        drawBox(SCORES_X+5+highlightX*4,0,3,19);
        box_color=ROP_ORANGE;
      }
    }

    highlightX=player;
  }
}

bool saveScreenBuffer() {
  // No room on CoCo 32K for second page
  return false;
}

void restoreScreenBuffer() {
  // No-op on CoCo
}

void drawText(unsigned char x, unsigned char y, char* s) {
  static unsigned char c;
  if (y==255)
    y=scoreY[14]*8-OFFSET_Y+1;
  else {
    y=y*8-OFFSET_Y;
    if (y==8*(HEIGHT-1)-OFFSET_Y) {
      y=182;
    } else if (y==8*(HEIGHT-3)-OFFSET_Y) {
      y=162;
    }
  }


  while(c=*s++) {
    if (c>=97 && c<=122) c-=32;
    hires_putc(x++,y,ROP_CPY,c);
  }
}

void drawChar(unsigned char x, unsigned char y, char c, unsigned char alt) {
  if (c>=97 && c<=122) c-=32;
  hires_putc(x,y*8-OFFSET_Y,alt ? ROP_BLUE : ROP_CPY,c);
}

void drawTextAlt(unsigned char x, unsigned char y, char* s) {
  static unsigned char c, mustAlt;
  static uint16_t rop;

  mustAlt = state.inGame && x>SCORES_X+5 && y<21;

  y=y*8-OFFSET_Y;
  if (y==8*(HEIGHT-1)-OFFSET_Y) {
    y=182;
  }

  while(c=*s++) {
    if (mustAlt) {
      rop = ROP_BLUE;
    } else if (y!=21*8-OFFSET_Y  && (c<65 || c> 90)) {
      rop = ROP_BLUE;
    } else {
      rop=ROP_CPY;
    }

    if (c>=97 && c<=122) c-=32;
    hires_putc(x++,y,rop,c);
  }
}


void resetScreen(bool forBorderScreen) {
  if (!forBorderScreen) {
    // Clear entire screen
    pcls(0);
  } else {
    // Moving from one border screen to next - just clear non border area
     hires_Mask(3,0,WIDTH-6,24,0);
     hires_Mask(0,24,WIDTH,168,0);
  }
}

void drawDie(unsigned char x, unsigned char y, unsigned char s, bool isKept, bool isHighlighted) {
  static unsigned char i,j,rop;
  static const unsigned char *source;

  // Don't draw die if invalid index passed in
  if (!s || s>16 || y==HEIGHT-3)
    return;

  // Change the dice color
  rop=ROP_CPY;
  if (isHighlighted || y==0 || y==HEIGHT-3) {
   rop=ROP_BLUE;
  }

  // Locate the diceChar index for this die number
  source=diceChars + (s-1)*9 ;

  // Is die being kept?
  if (isKept)
    source+=54;

  // Draw the dice to the screen
  y*=8;

  // If drawing the bottom dice, offset them lower to make room
  if (y==160) {
    y=165;
  }

  for (i=0;i<3;i++) {
    for (j=0;j<3;j++) {
      hires_Draw(x+j,y,1,8,rop,&charset[(uint16_t)*(source++)<<3]);
    }
    y+=8;
  }
}

void drawIcon(unsigned char x, unsigned char y, unsigned char icon) {
  hires_putc(x,y==HEIGHT-1 ? 182 : y*8-OFFSET_Y,ROP_CPY, icon);
}

void drawClock(unsigned char x, unsigned char y) {
  hires_putcc(x,y*8-OFFSET_Y,ROP_CPY, 0x2526);
}

void drawConnectionIcon(unsigned char x, unsigned char y) {
  hires_putcc(x,y==HEIGHT-1 ? 182 :y*8-OFFSET_Y,ROP_CPY, 0x5c5d);
  // Hard code Y to save space for now
  //hires_putcc(x,182,ROP_CPY, 0x5c5d);
}

void clearBelowBoard() {
  hires_Mask(0,161,40,31,0);
}

void drawBlank(unsigned char x, unsigned char y) {
  hires_putc(x,y*8-OFFSET_Y,ROP_CPY, 0x20);
}

void drawSpace(unsigned char x, unsigned char y, unsigned char w) {
  hires_Mask(x,y==HEIGHT-3 ? 162 : y*8-OFFSET_Y,w,8,0);
}

void drawBoard() {
  static uint8_t y,x;

  // Vertical lines
  for (x=SCORES_X+9;x<32;x+=4) {
    hires_Mask(x,0,1,160,0x20);
  }

  // Main scores box
  drawBox(SCORES_X+5,0,23,19);
  drawBox(SCORES_X-1,2,5,17);

  // Thin horz ines
  hires_Mask(SCORES_X,9*8,29,1, 0xff & ROP_ORANGE);
  hires_Mask(SCORES_X,12*8,29,1, 0xff & ROP_ORANGE);

  // Thick horz lines
  hires_Mask(SCORES_X,2*8,29,2, 0xff & ROP_ORANGE);
  hires_Mask(SCORES_X,20*8-1,29,2, 0xff & ROP_ORANGE);

  // Score names (16 for end game score)
  for(y = 0; y<14; y++) {
    drawTextAlt(SCORES_X,scoreY[y],scores[y]);
  }

  // Fujitzee score text
  drawFujitzee(SCORES_X,255);
}

void drawFujitzee(unsigned char x, unsigned char y) {
  char fujitzee[] = {0x1b, 0x1c, 0x1d, 0x1e, 0x1e, 0};
  drawText(x,y,&fujitzee);
}


void drawLine(unsigned char x, unsigned char y, unsigned char w) {
  if (y==HEIGHT) {
    y=191;
  } else {
    y=y*8-OFFSET_Y+1;
  }
  hires_Mask(x,y,w,2, ROP_ORANGE);
}

void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h) {
  y=y*8-OFFSET_Y+1;

  // Top Corners
  hires_putc(x,y+3,box_color, 0x3b);
  hires_putc(x+w+1,y+3,box_color, 0x3c);

  // Top/bottom lines
  hires_Mask(x+1,y+3,w,2, box_color);
  hires_Mask(x+1,y+(h+1)*8+2,w,2, box_color);

  // Sides
  for(i=0;i<h;++i) {
    y+=8;
    hires_putc(x,y,box_color, 0x3f);
    hires_putc(x+w+1,y,box_color,0x3f);
  }

  // Bottom Corners
  hires_putc(x,y+7,box_color, 0x3d);
  hires_putc(x+w+1,y+7,box_color, 0x3e);
}

void drawDiceCursor(unsigned char x) {
  if (x==ROLL_X-1)
    x++;

  // Sides
  hires_Mask(x-1,190-27,1,28,0x14);
  hires_Mask(x+3,190-27,1,28,0x14);

  // Top / Bottom
  hires_Mask(x-1,190-28,5,2, ROP_BLUE);
  hires_Mask(x-1,190,5,2, ROP_BLUE);

  // Corners
  hires_Mask(x-1,190-28,1,2, 0x0f & ROP_BLUE);
  hires_Mask(x-1,190,1,2, 0x0f & ROP_BLUE);
  hires_Mask(x+3,190-28,1,2, 0xf0 & ROP_BLUE);
  hires_Mask(x+3,190,1,2, 0xf0 & ROP_BLUE);
}

void hideDiceCursor(unsigned char x) {
  if (x==ROLL_X-1)
    x++;
  hires_Mask(x-1,190-28,5,2, 0);
  hires_Mask(x-1,190,5,2, 0);

  // Sides
  hires_Mask(x-1,190-27,1,28,0);
  hires_Mask(x+3,190-27,1,28,0);
}

void resetGraphics() {
  pmode(0, 0x400);
  pcls(0x60);
  screen(0,0);

  // Future - mount and start Lobby
}

void waitvsync() {
  static uint16_t i;
  i=getTimer();
  while (i==getTimer());
}

#endif /* COCO3 */

#endif

#ifdef __WATCOMC__

#include <i86.h>
#include <dos.h>
#include <conio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "../platform-specific/graphics.h"
#include "../misc.h"
#include "vars.h"

extern unsigned char charset[256][16];

#define VIDEO_RAM_ADDR     ((unsigned char far *)0xB8000000UL)
#define VIDEO_LINE_BYTES   80
#define VIDEO_ODD_OFFSET   0x2000

unsigned char far *video = VIDEO_RAM_ADDR;
unsigned char prevVideoMode;
unsigned char colorMode = 0;
int8_t  highlightX = -1;
int8_t  prevHighlightX = -1;

static unsigned char screenBuffer[0x4000];
static bool          haveSavedScreen = false;
static int8_t        savedHighlightX = -1;

#define MASK_WHITE   0xFF
#define MASK_CYAN    0x55
/* MASK_ALT converts color-3 (white) tile pixels to a highlight color when
 * rendering text/cursors. With the new charset and 4-color palette
 * (0=blue, 1=cyan, 2=black, 3=white), alt text and cursor highlights
 * should be cyan, so MASK_ALT routes through color 1 (mask=0x55) — not
 * color 2 (which is reserved for the black dice dots). */
#define MASK_ALT     0x55

/* Atari-derived dice/roll-button tile layout. The msdos charset has
 * the same game-tile glyphs at the same slot numbers as the Atari
 * port, so this table is a direct port. */
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

/* Forward decls so plot_raw can re-apply highlight after writing. */
static void hilite_cell(unsigned char x, unsigned char y, bool on);
static void hilite_cell_partial(unsigned char x, unsigned char y,
                                unsigned char mask0, unsigned char mask1, bool on);
static void hilite_cell_range(unsigned char x, unsigned char y,
                              unsigned char mask0, unsigned char mask1,
                              unsigned char rstart, unsigned char rend, bool on);

static void plot_raw(unsigned char x, unsigned char y, const unsigned char *tile)
{
    unsigned char i;
    unsigned char col = x << 1;
    unsigned char ybase = y << 3;
    for (i = 0; i < 8; i++) {
        unsigned char r  = ybase + i;
        unsigned char rh = r >> 1;
        unsigned int  ro = (unsigned int)rh * VIDEO_LINE_BYTES + col;
        if (r & 1) ro += VIDEO_ODD_OFFSET;
        video[ro]     = tile[i*2];
        video[ro + 1] = tile[i*2 + 1];
    }

    /* Auto-extend the active-column highlight: if the cell we just wrote
     * is anywhere in the active player's full column box (rows 0..20),
     * re-apply the bg color transform so the highlight survives score
     * updates, cursor moves, and any other tile draws into that area.
     * The column spans 5 cells: hx-1 (left vertical, partial right half),
     * hx..hx+2 (full interior), hx+3 (right vertical, partial left 2px). */
    if (highlightX >= 0 && y <= 20) {
        unsigned char hx = SCORES_X + 6 + (unsigned char)highlightX * 4;
        unsigned char rend = (y == 20) ? 3 : 7;  /* row 20: top half only */
        if (x == hx - 1) {
            hilite_cell_range(x, y, 0x00, 0xAA, 0, rend, true);
        } else if (x >= hx && x <= hx + 2) {
            hilite_cell_range(x, y, 0xAA, 0xAA, 0, rend, true);
        } else if (x == hx + 3) {
            hilite_cell_range(x, y, 0xA0, 0x00, 0, rend, true);
        }
    }
}

/* Plot a charset glyph with optional color mask.
 *  mask = 0xFF -> as-stored (white on black for letters, full-color for tiles)
 *  mask = 0xAA -> render in alt color (red)
 *  mask = 0x55 -> render in cyan
 */
static void plot_glyph(unsigned char x, unsigned char y, unsigned char idx, unsigned char mask)
{
    unsigned char tile[16];
    unsigned char i;
    if (mask == MASK_WHITE) {
        plot_raw(x, y, charset[idx]);
        return;
    }
    for (i = 0; i < 16; i++)
        tile[i] = charset[idx][i] & mask;
    plot_raw(x, y, tile);
}

/* PIT-based ~60Hz frame pacing. The straight CGA retrace poll is
 * unreliable on qemu (the status-port retrace bit isn't always emulated
 * at the correct duty cycle), so animations either freeze or run wild.
 * Reading PIT counter 0 directly gives accurate ~838ns ticks; 19886
 * ticks ~= 16.67ms = one 60Hz frame. */
void waitvsync()
{
    static unsigned int last_pit = 0;
    unsigned int now, elapsed;
    do {
        outp(0x43, 0x00);            /* latch counter 0 */
        now  = (unsigned int)inp(0x40);
        now |= (unsigned int)inp(0x40) << 8;
        elapsed = (unsigned int)(last_pit - now);   /* PIT counts down */
    } while (elapsed < 19886);
    last_pit = now;
}

void initGraphics()
{
    union REGS r;

    r.h.ah = 0x0F;
    int86(0x10, &r, &r);
    prevVideoMode = r.h.al;

    /* CGA mode 5 320x200, palette remapped via VGA so we render the
     * Atari-style 4-color set against a black table background. */
    r.h.ah = 0x00;
    r.h.al = 0x05;
    int86(0x10, &r, &r);

    /* Background = black via the CGA color-select port. */
    outp(0x3D9, 0x30);

    /* VGA palette remap (color 0 and 2 swapped from the original Atari
     * mapping): table background is now blue, blue-shadow elements are
     * black. Dice pips/text and Roll-button text get re-encoded to use
     * color 2 in the tile data so they still render BLACK. */
    r.h.ah = 0x10; r.h.al = 0x00;
    r.h.bl = 0x00; r.h.bh = 0x01;     /* color 0 -> blue (table bg) */
    int86(0x10, &r, &r);

    r.h.ah = 0x10; r.h.al = 0x00;
    r.h.bl = 0x01; r.h.bh = 0x0B;     /* color 1 -> cyan */
    int86(0x10, &r, &r);

    r.h.ah = 0x10; r.h.al = 0x00;
    r.h.bl = 0x02; r.h.bh = 0x00;     /* color 2 -> black (was blue) */
    int86(0x10, &r, &r);

    r.h.ah = 0x10; r.h.al = 0x00;
    r.h.bl = 0x03; r.h.bh = 0x0F;     /* color 3 -> white */
    int86(0x10, &r, &r);
}

void resetGraphics()
{
    union REGS r;
    r.h.ah = 0x00;
    r.h.al = prevVideoMode ? prevVideoMode : 0x03;
    int86(0x10, &r, &r);
}

void resetScreen(bool forBorderScreen)
{
    waitvsync();
    if (!forBorderScreen) {
        _fmemset(&video[0x0000], 0, 0x2000);
        _fmemset(&video[0x2000], 0, 0x2000);
        highlightX = -1;
    } else {
        /* Clear interior, leave dice corners drawn at the four edges. */
        unsigned char r;
        for (r = 3; r < HEIGHT - 3; r++) {
            unsigned char rh = (r << 3) >> 1;
            unsigned int  o  = (unsigned int)rh * VIDEO_LINE_BYTES;
            unsigned char i;
            for (i = 0; i < 8; i++) {
                _fmemset(&video[o + (i >> 1) * VIDEO_LINE_BYTES + ((i & 1) ? VIDEO_ODD_OFFSET : 0)],
                         0, VIDEO_LINE_BYTES);
            }
        }
        for (r = 0; r < 3; r++) {
            unsigned char x;
            for (x = 3; x < WIDTH - 3; x++) {
                plot_glyph(x, r,            0x00, MASK_WHITE);
                plot_glyph(x, HEIGHT-1-r,   0x00, MASK_WHITE);
            }
        }
    }
}

/* drawText: lowercase strings render in white. Atari shifts '0'-'9' down
 * by 32 to land on the digit slots at 0x10-0x19 - we follow that. */
void drawText(unsigned char x, unsigned char y, char *s)
{
    unsigned char c;
    while ((c = (unsigned char)*s++)) {
        if (c >= 32 && c < 65) c -= 32;
        plot_glyph(x++, y, c, MASK_WHITE);
    }
}

/* drawTextAlt: matches the Atari semantics. Uppercase letters render as
 * plain lowercase glyphs in the normal (white) color; lowercase letters,
 * digits, and punctuation render in the alt (highlight) color. */
void drawTextAlt(unsigned char x, unsigned char y, char *s)
{
    unsigned char c;
    while ((c = (unsigned char)*s++)) {
        unsigned char mask = MASK_ALT;
        if (c >= 32 && c < 65) c -= 32;
        if (c >= 'A' && c <= 'Z') {
            c += 32;
            mask = MASK_WHITE;
        }
        plot_glyph(x++, y, c, mask);
    }
}

void drawChar(unsigned char x, unsigned char y, char c, unsigned char alt)
{
    unsigned char ch = (unsigned char)c;
    if (ch >= 32 && ch < 65) ch -= 32;
    if (alt && ch >= 'A' && ch <= 'Z') ch += 32;
    plot_glyph(x, y, ch, alt ? MASK_ALT : MASK_WHITE);
}

void drawIcon(unsigned char x, unsigned char y, unsigned char icon)
{
    plot_glyph(x, y, icon, MASK_WHITE);
}

void drawBlank(unsigned char x, unsigned char y)
{
    plot_glyph(x, y, 0x00, MASK_WHITE);
}

void drawSpace(unsigned char x, unsigned char y, unsigned char w)
{
    while (w--) drawBlank(x++, y);
}

void drawClock(unsigned char x, unsigned char y)
{
    plot_glyph(x, y, 0x37, MASK_WHITE);
}

void drawConnectionIcon(unsigned char x, unsigned char y)
{
    plot_glyph(x,   y, 0x03, MASK_WHITE);
    plot_glyph(x+1, y, 0x04, MASK_WHITE);
}

void drawFujitzee(unsigned char x, unsigned char y)
{
    /* Same six-tile arrangement as Atari: 0x38..0x3D form the "fujiTZEE" logo. */
    plot_glyph(x-1, y, 0x38, MASK_WHITE);
    plot_glyph(x,   y, 0x39, MASK_WHITE);
    plot_glyph(x+1, y, 0x3A, MASK_WHITE);
    plot_glyph(x+2, y, 0x3B, MASK_WHITE);
    plot_glyph(x+3, y, 0x3C, MASK_WHITE);
    plot_glyph(x+4, y, 0x3D, MASK_WHITE);

    /* Glyph 0x38 carries the score-box left border for the in-game
     * placement. For a standalone logo (x != SCORES_X) erase that border
     * bar - the left 4 pixels (byte 0 of the cell) - keeping the tail. */
    if (x != SCORES_X) {
        unsigned char i;
        unsigned char col = (x - 1) << 1;
        unsigned char ybase = y << 3;
        for (i = 0; i < 8; i++) {
            unsigned char r  = ybase + i;
            unsigned int  ro = (unsigned int)(r >> 1) * VIDEO_LINE_BYTES + col;
            if (r & 1) ro += VIDEO_ODD_OFFSET;
            video[ro] = 0;
        }
    }
}

void drawLine(unsigned char x, unsigned char y, unsigned char w)
{
    while (w--) plot_glyph(x++, y, 0x52, MASK_WHITE);
}

void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h)
{
    unsigned char i;
    plot_glyph(x,         y, 0x51, MASK_WHITE);
    for (i = 0; i < w; i++) plot_glyph(x+1+i, y, 0x52, MASK_WHITE);
    plot_glyph(x+w+1,     y, 0x55, MASK_WHITE);

    for (i = 0; i < h; i++) {
        plot_glyph(x,       y+1+i, 0x7C, MASK_WHITE);
        plot_glyph(x+w+1,   y+1+i, 0x7C, MASK_WHITE);
    }

    plot_glyph(x,         y+h+1, 0x57, MASK_WHITE);
    for (i = 0; i < w; i++) plot_glyph(x+1+i, y+h+1, 0x52, MASK_WHITE);
    plot_glyph(x+w+1,     y+h+1, 0x5A, MASK_WHITE);
}

void drawDie(unsigned char x, unsigned char y, unsigned char s, bool isSelected, bool isHighlighted)
{
    const unsigned char *src;
    unsigned char r, c;
    if (!s || s > 16) return;

    src = diceChars + (s - 1) * 9;
    if (isSelected)    src += 54;
    if (isHighlighted) src += (s < 14) ? 171 : 27;

    for (r = 0; r < 3; r++) {
        for (c = 0; c < 3; c++) {
            plot_glyph(x + c, y + r, *src++, MASK_WHITE);
        }
    }
}

/* Direct port of the Atari drawBoard. Uses dedicated T-junction and
 * cross tiles (0x53/0x56/0x58) at every line intersection so the score
 * grid renders as a connected lattice instead of stacked segments. */
void drawBoard()
{
    unsigned char x, y;

    /* Thin horizontal rules (0x54). */
    for (x = 9;  x < 40; x++) plot_glyph(x, 9,  0x54, MASK_WHITE);
    for (x = 10; x < 40; x++) plot_glyph(x, 12, 0x54, MASK_WHITE);
    for (x = 0;  x <  9; x++) plot_glyph(x, 5,  0x54, MASK_WHITE);

    /* Thick horizontal rules (0x52). */
    for (x = 16; x < 40; x++) plot_glyph(x, 0,  0x52, MASK_WHITE);
    for (x = 10; x < 40; x++) plot_glyph(x, 2,  0x52, MASK_WHITE);
    for (x = 10; x < 40; x++) plot_glyph(x, 20, 0x52, MASK_WHITE);

    /* Score-name box. */
    drawBox(9, 2, 5, 17);

    /* Vertical dividers at cols 15, 19, 23, 27, 31, 35, 39; row 0 uses
     * the upper-tee tile (0x5B), other rows use the plain vertical (0x7C). */
    for (y = 0; y < 20; y++) {
        for (x = 15; x <= 39; x += 4)
            plot_glyph(x, y, (y == 0) ? 0x5B : 0x7C, MASK_WHITE);
    }

    /* Cross/T tiles at every intersection of horizontal rules with the
     * vertical dividers. Crosses for each player column live at cols 15,
     * 19, 23, 27, 31, 35, 39 (= SCORES_X+5+player*4 for player=0..5). */
    plot_glyph(9, 9,  0x53, MASK_WHITE);     /* score-box right edge meets thin horiz */
    plot_glyph(9, 12, 0x53, MASK_WHITE);
    for (x = 15; x <= 39; x += 4) {
        plot_glyph(x, 2,  0x50, MASK_WHITE);   /* thick FULL cross (no left gap) */
        plot_glyph(x, 9,  0x4F, MASK_WHITE);   /* thin FULL cross */
        plot_glyph(x, 12, 0x4F, MASK_WHITE);   /* thin FULL cross */
        plot_glyph(x, 20, 0x58, MASK_WHITE);   /* bottom thick (already full horiz) */
    }

    /* Top-left corner where the player columns join the score-name box. */
    plot_glyph(15, 0, 0x51, MASK_WHITE);

    /* Right corners and right-tees for the rightmost vertical at col 39.
     * The default 0x5B/0x53/0x56/0x58 tiles all have horizontal stubs
     * extending past the rightmost vertical. 0x55 and 0x5A are the
     * existing top/bottom-right corners; 0x5F and 0x60 are custom
     * right-tees (mirrors of 0x53 and 0x56) added for the rule rows. */
    plot_glyph(39, 0,  0x55, MASK_WHITE);
    plot_glyph(39, 2,  0x60, MASK_WHITE);
    plot_glyph(39, 9,  0x5F, MASK_WHITE);
    plot_glyph(39, 12, 0x5F, MASK_WHITE);
    plot_glyph(39, 20, 0x5A, MASK_WHITE);

    /* Score-row labels. */
    for (y = 0; y < 14; y++)
        drawTextAlt(SCORES_X, scoreY[y], scores[y]);

    drawFujitzee(SCORES_X, scoreY[14]);
}

/* Atari clears HEIGHT-5..HEIGHT-1 because it has HEIGHT=26 and the
 * score-box bottom border lives at row 20. msdos has HEIGHT=25, so the
 * same offset would erase the box's bottom border. Start one row lower. */
void clearBelowBoard()
{
    unsigned char y;
    for (y = HEIGHT - 4; y < HEIGHT; y++)
        drawSpace(0, y, WIDTH);
}

/* 2-pixel-thick checkerboard cyan(1)/blue(2) frame around the die with
 * a 1-pixel gap. The 30x30 frame sits one pixel outside the die's 24x24
 * footprint on every side. Outer corners are 1-pixel rounded by
 * skipping the four corner pixels of the outermost row/col. Color at
 * each pixel is ((px + py) & 1) ? blue : cyan, so the two layers of
 * each edge are phase-flipped. The 220 touched pixels' original 2-bit
 * colors are saved at draw time so hideDiceCursor can put the screen
 * back exactly. */
#define DICE_PERIM_PIXELS 220
#define CKR(px, py)       ((((px) + (py)) & 1) ? 2 : 1)
static unsigned char diceCursorSave[DICE_PERIM_PIXELS];
static unsigned char diceCursorX = 0;
static bool diceCursorActive = false;

static unsigned char get_pixel_2bpp(unsigned int px, unsigned int py)
{
    unsigned char shift = (3 - (px & 3)) << 1;
    unsigned int  ro    = (unsigned int)(py >> 1) * VIDEO_LINE_BYTES + (px >> 2);
    if (py & 1) ro += VIDEO_ODD_OFFSET;
    return (video[ro] >> shift) & 0x03;
}

static void set_pixel_2bpp(unsigned int px, unsigned int py, unsigned char color)
{
    unsigned char shift = (3 - (px & 3)) << 1;
    unsigned char m     = 0x03 << shift;
    unsigned int  ro    = (unsigned int)(py >> 1) * VIDEO_LINE_BYTES + (px >> 2);
    if (py & 1) ro += VIDEO_ODD_OFFSET;
    video[ro] = (video[ro] & ~m) | ((color & 0x03) << shift);
}

void drawDiceCursor(unsigned char x)
{
    unsigned int fL, fR, fT, fB;
    unsigned int i, idx = 0;
    /* If a cursor is already drawn somewhere, hide it first so we
     * don't overwrite the save buffer with the cursor's own pixels. */
    if (diceCursorActive) hideDiceCursor(diceCursorX);
    diceCursorX = x;
    fL = ((unsigned int)x << 3) - 3;     /* frame left  */
    fR = ((unsigned int)x << 3) + 26;    /* frame right */
    fT = ((unsigned int)(HEIGHT - 4) << 3) - 3;
    fB = ((unsigned int)(HEIGHT - 4) << 3) + 26;

    /* Top outer (rounded: skip cols fL and fR). */
    for (i = 1; i < 29; i++) {
        diceCursorSave[idx++] = get_pixel_2bpp(fL + i, fT);
        set_pixel_2bpp(fL + i, fT, CKR(fL + i, fT));
    }
    /* Top inner. */
    for (i = 0; i < 30; i++) {
        diceCursorSave[idx++] = get_pixel_2bpp(fL + i, fT + 1);
        set_pixel_2bpp(fL + i, fT + 1, CKR(fL + i, fT + 1));
    }
    /* Bottom inner. */
    for (i = 0; i < 30; i++) {
        diceCursorSave[idx++] = get_pixel_2bpp(fL + i, fB - 1);
        set_pixel_2bpp(fL + i, fB - 1, CKR(fL + i, fB - 1));
    }
    /* Bottom outer (rounded). */
    for (i = 1; i < 29; i++) {
        diceCursorSave[idx++] = get_pixel_2bpp(fL + i, fB);
        set_pixel_2bpp(fL + i, fB, CKR(fL + i, fB));
    }
    /* Left outer (rows 2..27 of the frame). */
    for (i = 2; i < 28; i++) {
        diceCursorSave[idx++] = get_pixel_2bpp(fL, fT + i);
        set_pixel_2bpp(fL, fT + i, CKR(fL, fT + i));
    }
    /* Left inner. */
    for (i = 2; i < 28; i++) {
        diceCursorSave[idx++] = get_pixel_2bpp(fL + 1, fT + i);
        set_pixel_2bpp(fL + 1, fT + i, CKR(fL + 1, fT + i));
    }
    /* Right inner. */
    for (i = 2; i < 28; i++) {
        diceCursorSave[idx++] = get_pixel_2bpp(fR - 1, fT + i);
        set_pixel_2bpp(fR - 1, fT + i, CKR(fR - 1, fT + i));
    }
    /* Right outer. */
    for (i = 2; i < 28; i++) {
        diceCursorSave[idx++] = get_pixel_2bpp(fR, fT + i);
        set_pixel_2bpp(fR, fT + i, CKR(fR, fT + i));
    }
    diceCursorActive = true;
}

void hideDiceCursor(unsigned char x)
{
    unsigned int fL, fR, fT, fB;
    unsigned int i, idx = 0;

    if (!diceCursorActive) return;
    /* Always restore at the position the cursor was last drawn — the
     * caller-supplied x can drift if the calling logic changes the
     * cursor target before sending the hide. */
    (void)x;
    fL = ((unsigned int)diceCursorX << 3) - 3;
    fR = ((unsigned int)diceCursorX << 3) + 26;
    fT = ((unsigned int)(HEIGHT - 4) << 3) - 3;
    fB = ((unsigned int)(HEIGHT - 4) << 3) + 26;
    for (i = 1; i < 29; i++) set_pixel_2bpp(fL + i, fT,         diceCursorSave[idx++]);
    for (i = 0; i < 30; i++) set_pixel_2bpp(fL + i, fT + 1,     diceCursorSave[idx++]);
    for (i = 0; i < 30; i++) set_pixel_2bpp(fL + i, fB - 1,     diceCursorSave[idx++]);
    for (i = 1; i < 29; i++) set_pixel_2bpp(fL + i, fB,         diceCursorSave[idx++]);
    for (i = 2; i < 28; i++) set_pixel_2bpp(fL,     fT + i,     diceCursorSave[idx++]);
    for (i = 2; i < 28; i++) set_pixel_2bpp(fL + 1, fT + i,     diceCursorSave[idx++]);
    for (i = 2; i < 28; i++) set_pixel_2bpp(fR - 1, fT + i,     diceCursorSave[idx++]);
    for (i = 2; i < 28; i++) set_pixel_2bpp(fR,     fT + i,     diceCursorSave[idx++]);
    diceCursorActive = false;
}

bool saveScreenBuffer()
{
    _fmemcpy(screenBuffer, VIDEO_RAM_ADDR, 0x4000);
    savedHighlightX = highlightX;
    haveSavedScreen = true;
    return true;
}

void restoreScreenBuffer()
{
    if (!haveSavedScreen) return;
    waitvsync();
    _fmemcpy(VIDEO_RAM_ADDR, screenBuffer, 0x4000);
    highlightX = savedHighlightX;
    haveSavedScreen = false;
}

/* Active-column interior highlight: transform color-0 (blue) pixels to
 * color-2 (black) inside the active player's column cells, so the column
 * has a visibly different background while keeping border-color pixels
 * (gold) and white text intact. Reverse transform flips color-2 back to
 * color-0. Both transforms are idempotent. Applied directly to VRAM so
 * the tile content (text, dividers) is preserved. Avoids the FUJITZEE
 * row (21) and below — that area is shared with the dice. */
static void hilite_byte_in_place(unsigned char far *p, bool on)
{
    unsigned char b = *p;
    if (on) {
        unsigned char hi = b & 0xAA;
        unsigned char lo = b & 0x55;
        unsigned char por_lo = (hi >> 1) | lo;
        unsigned char por_hi = por_lo << 1;
        unsigned char zero_hi = (~por_hi) & 0xAA;
        *p = b | zero_hi;
    } else {
        unsigned char hi = b & 0xAA;
        unsigned char lo_in_hi = (b & 0x55) << 1;
        unsigned char ten_hi = hi & ~lo_in_hi;
        *p = b & ~ten_hi;
    }
}

static void hilite_cell(unsigned char x, unsigned char y, bool on)
{
    unsigned char i;
    unsigned char col = x << 1;
    unsigned char ybase = y << 3;
    for (i = 0; i < 8; i++) {
        unsigned char r  = ybase + i;
        unsigned char rh = r >> 1;
        unsigned int  ro = (unsigned int)rh * VIDEO_LINE_BYTES + col;
        if (r & 1) ro += VIDEO_ODD_OFFSET;
        hilite_byte_in_place(&video[ro],     on);
        hilite_byte_in_place(&video[ro + 1], on);
    }
}

/* Partial-cell variant: keep_mask selects which 0xAA-position bits (the
 * "high bit of each pixel pair") to keep modified. Other pairs are
 * restored to their pre-hilite state. */
static void hilite_byte_partial(unsigned char far *p, unsigned char keep_mask, bool on)
{
    unsigned char orig = *p;
    hilite_byte_in_place(p, on);
    *p = (*p & keep_mask) | (orig & ~keep_mask);
}

/* Apply partial-cell highlight to a sub-range of internal rows [rstart, rend]
 * of cell (x, y). mask0/mask1 select which pixels of byte 0 / byte 1 to
 * affect (0xAA = all 4 pixels, 0xA0 = first 2 pixels, etc.). */
static void hilite_cell_range(unsigned char x, unsigned char y,
                              unsigned char mask0, unsigned char mask1,
                              unsigned char rstart, unsigned char rend, bool on)
{
    unsigned char i;
    unsigned char col = x << 1;
    unsigned char ybase = y << 3;
    for (i = rstart; i <= rend; i++) {
        unsigned char r  = ybase + i;
        unsigned char rh = r >> 1;
        unsigned int  ro = (unsigned int)rh * VIDEO_LINE_BYTES + col;
        if (r & 1) ro += VIDEO_ODD_OFFSET;
        if (mask0) hilite_byte_partial(&video[ro],     mask0, on);
        if (mask1) hilite_byte_partial(&video[ro + 1], mask1, on);
    }
}

static void hilite_cell_partial(unsigned char x, unsigned char y,
                                unsigned char mask0, unsigned char mask1, bool on)
{
    hilite_cell_range(x, y, mask0, mask1, 0, 7, on);
}

static void hilite_column_interior(int8_t player, bool on)
{
    unsigned char x = SCORES_X + 6 + player*4;
    unsigned char y;
    /* Cover the entire box from row 0 (column-header thick rule) down
     * to row 19 (FUJITZEE row). Cells x..x+2 are the full column
     * interior; x-1 (left vertical's cell) gets only its right half
     * highlighted (cols 4-7), and x+3 (right vertical's cell) gets
     * only its leftmost 2 pixels (cols 0-1). The verticals themselves
     * (cyan at cols 2-3) are not color 0 so the highlight transform
     * leaves them intact. */
    for (y = 0; y <= 19; y++) {
        hilite_cell_partial(x - 1, y, 0x00, 0xAA, on);
        hilite_cell(x,     y, on);
        hilite_cell(x + 1, y, on);
        hilite_cell(x + 2, y, on);
        hilite_cell_partial(x + 3, y, 0xA0, 0x00, on);
    }
    /* Row 20 = bottom thick rule cell. Highlight only its top half
     * (internal rows 0-3 = above + including the rule). Skipping the
     * lower half (rows 4-7) prevents the highlight from bleeding below
     * the rule into the dice area. */
    hilite_cell_range(x - 1, 20, 0x00, 0xAA, 0, 3, on);
    hilite_cell_range(x,     20, 0xAA, 0xAA, 0, 3, on);
    hilite_cell_range(x + 1, 20, 0xAA, 0xAA, 0, 3, on);
    hilite_cell_range(x + 2, 20, 0xAA, 0xAA, 0, 3, on);
    hilite_cell_range(x + 3, 20, 0xA0, 0x00, 0, 3, on);
}

void setHighlight(int8_t player, bool isThisPlayer, uint8_t flash)
{
    if (highlightX != player) {
        if (highlightX >= 0) hilite_column_interior(highlightX, false);
        highlightX = player;
    }
    if (player >= 0) hilite_column_interior(player, true);
    (void)isThisPlayer; (void)flash;
}

unsigned char cycleNextColor() { return 0; }
void          setColorMode()   { }

#endif /* __WATCOMC__ */

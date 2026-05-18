#ifdef _CMOC_VERSION_

#ifndef HIRES_H
#define HIRES_H

#include <stdint.h>

void hires_putc(uint8_t x, uint8_t y, uint8_t rop, uint8_t c);
void hires_putcc(uint8_t x, uint8_t y,uint8_t rop, unsigned cc);
void hires_Mask(uint8_t x, uint8_t y, uint8_t xlen, uint8_t ylen, uint8_t c);
void hires_Draw(uint8_t x, uint8_t y, uint8_t xlen, uint8_t ylen, uint8_t rop, char *src);

#ifdef COCO3

// The 32K screen lives in MMU blocks 52-55, swapped in at $8000 via task 1
// whenever graphics are drawn, and swapped out so normal IO / FujiNet
// operations can continue. The local address while task 1 is active is
// $8000 (the 5th 8K block).
#define SCREEN 0x8000U

#define task0() do { asm("clr", "$FF91"); } while (0) /* run program */
#define task1() do { asm("ldb", "#1"); asm("stb", "$FF91"); } while (0) /* graphics */

#define BEGIN_GFX        \
    disableInterrupts(); \
    task1();

#define END_GFX \
    task0();    \
    enableInterrupts();

// Palette selectors passed as the 'rop' argument. They index text_palettes[]
// in graphics.c; each entry maps the 4 source colors of the 2bpp font onto
// hardware palette registers, so the same glyph can be recolored per draw.
#define PAL_NORMAL 0   // white text / standard glyphs
#define PAL_ALT    1   // alt-color (cyan) text and icons
#define PAL_HILITE 2   // red highlight (active dice / cursors)
#define PAL_DIM    3   // dimmed (gray) glyphs
#define PAL_COUNT  4

// Background palette indices used by the active-column highlight.
#define COL_BG          2   // table background
#define COL_BG_HILITE   6   // active-column background

// Builds the 2bpp->4bpp expansion tables; call once from initGraphics().
void buildExpandTables(void);

#else

#define SCREEN 0x0200U // Normally 0x0E00U
#define BEGIN_GFX
#define END_GFX

#endif /* COCO3 */

#endif /* HIRES_H */
#endif

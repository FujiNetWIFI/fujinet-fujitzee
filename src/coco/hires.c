#ifdef _CMOC_VERSION_

#include <stdint.h>
#include <conio.h>
#include <string.h>
#include "../platform-specific/sound.h"
#include "hires.h"

#ifdef COCO3

#include <coco.h>
#include "charset_coco3.h"

// Maps the 4 source colors of the 2bpp font onto hardware palette registers.
extern const unsigned char text_palettes[PAL_COUNT][4];

// First cell of the active player's score column, or -1 if none. Set by
// setHighlight() in graphics.c.
extern int8_t hiliteCol;

// Per-palette 2bpp->4bpp expansion table: maps a source font nibble (2 2bpp
// pixels) to the 4bpp screen byte it becomes. Built once at init so
// hires_Draw is a table lookup per nibble instead of eight palette lookups.
static uint8_t expandNib[PAL_COUNT][16];

void buildExpandTables(void)
{
  uint8_t p, n;
  for (p = 0; p < PAL_COUNT; p++) {
    const unsigned char *pal = text_palettes[p];
    for (n = 0; n < 16; n++)
      expandNib[p][n] = (pal[(n >> 2) & 3] << 4) | pal[n & 3];
  }
}

// Re-tint any cells of [x, x+w) that fall inside the active score column
// (rows 1..19): background pixels go from COL_BG to COL_BG_HILITE. This
// keeps the column highlight intact as score values are drawn over it.
// Assumes task 1 is already mapped in.
static void retint_span(uint8_t x, uint8_t y, uint8_t w)
{
  uint8_t cx, r, j, b;
  uint8_t *pos;
  if (hiliteCol < 0 || y < 1 || y > 19)
    return;
  for (cx = x; cx < x + w; cx++) {
    if (cx < (uint8_t)hiliteCol || cx > (uint8_t)hiliteCol + 2)
      continue;
    pos = (uint8_t *)SCREEN + (uint16_t)y * 1280 + (uint16_t)cx * 4;
    for (r = 0; r < 8; r++) {
      for (j = 0; j < 4; j++) {
        b = pos[j];
        if ((b & 0xF0) == (COL_BG << 4)) b = (b & 0x0F) | (COL_BG_HILITE << 4);
        if ((b & 0x0F) == COL_BG)        b = (b & 0xF0) | COL_BG_HILITE;
        pos[j] = b;
      }
      pos += 160;
    }
  }
}

/*-----------------------------------------------------------------------*/
// Expand one 8x8 2bpp glyph into the 4bpp screen, recoloring through the
// selected palette. x/y are character cells; xlen/ylen are unused (kept
// for interface compatibility - a glyph is always 8x8).
void hires_Draw(uint8_t x, uint8_t y, uint8_t xlen, uint8_t ylen, uint8_t rop, char *src)
{
  const uint8_t *nib = expandNib[rop & 3];
  uint8_t *pos = (uint8_t *)SCREEN + (uint16_t)y * 1280 + (uint16_t)x * 4;
  uint8_t row, hi, lo;
  (void)xlen; (void)ylen;

  BEGIN_GFX
  for (row = 0; row < 8; row++) {
    hi = (uint8_t)src[row * 2];      // pixels 0-3
    lo = (uint8_t)src[row * 2 + 1];  // pixels 4-7
    pos[0] = nib[hi >> 4];
    pos[1] = nib[hi & 15];
    pos[2] = nib[lo >> 4];
    pos[3] = nib[lo & 15];
    pos += 160;
  }
  retint_span(x, y, 1);
  END_GFX
}

/*-----------------------------------------------------------------------*/
void hires_putc(uint8_t x, uint8_t y, uint8_t rop, uint8_t c)
{
  // Bit 7 of a glyph index selects the alt palette (the MSDOS charset
  // stores recolored copies there; here we recolor at render time).
  if ((c & 0x80) && rop == PAL_NORMAL)
    rop = PAL_ALT;
  hires_Draw(x, y, 1, 8, rop, (char *)charset_coco3[c & 0x7F]);
}

/*-----------------------------------------------------------------------*/
void hires_putcc(uint8_t x, uint8_t y, uint8_t rop, uint16_t cc)
{
  hires_putc(x, y, rop, (uint8_t)(cc >> 8));
  hires_putc(++x, y, rop, (uint8_t)cc);
}

// Fill a rectangle of cells with a packed 4bpp byte (both nibbles a color).
// x/y/xlen are cells; ylen is cells.
void hires_Mask(uint8_t x, uint8_t y, uint8_t xlen, uint8_t ylen, uint8_t c)
{
  uint8_t *pos = (uint8_t *)SCREEN + (uint16_t)y * 1280 + (uint16_t)x * 4;
  uint16_t rows = (uint16_t)ylen * 8;

  BEGIN_GFX
  while (rows--) {
    memset(pos, c, (uint16_t)xlen * 4);
    pos += 160;
  }
  retint_span(x, y, xlen);
  END_GFX
}

#else /* CoCo 1/2 */

extern uint8_t charset[];

/*-----------------------------------------------------------------------*/
void hires_putc(uint8_t x, uint8_t y, uint8_t rop, uint8_t c)
{
  hires_Draw(x,y,1,8,rop,&charset[(uint16_t)c<<3]);
}

/*-----------------------------------------------------------------------*/
void hires_putcc(uint8_t x, uint8_t y,uint8_t rop, uint16_t cc)
{
  hires_putc(x,y,rop,cc>>8);
  hires_putc(++x,y,rop,cc);
}

void hires_Mask(uint8_t x, uint8_t y, uint8_t xlen, uint8_t ylen, uint8_t c)
{
  uint8_t *pos = SCREEN+(uint16_t)y*32+x;
  ylen++;
  while (--ylen) {
    memset(pos,c,xlen);
    pos+=32;
  }
}

void hires_Draw(uint8_t x, uint8_t y, uint8_t xlen, uint8_t ylen, uint8_t rop, char *src)
{
  uint8_t *pos = SCREEN+(uint16_t)y*32+x;
  ylen++;
  while (--ylen) {
    *pos=*(src++)&rop;
    pos+=32;
  }
}

#endif /* COCO3 */

#endif

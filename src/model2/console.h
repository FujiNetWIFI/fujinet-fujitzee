#ifndef M2_CONSOLE_H
#define M2_CONSOLE_H

/*
  The Model II screen, drawn by cell codes into a back buffer and sent by
  difference through the P&T video driver (src/model2/console.c). A cell is
  one byte: bit 7 is reverse video, the rest a graphics character (00h-1Fh)
  or ASCII. The interface is the H89's (src/h89/console.h).
*/

#include <stdint.h>
#include <stdbool.h>

#define SCR_W 80
#define SCR_H 24

#define REV 0x80

// Model II graphics characters, from the Operation Manual's chart
#define G_CORNER_UL  0x00
#define G_CORNER_UR  0x01
#define G_CORNER_LR  0x02
#define G_CORNER_LL  0x03
#define G_TEE_DOWN   0x04
#define G_TEE_LEFT   0x05
#define G_TEE_UP     0x06
#define G_TEE_RIGHT  0x07
#define G_CROSS      0x0C
#define G_VLINE      0x14
#define G_HLINE      0x16
#define G_HLINE_THIN 0x17

void termInit(void);
void termExit(void);
void put(uint8_t x, uint8_t y, uint8_t code);
void fill(uint8_t x, uint8_t y, uint8_t w, uint8_t code);
void blankRows(uint8_t y, uint8_t n);

/// @brief The back buffer's cell at x,y (a space off screen)
uint8_t peek(uint8_t x, uint8_t y);

/// @brief Send the screen whatever differs from the back buffer
void flush(void);

void conSave(void);

/// @brief Put back the copy conSave() took (sent at the next flush)
void conRestore(void);

void termBell(void);

/// @brief One byte straight to the BIOS console
void conOut(uint8_t c) __z88dk_fastcall;

/// @brief Raw console byte, or -1 when none is waiting
int conIn(void);

/// @brief P&T's clock, in 1/100 s since boot
uint32_t ticks(void);

#endif /* M2_CONSOLE_H */

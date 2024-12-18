#ifdef _CMOC_VERSION_

#ifndef HIRES_H
#define HIRES_H

#include <stdint.h>

void hires_putc(uint8_t x, uint8_t y, uint8_t rop, uint8_t c);
void hires_putcc(uint8_t x, uint8_t y,uint8_t rop, unsigned cc);
void hires_Mask(uint8_t x, uint8_t y, uint8_t xlen, uint8_t ylen, uint8_t c);
void hires_Draw(uint8_t x, uint8_t y, uint8_t xlen, uint8_t ylen, uint8_t rop, char *src);

#define SCREEN 0x0200U // Normally 0x0E00U

#endif /* HIRES_H */
#endif
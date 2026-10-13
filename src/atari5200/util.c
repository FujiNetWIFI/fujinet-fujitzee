#ifdef BUILD_ATARI5200

/**
 * @brief   Utility Functions - Atari 5200
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 */

#include <stdint.h>
#include <peekpoke.h>
#include "../misc.h"
#include "../platform-specific/util.h"

/* The server sends the packed wire layout straight into clientState.game,
 * so cc65 must not pad Game. Fails the build if it ever does. */
typedef char game_wire_size_check[(sizeof(Game) == 599) ? 1 : -1];

/* The crt0's VBI keeps the BIOS's two-byte frame counter. */
#define RTCLOKH 0x01
#define RTCLOKL 0x02
#define ATRACT  0x04
#define PAL     0xC014

void resetTimer() {
  POKE(RTCLOKL, 0);
  POKE(RTCLOKH, 0);
}

uint16_t getTime() {
  static uint8_t hi, lo;

  // Read again if the VBI carried into the high byte in between
  do {
    hi = PEEK(RTCLOKH);
    lo = PEEK(RTCLOKL);
  } while (hi != PEEK(RTCLOKH));

  return hi * 256 + lo;
}

void housekeeping() {
  // Clear attract mode
  POKE(ATRACT, 0);
}

uint8_t getJiffiesPerSecond() {
  return (PEEK(PAL) & 0x0E) == 0 ? 50 : 60;
}

// quit() is in fujinet.c, with the rest of the cartridge calls.

#endif /* BUILD_ATARI5200 */

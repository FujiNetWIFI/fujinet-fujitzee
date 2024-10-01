#ifndef UTIL_H
#define UTIL_H

/*
  Platform specific utilities that don't fit in any category
*/
#include <stdint.h>

void resetTimer();
int getTime();
void quit();
void housekeeping();
uint8_t getJiffiesPerSecond();

#endif /* UTIL_H */
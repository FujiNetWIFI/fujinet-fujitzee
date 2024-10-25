/*
  Platform specific input. 
*/
#ifndef INPUT_H
#define INPUT_H

// Include platform specific defines before the input include
#include "../atari/vars.h"
#include "../apple2/vars.h"
#include "../coco/vars.h"
//#include "../c64/vars.h"
//#include "../adam/vars.h"


// Platform specific implementations
unsigned char readJoystick();

#endif /* INPUT_H */
#ifdef __ATARI__

#include <atari.h>
#include <peekpoke.h>
#include <joystick.h>

// Either joystick 1/2 will work
unsigned char readJoystick() {
  static unsigned char value;
  value = 15-OS.stick0 + (OS.strig0==0)*JOY_BTN_1_MASK;  // joystick 1
  if (!value)
    value = 15-OS.stick1 + (OS.strig1==0)*JOY_BTN_1_MASK;    // joystick 2
  
  return value;
}

#endif /* __ATARI__ */
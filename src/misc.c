#include <joystick.h>
#include <string.h>
#include "platform-specific/graphics.h"
#include "platform-specific/input.h"
#include "platform-specific/sound.h"
#include "misc.h"
#include "fujinet-fuji.h"

#include <stdbool.h>
#include <stdint.h>

InputStruct input;
unsigned char _lastJoy, _joy, _joySameCount=10;
bool _buttonReleased=true;

#ifndef JOY_BTN_2_MASK
#define JOY_BTN_2_MASK JOY_BTN_1_MASK
#endif

void pause(unsigned char frames) {
  while(frames--)
    waitvsync();
}

void clearCommonInput() {
  input.trigger=input.key=input.dirY=input.dirX=_lastJoy=_joy=_buttonReleased=0;
  while (kbhit()) 
    cgetc();
}

void readCommonInput() {
  input.trigger=input.key=input.dirX=input.dirY=0;

  _joy = readJoystick();

  if (_joy != _lastJoy) {
    if (_lastJoy!=99)
      _joySameCount=12;

    _lastJoy=_joy;

    if (JOY_LEFT(_joy))
      input.dirX = -1;
    else if (JOY_RIGHT(_joy)) 
      input.dirX =1;
    
    if (JOY_UP(_joy))
      input.dirY = -1;
    else if (JOY_DOWN(_joy))
      input.dirY = 1;

    // Trigger button press only if it was previously unpressed
    if (JOY_BTN_1(_joy) || JOY_BTN_2(_joy)) {
      if (_buttonReleased) {
        input.trigger=true;
        _buttonReleased=false;
      }
    } else {
      _buttonReleased = true;
    }
    
    return;
  } else if (_joy!=0) {
    if (!_joySameCount--) {
      _joySameCount=0;
      _lastJoy=99;
    }
  }

  input.key=0;

  if (!kbhit())
    return;
    
  input.key = cgetc();

  switch (input.key) {
    case KEY_LEFT_ARROW:
    case KEY_LEFT_ARROW_2:
    case KEY_LEFT_ARROW_3:
      input.dirX=-1;
      break;
    case KEY_RIGHT_ARROW:
    case KEY_RIGHT_ARROW_2:
    case KEY_RIGHT_ARROW_3:
      input.dirX=1;
      break;
    case KEY_UP_ARROW:
    case KEY_UP_ARROW_2:
    case KEY_UP_ARROW_3:
      input.dirY=-1;
      break;
    case KEY_DOWN_ARROW:
    case KEY_DOWN_ARROW_2:
    case KEY_DOWN_ARROW_3:
      input.dirY=1;
      break;
    case KEY_SPACEBAR:
    case KEY_RETURN:
      input.trigger=true;
      break;
  }
}

void loadPrefs() {
  read_appkey(AK_CREATOR_ID, AK_APP_ID, AK_KEY_PREFS, tempBuffer);
  
  if (strlen(tempBuffer)>0) {
    memcpy(&prefs, tempBuffer, sizeof(prefs));

    if (prefs.debugFlag == 0xff) {
      strcpy(serverEndpoint, localServer);
    }
  }

  // Ensure local player count is at least one (older appkeys may have this set to 0)
  if (prefs.localPlayerCount==0 || prefs.localPlayerCount>4) {
    prefs.localPlayerCount=1;
  }

  setColorMode(prefs.color);
}

void savePrefs() {
  write_appkey(AK_CREATOR_ID, AK_APP_ID, AK_KEY_PREFS, sizeof(prefs), (char*)&prefs);
}


uint16_t read_appkey(uint16_t creator_id, uint8_t app_id, uint8_t key_id, char* destination) {
  uint16_t read=0;
  
  #ifndef _CMOC_VERSION_
  fuji_set_appkey_details(creator_id, app_id, DEFAULT);
  if (!fuji_read_appkey(key_id, &read, (uint8_t*)destination))
    read=0;
  #endif

  // Add string terminator after the data ends in case it is being interpreted as a string
  destination[read] = 0;
  return read;
}
 
void write_appkey(uint16_t creator_id, uint8_t app_id, uint8_t key_id,  uint16_t count, char *data)
{
  #ifndef _CMOC_VERSION_
  fuji_set_appkey_details(creator_id, app_id, DEFAULT);
  fuji_write_appkey(key_id, count, (uint8_t*)data);
  #endif
}
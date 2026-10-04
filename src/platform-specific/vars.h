/*
  Platform specific vars. 
*/
#ifndef VARS_H
#define VARS_H

// Platform specific vars
#include "../atari/vars.h"
#include "../apple2/vars.h"
#include "../coco/vars.h"
#include "../msdos/vars.h"
//#include "../c64/vars.h"
#include "../adam/vars.h"
#include "../nes/vars.h"

// Default vars (covers most platforms). A platform without a keyboard
// overrides these with the names of its buttons.
#ifndef ESCAPE
#define ESCAPE "ESCAPE"
#endif
#ifndef ESC
#define ESC "ESC"
#endif
#ifndef TRIGGER_TEXT
#define TRIGGER_TEXT "TRIGGER/SPACE"
#endif
#ifndef LOBBY_STATUS_TEXT
#define LOBBY_STATUS_TEXT "Refresh  Help  Players  Quit"
#endif
#ifndef MENU_QUIT_TEXT
#define MENU_QUIT_TEXT "  Q: quit table"
#endif
#ifndef MENU_HELP_TEXT
#define MENU_HELP_TEXT "  H: how to play"
#endif
#ifndef MENU_SOUND_ON_TEXT
#define MENU_SOUND_ON_TEXT "  S: sound ON "
#endif
#ifndef MENU_SOUND_OFF_TEXT
#define MENU_SOUND_OFF_TEXT "  S: sound OFF"
#endif
#ifndef MENU_ESC_TEXT
#define MENU_ESC_TEXT ESC ": keep playing"
#endif

#endif /* VARS_H */

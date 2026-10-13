#ifdef BUILD_ATARI5200

#ifndef KEYMAP_H
#define KEYMAP_H

#include <joystick.h>

// Screen dimensions and layout for platform: the Atari 8-bit client's ANTIC
// mode 4 screen, unchanged.
#define WIDTH 40
#define HEIGHT 26

#define ROLL_SOUND_MOD 4 // How often to play roll sound
#define ROLL_FRAMES 31 // How many roll frames to play
#define SCORE_CURSOR_ALT 0x80 // Alternate score cursor color
#define BOTTOM_HEIGHT 4 // How high the bottom panel is
#define SCORES_X 11 // X start of scoreboard
#define GAMEOVER_PROMPT_Y HEIGHT-3
#define QUERY_SUFFIX "" // 6502 is little endian - no &be=1 needed
#define ROLL_X WIDTH-25
#define TIMER_X 12
#define TIMER_NUM_OFFSET_X 0
#define TIMER_NUM_OFFSET_Y 0

// Icons - the Atari 8-bit charset's (support/atari/charset.fnt)
#define ICON_TEXT_CURSOR  0xD9
#define ICON_MARK         0x1D
#define ICON_MARK_ALT     0x1C
#define ICON_PLAYER       0x0A
#define ICON_SPEC         0xDC
#define ICON_CURSOR       0xBE
#define ICON_CURSOR_ALT   0xBF
#define ICON_CURSOR_BLIP  0x3E

/* readJoystick() (src/atari5200/input.c) returns the stick only, as cc65's
   joystick bits. The fire buttons and the keypad are reported as key codes
   behind kbhit()/cgetc(), as on the 7800: top fire is KEY_RETURN, which
   readCommonInput() maps to the trigger and which also satisfies the screens
   that block on cgetc(). */
#undef JOY_BTN_1
#undef JOY_BTN_2
#define JOY_BTN_1(v) ((v) & 0)
#define JOY_BTN_2(v) ((v) & 0)

/**
 * Platform specific key map for common input
 */

// No keyboard: the keypad's 4, 6, 2 and 8 send the first arrow codes; the
// others are unreachable placeholders for readCommonInput()'s switch.
#define KEY_LEFT_ARROW      0xF1
#define KEY_LEFT_ARROW_2    0xF2
#define KEY_LEFT_ARROW_3    0xF3

#define KEY_RIGHT_ARROW     0xF4
#define KEY_RIGHT_ARROW_2   0xF5
#define KEY_RIGHT_ARROW_3   0xF6

#define KEY_UP_ARROW        0xF7
#define KEY_UP_ARROW_2      0xF8
#define KEY_UP_ARROW_3      0xF9

#define KEY_DOWN_ARROW      0xFA
#define KEY_DOWN_ARROW_2    0xFB
#define KEY_DOWN_ARROW_3    0xFC

#define KEY_RETURN       0x0D
#define KEY_ESCAPE       0x1B
#define KEY_ESCAPE_ALT   0x03
#define KEY_SPACEBAR     0x20
#define KEY_BACKSPACE    0x08

// One controller, so one local player: the local player screens want A to
// add a player and / to remove one, which the keypad cannot type.
#define SINGLE_LOCAL_PLAYER

// Name entry types from an on-screen letter grid (src/atari5200/osk.c).
#define ON_SCREEN_KEYBOARD
void oskOpen(void);
void oskClose(void);

// The fire buttons, as readButtons() reports them; readKeypad() hands out
// the last keypad key (fuji_a5200_key) once.
#define BTN_FIRE   0x01   // top fire
#define BTN_2      0x02   // lower fire
unsigned char readStick(void);
unsigned char readButtons(void);
unsigned char readKeypad(void);

// Button names for the shared screens' prompts (see input.c for the map).
// The charset draws * and # as icons, so no label names those two keys.
#define ESCAPE "PAUSE"
#define ESC "PAUSE"
#define TRIGGER_TEXT "FIRE"
#define LOBBY_STATUS_TEXT "START:refresh 1:help RESET:quit"
#define MENU_QUIT_TEXT "RESET:quit table"
#define MENU_HELP_TEXT "1:how to play"
#define MENU_SOUND_ON_TEXT "0:sound ON "
#define MENU_SOUND_OFF_TEXT "0:sound OFF"
#define MENU_ESC_TEXT "PAUSE:keep playing"

#endif /* KEYMAP_H */

#endif /* BUILD_ATARI5200 */

#ifdef _CMOC_VERSION_

#ifndef KEYMAP_H
#define KEYMAP_H

// Screen dimensions for platform

#define WIDTH 32
#define HEIGHT 24

// Other platform specific constnats

#define ROLL_SOUND_MOD 1 // How often to play roll sound
#define ROLL_FRAMES 10 // How many roll frames to play
//#define SCORE_CURSOR_ALT 2 // Alternate score cursor color
#define BOTTOM_HEIGHT 3 // How high the bottom panel is
#define SCORES_X 10 // X start of scoreboard
#define GAMEOVER_PROMPT_Y HEIGHT-1

// Icons
#define ICON_TEXT_CURSOR  0x22
#define ICON_MARK         0x22
#define ICON_MARK_ALT     0x5B
#define ICON_PLAYER       0x23
#define ICON_SPEC         0x28
#define ICON_CURSOR       0x29
#define ICON_CURSOR_ALT   0x2A
#define ICON_CURSOR_BLIP  0x2B

/**
 * Platform specific key map for common input
 */


#define KEY_LEFT_ARROW      0x08
#define KEY_LEFT_ARROW_2    0x9D
#define KEY_LEFT_ARROW_3    0x2C // ,

#define KEY_RIGHT_ARROW     0x15
#define KEY_RIGHT_ARROW_2   0x1D
#define KEY_RIGHT_ARROW_3   0x2E // .

#define KEY_UP_ARROW        0x5E
#define KEY_UP_ARROW_2      0x91
#define KEY_UP_ARROW_3      0x2D // -

#define KEY_DOWN_ARROW      0x0A
#define KEY_DOWN_ARROW_2    0x11
#define KEY_DOWN_ARROW_3    0x3D // =

#define KEY_RETURN       0x0D

#define KEY_ESCAPE       0x1B
#define KEY_ESCAPE_ALT   0x03

#define KEY_SPACEBAR     0x20
#define KEY_BACKSPACE    0x7F

#define CHAR_CURSOR      0x9F

#endif /* KEYMAP_H */

#endif
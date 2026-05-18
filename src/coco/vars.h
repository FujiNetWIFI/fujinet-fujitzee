#ifdef _CMOC_VERSION_

#ifndef KEYMAP_H
#define KEYMAP_H

// Screen dimensions and layout for platform

#ifdef COCO3

// CoCo 3: 320x200x16 graphics, 40x25 character grid (MSDOS-style layout)
#define WIDTH 40
#define HEIGHT 25

#define ROLL_SOUND_MOD 1
#define ROLL_FRAMES 8
#define BOTTOM_HEIGHT 4
#define SCORES_X 10
#define GAMEOVER_PROMPT_Y HEIGHT-3
#define ROLL_X WIDTH-25
#define TIMER_X 12
#define TIMER_NUM_OFFSET_X 0
#define TIMER_NUM_OFFSET_Y 0

// Icons (MSDOS charset glyph numbering; bit 7 selects the alt palette)
#define ICON_TEXT_CURSOR  0xD9
#define ICON_MARK         0x1D
#define ICON_MARK_ALT     0x1C
#define ICON_PLAYER       0x0A
#define ICON_SPEC         0xDC
#define ICON_CURSOR       0xBE
#define ICON_CURSOR_ALT   0xBF
#define ICON_CURSOR_BLIP  0x3E

#else

// CoCo 1/2: 256x192x2 graphics, 32x24 character grid
#define WIDTH 32
#define HEIGHT 24

#define ROLL_SOUND_MOD 1 // How often to play roll sound
#define ROLL_FRAMES 11 // How many roll frames to play
//#define SCORE_CURSOR_ALT 2 // Alternate score cursor color
#define BOTTOM_HEIGHT 3 // How high the bottom panel is
#define SCORES_X 2 // X start of scoreboard
#define GAMEOVER_PROMPT_Y HEIGHT-2
#define ROLL_X WIDTH-24
#define TIMER_X 5
#define TIMER_NUM_OFFSET_X 2
#define TIMER_NUM_OFFSET_Y 1

// CoCo 1/2 uses online help; the built-in help text overruns memory here.
#define ONLINE_HELP 1

// Icons
#define ICON_TEXT_CURSOR  0x22
#define ICON_MARK         0x22
#define ICON_MARK_ALT     0x5B
#define ICON_PLAYER       0x23
#define ICON_SPEC         0x28
#define ICON_CURSOR       0x29
#define ICON_CURSOR_ALT   0x2A
#define ICON_CURSOR_BLIP  0x2B

#endif /* COCO3 */

// Common to all CoCo models
#define QUERY_SUFFIX "&be=1" // Big Endian response for CoCo (6809)

#undef ESCAPE
#define ESCAPE "BREAK"
#undef ESC
#define ESC "BRK"

/**
 * Platform specific key map for common input
 */


#define KEY_LEFT_ARROW      0x08
#define KEY_LEFT_ARROW_2    0xF1
#define KEY_LEFT_ARROW_3    0xF2 // ,

#define KEY_RIGHT_ARROW     0x09
#define KEY_RIGHT_ARROW_2   0xF3
#define KEY_RIGHT_ARROW_3   0xF4 // .

#define KEY_UP_ARROW        0x5E
#define KEY_UP_ARROW_2      0xF5
#define KEY_UP_ARROW_3      0xF6 // -

#define KEY_DOWN_ARROW      0x0A
#define KEY_DOWN_ARROW_2    0xF7
#define KEY_DOWN_ARROW_3    0xF9 // =

#define KEY_RETURN       0x0D

#define KEY_ESCAPE       0x03
#define KEY_ESCAPE_ALT   0x1B

#define KEY_SPACEBAR     0x20
#define KEY_BACKSPACE    0x7F

#define CHAR_CURSOR      0x9F

#endif /* KEYMAP_H */

#endif

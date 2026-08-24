#ifdef __ADAM__

#ifndef KEYMAP_H
#define KEYMAP_H

// Screen dimensions and layout for platform
// TMS9918A mode 2 used as a 32x24 char map - same grid as CoCo 1/2,
// so the layout constants below match the CoCo 1/2 branch of coco/vars.h.

#define WIDTH 32
#define HEIGHT 24

#define ROLL_SOUND_MOD 1 // How often to play roll sound
#define ROLL_FRAMES 11 // How many roll frames to play
#define BOTTOM_HEIGHT 3 // How high the bottom panel is
#define SCORES_X 2 // X start of scoreboard
#define GAMEOVER_PROMPT_Y HEIGHT-2
#define ROLL_X WIDTH-24
#define TIMER_X 5
#define TIMER_NUM_OFFSET_X 2
#define TIMER_NUM_OFFSET_Y 1

#define QUERY_SUFFIX "" // Z80 is little endian - no &be=1 needed

// Icons (CoCo 1/2 charset glyph numbering; +0x80 selects the accent bank)
#define ICON_TEXT_CURSOR  0x22
#define ICON_MARK         0x22
#define ICON_MARK_ALT     0x5B
#define ICON_PLAYER       0x23
#define ICON_SPEC         0x28
#define ICON_CURSOR       0x29
#define ICON_CURSOR_ALT   0x2A
#define ICON_CURSOR_BLIP  0x2B

/**
 * Platform specific key map for common input (Adam keyboard via EOS)
 */

#define KEY_LEFT_ARROW      0xA3
#define KEY_LEFT_ARROW_2    0x9D
#define KEY_LEFT_ARROW_3    0x2C // ,

#define KEY_RIGHT_ARROW     0xA1
#define KEY_RIGHT_ARROW_2   0x1D
#define KEY_RIGHT_ARROW_3   0x2E // .

#define KEY_UP_ARROW        0xA0
#define KEY_UP_ARROW_2      0x91
#define KEY_UP_ARROW_3      0x2D // -

#define KEY_DOWN_ARROW      0xA2
#define KEY_DOWN_ARROW_2    0x11
#define KEY_DOWN_ARROW_3    0x3D // =

#define KEY_RETURN       0x0D

#define KEY_ESCAPE       0x1B
#define KEY_ESCAPE_ALT   0x03

#define KEY_SPACEBAR     0x20
#define KEY_BACKSPACE    0x08

/* Macros that evaluate the return code of readJoystick */
#define JOY_UP(v) ((v) & 1)
#define JOY_DOWN(v) ((v) & 2)
#define JOY_LEFT(v) ((v) & 4)
#define JOY_RIGHT(v) ((v) & 8)
#define JOY_BTN_1(v) ((v) & 16) /* Universally available */
#define JOY_BTN_2(v) ((v) & 32) /* Second button if available */

#endif /* KEYMAP_H */

#endif

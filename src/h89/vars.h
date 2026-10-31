#ifdef __H89__

#ifndef KEYMAP_H
#define KEYMAP_H

/*
  Heathkit H89: the H19 terminal's 80x24, plus its 25th line as row 24
  (HEIGHT-1, the status line), which src/h89/console.c turns on.

  The board is the 40 column one widened to 80: the score names in full in a
  box at columns 11-23, then six player columns eight wide (24-31, 33-40, ...
  69-76) between dividers at 23, 32, ... 77. The shared code's three wide
  score field sits in the middle of each (SCORE_COL_X), with the score cursor
  at its left and the initial over its centre. The dice are 7x3 -- the H19's
  cells are about two and a half times as tall as they are wide, so that is
  square -- every 9 columns, the roll button under the first player column.
  The layout is drawn in src/h89/graphics.c; the macros below only place what
  the shared code draws.

  Every macro is parenthesised and free of comparisons: sccz80 drops the 0/1
  of a > that feeds arithmetic.
*/

#define WIDTH 80
#define HEIGHT 25

#define ROLL_SOUND_MOD 4 // How often to play roll sound
#define ROLL_FRAMES 5    // How many roll frames to play (each takes ~0.3 s at 9600 baud)
#define BOTTOM_HEIGHT 4  // How high the bottom panel is
#define SCORES_X 12      // X start of the score names
#define GAMEOVER_PROMPT_Y (HEIGHT-3)
#define ROLL_X 24
#define TIMER_X 13
#define TIMER_NUM_OFFSET_X 0
#define TIMER_NUM_OFFSET_Y 0

// Board layout hooks (src/gamelogic.c)
#define SCORE_COL_X(i)   (26+9*(i))
#define INITIAL_X(i)     (SCORE_COL_X((i)-1)+1)
#define DIE_X(i)         (33+9*(i))
#define DICE_CURSOR_X(p) (24+9*(p))
#define KEPT_DIE_X(p)    DICE_CURSOR_X(p)

// The in-game help: a box over player columns 2-6 (only player 1 sees it)
#define HELP_X           46
#define HELP_CLEAR_X     33
#define HELP_CLEAR_W     44

// The 40 column help screens, centred (src/screens.c)
#define HELP_LEFT        ((WIDTH-40)/2)

// The alternate style is reverse video here: the selected table is the
// reversed one, and mixed case server names are drawn plain
#define LOBBY_SEL_TEXT     drawTextRev
#define LOBBY_UNSEL_TEXT   drawText
#define CENTER_SERVER_TEXT centerText

/// @brief Draw text all in reverse video (src/h89/graphics.c)
void drawTextRev(unsigned char x, unsigned char y, char *s);

#define QUERY_SUFFIX "" // Z80 is little endian - no &be=1 needed

#define TRIGGER_TEXT "SPACE"

// Icons: H19 screen codes (src/h89/console.h) -- the character ROM index,
// 0x00-0x1F and 0x7F the graphics characters, with bit 7 for reverse video
#define ICON_TEXT_CURSOR  0xA0 // reverse space
#define ICON_MARK         0x08 // right arrow
#define ICON_MARK_ALT     0x20 // (blinks)
#define ICON_PLAYER       0x7F // bullet
#define ICON_SPEC         0x09 // checkerboard
#define ICON_CURSOR       0x08 // right arrow
#define ICON_CURSOR_ALT   0x88 // reverse right arrow
#define ICON_CURSOR_BLIP  0x20 // (blinks)

/**
 * Platform specific key map for common input (H19 keyboard)
 *
 * src/h89/input.c turns the keypad's ESC A-D (keypad shifted mode, set by
 * termInit) into the arrow codes below. readCommonInput() switches on all
 * twelve: the _2 codes are unreachable placeholders, and the _3 keys are
 * spares for typing without the keypad.
 */
#define KEY_LEFT_ARROW      0x1D
#define KEY_LEFT_ARROW_2    0xF1
#define KEY_LEFT_ARROW_3    0x2C // ,

#define KEY_RIGHT_ARROW     0x1C
#define KEY_RIGHT_ARROW_2   0xF2
#define KEY_RIGHT_ARROW_3   0x2E // .

#define KEY_UP_ARROW        0x1E
#define KEY_UP_ARROW_2      0xF3
#define KEY_UP_ARROW_3      0x2D // -

#define KEY_DOWN_ARROW      0x1F
#define KEY_DOWN_ARROW_2    0xF4
#define KEY_DOWN_ARROW_3    0x3D // =

#define KEY_RETURN       0x0D
#define KEY_ESCAPE       0x1B
#define KEY_ESCAPE_ALT   0x03 // CTRL-C
#define KEY_SPACEBAR     0x20
#define KEY_BACKSPACE    0x08

// Keyboard only: readJoystick() always returns 0
#define JOY_UP(v)    ((v) & 0)
#define JOY_DOWN(v)  ((v) & 0)
#define JOY_LEFT(v)  ((v) & 0)
#define JOY_RIGHT(v) ((v) & 0)
#define JOY_BTN_1(v) ((v) & 0)
#define JOY_BTN_2(v) ((v) & 0)

#endif /* KEYMAP_H */

#endif /* __H89__ */

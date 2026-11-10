#ifdef __TRS80M2__

#ifndef KEYMAP_H
#define KEYMAP_H

/*
  TRS-80 Model II under Pickles & Trout CP/M: 80x24, drawn with the P&T
  video driver's graphics characters and reverse video.

  The board is the H89's (src/h89/vars.h): the score names in a box at
  columns 11-23, then six player columns eight wide between dividers at 23,
  32, ... 77, and 7x3 dice. With one row fewer than the H89, the dice sit in
  rows 21-23 below the board's bottom rule (src/model2/graphics.c moves them
  down from HEIGHT-4), beside the status text at the left of row 23.

  Every macro is parenthesized and free of comparisons: sccz80 drops the 0/1
  of a > that feeds arithmetic.
*/

#define WIDTH 80
#define HEIGHT 24

#define ROLL_SOUND_MOD 4
#define ROLL_FRAMES 5
#define BOTTOM_HEIGHT 3
#define SCORES_X 12
#define GAMEOVER_PROMPT_Y (HEIGHT-2)
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

#define HELP_X           46
#define HELP_CLEAR_X     33
#define HELP_CLEAR_W     44

#define HELP_LEFT        ((WIDTH-40)/2)

// The alternate style is reverse video, as on the H89
#define LOBBY_SEL_TEXT     drawTextRev
#define LOBBY_UNSEL_TEXT   drawText
#define CENTER_SERVER_TEXT centerText

void drawTextRev(unsigned char x, unsigned char y, char *s);

#define QUERY_SUFFIX ""

#define ESCAPE "ESC"
#define TRIGGER_TEXT "SPACE"

// Icons: cell codes (src/model2/console.h), bit 7 reverse video
#define ICON_TEXT_CURSOR  0xA0 // reverse space
#define ICON_MARK         '>'
#define ICON_MARK_ALT     ' '  // (blinks)
#define ICON_PLAYER       '*'
#define ICON_SPEC         '#'
#define ICON_CURSOR       '>'
#define ICON_CURSOR_ALT   0xBE // reverse >
#define ICON_CURSOR_BLIP  ' '  // (blinks)

// P&T CP/M key codes. F1 and F2 are spare up and down keys, BREAK is
// KEY_ESCAPE_ALT; the left and right _2 codes are unreachable placeholders.
#define KEY_LEFT_ARROW      0x1C
#define KEY_LEFT_ARROW_2    0xF1
#define KEY_LEFT_ARROW_3    0x2C // ,

#define KEY_RIGHT_ARROW     0x1D
#define KEY_RIGHT_ARROW_2   0xF2
#define KEY_RIGHT_ARROW_3   0x2E // .

#define KEY_UP_ARROW        0x1E
#define KEY_UP_ARROW_2      0x01 // F1
#define KEY_UP_ARROW_3      0x2D // -

#define KEY_DOWN_ARROW      0x1F
#define KEY_DOWN_ARROW_2    0x02 // F2
#define KEY_DOWN_ARROW_3    0x3D // =

#define KEY_RETURN       0x0D
#define KEY_ESCAPE       0x1B
#define KEY_ESCAPE_ALT   0x03 // BREAK
#define KEY_SPACEBAR     0x20
#define KEY_BACKSPACE    0x08

#define JOY_UP(v)    ((v) & 0)
#define JOY_DOWN(v)  ((v) & 0)
#define JOY_LEFT(v)  ((v) & 0)
#define JOY_RIGHT(v) ((v) & 0)
#define JOY_BTN_1(v) ((v) & 0)
#define JOY_BTN_2(v) ((v) & 0)

#endif /* KEYMAP_H */

#endif /* __TRS80M2__ */

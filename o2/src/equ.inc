; equ.inc -- constants: the console, the cartridge, the screen, the server.

; ---- P1 (all active low but COPY and LUM) ----
; Main code runs with external RAM and the cartridge on the bus; the interrupts
; switch to the VDC and put P1 back. COPY low always, or the cart's write
; strobe is gated off. The bank bits are left high: APPBANK pages are picked
; through the mailbox, not by P10/P11.
P1_MAIN equ 0AFh                ; XRAM + cart selected, VDC parked
P1_VDC  equ 0B7h                ; VDC selected
P1_JOY  equ 0BBh                ; keyboard scanner (joystick) selected

; ---- the mailbox (fuji_mailbox.h, protocol 2) ----
FN_W_REGSEL     equ 0E0h
FN_W_REGDATA    equ 0E1h
FN_W_DATA       equ 0E2h
FN_W_SEQ        equ 0E3h
FN_REG_DEVICE   equ 000h
FN_REG_CMD      equ 001h
FN_REG_NPARAM   equ 002h
FN_REG_DATA_RST equ 005h
FN_REG_CODEBANK equ 008h
FN_REG_POKE_LO  equ 00Ah
FN_REG_POKE_HI  equ 00Bh
FN_REG_POKE     equ 00Ch
FN_REG_RXWIN    equ 00Dh
FN_R_ACKSEQ     equ 0F20h
FN_R_ERR        equ 0F22h
FN_R_REPLY_CMD  equ 0F23h
FN_R_RXLEN_LO   equ 0F24h
FN_R_RXLEN_HI   equ 0F25h
FN_R_MAGIC0     equ 0F29h
FN_R_MAGIC1     equ 0F2Ah
FN_R_PROTO_VER  equ 0F2Bh
FN_R_DATA       equ 0F30h       ; the read window: 208 bytes from RXWIN*16
FN_PROTO_VER    equ 2
O2_APP_HDR      equ 0F30h
O2_APP_VERSION  equ 1

FUJI_DEVICEID   equ 070h
FC_ACK          equ 006h
FC_AKOPEN       equ 0DCh        ; OPEN_APPKEY
FC_AKREAD       equ 0DDh        ; READ_APPKEY
FC_AKWRITE      equ 0DEh        ; WRITE_APPKEY
NET_DEVICEID    equ 071h
NC_OPEN         equ 04Fh
NC_CLOSE        equ 043h
NC_READ         equ 052h
NC_STATUS       equ 053h
NM_GET          equ 00Ch        ; open mode: HTTP GET
NS_EOF          equ 136         ; nDevStatus: end of file

; The appkeys (creator 1, app 1): the shared name, and the room the lobby
; hands over ("<server>?table=<id>", Fujitzee's game type, 3).
AK_NAME equ 0
AK_ROOM equ 3

; ---- the VDC's character generator (o2doc appendix C) ----
G_SPACE equ 12
G_DOT   equ 39
G_DASH  equ 40
G_BLOCK equ 47
G_TEN   equ 48                  ; "10" in one cell
CH_BALL  equ 00h                ; the pictures' codes in text (mkcharmap.py)
CH_BLOCK equ 08h
CH_ARROW equ 0Ah
CH_TEN   equ 0Bh

; Character colours, as MAME's i8244 draws them (control byte bits 1-3).
; A character can't be black: 0 is the darkest, a dark grey.
C_DGREY  equ 0
C_RED    equ 1
C_GREEN  equ 2
C_TAN    equ 3
C_BLUE   equ 4
C_VIOLET equ 5
C_CYAN   equ 6
C_WHITE  equ 7

; The colour register ($A3): bits 5-3 the background (MAME bitswap 3,4,5:
; bit 5 red, 4 green, 3 blue), bits 6 and 2-0 the grid (bitswap 6,0,1,2).
    ifndef BG_CARD
BG_CARD  equ 008h               ; the card: blue
    endif
BG_BLACK equ 000h
GRID_WHITE equ 047h
COL_TEXT equ BG_BLACK
COL_CARD equ BG_CARD|GRID_WHITE

; The control register ($A0) on the card. The dice are the grid in fill
; mode: a vertical segment is a whole 16 x 24 cell, white.
CTL_TOP equ vdc_ctrl_fore                               ; T0: the grid hidden
CTL_TBL equ vdc_ctrl_fill|vdc_ctrl_fore|vdc_ctrl_grid   ; the card
CTL_GON equ vdc_ctrl_fill|vdc_ctrl_grid                 ; a band, dice on
CTL_OFF equ 0                                           ; a band, dice off

; ---- the screen ----
; Characters are 7 rows of 2 lines. Every screen has text line T0 at the top
; and T1 at the bottom (the quads, 16 characters), and eight rows of singles
; between them (fixed.inc).
X_TEXT  equ 16                  ; 16 characters at 8 px, centred
TXT_LEN equ 16
TXT_TAB equ 1+2*TXT_LEN         ; a text table: Y, then 16 x (pointer, control)
    ifndef Y_T0
Y_T0    equ 16                  ; MAME hides characters above Y=14 (NTSC)
    endif

; The card. Every row is the same NCOLS singles at the same X (obj_setup
; puts X once per screen; a band writes Y, pointer and control): the left
; cell (slot r: a 2-letter label and a 3-digit value), the right cell (slot
; 8 + r, the same), and on rows DICEROW..DICEROW+4 a die: a filled grid cell
; in column DICECOL with its face, a digit, over it. The card is the wire's
; own order: the upper section down the left (ones .. sixes, the upper
; total, the bonus), the lower down the right (3 and 4 of a kind, the full
; house, the straights, chance, Fujitzee, the total).
NCOLS   equ 11
C_LBL   equ 0                   ; a row's places: the left label (2) ...
C_VAL   equ 2                   ; ... its value (3)
C_RLBL  equ 5                   ; the right label (2) ...
C_RVAL  equ 7                   ; ... its value (3)
C_DIE   equ 10                  ; the die
    ifndef LBLX
LBLX    equ 8                   ; the left label's X
    endif
VALX    equ LBLX+24             ; its value's (after a space)
    ifndef RLBLX
RLBLX   equ LBLX+64             ; the right label's
    endif
RVALX   equ RLBLX+24
    ifndef DICECOL
DICECOL equ 8                   ; the dice's grid column
    endif
DIEX    equ 8+16*DICECOL+4      ; a face, centred in its cell
DICEROW equ 1                   ; die 0's row; die k is on DICEROW + k
NDICE   equ 5
ROW_SPR equ 2*NCOLS             ; a row: NCOLS x (pointer, control), then
ROW_TAB equ ROW_SPR+1           ; sprite 1's Y: its die's held mark (SPR_OFF:
                                ; none)
NROWS   equ 8
    ifndef Y_ROW0
Y_ROW0  equ 30                  ; row 0's characters; row r's is +24r
    endif
ROW_PITCH equ 24
Y_ROW7  equ Y_ROW0+7*ROW_PITCH  ; the last row
    ifndef Y_T1
Y_T1    equ 228                 ; the card's T1 (MAME draws to line 242)
    endif
    ifndef LEAD
LEAD    equ 1                   ; lines a band starts before its line ends
    endif
    ifndef SEPA
SEPA    equ 3                   ; a row band: the dice go off at this single
    endif
    ifndef SEPAF
SEPAF   equ 1                   ; ... after its pointer (fixed.inc row_write)
    endif
    ifndef SEPB
SEPB    equ 7                   ; ... and on again at this one
    endif
    ifndef SEPBF
SEPBF   equ 1
    endif
    ifndef GRIDNOP
GRIDNOP equ 0                   ; h_grid: nops to put its write in the blank
    endif
X_ROWTXT equ 44                 ; a text screen's rows: 11 characters, centred

; The card's sprites (screen.inc spr_card), 8 x 8 at a character's scale.
; One shows only while the foreground is on: inside a row's 14 lines, T0's
; or T1's.
SPR_OFF equ 0F8h                ; a sprite's Y below the screen
    ifndef CURDY
CURDY   equ 2                   ; 0: the cursor, an arrow, its Y in its row
    endif
CURLX   equ LBLX-8              ; ... left of the left label
CURRX   equ RLBLX-8             ; ... of the right one
CURDX   equ 8+16*DICECOL-10     ; ... of a die
CURTX   equ X_TEXT-8            ; ... of ROLL, left of T1
    ifndef HELDX
HELDX   equ 8+16*DICECOL+18     ; 1: a held die's mark, right of its cell
    endif
HELDDY  equ 2                   ; (each row band writes its Y: ROW_SPR)
    ifndef PADN
PADN    equ 0                   ; spike: pad a row band by 2+2n cycles
    endif

; The cursor (CUR): a die, ROLL, or a card slot.
CU_DIE  equ 0                   ; 0-4: die k
CU_ROLL equ 5
CU_CARD equ 16                  ; 16 + slot

; ---- the stick (input.inc): JOY / JOYEV bits ----
J_UP    equ 01h
J_DOWN  equ 02h
J_LEFT  equ 04h
J_RIGHT equ 08h
J_FIRE  equ 10h
J_DIRS  equ J_UP|J_DOWN|J_LEFT|J_RIGHT
REP_FIRST equ 18                ; a direction held: repeats after this ...
REP_NEXT  equ 5                 ; ... then every this (passes of the loop)

; ---- the keyboard (input.inc): BIOS readkey codes, row x 8 + column ----
K_0     equ 00h                 ; 0-9 are 00h-09h
K_1     equ 01h
K_5     equ 05h
K_SPACE equ 0Ch
K_QUEST equ 0Dh                 ; ? : help, as H is
K_L     equ 0Eh
K_P     equ 0Fh
K_W     equ 11h
K_E     equ 12h
K_R     equ 13h
K_T     equ 14h
K_U     equ 15h
K_I     equ 16h
K_O     equ 17h
K_Q     equ 18h
K_S     equ 19h
K_D     equ 1Ah
K_F     equ 1Bh
K_G     equ 1Ch
K_H     equ 1Dh
K_J     equ 1Eh
K_K     equ 1Fh
K_A     equ 20h
K_Z     equ 21h
K_X     equ 22h
K_C     equ 23h
K_V     equ 24h
K_B     equ 25h
K_M     equ 26h
K_YES   equ 2Ch
K_NO    equ 2Dh
K_CLR   equ 2Eh
K_ENT   equ 2Fh
K_NONE  equ 0FFh

; ---- sounds (snd.inc): beep ids; from SND_SOFT on they never cut another ----
SND_TURN   equ 0                ; our turn
SND_ROLL   equ 1                ; the dice rattle
SND_SCORE  equ 2                ; we took a row
SND_BOT    equ 3                ; somebody else took one: falling
SND_FUJI   equ 4                ; five of a kind
SND_OVER   equ 5                ; game over
SND_ERROR  equ 6                ; refused
SND_READY  equ 7                ; the ready toggle, a table joined
SND_SOFT   equ 8
SND_TICK   equ 8                ; our clock, its last seconds
SND_MOVE   equ 9                ; the cursor
SND_HOLD   equ 10               ; a die held
SND_FREE   equ 11               ; ... let go

; ---- requests (net.inc): ACT_*, and what a request came to (NETHAVE) ----
ACT_STATE  equ 1
ACT_READY  equ 2
ACT_ROLL   equ 3                ; ACTARG: the dice held (bit k: die k)
ACT_SCORE  equ 4                ; ACTARG: the slot
ACT_LEAVE  equ 5
ACT_TABLS  equ 6                ; tables?bin=1

NS_IDLE equ 0
NS_CLOS equ 1
NS_OPEN equ 2
NS_STAT equ 3
NS_READ equ 4

NH_FAIL   equ 0
NH_STATE  equ 1                 ; a whole state is in the reply
NH_TABLES equ 2                 ; the table list
NH_LEFT   equ 3                 ; /leave answered (nothing to read)

; ---- pages, and their farcall entries ----
PG_BOOT  equ 0
E_TABLES equ 1                  ; page 0's: back to the table list
PG_NET   equ 1
E_NBEGIN equ 0
E_NSTEP  equ 1
PG_MAIN  equ 2
PG_EDGE  equ 3                  ; a state has come: what it means ...
E_EDGE   equ 0
E_SHADOW equ 1                  ; ... and, after it is drawn, the shadows
PG_TICK  equ 4                  ; every frame
E_TICK   equ 0
PG_HELP  equ 5
E_HELP   equ 0
E_NAME   equ 1                  ; page 5's: the name, from the keyboard
HC_TABLES equ 0                 ; the help's contexts
HC_TABLE  equ 1
PG_CARD  equ 6                  ; the card: its rows, T0, T1
E_CARD   equ 0
E_ROW    equ 1                  ; ... and a row a pass after it
PG_TEXT  equ 7                  ; the lobby, the standings, game over
E_TEXT   equ 0                  ; (E_ROW: a step a pass after it)
E_TITLE  equ 2                  ; page 6's: the boot's title

; The screens (SCR, what the state draws; LSCR, what is up: 0FFh afresh).
SC_CARD  equ 0
SC_LOBBY equ 1
SC_OVER  equ 2
SC_STAND equ 3

; ---- the server's binary state (server/util.go serializeResults, ?bin=1):
; strings lowercased and NUL-padded to max+1, u16 little-endian ----
GAME_COUNT    equ 0             ; playerCount (spectators after the seats)
GAME_SERVER   equ 1             ; 21: the table's name
GAME_PROMPT   equ 22            ; 41
GAME_ROUND    equ 63            ; 0 the lobby, 1-13, 99 game over
GAME_ROLLS    equ 64            ; rollsLeft
GAME_ACTIVE   equ 65            ; the seat to move, us = 0; 0FFh nobody
GAME_MOVETIME equ 66            ; its seconds left
GAME_VIEWING  equ 67            ; 1: we are a spectator
GAME_DICE     equ 68            ; 6: five '1'-'6', NUL; empty at a turn's start
GAME_KEEP     equ 74            ; 6: five '0'/'1' ('1': re-rolled), NUL
GAME_VALID    equ 80            ; 15: what each slot would score, -1 none
GAME_SEATS    equ 95            ; SEAT_SIZE each, us first
SEAT_SIZE     equ 42
SE_NAME       equ 0             ; 9
SE_ALIAS      equ 9
SE_SCORES     equ 10            ; 16 x u16; $FFxx unset (the lobby: [0]
                                ; 1 ready, 0 not)
TB_SIZE       equ 36            ; a table: id (9), name (21), players (6)
TB_NAME       equ 9
TB_PLAYERS    equ 30
MAXSEATS      equ 12            ; the serializer's cap (6 seats + watchers)
ROUND_LOBBY   equ 0
ROUND_LAST    equ 13
ROUND_OVER    equ 99
SL_UP    equ 6                  ; the slots: 0-5 ones .. sixes,
SL_BONUS equ 7                  ; 8-14 set of 3 .. Fujitzee, 15 the total
SL_SET3  equ 8
SL_FUJI  equ 14
SL_TOTAL equ 15
NSLOTS   equ 16
NVALID   equ 15

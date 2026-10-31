; ram.inc -- where everything lives.
;
; Internal RAM (64 bytes):
;   $00-$07  register bank 0: the main program
;   $08-$17  the stack, 8 levels: the interrupts take up to 3
;   $18-$1F  register bank 1: the interrupts' own (r5: the next band, r4:
;            the staged row's Y, from the vblank to the last row)
;   $20-$3C  below
;   $3D-$3F  the BIOS's (its keyboard and IRQ routines)

ROWBUF  equ 020h                ; ROW_TAB: the next card row, staged
FRAMES  equ 038h                ; vblanks, for the main loop's clock
QUIET   equ 039h                ; frames whose last band has run
MODE    equ 03Ah                ; MODE_*
SNDCNT  equ 03Bh                ; the sound: frames left (fixed.inc beep)
CURPG   equ 03Ch                ; the page mapped at $800

    if ROWBUF+ROW_TAB > FRAMES
    error "ROWBUF has grown into FRAMES"
    endif

MODE_SETUP equ 002h             ; a new screen's objects (fixed.inc obj_setup)
MODE_TEXT  equ 004h             ; ... with the rows' singles side by side

; External RAM (128 bytes, $00-$7F, MOVX with P1_MAIN):
LINEBUF equ 000h                ; 16: a text line or a card row being built
TXCOL   equ 010h                ; its colour, and for characters with bit 7 set
TXCOL2  equ 011h
OLDPG   equ 012h                ; farcall's caller's page
TMP0    equ 013h                ; a loop's index across calls that keep nothing
TMP1    equ 014h
; the input (input.inc)
KLAST   equ 015h                ; BIOS readkey's memory (its r7)
KEY     equ 016h                ; the key pressed this frame (K_NONE: none)
JOY     equ 017h                ; the stick now: J_* bits
JOYP    equ 018h                ; last time's
JOYEV   equ 019h                ; pressed since last time (and repeating)
T1SIG   equ 01Ah                ; a signature of what T1 shows
TXSPL   equ 01Bh                ; tx_build: columns past it in C_INFO (0FFh none)
REP     equ 01Ch                ; a held direction's repeat countdown
; a card row being built (card.inc)
ROWN    equ 01Dh                ; the row
ROWC    equ 01Eh                ; 2: its values' colours, left and right
; the dice: what a tick needs, taken at state time
DICE    equ 020h                ; 5: the faces, '1'-'6' (0: none rolled)
WKEEP   equ 025h                ; the wire's keepRoll: bit k, die k re-rolled
HOLD    equ 026h                ; our held dice on our turn: bit k, die k
ROLLS   equ 027h                ; rollsLeft
LROLLS  equ 028h                ; ... at the last state (0FFh: none)
VIEWNG  equ 029h                ; we are a spectator
VIEW    equ 02Ah                ; the seat whose card is shown
CUR     equ 02Bh                ; the cursor (CU_*)
FILLED  equ 02Dh                ; 2: the slots the seat to move has filled
REVSL   equ 02Fh                ; a slot somebody just took (0FFh: none) ...
REVTK   equ 030h                ; ... and the frames it stays lit
ANIM    equ 031h                ; the roll's frames to go ...
ANMASK  equ 032h                ; ... and the dice it shakes
DSIG    equ 033h                ; the dice as last drawn
LSCR    equ 034h                ; the screen up (SC_*; 0FFh: draw it afresh)
STAND   equ 035h                ; the standings asked for (S)
; the request in flight (net.inc)
NSTATE  equ 036h                ; NS_*
NSEQ    equ 037h                ; the sequence number posted
NWAIT   equ 038h                ; frames without progress
ACT     equ 039h                ; the action queued (0: none, a poll)
ACTARG  equ 03Ah                ; its argument
ACTSENT equ 03Bh                ; the action under way
NARG    equ 03Ch                ; its argument
NBWL    equ 03Dh                ; 2: STATUS's last bytes-waiting
NETHAVE equ 03Fh                ; how it ended: NH_*
RXLEN   equ 040h                ; 2: the reply's length
; who and where
SCNAME  equ 042h                ; 9: the player's name, NUL-terminated
SCTABL  equ 04Bh                ; 9: the table's id
; the boot's, and at the table the same bytes again
SEL     equ 054h                ; a list's selection
NTBL    equ 055h                ; tables listed
NAMEN   equ 056h                ; the name typed so far
SCR     equ SEL                 ; the screen this state draws (SC_*)
AUTOT   equ NTBL                ; AUTOPLAY: frames to think
LKEYS   equ NAMEN               ; the room appkey: 1 it named our table
; the table: what a tick needs, taken at state time (the reply window is
; repainted once the next request is under way)
NSEAT   equ 057h                ; playerCount
ROUND   equ 058h                ; 0 the lobby, 1-13, 99 over
ACTV    equ 059h                ; the seat to move (0FFh: nobody)
MOVET   equ 05Ah                ; its seconds left
VALID   equ 05Bh                ; 15: what each slot would score (0FFh none)
LACTV   equ 06Ah                ; ACTV at the last state (0FFh: none)
LNSEAT  equ 06Bh                ; NSEAT ...
LROUND  equ 06Ch                ; ROUND ... (0FFh: none)
SECF    equ 06Dh                ; frames to the clock's next second
POLLW   equ 06Eh                ; frames until the next poll
RSIG    equ 06Fh                ; 8: each row's signature
HELP    equ 077h                ; 1: the help screen is up; 2: closing
T0SIG   equ 078h                ; a signature of what T0 shows
TMP2    equ 079h
MYTURN  equ 07Ah                ; 1: it's our move (round 1-13, us, seated)
JOINS   equ 07Bh                ; the boot: failed joins
LCUR    equ 07Ch                ; the cursor as drawn (0FFh: hidden)
MYREADY equ 07Dh                ; the lobby: we said we're ready
RNEXT   equ 07Eh                ; the card's next row to draw (NROWS: done)
LEADS   equ 07Fh                ; the leader's seat (0FFh: none yet)
    if JOINS > 07Fh
    error "external RAM is full"
    endif

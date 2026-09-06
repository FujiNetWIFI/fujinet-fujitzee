; dice.inc -- the five dice, the hold markers, the ROLL tile and the
; roll animation.
;
; A die is 2 cells wide by 3 tall -- 16 x 24 pixels -- drawn as a SOLID
; BODY in the die's state colour with the pips punched through to the
; blue page. That inversion is what makes six faces cost three UDCs
; instead of twenty-four: the body is the built-in full-cell block
; ($03), and a pip is just a block with a hole in it. The left and right
; pip columns land at the same cell-relative position, so one glyph
; serves both; only the centre pip straddles the cell boundary and needs
; its two halves (see tools/mkudc.py for the pixel geometry).
;
; The die's COLOUR is its state, which is the Intellivision port's
; scheme (white / yellow-kept / green-cursor) recast in the four inks
; this machine gives us -- and an ordinary die comes out WHITE with blue
; pips, which is what a die is supposed to look like:
;
;     ANORM white    an ordinary die
;     AHOT  magenta  held -- the wire's keepRoll says '0', keep it
;     APOT  cyan     under the dice cursor (overrides held)
;
; and because the cursor overrides, the hold markers on the row beneath
; carry the held state independently: two magenta blocks under a die
; will be kept. So a die that is both held and selected still reads.
;
; The animation rattles only the dice the wire's keepRoll echo says were
; re-rolled, so it is right for everyone's rolls, not just yours.

DIROW   EQU     10              ; the dice strip's first row
DICOL   EQU     1               ; die 0's left column; step 3 per die
MRKROW  EQU     13              ; the hold markers, directly beneath
ROLROW  EQU     14              ; the ROLL tile

; ---- one die ----------------------------------------------------------
; DIEPOS: put the screen cursor at row R1 of die (V_TM2)'s two cells.
; R1 = the ROW OFFSET 0-2 on entry. Tail-calls DPOS. Clobbers R0-R2.
DIEPOS: LODZ    R1
        ADDI,R0 DIROW
        STRZ    R1
        LODA,R0 V_TM2
        STRZ    R2
        RRL,R0                  ; i*2
        ADDZ    R2              ; + i = i*3
        ADDI,R0 DICOL
        STRZ    R2
        BCTA,UN DPOS

; DIEDRW: draw die (V_TM2) showing face R1 (1-6; anything else is an
; empty tray). DCOLOR must already hold the die's state colour.
; R3 carries the row across DPOS and DPUTC, both of which leave it
; alone. Calls two down (DIEPOS -> DPOS). Clobbers R0-R3, V_FBAS.
DIEDRW: LODZ    R1
        BCTR,EQ DIEMT
        COMI,R1 7
        BCFR,LT DIEMT
        SUBI,R0 1               ; the face's base into FACETAB, x6
        STRZ    R1
        RRL,R0                  ; x*2
        STRZ    R3
        RRL,R0                  ; x*4
        ADDZ    R3              ; x*6
        STRA,R0 V_FBAS
        LODI,R3 0
DID1:   LODZ    R3
        STRZ    R1
        BSTA,UN DIEPOS
        LODA,R0 V_FBAS
        STRZ    R1
        LODA,R0 *PFACE,R1       ; FACETAB is in block 2; indirect reaches
        BSTA,UN DPUTC           ;   it, direct indexed could not
        LODA,R0 V_FBAS
        ADDI,R0 1
        STRZ    R1
        LODA,R0 *PFACE,R1
        BSTA,UN DPUTC
        LODA,R0 V_FBAS
        ADDI,R0 2
        STRA,R0 V_FBAS
        LODZ    R3
        ADDI,R0 1
        STRZ    R3
        COMI,R0 3
        BCFA,EQ DID1
        RETC,UN
DIEMT:  LODI,R3 0               ; no die here yet: back to the page
DIM1:   LODZ    R3
        STRZ    R1
        BSTA,UN DIEPOS
        EORZ    R0
        BSTA,UN DPUTC
        EORZ    R0
        BSTA,UN DPUTC
        LODZ    R3
        ADDI,R0 1
        STRZ    R3
        COMI,R0 3
        BCFR,EQ DIM1
        RETC,UN

PFACE:  DWBE    FACETAB

; ---- the hold mask ----------------------------------------------------
; KEEPGT: R0 = the mask character for die (V_TM2) -- '0' keep, '1'
; re-roll. On your turn that is the LOCAL staging buffer, because you
; are still choosing; otherwise it is the wire's echo, which is what
; makes another player's held dice visible on your screen.
; Tail-calls RXHDR in the wire case, so it costs one frame either way.
; Clobbers R0,R1,R3.
KEEPGT: LODA,R0 V_MYTRN
        BCTR,EQ KPGW
        LODA,R0 V_TM2
        STRZ    R1
        LODA,R0 KEEPBF,R1       ; page-0 RAM, and this module is page 0
        RETC,UN
KPGW:   LODA,R0 V_TM2
        ADDI,R0 GOKEEP
        STRZ    R1
        BCTA,UN RXHDR           ; tail call

; DIECOL: DCOLOR = die (V_TM2)'s state colour.
; Calls one down (KEEPGT). Clobbers R0,R1,R3.
DIECOL: LODA,R0 V_MYTRN         ; the cursor exists only on your turn,
        BCTR,EQ DIC1            ;   and only in DICE mode
        LODA,R0 UIMODE
        BCFR,EQ DIC1
        LODA,R0 V_TM2
        ADDI,R0 1               ; cursor 0 is the ROLL tile
        COMA,R0 DCUR
        BCFR,EQ DIC1
        DCOL    APOT            ; under the cursor: cyan
        RETC,UN
DIC1:   BSTA,UN KEEPGT
        COMI,R0 '0'
        BCFR,EQ DIC2
        DCOL    AHOT            ; held: magenta
        RETC,UN
DIC2:   DCOL    ANORM
        RETC,UN

; ---- the strip --------------------------------------------------------
; DICEROW: the five dice as the wire has them. An out-of-range byte (or
; the NULs of a turn that has not rolled yet) draws an empty tray.
; Calls three down. Clobbers everything.
DICEROW: EORZ   R0
        STRA,R0 V_TM2
DICR1:  LODA,R0 V_TM2
        COMI,R0 5
        BCFA,LT DICR9
        BSTA,UN DIECOL
        LODA,R0 V_TM2
        ADDI,R0 GODICE
        STRZ    R1
        BSTA,UN RXHDR
        SUBI,R0 '0'             ; '1'-'6' -> 1-6; anything else -> 0
        BCTR,LT DICR2
        COMI,R0 7
        BCTR,LT DICR3
DICR2:  EORZ    R0
DICR3:  STRZ    R1
        BSTA,UN DIEDRW
        LODA,R0 V_TM2
        ADDI,R0 1
        STRA,R0 V_TM2
        BCTA,UN DICR1
DICR9:  RETC,UN

; DICEHUD: the hold markers (row 13, two magenta blocks under a die that
; will be kept) and the ROLL tile (row 14, with the rolls remaining).
; The tile is cyan under the cursor and vanishes when the rolls are --
; a dead tile that LOOKS dead is what stops the trigger being mashed at
; it. Calls three down. Clobbers everything.
DICEHUD: EORZ   R0
        STRA,R0 V_TM2
DIH1:   LODA,R0 V_TM2
        COMI,R0 5
        BCFA,LT DIH5
        BSTA,UN KEEPGT          ; the reply read first, then the screen
        STRZ    R3              ;   position: DPOS clobbers R0,R1,R2 but
        LODI,R1 MRKROW-DIROW    ;   leaves R3, so the mask rides there
        BSTA,UN DIEPOS
        LODZ    R3
        COMI,R0 '0'
        BCFR,EQ DIH2
        DCOL    AHOT
        LODI,R1 GBLOCK
        BCTR,UN DIH3
DIH2:   DCOL    ANORM
        LODI,R1 0
DIH3:   LODI,R3 2
        BSTA,UN DFILL
        LODA,R0 V_TM2
        ADDI,R0 1
        STRA,R0 V_TM2
        BCTA,UN DIH1

DIH5:   LODI,R1 GORLEFT
        BSTA,UN RXHDR
        STRA,R0 V_NUM+1         ; rollsLeft, for the "X n" field
        EORZ    R0
        STRA,R0 V_NUM
        LODA,R0 V_NUM+1
        BCFR,EQ DIH6
        DCOL    ADIM            ; no rolls left: a dead tile
        BCTR,UN DIH7
DIH6:   LODA,R0 V_MYTRN
        BCFR,EQ DIH6A
        DCOL    ADIM            ; someone else's roll: not yours to press
        BCTR,UN DIH7
DIH6A:  DCOL    ANORM
        LODA,R0 UIMODE
        BCFR,EQ DIH7
        LODA,R0 DCUR
        BCFR,EQ DIH7
        DCOL    APOT            ; the cursor is on the tile
DIH7:   DAT     ROLROW,0
        EORZ    R0
        BSTA,UN DPUTC
        SETSTR  TROLL
        LODI,R1 4
        BSTA,UN DPRTN           ; "ROLL", cols 1-4
        EORZ    R0
        BSTA,UN DPUTC
        LODI,R0 $31             ; 'X'
        BSTA,UN DPUTC
        LODI,R1 1
        BSTA,UN NUMFLD          ; the count, col 7
        LODI,R1 0               ; cols 8-15: opaque overdraw
        LODI,R3 8
        BCTA,UN DFILL           ; tail call

; ---- the roll animation -----------------------------------------------
; ROLLANM: a roll landed -- rattle the dice that moved. WHICH dice moved
; is the wire's own answer: the server echoes the mask the roll was made
; with, so keepRoll names them for your rolls and everyone else's alike.
; Eight flickers of random faces with a tick each; the caller repaints
; the truth on the ordinary render path afterwards.
; Calls three down. Clobbers everything, V_ANM, V_TM2.
ROLLANM: LODI,R0 8
        STRA,R0 V_ANM
RANM1:  EORZ    R0
        STRA,R0 V_TM2
RANM2:  LODA,R0 V_TM2
        COMI,R0 5
        BCFA,LT RANM3
        LODA,R0 V_TM2
        ADDI,R0 GOKEEP
        STRZ    R1
        BSTA,UN RXHDR
        COMI,R0 '1'
        BCFR,EQ RANM4           ; kept: it never moves
        DCOL    ANORM
        LODI,R1 6
        BSTA,UN RNDMOD
        ADDI,R0 1
        STRZ    R1
        BSTA,UN DIEDRW
RANM4:  LODA,R0 V_TM2
        ADDI,R0 1
        STRA,R0 V_TM2
        BCTA,UN RANM2
RANM3:  BSTA,UN SNDTICK
        LODA,R0 V_ANM
        SUBI,R0 1
        STRA,R0 V_ANM
        BCFA,EQ RANM1
        RETC,UN

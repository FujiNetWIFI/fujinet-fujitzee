; card.inc -- the scorecard and the standings panel.
;
; The card is the Intellivision port's compact two-column form, which
; happens to be exactly sixteen columns wide -- so it lands on this
; machine's screen without being redesigned. Slots 0-7 (ones..sixes, the
; upper total, the bonus) run down the left, slots 8-15 (the seven lower
; rows and the running total) down the right:
;
;     col 0    4      8    12
;         ONE  nnn    SET3 nnn
;         ...         ...
;         BON    -    TOT  nnnn
;
; Both geometries are arithmetic rather than a table -- row = (slot&7)+1,
; label col = slot&8, value col = (slot&8)+4 -- which is also what lets
; the cursor sprite find its row without one (sprites.inc:CARCUR).
;
; Everything renders straight out of the reply window. The only
; arithmetic in the whole client lives here: RUNTOT sums score slots
; 6-14, because the wire's grand total (slot 15) is empty until game
; over, and the standings need a live number every poll.
;
; Colour carries the state, which is the point of running normal mode:
;
;     filled score        ANORM white
;     potential score     APOT  cyan     (what that row would earn now)
;     closed / server row ADIM  blue     a dash in the page's own colour,
;                                        so the row simply reads as absent
;     the cursor's cell   AHOT  magenta  plus the yellow chevron sprite
;     a bot's new row      AHOT  magenta  held there for a beat by REVEAL
;
; and in the standings panel the whole row takes the seat's colour --
; magenta for whoever is up, cyan for the card you are looking at, white
; for everyone else. That is this machine's answer to the intv port's
; per-seat palette: six distinct name colours do not exist here, but the
; three that carry meaning do.

PANROW  EQU     15              ; the standings panel's first row
PANOVR  EQU     PANROW+PANMAX   ; the "+N MORE" line

; ---- geometry ---------------------------------------------------------
; CATLPS / CATPOS: put the screen cursor at slot R0's label / value
; field. Both tail-call DPOS, so neither costs a return-stack frame of
; its own. Clobbers R0-R3.
CATLPS: STRZ    R3
        ANDI,R0 7
        ADDI,R0 1               ; card rows are 1-8
        STRZ    R1
        LODZ    R3
        ANDI,R0 8               ; the label columns ARE 0 and 8
        STRZ    R2
        BCTA,UN DPOS

CATPOS: STRZ    R3
        ANDI,R0 7
        ADDI,R0 1
        STRZ    R1
        LODZ    R3
        ANDI,R0 8
        ADDI,R0 4               ; value fields at 4 and 12
        STRZ    R2
        BCTA,UN DPOS

; ---- the static labels ------------------------------------------------
; CARDST: the 16 row labels, drawn once per entry into the play class.
; The label table is four NUL-padded bytes per slot, parked in block 2;
; DPRTN reaches it through DSTRP, which is an indirect load and so
; carries the full 15 bits. Calls two down. Clobbers R0-R3.
CARDST: EORZ    R0
        STRA,R0 V_CAT
        DCOL    ANORM
CST1:   LODA,R0 V_CAT
        RRL,R0
        RRL,R0                  ; slot*4 = the label's offset
        ADDI,R0 CATLBL&$FF
        STRA,R0 DSTRP+1
        LODI,R0 0
        TPSL    $01             ; the carry out of the low add
        BCFR,EQ CST2
        LODI,R0 1
CST2:   ADDI,R0 CATLBL>>8
        STRA,R0 DSTRP
        LODA,R0 V_CAT
        BSTA,UN CATLPS
        LODI,R1 4
        BSTA,UN DPRTN
        LODA,R0 V_CAT
        ADDI,R0 1
        STRA,R0 V_CAT
        COMI,R0 16
        BCFA,EQ CST1
        RETC,UN

; ---- reading a player's scores ----------------------------------------
; RDSCOR: player R1, slot R2 -> V_NUM = the u16 score (stored high byte
; first, the way NUMFLD wants it; the wire is little-endian). R0 returns
; the HIGH byte, so $FF means unset ($FFFF) or spectator ($FFFE) --
; neither is ever a real score.
; Calls two down (RXNEXT -> MSLICE). Clobbers R0-R3.
RDSCOR: LODZ    R2
        RRL,R0                  ; slot*2
        ADDI,R0 PLSCORS
        STRZ    R2
        BSTA,UN PLSEEK
        BSTA,UN RXNEXT
        STRA,R0 V_NUM+1         ; low byte first on the wire
        BSTA,UN RXNEXT
        STRA,R0 V_NUM
        RETC,UN

; RUNTOT: V_NUM = player R1's running total -- the sum of score slots
; 6-14 (the upper total, the bonus, and the seven lower rows, all kept
; current by the server). Slot 15, the wire's own grand total, is empty
; until game over, so this is what the screen shows all game; it equals
; the wire's once the game ends.
;
; The nine words are contiguous, so this is ONE seek and a sequential
; walk -- RXNEXT preserves R2, which is why the loop counter can live
; there across the reads. Any $FFxx word is an unset or spectator
; marker, never a score: skipped. It accumulates straight into V_NUM,
; which is where NUMFLD reads, so every caller prints it without a copy
; -- and the two bytes that used to be V_TOT are the reveal's shadow.
;
; CLOBBERS R2, and so does RDSCOR: R2 is both PLSEEK's field argument
; and the screen cursor, so every caller reads the wire FIRST and
; positions AFTERWARDS. Getting that backwards is what put the whole
; standings panel's totals on the dice row.
; Calls two down (RXNEXT -> MSLICE). Clobbers R0-R3.
RUNTOT: LODI,R2 PLSCORS+2*SCUPTOT
        BSTA,UN PLSEEK
        EORZ    R0
        STRA,R0 V_NUM
        STRA,R0 V_NUM+1
        LODI,R2 9               ; slots 6..14
RTT1:   BSTA,UN RXNEXT
        STRA,R0 V_TMP           ; the addend, low
        BSTA,UN RXNEXT
        STRA,R0 V_TMP2          ; ...and high
        COMI,R0 $FF
        BCTR,EQ RTT2            ; $FFxx: not a score
        PPSL    $08             ; WC on for the 16-bit add
        CPSL    $01             ; no carry in
        LODA,R0 V_NUM+1
        ADDA,R0 V_TMP
        STRA,R0 V_NUM+1
        LODA,R0 V_NUM
        ADDA,R0 V_TMP2          ; high plus the carry out of the low
        STRA,R0 V_NUM
        CPSL    $09             ; WC off, carry clear: the convention
RTT2:   BDRR,R2 RTT1
        RETC,UN

; VALIDC: R0 = validScores[V_CAT] -- the exact points that row would
; earn with the dice on the table, or $FF for a closed row. Always in
; slice 0. Tail-calls RXHDR. Clobbers R0,R1,R3. Preserves R2.
VALIDC: LODA,R0 V_CAT
        ADDI,R0 GOVALID
        STRZ    R1
        BCTA,UN RXHDR

; SCCURQ: Z set when the score cursor is visible and sits on V_CAT.
; Leaf. Preserves R1,R2,R3.
SCCURQ: LODA,R0 V_MYTRN
        BCTR,EQ SCCQN
        LODA,R0 UIMODE          ; 1 = SCORE mode
        BCTR,EQ SCCQN
        LODA,R0 VIEWPL          ; and only ever on your own card
        BCFR,EQ SCCQN
        LODA,R0 SCUR
        COMA,R0 V_CAT
        RETC,UN
SCCQN:  LODI,R0 1               ; a load of 1 leaves CC nonzero
        RETC,UN

; ---- the card ---------------------------------------------------------
; CARDRW: the viewed player's 16 value cells. Potentials appear only on
; YOUR card, on YOUR turn, once the dice exist -- validScores[] is all
; zeros in the lobby and means nothing on anyone else's card.
; Calls four down (SCELL -> RDSCOR -> RXNEXT -> MSLICE). Clobbers all.
CARDRW: EORZ    R0
        STRA,R0 V_POT
        LODA,R0 VIEWPL
        BCFR,EQ CDW0
        LODA,R0 V_MYTRN
        BCTR,EQ CDW0
        LODI,R1 GODICE          ; NULs until the first roll lands
        BSTA,UN RXHDR
        BCTR,EQ CDW0
        LODI,R0 1
        STRA,R0 V_POT
CDW0:   EORZ    R0
        STRA,R0 V_CAT
CDW1:   BSTA,UN SCELL
        LODA,R0 V_CAT
        ADDI,R0 1
        STRA,R0 V_CAT
        COMI,R0 16
        BCFA,EQ CDW1
        RETC,UN

; SCELL: one value cell for slot (V_CAT). Reads come FIRST and the
; screen cursor is set LAST: CATPOS takes its column in R2, which is
; also the reply cursor's field argument, so positioning early would
; silently move the wrong one (the battleship PROMPT lesson).
; Calls three down. Clobbers all.
SCELL:  LODA,R0 V_CAT
        COMI,R0 15
        BCTA,EQ SCTOT
        STRZ    R2
        LODA,R0 VIEWPL
        STRZ    R1
        BSTA,UN RDSCOR
        COMI,R0 $FF
        BCTR,EQ SCUNS
        DCOL    ANORM           ; a filled score, in white
        BCTA,UN SCNUM
SCUNS:  LODA,R0 V_POT
        BCTA,EQ SCDASH
        LODA,R0 V_CAT
        COMI,R0 SCUPTOT         ; 6 and 7 are the server's own rows:
        BCTA,EQ SCDASH          ;   never selectable, no potential
        COMI,R0 SCBONUS
        BCTA,EQ SCDASH
        BSTA,UN VALIDC
        COMI,R0 $FF
        BCTA,EQ SCDASH          ; the row is closed
        STRZ    R1              ; keep the points across SCCURQ
        COMI,R0 0               ; explicit: STRZ happens to set CC on
        BCFR,EQ SCPOT           ;   this CPU, but do not rely on it
        BSTA,UN SCCURQ          ; a ZERO potential shows only under the
        BCFA,EQ SCBLNK          ;   cursor -- an open board is not a
SCPOT:  EORZ    R0              ;   wall of zeros (the C client's rule)
        STRA,R0 V_NUM
        LODZ    R1
        STRA,R0 V_NUM+1
        DCOL    APOT            ; a potential, in cyan
SCNUM:  BSTA,UN SCCURQ          ; the cursor's cell overrides everything
        BCFR,EQ SCNUM1
        DCOL    AHOT
SCNUM1: LODA,R0 V_CAT           ; ...and so does the row a bot just took.
        COMA,R0 REVSLT          ;   REVSLT is $FF except during a reveal,
        BCFR,EQ SCNUM2          ;   and $FF is not a slot, so this costs
        DCOL    AHOT            ;   one compare on every other render
SCNUM2: LODA,R0 V_CAT
        BSTA,UN CATPOS
        LODI,R1 3
        BCTA,UN NUMFLD          ; tail call

SCTOT:  LODA,R0 VIEWPL          ; slot 15: the running total, computed
        STRZ    R1              ;   here rather than by CARDRW, because
        BSTA,UN RUNTOT          ;   the loop's NUMFLD calls would have
        DCOL    ANORM           ;   eaten it. Four cells -- column 15 is
                                ;   free, and a long game can pass 999
        LODA,R0 V_CAT
        BSTA,UN CATPOS
        LODI,R1 4
        BCTA,UN NUMFLD          ; tail call

SCDASH: DCOL    ADIM            ; closed, or a row we may not read into:
        LODA,R0 V_CAT           ;   "  -" in the page's own blue, so the
        BSTA,UN CATPOS          ;   row reads as absent, not as a value
        EORZ    R0
        BSTA,UN DPUTC
        EORZ    R0
        BSTA,UN DPUTC
        LODI,R0 GDASH
        BCTA,UN DPUTC           ; tail call

SCBLNK: LODA,R0 V_CAT
        BSTA,UN CATPOS
        LODI,R1 0
        LODI,R3 3
        BCTA,UN DFILL           ; tail call

; ---- the header row ---------------------------------------------------
; CARDHD: row 0 -- the viewed player's name in the seat's colour, and
; the round as "R n/13" in magenta. Calls three down. Clobbers all.
CARDHD: LODA,R0 VIEWPL
        STRZ    R1
        BSTA,UN SEATCL          ; the viewed seat wears its own colour --
        LODA,R0 VIEWPL          ;   and SEATCL first, because PLSEEK
        STRZ    R1              ;   clobbers the R1 it reads
        LODI,R2 PLNAME
        BSTA,UN PLSEEK          ; seek BEFORE positioning: both take R2
        DAT     0,0
        LODI,R1 10
        BSTA,UN RXPRNT
        LODA,R0 V_RND           ; once it is over the round is 99, which
        COMI,R0 RNOVER          ;   is not a round: say so instead
        BCFR,EQ CHDR1
        DCOL    AHOT
        SETSTR  TOVER6
        LODI,R1 6
        BCTA,UN DPRTN           ; tail call
CHDR1:  DCOL    ANORM
        LODI,R0 $2B             ; 'R'
        BSTA,UN DPUTC
        DCOL    AHOT            ; the round number, in magenta
        EORZ    R0
        STRA,R0 V_NUM
        LODA,R0 V_RND
        STRA,R0 V_NUM+1
        LODI,R1 2
        BSTA,UN NUMFLD
        DCOL    ANORM
        LODI,R0 GDIAG           ; the built-in '/' -- no UDC spent
        BSTA,UN DPUTC
        LODI,R0 $11             ; "13"
        BSTA,UN DPUTC
        LODI,R0 $13
        BCTA,UN DPUTC           ; tail call

; SEATCL: set DCOLOR to seat R1's colour -- magenta for whoever is up,
; cyan for the seat being viewed, white otherwise. This is the one
; rule that makes a name mean something on a screen with four inks.
; Leaf. Preserves R1,R2.
SEATCL: LODZ    R1
        COMA,R0 V_ACT
        BCTR,EQ SEATA
        LODZ    R1
        COMA,R0 VIEWPL
        BCTR,EQ SEATV
        DCOL    ANORM
        RETC,UN
SEATA:  DCOL    AHOT
        RETC,UN
SEATV:  DCOL    APOT
        RETC,UN

; ---- the standings panel ----------------------------------------------
; PANEL: rows 15-20, one per seat: name, live running total, and the
; row's colour saying who is up and whose card is on screen. The intv
; port had 20 columns and needed a letter strip plus a hold-to-view
; overlay; 26 rows buy the real thing outright.
; Calls three down. Clobbers all.
PANEL:  EORZ    R0
        STRA,R0 V_SEAT
PAN1:   LODA,R0 V_SEAT
        COMA,R0 V_PC
        BCFA,LT PAN9            ; past the last seat
        COMI,R0 PANMAX
        BCFA,LT PAN9            ; past what fits on screen
        STRZ    R1
        BSTA,UN SEATCL
        LODA,R0 V_SEAT          ; the total FIRST -- RUNTOT clobbers R2,
        STRZ    R1              ;   and R2 is about to be the screen
        BSTA,UN RUNTOT          ;   cursor. V_NUM is RAM and survives.
        LODA,R0 V_SEAT
        STRZ    R1
        LODI,R2 PLNAME
        BSTA,UN PLSEEK          ; then the reply cursor...
        LODA,R0 V_SEAT
        ADDI,R0 PANROW
        STRZ    R1
        LODI,R2 0
        BSTA,UN DPOS            ; ...and only then the screen cursor
        LODI,R1 10
        BSTA,UN RXPRNT          ; the name, padded to 10 cells
        LODI,R1 4               ; RUNTOT left the total in V_NUM already
        BSTA,UN NUMFLD
        LODI,R1 0               ; the last two cells: opaque overdraw
        LODI,R3 2
        BSTA,UN DFILL
        LODA,R0 V_SEAT
        ADDI,R0 1
        STRA,R0 V_SEAT
        BCTA,UN PAN1
; The overflow line. A seat count change is one of the two things
; CHKNEW answers with a full clear, so this row never has to be erased
; here -- it is only ever drawn when it is true.
PAN9:   LODA,R0 V_PC
        COMI,R0 PANMAX+1
        BCTA,LT PANX
        SUBI,R0 PANMAX
        STRA,R0 V_NUM+1
        EORZ    R0
        STRA,R0 V_NUM
        DCOL    ANORM           ; NOT ADIM: this line has to be read
        DAT     PANOVR,0
        LODI,R0 $36             ; '+'
        BSTA,UN DPUTC
        LODI,R1 1
        BSTA,UN NUMFLD
        SETSTR  TMORE
        LODI,R1 14
        BCTA,UN DPRTN           ; tail call
PANX:   RETC,UN

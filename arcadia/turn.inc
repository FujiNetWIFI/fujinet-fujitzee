; turn.inc -- your turn. Two cursor modes over one bounded input slice:
; DICE mode walks the five dice and the ROLL tile beneath them (trigger
; holds a die or submits the roll), SCORE mode walks the open scorecard
; rows (trigger takes the row). Up from a die crosses to the scores;
; down from a die drops to the ROLL tile; down off the bottom of a score
; column returns to the dice while rolls remain.
;
; Like battleship's TARGET, TURNIN runs ONE bounded slice and returns;
; GAMELP re-polls and re-enters while it is still our turn, so a game
; that ends (or a table that empties) is noticed within a slice.
; UIMODE/DCUR/SCUR/KEEPBF persist across slices. KEYSCN costs exactly
; one frame, so the slice counter and the input scan are the same loop.
;
; The countdown reseeds from the wire's moveTime on every entry, so the
; clock self-corrects each poll; at zero it bails -- the server has
; already force-scored, and only the next poll can discover where the
; game went (the Intellivision lesson).
;
; A rejected press earns only a short slice. An accepted key restarts
; the full one, because a player still choosing must not be interrupted;
; but a trigger bounced off a dead ROLL tile has to give the poll loop
; its turn back, or mashing could starve it forever.
;
; V_FLAG on return: 0 slice out / timeout, 1 roll staged (KEEPBF),
; 2 score staged (SCOREIX), 3 leave.

TURNSL  EQU     240             ; ~4 s of frames: one accepted press
TURNBD  EQU     60              ; ~1 s: one rejected press
CLKROW  EQU     25
CLKCOL  EQU     13

TURNIN: EORZ    R0
        STRA,R0 V_FLAG
        STRA,R0 TGFRM
        LODI,R1 GOMVTIM
        BSTA,UN RXHDR
        STRA,R0 TGLEFT
        BSTA,UN CLKDRW          ; paint it on entry, not a second later
        LODI,R0 TURNSL
        STRA,R0 V_TICK
TILP:   BSTA,UN KEYSCN          ; exactly one frame
        COMI,R0 KNONE
        BCFA,EQ TIKEY
        LODA,R0 TGFRM           ; the local second hand
        ADDI,R0 1
        STRA,R0 TGFRM
        COMI,R0 60
        BCTR,LT TINX
        EORZ    R0
        STRA,R0 TGFRM
        LODA,R0 TGLEFT
        BCTA,EQ TIOUT           ; already zero: bail now
        SUBI,R0 1
        STRA,R0 TGLEFT
        BSTA,UN CLKDRW
        LODA,R0 TGLEFT
        BCTA,EQ TIOUT           ; hit zero: the server has skipped us
        COMI,R0 6
        BCFR,LT TINX
        BSTA,UN SNDTICK         ; the last five seconds tick
TINX:   LODA,R0 V_TICK
        SUBI,R0 1
        STRA,R0 V_TICK
        BCFA,EQ TILP
TIOUT:  RETC,UN
TIRST:  LODI,R0 TURNSL
        STRA,R0 V_TICK
        BCTA,UN TILP
TIBAD:  BSTA,UN SNDERR
        LODI,R0 TURNBD
        STRA,R0 V_TICK
        BCTA,UN TILP

; CLKDRW: the move clock, three red cells in the bottom-right corner.
; Calls two down. Clobbers R0-R3, V_NUM.
CLKDRW: EORZ    R0
        STRA,R0 V_NUM
        LODA,R0 TGLEFT
        STRA,R0 V_NUM+1
        DCOL    AHOT
        DAT     CLKROW,CLKCOL
        LODI,R1 3
        BCTA,UN NUMFLD          ; tail call

; ---- key dispatch -----------------------------------------------------
TIKEY:  STRA,R0 V_ST            ; the code, across every call below
        COMI,R0 KCLR            ; leave -- handed back to GAMELP so the
        BCFR,EQ TIK1            ;   return stack stays balanced
        LODI,R0 3
        STRA,R0 V_FLAG
        RETC,UN
TIK1:   LODA,R0 UIMODE
        BCFA,EQ TISCR

; ---- DICE mode --------------------------------------------------------
        LODA,R0 V_ST
        COMI,R0 KLEFT
        BCFR,EQ TID1
        LODA,R0 DCUR
        BCTA,EQ TIBAD           ; on the tile: left and right mean nothing
        SUBI,R0 1
        BCFR,EQ TIDL1
        LODI,R0 5               ; wrapped past die 1
TIDL1:  STRA,R0 DCUR
TIDMV:  BSTA,UN SNDKEY
        BSTA,UN DICEROW
        BSTA,UN DICEHUD
        BCTA,UN TIRST
TID1:   COMI,R0 KRIGHT
        BCFR,EQ TID2
        LODA,R0 DCUR
        BCTA,EQ TIBAD
        ADDI,R0 1
        COMI,R0 6
        BCTR,LT TIDR1
        LODI,R0 1
TIDR1:  STRA,R0 DCUR
        BCTA,UN TIDMV
TID2:   COMI,R0 KDOWN           ; down from a die: the ROLL tile
        BCFR,EQ TID3
        LODA,R0 DCUR
        BCTA,EQ TIBAD           ; already there
        EORZ    R0
        STRA,R0 DCUR
        BCTA,UN TIDMV
TID3:   COMI,R0 KUP
        BCFA,EQ TID4
        LODA,R0 DCUR            ; up from the tile: back to the dice
        BCFR,EQ TID3A
        LODI,R0 1
        STRA,R0 DCUR
        BCTA,UN TIDMV
TID3A:  LODI,R1 GODICE          ; up from a die: the score rows, once
        BSTA,UN RXHDR           ;   there are dice to score
        BCTA,EQ TIBAD
        LODI,R0 1
        STRA,R0 UIMODE
        BSTA,UN SCURFX
        BSTA,UN SNDSEL
        BSTA,UN CARDRW          ; show the score cursor
        BSTA,UN CARCUR
        BSTA,UN DICEROW         ; and clear the dice cursor
        BSTA,UN DICEHUD
        BCTA,UN TIRST
TID4:   COMI,R0 KENT            ; Enter and FIRE/'2' are both select
        BCTR,EQ TIDSEL
        COMI,R0 2
        BCFA,EQ TIRST
TIDSEL: LODA,R0 DCUR
        BCTA,EQ TIDROL
        SUBI,R0 1               ; a die: toggle held <-> re-roll
        STRZ    R1
        LODA,R0 KEEPBF,R1
        EORI,R0 1               ; '0' <-> '1'
        STRA,R0 KEEPBF,R1
        BSTA,UN SNDHLD
        BSTA,UN DICEROW
        BSTA,UN DICEHUD
        BCTA,UN TIRST
TIDROL: LODI,R1 GORLEFT         ; the ROLL tile: submit the roll
        BSTA,UN RXHDR
        BCTA,EQ TIBAD           ; no rolls left
        LODI,R1 0               ; at least one die must actually move
TIDR2:  LODA,R0 KEEPBF,R1
        COMI,R0 '1'
        BCTR,EQ TIDGO
        LODZ    R1
        ADDI,R0 1
        STRZ    R1
        COMI,R1 5
        BCTR,LT TIDR2
        BCTA,UN TIBAD           ; everything held: nothing to roll
TIDGO:  LODI,R0 1
        STRA,R0 V_FLAG
        BCTA,UN SNDROLL         ; tail call

; ---- SCORE mode -------------------------------------------------------
TISCR:  LODA,R0 V_ST
        COMI,R0 KUP
        BCFR,EQ TIS1
        LODA,R0 SCUR
        STRZ    R1
        BSTA,UN SKPUP
        COMI,R0 $FF
        BCTA,EQ TIBAD           ; already at the top of the column
TISMV:  STRA,R0 SCUR
        BSTA,UN SNDKEY
        BSTA,UN CARDRW
        BSTA,UN CARCUR
        BCTA,UN TIRST
TIS1:   COMI,R0 KDOWN
        BCFA,EQ TIS2
        LODA,R0 SCUR
        STRZ    R1
        BSTA,UN SKPDN
        COMI,R0 $FF
        BCFR,EQ TISMV
        LODI,R1 GORLEFT         ; off the bottom: back to the dice --
        BSTA,UN RXHDR           ;   unless the rolls are spent
        BCTA,EQ TIBAD
        EORZ    R0
        STRA,R0 UIMODE
        LODI,R0 1
        STRA,R0 DCUR
        BSTA,UN SNDSEL
        BSTA,UN CARDRW          ; clear the score cursor
        BSTA,UN CARHID
        BSTA,UN DICEROW         ; and show the dice one
        BSTA,UN DICEHUD
        BCTA,UN TIRST
TIS2:   COMI,R0 KLEFT
        BCTR,EQ TISCOL
        COMI,R0 KRIGHT
        BCFA,EQ TIS3
TISCOL: LODA,R0 SCUR            ; the matching row of the other column
        COMI,R0 8
        BCFR,LT TISC1
        ADDI,R0 8
        BCTR,UN TISC2
TISC1:  SUBI,R0 8
        COMI,R0 6               ; slots 14 and 15 sit beside the always-
        BCTR,LT TISC2           ;   closed upper total: settle on sixes
        LODI,R0 5
TISC2:  STRA,R0 V_TMP           ; the landing slot, across VALIDC. NOT
        STRA,R0 V_CAT           ;   V_SAV: that is RXNEXT's R2 shelter
        BSTA,UN VALIDC
        COMI,R0 $FF             ; branch on the compare BEFORE anything
        BCTR,EQ TISCC           ;   else touches CC -- on this CPU every
        LODA,R0 V_TMP           ;   load, LODI and STRZ included, sets it
        BCTA,UN TISMV           ; open: land there
TISCC:  LODA,R0 V_TMP           ; closed: nearest open below, then above
        STRZ    R1
        BSTA,UN SKPDN
        COMI,R0 $FF
        BCFA,EQ TISMV
        LODA,R0 V_TMP
        STRZ    R1
        BSTA,UN SKPUP
        COMI,R0 $FF
        BCFA,EQ TISMV
        BCTA,UN TIBAD           ; the whole column is closed
TIS3:   COMI,R0 KENT
        BCTR,EQ TISSEL
        COMI,R0 2
        BCFA,EQ TIRST
TISSEL: LODA,R0 SCUR
        STRA,R0 V_CAT
        BSTA,UN VALIDC
        COMI,R0 $FF
        BCTA,EQ TIBAD           ; a closed row (should be unreachable)
        STRA,R0 V_TMP           ; the points, for the fanfare test
        LODA,R0 SCUR
        STRA,R0 SCOREIX
        LODI,R0 2
        STRA,R0 V_FLAG
        LODA,R0 SCUR            ; a real fujitzee gets its own cue
        COMI,R0 SCFUJI
        BCFR,EQ TISSC
        LODA,R0 V_TMP
        BCFA,EQ SNDFUJI         ; tail call
TISSC:  BCTA,UN SNDSCOR         ; tail call

; ----------------------------------------------------------------------
; VALIDC lives in card.inc: R0 = validScores[V_CAT], $FF when closed.
;
; SKPDN / SKPUP: from slot R1, the next OPEN row down / up the same
; column; R0 = the slot, or $FF when there is none. The always-closed
; slots (the upper total and the bonus) skip like any other closed row,
; and slot 15 is never visited -- validScores[] has only fifteen entries
; and offset 95 is the first player record, not a score.
; The walking slot lives in V_CAT because VALIDC clobbers every register
; a loop counter could use.
; Calls two down (VALIDC -> RXNEXT). Clobbers R0-R3, V_CAT.
SKPDN:  LODZ    R1
        LODI,R2 15              ; the right column ends before slot 15
        COMI,R0 8               ; ...and the LODI must come FIRST: it
        BCFR,LT SKD0            ;   sets CC, so a compare it follows is
        LODI,R2 8               ;   already lost
SKD0:   LODZ    R1
        STRA,R0 V_CAT
SKD1:   LODA,R0 V_CAT
        ADDI,R0 1
        STRA,R0 V_CAT
        COMZ    R2
        BCFR,LT SKDF
        BSTA,UN VALIDC
        COMI,R0 $FF
        BCTR,EQ SKD1
        LODA,R0 V_CAT
        RETC,UN
SKDF:   LODI,R0 $FF
        RETC,UN

SKPUP:  LODZ    R1
        LODI,R2 8               ; the right column starts at slot 8
        COMI,R0 8
        BCFR,LT SKU0
        LODI,R2 0               ; the left column starts at slot 0
SKU0:   LODZ    R1
        STRA,R0 V_CAT
SKU1:   LODA,R0 V_CAT
        COMZ    R2
        BCTR,EQ SKUF            ; already at the top of the column
        SUBI,R0 1
        STRA,R0 V_CAT
        BSTA,UN VALIDC
        COMI,R0 $FF
        BCTR,EQ SKU1
        LODA,R0 V_CAT
        RETC,UN
SKUF:   LODI,R0 $FF
        RETC,UN

; PICKBEST: park the score cursor on the highest-paying open row (the
; first wins a tie) -- where it rests once the rolls run out, so
; trigger-trigger-trigger plays a legal if artless game.
; Calls two down. Clobbers everything, V_CAT, V_TMP, V_TMP2.
PICKBEST: EORZ  R0
        STRA,R0 V_CAT
        STRA,R0 V_TMP           ; the best points so far
        LODI,R2 0               ; the best slot; R2 survives VALIDC
        LODI,R0 $FF
        STRA,R0 V_TMP2          ; nothing found yet
PKB1:   LODA,R0 V_CAT
        COMI,R0 15
        BCFA,LT PKB9
        BSTA,UN VALIDC
        COMI,R0 $FF
        BCTR,EQ PKB2            ; closed
        STRZ    R3
        LODA,R0 V_TMP2
        COMI,R0 $FF
        BCTR,EQ PKBT            ; the first open row is the baseline
        LODZ    R3
        COMA,R0 V_TMP
        BCFR,GT PKB2            ; not strictly better: the first wins
PKBT:   LODZ    R3
        STRA,R0 V_TMP
        EORZ    R0
        STRA,R0 V_TMP2
        LODA,R0 V_CAT
        STRZ    R2
PKB2:   LODA,R0 V_CAT
        ADDI,R0 1
        STRA,R0 V_CAT
        BCTA,UN PKB1
PKB9:   LODZ    R2
        STRA,R0 SCUR
        RETC,UN

; SCURFX: keep the score cursor on an open row when entering the mode.
; Calls two down. Clobbers everything.
SCURFX: LODA,R0 SCUR
        STRA,R0 V_CAT
        BSTA,UN VALIDC
        COMI,R0 $FF
        BCFR,EQ SCFX9
        BCTA,UN PICKBEST        ; tail call
SCFX9:  RETC,UN

; ---- the keep mask ----------------------------------------------------
; KEEPRST: back to roll-everything. KEEPSYN: restage it from the wire's
; echo after a roll lands, so the next submit re-rolls the same dice
; unless the player retunes it; an empty echo falls back to
; roll-everything. Between them they are also what makes a REJOIN work:
; your turn can arrive with the dice already on the table.
; KEEPSYN calls two down. Clobber R0-R3.
KEEPRST: LODI,R1 0
KRS1:   LODI,R0 '1'
        STRA,R0 KEEPBF,R1
        LODZ    R1
        ADDI,R0 1
        STRZ    R1
        COMI,R1 5
        BCTR,LT KRS1
        EORZ    R0
        STRA,R0 KEEPBF+5
        RETC,UN

KEEPSYN: LODI,R2 0
KSY1:   LODZ    R2
        ADDI,R0 GOKEEP
        STRZ    R1
        BSTA,UN RXHDR           ; preserves R2
        STRZ    R3
        LODZ    R2
        STRZ    R1
        LODZ    R3
        STRA,R0 KEEPBF,R1
        LODZ    R2
        ADDI,R0 1
        STRZ    R2
        COMI,R2 5
        BCTR,LT KSY1
        EORZ    R0
        STRA,R0 KEEPBF+5
        LODA,R0 KEEPBF          ; an empty echo: roll everything
        BCFR,EQ KSY9
        BCTA,UN KEEPRST
KSY9:   RETC,UN

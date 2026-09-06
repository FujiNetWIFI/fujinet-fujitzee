; sprites.inc -- the 2637's movable objects, trimmed to the one this
; game needs: sprite 0, the scorecard cursor chevron.
;
; The sprite earns its place twice over. Normal colour mode has one
; global background, so there is no inverse cell to mark the cursor with
; -- and a sprite carries its OWN 3-bit colour, independent of the four
; screen inks, which makes it a fifth colour for free.
;
; Sprite images ARE the first four UDC slots, so sprite 0's shape is
; whatever sits at UDC $38 -- which is why GCHEVR is pinned there. Sprites
; 1-3 are parked for good, so their images ($39-$3B) are free to be the
; ordinary glyphs the dice are built from.
;
; Position registers take raw values: y raw = 256 - y_internal,
; x raw = x_internal + 43 (the MAME model; cells are 8x8 in 26-line mode
; so SPYOFF 0 / SPXOFF 43 lands a sprite dead on a cell).

SPYOFF  EQU     0               ; added to row*8 before negation
SPXOFF  EQU     43              ; added to col*8

; SPRINI: colour, size, and park everything. Sprite 0 is 8x8 and YELLOW
; -- a hue the four screen inks do not have, which is the whole reason
; the cursor is a sprite; white would vanish into the white text.
; The rest are parked offscreen. Leaf. Clobbers R0,R3.
SPRINI: LODI,R0 $C0|(CYELLOW<<3) ; b7/b6 8x8, spr0 yellow, spr1 white
        STRA,R0 SPRC01
        LODI,R0 $C0
        STRA,R0 SPRC23
        LODI,R3 7
SPIN1:  EORZ    R0
        STRA,R0 SPRPOS,R3
        BDRR,R3 SPIN1
        EORZ    R0
        STRA,R0 SPRPOS
        RETC,UN

; SPRSET: sprite R1 (0-3) to raw y=R2, x=R3. Leaf. Clobbers R0,R1.
SPRSET: LODZ    R1
        RRL,R0
        STRZ    R1              ; index*2
        LODZ    R2
        STRA,R0 SPRPOS,R1
        LODZ    R3
        STRA,R0 SPRPOS+1,R1
        RETC,UN

; SPRCELL: sprite R1 at cell row R2 / col R3. Cells are 8x8 in 26-line
; mode, so the mapping is exact. Calls one down (SPRSET).
; Clobbers R0,R1,R2,R3.
SPRCELL: LODZ   R2
        RRL,R0
        RRL,R0
        RRL,R0                  ; row*8
        ADDI,R0 SPYOFF
        EORI,R0 $FF             ; 256 - v
        ADDI,R0 1
        STRZ    R2
        LODZ    R3
        RRL,R0
        RRL,R0
        RRL,R0                  ; col*8
        ADDI,R0 SPXOFF
        STRZ    R3
        BCTR,UN SPRSET          ; tail call

; SPRPARK: park sprite R1 offscreen. Calls one down. Clobbers R0-R3.
SPRPARK: LODI,R2 0
        LODI,R3 0
        BCTR,UN SPRSET          ; tail call

; CARCUR: put the chevron beside the scorecard row the cursor is on.
; The card is two columns of eight rows, slots 0-7 down the left and 8-15
; down the right, so the geometry is arithmetic and needs no table:
; row = (slot & 7) + 1, and the chevron sits in the spare column just
; past each column's value field -- col 7 or col 15 -- pointing back at
; it. Calls one down (SPRCELL). Clobbers R0-R3.
CARCUR: LODA,R0 SCUR
        ANDI,R0 7
        ADDI,R0 1               ; card rows are 1-8
        STRZ    R2
        LODA,R0 SCUR
        ANDI,R0 8               ; 0 = left column, 8 = right
        ADDI,R0 7               ; -> col 7 or col 15
        STRZ    R3
        LODI,R1 0
        BCTA,UN SPRCELL         ; tail call

; CARHID: take the chevron off screen -- every mode but SCORE.
; Calls one down. Clobbers R0-R3.
CARHID: LODI,R1 0
        BCTA,UN SPRPARK         ; tail call

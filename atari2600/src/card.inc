; card.inc -- the scorecard: one category per row, and the green that makes it
; worth being one.
;
; ONE CATEGORY PER ROW IS NOT A LAYOUT PREFERENCE. The kernel writes COLUP0
; and COLUP1 from the same value on a seam line that has four cycles in hand,
; so a text row has exactly one ink colour. A two-column card -- which is what
; every other port in this family uses, and what fits 16 columns exactly --
; could not paint one category green without painting its neighbour green too.
;
; Rows RCARD0..RTOTAL, fifteen of them:
;
;   idx 0-5    ONE..SIX            slots 0-5
;   idx 6      "UP nnn  B nn"      slots 6 and 7, both server-computed
;   idx 7-13   SET 3..FUJITZEE     slots 8-14
;   idx 14     TOTAL               summed, not read: slot 15 is filled only
;                                  at game over
;
; and the label is eight columns, the value three, right-aligned, with a
; column of gap. Three is wider than the thirteen selectable categories need
; -- they cap at 50 -- but the totals reach three digits and a uniform field
; deletes the class of bug where a value outgrows its column.

; The slot each card row shows. $FF is the computed-pair row and $FE the
; running total; neither is a slot and neither can ever be chosen.
CSLOTF  EQU     $FF
CTOTF   EQU     $FE
CSLOTS: DB      0,1,2,3,4,5,CSLOTF,8,9,10,11,12,13,14,CTOTF

; ---------------------------------------------------------------------------
; CARDIX -- the card row index for score slot A, or $FF if it has none.
; The inverse of CSLOTS, used to put the cursor and the bot reveal on a row.
CARDIX: ldx     #NCARDR-1
CIX1:   cmp     CSLOTS,x
        beq     CIX2
        dex
        bpl     CIX1
        lda     #$FF
        rts
CIX2:   txa
        rts

; ---------------------------------------------------------------------------
; GRNBLD -- build the potential mask FZGRM from validScores.
;
; Row R's bit is bit 16-R, because the seam line that programmes row R is the
; Rth and each one consumes a bit from the top. Off by one here and the whole
; card is green one row up.
;
; THE LOBBY SENDS FIFTEEN ZEROS and zero is a legal potential, so this is only
; ever called in rounds 1-13; every other class clears the mask outright.
GRNBLD: lda     #0
        sta     FZGRM0
        sta     FZGRM1
        lda     FZRND
        beq     GRNB9           ; the lobby: nothing is a potential
        cmp     #RNOVER
        beq     GRNB9
        lda     FZMYTRN         ; and a potential is only yours to take
        beq     GRNB9
        lda     FZVIEW          ; ...on your own card
        bne     GRNB9
        ldx     #0
GRNB1:  lda     CSLOTS,x
        cmp     #CSLOTF
        bcs     GRNB3           ; the computed rows are never potentials
        tay
        lda     FNRPLY+GOVALID,y
        cmp     #$FF
        beq     GRNB3           ; closed
        cmp     #1
        bcc     GRNB3           ; a zero potential is not worth a colour
        txa                     ; row R = RCARD0 + idx, bit 16-R
        clc
        adc     #RCARD0
        sta     FZTMP
        lda     #16
        sec
        sbc     FZTMP
        tay                     ; 0..14
        cpy     #8
        bcs     GRNB2
        lda     BITTAB,y        ; bits 0-7 are the low half: rows 9-16
        ora     FZGRM1
        sta     FZGRM1
        jmp     GRNB3
GRNB2:  tya
        sec
        sbc     #8
        tay
        lda     BITTAB,y
        ora     FZGRM0
        sta     FZGRM0
GRNB3:  inx
        cpx     #NCARDR
        bne     GRNB1
GRNB9:  rts

; ---------------------------------------------------------------------------
; CARDHD -- the name row: whose card this is, and whether they are up.
CARDHD: lda     #RNAME
        jsr     FNROWA
        lda     FZVIEW
        cmp     FZACT
        bne     CHD1
        lda     #'>'            ; this seat is the one to move
        bne     CHD2
CHD1:   lda     #' '
CHD2:   jsr     FNCHR
        lda     FZVIEW
        jsr     PLRECP
        ldy     #PLNAME
        ldx     #NAMEMAX
CHD3:   lda     (FNPTRL),y
        beq     CHD4
        jsr     FNCHR
        iny
        dex
        bne     CHD3
        jmp     CHD5
CHD4:   txa                     ; NUL: pad the field out, so a shorter name
        jsr     FNSPC           ;   does not leave the last one behind it
CHD5:   lda     #FNTCOL-1-NAMEMAX
        jsr     FNSPC
        jmp     FNENDW

; ---------------------------------------------------------------------------
; CARDRW -- compose card row X (0..NCARDR-1) for the seat in FZVIEW.
; The row index lives in FZIDX, not FZTMP: RUNTOT uses FZTMP as half of its
; accumulator, and the TOTAL row is the one that calls it.
CARDRW: stx     FZIDX
        txa
        clc
        adc     #RCARD0
        jsr     FNROWA
        ldx     FZIDX
        lda     CSLOTS,x
        cmp     #CSLOTF
        bne     CRWORD          ; the computed pair has its own shape, and it
        jmp     CRWUP           ;   is at the far end of this file
CRWORD:

; An ordinary row: eight columns of label, a gap, three of value.
        txa
        asl     a
        asl     a
        asl     a               ; idx*8 into the label block
        tay
        ldx     #CLABW
CRW1:   lda     CLABELS,y
        jsr     FNCHR
        iny
        dex
        bne     CRW1
        lda     #1
        jsr     FNSPC

        ldx     FZIDX
        lda     CSLOTS,x
        cmp     #CTOTF
        beq     CRWTOT

; Filled? The high byte alone decides -- $FFFF (unset) and $FFFE (spectator)
; are the only non-scores and both carry $FF there.
        tax
        pha
        lda     FZVIEW
        jsr     PLRECP
        pla
        tax
        jsr     RDSCOR
        bcc     CRWVAL          ; filled: the score, in white

; Open. What goes here is the POTENTIAL, and only on your own card on your own
; turn: on anybody else's it is not yours to take and the number would be a
; lie about whose roll it is.
        lda     FZRND
        beq     CRWBLK
        cmp     #RNOVER
        beq     CRWBLK
        lda     FZMYTRN
        beq     CRWBLK
        lda     FZVIEW
        bne     CRWBLK
        ldx     FZIDX
        ldy     CSLOTS,x
        lda     FNRPLY+GOVALID,y
        cmp     #$FF
        beq     CRWDSH          ; closed: a dash, in the page's own colour
        cmp     #1
        bcs     CRWPOT          ; something to score: the number, in green
; A zero potential shows ONLY under the cursor. An open card would otherwise
; be a wall of zeros, and the one number a player actually wants -- what this
; roll is worth HERE -- would be the hardest thing on the screen to find.
        ldx     FZIDX
        lda     CSLOTS,x
        cmp     FZSCUR          ; the cursor carries a SLOT, not a row index
        bne     CRWBLK
        lda     #0
CRWPOT: sta     FZNUM0
        lda     #0
        sta     FZNUM1
        jmp     CRWDEC

CRWVAL: jmp     CRWDEC          ; RDSCOR already left it in FZNUM0/1

CRWTOT: lda     FZVIEW
        jsr     PLRECP
        jsr     RUNTOT
CRWDEC: lda     #CVALW
        jsr     FZDEC
        jmp     FNENDW

CRWBLK: lda     #CVALW
        jsr     FNSPC
        jmp     FNENDW

CRWDSH: lda     #CVALW-1
        jsr     FNSPC
        lda     #'-'
        jsr     FNCHR
        jmp     FNENDW

; The two server-computed upper rows share a row, because neither is ever
; selectable and neither is ever a potential: "UP nnn  B nn".
CRWUP:  ldx     #CUPLAB&$FF
        ldy     #CUPLAB>>8
        jsr     FNSTRP
        lda     FZVIEW
        jsr     PLRECP
        ldx     #SCUPTOT
        jsr     RDSCOR
        bcc     CRWU1
        lda     #0
        sta     FZNUM0
        sta     FZNUM1
CRWU1:  lda     #CVALW
        jsr     FZDEC
        ldx     #CBOLAB&$FF
        ldy     #CBOLAB>>8
        jsr     FNSTRP
        lda     FZVIEW
        jsr     PLRECP
        ldx     #SCBONUS
        jsr     RDSCOR
        bcc     CRWU2
        lda     #0
        sta     FZNUM0
        sta     FZNUM1
CRWU2:  lda     #2
        jsr     FZDEC
        lda     #2
        jsr     FNSPC
        jmp     FNENDW

CUPLAB: DB      "UP",0
CBOLAB: DB      " B",0

; Eight columns each, in card-row order. Fixed width and no terminator: the
; row wants exactly eight characters and a table of pointers would cost more
; than the padding does.
CLABELS:
        DB      "ONE     "
        DB      "TWO     "
        DB      "THREE   "
        DB      "FOUR    "
        DB      "FIVE    "
        DB      "SIX     "
        DB      "        "      ; the computed pair: never reached
        DB      "SET 3   "
        DB      "SET 4   "
        DB      "FULL HSE"
        DB      "SM STRT "
        DB      "LG STRT "
        DB      "CHANCE  "
        DB      "FUJITZEE"
        DB      "TOTAL   "

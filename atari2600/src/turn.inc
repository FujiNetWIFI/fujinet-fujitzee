; turn.inc -- your turn: where the cursor is, and what a press of it means.
;
; Two modes. FZUIMD 0 is the TRAY -- left and right walk the five dice and the
; ROLL tile, fire holds or rolls -- and 1 is the CARD, where up and down walk
; the fifteen rows and fire takes one. Up from the tray crosses to the card and
; down from the card's last row drops back to the ROLL tile, so the two are one
; space and not two screens.
;
; THE CURSOR STARTS ON ROLL, because rolling is always the first move of a
; turn, and after the last roll PICKBEST parks it on the highest-paying open
; row -- so fire, fire, fire, fire plays a legal, if artless, game.

; TURNIN -- one frame of your turn. A = the newly-pressed mask.
; Leaves FZREQ2 set when a move has been staged, so the poll that follows
; carries it INSTEAD of /state and there is never a second round trip.
TURNIN: sta     FZTMP
        lda     FZMYTRN
        bne     TIN1
        rts
TIN1:   lda     FNRPLY+GOVIEW   ; a spectator watches and does not play
        beq     TIN2
        rts
TIN2:   lda     FZUIMD
        bne     TICARD

; ---- the tray ----
        lda     FZTMP
        and     #IN_LEFT
        beq     TIT1
        lda     FZDCUR
        beq     TIT9
        dec     FZDCUR
        jsr     TROLLM
        lda     #SN_CLICK
        jmp     SNDCUE
TIT1:   lda     FZTMP
        and     #IN_RIGHT
        beq     TIT2
        lda     FZDCUR
        cmp     #NDICE          ; NDICE is the ROLL tile, one past the dice
        bcs     TIT9
        inc     FZDCUR
        jsr     TROLLM
        lda     #SN_CLICK
        jmp     SNDCUE
TIT2:   lda     FZTMP
        and     #IN_UP
        beq     TIT3
        lda     #1              ; cross to the card
        sta     FZUIMD
        jsr     PICKBEST
        lda     #SN_CLICK
        jmp     SNDCUE
TIT3:   lda     FZTMP
        and     #IN_FIRE
        beq     TIT9
        lda     FZDCUR
        cmp     #NDICE
        beq     TIROLL
; Hold or release the die under the cursor. FZKEEP carries the WIRE's sense --
; a set bit RE-ROLLS -- so this flips between held and in flight, and the art
; and the marker row both follow from the same bit.
        tay
        lda     BITTAB,y
        eor     FZKEEP
        sta     FZKEEP
        jsr     DSTAGE
        jsr     THOLDM
        lda     #SN_HOLD
        jmp     SNDCUE
TIROLL: lda     FZRLEFT
        beq     TIBAD           ; no rolls left: the only move now is a row
        lda     FZKEEP
        and     #(1<<NDICE)-1
        beq     TIBAD           ; every die held: there is nothing to roll
        lda     #RQROLL
        sta     FZREQ2
        lda     #SN_ROLL
        jmp     SNDCUE
TIT9:   rts
TIBAD:  lda     #SN_BAD
        jmp     SNDCUE

; ---- the card ----
TICARD: lda     FZTMP
        and     #IN_UP
        beq     TIC1
        jsr     SKPUP
        lda     #SN_CLICK
        jmp     SNDCUE
TIC1:   lda     FZTMP
        and     #IN_DOWN
        beq     TIC2
        jsr     SKPDN
        bcc     TIC1A
        lda     #0              ; past the last row: back to the tray
        sta     FZUIMD
        lda     #NDICE
        sta     FZDCUR
TIC1A:  lda     #SN_CLICK
        jmp     SNDCUE
TIC2:   lda     FZTMP
        and     #IN_FIRE
        beq     TIC9
; Taking a row. The server refuses a closed one silently and the turn simply
; never passes, so it is refused HERE instead, with a tone.
        ldx     FZSCUR
        lda     FNRPLY+GOVALID,x
        cmp     #$FF
        beq     TIBAD
        stx     FZSCIX
        lda     #RQSCORE
        sta     FZREQ2
        lda     #SN_SEL
        jmp     SNDCUE
TIC9:   rts

; SKPUP / SKPDN -- move FZSCUR to the next SELECTABLE slot.
;
; Slots 6, 7 and 15 are server-computed and can never be chosen, so the cursor
; steps over them rather than sitting on a row a press would do nothing to.
; SKPDN returns carry SET when it has run off the bottom.
SKPUP:  ldx     FZSCUR
SKU1:   dex
        bmi     SKU2
        cpx     #SCUPTOT
        beq     SKU1
        cpx     #SCBONUS
        beq     SKU1
        stx     FZSCUR
        clc
        rts
SKU2:   clc
        rts

SKPDN:  ldx     FZSCUR
SKD1:   inx
        cpx     #SCGRAND
        bcs     SKD2
        cpx     #SCUPTOT
        beq     SKD1
        cpx     #SCBONUS
        beq     SKD1
        stx     FZSCUR
        clc
        rts
SKD2:   sec
        rts

; ---------------------------------------------------------------------------
; The two markers that have to answer the BUTTON, not the next poll.
;
; Everything else on this screen is composed when a reply lands, which is
; right: it is all made of things the server said. These two are made of
; things the player just did, and a hold marker that appears a second and a
; half after the button reads as a button that did not work. One blit each,
; straight into the cell.
;
; The die under the cursor needs no such help -- missile 0 is repositioned
; every frame -- and the die's own art changes the moment FZKEEP does.

; TPUTC -- character A into text cell X.
TPUTC:  ldy     #0
        jsr     FNBARG
        lda     #FB_TCELL
        jmp     FNBLIT

; TROLLM -- the '>' on the ROLL tile, which is where a turn starts.
TROLLM: lda     FZDCUR
        cmp     #NDICE
        bne     TRM1
        lda     #'>'
        bne     TRM2
TRM1:   lda     #' '
TRM2:   ldx     #RROLL*FNTCOL
        jmp     TPUTC

; THOLDM -- the two cells under the die the cursor is on. Die i sits over text
; columns 2i and 2i+1 -- which is what NUSIZ1's two copies buy -- so the
; marker under it is just those two.
THOLDM: lda     FZDCUR
        cmp     #NDICE
        bcs     THM9
        asl     a
        clc
        adc     #RHOLD*FNTCOL
        tax
        stx     FZIDX
        ldy     FZDCUR
        lda     BITTAB,y
        and     FZKEEP
        bne     THM1            ; set = it re-rolls = no marker
        lda     #'='
        bne     THM2
THM1:   lda     #' '
THM2:   pha
        ldx     FZIDX
        jsr     TPUTC
        pla
        ldx     FZIDX
        inx
        jmp     TPUTC
THM9:   rts

; TURNRS -- a fresh turn: the cursor back on ROLL, the card cursor parked.
TURNRS: lda     #0
        sta     FZUIMD
        lda     #NDICE
        sta     FZDCUR
        lda     #SCONES
        sta     FZSCUR
        rts

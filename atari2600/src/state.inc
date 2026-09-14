; state.inc -- reading the reply where it lies.
;
; NOTHING IS BUFFERED. The reply stays painted in the cartridge's window until
; the next transaction, so every name, prompt and score is read straight out of
; it; the only strings ever copied anywhere are the two that ride out in a URL,
; and those go into the cartridge's own path buffers rather than console RAM.
;
; A record is 42 bytes at GOPLYRS + PLSTRID*i, so seat 8's last byte is at
; offset 472 -- past what a single-byte X index can reach. Everything here
; therefore goes through the FNPTRL/H pointer, and PLBASE is a table rather
; than repeated addition because a 16-bit multiply by 42 costs more bytes than
; the eighteen the table does.
;
; THE WINDOW IS ONE SLICE AND FR_RXSL IS NEVER WRITTEN. See PCREND in
; fzdefs.inc for why nine seats is the cap and why that is enough.

; Bit N of a byte, by N. UNGATED, because url.inc wants it to expand the roll
; mask and SCSCAN wants it to build the filled-slot shadow, and those two live
; in different banks. Eight bytes is cheaper than a second copy.
BITTAB: DB      1,2,4,8,16,32,64,128

; ---------------------------------------------------------------------------
; GHDR -- cache the header fields the whole program reads every frame.
;
; UNGATED, because the network bank caches them the moment a reply lands and
; the game bank re-derives the class when the difficulty switch moves, and
; those are different banks.
GHDR:   lda     FNRPLY+GOPC
        sta     FZPCNT
        lda     FNRPLY+GORND
        sta     FZRND
        lda     FNRPLY+GOACTPL
        sta     FZACT
        lda     FNRPLY+GORLEFT
        sta     FZRLEFT
; The move clock is RESEEDED from the wire every poll and counted down
; locally in between, so it self-corrects rather than drifting.
        lda     FNRPLY+GOMVTIM
        sta     FZCLK
        lda     #60
        sta     FZTICK
; Your turn: the server rotates you to index 0, so "activePlayer is 0" is the
; whole test -- unless you are a spectator, who has no turn to take.
        lda     #0
        sta     FZMYTRN
        lda     FNRPLY+GOVIEW
        bne     GHDR9
        lda     FZACT
        bne     GHDR9
        lda     #1
        sta     FZMYTRN
GHDR9:  rts

; GSETCLS -- the screen class, from the round and the difficulty switch.
;
; The standings are a CLASS and not an overlay: twenty-one rows cannot hold a
; scorecard and a seat list at once, which is why the switch exists at all.
GSETCLS: lda   FZDIFF
        beq     GSC1
        lda     #CLSTAND
        sta     FZCLS
        rts
GSC1:   lda     FZRND
        bne     GSC2
        lda     #CLLOBBY
        sta     FZCLS
        rts
GSC2:   cmp     #RNOVER
        bne     GSC3
        lda     #CLOVER
        sta     FZCLS
        rts
GSC3:   lda     #CLPLAY
        sta     FZCLS
        rts

        IF      FZHASUI|FZHASSCR
; ---------------------------------------------------------------------------
; PLRECP -- point FNPTRL/H at seat A's record. Seats past PCREND are clamped,
; because a caller walking playerCount must not index off the end of the table
; and the far seats are not rendered anyway.
PLRECP: cmp     #PCREND
        bcc     PLRP1
        lda     #PCREND-1
PLRP1:  tax
        lda     PLBASEL,x
        sta     FNPTRL
        lda     PLBASEH,x
        sta     FNPTRH
        rts

PLBASEL: DB     (FNRPLY+GOPLYRS+0*PLSTRID)&$FF
        DB      (FNRPLY+GOPLYRS+1*PLSTRID)&$FF
        DB      (FNRPLY+GOPLYRS+2*PLSTRID)&$FF
        DB      (FNRPLY+GOPLYRS+3*PLSTRID)&$FF
        DB      (FNRPLY+GOPLYRS+4*PLSTRID)&$FF
        DB      (FNRPLY+GOPLYRS+5*PLSTRID)&$FF
        DB      (FNRPLY+GOPLYRS+6*PLSTRID)&$FF
        DB      (FNRPLY+GOPLYRS+7*PLSTRID)&$FF
        DB      (FNRPLY+GOPLYRS+8*PLSTRID)&$FF
PLBASEH: DB     (FNRPLY+GOPLYRS+0*PLSTRID)>>8
        DB      (FNRPLY+GOPLYRS+1*PLSTRID)>>8
        DB      (FNRPLY+GOPLYRS+2*PLSTRID)>>8
        DB      (FNRPLY+GOPLYRS+3*PLSTRID)>>8
        DB      (FNRPLY+GOPLYRS+4*PLSTRID)>>8
        DB      (FNRPLY+GOPLYRS+5*PLSTRID)>>8
        DB      (FNRPLY+GOPLYRS+6*PLSTRID)>>8
        DB      (FNRPLY+GOPLYRS+7*PLSTRID)>>8
        DB      (FNRPLY+GOPLYRS+8*PLSTRID)>>8

        ENDIF

        IF      FZHASSCR
; RDSCOR -- the pointed seat's score for slot X, into FZNUM0/FZNUM1.
; Carry SET if the slot is unset or the seat is a spectator: $FFFF and $FFFE
; are the only two non-scores and BOTH carry $FF in the high byte, so one
; comparison answers it.
RDSCOR: txa
        asl     a
        clc
        adc     #PLSCORS
        tay
        lda     (FNPTRL),y
        sta     FZNUM0
        iny
        lda     (FNPTRL),y
        sta     FZNUM1
        cmp     #$FF
        beq     RDSC1
        clc
        rts
RDSC1:  sec
        rts

; RUNTOT -- the pointed seat's live running total into FZNUM0/FZNUM1.
;
; Slots 6 to 14, NOT slot 15: the grand total is filled only at game over, so
; summing it would read $FFFF for the whole of play. Slot 6 is the upper total
; and slot 7 the bonus, both server-computed, so the upper six are already in
; the sum and adding them again would double them.
; The accumulator is FZIDX/FZTMP and NOT FZTMP/FZTMP+1: FZTMP is $85 and
; FZTMP+1 is FNDEV, the transport's device byte. Nothing here runs inside a
; transaction, so it would have worked -- and it would have been a cell whose
; two owners had no reason at all to expect each other.
RUNTOT: lda     #0
        sta     FZIDX
        sta     FZTMP
        ldx     #SCUPTOT
RTOT1:  txa
        pha
        jsr     RDSCOR
        bcs     RTOT2           ; unset: contributes nothing
        lda     FZIDX
        clc
        adc     FZNUM0
        sta     FZIDX
        lda     FZTMP
        adc     FZNUM1
        sta     FZTMP
RTOT2:  pla
        tax
        inx
        cpx     #SCFUJI+1
        bne     RTOT1
        lda     FZIDX
        sta     FZNUM0
        lda     FZTMP
        sta     FZNUM1
        rts

; VALIDC -- validScores[X] into A, and Z set if that row is CLOSED.
;
; IT IS ONLY MEANINGFUL IN ROUNDS 1-13. The server sends fifteen zeros in the
; lobby, and zero is a perfectly legal potential, so an unguarded read paints
; the whole card green on the ready screen. Callers test FZRND first.
VALIDC: lda     FNRPLY+GOVALID,x
        cmp     #$FF
        rts

        ENDIF

        IF      FZHASUI
; ---------------------------------------------------------------------------
; SCSCAN -- which rows has seat A filled, and which one did they just take?
;
; The wire has no "last move" field: every endpoint returns the same blob and
; nothing in it says which row a seat has just chosen. The only way to know is
; to compare their filled rows against what they were last poll -- and a full
; shadow of sixteen 16-bit scores is 32 bytes, which this machine has not got.
; So the shadow is ONE BIT PER SLOT and costs two bytes.
;
; Leaves the new mask in FZNUM0 (slots 0-7) and FZNUM1 (slots 8-15), and puts
; in FZREVSL the LOWEST slot that is filled now and was not in FZSCM0/FZSCM1 --
; $FF when none.
;
; SLOTS 6, 7 AND 15 CAN NEVER BE THE ANSWER. The upper total, the bonus and the
; grand total fill as a SIDE EFFECT of the row actually chosen, and slot 6 in
; particular fills the moment ANY upper row is taken -- so without excluding
; them it would win every comparison, being the lowest.
SCSCAN: jsr     PLRECP
        ldx     #0              ; built in FZIDX/FZTMP; FZNUM0/1 take the
                                ;   answer at the end
        stx     FZIDX
        stx     FZTMP
SCS1:   txa
        asl     a
        clc
        adc     #PLSCORS+1      ; the HIGH byte alone decides: $FFFF (unset)
        tay                     ;   and $FFFE (spectator) both carry $FF there
        lda     (FNPTRL),y
        cmp     #$FF
        beq     SCS2            ; not filled
        txa
        cmp     #8
        bcs     SCS3            ; slots 8-15 go in the high half
        tay
        lda     BITTAB,y
        ora     FZIDX
        sta     FZIDX
        jmp     SCS2
SCS3:   sec
        sbc     #8
        tay
        lda     BITTAB,y
        ora     FZTMP
        sta     FZTMP
SCS2:   inx
        cpx     #NSLOTS
        bne     SCS1

; The three server-computed slots are struck out of BOTH masks before anything
; is compared, so they can neither be reported nor remembered.
        lda     FZIDX
        and     #$3F            ; drop slots 6 and 7
        sta     FZNUM0
        lda     FZTMP
        and     #$7F            ; drop slot 15
        sta     FZNUM1

; new AND NOT old, low half first, so the lowest slot wins.
        lda     #$FF
        sta     FZREVSL
        lda     FZSCM0
        eor     #$FF
        and     FZNUM0
        beq     SCS5
        ldx     #0
SCS4:   lsr     a
        bcs     SCS6
        inx
        bne     SCS4
SCS5:   lda     FZSCM1
        eor     #$FF
        and     FZNUM1
        beq     SCS7
        ldx     #8
SCS8:   lsr     a
        bcs     SCS6
        inx
        bne     SCS8
SCS6:   stx     FZREVSL
SCS7:   rts

; SHADST -- stage the shadow from the mask SCSCAN just built.
SHADST: lda     FZNUM0
        sta     FZSCM0
        lda     FZNUM1
        sta     FZSCM1
        rts

; SHADALL -- park the shadow with EVERY bit set.
;
; After a full clear -- a class change, or a seat count change where the
; shadow would belong to a different player -- no bit can have turned on, so
; nothing can be revealed from a comparison that was never valid.
SHADALL: lda    #$FF
        sta     FZSCM0
        sta     FZSCM1
        rts

        ENDIF

        IF      FZHASPICK
; PICKBEST -- the highest-paying OPEN category, into FZSCUR.
;
; Run when rollsLeft reaches zero, so the cursor is already on the best legal
; move and trigger-trigger-trigger-trigger plays a legal, if artless, game.
; Only the thirteen selectable slots are considered: 0-5 and 8-14.
PICKBEST: ldx   #0
        ldy     #$FF            ; best slot so far
        sty     FZTMP
        lda     #0
        sta     FZIDX           ; best points so far
PB1:    cpx     #SCUPTOT
        bcc     PB2
        cpx     #SCSET3
        bcs     PB2
        jmp     PB4             ; slots 6 and 7 are not selectable
PB2:    cpx     #SCGRAND
        beq     PB4
        lda     FNRPLY+GOVALID,x
        cmp     #$FF
        beq     PB4             ; closed
        cmp     FZIDX
        bcc     PB4
        sta     FZIDX
        stx     FZTMP
PB4:    inx
        cpx     #NSLOTS-1
        bne     PB1
        lda     FZTMP
        bmi     PB5             ; nothing open at all: leave the cursor alone
        sta     FZSCUR
PB5:    rts
        ENDIF

        IF      FZHASNET
; ---------------------------------------------------------------------------
; VALID8 -- is the reply long enough to be believed?
;
; MINIMUM, NOT EXACT: a longer reply is harmless, but a SHORTER one means
; every fixed offset below would be reading whatever the previous, bigger
; reply left behind -- and the window is not cleared between transactions, so
; that is real data from a real game and looks entirely plausible.
;
; Returns 0 if the reply may be used, FNEBADL if not.
VALID8: lda     FZREQ
        cmp     #RQTABLE
        bne     V8GAME
        lda     FZRXL1          ; a Tables: any nonzero length, and a count
        ora     FZRXL0
        beq     V8BAD
        lda     #0              ; and RETURN ZERO, not the length that proved
        rts                     ;   it: every caller reads this as an error
                                ;   code, and a 145-byte listing came back as
                                ;   error $91
V8GAME: lda     FZRXL1          ; at least the fixed header
        bne     V8G1
        lda     FZRXL0
        cmp     #GMINLEN
        bcc     V8BAD
V8G1:   lda     FNRPLY+GOPC
        beq     V8BAD           ; a table with nobody at it is not a state
        cmp     #PCMAX+1
        bcs     V8BAD
        tax
        lda     FNRPLY+GORND    ; 0-13, or 99
        cmp     #RNFINAL+1
        bcc     V8G2
        cmp     #RNOVER
        bne     V8BAD
V8G2:   txa                     ; ...and at least 95 + 42*playerCount
        asl     a
        tax
        lda     FZRXL1
        cmp     EXPGAM+1,x
        bcc     V8BAD
        bne     V8OK
        lda     FZRXL0
        cmp     EXPGAM+0,x
        bcc     V8BAD
V8OK:   lda     #0
        rts
V8BAD:  lda     #FNEBADL
        rts

; The thirteen minimum lengths, GMINLEN + PLSTRID*pc for pc = 0..12, low byte
; first because that is the order the compare above wants them in.
EXPGAM: DB      (GMINLEN+0*PLSTRID)&$FF,(GMINLEN+0*PLSTRID)>>8
        DB      (GMINLEN+1*PLSTRID)&$FF,(GMINLEN+1*PLSTRID)>>8
        DB      (GMINLEN+2*PLSTRID)&$FF,(GMINLEN+2*PLSTRID)>>8
        DB      (GMINLEN+3*PLSTRID)&$FF,(GMINLEN+3*PLSTRID)>>8
        DB      (GMINLEN+4*PLSTRID)&$FF,(GMINLEN+4*PLSTRID)>>8
        DB      (GMINLEN+5*PLSTRID)&$FF,(GMINLEN+5*PLSTRID)>>8
        DB      (GMINLEN+6*PLSTRID)&$FF,(GMINLEN+6*PLSTRID)>>8
        DB      (GMINLEN+7*PLSTRID)&$FF,(GMINLEN+7*PLSTRID)>>8
        DB      (GMINLEN+8*PLSTRID)&$FF,(GMINLEN+8*PLSTRID)>>8
        DB      (GMINLEN+9*PLSTRID)&$FF,(GMINLEN+9*PLSTRID)>>8
        DB      (GMINLEN+10*PLSTRID)&$FF,(GMINLEN+10*PLSTRID)>>8
        DB      (GMINLEN+11*PLSTRID)&$FF,(GMINLEN+11*PLSTRID)>>8
        DB      (GMINLEN+12*PLSTRID)&$FF,(GMINLEN+12*PLSTRID)>>8
        ENDIF

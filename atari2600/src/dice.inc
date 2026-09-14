; dice.inc -- the tray: what the five dice are, and what is being done to them.
;
; A die's whole state is the LOW BYTE of its kernel pointer -- held*DIEVAR +
; face*DIEH -- so staging is the only place any of this is decided and the
; kernel has no per-die test in it at all.

; ---------------------------------------------------------------------------
; KEEPSYN -- restage FZKEEP from the wire's echo.
;
; THE SENSE IS THE TRAP THE WHOLE FAMILY WRITES DOWN: keepRoll's '1' means
; that die RE-ROLLS and '0' means it is held. FZKEEP carries the same sense,
; so a set bit is a die in flight.
;
; And it is an ECHO of the last roll that actually went out, not a live
; picture: it does not move until the next one does. So your own holds come
; from the button (FZKEEP is edited in place on your turn) and everybody
; else's come from here -- read the wire for your own and the tray lags a
; whole poll behind the press.
;
; An empty echo -- NULs, which is what a fresh turn carries -- means roll
; everything. That is also what makes a REJOIN work: your turn can arrive with
; the dice already on the table.
KEEPSYN: lda    #0
        sta     FZKEEP
        ldy     #0
KSY1:   lda     FNRPLY+GOKEEP,y
        beq     KSY3            ; NUL: nothing has been rolled yet
        cmp     #'1'
        bne     KSY2
        lda     BITTAB,y
        ora     FZKEEP
        sta     FZKEEP
KSY2:   iny
        cpy     #NDICE
        bne     KSY1
        rts
KSY3:   lda     #(1<<NDICE)-1   ; roll everything
        sta     FZKEEP
        rts

; ---------------------------------------------------------------------------
; DSTAGE -- the five kernel pointers, from the wire's dice and FZKEEP.
;
; The high byte is FZART's page. It is written HERE and not once at startup
; because a bank switch does not disturb zero page but a cold entry does, and
; this runs on every compose either way.
DSTAGE: ldy     #NDICE-1
DSTG1:  tya
        asl     a
        tax                     ; X = die*2, the pointer pair's offset
        lda     FNRPLY+GODICE,y
        sec
        sbc     #'1'            ; ascii '1'-'6' -> 0-5
        bcc     DSTG2           ; NUL at turn start: show a one
        cmp     #6
        bcc     DSTG3
DSTG2:  lda     #0
DSTG3:  jsr     DFACE
        lda     BITTAB,y
        and     FZKEEP
        bne     DSTG4           ; set = in flight = the ordinary art
        lda     FZIDX
        clc
        adc     #DIEVAR
        jmp     DSTG5
DSTG4:  lda     FZIDX
DSTG5:  sta     DPTR0,x
        lda     #FZART>>8
        sta     DPTR0+1,x
        dey
        bpl     DSTG1
        rts

; DFACE -- FZIDX = the LOW BYTE of face A's pointer: FZART + face*DIEH.
;
; A multiply by twelve, as a shift and an add, because this machine has no
; multiply and a table would cost more than this does.
;
; THE BASE IS ADDED HERE, ONCE. It was left out while the art happened to be
; page-aligned, so the offset and the low byte were the same number; moving
; the art off a page boundary turned every die into a pointer thirty bytes
; below it, and the tray came up empty. The held variant adds DIEVAR on top,
; and $50 + 60 + 72 is 224, so none of it can carry out of the page.
DFACE:  asl     a               ; f*2
        asl     a               ; f*4
        sta     FZIDX
        asl     a               ; f*8
        clc
        adc     FZIDX           ; f*12
        clc
        adc     #FZART&$FF
        sta     FZIDX
        rts

; ---------------------------------------------------------------------------
; ROLLANM -- arm the roll animation.
;
; Armed on the ROLL EDGE, which is a DECREASE in rollsLeft: a fresh turn
; RAISES it, and animating that would rattle dice that have not moved.
ROLLANM: lda   #ANMFRM
        sta     FZANM
        rts

; ANMTICK -- one frame of it, from the vblank hook.
;
; Arcadia's version is a blocking loop of eight flickers with a tone each. On
; this console there is no frame to block in: the display loop IS the program,
; so the animation is a countdown the hook steps and the kernel simply draws
; whatever the pointers currently say.
;
; ONLY THE DICE THE WIRE SAYS MOVED ARE SHAKEN, and the wire is asked directly
; rather than through FZKEEP -- keepRoll names exactly the dice this roll was
; made with, for everybody's rolls and not just ours, which is what makes an
; opponent's roll animate correctly.
ANMTICK: lda   FZANM
        beq     ANMS4
        dec     FZANM
        bne     ANMS0
        jmp     DSTAGE          ; the last frame: put the truth back
ANMS0:  lda     FZANM
        ; every ANMSTEP frames, not every frame: eight flickers, not
        ; twenty-four, or the dice are a grey blur
ANMS1:  sec
ANMS1A: sbc     #ANMSTEP
        bcs     ANMS1A
        adc     #ANMSTEP
        bne     ANMS4           ; not on a step boundary
        ldy     #NDICE-1
ANMS2:  lda     FNRPLY+GOKEEP,y
        cmp     #'1'
        beq     ANMS3
        cmp     #0              ; a fresh turn: everything is in flight
        bne     ANMS5
ANMS3:  tya
        asl     a
        tax
        jsr     DRAND
        and     #7
        cmp     #6
        bcc     ANMS6
        and     #3
ANMS6:  jsr     DFACE
        lda     FZIDX           ; a die in flight is never drawn held
        sta     DPTR0,x
        lda     #FZART>>8
        sta     DPTR0+1,x
ANMS5:  dey
        bpl     ANMS2
ANMS4:  rts

; DRAND -- a 16-bit LFSR. Seeded from the frame counter at the first roll, so
; two consoles watching the same table do not rattle in step.
DRAND:  lda     FZRSD1
        lsr     a
        ror     FZRSD0
        bcc     DRND1
        lda     FZRSD1
        eor     #$B4
        sta     FZRSD1
        lda     FZRSD0
        eor     #$01
        sta     FZRSD0
DRND1:  lda     FZRSD0
        ora     FZRSD1
        bne     DRND2
        lda     #$A5            ; never let it reach zero and stop
        sta     FZRSD0
DRND2:  lda     FZRSD0
        rts

        IF      FZHASHUD
; ---------------------------------------------------------------------------
; DICEHUD -- the two rows under the tray: the hold markers and the ROLL tile.
;
; The markers are what carry the held state where colour cannot: in a
; 48-pixel six-copy span COLUP0 paints dice 0, 2 and 4 and COLUP1 dice 1 and
; 3, so there is no per-die colour on this machine at all. Die i sits over
; text columns 2i and 2i+1 -- that is what NUSIZ1's two copies buy -- so a
; marker under it is just those two cells.
DICEHUD: lda   #RHOLD
        jsr     FNROWA
        ldy     #0
DHUD1:  lda     BITTAB,y
        and     FZKEEP
        bne     DHUD2           ; in flight: no marker
        lda     #'='
        jsr     FNCHR
        lda     #'='
        jsr     FNCHR
        jmp     DHUD3
DHUD2:  lda     #2
        jsr     FNSPC
DHUD3:  iny
        cpy     #NDICE
        bne     DHUD1
        lda     #FNTCOL-2*NDICE
        jsr     FNSPC
        jsr     FNENDW

        lda     #RROLL
        jsr     FNROWA
        lda     FZUIMD          ; the cursor marker: the ROLL tile is where a
        bne     DHUD4           ;   turn starts, because rolling is always the
        lda     FZDCUR          ;   first move
        cmp     #NDICE
        bne     DHUD4
        lda     FZMYTRN
        beq     DHUD4
        lda     #'>'
        bne     DHUD5
DHUD4:  lda     #' '
DHUD5:  jsr     FNCHR
        ldx     #UROLLT&$FF
        ldy     #UROLLT>>8
        jsr     FNSTRP
        lda     #5
        jsr     FNSPC
        lda     #'X'
        jsr     FNCHR
        lda     FZRLEFT
        cmp     #10
        bcc     DHUD6
        lda     #9
DHUD6:  clc
        adc     #'0'
        jsr     FNCHR
        jmp     FNENDW

UROLLT: DB      "ROLL",0
        ENDIF

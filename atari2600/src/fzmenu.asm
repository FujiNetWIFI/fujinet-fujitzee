; fzmenu.asm -- bank 3: the SELECT menu, and the help behind it.
;
; SELECT is a SWITCH on this console -- a bit in a RIOT register the program
; reads -- and what it means is the client's to choose. Here it opens the
; menu, because RESET is worth more as "poll now" than as anything else: a
; player who thinks the game has stalled wants the screen to catch up, and
; that is one switch away.
;
; LEAVE sends /leave and then a fresh /tables, so the seat is GIVEN UP rather
; than abandoned. An abandoned seat is held by the server until its clock runs
; out, and everyone else at the table waits for it.

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "fzdefs.inc"
        INCLUDE "../build/tail.inc"
FZBANK  EQU     BANKMNU
FZHASDIC EQU    0
FZHASINP EQU    1               ; unused now: disp.inc scans unconditionally
FZHASUI EQU     0
FZHASSCR EQU    0
FZHASPICK EQU   0
FZHASCLS EQU    1
FZHASSTR EQU    1
FZHASRPL EQU    0
FZHASED EQU     0
FZHASDEC EQU    0
FZHASNET EQU    0
FZHASHUD EQU    0
SNDLAST EQU     SN_SEL

NITEMS  EQU     3
RITEM0  EQU     6
RHELP0  EQU     4

        ORG     $1000

UENTRY: lda     #0
        sta     FZSEL
        sta     FZCMOD          ; 0 = the menu, 1 = the help pages
        jsr     DINIT
        jsr     FNCLS
        jsr     UDRAW
        jmp     DFRESH

UDRAW:  lda     FZCMOD
        bne     UHELP
        lda     #RSTAT
        jsr     FNROWA
        ldx     #SMENUT&$FF
        ldy     #SMENUT>>8
        jsr     FNSTRP
        jsr     FNENDW
        lda     #0
        sta     FZCIX
UDR1:   lda     FZCIX
        asl     a
        clc
        adc     #RITEM0
        jsr     FNROWA
        lda     FZCIX
        cmp     FZSEL
        bne     UDR2
        lda     #'>'
        bne     UDR3
UDR2:   lda     #' '
UDR3:   jsr     FNCHR
        lda     FZCIX
        asl     a
        tax
        lda     UITEMS+0,x
        sta     FNPTRL
        lda     UITEMS+1,x
        sta     FNPTRH
        jsr     FNSTRA
        jsr     FNENDW
        inc     FZCIX
        lda     FZCIX
        cmp     #NITEMS
        bne     UDR1
        rts

; The help. Two screens would need a second page of text this bank has room
; for but the screen has not: nine rows of twelve is 108 characters, and the
; rules of a dice game that the scorecard does not already show fit in that.
UHELP:  lda     #RSTAT
        jsr     FNROWA
        ldx     #SHELPT&$FF
        ldy     #SHELPT>>8
        jsr     FNSTRP
        jsr     FNENDW
        lda     #0
        sta     FZCIX
UHL1:   lda     FZCIX
        clc
        adc     #RHELP0
        jsr     FNROWA
        lda     FZCIX
        asl     a
        tax
        lda     UHELPL+0,x
        sta     FNPTRL
        lda     UHELPL+1,x
        sta     FNPTRH
        jsr     FNSTRA
        jsr     FNENDW
        inc     FZCIX
        lda     FZCIX
        cmp     #NHELPL
        bne     UHL1
        rts

; ---------------------------------------------------------------------------
APPVBL: jsr     SNDSTEP
; Drain the latch DFRAME filled in. Not a scan: the scan already happened
; this frame, in this bank or in whichever one was mapped when the button
; actually went down.
        lda     FZILAT
        ldx     #0
        stx     FZILAT
        sta     FZTMP
        lda     FZCMOD
        beq     UA0
; In the help, any press goes back to the menu.
        lda     FZTMP
        beq     UAR
        lda     #0
        sta     FZCMOD
        jmp     UARD
UA0:    lda     FZTMP
        and     #IN_UP
        beq     UA1
        lda     FZSEL
        beq     UAR
        dec     FZSEL
        jmp     UARD
UA1:    lda     FZTMP
        and     #IN_DOWN
        beq     UA2
        lda     FZSEL
        cmp     #NITEMS-1
        bcs     UAR
        inc     FZSEL
        jmp     UARD
UA2:    lda     FZTMP
        and     #IN_SEL         ; SELECT again closes it, unchanged
        bne     UARES
        lda     FZTMP
        and     #IN_FIRE
        beq     UAR
        lda     FZSEL
        beq     UARES           ; RESUME
        cmp     #1
        beq     UAHELP          ; HOW TO PLAY
        jmp     ULEAVE          ; LEAVE
UAR:    rts
UARD:   lda     #SN_CLICK
        jsr     SNDCUE
        jmp     UDRAW

UAHELP: lda     #1
        sta     FZCMOD
        jsr     FNCLS
        jmp     UARD

; Resuming is a full recompose, because the menu has overwritten every row
; the game was using -- the text planes survive a bank switch, so a screen
; that simply switched back would show the menu under the card.
UARES:  lda     #SN_SEL
        jsr     SNDCUE
        lda     #$FF            ; force the composer's full-clear path
        sta     FZPRVCL
        lda     #ENEDGE
        sta     FZENT
        lda     #BANKEDG
        jmp     FZGOTO

ULEAVE: lda     #SN_SEL
        jsr     SNDCUE
        lda     #ENLEAVE
        sta     FZENT
        lda     #BANKNET
        jmp     FZGOTO

SMENUT: DB      "MENU",0
SHELPT: DB      "HOW TO PLAY",0
UITEMS: DW      SRESUME,SHELPI,SLEAVE
SRESUME: DB     "RESUME",0
SHELPI: DB      "HOW TO PLAY",0
SLEAVE: DB      "LEAVE TABLE",0

NHELPL  EQU     9
UHELPL: DW      SH0,SH1,SH2,SH3,SH4,SH5,SH6,SH7,SH8
SH0:    DB      "ROLL UP TO",0
SH1:    DB      "3 TIMES.",0
SH2:    DB      "FIRE HOLDS",0
SH3:    DB      "A DIE.",0
SH4:    DB      "UP GOES TO",0
SH5:    DB      "THE CARD.",0
SH6:    DB      "GREEN=WHAT",0
SH7:    DB      "IT SCORES.",0
SH8:    DB      "L.DIFF=STBY",0

        INCLUDE "fzlib.inc"
        INCLUDE "sound.inc"
        INCLUDE "disp.inc"

        IF      * > FZART
        ERROR   "fzmenu: code overflows into the reserved top page"
        ENDIF

        ORG     FZBGTA
FZBGT:  DB      CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG
        DB      CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG

        END

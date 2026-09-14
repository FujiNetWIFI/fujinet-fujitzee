; fzlobby.asm -- bank 0: the cold start, and the table list.

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "fzdefs.inc"
        INCLUDE "../build/tail.inc"
FZBANK  EQU     BANKLOB
FZHASDIC EQU    0               ; there are no dice in the lobby
FZHASINP EQU    1               ; unused now: disp.inc scans unconditionally
FZHASUI EQU     0
FZHASSCR EQU    0
FZHASPICK EQU   0
FZHASCLS EQU    1
FZHASSTR EQU    1
FZHASRPL EQU    1
FZHASED EQU     1               ; the table id goes into a path buffer
FZHASDEC EQU    0
FZHASNET EQU    0
FZHASHUD EQU    0
SNDLAST EQU     SN_SEL

; THE TABLE LIST IS CAPPED AT SEVEN. FNWRPL and FNRPLA index the reply window
; with a single byte, and table 7's record starts at offset 1 + 7*36 = 253, so
; the eighth is the first one whose id could not be copied out. Seven rows on
; a twelve-column screen is not the constraint anyone would have guessed, but
; it is the real one.
NTABLE  EQU     7
RTAB0   EQU     3

        ORG     $1000

LENTRY: lda     FZENT
        cmp     #ENLOBBY
        beq     LLIST

; ---- ENCOLD: power-on, and the RESET switch ----
;
; The cold stub in the fixed tail has already armed the control page and
; forced this entry; everything that needs more than a handful of bytes is
; here.
LCOLD:  sei
        cld
        ldx     #$FF
        txs
        lda     #0
        tax
LCLR:   sta     $80,x           ; the 128 bytes of RAM, every one of them
        inx
        bpl     LCLR
        lda     #2
        sta     VBLANK

        jsr     FNCHK
        beq     LGOT
; No cartridge answered. There is nothing this program can do and no way to
; say so in text, because the text is composed by the cartridge that is not
; there -- so it says it in colour.
        lda     #$34
        sta     COLUBK
LHALT:  jmp     LHALT

; THE FOUR PATH BUFFERS SURVIVE A CONSOLE RESET -- there is no reset line on
; this connector at all -- so a cold start must empty every one of them, or
; the first URL carries a table this program never joined.
LGOT:   ldx     #0
LPB1:   txa
        clc
        adc     #FP_SEL0
        jsr     FNWSEL
        jsr     FNWRST
        inx
        cpx     #4
        bne     LPB1

        jsr     FNCLS
        lda     #ENNAME
        sta     FZENT
        lda     #BANKNAM
        jmp     FZGOTO

; ---- ENLOBBY: the listing is in the window ----
LLIST:  lda     #0
        sta     FZSEL
        jsr     DINIT
; COUNT BEFORE DRAWING. The count was worked out in the hook and the first
; draw happened before the first hook, so the listing came up with a title, a
; hint, and no tables at all.
        jsr     LCOUNT
        jsr     LDRAW
        jmp     DFRESH

; LCOUNT -- how many tables there are, from the listing's own count byte. A
; reply that was only length-checked would list whatever the last one left.
LCOUNT: lda     FNRPLY
        cmp     #NTABLE+1
        bcc     LCT1
        lda     #NTABLE
LCT1:   sta     FZCNT
        rts

; LDRAW -- the whole screen. Cheap enough to do in one go: nothing here reads
; a player record, and the rows are short.
LDRAW:  lda     #RSTAT
        jsr     FNROWA
        ldx     #STITLE&$FF
        ldy     #STITLE>>8
        jsr     FNSTRP
        jsr     FNENDW
        lda     #0
        sta     FZCIX
LDR1:   lda     FZCIX
        cmp     FZCNT
        bcs     LDR9
        clc
        adc     #RTAB0
        jsr     FNROWA
        lda     FZCIX
        cmp     FZSEL
        bne     LDR2
        lda     #'>'
        bne     LDR3
LDR2:   lda     #' '
LDR3:   jsr     FNCHR
; name[21] at TBNAME, seven columns of it; then the seat count, which the wire
; sends as the literal "c / m" and which is shown here as "c/m" -- three
; columns is what is left, and the two spaces in it carry nothing.
        jsr     LDROFF
        clc
        adc     #TBNAME
        tax
        ldy     #7
        jsr     FNRPLA
        lda     #1
        jsr     FNSPC
        jsr     LDROFF
        clc
        adc     #TBPLYRS
        tax
        lda     FNRPLY,x
        jsr     FNCHR
        lda     #'/'
        jsr     FNCHR
        lda     FNRPLY+4,x
        jsr     FNCHR
        jsr     FNENDW
        inc     FZCIX
        jmp     LDR1

LDR9:   ldx     #SHINTL&$FF
        ldy     #SHINTL>>8
        jmp     HINTL

; LDROFF -- A = the reply offset of table FZCIX's record.
LDROFF: lda     #0
        ldx     FZCIX
        beq     LDRO2
LDRO1:  clc
        adc     #TBSTRID
        dex
        bne     LDRO1
LDRO2:  clc
        adc     #1              ; past the count byte
        rts

HINTL:  lda     #RHINTL
        jsr     FNROWA
        jsr     FNSTRP
        jmp     FNENDW

; ---------------------------------------------------------------------------
APPVBL: jsr     SNDSTEP
; Drain the latch DFRAME filled in. Not a scan: the scan already happened
; this frame, in this bank or in whichever one was mapped when the button
; actually went down.
        lda     FZILAT
        ldx     #0
        stx     FZILAT
        sta     FZTMP

        jsr     LCOUNT
        bne     LAV1
        rts                     ; an empty server: nothing to choose

LAV1:   lda     FZTMP
        and     #IN_UP
        beq     LAV2
        lda     FZSEL
        beq     LAVR
        dec     FZSEL
        jmp     LAVRD
LAV2:   lda     FZTMP
        and     #IN_DOWN
        beq     LAV3
        lda     FZSEL
        clc
        adc     #1
        cmp     FZCNT
        bcs     LAVR
        sta     FZSEL
        jmp     LAVRD
LAV3:   lda     FZTMP
        and     #IN_FIRE
        beq     LAV4
        jmp     LJOIN
LAV4:   lda     FZTMP
        and     #IN_RST
        beq     LAVR
        lda     #ENTABLE        ; RESET is a switch here: list them again
        sta     FZENT
        lda     #BANKNET
        jmp     FZGOTO
LAVR:   rts
LAVRD:  lda     #SN_CLICK
        jsr     SNDCUE
        jmp     LDRAW

; LJOIN -- take the highlighted table's id into the path buffer, and fetch.
; The id never enters console RAM: it goes from the reply window straight into
; the cartridge, and from there into every URL.
LJOIN:  lda     FZSEL
        sta     FZCIX
        lda     #PBTABLE
        jsr     FNWSEL
        jsr     FNWRST
        jsr     LDROFF
        clc
        adc     #TBID
        tax
        ldy     #9
        jsr     FNWRPL
        lda     #SN_SEL
        jsr     SNDCUE
        lda     #$FF            ; a joined table is a new game: mute every
        sta     FZPRVAC         ;   edge until the first reply has landed
        sta     FZPRVRL
        sta     FZPRVPC
        sta     FZPRVCL
        lda     #0
        sta     FZREQ2
        lda     #ENFETCH
        sta     FZENT
        lda     #BANKNET
        jmp     FZGOTO

STITLE: DB      "PICK A TABLE",0
SHINTL: DB      "FIRE=JOIN",0
RHINTL  EQU     12

        INCLUDE "fzlib.inc"
        INCLUDE "sound.inc"
        INCLUDE "disp.inc"

        IF      * > FZART
        ERROR   "fzlobby: code overflows into the reserved top page"
        ENDIF

        ORG     FZBGTA
FZBGT:  DB      CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG
        DB      CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG

        END

; fzname.asm -- bank 4: the shared username, and the keyboard that types one.
;
; THE NAME NEVER LANDS IN CONSOLE RAM. It goes from the appkey reply straight
; into a cartridge path buffer, and from there into every URL; the keyboard
; edits the buffer in place and the blit port shows it back. 128 bytes of RAM,
; of which the stack owns the top, cannot spare nine for a string the
; cartridge is already holding.
;
; The slot is the FujiNet lobby's own -- creator 1, app 1, key 0 -- which is
; the same one every game in the family reads. That is the whole point of it:
; type your name once, anywhere, and every client knows it.

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "fzdefs.inc"
        INCLUDE "../build/tail.inc"
FZBANK  EQU     BANKNAM
FZHASDIC EQU    0
FZHASINP EQU    1               ; unused now: disp.inc scans unconditionally
FZHASUI EQU     0
FZHASSCR EQU    0
FZHASPICK EQU   0
FZHASCLS EQU    1
FZHASSTR EQU    1
FZHASRPL EQU    0
FZHASED EQU     1
FZHASDEC EQU    0
FZHASNET EQU    0
FZHASHUD EQU    0
SNDLAST EQU     SN_SEL

KBROWS  EQU     3
KBCOLS  EQU     12
RKB0    EQU     10              ; the three keyboard rows
RNAMED  EQU     6               ; where what has been typed is shown

        ORG     $1000

MENTRY: jsr     AKREAD
        bne     MEDIT
        lda     #ENTABLE        ; a name was already there: straight on
        sta     FZENT
        lda     #BANKNET
        jmp     FZGOTO

; ---- the keyboard ----
MEDIT:  lda     #PBNAME
        jsr     FNWSEL
        jsr     FNWRST
        lda     #0
        sta     FZNLEN
        sta     FZSEL
        jsr     DINIT
        jsr     FNCLS
        jsr     MDRAW
        jmp     DFRESH

MDRAW:  lda     #RSTAT
        jsr     FNROWA
        ldx     #SNAMET&$FF
        ldy     #SNAMET>>8
        jsr     FNSTRP
        jsr     FNENDW
; What has been typed, straight out of the cartridge's buffer: FB_PATH copies
; cnt characters of it into a text row without any of them crossing the bus.
        lda     #0
        ldx     #RNAMED
        ldy     #NAMEMAX
        jsr     FNBARG
        lda     #FB_PATH
        jsr     FNBLIT
; The three rows of keys, with the cursor shown as an inverted-looking bracket
; because there is no inverse cell on this screen: the row's ink is one colour
; and the marker has to be a character.
        lda     #0
        sta     FZCIX
MDR1:   lda     FZCIX
        clc
        adc     #RKB0
        jsr     FNROWA
        lda     FZCIX
        asl     a
        asl     a
        asl     a
        sta     FZTMP           ; row*8
        lda     FZCIX
        asl     a
        asl     a
        clc
        adc     FZTMP           ; row*12
        sta     FZTMP
        ldy     #0
MDR2:   tya
        clc
        adc     FZTMP
        cmp     FZSEL
        bne     MDR3
        lda     #'['            ; the cursor sits ON the key it would take
        jsr     FNCHR
        jmp     MDR4
MDR3:   tya
        clc
        adc     FZTMP
        tax
        lda     KBKEYS,x
        jsr     FNCHR
MDR4:   iny
        cpy     #KBCOLS
        bne     MDR2
        jsr     FNENDW
        inc     FZCIX
        lda     FZCIX
        cmp     #KBROWS
        bne     MDR1
        lda     #RHINTN
        jsr     FNROWA
        ldx     #SNAMEH&$FF
        ldy     #SNAMEH>>8
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
        sta     FZCMOD
        and     #IN_LEFT
        beq     MA1
        lda     FZSEL
        beq     MARX
        dec     FZSEL
        jmp     MARD
; A local rts: the shared one at the bottom of the chain is out of branch
; range from up here, and a jmp per arm would cost more than this does.
MARX:   rts
MA1:    lda     FZCMOD
        and     #IN_RIGHT
        beq     MA2
        lda     FZSEL
        cmp     #KBROWS*KBCOLS-1
        bcs     MARX
        inc     FZSEL
        jmp     MARD
MA2:    lda     FZCMOD
        and     #IN_UP
        beq     MA3
        lda     FZSEL
        cmp     #KBCOLS
        bcc     MARX
        sec
        sbc     #KBCOLS
        sta     FZSEL
        jmp     MARD
MA3:    lda     FZCMOD
        and     #IN_DOWN
        beq     MA4
        lda     FZSEL
        clc
        adc     #KBCOLS
        cmp     #KBROWS*KBCOLS
        bcs     MAR
        sta     FZSEL
        jmp     MARD
; FIRE types, SELECT rubs out and RESET is done. There is no backspace KEY,
; because a fourth row would not fit beside the name and the thirty-six
; characters that do fit are all wanted.
MA4:    lda     FZCMOD
        and     #IN_FIRE
        beq     MA5
        lda     FZNLEN
        cmp     #NAMEMAX
        bcs     MAR
        ldx     FZSEL
        lda     KBKEYS,x
        sta     FNRSEL+FH_PATHC
        inc     FZNLEN
        lda     #SN_SEL
        jsr     SNDCUE
        jmp     MARD
MA5:    lda     FZCMOD
        and     #IN_SEL
        beq     MA6
        lda     FZNLEN
        beq     MAR
        lda     #PBNAME
        jsr     FNWSEL
        jsr     FNWPOP
        dec     FZNLEN
        jmp     MARD
MA6:    lda     FZCMOD
        and     #IN_RST
        beq     MAR
        lda     FZNLEN          ; done -- but not with an empty name, which
        beq     MAR             ;   the server would refuse
        jmp     MDONE
MAR:    rts
MARD:   lda     #SN_CLICK
        jsr     SNDCUE
        jmp     MDRAW

; MDONE -- keep the name, then go and list the tables.
MDONE:  jsr     AKWRITE
        lda     #ENTABLE
        sta     FZENT
        lda     #BANKNET
        jmp     FZGOTO

; ---------------------------------------------------------------------------
; AKOPEN -- OPEN_APPKEY with mode A. Returns 0 if the adapter took it.
;
; The payload is a SIX-BYTE PACKED STRUCT and not four parameters:
; AppKeyMixin::appkey_open() does one transaction_get() of sizeof(appkey),
; which is creator as a u16, then app, key, mode and a RESERVED byte. Leave
; that last one off and transaction_get() waits for a byte that never
; arrives -- it reads back as a timeout rather than as a protocol error, which
; is the trap the Intellivision port documents by name.
;
; It also fails outright when the adapter has no SD card, which is not a
; client bug and is why the keyboard is the fallback rather than an error.
AKOPEN: pha
        lda     #FNDEVF
        sta     FNDEV
        lda     #FCAKOPN
        sta     FNCMD
        lda     #0
        sta     FNNPR
        jsr     FNBEG
        lda     #(AKCREAT)&$FF
        sta     FNTX
        lda     #(AKCREAT)>>8
        sta     FNTX
        lda     #AKAPP
        sta     FNTX
        lda     #AKKEY
        sta     FNTX
        pla
        sta     FNTX            ; the mode
        lda     #0
        sta     FNTX            ; reserved, and not optional
        jsr     FNGO
        bne     AKOX
        jmp     FNACK
AKOX:   rts

; AKREAD -- the shared username into the name buffer. Z set if there was one.
AKREAD: lda     #AKMRD
        jsr     AKOPEN
        bne     AKBAD
        lda     #FNDEVF
        sta     FNDEV
        lda     #FCAKRD
        sta     FNCMD
        lda     #0
        sta     FNNPR
        jsr     FNBEG
        jsr     FNGO
        bne     AKBAD
        jsr     FNACK
        bne     AKBAD
; The reply is a length word and then the value.
        lda     FNRPLY+0
        ora     FNRPLY+1
        beq     AKBAD           ; the slot is empty: type one
        lda     #PBNAME
        jsr     FNWSEL
        jsr     FNWRST
        ldx     #2
        ldy     #NAMEMAX
        jsr     FNWRPL
        lda     #0
        rts
AKBAD:  lda     #$FF
        rts

; AKWRITE -- keep what was typed. The name is streamed out of the cartridge's
; own buffer, so it never crosses the bus in either direction.
AKWRITE: lda   #AKMWR
        jsr     AKOPEN
        bne     AKWX
        lda     #FNDEVF
        sta     FNDEV
        lda     #FCAKWR
        sta     FNCMD
        lda     #0
        sta     FNNPR
        jsr     FNBEG
        lda     FZNLEN
        sta     FNTX            ; the length word, little-endian
        lda     #0
        sta     FNTX
        lda     #PBNAME
        jsr     FNWSEL
        jsr     FNWRAW
        jsr     FNGO
AKWX:   rts

SNAMET: DB      "YOUR NAME",0
SNAMEH: DB      "RST=DONE",0
RHINTN  EQU     14

; Three rows of twelve: every letter and every digit, and nothing else.
KBKEYS: DB      "ABCDEFGHIJKL"
        DB      "MNOPQRSTUVWX"
        DB      "YZ0123456789"

        INCLUDE "fzlib.inc"
        INCLUDE "sound.inc"
        INCLUDE "disp.inc"

        IF      * > FZART
        ERROR   "fzname: code overflows into the reserved top page"
        ENDIF

        ORG     FZBGTA
FZBGT:  DB      CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG
        DB      CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG

        END

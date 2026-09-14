; layout.asm -- the whole scorecard, with no network at all.
;
; A flat 4K image whose TEXT PLANES ARE BAKED INTO THE ROM at $1800+: on a
; stock 2600 the image's own bytes are what the kernel reads there, so a full
; card, a tray of dice and a cursor are on screen before a socket has been
; opened. It exists because the kernel is the one component of this port with
; no template anywhere -- the per-row ink picked out of a potential mask, the
; dice band's five objects, the missile cursor and the 262-line frame all have
; to be right before any of them is worth debugging over a network.
;
; tools/mklayout.py writes the screen (build/layoutscr.inc) from a picture;
; emu/scrcheck.py reads a snapshot back against the same picture.
;
; The stick moves the cursor, FIRE holds and unholds the die under it -- which
; swaps the art and the marker row together -- and SELECT flips to the
; no-dice class, so one ROM exercises both shapes of the frame.
;
;   ./build.sh layout && ./run.sh layout

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "fzdefs.inc"
FZHASDIC EQU    1
FZHASCLS EQU    0
FZHASSTR EQU    0
FZHASRPL EQU    0
FZHASED EQU     0
FZHASDEC EQU    0

        ORG     $1000

START:  sei
        cld
        ldx     #$FF
        txs
        lda     #0
LCLR:   sta     $00,x           ; $00-$7F is the TIA, $80-$FF is RAM
        dex
        bne     LCLR
        sta     $00

        lda     #2
        sta     VBLANK

        lda     #CLPLAY
        sta     FZCLS
        lda     #LMASK0
        sta     FZGRM0
        lda     #LMASK1
        sta     FZGRM1
        lda     #LKEEP
        sta     FZKEEP
        lda     #LCURSOR
        sta     FZDCUR
        jsr     LSTAGE
        jsr     DINIT
; AFTER DINIT, not before: DINIT parks FZHL at $FF so a bank that wants no
; highlight gets none without saying so, and setting it first is silently
; undone.
        lda     #RCARD0+10      ; the cursor row: SM STRT
        sta     FZHL
        lda     #CCURSOR
        sta     FZHLC
        jmp     DFRESH

; ---------------- the per-frame hook ----------------
; The same input path the client uses: disp.inc scans once a frame and latches
; the result, and the hook drains it. This ROM had a scanner of its own until
; the client grew the latch -- and a test rig that exercises a different input
; path from the thing it is testing is worth rather less than it looks.
APPVBL: lda     FZILAT
        ldx     #0
        stx     FZILAT
        sta     FZTMP           ; newly pressed
        and     #IN_LEFT
        beq     APV1
        lda     FZDCUR
        beq     APV2
        dec     FZDCUR
        jmp     APV2
APV1:   lda     FZTMP
        and     #IN_RIGHT
        beq     APV3
        lda     FZDCUR
        cmp     #NDICE-1
        bcs     APV2
        inc     FZDCUR
APV2:   jmp     APVEND
APV3:   lda     FZTMP
        and     #IN_FIRE
        beq     APV4
; Toggle the cursor die's bit in FZKEEP. A SET bit is a RE-ROLL, so this
; UNHOLDS it; clear is held.
        ldy     FZDCUR
        lda     BITTAB,y
        eor     FZKEEP
        sta     FZKEEP
        jmp     APVEND
APV4:   lda     FZTMP
        and     #IN_SEL
        beq     APVEND
        lda     FZCLS           ; flip between the two shapes of the frame
        eor     #CLPLAY^CLOVER
        sta     FZCLS
APVEND: lda     #2              ; the bar shows only in the dice class
        ldx     FZCLS
        cpx     #CLPLAY
        beq     APV5
        lda     #0
APV5:   sta     FZMCUR
        jmp     LSTAGE

; ---------------- LSTAGE: the five dice pointers ----------------
; The low byte of a die's pointer IS its whole state -- held*DIEVAR +
; face*DIEH -- so there is no per-die test anywhere in the kernel and the
; held art costs nothing at all. The high byte is FZART's page and is written
; here rather than once, because this ROM has no bank switch to lose it to.
LSTAGE: ldy     #NDICE-1
LST1:   tya
        asl     a
        tax                     ; X = die*2: the DPTR pair's offset
        lda     LFACES,y
        asl     a               ; f*2
        asl     a               ; f*4
        sta     FZIDX
        asl     a               ; f*8
        clc
        adc     FZIDX           ; f*12 = f*DIEH
        clc
        adc     #FZART&$FF      ; ...and the art's own base; see dice.inc
        sta     FZIDX
        lda     BITTAB,y
        and     FZKEEP
        bne     LST2            ; set = re-rolls = the ordinary art
        lda     FZIDX
        clc
        adc     #DIEVAR
        jmp     LST3
LST2:   lda     FZIDX
LST3:   sta     DPTR0,x
        lda     #FZART>>8
        sta     DPTR0+1,x
        dey
        bpl     LST1
        rts

BITTAB: DB      1,2,4,8,16

        INCLUDE "fzlib.inc"
        INCLUDE "disp.inc"

        IF      * > FZART
        ERROR   "layout: code overflows into the reserved top page"
        ENDIF

        INCLUDE "diceart.inc"

; The per-row background table. Twenty-two entries, and it must not STRADDLE a
; page: the seam line's indexed load has four cycles in hand and a crossing
; costs a fifth.
        ORG     FZBGTA
FZBGT:  DB      CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG
        DB      CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG

        INCLUDE "../build/layoutscr.inc"

        ORG     $1FFC
        DW      START
        DW      START

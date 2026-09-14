; fzcard.asm -- bank 5: the scorecard.
;
; Entered from the edge bank, between frames, with everything already decided:
; whose card this is, which row is highlighted, and which rows are green.
;
; It composes IN PASSES OF ONE FRAME, because a card row costs about 450
; cycles and a vblank is 2812 -- so six rows a frame, and the card fills in
; over three. That reads as the card arriving rather than as a stall.
;
;   pass 0   the name row
;   pass 1+  the fifteen card rows, as many as the vblank has room for
;
; Then it hands the chrome to bank 6 from inside a vblank hook, so the frame
; the switch interrupts is finished by the bank entered rather than abandoned.

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "fzdefs.inc"
        INCLUDE "../build/tail.inc"
FZBANK  EQU     BANKCRD
FZHASDIC EQU    1
FZHASINP EQU    1               ; unused now: disp.inc scans unconditionally
FZHASUI EQU     0               ; the edges are bank 7's
FZHASSCR EQU    1               ; scores and the running total
FZHASPICK EQU   0
FZHASCLS EQU    0
FZHASSTR EQU    1
FZHASRPL EQU    0
FZHASED EQU     0
FZHASDEC EQU    1
FZHASNET EQU    0
FZHASHUD EQU    0

; The vblank is VBTIM (43) units of 64 cycles. Measured, not estimated: an
; ordinary row costs about eleven of them -- the eight-column label is eight
; calls and the three-column value is a repeated-subtraction decimal -- and
; the TOTAL row about twenty-eight more than that. The margins are those plus
; a little, and CMARGTOT is deliberately close to VBTIM: the TOTAL row only
; ever starts at the very top of a vblank, which is where every pass begins.
CMARGIN EQU     20
CMARGTOT EQU    36

        ORG     $1000

; ENTERED MID-FRAME, from the edge bank's vblank hook, so this bank finishes
; that frame before starting one of its own. Calling DLOOP here instead threw
; a 40-line frame at the set every time a reply landed -- DFRAME waited on a
; timer that had been armed for the VBLANK, not the overscan.
CENTRY: lda     #0
        sta     FZREDRW         ; pass 0
        jsr     DFRAME2
        jmp     DLOOP

; ---------------------------------------------------------------------------
APPVBL:
; ---- the card rows, a few to a frame ----
;
; Ask the timer after every row whether there is room for another, and TEST
; TIMINT FIRST: reading INTIM clears the latch disp.inc's own wait is watching
; for, so a hook that polled INTIM past zero would hang the frame.
        lda     FZCLS
        cmp     #CLPLAY
        bne     CDONE           ; no card on a panel screen
        lda     FZREDRW
        bne     CRW2
        inc     FZREDRW
        lda     #0
        sta     FZCIX
        jmp     GRNBLD          ; the green mask: a frame of its own
CRW2:   cmp     #1
        bne     CRWA1
        inc     FZREDRW
        jmp     CARDHD          ; the name row, and that is this frame's lot
; THE MARGIN HAS TO COVER THE ROW ABOUT TO BE DRAWN, NOT THE AVERAGE ONE.
;
; A category row is about 450 cycles. The TOTAL row is not: it calls RUNTOT,
; which walks nine score words through a sixteen-bit pointer, and then prints
; a three-digit field, and it costs nearer 1800. One margin for both is a
; choice between frames that run long -- which is what fourteen units, and
; then twenty-two, did, once per poll for the whole of a game -- and every
; ordinary row waiting a frame it did not need.
;
; So the margin is asked per row. TIMINT FIRST, always: reading INTIM clears
; the latch DFRAME2's own wait is watching for, and a hook that polled INTIM
; past zero would hang the frame rather than lengthen it.
; THE FIRST ROW OF A FRAME IS COMPOSED UNCONDITIONALLY, and that is not an
; optimisation -- it is what stops a budget from becoming a HANG.
;
; The margin is a guess about what a row costs. When the per-frame input scan
; was added to DFRAME it took about two timer units out of the vblank before
; this hook ever ran, and CMARGTOT -- deliberately close to VBTIM, because the
; TOTAL row is nearly a whole vblank's work -- stopped being reachable. The
; row was never composed, FZCIX never reached the end, the bank never handed
; over, and the client sat in the composer for the rest of the session with a
; frozen picture. A frame that runs a line long is a far smaller thing.
;
; At hook entry the vblank is always nearly whole, so the unconditional row is
; always affordable; the margin then decides only whether a SECOND one fits.
CRWA1:  ldx     FZCIX
        cpx     #NCARDR
        bcs     CDONE
        jsr     CARDRW
        inc     FZCIX
CRWA2:  ldx     FZCIX
        cpx     #NCARDR
        bcs     CDONE
        bit     TIMINT
        bmi     CRWX
        lda     #CMARGIN
        cpx     #NCARDR-1
        bne     CRWA3
        lda     #CMARGTOT
CRWA3:  cmp     INTIM
        bcs     CRWX            ; not enough left for THIS row
        jsr     CARDRW
        inc     FZCIX
        jmp     CRWA2
CRWX:   rts

CDONE:  lda     #ENCHROME
        sta     FZENT
        lda     #BANKCHR
        jmp     FZGOTO

        INCLUDE "fzlib.inc"
        INCLUDE "state.inc"
        INCLUDE "card.inc"
        INCLUDE "disp.inc"

        IF      * > FZART
        ERROR   "fzcard: code overflows into the reserved top page"
        ENDIF

        INCLUDE "diceart.inc"

        ORG     FZBGTA
FZBGT:  DB      CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG
        DB      CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG

        END

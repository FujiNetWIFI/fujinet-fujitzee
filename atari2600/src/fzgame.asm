; fzgame.asm -- bank 1: the picture, the tray, the cursors, the edges and the
; clock. The bank the program sits in whenever it is not talking or composing.

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "fzdefs.inc"
        INCLUDE "../build/tail.inc"
FZBANK  EQU     BANKGAM
FZHASDIC EQU    1
FZHASINP EQU    1               ; unused now: disp.inc scans unconditionally
FZHASUI EQU     0               ; the shadow and the diff live in the NET bank,
FZHASSCR EQU    0               ;   which has the reply fresh when it lands
FZHASPICK EQU   1               ; ...but the cursor's auto-park is input
FZHASCLS EQU    0
FZHASSTR EQU    0
FZHASRPL EQU    0
FZHASED EQU     0
FZHASDEC EQU    0
FZHASNET EQU    0
FZHASHUD EQU    0
SNDLAST EQU     SN_BAD          ; this bank fires every cue there is

        ORG     $1000

; The entry dispatch. FZENT says what the bank being entered should do, and a
; bank reached through the trampoline never returns.
GENTRY: lda     FZENT
        cmp     #ENGRUN
        beq     GERUN
        cmp     #ENGNEXT
        beq     GENEXT
; ENGCOLD: a table was just joined, or the screen class changed under us.
        jsr     DINIT
        jsr     TURNRS
        lda     #$FF
        sta     FZPRVAC
        sta     FZPRVRL
        sta     FZPRVPC
        sta     FZPRVCL
        jmp     DFRESH

; Entered from another bank's vblank hook: the frame that switch interrupted
; is still half drawn, so this bank finishes it. The RIOT timer does not care
; which bank is mapped.
GERUN:  jsr     DFRAME2
        jmp     DLOOP

; Entered BETWEEN frames -- the overscan timer is already armed by whoever
; called DFRAME2 last -- so this one must not arm it again.
GENEXT: jmp     DLOOP

; ---------------------------------------------------------------------------
; APPVBL -- the per-frame hook, inside the vblank.
APPVBL: jsr     SNDSTEP
        jsr     ANMTICK
; Drain the latch DFRAME filled in. Not a scan: the scan already happened
; this frame, in this bank or in whichever one was mapped when the button
; actually went down.
        lda     FZILAT
        ldx     #0
        stx     FZILAT
        sta     FZINP

; The left difficulty switch is a LEVEL. Flipping it swaps between the game
; and the standings, and that is a screen CLASS change, so it goes through the
; composer rather than being drawn here.
        lda     INCUR
        and     #IN_DIFF
        cmp     FZDIFF
        beq     GA1
        sta     FZDIFF
        jmp     GCLASS

GA1:    lda     FZINP
        and     #IN_SEL
        beq     GA2
        lda     #ENMENU
        sta     FZENT
        lda     #BANKMNU
        jmp     FZGOTO

GA2:    lda     FZINP
        and     #IN_RST
        beq     GA3
        lda     #0              ; RESET is a SWITCH on this console and it
        sta     FZPOLL          ;   restarts nothing: poll now
        jmp     GA4

; READYING UP IS SAMPLED HERE, EVERY FRAME, and not in the lobby's own screen:
; the lobby is a panel the composer draws once a poll, so a press made between
; two polls would simply be dropped. Every port in this family ended up
; sampling the toggle in the wait loop for the same reason.
GA3:    lda     FZCLS
        cmp     #CLLOBBY
        bne     GA3B
        lda     FZINP
        and     #IN_FIRE
        beq     GA4
        lda     #RQREADY        ; a BARE /ready toggles; the seat's own flag
        sta     FZREQ2          ;   comes back in scores[0]
        lda     #SN_SEL
        jsr     SNDCUE
        jmp     GA4
GA3B:   lda     FZINP
        jsr     TURNIN

GA4:    jsr     GCLOCK
; The poll clock stops AT zero and the switch is taken here, INSIDE the
; vblank, so the network bank finishes this frame. The test comes before the
; decrement, or a due poll would wrap the clock to 255.
        lda     FZREQ2
        bne     GAFETCH
        lda     FZPOLL
        beq     GAFETCH
        dec     FZPOLL
        rts
GAFETCH: lda    #ENFETCH
        sta     FZENT
        lda     #BANKNET
        jmp     FZGOTO

; GCLASS -- the screen class changed: recompose everything.
GCLASS: jsr     GSETCLS
        lda     #ENGAME
        sta     FZENT
        lda     #BANKCRD
        jmp     FZGOTO

; ---------------------------------------------------------------------------
; GCLOCK -- the move clock, counted down LOCALLY.
;
; The reply window belongs to the cartridge, so counting down in place would
; go nowhere; and when it reaches zero the server has already force-scored and
; moved on, so the clock POLLS rather than waiting for a button that will
; never come.
GCLOCK: lda     FZMYTRN
        beq     GCL9
        lda     FZCLK
        beq     GCL9
        dec     FZTICK
        bne     GCL9
        lda     #60
        sta     FZTICK
        dec     FZCLK
        bne     GCL1
        lda     #0              ; time up: find out where the game went
        sta     FZPOLL
        rts
GCL1:   lda     FZCLK           ; a click on the last few seconds
        cmp     #6
        bcs     GCL9
        lda     #SN_CLICK
        jmp     SNDCUE
GCL9:   rts

        INCLUDE "fzlib.inc"
        INCLUDE "state.inc"
        INCLUDE "dice.inc"
        INCLUDE "turn.inc"
        INCLUDE "sound.inc"
        INCLUDE "disp.inc"

; THE CODE MUST STOP BEFORE THE RESERVED TOP PAGE. Without this the art's ORG
; silently assembles on top of whatever ran past it, the listing reports a
; bank that fits, and the dice draw the bank's own instructions.
        IF      * > FZART
        ERROR   "fzgame: code overflows into the reserved top page"
        ENDIF

        INCLUDE "diceart.inc"

; The per-row background table. Twenty-two entries: the seam at the bottom of
; row 20 programmes a row 21 that does not exist, and making that entry the
; page colour hands over to the bottom band for free. It must not STRADDLE a
; page -- the seam's indexed load has four cycles in hand and a page crossing
; costs a fifth -- which $1790 and twenty-two bytes cannot do.
        ORG     FZBGTA
FZBGT:  DB      CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG
        DB      CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG

        END

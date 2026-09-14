; fzchrome.asm -- bank 6: everything on the screen that is not the card.
;
; The status row, the server's prompt, and the three panel screens -- the
; lobby's ready list, the standings and the game-over banner -- which are all
; the same picture with a different right-hand field.
;
; Entered from the card bank's vblank hook, so it finishes that frame before
; starting its own, and hands back to the game bank the same way.

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "fzdefs.inc"
        INCLUDE "../build/tail.inc"
FZBANK  EQU     BANKCHR
FZHASDIC EQU    1
FZHASINP EQU    1               ; unused now: disp.inc scans unconditionally
FZHASUI EQU     0
FZHASSCR EQU    1               ; names, ready flags and running totals
FZHASPICK EQU   0
FZHASCLS EQU    0
FZHASSTR EQU    1
FZHASRPL EQU    1               ; the server's name, straight out of the window
FZHASED EQU     0
FZHASDEC EQU    1
FZHASNET EQU    0
FZHASHUD EQU    0

        ORG     $1000

; Entered MID-FRAME from the card bank's hook: finish that frame here, then
; run. The RIOT timer does not care which bank is mapped.
HENTRY: lda     #0
        sta     FZREDRW
        jsr     DFRAME2
        jmp     DLOOP

; ---------------------------------------------------------------------------
; One pass a frame, so no single vblank has to carry the whole chrome.
APPVBL: lda     FZREDRW
        beq     HP0
        cmp     #1
        beq     HP1
        cmp     #2
        beq     HP2
; Done: hand the picture back to the game bank, which finishes this frame.
        lda     #ENGRUN
        sta     FZENT
        lda     #BANKGAM
        jmp     FZGOTO

HP0:    inc     FZREDRW
        jsr     PANRST
        jmp     STATROW

; The panel: the seat list. In play there is no panel -- the card is the
; screen -- so this pass is the hint line instead.
; The panel does ONE SEAT A FRAME and says when it is done, so this pass stays
; on until it is. Nine seats of running totals in one vblank is a 270-line
; frame.
HP1:    lda     FZCLS
        cmp     #CLPLAY
        beq     HP1G
        cmp     #CLLOBBY
        bne     HP1T
        lda     #0              ; the lobby: scores[0] is the ready flag
        jmp     HP1D
HP1T:   lda     #1              ; standings and game over: running totals
HP1D:   jsr     PANEL
        bcc     HP1X
HP1G:   inc     FZREDRW
HP1X:   rts

; The prompt. IN PLAY IT IS NOT DRAWN: rows 17-20 are the dice tray and the
; ROLL tile, and the server sends an empty prompt for the whole of play
; anyway. At game over it is the entire point of the screen -- it is the only
; thing on the wire that names the winner.
HP2:    inc     FZREDRW
        lda     FZCLS
        cmp     #CLPLAY
        beq     HP2H
        jmp     PROMPTW
HP2H:   ldx     #SHINT&$FF      ; in play, one line of advice instead
        ldy     #SHINT>>8
        jmp     HINT

SHINT:  DB      "SEL=MENU",0

        INCLUDE "fzlib.inc"
        INCLUDE "state.inc"
        INCLUDE "chrome.inc"
        INCLUDE "disp.inc"

        IF      * > FZART
        ERROR   "fzchrome: code overflows into the reserved top page"
        ENDIF

        INCLUDE "diceart.inc"

        ORG     FZBGTA
FZBGT:  DB      CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG
        DB      CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG

        END

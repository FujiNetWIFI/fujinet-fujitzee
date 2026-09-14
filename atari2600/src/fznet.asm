; fznet.asm -- bank 2: one request, with the picture up, and back.
;
; THE FETCH HAS A BANK OF ITS OWN, and that is why there are seven. net.inc
; and url.inc are ~550 bytes together and the game bank needs every byte it
; can get. A poll happens every ninety frames, so paying a bank switch for one
; costs nothing.
;
; IT CARRIES THE KERNEL AND THE DICE ART, though it composes nothing. A
; transaction is several socket round trips and the transport's wait for
; ACKSEQ is a spin loop; spun blind, the picture stops for the duration and
; the set sees a flash once a poll. NFRAME spends every wait in DFRAME
; instead, redrawing out of the text rows the composer built and the dice
; pointers the last roll staged -- the reply window is not repainted until the
; READ, so what is on screen stays true right up to the moment it changes.
;
; And that is why THE ART MUST BE AT FZART HERE TOO: DPTRn's high byte was
; written by a DINIT this bank has never run.

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "fzdefs.inc"
        INCLUDE "../build/tail.inc"
FZBANK  EQU     BANKNET
FZHASDIC EQU    1
FZHASINP EQU    1               ; unused now: disp.inc scans unconditionally
FZHASUI EQU     0               ; the edges and the shadow are the COMPOSER's:
FZHASSCR EQU    0               ;   it has to react to them anyway, and this
                                ;   bank had no room for them at all
FZHASPICK EQU   0
FZHASCLS EQU    0
FZHASSTR EQU    0
FZHASRPL EQU    0
FZHASED EQU     0
FZHASDEC EQU    0
FZHASNET EQU    1
FZHASHUD EQU    0
; No sound.inc at all: nothing happens in this bank that a player did.

        ORG     $1000

NENTRY: lda     FZENT
        cmp     #ENTABLE
        beq     NTAB
        cmp     #ENLEAVE
        beq     NLEAVE

; ---- ENFETCH: /state, or the move staged in its place ----
;
; A STAGED ACTION REPLACES /state RATHER THAN PRECEDING IT. Every endpoint
; returns the same blob, so the roll or the score IS the next state and there
; is never a second round trip.
        jsr     DFRAME2         ; finish the frame the game bank's hook left
        lda     FZREQ2
        bne     NF1
        lda     #RQSTATE
NF1:    ldx     #0
        stx     FZREQ2
        jsr     APICALL
        cmp     #FNEOK
        bne     NFAIL
        jsr     NGOOD
        lda     #ENEDGE         ; the edges first, then the card, then the
        sta     FZENT           ;   chrome, then back to the game bank
        lda     #BANKEDG
        jmp     FZGOTO

; A failed poll keeps the picture it has and tries again later. The menu is
; still reachable, so a server that has gone away is not a hang.
NFAIL:  sta     FZERR
        lda     #FAILFRM
        sta     FZPOLL
        lda     #ENGNEXT
        sta     FZENT
        lda     #BANKGAM
        jmp     FZGOTO

; ---- ENTABLE: the lobby's listing ----
NTAB:   lda     #RQTABLE
        jsr     APICALL
        sta     FZERR
        lda     #ENLOBBY
        sta     FZENT
        lda     #BANKLOB
        jmp     FZGOTO

; ---- ENLEAVE: give the seat up, then list the tables again ----
;
; LEAVE and then a fresh /tables, so the seat is GIVEN UP rather than
; abandoned -- the server holds an abandoned seat until its clock runs out and
; everyone else waits for it.
NLEAVE: lda     #RQLEAVE
        jsr     APICALL
        jsr     NCLRTB          ; forget the table id and the room appkey
        jmp     NTAB

; NCLRTB -- empty the table-id buffer, so the next URL cannot carry a seat we
; have just given up.
NCLRTB: lda     #PBTABLE
        sta     FNRSEL+FH_PATHO
        lda     #FP_RST
        sta     FNRSEL+FH_PATHO
        rts

; ---------------------------------------------------------------------------
; NGOOD -- a reply landed and validated. Cache it, and work out what changed.
NGOOD:  lda     #0
        sta     FZERR
        jsr     GHDR
        jsr     GSETCLS

; The poll cadence is per phase: the lobby's countdown and the game-over reset
; are the server's own wall clock, and neither is worth 1.5 seconds of polling.
        ldx     #POLLFRM
        lda     FZCLS
        cmp     #CLLOBBY
        bne     NG1
        ldx     #LOBFRM
        jmp     NG2
NG1:    cmp     #CLOVER
        bne     NG2
        ldx     #OVERFRM
NG2:    stx     FZPOLL

        rts

; This bank draws but never composes, so its hook is an rts -- and it must
; still be called, because DFRAME calls it unconditionally.
APPVBL: rts

        INCLUDE "fzlib.inc"
        INCLUDE "state.inc"
        INCLUDE "net.inc"
        INCLUDE "url.inc"
        INCLUDE "disp.inc"

        IF      * > FZART
        ERROR   "fznet: code overflows into the reserved top page"
        ENDIF

        INCLUDE "diceart.inc"

        ORG     FZBGTA
FZBGT:  DB      CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG
        DB      CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG

        END

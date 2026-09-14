; url.inc -- building the devicespec, streamed straight into the TX page a
; character at a time.
;
; NOTHING IS ASSEMBLED IN RAM, because there is no RAM to assemble it in: the
; cartridge counts the payload itself, so nothing here tracks a length. The
; worst case is /score/14 with the query, about 90 bytes against the 320-byte
; TX ceiling.
;
; The table id and the player name never enter console RAM at all. They live
; in the cartridge's own path buffers and are streamed out with FP_TXRAW --
; "raw", not FP_TX, because FP_TX pads to exactly 256 bytes and padding in the
; middle of a URL is not a URL.
;
; THE ROLL MASK IS EXPANDED HERE AND NOWHERE ELSE. FZKEEP is one byte, a bit
; per die, in the WIRE's sense: a SET bit means that die RE-ROLLS. The server
; wants five ascii '0'/'1' in that same sense, so the expansion is a bit test
; and a character -- and no five-byte buffer exists anywhere in the program.

; UTXS -- append the NUL-terminated string at FNPTRL/H to the TX stream.
UTXS:   ldy     #0
UTXS1:  lda     (FNPTRL),y
        beq     UTXS2
        sta     FNTX
        iny
        bne     UTXS1
UTXS2:  rts

; UTXP -- point at the string whose address is X (lo) / Y (hi) and stream it.
UTXP:   stx     FNPTRL
        sty     FNPTRH
        jmp     UTXS

; UTXB -- stream the selected cartridge path buffer exactly as it stands.
UTXB:   sta     FNRSEL+FH_PATHO ; select
        lda     #FP_TXRAW
        sta     FNRSEL+FH_PATHO
        rts

; ---------------------------------------------------------------------------
; BLDURL -- stream the devicespec for request FZREQ.
BLDURL: lda     #'N'
        sta     FNTX
        lda     #':'
        sta     FNTX
        ldx     #UENDPT&$FF
        ldy     #UENDPT>>8
        jsr     UTXP

        lda     FZREQ
        cmp     #RQTABLE
        bne     BURL1
; /tables carries its own query and takes neither a table nor a player: the
; whole point of it is that we have not joined one yet.
        ldx     #UTABLS&$FF
        ldy     #UTABLS>>8
        jmp     UTXP

BURL1:  cmp     #RQSTATE
        bne     BURL2
        ldx     #USTATE&$FF
        ldy     #USTATE>>8
        jsr     UTXP
        jmp     BURLQ

BURL2:  cmp     #RQREADY
        bne     BURL3
; A BARE /ready TOGGLES, matching every other client in the family: the lobby
; reads the seat's own ready flag back out of scores[0] rather than tracking
; what it thinks it sent.
        ldx     #UREADY&$FF
        ldy     #UREADY>>8
        jsr     UTXP
        jmp     BURLQ

BURL3:  cmp     #RQROLL
        bne     BURL4
        ldx     #UROLL&$FF
        ldy     #UROLL>>8
        jsr     UTXP
        ldx     #0
BURL3A: lda     BITTAB,x
        and     FZKEEP
        beq     BURL3B
        lda     #'1'            ; set = this die RE-ROLLS
        bne     BURL3C
BURL3B: lda     #'0'            ; clear = held
BURL3C: sta     FNTX
        inx
        cpx     #NDICE
        bne     BURL3A
        jmp     BURLQ

BURL4:  cmp     #RQSCORE
        bne     BURL5
        ldx     #USCORE&$FF
        ldy     #USCORE>>8
        jsr     UTXP
        lda     FZSCIX          ; 0-14: a tens digit only when there is one
        cmp     #10
        bcc     BURL4A
        sec
        sbc     #10
        pha
        lda     #'1'
        sta     FNTX
        pla
BURL4A: clc
        adc     #'0'
        sta     FNTX
        jmp     BURLQ

BURL5:  ldx     #ULEAVE&$FF
        ldy     #ULEAVE>>8
        jsr     UTXP

BURLQ:  ldx     #UQTAB&$FF
        ldy     #UQTAB>>8
        jsr     UTXP
        lda     #PBTABLE
        jsr     UTXB
        ldx     #UQPLR&$FF
        ldy     #UQPLR>>8
        jsr     UTXP
        lda     #PBNAME
        jsr     UTXB
        ldx     #UQBIN&$FF
        ldy     #UQBIN>>8
        jmp     UTXP

        INCLUDE "../build/endpoint.inc"
UTABLS: DB      "tables?bin=1",0
USTATE: DB      "state",0
UREADY: DB      "ready",0
UROLL:  DB      "roll/",0
USCORE: DB      "score/",0
ULEAVE: DB      "leave",0
UQTAB:  DB      "?table=",0
UQPLR:  DB      "&player=",0
UQBIN:  DB      "&bin=1",0

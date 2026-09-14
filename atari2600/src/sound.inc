; sound.inc -- one TIA channel, a script stepped once a frame.
;
; A cue is a list of (frequency, frames) pairs ending in $FF. AUDC and the
; volume are fixed for the whole vocabulary, which is what keeps a cue two
; bytes a step instead of four -- this is a dice game, not a synthesiser.
;
; THE CUES ARE NUMBERED SO THAT EACH BANK'S ARE A PREFIX OF THE LIST, and
; SNDLAST says where that bank's copy of the table stops. A byte in a shared
; include costs one byte of EVERY bank that includes it, and the lobby fires
; three of these nine.

SNDC    EQU     4               ; AUDC: a pure tone
SNDV    EQU     8               ; AUDV: half volume, so a cue is a cue and not
                                ;   an event

SN_NONE EQU     0
SN_CLICK EQU    1               ; a cursor step. ONE FRAME, so auto-repeat
                                ;   does not drag a tone behind it
SN_SEL  EQU     2               ; a choice taken
SN_HOLD EQU     3               ; a die held or let go
SN_ROLL EQU     4               ; the dice go out
SN_TURN EQU     5               ; your turn -- BEFORE the draw: an alert after
                                ;   the fact is not one
SN_BOT  EQU     6               ; somebody else took a row; falls where ours
                                ;   rise, so the two are never confused
SN_OVER EQU     7               ; game over
SN_BAD  EQU     8               ; a move the client refuses

; SNDCUE -- start cue A. A cue already running is simply replaced: the newer
; thing is always the one worth hearing.
SNDCUE: cmp     #SNDLAST+1
        bcs     SNDC9           ; a cue this bank does not carry: silence is
                                ;   better than the next cue's first step
        tax
        lda     SNDOFF,x
        sta     SNDPTR
        lda     #0
        sta     SNDCNT
        rts
SNDC9:  rts

; SNDSTEP -- one frame of it, from the vblank hook.
SNDSTEP: lda   SNDPTR
        beq     SNDS9
        lda     SNDCNT
        beq     SNDS1
        dec     SNDCNT
        rts
SNDS1:  ldx     SNDPTR
        lda     SNDTAB,x
        cmp     #$FF
        beq     SNDS8
        sta     AUDF0
        lda     #SNDC
        sta     AUDC0
        lda     #SNDV
        sta     AUDV0
        inx
        lda     SNDTAB,x
        sta     SNDCNT
        inx
        stx     SNDPTR
        rts
SNDS8:  lda     #0
        sta     AUDV0
        sta     SNDPTR
SNDS9:  rts

; Offsets into SNDTAB, by cue. Entry 0 is the silent one and is never started.
SNDOFF: DB      SNT_NONE-SNDTAB
        DB      SNT_CLICK-SNDTAB
        DB      SNT_SEL-SNDTAB
        DB      SNT_HOLD-SNDTAB
        DB      SNT_ROLL-SNDTAB
        DB      SNT_TURN-SNDTAB
        DB      SNT_BOT-SNDTAB
        DB      SNT_OVER-SNDTAB
        DB      SNT_BAD-SNDTAB

SNDTAB:
SNT_NONE: DB    $FF
SNT_CLICK: DB   12,1,$FF
SNT_SEL: DB     16,3,10,4,$FF
SNT_HOLD: DB    20,3,$FF
SNT_ROLL: DB    6,2,10,2,14,2,18,3,$FF
SNT_TURN: DB    20,4,14,4,8,6,$FF
SNT_BOT: DB     10,5,18,7,$FF
SNT_OVER: DB    8,10,14,10,22,16,$FF
SNT_BAD: DB     30,8,$FF

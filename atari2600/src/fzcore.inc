; fzcore.inc -- the contents of the fixed half.
;
; $1800-$1F1F is the cartridge's mailbox -- text planes, reply window, control
; page, TX page, status -- and the cartridge paints it. The client owns
; $1F20-$1FFB, which fuji_mailbox.h calls the fixed tail, plus the vectors.
;
; Everything here has to be at an address that does not move, for three
; different reasons:
;
;   * The store that switches bank is the LAST instruction fetched from the
;     old bank and the very next fetch comes from the new one, so the jump
;     after it cannot live in a bank.
;   * This console has no reset line to the cartridge. The RESET switch
;     restarts the 6507 with whatever bank was last selected still mapped, so
;     a cold stub living in bank 0 would simply not be there when it was
;     needed.
;   * The transport is the same ~140 bytes in every bank that talks, and a
;     bank is 2048 bytes. Here it is one copy all of them can reach.
;
; Included by fztail.asm, which becomes the real client's last 2K. The banks
; do NOT include it: they include build/tail.inc, which tools/mktail.py
; generates from the addresses this file actually assembled to, so there is no
; hand-kept address list to go stale.

; FZGOTO is not a label: fzdefs.inc gives it a fixed address and this ORG is
; what makes that true, so the two cannot drift apart. It must be FIRST in the
; tail, because it is the one address a bank has to know before build/tail.inc
; exists.
        ORG     FZGOTO

; ---------------------------------------------------------------------------
; FZGOTO -- select bank A and enter it at $1000.
;
; `sta FNRSEL,x` is the documented-safe indexed form: the base low byte is $00
; so the index cannot carry, which means the dummy read STA abs,X always
; performs lands on the same address as the write.
        clc
        adc     #FH_BANK
        tax
        sta     FNRSEL,x
; RESET THE STACK. A bank switch is a JUMP and nothing ever returns through
; one, so every switch abandons whatever return addresses were on the stack --
; and the switches here are taken from inside APPVBL, which DLOOP reaches with
; a JSR. Battleship leaked two bytes per poll, a poll every ninety frames, and
; after thirty-five of them the stack had grown down out of its region and was
; overwriting the entry code, so a bank dispatched on the low byte of a return
; address.
;
; The store above has already switched the bank, but these instructions are
; fetched from the FIXED tail, which is not banked, so they still execute.
        ldx     #$FF
        txs
        jmp     $1000

; ---------------------------------------------------------------------------
; The shared transport.
;
; TWO RULES ABOUT THE CONTROL PAGES, both enforced by tools/checkrom.py:
;
;   A register write is TWO stores on this console, not one:
;
;       lda #value
;       sta FNRSEL+reg          ; arm; this store's data is ignored
;       sta FNCMT               ; commit: register `reg` = value
;
;   And never a read-modify-write instruction on the control or TX pages.
;   INC/DEC/ASL/LSR/ROL/ROR absolute are three bus cycles at one address --
;   read, write old, write new -- and the cartridge's "last value before the
;   address changed" sampling has no defined answer in that burst. Only
;   STA/STX/STY, and never an indirect store anywhere.
FNRW:   sta     FNRSEL,x
        sta     FNCMT
        rts

; FNARM -- open the decode gate. Two stores, in order, carrying two specific
; values; nothing in the control page decodes until they arrive.
FNARM:  lda     #FNAM1
        sta     FNRSEL+FH_ARM1
        lda     #FNAM2
        sta     FNRSEL+FH_ARM2
        rts

; FNCHK -- Z set if a FujiNet cartridge is answering.
FNCHK:  lda     FNMAG0
        cmp     #'F'
        bne     FNCHK1
        lda     FNMAG1
        cmp     #'N'
FNCHK1: rts

; FNBEG -- begin a transaction described by FNDEV / FNCMD / FNNPR.
; Rewinds the TX stream LAST, so parameters may be appended immediately.
FNBEG:  lda     #16             ; ~9s; longer than the cart's own 5s budget so
        sta     FNTMO           ;   a real timeout is reported as ITS error
        lda     FNDEV
        ldx     #FR_DEV
        jsr     FNRW
        lda     FNCMD
        ldx     #FR_CMD
        jsr     FNRW
        lda     FNNPR
        ldx     #FR_NPAR
        jsr     FNRW
        lda     #0
        ldx     #FR_DRST
        jmp     FNRW

; FNPB -- append a one-byte parameter with value A. The stream is
; NPARAM x { size byte, then that many value bytes } and then the payload.
FNPB:   pha
        lda     #1
        sta     FNTX
        pla
        sta     FNTX
        rts

; FNPW -- append a two-byte little-endian parameter. A = low, X = high.
FNPW:   pha
        lda     #2
        sta     FNTX
        pla
        sta     FNTX
        txa
        sta     FNTX
        rts

; FNGO -- commit and wait. Returns A = FNE* (0 = the transaction completed).
;
; THE RULE EVERY PORT IN THIS FAMILY LEARNED THE HARD WAY: the next sequence
; number comes from the CART'S OWN persisted ACKSEQ + 1, never from a counter
; in RAM. A console RESET restarts this client and re-zeroes its variables but
; does NOT reset the cartridge -- there is no reset line on the connector at
; all, and the RESET switch is a bit in a RIOT register the cart never sees.
; A program-local counter would restart at 1, collide with a sequence already
; answered, and every later transaction would be dropped in silence.
;
; net.inc has a twin of this, NPGO, which spends the wait DRAWING FRAMES. This
; one is for the transactions taken on a screen nobody is looking at -- the
; appkey reads, and /tables on the way into the lobby.
FNGO:   lda     FNACKS
        clc
        adc     #1
        bne     FNGO1
        lda     #1              ; 0 is reserved as "never used"
FNGO1:  sta     FNSEQ
        ldx     #FR_SEQ
        jsr     FNRW            ; launching it is this single commit
        lda     FNTMO
        sta     FNCNT
FNGO2:  ldy     #0
FNGO3:  ldx     #0
        ; ACKSEQ is published LAST, after the reply, its length and the error
        ; code are all in place, so a match means everything behind it is
        ; already readable.
FNGO4:  lda     FNACKS
        cmp     FNSEQ
        beq     FNGO5
        dex
        bne     FNGO4
        dey
        bne     FNGO3
        dec     FNCNT
        bne     FNGO2
        lda     #FNEWAIT
        rts
FNGO5:  lda     FNERR
        rts

; FNACK -- 0 on success, FNENAK if the server said no.
FNACK:  lda     FNRCMD
        cmp     #$06
        beq     FNACK1
        lda     #FNENAK
        rts
FNACK1: lda     #FNEOK
        rts

; -----------------------------------------------------------------------
; Text, composed by the cartridge. Shared for the same reason the transport
; is: every bank draws.

; FNROWA -- begin composing text row A.
FNROWA: sta     FNRSEL+FH_TROW
        rts

; FNCHR -- append character A to the row being composed.
FNCHR:  sta     FNRSEL+FH_TCHR
        rts

; FNENDR -- render the composed row.
FNENDR: lda     #0
        sta     FNRSEL+FH_TEND
        rts

; FNSPC -- append A spaces.
FNSPC:  tax
FNSPC1: lda     #' '
        sta     FNRSEL+FH_TCHR
        dex
        bne     FNSPC1
        rts

; ---------------------------------------------------------------------------
; FZCOLD -- power-on and RESET.
;
; Deliberately TINY. Everything a cold start has to do -- clear the RAM, blank
; the screen, empty the cartridge's four path buffers -- is done by bank 0,
; because the tail is 220 bytes and the transport and text primitives want
; most of them. All that has to be here is what cannot be anywhere else: the
; arming pair, because banking is a control-page operation and that page
; decodes nothing until an ordered pair of stores carrying two specific values
; arrives, and the bank switch itself.
FZCOLD: sei
        cld
        ldx     #$FF
        txs
; FORCE A COLD ENTRY. This console does not clear its RAM on a reset, so FZENT
; -- the byte that says what the bank being entered should do -- still holds
; whatever the last screen set it to. A reset taken at the keyboard would come
; back into the lobby's WARM path, which redraws a table list out of a reply
; window that is holding a Game, and reads table names out of the middle of a
; player record.
        lda     #ENCOLD
        sta     FZENT
        lda     #FNAM1
        sta     FNRSEL+FH_ARM1
        lda     #FNAM2
        sta     FNRSEL+FH_ARM2
        lda     #BANKLOB
        jmp     FZGOTO

        ORG     $1FFC
        DW      FZCOLD
        DW      FZCOLD

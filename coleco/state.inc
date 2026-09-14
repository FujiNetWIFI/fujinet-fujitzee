; state.inc -- reading the reply where it lies.
;
; This is the module the ColecoVision makes SIMPLE. Its sibling ports page a
; 1K reply through a 256-byte slice window, so each of them carries a flat
; 16-bit cursor, a per-byte RXNEXT that notices slice crossings, and a
; slice-select handshake -- arcadia's twelfth seat ends at offset 598, three
; slices in. Here the cart paints the whole reply at FNRDATA in one piece, so
; a field is an address and nothing more.
;
; Nothing is buffered anywhere. Names and the server prompt render straight
; out of the window (disp.inc's DPRTN), numbers go straight into cells
; (NUMFLD), and the only strings copied to RAM are the two that ride out in a
; URL -- the player name and the table id.
;
; The window is only stable between transactions: the cart repaints it on a
; SEQ commit. So everything that reads state does so after a poll and before
; the next request goes out, which is exactly how the render path is ordered.

; RXHDR: A = the header byte at offset A (0-94). Clobbers D,E,H,L.
RXHDR:  LD      L,A
        LD      H,0
        LD      DE,FNRDATA
        ADD     HL,DE
        LD      A,(HL)
        RET

; RXHDRP: HL = the address of header offset A. Clobbers A,D,E.
RXHDRP: LD      L,A
        LD      H,0
        LD      DE,FNRDATA
        ADD     HL,DE
        RET

; RXPLR: HL = the base of player record A.
; The stride is 42, built as 32 + 8 + 2 rather than by repeated addition:
; twelve seats reach offset 598, so this has to be 16-bit throughout.
; Clobbers A,B,C,D,E.
RXPLR:  LD      L,A
        LD      H,0
        ADD     HL,HL           ; 2A
        LD      D,H
        LD      E,L
        ADD     HL,HL           ; 4A
        ADD     HL,HL           ; 8A
        LD      B,H
        LD      C,L
        ADD     HL,HL           ; 16A
        ADD     HL,HL           ; 32A
        ADD     HL,BC           ; 40A
        ADD     HL,DE           ; 42A
        LD      DE,FNRDATA+GOPLYRS
        ADD     HL,DE
        RET

; RXPNAM: HL = player A's name field, ready for DPRTN with B = 9.
RXPNAM  EQU     RXPLR

; RXSCOR: HL = player A's score for slot C, as the raw 16-bit wire value.
; FFFFH means unset, FFFEH means the seat is a spectator; slot 0 doubles as
; the lobby's ready flag. Clobbers A,B,C,D,E.
RXSCOR: PUSH    BC              ; RXPLR builds its stride in BC and would
        CALL    RXPLR           ; otherwise eat the slot number
        POP     BC
        PUSH    HL
        LD      L,C
        LD      H,0
        ADD     HL,HL           ; slot * 2
        LD      DE,PLSCORS
        ADD     HL,DE
        POP     DE
        ADD     HL,DE
        LD      E,(HL)
        INC     HL
        LD      D,(HL)
        EX      DE,HL
        RET

; RXSET: NC when HL holds a real score, C when it is unset or spectator.
; Both sentinels are FFFEH or above, so one compare settles it.
RXSET:  LD      A,H
        CP      0FFH
        JR      Z,RXS1
        OR      A               ; a real score. CP left a borrow behind for
        RET                     ; every high byte below FFH, so clear it
RXS1:   LD      A,L
        CP      0FEH            ; FFFE and FFFF are the two sentinels
        CCF                     ; carry set for FEH and above
        RET

; RXVALID: A = validScores[A] -- the exact points that row would earn now.
; FFH means the row is closed. ALL FIFTEEN ARE ZERO IN THE LOBBY, so only
; rounds 1-13 may call this; reading it in the lobby would paint a board full
; of zeros. Clobbers D,E,H,L.
RXVALID:
        ADD     A,GOVALID
        JP      RXHDR

; RXDIE: A = the ascii face of die A ('1'-'6'), or 0 at the start of a turn
; before any roll has landed.
RXDIE:  ADD     A,GODICE
        JP      RXHDR

; RXKEEP: A = keepRoll[A]: ascii '1' if that die will be re-rolled, '0' if it
; is being held. Note the sense -- the wire's "keep" flag names the roll, not
; the die.
RXKEEP: ADD     A,GOKEEP
        JP      RXHDR

; RXSTRN: copy B bytes from reply offset HL into RAM at DE, NUL-terminated.
; The only three things ever copied out of the window: your name, the table
; id, and the roll mask -- the strings that have to outlive the next
; transaction because they ride out in a URL.
; Clobbers A,B,C,D,E,H,L.
RXSTRN: PUSH    DE
        LD      DE,FNRDATA
        ADD     HL,DE
        POP     DE
        LD      C,B
        LD      B,0
        LDIR
        XOR     A
        LD      (DE),A
        RET

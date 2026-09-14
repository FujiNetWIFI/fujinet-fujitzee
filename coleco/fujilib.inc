; fujilib.inc -- Z80 client half of the FujiNet cartridge mailbox.
;
; Hand-mirrors fujinet-firmware/pico/coleco/firmware/include/fuji_mailbox.h,
; which is the single source of truth; every equate here is a CONSOLE address,
; i.e. the cart image offset plus 8000H. Keep the two in step.
;
; The cartridge connector carries A0-A14, D0-D7, four pre-decoded chip selects
; and power -- no /RD, no /WR, no /MREQ. The cart cannot tell a read from a
; write, so BOTH directions ride reads: a "register write" is a read of
; FNREGSEL+reg followed by a read of FNREGDAT+value (the low address byte IS
; the payload), and a reply is plain ROM the cart repaints. A REGDATA read with
; no immediately preceding REGSEL read is ignored -- that pairing rule is the
; stray-read defense, so the helpers below always issue the pair back to back.
;
; TWO DIFFERENCES FROM THE ASTROCADE AND ARCADIA CLIENTS
;
; 1. The reply is one 1K block, not a paged 256-byte slice. The window here is
;    32K and the CONSOLE is what is scarce, so the cart hands over the whole
;    reply as directly addressable ROM. FNSLICE and the slice-echo poll simply
;    do not exist; a field is LD HL,FNRDATA+off / LD A,(HL).
;
; 2. Those clients run with DI for their whole life, which makes the mailbox
;    unreachable by any interrupt. We cannot: the ColecoVision's vblank is on
;    /NMI and DI will not stop it, and we want it anyway -- POLLER and the
;    frame counter ride it. It WILL land in the middle of a REGSEL/REGDATA
;    pair. That is harmless by design, provided the handler never reads FNRDATA
;    and up: an armed register waits, and the TX stream is append-only, so an
;    interrupted transaction simply resumes. NMI in fujitzee.asm keeps that
;    rule; tools/checkrom.py enforces the image half of it.
;
; Refresh cycles are inhibited by the console's own decoder (/RFSH is an enable
; on U5), so the R-register stray hazard the Astrocade had to defend against
; cannot arise here at all.

; ---- reply bytes (repainted ROM) --------------------------------------
FNRDATA EQU     0F800H          ; the whole 1K reply, in one piece
FNRDMAX EQU     1024
FNACKSQ EQU     0FC00H          ; echoes SEQ when the reply is ready
FNSTATS EQU     0FC01H          ; bit0 link up, bit1 busy
FNERR   EQU     0FC02H          ; fb_status_t of the last transaction
FNREPLY EQU     0FC03H          ; 06H ACK / 15H NAK
FNRXLL  EQU     0FC04H          ; reply length, low
FNRXLH  EQU     0FC05H          ; reply length, high
FNMAGF  EQU     0FC09H          ; 'F'
FNMAGN  EQU     0FC0AH          ; 'N'
FNPVER  EQU     0FC0BH          ; protocol version

; ---- hotspot pages ----------------------------------------------------
FNREGSEL EQU    0FD00H          ; read +reg: arm register
FNREGDAT EQU    0FE00H          ; read +val: armed register = val
FNDATA  EQU     0FF00H          ; read +val: append val to the TX stream
FNRSELP EQU     0FDH            ; page bytes of the above (zmac 1.3 has no
FNRDATP EQU     0FEH            ; HIGH operator)
FNDATAP EQU     0FFH

; ---- register numbers -------------------------------------------------
FRDEVIC EQU     00H
FRCMD   EQU     01H
FRNPARM EQU     02H
FRDATRS EQU     05H
FRSEQ   EQU     10H

; ---- values -----------------------------------------------------------
FCACK   EQU     06H             ; reply command: ACK
FCNAK   EQU     15H             ; reply command: NAK
FEOK    EQU     0               ; FNERR values
FENOLNK EQU     1
FETMOUT EQU     2
FNTXMAX EQU     320             ; TX stream ceiling

; ---- routines ---------------------------------------------------------

; FNREGWR: write mailbox register C = value A.
; The two reads are back to back on purpose; an NMI between them is safe.
; Clobbers A,B,D,E.
FNREGWR:
        LD      B,A             ; hold the value
        LD      D,FNRSELP
        LD      E,C
        LD      A,(DE)          ; arm the register
        LD      D,FNRDATP
        LD      E,B
        LD      A,(DE)          ; deliver the value
        RET

; FNTXBYT: append A to the TX stream. Clobbers A,D,E.
FNTXBYT:
        LD      D,FNDATAP
        LD      E,A
        LD      A,(DE)
        RET

; FNPARB: append a 1-byte parameter with value A. Clobbers A,B,D,E.
FNPARB:
        LD      B,A
        LD      A,1             ; size byte
        CALL    FNTXBYT
        LD      A,B
        JP      FNTXBYT

; FNPARW: append a 2-byte little-endian parameter with value HL.
; Clobbers A,B,D,E,H,L.
FNPARW:
        LD      A,2
        CALL    FNTXBYT
        LD      A,L
        CALL    FNTXBYT
        LD      A,H
        JP      FNTXBYT

; FNTXSTR: append the 0-terminated string at HL. Clobbers A,D,E,HL.
FNTXSTR:
        LD      A,(HL)
        OR      A
        RET     Z
        CALL    FNTXBYT
        INC     HL
        JR      FNTXSTR

; FNBEGIN: begin a transaction. A = device, E = command, L = nparam.
; Rewinds the TX pointer; the caller then streams params and payload.
; Clobbers A,B,C,D,E,H,L.
FNBEGIN:
        PUSH    HL
        PUSH    DE
        PUSH    AF
        LD      C,FRDATRS
        XOR     A
        CALL    FNREGWR
        POP     AF
        LD      C,FRDEVIC
        CALL    FNREGWR
        POP     DE
        LD      A,E
        LD      C,FRCMD
        CALL    FNREGWR
        POP     HL
        LD      A,L
        LD      C,FRNPARM
        JP      FNREGWR

; FNCOMMIT: commit and wait for the cart to acknowledge.
; B = timeout in ~2 second quanta.
;
; The next sequence number is derived from the cart's own persisted ACKSEQ + 1,
; wrapping 255 -> 1 with 0 reserved as "never used" -- NEVER from a local
; counter. A console RESET restarts this program and re-zeroes its variables
; but does NOT reset the cart, so a locally derived sequence replays a number
; the cart has already acknowledged: no request goes out, and the stale reply
; still sitting in the window looks exactly like success.
;
; Returns carry set on timeout, else A = FNERR value (0 = ok).
; Clobbers A,B,C,D,E,H,L.
FNCOMMIT:
        LD      A,(FNACKSQ)
        INC     A
        JR      NZ,FNCMS
        INC     A               ; 255 wraps past the reserved 0
FNCMS:  LD      H,A             ; expected ACKSEQ
        LD      C,FRSEQ
        CALL    FNREGWR
FNCMO:  LD      DE,0            ; inner: 65536 polls, ~2 s at 3.58 MHz
FNCMI:  LD      A,(FNACKSQ)
        CP      H
        JR      Z,FNCMOK
        DEC     DE
        LD      A,D
        OR      E
        JR      NZ,FNCMI
        DJNZ    FNCMO
        SCF                     ; timed out
        RET
FNCMOK: LD      A,(FNERR)
        OR      A               ; clears carry
        RET

; FNCHECK: is a FujiNet cartridge underneath us? Z set if the magic is there.
; On a plain game cartridge these addresses hold whatever the game put there,
; or 0FFH. Clobbers A.
FNCHECK:
        LD      A,(FNMAGF)
        CP      'F'
        RET     NZ
        LD      A,(FNMAGN)
        CP      'N'
        RET

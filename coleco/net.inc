; net.inc -- the N: device round trip.
;
; A transcription of the astrocade client's sequence over the ColecoVision
; mailbox. It carries that port's five hard-won rules forward verbatim,
; because every one of them cost somebody a day:
;
;  * The CLOSE belongs at the START of the next request, never after the
;    READ. Every transaction repaints the whole reply window, a CLOSE reply is
;    empty, and every screen here renders straight out of the window between
;    polls -- closing early would wipe the thing being drawn.
;  * FNRXLL/FNRXLH belong to the most recent transaction, so the reply length
;    is captured immediately after the READ.
;  * The ESP32 reports STATUS as soon as SOME of the response has arrived.
;    Poll until two consecutive readings agree, bounded, rather than guessing
;    a delay.
;  * The 4th STATUS reply byte is nDevStatus_t and checking it is NOT
;    optional: the HTTP GET is deferred until the first STATUS, and an HTTP
;    error page still has a perfectly readable body.
;  * Validate the reply length against what playerCount and the round imply
;    before believing a byte of it. The window is never cleared, so a short
;    read leaves stale bytes from a bigger previous reply in place.
;
; Timeout quanta are ~1 second here, not the astrocade's ~2: FNCOMMIT's inner
; loop is a fixed 65536 polls and this Z80 runs at 3.58 MHz rather than 1.79.
; The counts below are doubled to keep the same wall-clock budgets, which
; matters because the cart's own transaction budget is 5 s -- we have to
; outlast it for a real timeout to surface as the cart's error code instead of
; ours.
;
; All routines return carry SET on failure, clear on success.

; NCMD: begin a network-device transaction. E = command, L = nparam.
NCMD:   LD      A,NETDEV
        JP      FNBEGIN

; NCLOSE: close the previous request's connection. A first call has nothing to
; close and earns a NAK; both results are ignored.
NCLOSE: LD      E,NCCLOSE
        LD      L,0
        CALL    NCMD
        LD      B,6
        JP      FNCOMMIT

; NOPEN: OPEN the devicespec for V_URL as HTTP GET, no translation.
NOPEN:  LD      E,NCOPEN
        LD      L,2
        CALL    NCMD
        LD      A,NMHTTPG
        CALL    FNPARB
        LD      A,NTRNONE
        CALL    FNPARB
        CALL    BLDURL
        LD      B,16            ; HTTPS handshakes take a while
        CALL    FNCOMMIT
        RET     C
        JR      NZ,NOBAD
        LD      A,(FNREPLY)
        CP      FCACK
        RET     Z               ; equal: carry is already clear
NOBAD:  SCF
        RET

; NSTATUS: one STATUS poll. On success AVAIL = bytes waiting.
NSTATUS:
        LD      E,NCSTAT
        LD      L,2
        CALL    NCMD
        XOR     A
        CALL    FNPARB
        XOR     A
        CALL    FNPARB
        LD      B,8
        CALL    FNCOMMIT
        RET     C
        JR      NZ,NSBAD
        LD      A,(FNREPLY)
        CP      FCACK
        JR      NZ,NSBAD
        LD      A,(FNRDATA+NSDEVST)
        CP      1               ; SUCCESS. Anything else is an HTTP error
        JR      NZ,NSBAD        ; whose body would read like a game state
        LD      A,(FNRDATA+NSAVLO)
        LD      (AVAIL),A
        LD      A,(FNRDATA+NSAVHI)
        LD      (AVAIL+1),A
        OR      A               ; clears carry
        RET
NSBAD:  SCF
        RET

; NREAD: READ min(AVAIL, GMAXLEN) bytes into the reply window, then capture
; the length. No slice re-homing: the window is one flat 1K block here.
NREAD:  LD      HL,(AVAIL)
        LD      DE,GMAXLEN
        OR      A
        SBC     HL,DE
        JR      C,NRD1          ; avail < clamp: take avail
        LD      H,D
        LD      L,E
        JR      NRD2
NRD1:   ADD     HL,DE
NRD2:   PUSH    HL
        LD      E,NCREAD
        LD      L,1
        CALL    NCMD
        LD      A,2             ; one 2-byte parameter: the length, LE
        CALL    FNTXBYT
        POP     HL
        PUSH    HL
        LD      A,L
        CALL    FNTXBYT
        POP     HL
        LD      A,H
        CALL    FNTXBYT
        LD      B,16
        CALL    FNCOMMIT
        RET     C
        JR      NZ,NRBAD
        LD      A,(FNREPLY)
        CP      FCACK
        JR      NZ,NRBAD
        LD      A,(FNRXLL)      ; capture NOW: the next transaction's reply
        LD      (RXLEN),A       ; length would replace it
        LD      A,(FNRXLH)
        LD      (RXLEN+1),A
        XOR     A               ; clears carry
        RET
NRBAD:  SCF
        RET

; ----------------------------------------------------------------------
; APICALL: A = request id (RQTABLE..RQLEAVE). One full round trip; on success
; (carry clear) the reply sits in the window and stays valid until the next
; APICALL.
APICALL:
        LD      (V_URL),A
        CALL    NCLOSE          ; the PREVIOUS request's connection
        CALL    NOPEN
        RET     C
        XOR     A
        LD      (PRVAVL),A
        LD      (PRVAVL+1),A
        LD      B,20            ; settle-loop bound
APSET:  PUSH    BC
        CALL    NSTATUS
        POP     BC
        JR      C,APCFL
        LD      HL,(AVAIL)
        LD      A,H
        OR      L
        JR      Z,APAGN         ; nothing has arrived at all yet
        LD      DE,(PRVAVL)
        OR      A
        SBC     HL,DE
        JR      Z,APRD          ; two readings agree: it has landed
APAGN:  LD      HL,(AVAIL)
        LD      (PRVAVL),HL
        PUSH    BC
        LD      B,3             ; three frames between polls, counted on
        CALL    PAUSE           ; vblanks rather than a delay loop, so the
        POP     BC              ; cadence does not change with the clock
        DJNZ    APSET
APCFL:  CALL    NCLOSE          ; failed before the READ: nothing to keep
        SCF
        RET
APRD:   CALL    NREAD
        JR      C,APCFL
; A validation failure AFTER the read returns carry without a close: the next
; APICALL's leading NCLOSE retires the connection, and closing here would wipe
; a window the caller may still be rendering the last good reply from.
; Falls through into VALID8.

; VALID8: sanity-check the reply for the request in V_URL.
; Tables: any nonzero length -- the count byte bounds the walk. State: at
; least the fixed header, playerCount <= 12, the round on its enum (0-13 or
; 99), and at least 95 + 42*playerCount bytes.
;
; Minimum, not exact: a longer reply is harmless, but a SHORTER one means the
; fixed offsets would be reading whatever the previous, bigger reply left
; behind.
VALID8: LD      A,(V_URL)
        OR      A
        JR      NZ,VALGM
        LD      HL,(RXLEN)
        LD      A,H
        OR      L
        RET     NZ              ; OR cleared carry
        SCF
        RET
VALGM:  LD      HL,(RXLEN)
        LD      DE,GMINLEN
        OR      A
        SBC     HL,DE
        RET     C               ; shorter than the fixed header
        LD      A,GOPC
        CALL    RXHDR
        CP      PCMAX+1
        JR      NC,VALBAD       ; more seats than the serializer can emit
        LD      C,A             ; C = playerCount
        LD      A,GORND
        CALL    RXHDR
        CP      RNOVER          ; 99 is on the enum...
        JR      Z,VALS2
        CP      RNFINAL+1       ; ...and so is 0-13. The rest is stale bytes
        JR      NC,VALBAD
VALS2:  LD      HL,GMINLEN      ; expected minimum = 95 + 42*playerCount
        LD      DE,PLSTRID
        LD      A,C
        OR      A
        JR      Z,VALS6
        LD      B,C
VALSA:  ADD     HL,DE
        DJNZ    VALSA
VALS6:  EX      DE,HL           ; DE = the expected minimum
        LD      HL,(RXLEN)
        OR      A
        SBC     HL,DE           ; carry set exactly when RXLEN < expected
        RET
VALBAD: SCF
        RET

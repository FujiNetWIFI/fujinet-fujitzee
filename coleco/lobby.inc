; lobby.inc -- choosing a table, and waiting in it.
;
; /tables?bin=1 returns a count byte followed by that many 36-byte records:
; id[9], name[21], players[6] (the literal "cur / max" the server formats).
; As everywhere else, the list renders straight out of the reply window -- the
; only thing copied to RAM is the id of the table actually joined, because
; that one has to survive every later transaction to ride out in the URL.

TBCOUNT EQU     0               ; the count byte
TBFIRST EQU     1               ; ...then the records

TBLTOP  EQU     6               ; first list row
TBLROWS EQU     10              ; the serializer's cap

TTABLES: DB     "PICK A TABLE",0
TTKEYS: DB      "FIRE JOIN   * BACK",0
TNOTBL: DB      "NO TABLES",0
TLOBBY: DB      "WAITING TO START",0
TLKEYS: DB      "FIRE READY   * LEAVE",0
TREADY: DB      "READY",0
TDOTS:  DB      "...",0
TSPECT: DB      "WATCHING",0

; ---- the table list ---------------------------------------------------
TBLSCR: CALL    KEYRAW          ; drain a key still held from the last screen
        OR      A
        JR      NZ,TBLSCR
        XOR     A
        LD      (V_SEL),A
        LD      (PENDACT),A
        LD      A,0FFH          ; the next join starts a fresh board
        LD      (PRVCLS),A
        LD      (PRVACT),A
        LD      (PRVRLS),A
TBLRF:  CALL    DHIDE
        CALL    DCLS
        CALL    DBANKA
        LD      HL,TTABLES
        LD      D,2
        LD      E,10
        CALL    DPRINTA
        LD      HL,TWAIT
        LD      D,TBLTOP
        LD      E,11
        CALL    DPRINT
        CALL    DSHOW           ; blank, draw, THEN reveal -- never the other
                                ; way round; the fetch below takes seconds
        LD      A,RQTABLE
        CALL    APICALL
        JR      C,TBLERR
        LD      A,(FNRDATA+TBCOUNT)
        CP      TBLROWS+1
        JR      C,TBOK2
        LD      A,TBLROWS       ; more than the serializer should have sent
TBOK2:  LD      (V_CNT),A
        OR      A
        JR      Z,TBLNONE
        CALL    TBDRAW
TBKEY:  CALL    KEYSCN
        OR      A
        JR      Z,TBKEY
        CP      'U'
        JR      NZ,TBK1
        LD      A,(V_SEL)
        OR      A
        JR      Z,TBKEY
        DEC     A
TBMOVE: LD      (V_SEL),A
        CALL    SNDKEY
        CALL    TBDRAW
        JR      TBKEY
TBK1:   CP      'D'
        JR      NZ,TBK2
        LD      A,(V_SEL)
        INC     A
        LD      HL,V_CNT
        CP      (HL)
        JR      NC,TBKEY
        JR      TBMOVE
TBK2:   CP      'E'             ; keypad *: back to the name screen
        JR      NZ,TBK2A
        CALL    NAMSCR
        JR      TBLRF
TBK2A:  CP      '0'             ; keypad 0: refresh the list
        JR      Z,TBLRF
        CP      'T'
        JR      NZ,TBK3
        JR      TBJOIN
TBK3:   CP      '1'             ; digits 1-9 jump straight to a row
        JR      C,TBKEY
        CP      '9'+1
        JR      NC,TBKEY
        SUB     '1'
        LD      HL,V_CNT
        CP      (HL)
        JR      NC,TBKEY
        JR      TBMOVE

TBLNONE:
        CALL    DBANKA
        LD      HL,TNOTBL
        LD      D,TBLTOP
        LD      E,11
        CALL    DPRINT
        JR      TBLWK
TBLERR: CALL    DBANKA
        LD      HL,TNOTBL
        LD      D,TBLTOP
        LD      E,11
        CALL    DPRINT
TBLWK:  CALL    KEYSCN
        OR      A
        JR      Z,TBLWK
        JP      TBLRF

; TBJOIN: copy the selected table's id and drop into the poll loop.
TBJOIN: CALL    SNDSEL
        LD      A,(V_SEL)
        CALL    TBOFF           ; HL = the record's reply offset
        LD      DE,TBLBUF
        LD      B,9
        CALL    RXSTRN
        JP      GAMELP

; TBOFF: HL = the reply offset of table record A. Stride 36 = 32 + 4.
; Clobbers A,B,C,D,E.
TBOFF:  LD      L,A
        LD      H,0
        ADD     HL,HL
        ADD     HL,HL           ; 4A
        LD      B,H
        LD      C,L
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL           ; 32A
        ADD     HL,BC           ; 36A
        LD      BC,TBFIRST
        ADD     HL,BC
        RET

; TBDRAW: the list, with the cursor on V_SEL. Names and seat counts come
; straight out of the window. Clobbers everything.
TBDRAW: XOR     A
        LD      (V_TMP),A
TBD1:   LD      A,(V_TMP)
        LD      HL,V_CNT
        CP      (HL)
        JR      NC,TBD9
        LD      A,(V_TMP)
        ADD     A,TBLTOP
        LD      D,A             ; D = the row
        LD      E,0
        PUSH    DE
        LD      A,(V_TMP)       ; the cursor
        LD      HL,V_SEL
        CP      (HL)
        LD      A,DSPACE
        JR      NZ,TBD2
        LD      A,TICURS
TBD2:   CALL    DPUTC
        POP     DE
        INC     E
        PUSH    DE
        LD      A,(V_TMP)       ; "n " -- the digit that jumps here
        ADD     A,'1'
        ADD     A,DBCYAN
        CALL    DPUTC
        POP     DE
        LD      E,3
        PUSH    DE
        LD      A,(V_TMP)
        CALL    TBOFF
        LD      BC,TBNAME
        ADD     HL,BC
        LD      DE,FNRDATA
        ADD     HL,DE
        POP     DE
        CALL    DBANKW
        LD      B,18
        CALL    DPRTN
        LD      A,(V_TMP)       ; the seat count, at the right edge
        ADD     A,TBLTOP
        LD      D,A
        LD      E,22
        PUSH    DE
        LD      A,(V_TMP)
        CALL    TBOFF
        LD      BC,TBPLYRS
        ADD     HL,BC
        LD      DE,FNRDATA
        ADD     HL,DE
        POP     DE
        CALL    DBANKA
        LD      B,8
        CALL    DPRTN
        LD      HL,V_TMP
        INC     (HL)
        JR      TBD1
TBD9:   CALL    DBANKA
        LD      HL,TTKEYS
        LD      D,20
        LD      E,7
        JP      DPRINTA


; ---- the lobby --------------------------------------------------------
; LOBBYR: one lobby render. Every seat with its ready state; scores[0] doubles
; as the ready flag while the round is 0, which is the one place in the wire
; format where a score field means something else.
; Clobbers everything.
LOBBYR: CALL    PRMPT
        CALL    DBANKA
        LD      HL,TLOBBY
        LD      D,2
        LD      E,8
        CALL    DPRINTA
        XOR     A
        LD      (V_SEAT),A
LOB1:   LD      A,(V_SEAT)
        ADD     A,TBLTOP
        LD      D,A
        LD      A,(V_SEAT)
        LD      HL,V_PC
        CP      (HL)
        JR      NC,LOBBLK
        LD      E,4
        PUSH    DE
        LD      A,(V_SEAT)
        CALL    RXPLR
        POP     DE
        CALL    DBANKW
        LD      B,9
        CALL    DPRTN
        LD      A,(V_SEAT)      ; ready? scores[0] carries it in the lobby
        LD      C,0
        CALL    RXSCOR
        CALL    RXSET
        JR      C,LOBSPEC       ; -2: this seat is watching
        LD      A,H
        OR      L
        JR      Z,LOBWAIT
        LD      HL,TREADY
        JR      LOBST
LOBWAIT:
        LD      HL,TDOTS
        JR      LOBST
LOBSPEC:
        LD      HL,TSPECT
LOBST:  LD      A,(V_SEAT)
        ADD     A,TBLTOP
        LD      D,A
        LD      E,16
        CALL    DBANKA
        CALL    DPRINTA
        LD      A,E             ; blank the tail so a shorter state cannot
        LD      B,A             ; leave a longer one's end behind
        LD      A,26
        SUB     B
        LD      B,A
        CALL    DFILL
        JR      LOBNXT
LOBBLK: LD      E,4
        LD      B,24
        CALL    DFILL
LOBNXT: LD      HL,V_SEAT
        INC     (HL)
        LD      A,(HL)
        CP      PCMAX
        JR      NC,LOB9
        CP      13              ; the rows we have
        JR      C,LOB1
LOB9:   CALL    DBANKA
        LD      HL,TLKEYS
        LD      D,20
        LD      E,6
        JP      DPRINTA

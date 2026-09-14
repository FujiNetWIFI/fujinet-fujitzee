; card.inc -- the six-column scorecard.
;
; The geometry is the CoCo 1/2 and MS-DOS board at 32 columns, so every seat's
; card is on screen at once and a bot's move lands where you are already
; looking. Both halves are arithmetic rather than tables:
;
;   seat i's value field   x = SCORESX + 6 + i*4   -- 8, 12, 16, 20, 24, 28
;                          three cells wide, right-aligned at x+2
;   seat i's initial       x + 1, row 1
;   slot j's row           SCOREY[j]
;   column dividers        SCORESX + 5 + i*4       -- 7, 11, 15, 19, 23, 27, 31
;
; Slots 0-13 carry a label; slot 14 -- five of a kind -- is labelled by the
; fujiTZEE LOGO itself, which is what the MS-DOS board does and the reason the
; logo tiles are in the charset at all. Slot 15 is the grand total, which the
; server leaves empty until game over.
;
; Nothing is buffered. Names come straight out of the reply window and numbers
; go straight into cells.

; The screen row of each score slot. Slot 6 and 7 (the upper total and the
; bonus) sit under their own rule, and 15 (the grand total) below the board.
SCOREY: DB      3,4,5,6,7,8,10,11,13,14,15,16,17,18,19,21

; Six bytes per label: five characters and a terminator.
LABELS: DB      "ONE",0,0,0
        DB      "TWO",0,0,0
        DB      "THREE",0
        DB      "FOUR",0,0
        DB      "FIVE",0,0
        DB      "SIX",0,0,0
        DB      "TOTAL",0
        DB      "BONUS",0
        DB      "SET 3",0
        DB      "SET 4",0
        DB      "HOUSE",0
        DB      "S RUN",0
        DB      "L RUN",0
        DB      "COUNT",0

; CCOLX: E = the value column of seat A. Clobbers A.
CCOLX:  ADD     A,A
        ADD     A,A
        ADD     A,SCORESX+6
        LD      E,A
        RET

; CROWY: D = the screen row of slot A. Clobbers A,H,L.
CROWY:  LD      HL,SCOREY
        ADD     A,L
        LD      L,A
        JR      NC,CRY1
        INC     H
CRY1:   LD      D,(HL)
        RET


; ---- the static board -------------------------------------------------
; Drawn once per screen class change. The caller has already blanked the
; display and it stays blanked until the first FULL paint is done -- see
; DSHOWN. Turning it back on here shows a board with no dice, no scores and no
; prompt for the third of a second it takes to fill them in.
; Clobbers everything.
CBOARD: LD      D,2             ; the two thick rules, top and bottom
        CALL    CRULE
        LD      D,20
        CALL    CRULE
        LD      D,9             ; the two thin ones, under the upper section
        CALL    CRULET          ; and under the bonus
        LD      D,12
        CALL    CRULET
        CALL    CDIVS
        JP      CLABEL

; CRULE / CRULET: a thick or thin horizontal rule across the board, row D.
CRULE:  LD      A,TCTHICK
        JR      CRUL1
CRULET: LD      A,TCTHIN
CRUL1:  LD      (V_TMP),A
        LD      E,1
CRUL2:  LD      A,(V_TMP)
        CALL    DPUTC
        INC     E
        LD      A,E
        CP      WIDTH
        JR      C,CRUL2
        RET

; CDIVS: the eight column dividers, rows 1-20, crossing the rules as they go.
; Row 0 is the server's prompt line and belongs to nobody else.
CDIVS:  LD      A,1
        LD      (V_COL),A
CDV1:   LD      A,1
        LD      (V_TMP),A       ; V_TMP = the row
CDV2:   LD      A,(V_TMP)
        LD      D,A
        LD      A,(V_COL)
        LD      E,A
        LD      A,D
        CP      2
        JR      Z,CDVTHK
        CP      20
        JR      Z,CDVTHK
        CP      9
        JR      Z,CDVTHN
        CP      12
        JR      Z,CDVTHN
        LD      A,TCVERT
        JR      CDV3
CDVTHK: LD      A,TCXTHICK
        JR      CDV3
CDVTHN: LD      A,TCXTHIN
CDV3:   CALL    DPUTC
        LD      HL,V_TMP
        INC     (HL)
        LD      A,(HL)
        CP      21
        JR      C,CDV2
        LD      HL,V_COL        ; col 1, then 7, 11, 15, 19, 23, 27, 31
        LD      A,(HL)
        CP      1
        JR      NZ,CDV4
        LD      (HL),7
        JR      CDV1
CDV4:   ADD     A,4
        LD      (HL),A
        CP      WIDTH
        JR      C,CDV1
        RET

; CLABEL: the fourteen row labels, plus the logo that labels the fifteenth.
CLABEL: XOR     A
        LD      (V_CAT),A
CLB1:   LD      A,(V_CAT)
        CALL    CROWY           ; D = row
        LD      A,(V_CAT)       ; the label, six bytes per record
        LD      L,A
        LD      H,0
        ADD     HL,HL
        LD      B,H
        LD      C,L
        ADD     HL,HL
        ADD     HL,BC           ; *6
        LD      BC,LABELS
        ADD     HL,BC
        LD      E,SCORESX
        CALL    DPRINTA
        LD      HL,V_CAT
        INC     (HL)
        LD      A,(HL)
        CP      14
        JR      C,CLB1
        LD      A,14            ; slot 14's label IS the logo
        CALL    CROWY
        LD      E,SCORESX
        LD      HL,CLOGO
        LD      B,LOGOCNT
CLB2:   LD      A,(HL)
        PUSH    HL
        CALL    DTILE
        POP     HL
        INC     HL
        INC     E
        DJNZ    CLB2
        RET

; Five tiles, not six. The label column is cols 2-6 with the first player's
; divider at 7, and the MS-DOS logo's sixth tile is that port's box border --
; see logo_tiles() in tools/mkcharset.py. LOGOCNT comes from the asset so the
; two cannot drift apart.
CLOGO:  DB      TLOGO0,TLOGO1,TLOGO2,TLOGO3,TLOGO4


; ---- seats ------------------------------------------------------------
; CSEATS: the initials row and the turn mark. Six columns' worth; a seat that
; is a spectator, or past playerCount, gets a blank so a shrinking table
; cannot leave someone's initial behind.
; Clobbers everything.
CSEATS: XOR     A
        LD      (V_SEAT),A
CST1:   LD      A,(V_SEAT)
        CALL    CCOLX           ; E = the column
        LD      D,1
        PUSH    DE
        LD      A,DSPACE        ; the turn mark cell, cleared
        CALL    DPUTC
        POP     DE
        INC     E
        PUSH    DE
        LD      A,(V_SEAT)
        LD      HL,V_PC
        CP      (HL)
        JR      NC,CSTBLK       ; past the seat count
        CALL    CSTINI
        JR      CST2
CSTBLK: LD      A,DSPACE
        CALL    DPUTC
CST2:   POP     DE
        LD      HL,V_SEAT
        INC     (HL)
        LD      A,(HL)
        CP      PANMAX
        JR      C,CST1
        ; the turn mark, on the active seat's column
        LD      A,(V_ACT)
        CP      PANMAX
        RET     NC              ; nobody, or a seat with no column
        CALL    CCOLX
        LD      D,1
        LD      A,TIMARK
        JP      DPUTC

; CSTINI: the initial of seat V_SEAT at (D,E) -- the character its own alias
; index picks out of its name, which is how the server marks what makes each
; name unique. Clobbers everything.
CSTINI: PUSH    DE              ; RXPLR clobbers DE, and DPUTC needs it
        LD      A,(V_SEAT)
        CALL    RXPLR           ; HL = the record
        PUSH    HL
        LD      BC,PLALIAS
        ADD     HL,BC
        LD      A,(HL)          ; the alias index
        POP     HL
        CP      9
        JR      C,CSI1
        XOR     A               ; out of range: fall back to the first letter
CSI1:   ADD     A,L
        LD      L,A
        JR      NC,CSI2
        INC     H
CSI2:   LD      A,(HL)
        OR      A
        JR      NZ,CSI3
        LD      A,' '
CSI3:   POP     DE
        CALL    DFOLD           ; a wire byte: the server lowercases everything
        ADD     A,DBWHITE
        JP      DPUTC


; ---- scores -----------------------------------------------------------
; CARDRW: every seat's filled scores, then this seat's potentials, then the
; cursor. Overdraw is opaque -- an unset slot is blank-filled -- so this never
; needs a clear first and can run on every poll.
; Clobbers everything.
CARDRW: XOR     A
        LD      (V_SEAT),A
CRW1:   LD      A,(V_SEAT)
        LD      HL,V_PC
        CP      (HL)
        JR      NC,CRWBLK
        CALL    CRWSEAT
        JR      CRW2
CRWBLK: CALL    CRWCLR          ; a seat that has gone: clear its column
CRW2:   LD      HL,V_SEAT
        INC     (HL)
        LD      A,(HL)
        CP      PANMAX
        JR      C,CRW1
        CALL    POTALL
        JP      CURSDR

; CRWMAX: A = how many slots the render loops should draw.
;
; SCOREY[15], the grand total, is row 21 -- which during play is the TOP ROW OF
; THE DICE PANEL. Drawing it there on every poll blanks the top third of the
; ROLL tile and all five dice, and DICEHUD paints them straight back a
; millisecond later: a flicker once per poll, right where the eye is. The
; shared engine stops at 15 until the game is over for the same reason
; (gamelogic.c:278), and the dice are gone by then.
CRWMAX: LD      A,(V_CLS)
        CP      2
        LD      A,15
        RET     NZ
        LD      A,16
        RET

; CRWSEAT: one seat's slots.
CRWSEAT:
        XOR     A
        LD      (V_CAT),A
CRS1:   LD      A,(V_CAT)
        CALL    CROWY           ; D = row
        PUSH    DE
        LD      A,(V_CAT)
        LD      C,A
        LD      A,(V_SEAT)
        CALL    RXSCOR          ; HL = the raw wire value
        CALL    RXSET
        POP     DE
        JR      C,CRSBLK        ; unset or spectator: blank the field
        PUSH    HL
        LD      A,(V_SEAT)
        CALL    CCOLX           ; E = the column
        CALL    CRWINK          ; white, or accent if this is the reveal
        POP     HL
        LD      C,3
        CALL    NUMFLD
        JR      CRS2
CRSBLK: LD      A,(V_SEAT)
        CALL    CCOLX
        LD      B,3
        CALL    DFILL
CRS2:   LD      HL,V_CAT
        INC     (HL)
        CALL    CRWMAX          ; a LESS-THAN test, not equality: if V_CAT ever
        LD      B,A             ; overshot the limit an equality test would
        LD      A,(V_CAT)       ; never end, and CROWY would read past SCOREY
        CP      B               ; into the label strings -- a row number near
        JR      C,CRS1           ; 80, which DVADDR turns into an address in the
        RET                     ; COLOUR table


; CRWINK: pick the bank this cell draws in. Everything is white except the one
; row a bot just took, which is drawn in the accent so the eye finds it -- see
; REVEAL. Clobbers A,H,L.
CRWINK: LD      A,(REVSLT)
        CP      0FFH
        JR      Z,CRWW
        LD      HL,V_CAT
        CP      (HL)
        JR      NZ,CRWW
        LD      A,(REVPLR)
        LD      HL,V_SEAT
        CP      (HL)
        JR      NZ,CRWW
        JP      DBANKA
CRWW:   JP      DBANKW

; CRWCLR: blank a whole column, initial included.
CRWCLR: XOR     A
        LD      (V_CAT),A
CRC1:   LD      A,(V_CAT)
        CALL    CROWY
        LD      A,(V_SEAT)
        CALL    CCOLX
        LD      B,3
        CALL    DFILL
        LD      HL,V_CAT
        INC     (HL)
        CALL    CRWMAX          ; less-than, not equality -- see CRWSEAT
        LD      B,A
        LD      A,(V_CAT)
        CP      B
        JR      C,CRC1
        RET


; ---- potential scores -------------------------------------------------
; The whole point of validScores[]: the exact points every open row would earn
; for the dice now on the table, in LIGHT GREEN so they read as an offer
; rather than as a score already taken.
;
; Three rules, all from the shared engine:
;   * only rounds 1-13 -- validScores[] is all zeros in the lobby, and drawing
;     it there would paint a column of noise;
;   * only the active seat's column, and only once the roll animation stops;
;   * a potential of exactly 0 is HIDDEN except on the row under the cursor,
;     so an open board is not a wall of zeros before you have decided.
POTALL: LD      A,(V_MYTRN)
        OR      A
        RET     Z
        LD      A,(V_CLS)
        CP      1
        RET     NZ              ; play class only
        LD      A,GODICE        ; no dice on the table yet: nothing to offer
        CALL    RXHDR
        OR      A
        RET     Z
        XOR     A
        LD      (V_CAT),A
POT1:   LD      A,(V_CAT)
        CALL    POTONE
        LD      HL,V_CAT
        INC     (HL)
        LD      A,(HL)
        CP      15
        JR      C,POT1
        RET

; POTONE: slot A's potential. Clobbers everything.
POTONE: LD      (V_CAT),A
        CALL    RXVALID
        CP      0FFH
        RET     Z               ; closed: the row stays as it is
        LD      C,A             ; C = the points
        LD      A,(V_CAT)
        CALL    CROWY
        LD      A,(V_ACT)
        CALL    CCOLX
        LD      A,C
        OR      A
        JR      NZ,POTNUM
        LD      A,(UIMODE)      ; a zero shows only under the SCORE cursor --
        OR      A               ; in dice mode that cursor is not on screen,
        JR      Z,POTHIDE       ; so a lone 0 would look arbitrary
        LD      HL,SCUR
        LD      A,(V_CAT)
        CP      (HL)
        JR      Z,POTZERO
POTHIDE:
        INC     E               ; leave the cursor cell alone
        LD      B,2
        JP      DFILL
POTZERO:
        LD      A,'0'+DBGREEN
        INC     E
        INC     E
        JP      DPUTC
POTNUM: CALL    DBANKG
        LD      L,C
        LD      H,0
        INC     E               ; the cursor keeps the leftmost cell
        LD      C,2
        JP      NUMFLD

; POTCLR: wipe slot A's potential cell. Clobbers everything.
POTCLR: CALL    CROWY
        LD      A,(V_ACT)
        CALL    CCOLX
        INC     E
        LD      B,2
        JP      DFILL


; ---- the score cursor -------------------------------------------------
; CURSDR: the cursor on the row it belongs on, or nothing at all when the
; score cursor is not the active one. Clobbers everything.
CURSDR: LD      A,(UIMODE)
        OR      A
        RET     Z
        LD      A,(V_MYTRN)
        OR      A
        RET     Z
        LD      A,(SCUR)
        ; falls through

; CURSAT: draw the cursor at slot A. Clobbers everything.
CURSAT: CALL    CROWY
        LD      A,(V_ACT)
        CALL    CCOLX
        LD      A,TICURS
        JP      DPUTC


; ---- watching somebody else score -------------------------------------
;
; The wire has no "last move" field: every endpoint returns the same blob and
; nothing in it says which row a seat just took. So the client works it out by
; comparing each seat's filled rows against what they were last poll.
;
; A full shadow would be six seats x sixteen 16-bit scores, 192 bytes. One BIT
; per slot is twelve, and a bit is all the question needs.
;
; SLOTS 6, 7 AND 15 CAN NEVER BE THE ANSWER. The upper total, the bonus and
; the grand total fill as a side effect of the row actually chosen -- slot 6
; the moment ANY upper row is taken -- so without excluding them slot 6 would
; win every time, being the lowest.

; SHADST: restage the shadow from the current reply, for every seat.
; Clobbers everything.
SHADST: XOR     A
        LD      (V_SEAT),A
SHS1:   LD      A,(V_SEAT)
        CALL    SHMASK          ; HL = this seat's filled mask
        LD      A,(V_SEAT)
        CALL    SHADDR          ; DE = its shadow slot
        EX      DE,HL
        LD      (HL),E
        INC     HL
        LD      (HL),D
        LD      HL,V_SEAT
        INC     (HL)
        LD      A,(HL)
        CP      PANMAX
        JR      C,SHS1
        RET

; SHADCL: park every shadow at FFFFH. After a class change or a seat-count
; change the shadow may belong to a different player entirely, so no bit can
; legitimately have turned on -- and a comparison that was never valid must
; not reveal anything. Clobbers A,B,H,L.
SHADCL: LD      HL,SCSHAD
        LD      B,PANMAX*2
SHC1:   LD      (HL),0FFH
        INC     HL
        DJNZ    SHC1
        LD      A,0FFH
        LD      (REVSLT),A
        RET

; SHADDR: DE = the shadow word of seat A. Clobbers A.
SHADDR: ADD     A,A
        LD      E,A
        LD      D,0
        PUSH    HL
        LD      HL,SCSHAD
        ADD     HL,DE
        EX      DE,HL
        POP     HL
        RET

; SHMASK: HL = a bit per filled slot of seat A, bit 0 = slot 0.
; Clobbers everything but the result.
SHMASK: LD      (V_TMP2),A
        LD      HL,0
        LD      (V_MASK),HL     ; the mask under construction -- a WORD, so it
                                ; gets word-sized storage of its own
        XOR     A
        LD      (V_CAT),A
SHM1:   LD      A,(V_CAT)
        LD      C,A
        LD      A,(V_TMP2)
        CALL    RXSCOR
        CALL    RXSET
        JR      C,SHM2          ; unset: leave the bit clear
        LD      A,(V_CAT)
        CALL    SHBIT           ; HL = 1 << slot
        LD      DE,(V_MASK)
        LD      A,H
        OR      D
        LD      H,A
        LD      A,L
        OR      E
        LD      L,A
        LD      (V_MASK),HL
SHM2:   LD      HL,V_CAT
        INC     (HL)
        LD      A,(HL)
        CP      16
        JR      C,SHM1
        LD      HL,(V_MASK)
        RET

; SHBIT: HL = 1 << A, for A up to 15. Clobbers A,B.
SHBIT:  LD      HL,1
        OR      A
        RET     Z
        LD      B,A
SHB1:   ADD     HL,HL
        DJNZ    SHB1
        RET

; SCSCAN: which row did somebody just take?
; Returns carry CLEAR with REVSLT/REVPLR set when exactly that happened, carry
; SET when nothing did. Only the seat that has just finished a turn is looked
; at -- this client can lag several polls behind, and animating every score
; that appeared since would be a slideshow.
; Clobbers everything.
SCSCAN: LD      A,0FFH
        LD      (REVSLT),A
        LD      A,(V_CLS)
        CP      1
        JR      NZ,SCSNO        ; play class only
        LD      A,(V_ACT)       ; the seat before the active one is whoever
        CP      0FFH            ; just moved
        JR      Z,SCSNO
        LD      HL,V_PC
        OR      A
        JR      NZ,SCS1
        LD      A,(HL)          ; wrap: seat 0's predecessor is the last seat
SCS1:   DEC     A
        OR      A
        JR      Z,SCSNO         ; that is US. The engine excludes the local
                                ; seat (we always arrive as index 0), because
                                ; a row you took yourself needs no reveal --
                                ; you were the one who chose it
        CP      PANMAX
        JR      NC,SCSNO        ; off the columned part of the board
        LD      (V_TMP2),A
        LD      (REVPLR),A
        CALL    SHMASK          ; HL = filled now
        PUSH    HL
        LD      A,(V_TMP2)
        CALL    SHADDR
        EX      DE,HL
        LD      E,(HL)
        INC     HL
        LD      D,(HL)          ; DE = filled last poll
        POP     HL
        LD      A,D             ; a shadow of FFFFH is "never compared"
        AND     E
        CP      0FFH
        JR      Z,SCSNO
        LD      A,E             ; newly filled = now AND NOT then
        CPL
        AND     L
        LD      L,A
        LD      A,D
        CPL
        AND     H
        LD      H,A
        LD      B,0             ; walk the bits, lowest slot first
SCS2:   LD      A,B
        CP      16
        JR      NC,SCSNO
        SRL     H               ; HL >>= 1; the old bit 0 lands in carry
        RR      L
        JR      NC,SCS3
        LD      A,B             ; this slot turned on. Is it a real answer?
        CP      SCUPTOT         ; 6, 7 and 15 fill as a side effect of the
        JR      Z,SCS3          ; row actually chosen, so never
        CP      SCBONUS
        JR      Z,SCS3
        CP      SCGRAND
        JR      Z,SCS3
        JR      SCSHIT
SCS3:   INC     B
        JR      SCS2
SCSHIT: LD      A,B
        LD      (REVSLT),A
        OR      A               ; clears carry
        RET
SCSNO:  LD      A,0FFH
        LD      (REVSLT),A
        SCF
        RET

; REVEAL: show the move SCSCAN found. The row is already drawn in the accent
; by CARDRW (CRWINK), so this is a cue and a hold, not an animation -- point
; at it, say so, and give it long enough to read before play moves on.
;
; The shape and the timings are the shared engine's arrow (gamelogic.c:305-315)
; and add up to about a second, with REVHOLD on top.
; Clobbers everything.
REVEAL: LD      A,(REVSLT)
        CP      0FFH
        RET     Z
        LD      C,A
        LD      A,(REVPLR)
        CALL    CCOLX           ; E = that seat's column
        LD      A,C
        CALL    CROWY           ; D = the row
        PUSH    DE
        LD      A,TICURS
        CALL    DPUTC
        LD      B,25
        CALL    PAUSE
        POP     DE
        PUSH    DE
        LD      A,TICURSA
        CALL    DPUTC
        CALL    SNDBOT
        LD      B,5
        CALL    PAUSE
        POP     DE
        PUSH    DE
        LD      A,TICURS
        CALL    DPUTC
        LD      B,REVHOLD
        CALL    PAUSE
        POP     DE
        LD      A,DSPACE
        CALL    DPUTC
        LD      A,0FFH          ; disarm: the accent goes away on the next draw
        LD      (REVSLT),A
        RET

; dice.inc -- the five dice, the ROLL tile, and the rattle.
;
; A die is nine cells, three by three, drawn from DICETAB -- the MS-DOS port's
; own composition table, re-indexed by tools/mkcharset.py into this charset's
; codes. Its banks:
;
;   entry  0-5    the six faces, WHITE body, black pips
;   entry  6-11   the six faces HELD: the clamped art, in YELLOW
;   entry 12      blank
;   entry 13-15   the ROLL tile: one roll left, two, and dead
;   entry 16-21   the six faces in cyan -- a fujitzee, all five alike
;   entry 22-27   held and fujitzee at once
;
; Held dice are yellow because that is the one colour rule this port was asked
; for by name, and it costs nothing: the pattern is shared with the white bank
; and only the colour table entry differs.
;
; THE WIRE'S "KEEP" FLAG NAMES THE ROLL, NOT THE DIE. keepRoll[i] == '1' means
; that die will be RE-ROLLED; '0' means it is being kept. Getting this
; backwards paints every die the wrong colour and re-rolls the wrong ones.

DIEBLNK EQU     12              ; the blank entry
DIEHELD EQU     6               ; + face-1
DIEFUJI EQU     16              ; + face-1
DIEHFUJ EQU     22              ; + face-1
ROLL1   EQU     13
ROLL2   EQU     14
ROLLDED EQU     15

; DICEHUD: repaint the whole bottom panel -- the ROLL tile, five dice, and the
; cursor bars. 45 name-table bytes for the dice, 9 for the tile, 2 for the
; cursor: well inside a frame, so this never blanks the display.
; Clobbers everything.
DICEHUD:
        CALL    ROLLTIL
        CALL    ROLLNO
        XOR     A
        LD      (V_TMP2),A
DHD1:   LD      A,(V_TMP2)
        CALL    DIEENT          ; A = the DICETAB entry for die V_TMP2
        PUSH    AF
        LD      A,(V_TMP2)
        CALL    DIEX            ; E = its column
        LD      D,BOTTOMY
        POP     AF
        CALL    DDIE
        LD      HL,V_TMP2
        INC     (HL)
        LD      A,(HL)
        CP      5
        JR      C,DHD1
        JP      DCURSOR

; ROLLNO: how many rolls are left, in the bottom-left corner -- five columns
; by three rows that nothing else uses. The ROLL tile encodes the same number
; as a glyph, but only on your own turn and only if you already know to read
; it; this says it in words, and says it for whoever is rolling.
; Clobbers everything.
ROLLNO: LD      A,(V_CLS)
        CP      1
        JR      NZ,RNOCLR
        LD      D,BOTTOMY
        LD      E,0
        LD      HL,TROLLS
        CALL    DPRINTA         ; sets the accent bank for NUMFLD below
        LD      A,GORLEFT
        CALL    RXHDR
        LD      L,A
        LD      H,0
        LD      D,BOTTOMY+1
        LD      E,0
        LD      C,3             ; centred under the label
        JP      NUMFLD
RNOCLR: LD      D,BOTTOMY
        LD      E,0
        LD      B,5
        CALL    DFILL
        LD      D,BOTTOMY+1
        LD      E,0
        LD      B,5
        JP      DFILL

TROLLS: DB      "ROLLS",0

; DIEX: E = the screen column of die A. Clobbers A.
DIEX:   ADD     A,A
        ADD     A,A
        ADD     A,DICEX
        LD      E,A
        RET

; DIEENT: A = the DICETAB entry for die A -- its face, its held state, and
; whether the whole hand is a fujitzee. Clobbers B,C,D,E,H,L.
DIEENT: LD      (V_TMP),A
        CALL    RXDIE           ; ascii face, or 0 before the first roll
        OR      A
        JR      Z,DIEBLK
        SUB     '1'
        CP      6
        JR      NC,DIEBLK       ; not a face the wire should ever send
        LD      C,A             ; C = face-1
        CALL    DIEKEEP
        CP      '0'             ; '0' = being kept
        LD      B,0
        JR      NZ,DIE1
        LD      B,DIEHELD
DIE1:   PUSH    BC              ; ISFUJI walks the hand with B and C of its
        CALL    ISFUJI          ; own; without this it eats the bank and the
        POP     BC              ; face being accumulated here, and DDIE then
        JR      NZ,DIE2         ; indexes off the end of DICETAB
        LD      A,B             ; a fujitzee moves both banks along
        ADD     A,DIEFUJI
        LD      B,A
DIE2:   LD      A,B
        ADD     A,C
        RET
DIEBLK: LD      A,DIEBLNK
        RET

; DIEKEEP: A = the keep flag of the die whose index is in V_TMP -- '0' held,
; '1' will be re-rolled.
;
; ON YOUR TURN THIS IS THE LOCALLY STAGED MASK, not the wire's echo. The echo
; only changes when a roll lands, so reading it meant a die you had just held
; looked exactly like one you had not, right up until the next roll went out --
; the hold had no visible effect at the moment you made it, which is the moment
; it needs one.
;
; Off your turn there is no local intent to show and the echo is the right
; answer: it is what that player actually re-rolled.
; Preserves B and C. Clobbers A,H,L.
DIEKEEP:
        LD      A,(V_MYTRN)
        OR      A
        LD      A,(V_TMP)       ; LD does not disturb the flags
        JR      Z,DKWIRE
        LD      HL,KEEPBF
        ADD     A,L
        LD      L,A
        JR      NC,DKLOC
        INC     H
DKLOC:  LD      A,(HL)
        RET
DKWIRE: JP      RXKEEP

; ISFUJI: Z when all five dice show the same face and a face exists.
; Clobbers A,B,C,D,E,H,L.
ISFUJI: XOR     A
        CALL    RXDIE           ; die 0 is the reference face
        OR      A
        JR      Z,ISFNO         ; no dice on the table yet
        LD      C,A
        LD      B,1
ISF1:   LD      A,B
        CALL    RXDIE           ; clobbers D,E,H,L; B and C survive
        CP      C
        JR      NZ,ISFNO
        INC     B
        LD      A,B
        CP      5
        JR      C,ISF1
        XOR     A               ; all five alike
        RET
ISFNO:  LD      A,1
        OR      A               ; NZ
        RET

; ROLLTIL: the ROLL tile, whose face is how many rolls are left. Zero rolls
; draws the dead variant rather than a blank, so a trigger that lands there
; has something to have bounced off.
; Clobbers everything.
ROLLTIL:
        LD      A,(V_MYTRN)
        OR      A
        JR      Z,RTBLANK
        LD      A,GORLEFT
        CALL    RXHDR
        OR      A
        JR      Z,RTDEAD
        CP      1
        JR      Z,RT1
        LD      A,ROLL2
        JR      RTDRAW
RT1:    LD      A,ROLL1
        JR      RTDRAW
RTDEAD: LD      A,ROLLDED
        JR      RTDRAW
RTBLANK:
        LD      A,DIEBLNK
RTDRAW: LD      E,ROLLX
        LD      D,BOTTOMY
        JP      DDIE

; DCURSOR: the dice cursor -- two bars flanking the selected die. The MS-DOS
; port draws a 30x30 pixel frame around it, which a character map cannot do;
; bars either side read as clearly and cost two cells.
;
; ONE PASS, every cell written exactly once. Clearing the whole bar row and
; then drawing the bar leaves a gap the VDP is free to display in between --
; another once-per-poll flicker, at the cursor this time.
; Clobbers everything.
DCURSOR:
        LD      A,0FFH          ; which position wears the bars, if any
        LD      (V_TMP),A
        LD      A,(V_MYTRN)
        OR      A
        JR      Z,DCU0
        LD      A,(UIMODE)
        OR      A
        JR      NZ,DCU0         ; the score cursor has it
        LD      A,(DCUR)
        LD      (V_TMP),A
DCU0:   XOR     A
        LD      (V_TMP2),A      ; 0 = the ROLL tile, 1-5 = the dice
DCU1:   LD      A,(V_TMP2)
        OR      A
        JR      NZ,DCU2
        LD      E,ROLLX
        JR      DCU3
DCU2:   DEC     A
        CALL    DIEX
DCU3:   LD      D,BOTTOMY+1
        DEC     E               ; the left bar
        CALL    DCUCELL
        LD      A,TCCURL
        JR      Z,DCU4
        LD      A,DSPACE
DCU4:   PUSH    DE
        CALL    DPUTC
        POP     DE
        LD      A,E             ; ...and the right one, four cells along
        ADD     A,4
        LD      E,A
        CALL    DCUCELL
        LD      A,TCCURR
        JR      Z,DCU5
        LD      A,DSPACE
DCU5:   CALL    DPUTC
        LD      HL,V_TMP2
        INC     (HL)
        LD      A,(HL)
        CP      6
        JR      C,DCU1
        RET

; DCUCELL: Z when the position being drawn is the one under the cursor.
; Preserves DE. Clobbers A,H,L.
DCUCELL:
        LD      A,(V_TMP2)
        LD      HL,V_TMP
        CP      (HL)
        RET


; ---- the rattle -------------------------------------------------------
; ROLLANM: eleven frames of a roll landing.
;
; Only the dice the wire's keepRoll echo says were RE-ROLLED rattle, so the
; animation is right for everyone's rolls and not just yours -- a held die
; that sat still stays still on screen too.
;
; Runs after a poll, before the render that shows the real hand, so the last
; thing on screen is the truth.
; Clobbers everything.
ROLLANM:
        LD      A,ROLLFRM
        LD      (V_ANM),A
RAN1:   XOR     A
        LD      (V_TMP2),A
RAN2:   LD      A,(V_TMP2)
        CALL    RXKEEP
        CP      '1'             ; '1' = this one is being re-rolled
        JR      NZ,RAN3
        CALL    RANDFC          ; A = a random entry 0-5
        PUSH    AF
        LD      A,(V_TMP2)
        CALL    DIEX
        LD      D,BOTTOMY
        POP     AF
        CALL    DDIE
RAN3:   LD      HL,V_TMP2
        INC     (HL)
        LD      A,(HL)
        CP      5
        JR      C,RAN2
        CALL    RANDFC          ; rattle: a fresh noise pitch each frame
        ADD     A,A
        ADD     A,A
        ADD     A,10H
        CALL    SNDROLL
        LD      B,RANWAIT       ; ITS OWN pacing. The delay used to live
        CALL    PAUSE           ; entirely inside SNDROLL, which returns at
                                ; once when the sound is muted -- so muting it
                                ; ran the whole animation in microseconds
        LD      HL,V_ANM
        DEC     (HL)
        JR      NZ,RAN1
        JP      SNDRLOF

; RANDFC: A = a random die face entry, 0-5, from OS7's own generator.
;
; Guarded against the zero fixed point: if RANDNUM ever reaches zero the
; generator can never leave it again, and every die comes up the same face.
; Clobbers B,C,D,E,H,L.
RANDFC: CALL    RANDGEN
        LD      HL,(RANDNUM)
        LD      A,H
        OR      L
        JR      NZ,RFC0
        LD      A,(FRAMES)
        OR      41H             ; anything, so long as it is never zero
        LD      L,A
        LD      H,5FH
        LD      (RANDNUM),HL
RFC0:   LD      A,H
        XOR     L
        AND     7
        CP      6
        JR      C,RFC1
        SUB     6
RFC1:   RET

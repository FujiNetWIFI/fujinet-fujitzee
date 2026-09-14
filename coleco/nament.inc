; nament.inc -- entering your name with a joystick.
;
; The ColecoVision has twelve keys and no letters among them, so the name is
; typed off an on-screen grid: the stick walks it, the trigger types the cell,
; keypad * backspaces and keypad # accepts. Two to eight characters, which is
; what the server takes.
;
; CANCEL IS THE * KEY AND NOTHING ELSE. Backspace must never discard the whole
; edit, because reaching for it repeatedly is exactly when a reflexive extra
; press happens -- the rule the Intellivision and Astrocade CONFIG clients both
; arrived at independently.

NAMCOLS EQU     9               ; the grid is 9 x 4
NAMROWS EQU     4
NAMX    EQU     6               ; cell (c,r) at column NAMX + c*2
NAMY    EQU     10              ; ...and row NAMY + r*2
NAMMAX  EQU     8
NAMMIN  EQU     2

TNAME:  DB      "YOUR NAME",0
TNKEYS: DB      "FIRE TYPE  * DEL  # OK",0
TNSHORT: DB     "AT LEAST TWO LETTERS",0
TDEFNAM: DB     "PLAYER",0

; The grid, row by row. 36 cells: A-Z then 0-9.
NAMGRID: DB     "ABCDEFGHI"
        DB      "JKLMNOPQR"
        DB      "STUVWXYZ0"
        DB      "123456789"

; NAMSCR: run the editor. On return PLNBUF holds the name.
; Clobbers everything.
NAMSCR: LD      A,(PLNBUF)      ; keep whatever we already had; first time
        OR      A               ; through, start from a default rather than
        JR      NZ,NAMS1        ; an empty field
        LD      HL,TDEFNAM
        LD      DE,PLNBUF
        CALL    NAMCPY
NAMS1:  LD      HL,PLNBUF       ; the editor works on its own copy
        LD      DE,NAMEED
        CALL    NAMCPY
        LD      HL,NAMEED
        CALL    NAMLEN
        LD      (V_NCUR),A
        XOR     A
        LD      (V_CAT),A       ; the grid cursor
        CALL    DHIDE
        CALL    DCLS
        CALL    NAMSTA
        CALL    NAMDRW
        CALL    DSHOW
NAMK:   CALL    KEYSCN
        OR      A
        JR      Z,NAMK
        CP      'L'
        JR      NZ,NAMK1
        LD      A,(V_CAT)
        OR      A
        JR      Z,NAMK
        DEC     A
NAMMV:  LD      (V_CAT),A
        CALL    SNDKEY
        CALL    NAMDRW
        JR      NAMK
NAMK1:  CP      'R'
        JR      NZ,NAMK2
        LD      A,(V_CAT)
        INC     A
        CP      NAMCOLS*NAMROWS
        JR      NC,NAMK
        JR      NAMMV
NAMK2:  CP      'U'
        JR      NZ,NAMK3
        LD      A,(V_CAT)
        CP      NAMCOLS
        JR      C,NAMK
        SUB     NAMCOLS
        JR      NAMMV
NAMK3:  CP      'D'
        JR      NZ,NAMK4
        LD      A,(V_CAT)
        ADD     A,NAMCOLS
        CP      NAMCOLS*NAMROWS
        JR      NC,NAMK
        JR      NAMMV
NAMK4:  CP      'T'
        JR      NZ,NAMK5
        LD      A,(V_NCUR)      ; type the cell under the cursor
        CP      NAMMAX
        JP      NC,NAMBAD
        LD      HL,NAMGRID
        LD      A,(V_CAT)
        ADD     A,L
        LD      L,A
        JR      NC,NAMT1
        INC     H
NAMT1:  LD      A,(HL)
        LD      HL,NAMEED
        LD      C,A
        LD      A,(V_NCUR)
        ADD     A,L
        LD      L,A
        JR      NC,NAMT2
        INC     H
NAMT2:  LD      (HL),C
        INC     HL
        LD      (HL),0
        LD      HL,V_NCUR
        INC     (HL)
        CALL    SNDHLD
        CALL    NAMDRW
        JR      NAMK
NAMK5:  CP      'E'             ; keypad *: backspace, and ONLY backspace
        JR      NZ,NAMK6
        LD      A,(V_NCUR)
        OR      A
        JR      Z,NAMBAD
        DEC     A
        LD      (V_NCUR),A
        LD      HL,NAMEED
        ADD     A,L
        LD      L,A
        JR      NC,NAMB1
        INC     H
NAMB1:  LD      (HL),0
        CALL    SNDKEY
        CALL    NAMDRW
        JP      NAMK
NAMK6:  CP      '.'             ; keypad #: accept
        JR      NZ,NAMK7
        LD      A,(V_NCUR)
        CP      NAMMIN
        JR      C,NAMSHRT
        LD      HL,NAMEED
        LD      DE,PLNBUF
        CALL    NAMCPY
        JP      SNDSEL
NAMK7:  CP      '0'             ; the keypad digits type themselves
        JP      C,NAMK
        CP      '9'+1
        JP      NC,NAMK
        LD      C,A
        LD      A,(V_NCUR)
        CP      NAMMAX
        JR      NC,NAMBAD
        LD      HL,NAMEED
        ADD     A,L
        LD      L,A
        JR      NC,NAMD1
        INC     H
NAMD1:  LD      (HL),C
        INC     HL
        LD      (HL),0
        LD      HL,V_NCUR
        INC     (HL)
        CALL    SNDHLD
        CALL    NAMDRW
        JP      NAMK
NAMBAD: CALL    SNDBAD
        JP      NAMK
NAMSHRT:
        CALL    SNDBAD
        CALL    DBANKA
        LD      HL,TNSHORT
        LD      D,20
        LD      E,6
        CALL    DPRINTA
        LD      B,90
        CALL    PAUSE
        CALL    NAMSTA
        JP      NAMK

; NAMSTA: the parts of the screen that do not change.
NAMSTA: CALL    DBANKA
        LD      HL,TNAME
        LD      D,2
        LD      E,11
        CALL    DPRINTA
        LD      D,20
        LD      E,5
        LD      B,22
        CALL    DFILL
        LD      HL,TNKEYS
        LD      D,20
        LD      E,5
        JP      DPRINTA

; NAMDRW: the edit field and the grid, cursor included.
; Clobbers everything.
NAMDRW: LD      D,5             ; the name so far, with an underscore caret
        LD      E,12
        LD      HL,NAMEED
        CALL    DBANKW
        LD      B,NAMMAX
        CALL    DPRTN
        LD      A,(V_NCUR)
        CP      NAMMAX
        JR      NC,NAMDR1
        ADD     A,12
        LD      E,A
        LD      D,5
        LD      A,'_'+DBCYAN
        CALL    DPUTC
NAMDR1: XOR     A
        LD      (V_TMP),A
NAMDR2: LD      A,(V_TMP)
        CP      NAMCOLS*NAMROWS
        RET     NC
        LD      C,A
        ; row = NAMY + (i / NAMCOLS)*2, col = NAMX + (i mod NAMCOLS)*2
        LD      B,0
NAMDR3: LD      A,C
        CP      NAMCOLS
        JR      C,NAMDR4
        SUB     NAMCOLS
        LD      C,A
        INC     B
        JR      NAMDR3
NAMDR4: LD      A,B
        ADD     A,A
        ADD     A,NAMY
        LD      D,A
        LD      A,C
        ADD     A,A
        ADD     A,NAMX
        LD      E,A
        LD      HL,NAMGRID      ; the letter, accented under the cursor
        LD      A,(V_TMP)
        ADD     A,L
        LD      L,A
        JR      NC,NAMDR5
        INC     H
NAMDR5: LD      A,(V_TMP)
        LD      HL,V_CAT
        CP      (HL)
        LD      C,DBWHITE
        JR      NZ,NAMDR6
        LD      C,DBCYAN
NAMDR6: LD      HL,NAMGRID
        LD      A,(V_TMP)
        ADD     A,L
        LD      L,A
        JR      NC,NAMDR7
        INC     H
NAMDR7: LD      A,(HL)
        ADD     A,C
        CALL    DPUTC
        LD      HL,V_TMP
        INC     (HL)
        JR      NAMDR2

; NAMCPY: copy the NUL-terminated string at HL to DE, at most NAMMAX+1 bytes.
; Clobbers A,B,D,E,H,L.
NAMCPY: LD      B,NAMMAX
NCP1:   LD      A,(HL)
        LD      (DE),A
        OR      A
        RET     Z
        INC     HL
        INC     DE
        DJNZ    NCP1
        XOR     A
        LD      (DE),A
        RET

; NAMLEN: A = the length of the NUL-terminated string at HL.
; Clobbers A,B,H,L.
NAMLEN: LD      B,0
NLN1:   LD      A,(HL)
        OR      A
        JR      Z,NLN2
        INC     HL
        INC     B
        LD      A,B
        CP      NAMMAX
        JR      C,NLN1
NLN2:   LD      A,B
        RET

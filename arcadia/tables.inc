; tables.inc -- lookup tables and string literals parked in BLOCK 2 for
; the ROM budget.
;
; Block 1 is the tight block -- everything that touches the screen has
; to live there -- and these are inert bytes. It is legal because every
; consumer reaches them INDIRECTLY: DPRINT/DPRTN through DSTRP, DXLAT
; through PXLAT, input.inc through PKT0/1/2, dice.inc through PFACE.
; Indirect operands carry the full 15 bits; the direct indexed forms
; they started as cannot leave their own 8K page.
;
; The two literals that CANNOT move are DEFNAM and WHEEL, both read with
; direct indexed loads (LODA,R0 WHEEL,R3); they stay in block 1.

; ---- ASCII $20-$7F -> glyph, 96 entries; no UDC codes ------------------
; $38-$3B are the cursor chevron and the dice pips, so a stray '-' or
; ':' in a server string must never translate to one of them.
XLAT:   DB      $00,$34,$34,$34,$37,$34,$34,$34 ; spc ! " # $ % & '
        DB      $34,$34,$34,$36,$35,$06,$34,GDIAG ; ( ) * + , - . /
        DB      $10,$11,$12,$13,$14,$15,$16,$17 ; 0-7
        DB      $18,$19,$34,$34,$34,$34,$34,$34 ; 8 9 : ; < = > ?
        DB      $34,$1A,$1B,$1C,$1D,$1E,$1F,$20 ; @ A-G
        DB      $21,$22,$23,$24,$25,$26,$27,$28 ; H-O
        DB      $29,$2A,$2B,$2C,$2D,$2E,$2F,$30 ; P-W
        DB      $31,$32,$33,$34,$34,$34,$34,$06 ; X Y Z [ \ ] ^ _
        DB      $34,$1A,$1B,$1C,$1D,$1E,$1F,$20 ; ` a-g
        DB      $21,$22,$23,$24,$25,$26,$27,$28 ; h-o
        DB      $29,$2A,$2B,$2C,$2D,$2E,$2F,$30 ; p-w
        DB      $31,$32,$33,$34,$34,$34,$34,$34 ; x y z { | } ~ del

; nibble -> key code, highest bit wins (top-row keys take precedence) so
; a mashed pair cannot alias to a third key.
KT0:    DB      KNONE,KCLR,7,7,4,4,4,4,1,1,1,1,1,1,1,1
KT1:    DB      KNONE,0,8,8,5,5,5,5,2,2,2,2,2,2,2,2
KT2:    DB      KNONE,KENT,9,9,6,6,6,6,3,3,3,3,3,3,3,3

; ---- the six die faces -------------------------------------------------
; Six cells each, row-major over the die's 2 wide by 3 tall grid. S is
; the built-in solid block, P a cell with one pip punched out, and L/R
; the two halves of the centre pip -- which is the only pip that
; straddles the cell boundary. See tools/mkudc.py.
;
;      1        2        3        4        5        6
;     S S      P S      P S      P P      P P      P P
;     L R      S S      L R      S S      L R      P P
;     S S      S P      S P      P P      P P      P P
FACETAB:
        DB      GBLOCK,GBLOCK, GPIPCL,GPIPCR, GBLOCK,GBLOCK
        DB      GPIP,  GBLOCK, GBLOCK,GBLOCK, GBLOCK,GPIP
        DB      GPIP,  GBLOCK, GPIPCL,GPIPCR, GBLOCK,GPIP
        DB      GPIP,  GPIP,   GBLOCK,GBLOCK, GPIP,  GPIP
        DB      GPIP,  GPIP,   GPIPCL,GPIPCR, GPIP,  GPIP
        DB      GPIP,  GPIP,   GPIP,  GPIP,   GPIP,  GPIP

; ---- the sixteen scorecard labels, four NUL-padded bytes each ----------
; In slot order: 0-5 the upper rows, 6 the upper total, 7 the bonus,
; 8-14 the lower rows, 15 the running total. CARDST indexes this by
; slot*4 and prints it with DPRTN, so the padding blanks the field.
CATLBL: DB      "ONE",0,"TWO",0,"THR",0,"FOU",0
        DB      "FIV",0,"SIX",0,"UP",0,0,"BON",0
        DB      "SET3","SET4","HOUS","SRUN"
        DB      "LRUN","CNT",0,"FUJI","TOT",0

; ---- every literal the screens print -----------------------------------
SNOFN:  DB      "NO FUJINET",0
TTITLE: DB      "FUJITZEE",0
TNAME:  DB      "YOUR NAME",0
TEDIT:  DB      "DISC SPINS 4 6",0
TACCPT: DB      "ENTER ACCEPTS",0
TCHOOS: DB      "CHOOSE A TABLE",0
TJOINK: DB      "ENTER JOINS",0
TBACKK: DB      "CLEAR RENAMES",0
TENET:  DB      "NET ERR - RETRY",0
TJOIN:  DB      "JOINING...",0
TLOBBY: DB      "LOBBY",0
TREADY: DB      " READY",0
TWAIT:  DB      "     .",0
TSPECT: DB      " WATCH",0
TFREADY: DB     "FIRE TO READY",0
TYTURN: DB      "YOUR TURN",0
TOVER:  DB      "GAME OVER",0
TOVER6: DB      "  OVER",0
TROLL:  DB      "ROLL",0
TMORE:  DB      " MORE",0
TBLANK: DB      0

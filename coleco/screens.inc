; screens.inc -- the poll loop and the screens either side of it.
;
; MAIN runs three blocking screens -- splash, name, table -- and then GAMELP,
; which is the whole rest of the program: poll, render, take input for a
; bounded slice, poll again.
;
; The astrocade client's loop, transcribed, with three ColecoVision additions:
; the bot-move reveal (which that port does not have and arcadia does), the
; not-your-turn holds from the shared C engine, and a clock that counts real
; vblanks instead of a calibrated delay.

; ---- text -------------------------------------------------------------
TFUJI:  DB      "FUJITZEE",0
TPRESS: DB      "PRESS FIRE",0
TWAIT:  DB      "CONNECTING",0
TNOCART: DB     "NO FUJINET CARTRIDGE",0
TROUND: DB      "R",0
TOF13:  DB      "/13",0
TGAMEOV: DB     "GAME OVER",0
TSCORE: DB      "SCORE",0
TTIMOUT: DB     "YOU TIMED OUT. AUTO SCORING.",0
TMENU1: DB      "T RESUME",0
TMENU2: DB      "1 SOUND",0
TMENU3: DB      "2 LEAVE TABLE",0
TON:    DB      "ON ",0
TOFF:   DB      "OFF",0
TNUL:   DB      0


; ---- MAIN -------------------------------------------------------------
MAIN:   CALL    SPLASH
        CALL    NAMSCR
        JP      TBLSCR

; SPLASH: the fujiTZEE logo, and the one check worth making before anything
; else -- that there is a FujiNet cartridge underneath us at all. On a plain
; cartridge the magic bytes hold whatever the game put there, so a client that
; assumes it is talking to a mailbox just hangs on its first poll.
; Clobbers everything.
SPLASH: CALL    DHIDE
        CALL    DCLS
        LD      D,9
        LD      E,(WIDTH-LOGOCNT)/2
        LD      HL,CLOGO
        LD      B,LOGOCNT
SPL1:   LD      A,(HL)
        PUSH    HL
        CALL    DTILE
        POP     HL
        INC     HL
        INC     E
        DJNZ    SPL1
        LD      D,11
        LD      E,12
        LD      HL,TFUJI
        CALL    DPRINTA
        CALL    DSHOW
        CALL    FNCHECK
        JR      Z,SPL2
        LD      D,14            ; no cart: say so and stop. Nothing else this
        LD      E,6             ; program can do will work
        LD      HL,TNOCART
        CALL    DPRINT
SPLHNG: JR      SPLHNG
SPL2:   LD      D,14
        LD      E,11
        LD      HL,TPRESS
        CALL    DPRINT
SPL3:   LD      HL,(RANDNUM)    ; stir while we wait. How long somebody takes
        INC     HL              ; to reach for the trigger is the only real
        LD      (RANDNUM),HL    ; entropy this machine has
        CALL    KEYSCN
        CP      'T'
        JR      NZ,SPL3
        JP      SNDSEL


; ---- the poll loop ----------------------------------------------------
GAMELP: LD      A,(PENDACT)     ; a staged action rides the next request:
        INC     A               ; 0 none -> /state, 1 -> /ready,
        CALL    APICALL         ; 2 -> /roll, 3 -> /score
        JP      C,GLFAIL
        XOR     A
        LD      (PENDACT),A     ; consumed
        LD      (FAILCNT),A
        LD      A,GOACTPL       ; this poll's header, cached
        CALL    RXHDR
        LD      (V_ACT),A
        LD      HL,PRVACT       ; THE TURN EDGE, computed on EVERY good poll,
        CP      (HL)            ; lobby included: activePlayer stays 0 for
        LD      (HL),A          ; every poll of your turn, so this is the only
        LD      A,0             ; place the arrival can be seen
        JR      Z,GLTE1
        INC     A
GLTE1:  LD      (V_TRNF),A
        LD      A,GOPC
        CALL    RXHDR
        LD      (V_PC),A
        LD      A,GORND
        CALL    RXHDR
        LD      (V_RND),A
        XOR     A               ; my turn = active seat 0, and not a spectator
        LD      (V_MYTRN),A
        LD      A,(V_ACT)
        OR      A
        JR      NZ,GLMT1
        LD      A,GOVIEW
        CALL    RXHDR
        LD      (V_VIEW),A
        OR      A
        JR      NZ,GLMT1
        LD      A,1
        LD      (V_MYTRN),A
GLMT1:  CALL    CHKNEW
        LD      A,(V_CLS)       ; dispatch by class
        OR      A
        JP      Z,GLLOB
        CP      1
        JP      Z,GLGAM
        CALL    GOVERR          ; class 2: game over
        CALL    DSHOWN
        LD      B,240           ; the shared engine's four-second hold
        CALL    PAUSE
        LD      B,60
        JP      GLWAIT

GLLOB:  CALL    LOBBYR
        CALL    DSHOWN
        LD      B,120           ; ~2 s between lobby polls
        JP      GLWAIT

; ---- a play-class poll ------------------------------------------------
GLGAM:  LD      A,GORLEFT       ; THE ROLL EDGE IS A DECREASE. A fresh turn
        CALL    RXHDR           ; resets rollsLeft upward, so only a decrease
        LD      (V_ST),A        ; means a roll actually landed
        LD      (V_RLS),A
        LD      A,(PRVRLS)
        CP      0FFH            ; the sentinel mutes it across a boundary
        JR      Z,GLGR3
        LD      HL,V_ST
        CP      (HL)
        JR      Z,GLGR3         ; unchanged
        JR      C,GLGR3         ; increased: a fresh turn, not a roll
        CALL    ROLLANM         ; a roll landed: rattle the dice
        LD      A,(V_MYTRN)
        OR      A
        JR      Z,GLGR3
        CALL    KEEPSYN         ; restage the mask the server echoed
        LD      A,(V_ST)
        OR      A               ; out of rolls: jump to the best row
        JR      NZ,GLGR3
        LD      A,1
        LD      (UIMODE),A
        CALL    PICKBEST
GLGR3:  LD      A,(V_ST)
        LD      (PRVRLS),A
        LD      A,(V_TRNF)      ; the turn edge
        OR      A
        JR      Z,GLGT2
        LD      A,(V_MYTRN)
        OR      A
        JR      Z,GLGT1
        XOR     A               ; my turn arrived: dice mode, cursor on the
        LD      (UIMODE),A      ; ROLL tile, my own card
        LD      (DCUR),A
        LD      (VIEWPL),A
        LD      A,GODICE        ; A REJOIN CAN LAND ANYWHERE. A fresh turn
        CALL    RXHDR           ; stages roll-everything; arriving mid-turn
        OR      A               ; restages the wire's own mask instead
        JR      Z,GLGT0
        CALL    KEEPSYN
        JR      GLGT0A
GLGT0:  CALL    KEEPRST
GLGT0A: LD      A,(V_ST)        ; rejoined with the rolls already spent:
        OR      A               ; straight to the score rows
        JR      NZ,GLGT0B
        LD      A,1
        LD      (UIMODE),A
        CALL    PICKBEST
GLGT0B: CALL    SNDTRN          ; announce BEFORE the input slice
        JR      GLGT2
GLGT1:  LD      A,(V_ACT)       ; someone else's turn: follow the roller
        INC     A
        JR      Z,GLGT2         ; FFH: nobody is up
        DEC     A
        LD      HL,V_PC
        CP      (HL)
        JR      NC,GLGT2
        LD      (VIEWPL),A
GLGT2:  CALL    SCSCAN          ; did somebody else just take a row?
        CALL    PLAYRD
        CALL    DSHOWN          ; the board is whole now; let it be seen
        CALL    REVEAL          ; ...show it, and hold long enough to read
        CALL    SHADST          ; restage the shadow for the next comparison
        LD      A,(V_MYTRN)
        OR      A
        JR      Z,GLGW          ; not my turn: watch only
        CALL    TURNIN
        LD      A,(V_FLAG)
        CP      1
        JR      NZ,GLG2
        LD      A,2             ; rolled: stage /roll
        LD      (PENDACT),A
        JP      GAMELP
GLG2:   CP      2
        JR      NZ,GLG3
        LD      A,3             ; scored: stage /score
        LD      (PENDACT),A
        JP      GAMELP
GLG3:   CP      3
        JP      Z,GLLEAV        ; keypad * inside the turn
        CP      4
        JR      NZ,GLG3A
        CALL    MENUSCR         ; keypad # inside the turn
        LD      A,0FFH
        LD      (PRVCLS),A      ; force the statics back on return
        JP      GAMELP
GLG3A:  CP      5
        JR      NZ,GLG4
        CALL    TIMEOUT         ; the clock ran out; the server has scored
GLG4:   JP      GAMELP          ; slice burned either way: poll now

; NOT YOUR TURN. The shared engine holds half a second either side of the
; prompt landing so there is time to read what just happened before the board
; changes again (gamelogic.c:361 and :399). Without it a fast table is a
; flicker.
GLGW:   LD      B,30
        CALL    PAUSE
        LD      B,60            ; ~1 s of input scanning between game polls
        JP      GLWAIT

; The between-polls input scan. THE READY TOGGLE LIVES HERE, in the input path
; and edge-detected by KEYSCN -- not in the once-per-poll renderer. One press
; cannot ready and un-ready across two polls, and the trigger that joined the
; table cannot toggle on arrival while it is still held.
GLWAIT: PUSH    BC
        CALL    KEYSCN
        POP     BC
        OR      A
        JR      NZ,GLKEY
        PUSH    BC
        CALL    WAITVB
        POP     BC
        DJNZ    GLWAIT
        JP      GAMELP
GLKEY:  CP      'E'             ; keypad * leaves the table
        JR      Z,GLLEAV
        CP      '.'
        JR      NZ,GLK1
        CALL    MENUSCR
        LD      A,0FFH
        LD      (PRVCLS),A      ; force the statics on return
        JP      GAMELP
GLK1:   CP      'T'
        JR      NZ,GLK2
        LD      A,(V_CLS)
        OR      A
        JP      NZ,GLWAIT       ; the trigger only means READY in the lobby
        CALL    SNDSEL
        LD      A,1
        LD      (PENDACT),A     ; stage /ready
        JP      GAMELP
GLK2:   CP      '0'
        JP      Z,GAMELP        ; poll now
        JP      GLWAIT
GLLEAV: LD      A,RQLEAVE
        CALL    APICALL         ; best effort; we are going regardless
        JP      TBLSCR

; GLFAIL: the poll failed. Back off -- the shared engine's 1, 2, 3, 4 second
; ladder -- and show the connection icon once it has failed twice, so a single
; dropped poll does not flash something alarming.
GLFAIL: LD      HL,FAILCNT
        LD      A,(HL)
        CP      4
        JR      NC,GLF1
        INC     (HL)
GLF1:   LD      A,(FAILCNT)
        CP      2
        JR      C,GLF2
        LD      D,0
        LD      E,0
        LD      A,TICONN1
        CALL    DPUTC
        LD      A,TICONN2
        CALL    DPUTC
GLF2:   LD      A,(FAILCNT)
        LD      B,A
        LD      A,0
GLF3:   ADD     A,60            ; 60 frames per failure, capped at four
        DJNZ    GLF3
        LD      B,A
        OR      A
        JR      NZ,GLF4
        LD      B,60
GLF4:   PUSH    BC
        CALL    KEYSCN
        POP     BC
        CP      'E'
        JR      Z,GLLEAV
        PUSH    BC
        CALL    WAITVB
        POP     BC
        DJNZ    GLF4
        JP      GAMELP

; ----------------------------------------------------------------------
; CHKNEW: map the round to a screen class (0 lobby, 1 play, 2 over). A class
; or seat-count change means the screen is holding content that no longer
; exists: clear, redraw the statics, and reset the edges that must not fire
; across the boundary.
CHKNEW: LD      A,GORND
        CALL    RXHDR
        OR      A
        JR      Z,CHKC          ; 0 -> lobby
        CP      RNOVER
        JR      NZ,CHKC1
        LD      A,2
        JR      CHKC
CHKC1:  LD      A,1
CHKC:   LD      (V_CLS),A
        LD      HL,PRVCLS
        CP      (HL)
        LD      (HL),A
        JR      NZ,CHKN1
        LD      A,(V_PC)
        LD      HL,PRVPC
        CP      (HL)
        LD      (HL),A
        JR      NZ,CHKN1
        XOR     A
        LD      (V_NEWS),A
        RET
CHKN1:  LD      A,(V_PC)        ; keep PRVPC honest on the class path too
        LD      (PRVPC),A
        LD      A,0FFH          ; a roll edge must not fire across a boundary
        LD      (PRVRLS),A
        LD      A,1
        LD      (V_NEWS),A
        CALL    SHADCL          ; and nothing may be revealed from a shadow
        CALL    DHIDE           ; that belonged to a different table. The
        CALL    DCLS            ; display stays off until DSHOWN, after the
        LD      A,(V_CLS)       ; first full paint
        CP      1
        RET     NZ              ; lobby and game over redraw everything anyway
        JP      CBOARD          ; play statics: the grid and the labels

; DSHOWN: reveal the screen once the first full paint after a class change has
; happened, and only then. CHKNEW blanks and redraws the statics; revealing
; there shows a half-built screen for as long as it takes to fill in.
DSHOWN: LD      A,(V_NEWS)
        OR      A
        RET     Z
        XOR     A
        LD      (V_NEWS),A
        JP      DSHOW

; ----------------------------------------------------------------------
; PLAYRD: one play-class render. Prompt, round, seats, scores, dice.
; Clobbers everything.
PLAYRD: LD      A,(V_MYTRN)     ; the clock belongs to your turn alone; without
        OR      A               ; this a turn that ends while it is on screen
        CALL    Z,CLOCKCL       ; leaves its last second sitting there
        CALL    PRMPT
        CALL    RNDDRW
        CALL    CSEATS
        CALL    CARDRW
        JP      DICEHUD

; PRMPT: the server's line, straight out of the reply window, as a fixed-width
; field across row 0. It writes the full 32 and RNDDRW paints the round back
; over the last six in the play class, so the lobby -- which has no round to
; show -- gets the whole line for its message. Nothing is copied: DPRTN walks the wire bytes
; and blank-pads the tail, so a shorter prompt cannot leave a longer one's
; end behind. Clobbers everything.
PRMPT:  CALL    DBANKW
        LD      HL,FNRDATA+GOPRMPT
        LD      D,PROMPTY
        LD      E,0
        LD      B,32
        JP      DPRTN

; RNDDRW: "R n/13" in the six cells the prompt does not use.
; Clobbers everything.
RNDDRW: LD      A,(V_CLS)
        CP      1
        JR      NZ,RNDCL
        CALL    DBANKA
        LD      D,PROMPTY
        LD      E,26
        LD      HL,TROUND
        CALL    DPRINTA
        LD      A,(V_RND)
        LD      L,A
        LD      H,0
        LD      D,PROMPTY
        LD      E,27
        LD      C,2
        CALL    NUMFLD
        LD      D,PROMPTY
        LD      E,29
        LD      HL,TOF13
        JP      DPRINTA
RNDCL:  LD      D,PROMPTY
        LD      E,26
        LD      B,6
        JP      DFILL

; ----------------------------------------------------------------------
; TIMEOUT: the clock reached zero. The server has already force-scored, so
; there is nothing to send -- just say what happened and let the next poll
; find out where the game went.
TIMEOUT:
        CALL    DBANKA
        LD      HL,TTIMOUT
        LD      D,PROMPTY
        LD      E,0
        CALL    DPRINTA
        CALL    SNDBAD
        LD      B,60
        JP      PAUSE

; GOVERR: the game-over board. The scorecard is still on screen from the play
; class; this adds the header and the final totals row.
; Clobbers everything.
GOVERR: CALL    PRMPT
        CALL    RNDDRW
        CALL    CSEATS
        CALL    CARDRW
        CALL    DBANKA
        LD      HL,TSCORE
        LD      D,21
        LD      E,SCORESX
        CALL    DPRINTA
        LD      A,(V_NEWS)
        OR      A
        RET     Z
        JP      SNDOVER

; ----------------------------------------------------------------------
; MENUSCR: the in-game menu. There is no keyboard, so the options are keypad
; digits and the footer says so; keypad * backs out, which is the family rule
; -- cancel is that key and nothing else.
; Clobbers everything.
MENUSCR:
        CALL    DHIDE
        CALL    DCLS
        CALL    MENUDR
        CALL    DSHOW
MEN1:   CALL    KEYSCN
        OR      A
        JR      Z,MEN1
        CP      'T'
        RET     Z
        CP      'E'
        RET     Z
        CP      '1'
        JR      NZ,MEN2
        LD      HL,PREFMUT      ; sound on/off
        LD      A,(HL)
        XOR     1
        LD      (HL),A
        CALL    SNDSEL
        CALL    MENUDR
        JR      MEN1
MEN2:   CP      '2'
        JR      NZ,MEN1
        CALL    SNDSEL
        JP      GLLEAV

MENUDR: LD      D,4
        LD      E,10
        LD      HL,TFUJI
        CALL    DPRINTA
        LD      D,8
        LD      E,8
        LD      HL,TMENU1
        CALL    DPRINT
        LD      D,10
        LD      E,8
        LD      HL,TMENU2
        CALL    DPRINT
        LD      D,10
        LD      E,17
        LD      A,(PREFMUT)
        OR      A
        LD      HL,TON
        JR      Z,MEN3
        LD      HL,TOFF
MEN3:   CALL    DPRINTA
        LD      D,12
        LD      E,8
        LD      HL,TMENU3
        JP      DPRINT

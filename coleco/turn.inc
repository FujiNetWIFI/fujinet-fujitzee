; turn.inc -- your turn. Two cursor modes over one bounded input slice.
;
; DICE mode walks the ROLL tile and the five dice (trigger holds a die or
; submits the roll); SCORE mode walks the open scorecard rows (trigger takes
; the row). Stick-up crosses from the dice to the scores; down from the bottom
; of a column drops back to the dice while rolls remain.
;
; TURNIN runs one bounded ~4 s slice and returns; GAMELP re-polls and re-enters
; while it is still our turn, so a game that ends -- or a table that empties --
; is noticed within a slice. UIMODE/DCUR/SCUR/KEEPBF persist across slices, a
; keypress restarts the slice, and KEYSCN's edge detection means holding a key
; cannot spin it.
;
; THE CLOCK. The countdown reseeds from the wire's moveTime on every entry, so
; it self-corrects each poll, and here it is counted in VBLANKS rather than the
; astrocade's calibrated delay loop -- the ColecoVision has a real 60 Hz
; interrupt, so a second is exactly sixty frames and the clock cannot drift
; against the server's. It shows only in the last fifteen seconds, repaints
; only when the whole-second value changes, and ticks audibly as it goes.
;
; At zero it BAILS rather than waiting: the server has already force-scored,
; and only the next poll can discover where the game went. That is the
; Intellivision lesson, and it is why V_FLAG has a value for it.
;
; V_FLAG on return: 0 slice out, 1 roll staged (KEEPBF), 2 score staged
; (SCOREIX), 3 leave, 4 menu, 5 timed out.

TISLICE EQU     240             ; ~4 s of frames: a player still choosing
TIBADSL EQU     60              ; ~1 s: what a rejected press earns
CLOCKAT EQU     15              ; seconds remaining when the clock appears

TURNIN: XOR     A
        LD      (V_FLAG),A
        LD      (TGFRM),A
        LD      A,0FFH
        LD      (TGSHOWN),A
        LD      A,GOMVTIM
        CALL    RXHDR
        LD      (TGLEFT),A
        LD      B,TISLICE
TILP:   PUSH    BC
        CALL    KEYSCN
        POP     BC
        OR      A
        JP      NZ,TIKEY
        PUSH    BC
        CALL    WAITVB
        LD      HL,TGFRM        ; the local second hand
        INC     (HL)
        LD      A,(HL)
        CP      60
        JR      C,TINX
        LD      (HL),0
        LD      A,(TGLEFT)
        OR      A
        JR      Z,TIOUT         ; already zero: bail now
        DEC     A
        LD      (TGLEFT),A
        PUSH    AF
        CALL    CLOCKRW
        POP     AF
        OR      A
        JR      Z,TIOUT         ; hit zero: the server has skipped us
        CP      6
        CALL    C,SNDTICK       ; the last five seconds tick
TINX:   POP     BC
        DJNZ    TILP
        RET                     ; slice out: V_FLAG stays 0
TIOUT:  POP     BC              ; discard the slice counter
        LD      A,5
        LD      (V_FLAG),A
        RET
TIRST:  LD      B,TISLICE       ; a press restarts the slice
        JP      TILP

; CLOCKRW: paint the move clock for TGLEFT, but only inside the last fifteen
; seconds and only when the second actually changed. Three cells of VRAM.
; Clobbers everything.
CLOCKRW:
        LD      A,(TGLEFT)
        CP      CLOCKAT+1
        RET     NC              ; still plenty of time: nothing on screen
        LD      HL,TGSHOWN
        CP      (HL)
        RET     Z               ; the same second is already painted
        LD      (HL),A
        LD      E,TIMERX
        LD      D,BOTTOMY
        LD      A,TICLOCK
        CALL    DTILE
        CALL    DBANKA
        LD      A,(TGLEFT)
        LD      L,A
        LD      H,0
        LD      D,BOTTOMY+1
        LD      E,TIMERX
        LD      C,2             ; two cells, not three: column 7 is the dice
        JP      NUMFLD          ; cursor's left bar and a third digit would
                                ; sit on top of it

; CLOCKCL: wipe the clock. Called the moment a roll goes out, because the
; server restarts the timer and a stale "3" sitting there is worse than
; nothing. Clobbers everything.
CLOCKCL:
        LD      A,0FFH
        LD      (TGSHOWN),A
        LD      D,BOTTOMY
        LD      E,TIMERX
        LD      B,1
        CALL    DFILL
        LD      D,BOTTOMY+1
        LD      E,TIMERX
        LD      B,2
        JP      DFILL

; ---- key dispatch -----------------------------------------------------
TIKEY:  LD      C,A
        CP      'E'             ; leave -- handed back to GAMELP so the stack
        JR      NZ,TIK0         ; stays balanced
        LD      A,3
        LD      (V_FLAG),A
        RET
TIK0:   CP      '.'             ; the in-game menu, same hand-back
        JR      NZ,TIK1
        LD      A,4
        LD      (V_FLAG),A
        RET
TIK1:   LD      A,(UIMODE)
        OR      A
        JP      NZ,TISCR

; ---- DICE mode --------------------------------------------------------
        LD      A,C
        CP      'L'
        JR      NZ,TID1
        CALL    TIROLLS         ; the engine ignores left/right with no rolls
        JP      Z,TIRST
        LD      A,(DCUR)
        OR      A
        JR      NZ,TIDL1
        LD      A,6
TIDL1:  DEC     A
        LD      (DCUR),A
TIDMV:  CALL    SNDKEY
        CALL    DICEHUD
        JP      TIRST
TID1:   CP      'R'
        JR      NZ,TID1A
        CALL    TIROLLS
        JP      Z,TIRST
        LD      A,(DCUR)
        INC     A
        CP      6
        JR      C,TIDR1
        XOR     A
TIDR1:  LD      (DCUR),A
        JR      TIDMV
TID1A:  CP      'D'             ; down from the dice drops onto the ROLL tile
        JR      NZ,TID2
        XOR     A
        LD      (DCUR),A
        JR      TIDMV
TID2:   CP      'U'             ; up crosses into the scorecard -- at the
        JR      NZ,TID3         ; BOTTOM of it, which is where the engine puts
        LD      A,GODICE        ; you, and it needs dice on the table to score
        CALL    RXHDR
        OR      A
        JP      Z,TIBAD
        LD      A,1
        LD      (UIMODE),A
        LD      A,15            ; walk up from past the end: the lowest open row
        CALL    SKPUP
        JR      C,TID2A
        LD      (SCUR),A
        JR      TID2B
TID2A:  CALL    PICKBEST        ; nothing open at all below: fall back
TID2B:  CALL    SNDSEL
        CALL    CARDRW          ; show the score cursor
        CALL    DICEHUD         ; clear the dice cursor
        JP      TIRST
TID3:   CP      'T'
        JP      NZ,TIRST
        LD      A,(DCUR)
        OR      A
        JR      Z,TIDROL
        DEC     A               ; a die: toggle held <-> re-roll
        LD      HL,KEEPBF
        ADD     A,L
        LD      L,A
        JR      NC,TIDT1
        INC     H
TIDT1:  LD      A,(HL)
        XOR     1               ; '0' <-> '1'
        LD      (HL),A
        CALL    SNDHLD
        CALL    DICEHUD
        JP      TIRST
TIDROL: LD      A,GORLEFT       ; the ROLL tile: submit the roll
        CALL    RXHDR
        OR      A
        JP      Z,TIBAD         ; no rolls left
        LD      HL,KEEPBF       ; at least one die must move
        LD      B,5
TIDR2:  LD      A,(HL)
        CP      '1'
        JR      Z,TIDGO
        INC     HL
        DJNZ    TIDR2
        JP      TIBAD           ; everything held: nothing to roll
TIDGO:  LD      A,1
        LD      (V_FLAG),A
        CALL    CLOCKCL         ; the server restarts the clock on a roll
        JP      SNDROLL

; TIROLLS: Z when there are no rolls left. Preserves B,C. Clobbers A,D,E,H,L.
TIROLLS:
        LD      A,GORLEFT
        CALL    RXHDR
        OR      A
        RET

; ---- SCORE mode -------------------------------------------------------
; ONE LIST, top to bottom. The astrocade client this was transcribed from has a
; two-COLUMN compact card, so its up/down stayed inside a column half and
; left/right crossed between them. This port uses the CoCo and MS-DOS layout,
; where the fifteen rows are a single vertical list -- so that navigation left
; slots 8-14 unreachable without a sideways press nobody would think to make.
; Here up and down walk the whole card, exactly as the shared engine does
; (gamelogic.c:753-772), and left/right belong to the dice alone.
TISCR:  LD      A,C
        CP      'U'
        JR      NZ,TIS1
        LD      A,(SCUR)
        CALL    SKPUP
        JP      C,TIRST         ; already at the top: stay put
TISMV:  LD      (SCUR),A
        CALL    SNDKEY
        CALL    CARDRW
        JP      TIRST
TIS1:   CP      'D'
        JR      NZ,TIS2
        LD      A,(SCUR)
        CALL    SKPDN
        JR      NC,TISMV
        CALL    TIROLLS         ; off the bottom: back to the dice, but only
        JP      Z,TIRST         ; while there is still something to roll
        LD      A,1             ; the engine lands on die 1
        LD      (DCUR),A
        JR      TISDIC
TIS2:   CP      'L'
        JR      Z,TISLR
        CP      'R'
        JR      NZ,TIS3
TISLR:  CALL    TIROLLS         ; sideways from the card returns to the dice:
        JP      Z,TIRST         ; the engine snaps to die 1 and then applies
        LD      A,C             ; the direction
        CP      'L'
        LD      A,0             ; left from die 1 is the ROLL tile
        JR      Z,TISLR1
        LD      A,2             ; right from die 1 is die 2
TISLR1: LD      (DCUR),A
TISDIC: XOR     A
        LD      (UIMODE),A
        CALL    SNDSEL
        CALL    CARDRW          ; clear the score cursor
        CALL    DICEHUD         ; show the dice cursor
        JP      TIRST
TIS3:   CP      'T'
        JP      NZ,TIRST
        LD      A,(SCUR)
        CALL    RXVALID
        CP      0FFH
        JP      Z,TIBAD         ; a closed row; should be unreachable
        LD      E,A             ; E = the points this row would earn
        LD      A,(SCUR)
        LD      (SCOREIX),A
        LD      A,2
        LD      (V_FLAG),A
        PUSH    DE
        CALL    TAKEFX          ; the commit animation the engine does
        POP     DE
        LD      A,(SCUR)        ; the fanfare, for a real fujitzee only
        CP      SCFUJI
        JR      NZ,TISSC
        LD      A,E
        OR      A
        JR      Z,TISSC
        JP      SNDFUJI
TISSC:  JP      SNDSCOR

; TAKEFX: commit the chosen row. Every OTHER potential is wiped first, so for
; the moment before the request goes out the card shows exactly what you chose
; and nothing else, then the cursor blinks on it. Transcribed from the shared
; engine (gamelogic.c:672-680), including its two pauses.
; Clobbers everything.
TAKEFX: LD      A,0FFH          ; wipe every potential but the chosen row
        LD      (V_CAT),A
TKF1:   LD      HL,V_CAT
        INC     (HL)
        LD      A,(HL)
        CP      15
        JR      NC,TKF3
        LD      HL,SCUR
        CP      (HL)
        JR      Z,TKF1          ; leave the chosen one on screen
        CALL    POTCLR
        JR      TKF1
TKF3:   LD      B,5
        CALL    PAUSE
        LD      A,(SCUR)
        CALL    CURSAT
        LD      B,10
        JP      PAUSE

; A rejected press earns only a short slice: an accepted key restarting the
; full one is a player still aiming, but a trigger bounced off a dead ROLL
; tile must not be able to starve the poll loop.
TIBAD:  CALL    SNDBAD
        LD      B,TIBADSL
        JP      TILP

; ----------------------------------------------------------------------
; SKPDN: from slot A, the next open row DOWN the whole card; carry set when
; there is none. SKPUP: the same, upward -- and it accepts 15 as a start, which
; is how "enter the card from the bottom" is spelled.
;
; The always-closed slots (the upper total and the bonus) skip like any other
; closed row, which is what keeps the cursor off the three the server computes
; for itself.
; Clobber B,C,D,E,H,L.
SKPDN:  LD      C,A
SKD1:   INC     C
        LD      A,C
        CP      15
        JR      NC,SKDF
        LD      A,C
        CALL    RXVALID
        CP      0FFH
        JR      Z,SKD1
        LD      A,C
        OR      A               ; clears carry
        RET
SKDF:   SCF
        RET

SKPUP:  LD      C,A
SKU1:   LD      A,C
        OR      A
        JR      Z,SKUF          ; already at the top row
        DEC     C
        LD      A,C
        CALL    RXVALID
        CP      0FFH
        JR      Z,SKU1
        LD      A,C
        OR      A
        RET
SKUF:   SCF
        RET

; PICKBEST: put the score cursor on the highest-paying open row, first winning
; a tie. This is where the cursor rests once the rolls run out, so that
; trigger-trigger-trigger-trigger plays a legal, if artless, game.
; Clobbers everything.
; RXVALID reads through DE, so the running best lives in RAM rather than in a
; register: keeping it in E is the natural way to write this and it is quietly
; wrong -- every candidate then compares against a fragment of the reply
; window's address, and the cursor lands on whichever row happens to win that.
PICKBEST:
        LD      B,0
        LD      C,0
        LD      A,0FFH          ; no candidate yet
        LD      (V_TMP2),A
PKB1:   LD      A,B
        CP      15
        JR      NC,PKB9
        LD      A,B
        CALL    RXVALID
        CP      0FFH
        JR      Z,PKB2
        LD      (V_TMP),A       ; this row's points
        LD      A,(V_TMP2)
        CP      0FFH
        JR      Z,PKBT          ; the first open row is the baseline
        LD      HL,V_TMP
        CP      (HL)            ; best - candidate
        JR      NC,PKB2         ; best >= candidate: first of a tie wins
PKBT:   LD      A,(V_TMP)
        LD      (V_TMP2),A
        LD      C,B
PKB2:   INC     B
        JR      PKB1
PKB9:   LD      A,C
        LD      (SCUR),A
        RET

; SCURFX: keep the score cursor on an open row when entering the mode.
SCURFX: LD      A,(SCUR)
        CALL    RXVALID
        CP      0FFH
        RET     NZ
        JP      PICKBEST

; ----------------------------------------------------------------------
; KEEPRST: the mask back to roll-everything.
; KEEPSYN: restage it from the wire's echo after a roll lands, so the next
; submit re-rolls the same dice unless the player retunes it. An empty echo --
; which is what a fresh turn looks like -- falls back to roll-everything.
KEEPRST:
        LD      HL,KEEPBF
        LD      B,5
KRS1:   LD      (HL),'1'
        INC     HL
        DJNZ    KRS1
        LD      (HL),0
        RET

KEEPSYN:
        LD      HL,GOKEEP
        LD      DE,KEEPBF
        LD      B,5
        CALL    RXSTRN
        LD      A,(KEEPBF)
        OR      A
        RET     NZ
        JR      KEEPRST

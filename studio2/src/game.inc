; game.inc -- st_game: the table.
;
; Poll /state, draw it, act on the keys. Every poll redraws the whole screen
; out of the reply window; rewriting a cell with what it already holds changes
; no pixel, so nothing flickers and nothing is tracked.
;
; A roll, a score and a ready are each one request whose reply is the next
; /state (the server rolls the first roll of every turn itself).
;
; Cursor V_CUR: 0-15 a card slot (left column 0-7, right column 8-15), 16-20 a
; die. Keys: 2468 move, 5 holds a die or scores the slot, 1 rolls, 3 and 9 turn
; to the other cards, 7 the standings, 0 the menu. In the lobby 5 is ready.

POLLF   EQU     60              ; frames between polls
FAILF   EQU     180             ; and after a failed one

; st_game -- sit down at V_TABLE (or come back from the menu).
        FIT     72
st_game:
        RESET
        SETV    V_SEAT, 1
        SETV    V_REQ, REQ_STATE
        SETV    V_OK, 0
        SETV    V_SCR, 0FFH
        SETV    V_PME, 0
        SETV    V_PRL, 0FFH
        SETV    V_PACT, 0FEH
        SETV    V_STAND, 0
        SETV    V_VSEAT, 0
        SETV    V_ME, 0
        SETV    V_CUR, 0
        FCALL   d_cls
        PUTS    40H, s_joining
        FJMP    gpoll

; gpoll -- one request (V_REQ: a /state, or the action whose reply is the
; next state), validated, then drawn.
        FIT     120
gpoll:  LDR     R8, GMINLEN
        LDR     R9, GMAXLEN
        FCALL   setlim
        FCALL   nfetch
        PLO     RC
        SETV    V_REQ, REQ_STATE
        GLO     RC
        BNZ     gfail
        LDI     HPCNT                   ; 1 to GMAXPL seats, and exactly the
        FCALL   rpeek                   ; length they imply
        BZ      gbad
        SMI     GMAXPL+1
        BDF     gbad
        LDR     R8, HDRLEN
        LDI     PSTRIDE
        PLO     R9
        LDI     HPCNT
        FCALL   rpeek
        FCALL   vlen
        BNZ     gbad
        LDI     HROUND                  ; 0-13, or 99 at the end
        FCALL   rpeek
        XRI     99
        BZ      gok
        LDN     RB
        SMI     14
        BDF     gbad
gok:    SETV    V_OK, 1
        FCALL   gcues
        FCALL   grender
        SETV    V_IVL, POLLF
        FJMP    gwait
gbad:   LDI     FE_LEN
gfail:  PUTV    V_ERRC
        SETV    V_OK, 0                 ; the window is not a state now
        AT      90H
        OP      TOP_INV_ON
        LDR     R8, s_neterr
        FCALL   d_puts
        GETV    V_ERRC
        FCALL   d_hex
        OP      TOP_INV_OFF
        OP      TOP_CLREOL
        BEEP    20
        SETV    V_IVL, FAILF
        FJMP    gwait

; gwait -- V_IVL frames of keys, then the next poll.
        FIT     80
gwait:  GLO     RE
        PUTV    V_T0
gw1:    FCALL   kget
        BNZ     gkey
        GETV    V_T0
        STR     R2
        GLO     RE
        SM                              ; frames gone
        STR     R2
        GETV    V_IVL
        SD                              ; gone - interval
        BNF     gw1
        FJMP    gpoll

; gkey -- D = KEY_A/KEY_B + k.
        FIT     120
gkey:   ANI     0FH
        PLO     RC
        BZ      gkmenu
        GETV    V_OK
        BZ      gw1x                    ; no state to act on
        GETV    V_STAND
        BZ      gk0
        SETV    V_STAND, 0              ; any key leaves the standings
        FCALL   grender
        BR      gw1x
gk0:    GLO     RC
        XRI     5
        BZ      gk5
        GLO     RC
        XRI     1
        BZ      gk1
        GLO     RC
        XRI     2
        BZ      gk2
        GLO     RC
        XRI     4
        BZ      gk4
        GLO     RC
        XRI     6
        BZ      gk6
        GLO     RC
        XRI     8
        BZ      gk8
        GLO     RC
        XRI     3
        BZ      gk3
        GLO     RC
        XRI     9
        BZ      gk9
        GLO     RC
        XRI     7
        BNZ     gw1x
        FJMP    gk7
gkmenu: FJMP    st_menu
gw1x:   FJMP    gw1
gk5:    FJMP    gact
gk1:    FJMP    groll
gk2:    FJMP    gup
gk4:    FJMP    gleft
gk6:    FJMP    gright
gk8:    FJMP    gdown
gk3:    FJMP    gprev
gk9:    FJMP    gnext

; gsend -- the request is set: send it, and draw its reply.
        FIT     24
gsend:  SETV    V_OK, 0
        PUTS    90H, s_sending
        OP      TOP_CLREOL
        BEEP    6
        FJMP    gpoll

; gact -- 5: ready in the lobby; on our turn hold a die or score a slot.
        FIT     96
gact:   GETV    V_RND
        BNZ     ga1
        SETV    V_REQ, REQ_READY
        FJMP    gsend
ga1:    GETV    V_ME
        BZ      ga9
        GETV    V_CUR
        SMI     16
        BDF     ga2                     ; D = the die
        GETV    V_CUR                   ; a slot
        PUTV    V_ARG
        SETV    V_REQ, REQ_SCORE
        FJMP    gsend
ga2:    ADI     LO(V_HELD)
        PLO     RB
        LDI     HI(V_HELD)
        PHI     RB
        LDN     RB
        XRI     1
        STR     RB
        FCALL   gdice
ga9:    FJMP    gw1

; groll -- 1: re-roll the dice we are not holding.
        FIT     96
groll:  GETV    V_ME
        BZ      gr9
        GETV    V_RL
        BZ      gr9
        LDI     0
        PLO     RC                      ; dice to re-roll
        PLO     R9                      ; this die
gr1:    GLO     R9
        ADI     LO(V_HELD)
        PLO     RB
        LDI     HI(V_HELD)
        PHI     RB
        LDN     RB
        BNZ     gr2
        INC     RC
        LDI     '1'                     ; not held: roll it
        BR      gr3
gr2:    LDI     '0'
gr3:    PHI     R8
        GLO     R9
        ADI     LO(V_ARG)
        PLO     RB
        GHI     R8
        STR     RB
        INC     R9
        GLO     R9
        XRI     5
        BNZ     gr1
        GLO     RC
        BZ      gr9                     ; all five held: nothing to roll
        SETV    V_REQ, REQ_ROLL
        FJMP    gsend
gr9:    FJMP    gw1

; ---- the cursor ----

; cvalid -- D = 1 if slot D is one we may score now, else 0.
        FIT     48
cvalid: PLO     R9
        GETV    V_ME
        BZ      cv0
        GLO     R9
        SMI     NSLOTS
        BDF     cv0
        GLO     R9
        ADI     LO(FN_REPLY+HVALID)
        PLO     RB
        LDI     HI(FN_REPLY+HVALID)
        PHI     RB
        LDN     RB
        ANI     80H
        BNZ     cv0                     ; negative: not offered
        LDI     1
        FRET
cv0:    LDI     0
        FRET

; cstep -- D = 1 (down) or $FF (up): the next offered slot that way in the
; cursor's column; the cursor stays if there is none.
        FIT     80
cstep:  PUTV    V_T2
        GETV    V_CUR
        PUTV    V_T1
cst1:   GETV    V_T2
        STR     R2
        GETV    V_T1
        ADD
        PUTV    V_T1
        ANI     7
        PLO     R9                      ; the new slot's row
        GETV    V_T2
        SHR                             ; 1 -> 0, $FF -> $7F: the row that
        ANI     7                       ; means we left the column
        STR     R2
        GLO     R9
        XOR
        BZ      cst9
        GETV    V_T1
        FCALL   cvalid
        BZ      cst1
        GETV    V_T1
        PUTV    V_CUR
cst9:   FRET

; cscan -- D = a slot: the first offered slot from it down its column,
; wrapping at the end; the cursor stays if there is none.
        FIT     80
cscan:  PUTV    V_T1
        SETV    V_CTR, 8
csc1:   GETV    V_T1
        FCALL   cvalid
        BNZ     csc8
        GETV    V_T1
        ADI     1
        ANI     7
        PLO     R9
        GETV    V_T1
        ANI     8
        STR     R2
        GLO     R9
        OR
        PUTV    V_T1
        GETV    V_CTR
        SMI     1
        STR     RA
        BNZ     csc1
        FRET
csc8:   GETV    V_T1
        PUTV    V_CUR
        FRET

; gbest -- the cursor to the slot that pays the most.
        FIT     80
gbest:  SETV    V_T1, 0
        SETV    V_T2, 0                 ; best score + 1 so far
gb1:    GETV    V_T1
        FCALL   cvalid
        BZ      gb3
        GETV    V_T1
        ADI     LO(FN_REPLY+HVALID)
        PLO     RB
        LDI     HI(FN_REPLY+HVALID)
        PHI     RB
        LDN     RB
        ADI     1
        PLO     R9
        GETV    V_T2
        STR     R2
        GLO     R9
        SM
        BNF     gb3
        BZ      gb3                     ; not better
        GLO     R9
        PUTV    V_T2
        GETV    V_T1
        PUTV    V_CUR
gb3:    GETV    V_T1
        ADI     1
        PUTV    V_T1
        XRI     NSLOTS
        BNZ     gb1
        FRET

; the cursor keys. All of them need our turn.
        FIT     40
gdown:  GETV    V_ME
        BZ      gm9
        GETV    V_CUR
        SMI     16
        BNF     gdn1
        SMI     4
        BDF     gm9                     ; the last die
        GETV    V_CUR
        ADI     1
        PUTV    V_CUR
        BR      gmv
gdn1:   LDI     1
        FCALL   cstep
        BR      gmv
gm9:    FJMP    gw1
gmv:    FCALL   grender
        FJMP    gw1

        FIT     40
gup:    GETV    V_ME
        BZ      gm9u
        GETV    V_CUR
        SMI     16
        BNF     gup1
        BZ      gm9u                    ; the first die
        GETV    V_CUR
        SMI     1
        PUTV    V_CUR
        BR      gmvu
gup1:   LDI     0FFH
        FCALL   cstep
gmvu:   FCALL   grender
gm9u:   FJMP    gw1

        FIT     56
gright: GETV    V_ME
        BZ      grt9
        GETV    V_CUR
        SMI     16
        BDF     grt9                    ; on a die
        GETV    V_CUR
        ANI     8
        BNZ     grt2
        GETV    V_CUR                   ; left column: the same row, right
        ANI     7
        ORI     8
        FCALL   cscan
        BR      grt4
grt2:   GETV    V_CUR                   ; right column: the die on that row
        ANI     7
        PLO     R9
        SMI     5
        BNF     grt3
        LDI     4
        BR      grt3a
grt3:   GLO     R9
grt3a:  ADI     16
        PUTV    V_CUR
grt4:   FCALL   grender
grt9:   FJMP    gw1

        FIT     48
gleft:  GETV    V_ME
        BZ      glt9
        GETV    V_CUR
        SMI     16
        BNF     glt1
        ORI     8                       ; a die: the right column, its row
        FCALL   cscan
        BR      glt4
glt1:   GETV    V_CUR
        ANI     8
        BZ      glt9
        GETV    V_CUR
        ANI     7
        FCALL   cscan
glt4:   FCALL   grender
glt9:   FJMP    gw1

; gnext, gprev -- 9 and 3: turn to the next or the last seat's card.
        FIT     40
gnext:  GETV    V_VSEAT
        ADI     1
        PLO     R9
        GETV    V_PC
        STR     R2
        GLO     R9
        XOR
        BNZ     gnx1
        LDI     0
        BR      gnx2
gnx1:   GLO     R9
gnx2:   PUTV    V_VSEAT
        FCALL   grender
        FJMP    gw1

        FIT     40
gprev:  GETV    V_VSEAT
        BNZ     gpv1
        GETV    V_PC
gpv1:   SMI     1
        PUTV    V_VSEAT
        FCALL   grender
        FJMP    gw1

; gk7 -- 7: the standings, while a game is on.
        FIT     24
gk7:    GETV    V_RND
        BZ      gk79
        SETV    V_STAND, 1
        FCALL   grender
gk79:   FJMP    gw1

; ---- what a new state means ----

; gcues -- keep what the screens need from the window, and act on the edges:
; our turn begins (hold nothing, park on the best slot, beep), our rolls run
; out (park on the best slot), someone else rolls (turn to their card).
        FIT     200
gcues:  LDI     HPCNT
        FCALL   rpeek
        PUTV    V_PC
        LDI     HROUND
        FCALL   rpeek
        PUTV    V_RND
        LDI     HROLLS
        FCALL   rpeek
        PUTV    V_RL
        LDI     HACT
        FCALL   rpeek
        PUTV    V_ACT
        SETV    V_ME, 0                 ; ours: a game on, we act, not watching
        GETV    V_RND
        BZ      gc1
        SMI     14
        BDF     gc1
        GETV    V_ACT
        BNZ     gc1
        LDI     HSPEC
        FCALL   rpeek
        BNZ     gc1
        SETV    V_ME, 1
gc1:    GETV    V_ME
        BZ      gc5
        GETV    V_PME
        BNZ     gc2
        FCALL   gnewturn
        BEEP    40
        BR      gc4
gc2:    GETV    V_RL
        BNZ     gc3
        GETV    V_PRL
        BZ      gc3
        FCALL   gbest                   ; the rolls just ran out
gc3:    GETV    V_CUR                   ; the cursor must be on something
        SMI     16                      ; we may take
        BDF     gc4
        GETV    V_CUR
        FCALL   cvalid
        BNZ     gc4
        FCALL   gbest
        BR      gc4
gc5:    GETV    V_PACT                  ; not ours: follow the roller
        STR     R2
        GETV    V_ACT
        XOR
        BZ      gc4
        GETV    V_ACT
        XRI     0FFH
        BNZ     gc6
        LDI     0
        BR      gc7
gc6:    GETV    V_ACT
gc7:    PUTV    V_VSEAT
gc4:    GETV    V_VSEAT                 ; players come and go
        STR     R2
        GETV    V_PC
        SD                              ; seat - count
        BNF     gc8
        SETV    V_VSEAT, 0
gc8:    GETV    V_ME
        PUTV    V_PME
        GETV    V_RL
        PUTV    V_PRL
        GETV    V_ACT
        PUTV    V_PACT
        FRET

; gnewturn -- our turn begins.
        FIT     56
gnewturn:
        SETV    V_VSEAT, 0
        SETV    V_STAND, 0
        LDI     0
        PLO     R9
gnt1:   GLO     R9
        ADI     LO(V_HELD)
        PLO     RB
        LDI     HI(V_HELD)
        PHI     RB
        LDI     0
        STR     RB
        INC     R9
        GLO     R9
        XRI     5
        BNZ     gnt1
        FJMP    gbest

; game.inc -- the poll loop and the per-class renderers.
;
; There is no state variable and no dispatch table: the server's ROUND
; is the state, and GCLASS maps it to one of three screen classes --
; 0 lobby, 1-13 play, 99 over. Every endpoint returns the same state
; blob, so a staged action (PENDACT) rides the next request in place of
; /state and its reply IS the next state. There is never a second round
; trip.
;
; Rendering policy, the card games': every poll redraws everything,
; opaquely, straight out of the reply window. Identical cells rewritten
; are invisible, so there is no flicker and no dirty bookkeeping. The
; one thing opaque overdraw cannot fix is content that SHRANK, so
; CHKNEW watches the screen class and the seat count and answers a
; change with a full clear plus the statics.
;
; The five invariants below are transcribed from the astrocade port's
; README, and each one is a bug that has already been paid for:
;
;  * PRVACT advances at the bottom of EVERY successful poll, lobby
;    included -- activePlayer stays 0 for every poll of your turn, so
;    the edge has to be taken against the previous poll, and the
;    your-turn cue has to fire BEFORE the input slice.
;  * The roll edge is a DECREASE. A fresh turn resets rollsLeft upward,
;    so only a decrease means a roll landed; the $FF sentinel in PRVRLS
;    mutes it across a screen boundary.
;  * The ready toggle is sampled in the WAIT loop, every frame, never
;    once per poll -- a toggle read every two seconds feels dead.
;  * VALID8 has already gated the reply before anything here reads it.
;  * Interaction is time-sliced and returns (turn.inc:TURNIN), so a game
;    that ends is noticed within a slice and the move clock keeps running.

PLYWAIT EQU     90              ; ~1.5 s between polls while playing
LOBWAIT EQU     120             ; ~2 s in the lobby
OVRWAIT EQU     180             ; ~3 s once it is over
FAILWT  EQU     150             ; the backoff after a failed call

; GJOINI: arm everything the poll loop edge-detects against, called by
; lobby.inc as it joins. It lives here rather than there because these
; are all page-0 cells and lobby.inc runs from block 2 -- one call is
; cheaper than nine pointer words.
; Calls one down (KEEPRST). Clobbers R0-R3.
GJOINI: LODI,R0 $FF             ; force the statics on the first poll and
        STRA,R0 PRVCLS          ;   arm the your-turn cue. PRVRLS's $FF
        STRA,R0 PRVACT          ;   also mutes the roll animation across
        STRA,R0 PRVPC           ;   the screen boundary: a fresh turn
        STRA,R0 PRVRLS          ;   RAISES rollsLeft, and only a DECREASE
        EORZ    R0              ;   means a roll landed
        STRA,R0 PENDACT
        STRA,R0 VIEWPL
        STRA,R0 UIMODE
        STRA,R0 SCUR
        STRA,R0 DCUR            ; and on the ROLL tile, as a turn does
        BCTA,UN KEEPRST         ; tail call

GAMEIN: LODI,R0 1               ; the play class draws the card labels
        STRA,R0 V_CLS
        BSTA,UN GSTATIC
        DCOL    ANORM
        DAT     9,0
        SETSTR  TJOIN
        LODI,R1 16
        BSTA,UN DPRTN

; ---- the poll ---------------------------------------------------------
GAMELP: LODA,R0 PENDACT         ; a staged action rides the next request;
        BCFR,EQ GLREQ           ;   0 is not a request id, so it means
        LODI,R0 RQSTATE         ;   "nothing staged"
GLREQ:  BSTA,UN APICALL
        BCFA,EQ GLFAIL
        LODA,R0 PENDACT
        COMI,R0 RQLEAVE         ; we have left: do not parse a reply for
        BCTA,EQ GLGONE          ;   a table we are no longer at
        EORZ    R0
        STRA,R0 PENDACT
        BSTA,UN GHDR
        BSTA,UN GCLASS
        BSTA,UN CHKNEW
        BSTA,UN REVEAL          ; before GEDGE: it needs PRVACT unadvanced
        BSTA,UN GEDGE
        BSTA,UN RENDER
        LODA,R0 V_CLS           ; your turn: hand over to the input slice
        COMI,R0 1
        BCFR,EQ GLW0            ; not the play class
        LODA,R0 V_MYTRN         ; a LOAD sets CC, so zero is EQ: branch
        BCTR,EQ GLW0            ;   AWAY when it is NOT our turn
        BSTA,UN TURNIN
        LODA,R0 V_FLAG
        BCTA,EQ GAMELP          ; slice out or clock expired: poll again
        COMI,R0 1
        BCFR,EQ GLT2
        LODI,R0 RQROLL
        BCTR,UN GLTST
GLT2:   COMI,R0 2
        BCFR,EQ GLT3
        LODI,R0 RQSCORE
        BCTR,UN GLTST
GLT3:   LODI,R0 RQLEAVE
GLTST:  STRA,R0 PENDACT
        BCTA,UN GAMELP          ; the action rides the poll going out now
GLW0:   LODI,R0 PLYWAIT
        LODA,R1 V_CLS
        BCFR,EQ GLW1
        LODI,R0 LOBWAIT
        BCTR,UN GLW2
GLW1:   COMI,R1 2
        BCFR,EQ GLW2
        LODI,R0 OVRWAIT
GLW2:   STRA,R0 V_TICK

; GLWAIT: the gap between polls, and the input scan -- one KEYSCN costs
; exactly one frame, so they are the same loop. The lobby's ready toggle
; lives HERE and not in the renderer, which is the whole point.
GLWAIT: BSTA,UN KEYSCN
        COMI,R0 KNONE
        BCFA,EQ GLKEY
GLWNX:  LODA,R0 V_TICK
        SUBI,R0 1
        STRA,R0 V_TICK
        BCFA,EQ GLWAIT
        BCTA,UN GAMELP

GLKEY:  COMI,R0 KCLR            ; Clear leaves the table
        BCFR,EQ GLK1
GLLEAV: LODI,R0 RQLEAVE
        STRA,R0 PENDACT
        BCTA,UN GAMELP
GLK1:   COMI,R0 0               ; keypad 0 polls now
        BCTA,EQ GAMELP
GLK2:   COMI,R0 KENT            ; Enter or FIRE readies up, in the lobby
        BCTR,EQ GLKSEL
        COMI,R0 2
        BCFR,EQ GLK3
GLKSEL: LODA,R0 V_CLS
        BCFA,EQ GLWNX
        LODI,R0 RQREADY         ; bare /ready TOGGLES
        STRA,R0 PENDACT
        BSTA,UN SNDSEL
        BCTA,UN GAMELP
GLK3:   COMI,R0 KLEFT           ; page the viewed scorecard -- served
        BCTR,EQ GLKLF           ;   entirely from the cached reply, since
        COMI,R0 KRIGHT          ;   every seat's record arrived in the
        BCFA,EQ GLWNX           ;   same poll
        LODA,R0 VIEWPL
        ADDI,R0 1
        COMA,R0 V_PC
        BCTR,LT GLKPG
        EORZ    R0
        BCTR,UN GLKPG
GLKLF:  LODA,R0 VIEWPL
        BCFR,EQ GLKL1
        LODA,R0 V_PC            ; wrapped past seat 0
GLKL1:  SUBI,R0 1
GLKPG:  STRA,R0 VIEWPL
        LODA,R0 V_CLS           ; the lobby has no scorecard to page
        BCTA,EQ GLWNX
        BSTA,UN SNDKEY
        BSTA,UN CARDHD
        BSTA,UN CARDRW
        BSTA,UN PANEL           ; the seat colours moved with the view
        BCTA,UN GLWNX

GLGONE: EORZ    R0              ; back to table selection
        STRA,R0 PENDACT
        BCTA,UN LOBBY

GLFAIL: DCOL    AHOT            ; a magenta X in the corner, then a
        DAT     0,15            ;   backoff. Clear still works: a dead
        LODI,R0 $31             ;   server must not trap you at the table
        BSTA,UN DPUTC
        BSTA,UN SNDERR
        LODI,R0 FAILWT
        STRA,R0 V_TICK
GLFW:   BSTA,UN KEYSCN
        COMI,R0 KCLR
        BCTA,EQ GLGONE
        LODA,R0 V_TICK
        SUBI,R0 1
        STRA,R0 V_TICK
        BCFA,EQ GLFW
        BCTA,UN GAMELP

; ---- the header cache -------------------------------------------------
; GHDR: playerCount, and the five contiguous state bytes at 63-67 in one
; seek. moveTime is not cached -- TURNIN reseeds from the wire itself on
; every entry, which is what makes the clock self-correcting.
; Calls two down. Clobbers R0-R3.
GHDR:   LODI,R1 GOPC
        BSTA,UN RXHDR
        STRA,R0 V_PC
        LODI,R1 GORND           ; 63..67 are adjacent: one seek serves all
        BSTA,UN RXHDR
        STRA,R0 V_RND
        BSTA,UN RXNEXT          ; rollsLeft -- read again by GEDGE and
        BSTA,UN RXNEXT          ;   DICEHUD; not worth a RAM cell
        STRA,R0 V_ACT
        BSTA,UN RXNEXT          ; moveTime
        BSTA,UN RXNEXT
        STRZ    R1              ; viewing: 1 = you are a spectator
        EORZ    R0
        STRA,R0 V_MYTRN
        LODZ    R1
        BCFR,EQ GHD9
        LODA,R0 V_ACT           ; the server rotates you to index 0
        BCFR,EQ GHD9
        LODI,R0 1
        STRA,R0 V_MYTRN
GHD9:   RETC,UN

; GCLASS: V_CLS = 0 lobby / 1 play / 2 over. Leaf. Clobbers R0.
GCLASS: LODA,R0 V_RND
        BCTR,EQ GCL9            ; round 0: the lobby, and V_CLS = 0
        COMI,R0 RNOVER
        BCFR,EQ GCL2
        LODI,R0 2
        BCTR,UN GCL9
GCL2:   LODI,R0 1
GCL9:   STRA,R0 V_CLS
        RETC,UN

; CHKNEW: the only thing opaque overdraw cannot fix. A class change or a
; seat count change means the screen holds rows that no longer exist.
; Calls four down (GSTATIC -> CARDST -> DPRTN -> DPUTC). Clobbers all.
CHKNEW: LODA,R0 V_CLS
        COMA,R0 PRVCLS
        BCFR,EQ CHKGO
        LODA,R0 V_PC
        COMA,R0 PRVPC
        BCTR,EQ CHK9
CHKGO:  BSTA,UN GSTATIC
CHK9:   LODA,R0 V_CLS
        STRA,R0 PRVCLS
        LODA,R0 V_PC
        STRA,R0 PRVPC
        LODA,R0 VIEWPL          ; a seat can leave under the viewer, and
        COMA,R0 V_PC            ;   PLSEEK past the last record would
        BCTR,LT CHK8            ;   read whatever the window still holds
        EORZ    R0
        STRA,R0 VIEWPL
CHK8:   RETC,UN

; GSTATIC: flip to the 26-line game screen and lay down what never
; changes. DCLSL runs FIRST: $1A00-$1ACF was the name editor's scratch
; and becomes visible screen the instant 26-line mode comes on.
; Calls three down. Clobbers all.
GSTATIC: LODI,R0 $FF            ; a full clear means the reveal's shadow
        STRA,R0 PRVSCM          ;   belongs to some other screen: with
        STRA,R0 PRVSCM+1        ;   every bit set, nothing can turn on
        BSTA,UN DCLSL
        BSTA,UN DCLSU
        BSTA,UN DINI26
        BSTA,UN SPRINI
        LODA,R0 V_CLS
        COMI,R0 1
        BCFR,EQ GST9
        BCTA,UN CARDST          ; the play class: the sixteen row labels
GST9:   RETC,UN

; ---- the bot reveal ---------------------------------------------------
; REVEAL: a seat other than ours just finished a turn. Show WHICH row
; they took -- on their own card, held in magenta for long enough to
; read, with a cue that falls where ours rise.
;
; The highlight is not drawn here: SCELL paints any cell matching REVSLT
; in AHOT, so the whole reveal is "point the view at them, redraw, wait".
; REVSLT going back to $FF on the way out is what disarms it.
;
; It runs from GAMELP rather than inside GEDGE for the return stack:
; CARDRW is five frames deep on its own, and a call from GEDGE would put
; the chain at seven of the 2650's eight. That also means REVEAL takes
; the turn edge itself -- fine, because GEDGE has not advanced PRVACT.
;
; GSTATIC parks PRVSCM at $FFFF after any full clear, so the first poll
; of a game -- and any poll after a seat count change, where the shadow
; would belong to a different player -- reveals nothing: with every bit
; already set, no bit can have turned on.
; Calls four down. Clobbers everything.
REVHOLD EQU     75              ; ~1.2 s, on top of the cue's own 12

REVEAL: LODA,R0 V_CLS
        COMI,R0 1
        BCFA,EQ REV9            ; the play class only
        LODA,R0 V_ACT
        COMA,R0 PRVACT
        BCTA,EQ REV9            ; nobody finished
        LODA,R0 PRVACT
        BCTA,EQ REV9            ; seat 0 is us: we chose it and heard it
        COMI,R0 $FF
        BCTA,EQ REV9            ; nobody was up
        STRZ    R1
        BSTA,UN SCSCAN          ; -> REVSLT, and the mask into V_NUM
        LODA,R0 REVSLT
        COMI,R0 $FF
        BCTR,EQ REV9            ; nothing newly filled
        LODA,R0 PRVACT          ; their card -- GEDGE moves the view on
        STRA,R0 VIEWPL          ;   to the new roller immediately after
        BSTA,UN CARDHD
        BSTA,UN CARDRW
        BSTA,UN PANEL           ; the panel too, so the seat colours and
                                ;   the running total move with the row
        BSTA,UN SNDBOT
        LODI,R3 REVHOLD
REV5:   BSTA,UN WAITVB
        BDRR,R3 REV5
REV9:   LODI,R0 $FF             ; disarm: no later render may highlight
        STRA,R0 REVSLT
        RETC,UN

; ---- the edges --------------------------------------------------------
; GEDGE: the turn edge and the roll edge, computed against the PREVIOUS
; poll and advanced here at the bottom -- lobby included, so the
; lobby->play transition rides the same comparison as any turn change.
; Calls three down. Clobbers all.
GEDGE:  LODA,R0 V_ACT
        COMA,R0 PRVACT
        BCTA,EQ GEDR            ; no turn change
        LODA,R0 V_ACT           ; follow the roller, the way the intv
        COMI,R0 $FF             ;   client does; $FF is nobody
        BCTR,EQ GED1
        STRA,R0 VIEWPL
GED1:   LODA,R0 V_MYTRN
        BCTR,EQ GEDR
        BSTA,UN SNDTRN
        EORZ    R0              ; a fresh turn starts on the ROLL tile:
        STRA,R0 UIMODE          ;   rolling is always the first move, and
        STRA,R0 DCUR            ;   it is what lets a trigger-only player
                                ;   get through a turn at all
; A REJOIN can land anywhere: the server holds your seat, so your turn
; can arrive mid-turn with dice already on the table and the rolls
; spent. Restage the wire's mask when there are dice, and drop straight
; into the score rows when there is nothing left to roll.
        LODI,R1 GODICE
        BSTA,UN RXHDR
        BCFR,EQ GED2
        BSTA,UN KEEPRST
        BCTR,UN GEDR
GED2:   BSTA,UN KEEPSYN
        LODI,R1 GORLEFT
        BSTA,UN RXHDR
        BCFR,EQ GEDR
        LODI,R0 1
        STRA,R0 UIMODE
        BSTA,UN PICKBEST
; The roll edge. Only a DECREASE means a roll landed, and only in the
; play class; the $FF sentinel mutes the first poll after any screen
; change. Which dice moved is the wire's own answer (see ROLLANM).
GEDR:   LODI,R1 GORLEFT
        BSTA,UN RXHDR
        STRA,R0 V_TMP
        LODA,R0 PRVRLS
        COMI,R0 $FF
        BCTR,EQ GEDT            ; sentinel: muted
        LODA,R0 V_CLS
        COMI,R0 1
        BCFR,EQ GEDT
        LODA,R0 V_TMP
        COMA,R0 PRVRLS
        BCFR,LT GEDT            ; not a decrease
        BSTA,UN ROLLANM
; ...and if that was the last roll, the cursor's resting place is the
; highest-paying open row. This is what makes trigger-trigger-trigger
; play a legal, if artless, game -- the Intellivision rule, and the one
; that stops a player being stranded on a dead ROLL tile.
        LODA,R0 V_MYTRN
        BCTR,EQ GEDT
        LODI,R1 GORLEFT         ; re-read: ROLLANM clobbers V_TMP's
        BSTA,UN RXHDR           ;   neighbours and costs nothing here
        BCFR,EQ GEDT
        LODI,R0 1
        STRA,R0 UIMODE
        BSTA,UN PICKBEST
GEDT:   LODI,R1 GORLEFT         ; re-read rather than shelter it: ROLLANM
        BSTA,UN RXHDR           ;   clobbers every register and V_TMP is
        STRA,R0 PRVRLS          ;   not worth defending for one byte
        LODA,R0 V_ACT
        STRA,R0 PRVACT
; ...and shadow the filled rows of whoever is up NOW, so the next turn
; edge can name the row they take. SCSCAN leaves the mask in V_NUM.
        LODA,R0 V_ACT
        COMI,R0 $FF
        BCTR,EQ GEDT7           ; nobody up: nothing to compare against
        COMA,R0 V_PC
        BCFR,LT GEDT7
        STRZ    R1
        BSTA,UN SCSCAN
        LODA,R0 V_NUM
        STRA,R0 PRVSCM
        LODA,R0 V_NUM+1
        STRA,R0 PRVSCM+1
        BCTR,UN GEDT8
GEDT7:  LODI,R0 $FF             ; nobody to shadow: suppress the next
        STRA,R0 PRVSCM          ;   reveal outright
        STRA,R0 PRVSCM+1
GEDT8:  LODI,R0 $FF             ; SCSCAN arms REVSLT as a side effect,
        STRA,R0 REVSLT          ;   and RENDER is next: disarm it
        RETC,UN

; ---- rendering --------------------------------------------------------
; RENDER: dispatch on the class with BRANCHES, so no renderer costs a
; return-stack frame of its own. Clobbers everything.
RENDER: LODA,R0 V_CLS
        BCTA,EQ RNDLOB
        COMI,R0 2
        BCTA,EQ RNDOVR
; The play screen.
RNDPLY: BSTA,UN CARDHD
        BSTA,UN CARDRW
        BSTA,UN CUEDRW
        BSTA,UN DICEROW
        BSTA,UN DICEHUD
        BSTA,UN PANEL
        BSTA,UN CURFIX
        BCTA,UN PROMPT          ; tail call

; The game-over screen: the same card and standings, frozen, with the
; dice strip given over to the banner.
RNDOVR: BSTA,UN CARDHD
        BSTA,UN CARDRW
        DCOL    AHOT
        DAT     10,0
        SETSTR  TOVER
        LODI,R1 16
        BSTA,UN DPRTN
        BSTA,UN PANEL
        BSTA,UN CARHID
        BCTA,UN PROMPT          ; tail call

; The lobby: who is here and who is ready. scores[0] doubles as the
; ready flag, which is why this is not the standings panel.
RNDLOB: DCOL    ANORM
        DAT     0,0
        SETSTR  TLOBBY
        LODI,R1 16
        BSTA,UN DPRTN
        EORZ    R0
        STRA,R0 V_SEAT
RLB1:   LODA,R0 V_SEAT
        COMA,R0 V_PC
        BCFA,LT RLB9
        COMI,R0 8               ; eight rows fit under the title
        BCFA,LT RLB9
        STRZ    R1              ; the ready flag FIRST: RDSCOR clobbers
        LODI,R2 0               ;   R2, which is about to be the screen
        BSTA,UN RDSCOR          ;   cursor. scores[0] IS that flag.
        COMI,R0 $FF
        BCTR,EQ RLBSP           ; $FFFE: a spectator, not a seat
        LODA,R0 V_NUM+1
        BCFR,EQ RLBRD
        LODI,R0 0               ; seated, not ready
        BCTR,UN RLBST
RLBRD:  LODI,R0 1
        BCTR,UN RLBST
RLBSP:  LODI,R0 2
RLBST:  STRA,R0 V_TMP           ; NUMFLD is the only user of V_TMP and
        LODA,R0 V_SEAT          ;   this screen never calls it
        STRZ    R1
        LODI,R2 PLNAME
        BSTA,UN PLSEEK          ; then the reply cursor...
        LODA,R0 V_SEAT
        ADDI,R0 2
        STRZ    R1
        LODI,R2 0
        BSTA,UN DPOS            ; ...and only then the screen cursor
        DCOL    ANORM
        LODI,R1 10
        BSTA,UN RXPRNT
        LODA,R0 V_TMP
        BCFR,EQ RLBD1
        DCOL    ADIM
        SETSTR  TWAIT
        BCTR,UN RLBDR
RLBD1:  COMI,R0 1
        BCFR,EQ RLBD2
        DCOL    APOT
        SETSTR  TREADY
        BCTR,UN RLBDR
RLBD2:  DCOL    ANORM           ; NOT ADIM: this one has to be read
        SETSTR  TSPECT
RLBDR:  LODI,R1 6
        BSTA,UN DPRTN
        LODA,R0 V_SEAT
        ADDI,R0 1
        STRA,R0 V_SEAT
        BCTA,UN RLB1
RLB9:   DCOL    ANORM
        DAT     11,0
        SETSTR  TFREADY
        LODI,R1 16
        BSTA,UN DPRTN
        BSTA,UN CARHID
        BCTA,UN PROMPT          ; tail call

; CUEDRW: row 9 -- the one line that says whose move it is.
; Calls two down. Clobbers R0-R3.
CUEDRW: DAT     9,0
        LODA,R0 V_MYTRN
        BCFR,EQ CUE1
        DCOL    ANORM           ; someone else's move: clear the cue, and
        SETSTR  TBLANK          ;   the clock with it -- TURNIN owns that
        LODI,R1 16              ;   field and only runs on your turn, so
        BSTA,UN DPRTN           ;   nothing else would ever erase it
        DAT     CLKROW,CLKCOL
        LODI,R1 0
        LODI,R3 3
        BCTA,UN DFILL           ; tail call
CUE1:   DCOL    AHOT
        SETSTR  TYTURN
        LODI,R1 16
        BCTA,UN DPRTN           ; tail call

; CURFIX: the cursor chevron belongs on screen only while the score
; rows have it. Calls two down. Clobbers R0-R3.
CURFIX: LODA,R0 V_MYTRN
        BCTR,EQ CURF0
        LODA,R0 UIMODE
        BCTR,EQ CURF0
        BCTA,UN CARCUR          ; tail call
CURF0:  BCTA,UN CARHID          ; tail call

; PROMPT: the server's own line, rows 22-24, straight out of the reply
; window. The field is 41 bytes and the screen is 16 wide, so it takes
; three rows -- and they are WORD-wrapped, because a hard wrap at column
; 16 turns "waiting for everyone" into "waiting for ever / yone", which
; is the first thing anyone notices about the screen.
; Calls three down. Clobbers everything.
PROMPT: EORZ    R0
        STRA,R0 V_SPLIT
        LODI,R0 22
        STRA,R0 V_CAT           ; free here: CARDRW has finished with it
PRM0:   BSTA,UN PRMROW
        LODA,R0 V_CAT
        ADDI,R0 1
        STRA,R0 V_CAT
        COMI,R0 25
        BCTR,LT PRM0
        LODI,R0 $FF             ; V_SPLIT IS REVSLT, and a wrap offset
        STRA,R0 V_SPLIT         ;   that happened to land in 0-15 would
        RETC,UN                 ;   light a scorecard row for good. The
                                ;   reveal is a one-shot, so leaving this
                                ;   byte disarmed after every render is
                                ;   what makes the alias safe.

; PRMROW: one 16-cell row of the prompt at screen row (V_CAT), starting
; from prompt offset (V_SPLIT) and breaking at the last space that fits;
; with no space in reach it hard-wraps at 16. V_SPLIT advances past the
; break. Past the prompt's NUL every row simply blanks itself, so a
; short prompt needs no special case. V_TMP carries the break index --
; nothing else in the render path is using it.
; Calls two down. Clobbers everything.
PRMROW: LODI,R1 0               ; the scan pass
        LODA,R0 V_SPLIT
        ADDI,R0 GOPRMPT
        STRZ    R2
        BSTA,UN RXSEEK
        LODI,R0 16
        STRA,R0 V_TMP
        LODI,R2 0               ; R2 survives RXNEXT: the scan index
PRW1:   BSTA,UN RXNEXT
        BCTR,EQ PRW4            ; the NUL: nothing left to break
        COMI,R0 ' '
        BCFR,EQ PRW2
        LODZ    R2
        STRA,R0 V_TMP
PRW2:   LODZ    R2
        ADDI,R0 1
        STRZ    R2
        COMI,R2 16
        BCTR,LT PRW1
PRW4:   LODI,R1 0               ; the draw pass, from the row's start
        LODA,R0 V_SPLIT
        ADDI,R0 GOPRMPT
        STRZ    R2
        BSTA,UN RXSEEK          ; seek BEFORE positioning: both take R2
        DCOL    ANORM
        LODA,R0 V_CAT
        STRZ    R1
        LODI,R2 0
        BSTA,UN DPOS
        LODA,R1 V_TMP
        BSTA,UN RXPRNT
        LODI,R0 16              ; blank the rest of the row
        SUBA,R0 V_TMP
        STRZ    R3
        BCTR,EQ PRW5
        LODI,R1 0
        BSTA,UN DFILL
PRW5:   LODA,R0 V_TMP           ; step past the break; a hard wrap at 16
        COMI,R0 16              ;   has no space to skip
        BCTR,EQ PRW6
        ADDI,R0 1
PRW6:   ADDA,R0 V_SPLIT
        STRA,R0 V_SPLIT
        RETC,UN

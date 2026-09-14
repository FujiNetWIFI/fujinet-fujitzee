; fzedge.asm -- bank 7: what the reply CHANGED.
;
; Entered from the network bank the moment a reply has landed and validated.
; Every edge in the game is decided here, once, in one vblank: which die
; rattles, whose card is on screen, which row a bot just took and is being
; held up, where the cursor goes, which rows are green, and which cue plays.
;
; IT IS A BANK OF ITS OWN because the card bank came in 507 bytes over with it
; in. A bank is 2048 bytes whatever the image is, so that was not something a
; bigger image could fix -- the work had to be split, and splitting it is what
; took the image from seven banks to fifteen.

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "fzdefs.inc"
        INCLUDE "../build/tail.inc"
FZBANK  EQU     BANKEDG
FZHASDIC EQU    1
FZHASINP EQU    1               ; unused now: disp.inc scans unconditionally
FZHASUI EQU     1               ; the shadow and the diff
FZHASSCR EQU    0
FZHASPICK EQU   1               ; the cursor parks on the best open row
FZHASCLS EQU    1               ; a class change blanks what it is leaving
FZHASSTR EQU    1               ; DICEHUD's ROLL tile
FZHASRPL EQU    0
FZHASED EQU     0
FZHASDEC EQU    0
FZHASNET EQU    0
FZHASHUD EQU    1               ; the hold markers and the ROLL tile
SNDLAST EQU     SN_BOT          ; the reveal's falling pair is the last cue
                                ;   this bank can fire

        ORG     $1000

; Entered BETWEEN frames, so DFRAME waits out the overscan the network bank
; armed. The edges are done in the first hook rather than here, because two
; SCSCANs and a compose do not fit in an overscan and the vblank is bigger.
EENTRY: lda     #0
        sta     FZREDRW
        jmp     DLOOP

; TWO PASSES, not one. The edges alone are two SCSCANs -- each walking
; sixteen score words through a sixteen-bit pointer -- a screen blank and a
; cursor search, and staging the tray on top of that ran the vblank to 270
; lines. The tray is a frame of its own.
APPVBL: lda     FZREDRW
        bne     EDG1
        inc     FZREDRW
        jmp     CEDGE
EDG1:   cmp     #1
        bne     EDONE
        inc     FZREDRW
        jsr     CESTAGE         ; the shadow, which is the other SCSCAN
        lda     FZCLS
        cmp     #CLPLAY
        bne     EDG2
        jsr     DSTAGE
        jmp     DICEHUD
EDG2:   rts

EDONE:  lda     #ENGAME
        sta     FZENT
        lda     #BANKCRD
        jmp     FZGOTO

; ---------------------------------------------------------------------------
; CEDGE -- everything this reply says has changed.
CEDGE:
; A FULL CLEAR parks the shadow with every bit set and mutes both edges: after
; a class change, or a seat count change where the shadow would belong to a
; different player, no bit can have turned on, so nothing can be revealed from
; a comparison that was never valid.
        lda     FZPCNT
        cmp     FZPRVPC
        bne     CECLR
        lda     FZCLS
        cmp     FZPRVCL
        beq     CE1
CECLR:  jsr     SHADALL
        jsr     FNCLS           ; the text planes survive a bank switch, so a
                                ;   class change must blank what it is leaving
        lda     #$FF
        sta     FZPRVAC
        sta     FZPRVRL
        jsr     EDTURNRS
CE1:    lda     FZPCNT
        sta     FZPRVPC
        lda     FZCLS
        sta     FZPRVCL

; ---- the roll edge ----
; A DECREASE in rollsLeft. A fresh turn RAISES it, and animating that would
; rattle dice that have not moved. $FF mutes the edge across a screen change.
;
; FZCMOD carries the answer down to the held mask below: a roll having gone
; out is exactly the moment the wire's echo becomes the truth again.
        lda     #0
        sta     FZCMOD
        lda     FZPRVRL
        cmp     #$FF
        beq     CE2A            ; muted: treat it as a fresh turn and resync
        lda     FZRLEFT
        cmp     FZPRVRL
        bcs     CE2             ; same or higher: not a roll
        jsr     ROLLANM
        lda     FZFRAME         ; seed the shake, so two consoles watching the
        ora     #1              ;   same table do not rattle in step
        sta     FZRSD0
CE2A:   lda     #1
        sta     FZCMOD
CE2:    lda     FZRLEFT
        sta     FZPRVRL

; ---- the held mask ----
;
; ON YOUR OWN TURN THE BUTTON OWNS IT, and the wire must not be allowed to
; overwrite it. keepRoll is an ECHO of the last roll that ACTUALLY WENT OUT --
; it does not move until the next one does -- so restaging from it between
; rolls throws away every hold made since. The symptom is not subtle and it is
; not a redraw bug: the dice you picked sit there held, and then deselect
; themselves at the next poll, a second and a half later.
;
; So it is resynced only when the echo is the truth or when nothing local can
; be lost: a roll has just gone out, the turn has just changed, or it is not
; your turn at all.
        lda     FZCMOD
        bne     CEKS            ; a roll landed: the echo is now the truth
        lda     FZMYTRN
        beq     CEKS            ; somebody else's holds: only the wire knows
        lda     FZACT
        cmp     FZPRVAC
        beq     CE2B            ; your turn, no roll, same turn: hands off
CEKS:   jsr     KEEPSYN
CE2B:

; ---- the turn edge, and the reveal ----
        lda     #$FF
        sta     FZREVSL
        lda     FZACT
        cmp     FZPRVAC
        beq     CE7

; Somebody's turn has just ended. If it was not ours, work out which row they
; took: the wire has no "last move" field, so the only way to know is to diff
; their filled rows against the shadow staged for them last poll.
        lda     FZPRVAC
        cmp     #$FF
        beq     CE5             ; nobody was up
        cmp     #0
        beq     CE5             ; it was ours, and we know what we took
        jsr     SCSCAN
        lda     FZREVSL
        cmp     #$FF
        beq     CE5
; Point the view at them, hold the row up, and hold the poll off while it is
; read. The reveal's hold IS the poll being deferred: one counter, one
; meaning. The highlight is a COLOUR the seam line was going to write anyway,
; which is why holding up a bot's row costs two bytes and no kernel time.
        lda     FZPRVAC
        sta     FZVIEW
        lda     FZREVSL
        jsr     EDGIX
        cmp     #$FF
        beq     CE5
        clc
        adc     #RCARD0
        sta     FZHL
        lda     #CCURSOR
        sta     FZHLC
        lda     #REVFRM
        sta     FZREVHD
        lda     #SN_BOT
        jsr     SNDCUE
        jmp     CE6

CE5:    lda     FZACT           ; no reveal: follow whoever is up now
        cmp     #$FF
        beq     CE6
        sta     FZVIEW
        jsr     EDTURNRS
CE6:    lda     FZMYTRN         ; three rising tones, BEFORE the draw: an
        beq     CE7             ;   alert after the fact is not one
        lda     #SN_TURN
        jsr     SNDCUE
CE7:    lda     FZACT
        sta     FZPRVAC

; When the last roll has gone, park the cursor on the highest-paying open row:
; fire, fire, fire, fire then plays a legal, if artless, game.
CE8:    lda     FZMYTRN
        beq     CE9
        lda     FZRLEFT
        bne     CE9
        jsr     PICKBEST
CE9:
; The cursor's own highlight, unless a reveal has taken it.
        lda     FZREVSL
        cmp     #$FF
        bne     CE10
        lda     #$FF
        sta     FZHL
        lda     FZMYTRN
        beq     CE10
        lda     FZUIMD
        beq     CE10
        lda     FZSCUR
        jsr     EDGIX
        cmp     #$FF
        beq     CE10
        clc
        adc     #RCARD0
        sta     FZHL
        lda     #CCURSOR
        sta     FZHLC
; GRNBLD is NOT called here: the potential mask is about the card, and the
; card bank builds it in the same pass it draws the name row.
;
; The tray is staged in the NEXT pass, not here: see APPVBL.
;
CE10:   rts

; ---- staging the shadow, in the NEXT pass ----
;
; Two reasons it is not in the pass above. SCSCAN ARMS FZREVSL AS A SIDE
; EFFECT, and the shadow it is about to replace still belongs to the seat
; whose turn just ended -- so that comparison is between a seat and somebody
; else's history and its answer is meaningless. Done before anything read the
; real answer it overwrites it: a bot that took ONE was correctly found,
; correctly drawn, and then reported as having taken CHANCE.
;
; And it is the SECOND SCSCAN. Each one walks sixteen score words through a
; sixteen-bit pointer; two of them, a screen blank and a cursor search in one
; vblank is what was left of the frames that still ran a line long.
CESTAGE: lda   FZACT
        cmp     #$FF
        beq     CES1
        jsr     SCSCAN
        jsr     SHADST
CES1:   lda     #$FF
        sta     FZREVSL
        rts


; EDGIX -- the card ROW for score slot A, or $FF. card.inc has the same table
; under the name CARDIX; carrying a copy of fifteen bytes and six instructions
; is cheaper than carrying the rest of card.inc to reach it.
EDGIX:  ldx     #NCARDR-1
EDIX1:  cmp     ECSLOTS,x
        beq     EDIX2
        dex
        bpl     EDIX1
        lda     #$FF
        rts
EDIX2:  txa
        rts

; EDTURNRS -- a fresh turn: the cursor back on ROLL, because rolling is always
; the first move. turn.inc has the same ten bytes under the name TURNRS; this
; bank does not carry the rest of turn.inc to reach them.
EDTURNRS: lda   #0
        sta     FZUIMD
        lda     #NDICE
        sta     FZDCUR
        lda     #SCONES
        sta     FZSCUR
        rts
ECSLOTS: DB     0,1,2,3,4,5,$FF,8,9,10,11,12,13,14,$FE

        INCLUDE "fzlib.inc"
        INCLUDE "state.inc"
        INCLUDE "dice.inc"
        INCLUDE "sound.inc"
        INCLUDE "disp.inc"

        IF      * > FZART
        ERROR   "fzedge: code overflows into the reserved top page"
        ENDIF

        INCLUDE "diceart.inc"

        ORG     FZBGTA
FZBGT:  DB      CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG
        DB      CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG,CBG

        END

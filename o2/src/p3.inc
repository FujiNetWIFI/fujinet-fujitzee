; p3.inc -- page 3: what a state means, before it is drawn (E_EDGE), and
; the shadows it leaves after (E_SHADOW). The game logic is the Channel F
; port's (channelf/src/game.inc GEDGE, GSHADW, KEEPSYN, TIBEST), which is
; the Intellivision's by way of the ColecoVision and the Arcadia:
;
;   - The server's round is the state: 0 the lobby, 1-13 play, 99 over. A
;     change of screen is the only thing that draws a screen afresh.
;   - THE TURN EDGE: activePlayer stays 0 for every poll of our turn, so the
;     shadows advance at the bottom of every poll, and our turn's cue is
;     queued before the input sees it.
;   - THE ROLL EDGE IS A DECREASE of rollsLeft: a fresh turn resets it up.
;   - keepRoll is an echo of the last roll sent. On our turn HOLD (ours)
;     owns the held dice; the wire's mask is taken only when our turn
;     begins with dice already out (a rejoin lands anywhere).
;   - validScores is all zeros in the lobby (a nil slice): only play reads
;     it.
;   - Somebody else's turn ended: the slot they took is the one their card
;     has that the shadow (FILLED) didn't. It's lit, and their card shown,
;     for a second and a half.
;
; Everything a tick needs is taken here (the reply window is repainted once
; the next request is under way): the header, the dice, the potentials.

REVHOLD equ 90                  ; frames a slot somebody took stays lit
ANIMN   equ 24                  ; frames the dice shake

    PAGE_HEAD
    db e_edge&0FFh, e_shadow&0FFh
e_edge:
    jmp on_edge
e_shadow:
    jmp on_shadow

; on_edge: the header into XRAM, the screen (SCR), and what changed since
; the last state. Clobbers A, r0-r7.
on_edge:
    call hdr_copy
    call scr_pick
    call class_edge
    call turn_edge
    call roll_edge
    mov r0,#ROUND               ; game over: its cue, once
    movx a,@r0
    xrl a,#ROUND_OVER
    jnz oe_x
    mov r0,#LROUND              ; (not if it's the first we've seen)
    movx a,@r0
    xrl a,#ROUND_OVER
    jz oe_x
    xrl a,#ROUND_OVER!0FFh
    jz oe_x
    mov a,#SND_OVER
    jmp beep
oe_x:
    ret

; scr_pick: SCR = the screen this state draws, and MYTURN. Clobbers A, r0.
scr_pick:
    mov r0,#MYTURN
    clr a
    movx @r0,a
    mov r0,#ROUND
    movx a,@r0
    mov r1,#SC_LOBBY
    jz sp_put
    mov r1,#SC_OVER
    xrl a,#ROUND_OVER
    jz sp_put
    mov r0,#VIEWNG              ; play: our move if we're seated and up
    movx a,@r0
    jnz sp_card
    mov r0,#ACTV
    movx a,@r0
    jnz sp_card
    mov r0,#MYTURN
    mov a,#1
    movx @r0,a
sp_card:
    mov r1,#SC_STAND            ; the standings, if they were asked for
    mov r0,#STAND
    movx a,@r0
    jnz sp_put
    mov r1,#SC_CARD
sp_put:
    mov a,r1
    mov r0,#SCR
    movx @r0,a
    ret

; class_edge: a new class of screen (lobby, play, over) clears what the
; last one left: the reveal, the card shown, the standings, the shadows.
; Clobbers A, r0, r1.
class_edge:
    mov r0,#ROUND
    movx a,@r0
    call r_class
    mov r1,a
    mov r0,#LROUND
    movx a,@r0
    inc a
    jz ce_new                   ; the first state
    dec a
    call r_class
    xrl a,r1
    jz ce_seats
ce_new:
    mov r0,#VIEW
    clr a
    movx @r0,a
    mov r0,#REVTK
    movx @r0,a
    mov r0,#HOLD
    movx @r0,a
    mov r0,#ANIM
    movx @r0,a
    mov r0,#LNSEAT              ; no shadow to diff against
    movx @r0,a
    mov a,#0FFh
    mov r0,#REVSL
    movx @r0,a
    mov r0,#LACTV
    movx @r0,a
    mov r0,#LROLLS
    movx @r0,a
    clr a                       ; the standings put away
    mov r0,#STAND
    movx @r0,a
ce_x:
    ret
ce_seats:                       ; somebody came or went: the shadow may be
    mov r0,#NSEAT               ; someone else's now
    movx a,@r0
    mov r1,a
    mov r0,#LNSEAT
    movx a,@r0
    xrl a,r1
    jz ce_x
    mov r0,#LACTV
    mov a,#0FFh
    movx @r0,a
    ret

; r_class: A = the class of round A: 0 the lobby, 1 play, 2 over.
r_class:
    jz rc_x
    xrl a,#ROUND_OVER
    jz rc_over
    mov a,#1
rc_x:
    ret
rc_over:
    mov a,#2
    ret

    if $ > 900h
    error "page $8 has run out"
    endif

; ===========================================================================
    org 900h
    include "rx.inc"
    include "beep.inc"

; hdr_copy: the header's fields into XRAM; the dice, their echo, and in
; play the potentials (0FFh everywhere else); the clock's second begun.
; Clobbers A, r0-r7.
hdr_copy:
    clr a
    mov r2,a
    call rx_winr                ; the header: all in the first window
    mov r5,#hdr_tab&0FFh
hc_l:
    mov a,r5
    movp a,@a                   ; the field's offset (0FFh: done)
    inc a
    jz hc_dice
    dec a
    call rx_get
    mov r4,a
    inc r5
    mov a,r5
    movp a,@a                   ; where it goes
    mov r0,a
    mov a,r4
    movx @r0,a
    inc r5
    jmp hc_l
hc_dice:
    mov r0,#SECF
    mov a,#60
    movx @r0,a
    mov r3,#0                   ; the dice ('1'-'6', 0 none) and the echo
    mov r1,#DICE
    mov r2,#0                   ; (WKEEP built in r2)
hc_d:
    mov a,r3
    add a,#GAME_DICE
    call rx_get
    movx @r1,a
    mov a,r3
    add a,#GAME_KEEP
    call rx_get
    xrl a,#'1'
    add a,#0FFh                 ; carry: not '1'
    mov a,r2
    rrc a
    mov r2,a
    inc r1
    inc r3
    mov a,r3
    xrl a,#NDICE
    jnz hc_d
    mov a,r2                    ; bits 7-3, inverted: die k = bit k
    cpl a
    rr a
    rr a
    rr a
    anl a,#(1<<NDICE)-1
    mov r0,#WKEEP
    movx @r0,a
    mov r1,#VALID               ; the potentials: in play only
    mov r3,#0
hc_v:
    mov r6,#0FFh
    mov r0,#ROUND
    movx a,@r0
    jz hc_vput
    xrl a,#ROUND_OVER
    jz hc_vput
    mov a,r3
    add a,#GAME_VALID
    call rx_get
    mov r6,a
hc_vput:
    mov a,r6
    movx @r1,a
    inc r1
    inc r3
    mov a,r3
    xrl a,#NVALID
    jnz hc_v
    ret

hdr_tab:                        ; (offset, where)
    db GAME_COUNT, NSEAT
    db GAME_ROUND, ROUND
    db GAME_ROLLS, ROLLS
    db GAME_ACTIVE, ACTV
    db GAME_MOVETIME, MOVET
    db GAME_VIEWING, VIEWNG
    db 0FFh

    if $ > 0A00h
    error "page $9 has run out"
    endif

; ===========================================================================
    org 0A00h

; on_shadow: the shadows, at the bottom of every state: the slots the seat
; to move has filled (FILLED: the next turn edge diffs them), and the
; state's round, seat, rolls and seat count. Clobbers A, r0-r7.
on_shadow:
    mov r0,#ACTV
    movx a,@r0
    mov r6,a
    inc a
    jz os_keep                  ; nobody: the shadow as it is
    mov a,r6
    call seat_ok
    jnc os_keep
    mov a,r6
    call fill_bits
    mov r0,#FILLED
    mov a,r2
    movx @r0,a
    inc r0
    mov a,r3
    movx @r0,a
os_keep:
    mov r0,#ROUND
    movx a,@r0
    mov r0,#LROUND
    movx @r0,a
    mov r0,#ACTV
    movx a,@r0
    mov r0,#LACTV
    movx @r0,a
    mov r0,#ROLLS
    movx a,@r0
    mov r0,#LROLLS
    movx @r0,a
    mov r0,#NSEAT
    movx a,@r0
    mov r0,#LNSEAT
    movx @r0,a
    ret

; seat_ok: carry if seat A is one the state has (A < NSEAT). Clobbers A,
; r0, r1.
seat_ok:
    cpl a
    mov r1,a
    mov r0,#NSEAT
    movx a,@r0
    add a,r1                    ; ~seat + NSEAT: carry iff NSEAT > seat
    ret

; fill_bits: r3:r2 = the slots seat A has filled, bit k slot k (its score's
; high byte isn't $FF); the computed ones (6, 7, 15) never. Clobbers A,
; r0, r1, r4-r7.
fill_bits:
    call seat_win
    mov r4,#0                   ; slot
    mov r2,#0
    mov r3,#0
fb_l:
    mov a,r4                    ; its high byte: 2k + 1 on
    rl a
    add a,r7
    inc a
    call rx_get
    inc a                       ; carry: $FF, unset
    add a,#0FFh
    mov a,r3                    ; r3:r2 >>= 1, the bit in at the top
    rrc a
    mov r3,a
    mov a,r2
    rrc a
    mov r2,a
    inc r4
    mov a,r4
    xrl a,#NSLOTS
    jnz fb_l
    mov a,r2                    ; (carry was set: filled)
    anl a,#0FFh-((1<<SL_UP)|(1<<SL_BONUS))
    mov r2,a
    mov a,r3
    anl a,#7Fh                  ; (slot 15)
    mov r3,a
    ret

; seat_win: the window on seat A's scores: r7 = slot 0's offset in it.
; Clobbers A, r0, r1, r2, r5, r6.
seat_win:
    mov r5,a                    ; 95 + 42A + 10
    mov r2,#GAME_SEATS+SE_SCORES
    mov r6,#0
    inc r5
    jmp swn_t
swn_l:
    mov a,r2
    add a,#SEAT_SIZE
    mov r2,a
    clr a
    addc a,r6
    mov r6,a
swn_t:
    djnz r5,swn_l
    mov a,r2                    ; window r6:r2 / 16, the offset r2 & 15
    anl a,#0Fh
    mov r7,a
    mov a,r2
    anl a,#0F0h
    orl a,r6
    swap a
    mov r2,a
    jmp rx_winr

; low_slot: A = the lowest slot set in r3:r2 (0FFh: none). Clobbers r4.
low_slot:
    mov r4,#0
ls_l:
    mov a,r3                    ; r3:r2 >>= 1, the bit out in carry
    rrc a
    mov r3,a
    mov a,r2
    rrc a
    mov r2,a
    jc ls_hit
    inc r4
    mov a,r4
    xrl a,#NSLOTS
    jnz ls_l
    mov a,#0FFh
    ret
ls_hit:
    mov a,r4
    ret

    if $ > 0B00h
    error "page $A has run out"
    endif

; ===========================================================================
    org 0B00h

; turn_edge: the seat to move has changed. The one who just finished, if
; not us: the slot they took, lit, their card shown (REVSL, REVTK, VIEW).
; Else the card follows whoever is up. Our turn begun: its cue, the
; cursor on ROLL, our held dice from the wire if there are dice out, and
; with no rolls left the cursor on the best open slot. Clobbers A, r0-r7.
turn_edge:
    mov r0,#ACTV
    movx a,@r0
    mov r7,a
    mov r0,#LACTV
    movx a,@r0
    mov r6,a
    xrl a,r7
    jnz te_go
    ret
te_go:
    mov a,r6                    ; nobody was up, or we were: nothing to show
    inc a
    jz te_follow
    dec a
    jz te_follow
    call seat_ok                ; (a seat past the table's: nothing)
    jnc te_follow
    mov a,r6
    call fill_bits              ; r3:r2 their slots filled now
    mov r0,#FILLED              ; ... that weren't
    movx a,@r0
    cpl a
    anl a,r2
    mov r2,a
    inc r0
    movx a,@r0
    cpl a
    anl a,r3
    mov r3,a
    call low_slot
    inc a
    jz te_follow                ; none
    dec a
    mov r0,#REVSL
    movx @r0,a
    mov r0,#REVTK
    mov a,#REVHOLD
    movx @r0,a
    mov r0,#LACTV               ; (fill_bits has had r6, r7)
    movx a,@r0
    mov r0,#VIEW
    movx @r0,a
    mov a,#SND_BOT
    call beep
    jmp te_mine
te_follow:
    mov r0,#ACTV                ; the card follows the turn
    movx a,@r0
    mov r7,a
    inc a
    jz te_mine
    mov r0,#VIEW
    mov a,r7
    movx @r0,a
te_mine:
    mov r0,#MYTURN
    movx a,@r0
    jz te_x
    mov a,#SND_TURN
    call beep
    clr a                       ; the standings make way for the card
    mov r0,#STAND
    movx @r0,a
    mov r0,#SCR
    mov a,#SC_CARD
    movx @r0,a
    mov r0,#CUR
    mov a,#CU_ROLL
    movx @r0,a
    clr a
    mov r0,#DICE                ; dice out: we rejoined mid-turn
    movx a,@r0
    jz te_hold
    mov r0,#WKEEP
    movx a,@r0
    cpl a
    anl a,#(1<<NDICE)-1
te_hold:
    mov r0,#HOLD
    movx @r0,a
    mov r0,#ROLLS
    movx a,@r0
    jnz te_x
    jmp ti_best
te_x:
    ret

    if $ > 0C00h
    error "page $B has run out"
    endif

; ===========================================================================
    org 0C00h

; roll_edge: rollsLeft went DOWN: dice landed. The ones re-rolled (the
; wire's keepRoll echo) shake; five alike is a Fujitzee. Our last roll
; spent: the cursor to the best open slot (the turn edge never comes: we
; are still up). Clobbers A, r0-r7.
roll_edge:
    mov r0,#ROLLS
    movx a,@r0
    mov r7,a
    mov r0,#LROLLS
    movx a,@r0
    inc a
    jz re_x                     ; nothing to compare with
    dec a
    cpl a                       ; ~prev + now: carry iff now > prev ...
    add a,r7
    jc re_x
    mov r0,#LROLLS              ; ... and no carry, equal: neither went down
    movx a,@r0
    xrl a,r7
    jz re_x
    mov r0,#WKEEP
    movx a,@r0
    mov r0,#ANMASK
    movx @r0,a
    mov r0,#ANIM
    mov a,#ANIMN
    movx @r0,a
    mov r0,#DICE                ; five alike?
    movx a,@r0
    mov r6,a
    mov r2,#NDICE-1
re_5:
    inc r0
    movx a,@r0
    xrl a,r6
    jnz re_roll
    djnz r2,re_5
    mov a,#SND_FUJI
    jmp re_snd
re_roll:
    mov a,#SND_ROLL
re_snd:
    call beep
    mov r0,#MYTURN
    movx a,@r0
    jz re_x
    mov a,r7
    jnz re_x
    jmp ti_best
re_x:
    ret

; ti_best: the cursor on the open slot that would score the most (the
; first open one if none scores). Clobbers A, r0-r5.
ti_best:
    mov r4,#0                   ; the best so far
    mov r5,#0FFh                ; its slot (0FFh: none yet)
    mov r3,#0
tb_l:
    mov a,r3
    xrl a,#SL_UP
    jz tb_n
    xrl a,#SL_UP!SL_BONUS
    jz tb_n
    mov a,r3
    add a,#VALID
    mov r0,a
    movx a,@r0
    mov r2,a
    inc a
    jz tb_n                     ; closed
    mov a,r5
    inc a
    jz tb_take                  ; the first open one
    mov a,r4                    ; better? ~best + it: carry iff it > best
    cpl a
    add a,r2
    jnc tb_n
tb_take:
    mov a,r2
    mov r4,a
    mov a,r3
    mov r5,a
tb_n:
    inc r3
    mov a,r3
    xrl a,#NVALID
    jnz tb_l
    mov a,r5
    inc a
    jz tb_x
    mov a,r5
    add a,#CU_CARD
    mov r0,#CUR
    movx @r0,a
tb_x:
    ret

    if $ > 0D00h
    error "page $C has run out"
    endif

    PAGE_TAIL

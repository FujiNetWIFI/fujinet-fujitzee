; p4.inc -- page 4: every pass of the main loop. The clock, the dice
; shaking, a taken slot's light going out; in the lobby the button says
; we're ready (a toggle: the server's /ready is one); on the card our
; move -- the cursor over the card, the dice and ROLL (sprite 0), the
; button scoring a slot, holding a die or rolling (or keys 1-5 and R),
; and T1 saying what's under the cursor; otherwise the stick pages through
; the others' cards. It reads nothing of the reply: page 3 took what it
; needs (DICE, VALID, ROLLS ...).

    PAGE_HEAD
    db e_tick&0FFh
e_tick:
    jmp on_tick

on_tick:
    call clock
    call shake
    call rev_tick
    mov r0,#LSCR
    movx a,@r0
    jz ot_card
    xrl a,#SC_LOBBY
    jnz ot_x
    jmp lobby_tick
ot_card:
    mov r0,#MYTURN
    movx a,@r0
    jz ot_other
    call turn_tick
    jmp ot_draw
ot_other:
    call page_tick
ot_draw:
    call dice_draw
    jmp cur_draw
ot_x:
    ret

; clock: a second gone: the clock to move ticks down (and stops at 0: the
; server has scored for us by then); on our move its last ten beep.
; Clobbers A, r0-r2.
clock:
    mov r0,#SECF
    movx a,@r0
    dec a
    movx @r0,a
    jnz ck_x
    mov a,#60
    movx @r0,a
    mov r0,#MOVET
    movx a,@r0
    jz ck_x
    dec a
    movx @r0,a
    jz ck_x
    add a,#256-11
    jc ck_x
    mov r0,#MYTURN
    movx a,@r0
    jz ck_x
    mov a,#SND_TICK
    jmp beep
ck_x:
    ret

; shake: the dice's frames of shaking, counted down. Clobbers A, r0.
shake:
    mov r0,#ANIM
    movx a,@r0
    jz sk_x
    dec a
    movx @r0,a
sk_x:
    ret

; rev_tick: a taken slot lit long enough: its light out, the card back to
; whoever is up, redrawn with a state at once. Clobbers A, r0, r1.
rev_tick:
    mov r0,#REVTK
    movx a,@r0
    jz rt_x
    dec a
    movx @r0,a
    jnz rt_x
    mov r0,#REVSL
    mov a,#0FFh
    movx @r0,a
    mov r0,#ACTV
    movx a,@r0
    mov r1,a
    inc a
    jnz rt_v
    mov r1,#0
rt_v:
    mov r0,#VIEW
    mov a,r1
    movx @r0,a
    clr a
    mov r0,#POLLW
    movx @r0,a
rt_x:
    ret

; busy: A = nonzero while an action of ours is queued or under way (the
; next state says what it did). Clobbers r0.
busy:
    mov r0,#ACT
    movx a,@r0
    jnz by_x
    mov r0,#NSTATE
    movx a,@r0
    jz by_x
    mov r0,#ACTSENT
    movx a,@r0
    xrl a,#ACT_STATE
by_x:
    ret

; lobby_tick: the button: ready, or not (the server's /ready toggles).
; Clobbers A, r0, r1.
lobby_tick:
    if AUTOPLAY
    mov r0,#MYREADY             ; not ready: a second, then the button
    movx a,@r0
    jnz lt_btn
    mov r0,#AUTOT
    movx a,@r0
    inc a
    movx @r0,a
    add a,#256-60
    jnc lt_btn
    clr a
    movx @r0,a
    jmp lt_go
lt_btn:
    endif
    mov r0,#JOYEV
    movx a,@r0
    anl a,#J_FIRE
    jz lt_x
lt_go:
    call busy
    jnz lt_x
    mov r0,#ACT
    mov a,#ACT_READY
    movx @r0,a
    mov a,#SND_READY
    jmp beep
lt_x:
    ret

; page_tick: not our move: left and right page through the cards (the
; card is drawn from a state, asked for at once). Clobbers A, r0, r1.
page_tick:
    mov r0,#JOYEV
    movx a,@r0
    jb2 pg_left
    jb3 pg_right
    ret
pg_left:
    mov r0,#VIEW
    movx a,@r0
    jnz pg_l1
    mov r0,#NSEAT
    movx a,@r0
pg_l1:
    dec a
    jmp pg_set
pg_right:
    mov r0,#VIEW
    movx a,@r0
    inc a
    mov r1,a
    mov r0,#NSEAT
    movx a,@r0
    xrl a,r1
    jz pg_set                   ; (A = 0: around to us)
    mov a,r1
pg_set:
    mov r0,#VIEW
    movx @r0,a
    clr a
    mov r0,#POLLW
    movx @r0,a
    mov a,#SND_MOVE
    jmp beep

    if $ > 900h
    error "page $8 has run out"
    endif

; ===========================================================================
    org 900h
    include "disp.inc"

; ===========================================================================
    org 0A00h
    include "beep.inc"

; turn_tick: our move. The stick moves the cursor; the button (or a key:
; 1-5 a die, R ROLL) acts on what is under it. Clobbers A, r0-r7.
turn_tick:
    if AUTOPLAY
    call auto
    endif
    mov r0,#KEY
    movx a,@r0
    xrl a,#K_R
    jz tt_roll
    movx a,@r0
    add a,#256-K_1              ; 1-5: die 0-4
    jnc tt_stick
    mov r6,a
    add a,#256-NDICE
    jc tt_stick
    jmp tt_hold
tt_stick:
    mov r0,#JOYEV
    movx a,@r0
    jb4 tt_fire
    anl a,#J_DIRS
    jz tt_x
    jmp cur_move
tt_fire:
    mov r0,#CUR
    movx a,@r0
    mov r6,a
    add a,#256-CU_CARD
    jc tt_score
    mov a,r6
    xrl a,#CU_ROLL
    jnz tt_hold
tt_roll:
    call busy                   ; ROLL: rolls left, a die not held
    jnz tt_bad
    mov r0,#ROLLS
    movx a,@r0
    jz tt_bad
    mov r0,#HOLD
    movx a,@r0
    mov r6,a
    xrl a,#(1<<NDICE)-1
    jz tt_bad
    mov r0,#ACTARG
    mov a,r6
    movx @r0,a
    mov r0,#ACT
    mov a,#ACT_ROLL
    movx @r0,a
tt_x:
    ret
tt_hold:                        ; die r6: held or let go, once rolled
    mov r0,#DICE
    movx a,@r0
    jz tt_bad
    mov r0,#ROLLS
    movx a,@r0
    jz tt_bad
    mov a,r6
    inc a
    mov r1,a
    mov a,#80h
th_b:
    rl a
    djnz r1,th_b
    mov r1,a
    mov r0,#HOLD
    movx a,@r0
    xrl a,r1
    movx @r0,a
    anl a,r1
    jnz th_held
    mov a,#SND_FREE
    jmp beep
th_held:
    mov a,#SND_HOLD
    jmp beep
tt_score:                       ; slot A: open, and something rolled
    mov r6,a
    call busy
    jnz tt_bad
    mov r0,#DICE
    movx a,@r0
    jz tt_bad
    mov a,r6
    add a,#256-NVALID
    jc tt_bad
    mov a,r6
    add a,#VALID
    mov r0,a
    movx a,@r0
    inc a
    jz tt_bad
    mov r0,#ACTARG
    mov a,r6
    movx @r0,a
    mov r0,#ACT
    mov a,#ACT_SCORE
    movx @r0,a
    mov a,#SND_SCORE
    jmp beep
tt_bad:
    mov a,#SND_ERROR
    jmp beep

    if $ > 0B00h
    error "page $A has run out"
    endif

; ===========================================================================
    org 0B00h

; cur_draw: sprite 0 where the cursor is -- on our move only -- unless it
; is there already (LCUR). Clobbers A, r0-r4.
cur_draw:
    mov r3,#0FFh                ; hidden, unless it's our move
    mov r0,#MYTURN
    movx a,@r0
    jz cd_cmp
    mov r0,#CUR
    movx a,@r0
    mov r3,a
cd_cmp:
    mov r0,#LCUR
    movx a,@r0
    xrl a,r3
    jz cd_x
    mov a,r3
    movx @r0,a
    inc a
    jz cd_off
    dec a
    add a,#256-CU_CARD
    jc cd_card
    mov a,r3
    xrl a,#CU_ROLL
    jz cd_roll
    mov a,r3                    ; a die: its row
    add a,#DICEROW
    mov r4,#CURDX
    jmp cd_row
cd_roll:
    mov r3,#Y_T1+CURDY
    mov r4,#CURTX
    jmp cd_put
cd_card:
    mov r3,a
    mov r4,#CURLX
    anl a,#8
    jz cd_l
    mov r4,#CURRX
cd_l:
    mov a,r3
    anl a,#7
cd_row:
    rl a                        ; Y_ROW0 + 24 x row
    rl a
    rl a
    mov r3,a
    rl a
    add a,r3
    add a,#Y_ROW0+CURDY
    mov r3,a
    jmp cd_put
cd_off:
    mov r3,#SPR_OFF
    mov r4,#0
cd_put:
    mov r2,#SPRTAB>>8
    mov a,#SPRTAB&0FFh
    call poke_at
    mov a,r3
    call poke_b
    mov a,r4
    jmp poke_b
cd_x:
    ret

; dice_draw: the dice into their rows, when what they show has changed
; (DSIG: the faces, the held ones, the shaking). Clobbers A, r0-r7.
dice_draw:
    call held_mask
    mov r7,a
    mov r0,#ANIM                ; shaking: a new face every fourth frame
    movx a,@r0
    jz dd_s
    mov r1,#FRAMES
    mov a,@r1
    anl a,#0FCh
    add a,r7
    mov r7,a
dd_s:
    mov r0,#DICE
    mov r2,#NDICE
dd_l:
    movx a,@r0
    xch a,r7
    rl a
    add a,r7
    mov r7,a
    inc r0
    djnz r2,dd_l
    mov a,r7                    ; (0FFh: drawn afresh, never a signature)
    inc a
    jnz dd_k
    dec r7
dd_k:
    mov r0,#DSIG
    movx a,@r0
    xrl a,r7
    jz dd_x
    mov a,r7
    movx @r0,a
    jmp dice_put
dd_x:
    ret

    if AUTOPLAY
; auto: our move, played: a second to think, a roll (the server has rolled
; the first), then the slot that scores the most. Clobbers A, r0-r5.
auto:
    mov r0,#AUTOT
    movx a,@r0
    inc a
    movx @r0,a
    add a,#256-45
    jnc au_x
    clr a
    movx @r0,a
    mov r0,#ROLLS               ; (the server rolls the first: one more)
    movx a,@r0
    add a,#256-2
    jnc au_score
    mov r0,#CUR
    mov a,#CU_ROLL
    jmp au_go
au_score:
    call ti_best
    mov r0,#CUR
    movx a,@r0
au_go:
    movx @r0,a
    mov r0,#JOYEV
    mov a,#J_FIRE
    movx @r0,a
au_x:
    ret

; ti_best: the cursor on the open slot that would score the most (page
; 3's). Clobbers A, r0-r5.
ti_best:
    mov r4,#0
    mov r5,#0FFh
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
    jz tb_n
    mov a,r5
    inc a
    jz tb_take
    mov a,r4
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
    endif

    if $ > 0C00h
    error "page $B has run out"
    endif

; ===========================================================================
    org 0C00h
    include "card.inc"

; ===========================================================================
    org 0E00h

; cur_move: the stick's direction (A: J_*) moves the cursor: over the card
; (left column rows 0-5, right 0-6: the totals are no stop), right of the
; right column onto the dice, down past the last die or a column's
; bottom onto ROLL. Clobbers A, r0-r7.
cur_move:
    mov r5,a                    ; the direction
    mov r0,#CUR
    movx a,@r0
    mov r6,a
    add a,#256-CU_CARD
    jc cm_card
    mov a,r6
    xrl a,#CU_ROLL
    jz cm_roll
    mov a,r5                    ; a die, r6
    jb0 cm_dup
    jb1 cm_ddn
    jb2 cm_dleft
    ret
cm_dup:
    mov a,r6
    jz cm_none
    dec a
    jmp cm_set
cm_ddn:
    mov a,r6
    inc a
    xrl a,#NDICE
    jz cm_toroll
    xrl a,#NDICE
    jmp cm_set
cm_dleft:
    mov a,r6                    ; the right column, the die's row
    add a,#CU_CARD+8+DICEROW
    jmp cm_set
cm_roll:
    mov a,r5
    anl a,#J_UP|J_RIGHT
    jz cm_none
    mov a,#CU_DIE+NDICE-1
    jmp cm_set
cm_card:
    mov r6,a                    ; slot: r7 the column (8: right), r4 the row
    anl a,#8
    mov r7,a
    mov a,r6
    anl a,#7
    mov r4,a
    mov a,r5
    jb0 cm_up
    jb1 cm_down
    jb2 cm_left
    mov a,r7                    ; right
    jnz cm_todie
    mov a,r4
    add a,#CU_CARD+8
    jmp cm_set
cm_todie:
    mov a,r4                    ; its row's die, or the nearest
    add a,#256-DICEROW
    jc cm_d1
    clr a
cm_d1:
    mov r4,a
    add a,#256-NDICE
    jnc cm_d2
    mov r4,#NDICE-1
cm_d2:
    mov a,r4
    jmp cm_set
cm_up:
    mov a,r4
    jz cm_none
    dec a
    jmp cm_slot
cm_down:
    mov a,r4                    ; the column's last: 5 left, 6 right
    inc a
    mov r4,a
    mov a,r7
    rr a
    rr a
    rr a                        ; 1 right, 0 left
    add a,#6
    xrl a,r4
    jz cm_toroll
    mov a,r4
    jmp cm_slot
cm_left:
    mov a,r7
    jz cm_none
    mov a,r4                    ; the left column has rows 0-5
    add a,#256-6
    jnc cm_l1
    mov r4,#5
cm_l1:
    mov r7,#0
    mov a,r4
cm_slot:
    add a,r7
    add a,#CU_CARD
    jmp cm_set
cm_toroll:
    mov a,#CU_ROLL
cm_set:
    mov r0,#CUR
    movx @r0,a
    mov a,#SND_MOVE
    jmp beep
cm_none:
    ret

    if $ > 0F00h
    error "page $E has run out"
    endif

    PAGE_TAIL

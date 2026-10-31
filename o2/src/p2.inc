; p2.inc -- page 2: the game's main loop. Each pass, in the quiet stretch
; after the last band: the keyboard and the stick, a step of the request in
; flight (page 1), and when a state has come what it means (page 3), its
; screen (page 6 the card, page 7 the lobby, the standings, game over) and
; the shadows it leaves (page 3); then page 4, every pass. All by farcall
; from here, the top level (tools/checkstack.py).

POLLN   equ 30                  ; frames between polls: half a second

    PAGE_HEAD
    db e_main&0FFh              ; entry 0: from the boot, one way
e_main:
    jmp main

ml:
    call frame_sync
    call key_poll
    call joy_poll
    mov r0,#HELP                ; the help screen up: a key takes it down
    movx a,@r0
    jz ml_keys
    call help_end
    jmp ml_net
ml_keys:
    mov r0,#KEY                 ; H or ?: the help screen
    movx a,@r0
    xrl a,#K_H
    jz ml_help
    xrl a,#K_H!K_QUEST
    jz ml_help
    xrl a,#K_QUEST!K_S          ; S: the standings, and back
    jz ml_stand
    xrl a,#K_S!K_L              ; L: leave the table
    jnz ml_net
    mov r0,#ACT
    mov a,#ACT_LEAVE
    movx @r0,a
    jmp ml_net
ml_stand:
    mov r0,#STAND
    movx a,@r0
    xrl a,#1
    movx @r0,a
    call redraw
    jmp ml_net
ml_help:
    mov r0,#HELP
    mov a,#1
    movx @r0,a
    mov r5,#HC_TABLE
    mov r7,#PG_HELP
    mov a,#E_HELP
    FCALL farcall
ml_net:
    mov r7,#PG_NET
    mov a,#E_NSTEP
    FCALL farcall
    mov r0,#NSTATE
    movx a,@r0
    jnz ml_tick                 ; a request is out
    mov r0,#ACTSENT             ; we have left: the table list
    movx a,@r0
    xrl a,#ACT_LEAVE
    jnz ml_have
    mov r7,#E_TABLES
    mov a,#PG_BOOT
    jmp far_go
ml_have:
    mov r0,#NETHAVE
    movx a,@r0
    xrl a,#NH_STATE
    jnz ml_row
    clr a                       ; a state: taken
    movx @r0,a
    mov r0,#HELP                ; (not while the help is up)
    movx a,@r0
    jnz ml_idle
    mov r7,#PG_EDGE             ; what it means, and which screen
    mov a,#E_EDGE
    FCALL farcall
    mov r0,#SCR
    movx a,@r0
    mov r7,#PG_CARD
    jz ml_draw
    mov r7,#PG_TEXT
ml_draw:
    clr a                       ; (E_CARD, E_TEXT)
    FCALL farcall
    mov r7,#PG_EDGE             ; and the shadows it leaves
    mov a,#E_SHADOW
    FCALL farcall
    jmp ml_idle
ml_row:
    mov r0,#HELP                ; no state: the card's next row, if one is
    movx a,@r0                  ; left (the reply is still the last state's:
    jnz ml_idle                 ; no request has begun)
    mov r0,#RNEXT
    movx a,@r0
    xrl a,#NROWS
    jz ml_idle
    mov r0,#LSCR                ; the card's page, or the text screens'
    movx a,@r0
    mov r7,#PG_CARD
    jz ml_rgo
    inc a
    jz ml_idle                  ; (none up)
    mov r7,#PG_TEXT
ml_rgo:
    mov a,#E_ROW
    FCALL farcall
ml_idle:
    mov r0,#ACT                 ; an action goes at once; a poll waits its turn
    movx a,@r0
    jnz ml_begin
    mov r0,#POLLW
    movx a,@r0
    jz ml_begin
    dec a
    movx @r0,a
    jmp ml_tick
ml_begin:
    mov r0,#POLLW
    mov a,#POLLN
    movx @r0,a
    mov r7,#PG_NET
    mov a,#E_NBEGIN
    FCALL farcall
ml_tick:
    mov r0,#HELP                ; the table's ticks wait for its screen
    movx a,@r0
    jnz ml_snd
    mov r7,#PG_TICK
    mov a,#E_TICK
    FCALL farcall
    mov r0,#LSCR                ; our move at the card: T1 says what's
    movx a,@r0                  ; under the cursor
    jnz ml_snd
    mov r0,#MYTURN
    movx a,@r0
    jz ml_snd
    call t1_mine
ml_snd:
    call snd_go
    jmp ml

    if $ > 900h
    error "page $8 has run out"
    endif

; ===========================================================================
    org 900h
JOYREP  equ 1
    include "input.inc"

; ===========================================================================
    org 0A00h
    include "snd.inc"

main:
    mov r0,#LROUND              ; nothing shown yet
    mov a,#0FFh
    movx @r0,a
    mov r0,#LACTV
    movx @r0,a
    mov r0,#LSCR
    movx @r0,a
    mov r0,#LROLLS
    movx @r0,a
    mov r0,#REVSL
    movx @r0,a
    mov r0,#T0SIG
    movx @r0,a
    mov r0,#T1SIG
    movx @r0,a
    mov r0,#CUR
    mov a,#CU_ROLL
    movx @r0,a
    clr a
    mov r0,#POLLW
    movx @r0,a
    mov r0,#ACT
    movx @r0,a
    mov r0,#HELP
    movx @r0,a
    mov r0,#STAND
    movx @r0,a
    mov r0,#REVTK
    movx @r0,a
    mov r0,#ANIM
    movx @r0,a
    mov r0,#VIEW
    movx @r0,a
    mov r0,#HOLD
    movx @r0,a
    mov r0,#MYTURN
    movx @r0,a
    mov r0,#LNSEAT
    movx @r0,a
    mov r0,#AUTOT
    movx @r0,a
    mov r0,#RNEXT
    mov a,#NROWS
    movx @r0,a
    jmp ml


; help_end: a key or the button takes the help screen down: the table comes
; back afresh with the next state, polled at once. The key is spent.
; Clobbers A, r0.
help_end:
    mov r0,#KEY
    movx a,@r0
    xrl a,#K_NONE
    jnz he_go
    mov r0,#JOYEV
    movx a,@r0
    anl a,#J_FIRE
    jnz he_go
    ret
he_go:
    mov r0,#KEY
    mov a,#K_NONE
    movx @r0,a
    clr a
    mov r0,#JOYEV
    movx @r0,a
    mov r0,#HELP
    movx @r0,a
    ; fall into redraw

; redraw: the screen afresh with the next state, polled at once.
; Clobbers A, r0.
redraw:
    clr a
    mov r0,#POLLW
    movx @r0,a
    mov r0,#LSCR
    mov a,#0FFh
    movx @r0,a
    ret

    if $ > 0B00h
    error "page $A has run out"
    endif

; ===========================================================================
    org 0B00h
    include "disp.inc"

; ===========================================================================
    org 0C00h
    include "text.inc"

; line_say: T-line r4 = LINEBUF in TXCOL / TXCOL2, unless that's what it
; shows already (T0SIG, T1SIG). Clobbers A, r0-r7.
line_say:
    mov r0,#LINEBUF
    mov r2,#TXT_LEN+2           ; (TXCOL, TXCOL2 follow LINEBUF)
    clr a
lsg_l:
    rl a
    mov r3,a
    movx a,@r0
    add a,r3
    inc r0
    djnz r2,lsg_l
    mov r3,a
    inc a                       ; (0FFh is "draw it": never a signature)
    jnz lsg_k
    dec r3
lsg_k:
    mov r0,#T0SIG
    mov a,r4
    jz lsg_t
    mov r0,#T1SIG
lsg_t:
    movx a,@r0
    xrl a,r3
    jz lsg_x
    mov a,r3
    movx @r0,a
    jmp say_line_c
lsg_x:
    ret

; dec3: r4 (0-255) into LINEBUF at r1 as 3 characters, right-aligned, bit
; 7 set (TXCOL2). Clobbers A, r1, r4-r6.
dec3:
    mov a,r4
    mov r6,#'0'-1
d3_100:
    inc r6
    add a,#256-100
    jc d3_100
    add a,#100
    mov r5,#'0'-1
d3_10:
    inc r5
    add a,#256-10
    jc d3_10
    add a,#'0'+10
    mov r4,a
    mov a,r6                    ; hundreds, unless 0
    xrl a,#'0'
    jnz d3_h
    mov r6,#' '
d3_h:
    mov a,r6
    orl a,#80h
    movx @r1,a
    inc r1
    mov a,r6                    ; tens, unless both 0
    xrl a,#' '
    jnz d3_t
    mov a,r5
    xrl a,#'0'
    jnz d3_t
    mov r5,#' '
d3_t:
    mov a,r5
    orl a,#80h
    movx @r1,a
    inc r1
    mov a,r4
    orl a,#80h
    movx @r1,a
    ret

    if $ > 0D00h
    error "page $C has run out"
    endif

; ===========================================================================
    org 0D00h

; t1_mine: T1 on our move: under the cursor a slot's name and what it
; would score (green), or ROLL and the rolls left; and the clock, m:ss.
; Unless it shows that already. Clobbers A, r0-r7.
t1_mine:
    call lb_clear
    mov r0,#CUR
    movx a,@r0
    mov r7,a
    add a,#256-CU_CARD
    jnc tm_roll
    mov r7,a                    ; a slot: its name ...
    add a,#256-SL_UP            ; (8 characters: slots 0-5, then 8-14)
    jnc tm_n
    add a,#256-2
    jnc tm_clk                  ; (6, 7: no stop)
tm_n:
    add a,#SL_UP
    rl a
    rl a
    rl a
    add a,#names&0FFh
    mov r1,a
    mov r0,#LINEBUF
    mov r2,#8
tm_c:
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    djnz r2,tm_c
    mov r0,#DICE                ; ... and what it scores, once rolled
    movx a,@r0
    jz tm_clk
    mov a,r7
    add a,#VALID
    mov r0,a
    movx a,@r0
    mov r4,a
    inc a
    jz tm_clk
    mov r1,#LINEBUF+8
    call dec3
    jmp tm_clk
tm_roll:
    mov r0,#ROLLS
    movx a,@r0
    mov r7,a
    mov r1,#tm_rl&0FFh
    jnz tm_r
    mov r1,#tm_no&0FFh
tm_r:
    mov r0,#LINEBUF
    mov r2,#8
tm_rc:
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    djnz r2,tm_rc
    mov a,r7
    jz tm_clk
    add a,#'0'
    mov r0,#LINEBUF+6
    movx @r0,a
tm_clk:
    mov r0,#MOVET               ; the clock: m:ss
    movx a,@r0
    mov r6,#'0'-1
tm_60:
    inc r6
    add a,#256-60
    jc tm_60
    add a,#60
    mov r5,#'0'-1
tm_10:
    inc r5
    add a,#256-10
    jc tm_10
    add a,#'0'+10
    mov r0,#LINEBUF+15
    movx @r0,a
    dec r0
    mov a,r5
    movx @r0,a
    dec r0
    mov a,#':'
    movx @r0,a
    dec r0
    mov a,r6
    movx @r0,a
    mov r0,#TXCOL
    mov a,#C_WHITE
    movx @r0,a
    inc r0
    mov a,#C_GREEN
    movx @r0,a
    mov r4,#L_BOT
    jmp line_say

tm_rl:  db "ROLL X  "
tm_no:  db "NO ROLLS"
names:                          ; each slot's, 8 characters
    db "ONES    TWOS    THREES  FOURS   FIVES   SIXES   "
    db "SET OF 3SET OF 4FULL HSESM STRT LG STRT CHANCE  FUJITZEE"

    if $ > 0E00h
    error "page $D has run out"
    endif

    PAGE_TAIL


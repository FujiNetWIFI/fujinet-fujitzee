; p7.inc -- page 7: the text screens a state draws (E_TEXT) -- the lobby,
; the standings (S at the card) and game over.
;
; The lobby: the table's name in T0, the seats on rows 1-6 (a name, OK when
; it's ready; ours in cyan), the server's prompt in T1. The standings and
; game over: each seat's name and total, the leader in tan (game over: the
; prompt in T0, which names the winner). The seats are the first six
; records: the server puts the watchers after them.
;
; A state draws T0 and T1; then a step a pass (E_ROW, RNEXT) while the
; reply is still the state's: the leader's total, each seat's row (redrawn
; only when it changes: RSIG), the lobby's hint. All at once is most of a
; second's frames, and a key pressed meanwhile would never be seen.

NLIST   equ 6                   ; seats listed, rows 1-6
SEAT    equ TMP0                ; the seat being listed (row_line keeps it)

    PAGE_HEAD
    db e_text&0FFh, e_trow&0FFh
e_text:
    jmp on_text
e_trow:
    jmp text_step

on_text:
    mov r0,#SCR                 ; its screen, afresh
    movx a,@r0
    mov r6,a
    mov r0,#LSCR
    movx a,@r0
    xrl a,r6
    jz ot_up
    mov a,r6
    movx @r0,a
    call scr_text
    call rsig_clear
ot_up:
    mov r0,#SCR                 ; T0, T1 (game over's: after the leader's
    movx a,@r0                  ; found, for the winner's colour)
    xrl a,#SC_OVER
    jz ot_st
    call t_lines
ot_st:
    mov r0,#RNEXT               ; the steps from the first
    clr a
    movx @r0,a
    ret

; list_row: row 1 + SEAT: the seat's name in its colour and, in the lobby,
; OK (green) if it's ready; else its total, white or the leader's C_INFO
; -- unless that's what the row shows (RSIG).
; Clobbers A, r0-r7.
list_row:
    call lb_clear
    mov r0,#TXCOL               ; white, the second colour green
    mov a,#C_WHITE
    movx @r0,a
    inc r0
    mov a,#C_GREEN
    movx @r0,a
    mov r0,#SEAT                ; a seat the state has?
    movx a,@r0
    mov r6,a
    call seat_ok
    jnc lr_put
    mov a,r6                    ; its name: 8 in the lobby, else 7 (a
    mov r2,#GAME_SEATS+SE_NAME  ; space before the total)
    call seat_at
    mov r1,#LINEBUF
    mov r6,#8
    mov r0,#SCR
    movx a,@r0
    xrl a,#SC_LOBBY
    jz lr_n
    mov r6,#7
lr_n:
    mov a,r7
    call rx_get
    jz lr_nd
    movx @r1,a
    inc r1
    inc r7
    djnz r6,lr_n
lr_nd:
    mov r0,#SEAT                ; the seat's colour
    movx a,@r0
    call seat_col
    mov r0,#TXCOL
    movx @r0,a
lr_v:
    mov r0,#SCR
    movx a,@r0
    xrl a,#SC_LOBBY
    jnz lr_tot
    mov r0,#SEAT                ; the lobby: scores[0] is 1 when ready
    movx a,@r0
    call seat_tot0
    xrl a,#1
    jnz lr_put
    mov r0,#LINEBUF+9
    mov a,#80h|'O'
    movx @r0,a
    inc r0
    mov a,#80h|'K'
    movx @r0,a
    mov r0,#SEAT
    movx a,@r0
    jnz lr_put
    mov r0,#MYREADY             ; (ours, for AUTOPLAY)
    mov a,#1
    movx @r0,a
    jmp lr_put
lr_tot:
    mov r0,#TXCOL2              ; its total: white, the leader's C_INFO
    mov a,#C_WHITE
    movx @r0,a
    mov r0,#SEAT
    movx a,@r0
    call seat_total
    mov r0,#TMP1
    movx a,@r0
    xrl a,r4
    jnz lr_num
    mov r0,#TMP2
    movx a,@r0
    xrl a,r5
    jnz lr_num
    mov a,r4                    ; (not when it's 0: nobody leads yet)
    orl a,r5
    jz lr_num
    mov r0,#TXCOL2
    mov a,#C_INFO
    movx @r0,a
lr_num:
    mov r1,#LINEBUF+8           ; (num3 spends r5:r4)
    call num3
    mov r1,#LINEBUF+8           ; (bit 7: TXCOL2)
    mov r2,#3
lr_b:
    movx a,@r1
    orl a,#80h
    movx @r1,a
    inc r1
    djnz r2,lr_b
lr_put:
    mov r0,#LINEBUF             ; the signature
    mov r2,#NCOLS
    clr a
lrs_l:
    rl a
    mov r3,a
    movx a,@r0
    add a,r3
    inc r0
    djnz r2,lrs_l
    mov r0,#TXCOL
    rl a
    mov r3,a
    movx a,@r0
    add a,r3
    mov r3,a
    inc a
    jnz lrs_k
    dec r3
lrs_k:
    mov r0,#SEAT
    movx a,@r0
    add a,#RSIG
    mov r0,a
    movx a,@r0
    xrl a,r3
    jz lr_x
    mov a,r3
    movx @r0,a
    mov r0,#SEAT
    movx a,@r0
    inc a
    mov r4,a
    jmp row_line
lr_x:
    ret

    if $ > 900h
    error "page $8 has run out"
    endif

; ===========================================================================
    org 900h
    include "disp.inc"

; ===========================================================================
    org 0A00h
    include "text.inc"

s_hint:  db "FIRE=READY",0
s_stand: db "STANDINGS",0
s_back:  db "S: BACK",0
s_over:  db "GAME OVER",0

; t_lines: T0 and T1 for the screen: the lobby, the table's name and the
; prompt; the standings, STANDINGS and S: BACK; game over, the prompt and
; GAME OVER. Clobbers A, r0-r7.
t_lines:
    mov r0,#TXCOL
    mov a,#C_WHITE
    movx @r0,a
    inc r0
    movx @r0,a
    mov r0,#SCR
    movx a,@r0
    xrl a,#SC_LOBBY
    jz tl_lobby
    xrl a,#SC_LOBBY!SC_OVER
    jz tl_over
    call lb_clear               ; the standings
    mov a,#s_stand&0FFh
    call lb_str
    mov r4,#L_TOP
    call line_say
    call lb_clear
    mov a,#s_back&0FFh
    call lb_str
    mov r4,#L_BOT
    jmp line_say
tl_over:
    mov r0,#LEADS               ; in the winner's colour
    movx a,@r0
    call seat_col
    mov r0,#TXCOL
    movx @r0,a
    mov r7,#GAME_PROMPT         ; the prompt over both (the winner named
    mov r4,#L_TOP               ; whole), else GAME OVER under it
    call wire_line
    mov r0,#TMP0
    movx a,@r0
    mov r7,a
    call rx_get
    jz tl_ov
    mov r4,#L_BOT
    jmp wire_line
tl_ov:
    call lb_clear
    mov a,#s_over&0FFh
    call lb_str
    mov r4,#L_BOT
    jmp line_say
tl_lobby:
    mov r7,#GAME_SERVER
    mov r4,#L_TOP
    call wire_line
    mov r7,#GAME_PROMPT
    mov r4,#L_BOT
    jmp wire_line

    if $ > 0B00h
    error "page $A has run out"
    endif

; ===========================================================================
    org 0B00h
TEXTSCR equ 1
CARDSCR equ 1
    include "screen.inc"

    if $ > 0C00h
    error "page $B has run out"
    endif

; ===========================================================================
    org 0C00h
    include "rx.inc"

; wire_line: T-line r4 = the reply's string from byte r7 (in the first
; window): what fits in 16, cut at a space if a word would be split; TMP0 =
; where the rest starts (TMP1:TMP2 hold the leader's total). Clobbers A,
; r0-r7.
wire_line:
    call lb_clear
    clr a
    mov r2,a
    call rx_winr
    mov r1,#LINEBUF
    mov r6,#0                   ; how many
    mov r5,#0                   ; the last space's place (0: none)
wl_l:
    mov a,r7
    add a,r6
    call rx_get
    jz wl_end                   ; the end: all of it
    mov r3,a
    mov a,r6
    xrl a,#TXT_LEN
    jz wl_full
    mov a,r3
    xrl a,#' '
    jnz wl_c
    mov a,r6
    mov r5,a
wl_c:
    mov a,r3
    movx @r1,a
    inc r1
    inc r6
    jmp wl_l
wl_full:                        ; a 17th (r3): a space, cut there; else at
    mov a,r3                    ; the last space, if there was one
    xrl a,#' '
    jz wl_sp
    mov a,r5
    jz wl_end
    mov r6,a
    add a,#LINEBUF
    mov r1,a
wl_bl:
    mov a,#' '
    movx @r1,a
    inc r1
    mov a,r1
    xrl a,#LINEBUF+TXT_LEN
    jnz wl_bl
wl_sp:
    inc r6                      ; (past the space)
wl_end:
    mov a,r7
    add a,r6
    mov r0,#TMP0
    movx @r0,a
    ; fall into line_say

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

; text_step: step RNEXT, RNEXT on: 0 the leader's total (the lobby: our
; ready flag cleared, for list_row to set), 1-6 seat RNEXT-1's row, 7 the
; lobby's hint. Clobbers A, r0-r7.
text_step:
    mov r0,#RNEXT
    movx a,@r0
    mov r6,a
    inc a
    movx @r0,a
    mov a,r6
    jz ts_lead
    xrl a,#NLIST+1
    jz ts_hint
    dec r6
    mov r0,#SEAT
    mov a,r6
    movx @r0,a
    jmp list_row
ts_lead:
    mov r0,#SCR
    movx a,@r0
    xrl a,#SC_LOBBY
    jnz ts_l2
    mov r0,#MYREADY
    movx @r0,a
    ret
ts_l2:
    call lead_find
    mov r0,#SCR                 ; game over: its lines now, the winner known
    movx a,@r0
    xrl a,#SC_OVER
    jnz ts_x
    jmp t_lines
ts_hint:
    mov r0,#RNEXT               ; (the last step)
    mov a,#NROWS
    movx @r0,a
    mov r0,#SCR
    movx a,@r0
    xrl a,#SC_LOBBY
    jnz ts_x
    mov r0,#RSIG+7              ; once a screen
    movx a,@r0
    jz ts_x
    clr a
    movx @r0,a
    mov r4,#7
    mov a,#s_hint&0FFh
    jmp say_row
ts_x:
    ret

; rsig_clear: every row drawn with this state, and T0 and T1. Clobbers A,
; r0, r1.
rsig_clear:
    mov r0,#RSIG
    mov r1,#NROWS
    mov a,#0FFh
rs_l:
    movx @r0,a
    inc r0
    djnz r1,rs_l
    mov r0,#T0SIG
    movx @r0,a
    mov r0,#T1SIG
    movx @r0,a
    ret

; seat_col: A = seat A's colour (pcols; white past the sixth, and for
; 0FFh). Clobbers nothing else.
seat_col:
    add a,#256-6
    jc scl_w
    add a,#6+(pcols&0FFh)
    movp a,@a
    ret
scl_w:
    mov a,#C_WHITE
    ret
pcols:                          ; each seat's: you, then the others
    db C_WHITE, C_RED, C_GREEN, C_VIOLET, C_CYAN, C_TAN

    if $ > 0E00h
    error "page $D has run out"
    endif

; ===========================================================================
    org 0E00h

; seat_ok: carry if seat A is one the state has (A < NSEAT, at most
; NLIST). Clobbers A, r0, r1.
seat_ok:
    cpl a
    mov r1,a
    mov r0,#NSEAT
    movx a,@r0
    add a,r1                    ; ~seat + NSEAT: carry iff NSEAT > seat
    ret

; seat_at: the window on seat A's record from byte r2 of it: r7 = that
; byte's offset in the window (rx_winr). Clobbers A, r0, r2, r5-r7.
seat_at:
    mov r5,a                    ; 42A + r2
    mov r6,#0
    inc r5
    jmp sa_t
sa_l:
    mov a,r2
    add a,#SEAT_SIZE
    mov r2,a
    clr a
    addc a,r6
    mov r6,a
sa_t:
    djnz r5,sa_l
    mov a,r2                    ; window r6:r2 / 16, the offset r2 & 15
    anl a,#0Fh
    mov r7,a
    mov a,r2
    anl a,#0F0h
    orl a,r6
    swap a
    mov r2,a
    jmp rx_winr

; seat_tot0: A = seat A's scores[0], its low byte (the lobby's ready).
; Clobbers A, r0, r2, r5-r7.
seat_tot0:
    mov r2,#GAME_SEATS+SE_SCORES
    call seat_at
    mov a,r7
    jmp rx_get

; seat_total: r5:r4 = seat A's total: slot 15 once the server has it, else
; slots 6-14 as filled. Clobbers A, r0-r3, r5-r7.
seat_total:
    mov r2,#GAME_SEATS+SE_SCORES
    call seat_at
    mov r4,#0
    mov r5,#0
    mov a,r7                    ; slot 15: 30 on
    add a,#2*SL_TOTAL+1
    call rx_get
    inc a
    jz st_sum
    mov a,r7
    add a,#2*SL_TOTAL
    call rx_get
    mov r4,a
    mov a,r7
    add a,#2*SL_TOTAL+1
    call rx_get
    mov r5,a
    ret
st_sum:
    mov r2,#SL_UP
st_l:
    mov a,r2
    rl a
    add a,r7
    inc a
    call rx_get
    mov r3,a                    ; its high byte
    inc a
    jz st_n                     ; unset
    mov a,r2
    rl a
    add a,r7
    call rx_get
    add a,r4
    mov r4,a
    mov a,r3
    addc a,r5
    mov r5,a
st_n:
    inc r2
    mov a,r2
    xrl a,#SL_TOTAL
    jnz st_l
    ret

; lead_find: TMP1:TMP2 = the highest total of the seats listed (lo, hi),
; LEADS its seat (0FFh: nobody has scored).
; Clobbers A, r0-r7.
lead_find:
    mov r0,#LEADS               ; (none yet)
    mov a,#0FFh
    movx @r0,a
    clr a
    mov r0,#TMP1
    movx @r0,a
    mov r0,#TMP2
    movx @r0,a
    mov r0,#SEAT
    movx @r0,a
lf_l:
    mov r0,#SEAT
    movx a,@r0
    mov r6,a
    call seat_ok
    jnc lf_x
    mov a,r6
    call seat_total
    mov r0,#TMP2                ; r5:r4 > the best? hi first
    movx a,@r0
    cpl a
    add a,r5                    ; carry iff r5 > hi
    jc lf_take
    mov r0,#TMP2
    movx a,@r0
    xrl a,r5
    jnz lf_n                    ; r5 < hi
    mov r0,#TMP1
    movx a,@r0
    cpl a
    add a,r4                    ; carry iff r4 > lo
    jnc lf_n
lf_take:
    mov r0,#TMP1
    mov a,r4
    movx @r0,a
    mov r0,#TMP2
    mov a,r5
    movx @r0,a
    mov r0,#SEAT
    movx a,@r0
    mov r0,#LEADS
    movx @r0,a
lf_n:
    mov r0,#SEAT
    movx a,@r0
    inc a
    movx @r0,a
    xrl a,#NLIST
    jnz lf_l
lf_x:
    ret

    include "num3.inc"

    if $ > 0F00h
    error "page $E has run out"
    endif

    PAGE_TAIL

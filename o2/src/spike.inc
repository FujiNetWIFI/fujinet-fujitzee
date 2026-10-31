; spike.inc -- page 0 of a SPIKE build: the card engine's test pattern.
; Every row full -- both cells' labels and values, a die on rows 1-5 (two
; held: their mark down the rows), the cursor, T0 and T1. No network. For
; measuring a band's slack: PADN pads a row band (build.sh), and
; tools/rowcheck.py counts the frames that lost a line.

    PAGE_HEAD
    db e_boot&0FFh
e_boot:
    jmp boot

; ===========================================================================
    org 900h
    include "disp.inc"

; ===========================================================================
    org 0A00h

; frame_wait: until the next vblank. Clobbers A, r1, r2.
frame_wait:
    mov r1,#FRAMES
    mov a,@r1
    mov r2,a
fw_l:
    mov a,@r1
    xrl a,r2
    jz fw_l
    ret

; spk_load: the rows from spk_rows. Clobbers A, r0-r7.
spk_load:
    clr a
spk_row:
    mov r0,#ROWN
    movx @r0,a
    mov r7,a                    ; its 12 bytes: ten characters, two colours
    rl a
    add a,r7
    rl a
    rl a                        ; 12n
    add a,#spk_rows&0FFh
    mov r1,a
    mov r0,#LINEBUF
    mov r2,#10
spk_cp:
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    djnz r2,spk_cp
    mov r0,#ROWC
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    mov a,r1
    movp a,@a
    movx @r0,a
    call row_put
    mov r0,#ROWN
    movx a,@r0
    inc a
    xrl a,#NROWS
    jz spk_x
    xrl a,#NROWS
    jmp spk_row
spk_x:
    ret

spk_rows:                       ; ten characters, the values' colours
    db "1S  33K   ", C_WHITE, C_WHITE
    db "2S  64K   ", C_WHITE, C_WHITE
    db "3S  9FH 25", C_WHITE, C_WHITE
    db "4S  4SM 30", C_GREEN, C_GREEN
    db "5S  5LG 40", C_GREEN, C_GREEN
    db "6S  6CH 20", C_GREEN, C_GREEN
    db "UP 18FZ 50", C_WHITE, C_TAN
    db "BN   TT 93", C_WHITE, C_WHITE

    if $ > 0B00h
    error "page $A has run out"
    endif

; ===========================================================================
    org 0B00h
    include "text.inc"
    include "vdcreset.inc"

s_t0:   db CH_ARROW,"CLYDE    ",80h|'R',80h|'1',80h|'2',80h|'/',80h|'1',80h|'3',0
s_t1:   db "LG STRT ",80h|'4',80h|'0',"  0:45",0

; spk_t0, spk_t1: the lines -- T0 seat 1's red and the round's C_INFO; T1
; white, the value green, the clock C_INFO (past TXSPL). spk_line: line
; r4 = string A of this page (LINEBUF cleared). Clobbers A, r0-r7.
spk_t0:
    call lb_clear
    mov r0,#TXCOL
    mov a,#C_RED
    movx @r0,a
    inc r0
    mov a,#C_INFO
    movx @r0,a
    mov r4,#L_TOP
    mov a,#s_t0&0FFh
    jmp spk_line
spk_t1:
    call lb_clear
    mov r0,#TXCOL
    mov a,#C_WHITE
    movx @r0,a
    inc r0
    mov a,#C_GREEN
    movx @r0,a
    mov r0,#TXSPL
    mov a,#11
    movx @r0,a
    mov r4,#L_BOT
    mov a,#s_t1&0FFh
spk_line:
    call lb_str
    call say_line_c
    mov r0,#TXSPL
    mov a,#0FFh
    movx @r0,a
    ret

    if $ > 0C00h
    error "page $B has run out"
    endif

; ===========================================================================
    org 0C00h
CARDSCR equ 1
    include "screen.inc"

    if $ > 0D00h
    error "page $C has run out"
    endif

; ===========================================================================
    org 0D00h
    include "card.inc"

    if $ > 0E80h
    error "page $D has run out"
    endif

; ===========================================================================
    org 0E80h

boot:
    call vdc_reset
    mov r0,#TXSPL               ; (tx_build's third colour off)
    mov a,#0FFh
    movx @r0,a
    mov r0,#DICE                ; the dice: 6 5 4 3 2, the 5 and 3 held
    mov r1,#spk_dice&0FFh
    mov r2,#NDICE
bt_d:
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    djnz r2,bt_d
    mov r0,#MYTURN
    mov a,#1
    movx @r0,a
    mov r0,#HOLD
    mov a,#0Ah
    movx @r0,a
    clr a
    mov r0,#ANIM
    movx @r0,a
    call scr_card
    clr a
    mov t,a
    strt cnt
    en tcnti
    en i
    call spk_t0
    call spk_t1
    call spk_load
    mov r2,#SPRTAB>>8           ; the cursor on LG
    mov a,#SPRTAB&0FFh
    call poke_at
    mov a,#Y_ROW0+4*ROW_PITCH+CURDY
    call poke_b
    mov a,#CURRX
    call poke_b
spk_l:
    call frame_wait
    jmp spk_l

spk_dice: db "65432"

    PAGE_TAIL

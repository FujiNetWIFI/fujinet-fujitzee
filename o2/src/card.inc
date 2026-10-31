; card.inc -- the card screen: eight rows between T0 and T1 on blue, each
; two cells of the scorecard (a label and a value: slot r on the left, slot
; 8 + r on the right) and, on rows DICEROW to DICEROW+4, a die: a white cell
; of the grid in fill mode with its face, a digit, on it -- red and marked
; with a ball (sprite 1, which each row band moves down a row) when it is
; held. The grid's horizontal segments rule the card, under every row. A row's table is the fixed half's page $6 (ROWS), the grid's
; GRIDH and GRIDTAB, all written through POKE.
; One 256-byte page: these routines read its tables with MOVP. Main program.

; The tables first: the routines read them with MOVP, and run past the page.
rowb:                           ; each row's bias, -(Y/2)
K   set 0
    rept NROWS
    db (256-(Y_ROW0+K*ROW_PITCH)/2)&0FFh
K   set K+1
    endm
rowt:                           ; each row's table in ROWS (page $6)
K   set 0
    rept NROWS
    db (ROWS&0FFh)+K*ROW_TAB
K   set K+1
    endm
pcol:                           ; each card place's colour: a label's, or
    db C_CYAN, C_CYAN           ; 80h ROWC's, 81h ROWC+1's
    db 80h, 80h, 80h
    db C_CYAN, C_CYAN
    db 81h, 81h, 81h
rnd6:                           ; a shaken die's faces
    db "3615242651364152"

; row_put: row ROWN = LINEBUF's ten card characters (the labels cyan, the
; values in ROWC and ROWC+1) and its die. Clobbers A, r0-r7.
row_put:
    mov r0,#ROWN
    movx a,@r0
    mov r7,a                    ; the row
    add a,#rowb&0FFh
    movp a,@a
    mov r5,a                    ; its bias: -(Y/2)
    mov a,r7
    add a,#rowt&0FFh
    movp a,@a
    mov r2,#ROWS>>8
    call poke_at
    mov r6,#0
rp_l:
    mov a,r6
    add a,#LINEBUF
    mov r0,a
    movx a,@r0
    call gptr
    mov a,r6
    add a,#pcol&0FFh
    movp a,@a
    jb7 rp_val
    jmp rp_put
rp_val:
    anl a,#1
    add a,#ROWC
    mov r0,a
    movx a,@r0
rp_put:
    call put_cell
    inc r6
    mov a,r6
    xrl a,#C_DIE
    jnz rp_l
    call die_cell               ; the die, and its mark: sprite 1's Y
    call put_cell
    mov a,r2
    jmp poke_b

; dice_put: the five dice, as die_cell has them, into their rows (the rest
; of each row as it is). Clobbers A, r0-r7.
dice_put:
    mov r7,#DICEROW
dp_l:
    mov a,r7
    add a,#rowb&0FFh
    movp a,@a
    mov r5,a
    mov a,r7
    add a,#rowt&0FFh
    movp a,@a
    add a,#2*C_DIE
    mov r2,#ROWS>>8
    call poke_at
    call die_cell
    call put_cell
    mov a,r7                    ; the mark, ROW_SPR - 2*C_DIE - 2 on
    add a,#rowt&0FFh
    movp a,@a
    add a,#ROW_SPR
    mov r6,a
    mov a,r2                    ; (r2: the mark's Y, kept over poke_at)
    mov r4,a
    mov r2,#ROWS>>8
    mov a,r6
    call poke_at
    mov a,r4
    call poke_b
    inc r7
    mov a,r7
    xrl a,#DICEROW+NDICE
    jnz dp_l
    ret

; die_cell: row r7's die: r3/r1 its pointer (gptr, bias r5), A its colour,
; r2 its mark's Y (SPR_OFF: not held). A row without a die: a blank.
; A die shaking (ANIM, ANMASK) shows a face from rnd6; a held one is red.
; Clobbers r0, r1, r3, r4, r6.
die_cell:
    mov a,r7
    add a,#256-DICEROW
    mov r6,a                    ; the die
    add a,#256-NDICE
    jc dc_none
    mov a,r6                    ; its bit
    inc a
    mov r1,a
    mov a,#80h
dc_bit:
    rl a
    djnz r1,dc_bit
    mov r1,a
    call held_mask
    anl a,r1
    mov r4,a                    ; nonzero: held
    mov r0,#ANIM                ; shaking?
    movx a,@r0
    jz dc_face
    mov r0,#ANMASK
    movx a,@r0
    anl a,r1
    jz dc_face
    mov r0,#FRAMES              ; a face, any face
    mov a,@r0
    rr a
    rr a
    add a,r6
    add a,r6
    add a,r6
    anl a,#0Fh
    add a,#rnd6&0FFh
    movp a,@a
    mov r4,#0                   ; (not held while it shakes)
    jmp dc_ch
dc_face:
    mov a,r6
    add a,#DICE
    mov r0,a
    movx a,@r0
    jnz dc_ch
    mov a,#' '                  ; none rolled
dc_ch:
    call gptr
    mov a,r4
    jz dc_free
    mov a,r7                    ; held: red, marked
    rl a
    rl a
    rl a
    mov r2,a
    rl a
    add a,r2                    ; 24r
    add a,#Y_ROW0+HELDDY
    mov r2,a
    mov a,#C_RED
    ret
dc_free:
    mov r2,#SPR_OFF
    mov a,#C_DGREY
    ret
dc_none:
    mov a,#' '
    call gptr
    mov r2,#SPR_OFF
    mov a,#C_DGREY
    ret

; held_mask: A = the dice to show held (bit k: die k): ours (HOLD) on our
; turn, else what the last roll kept (the wire's echo: WKEEP's 0s) once
; there are dice. Clobbers r0.
held_mask:
    mov r0,#MYTURN
    movx a,@r0
    jz hm_wire
    mov r0,#HOLD
    movx a,@r0
    ret
hm_wire:
    mov r0,#DICE
    movx a,@r0
    jz hm_x
    mov r0,#WKEEP
    movx a,@r0
    cpl a
    anl a,#(1<<NDICE)-1
hm_x:
    ret

; put_cell: one single's (pointer, control): pointer r3 (gptr's), colour A,
; pointer bit 8 r1. Clobbers A, r0, r4.
put_cell:
    mov r4,a
    mov a,r3
    call poke_b
    mov a,r4
    rl a
    orl a,r1
    jmp poke_b

    ifdef CARDSCR
; The grid (fixed.inc: GRIDH's 9 segments -- bit y line y, bit 0 also line
; 8, the bottom -- then GRIDTAB's 10 columns, bit r row r). The card: a rule
; under every row, under each half (X 8-57 and 72-121: segment 3 left out
; between them); the dice the fill-mode cells of column 8, rows 1-5. The
; title: the dice alone.
tg_card:
    db 0FFh, 0FFh, 0FFh, 0, 0FFh, 0FFh, 0FFh, 0, 0
    db 0, 0, 0, 0, 0, 0, 0, 0, 3Eh, 0
tg_dice:
    db 0, 0, 0, 0, 0, 0, 0, 0, 0
    db 0, 0, 0, 0, 0, 0, 0, 0, 3Eh, 0

; scr_card: the card screen -- every row empty, the grid's rules and the
; dice's boxes, T0 and T1 blank, the sprites set up and hidden; scr_dice:
; the same with the dice's boxes alone (the title). Rows first, then the
; grid and the band list: a frame between them draws the old screen.
; Clobbers A, r0-r7.
scr_dice:
    mov a,#1
    jmp sc_go
scr_card:
    clr a
sc_go:
    mov r0,#TMP1                ; (which grid)
    movx @r0,a
    call lb_clear
    clr a
    mov r0,#ROWN
sc_r:
    movx @r0,a
    call row_put
    mov r0,#ROWN
    movx a,@r0
    inc a
    xrl a,#NROWS
    jz sc_b
    xrl a,#NROWS
    jmp sc_r
sc_b:
    mov r2,#GRIDH>>8            ; the grid: obj_setup writes GRIDH (the
    mov a,#GRIDH&0FFh           ; screen's MODE_SETUP), h_top GRIDTAB
    call poke_at
    mov r1,#tg_card&0FFh
    mov r0,#TMP1
    movx a,@r0
    jz sc_gt
    mov r1,#tg_dice&0FFh
sc_gt:
    mov r6,#19
sc_g:
    mov a,r1
    movp a,@a
    call poke_b
    inc r1
    djnz r6,sc_g
    call bands_card             ; the colour and the band list
    call spr_card
    mov r1,#MODE
    mov a,#MODE_SETUP
    mov @r1,a
    mov r0,#DSIG                ; the dice and the cursor drawn afresh
    mov a,#0FFh
    movx @r0,a
    mov r0,#LCUR
    mov a,#0FEh
    movx @r0,a
    call lb_clear
    mov r4,#L_TOP
    call say_line
    call lb_clear
    mov r4,#L_BOT
    jmp say_line
    endif

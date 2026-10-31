; p6.inc -- page 6: the card, when a state has come (after page 3 has said
; what it means): T0 (whose card, the round), when it isn't our move the
; server's prompt in T1 (page 2 has T1 on our move, page 4 the dice and the
; cursor), and row 0; then a row a pass (E_ROW) while the reply is still
; this state's -- until the next request begins. Building a row is most of
; a frame, and a whole card in one pass is eight: a key pressed and let go
; meanwhile is never seen (the BIOS reads the keyboard once a pass). A row
; is drawn only if its cells have changed (a signature a row).
;
; A cell is the slot's label and its value: white once it is filled (tan
; while it is the slot somebody just took), green while it is our move and
; it would score (a 0 is left off: T1 says it under the cursor), else
; blank. The total (slot 15) is the server's only at game over, so it is
; summed here: the upper total, its bonus and the lower slots, as filled.

    PAGE_HEAD
    db e_card&0FFh, e_row&0FFh, e_title&0FFh
e_card:
    jmp on_card
e_row:
    jmp row_next
e_title:
    jmp title

on_card:
    mov r0,#LSCR                ; the card's screen, afresh
    movx a,@r0
    xrl a,#SC_CARD
    jz oc_up
    mov a,#SC_CARD
    movx @r0,a
    call scr_card
    call rsig_clear
oc_up:
    mov r0,#VIEW                ; (a seat that has gone: ours)
    movx a,@r0
    call seat_ok
    jc oc_view
    mov r0,#VIEW
    clr a
    movx @r0,a
oc_view:
    call t0_draw
    mov r0,#MYTURN              ; T1: the prompt, unless it's our move
    movx a,@r0
    jnz oc_rows
    call t1_prompt
oc_rows:
    mov r0,#RNEXT               ; the rows from the top, row 0 now
    clr a
    movx @r0,a
    ; fall into row_next

; row_next: row RNEXT, if its cells have changed since it was drawn; RNEXT
; on. Clobbers A, r0-r7.
row_next:
    mov r0,#RNEXT
    movx a,@r0
    mov r6,a
    xrl a,#NROWS
    jz rd_x
    mov a,r6
    inc a
    movx @r0,a
    mov a,r6
    mov r0,#ROWN
    movx @r0,a
    call row_build              ; LINEBUF, ROWC; A = the signature
    mov r4,a
    inc a                       ; (0FFh is "draw it": never a signature)
    jnz rd_k
    dec r4
rd_k:
    mov r0,#ROWN
    movx a,@r0
    add a,#RSIG
    mov r0,a
    movx a,@r0
    xrl a,r4
    jz rd_n
    mov a,r4
    movx @r0,a
    jmp row_put
rd_n:
rd_x:
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

; seat_ok: carry if seat A is one the state has (A < NSEAT). Clobbers A,
; r0, r1.
seat_ok:
    cpl a
    mov r1,a
    mov r0,#NSEAT
    movx a,@r0
    add a,r1                    ; ~seat + NSEAT: carry iff NSEAT > seat
    ret

; line_say: T-line r4 = LINEBUF in TXCOL / TXCOL2, unless that's what it
; shows already (T0SIG, T1SIG: a signature of the line and its colours; a
; screen drawn afresh sets them 0FFh). Clobbers A, r0-r7.
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

; title: the boot's title on the card: five 5s (a Fujitzee), the name
; above and below. Clobbers A, r0-r7.
title:
    mov r0,#DICE
    mov r2,#NDICE
    mov a,#'5'
tt_d:
    movx @r0,a
    inc r0
    djnz r2,tt_d
    mov r0,#WKEEP               ; (none held)
    mov a,#(1<<NDICE)-1
    movx @r0,a
    clr a
    mov r0,#MYTURN
    movx @r0,a
    mov r0,#ANIM
    movx @r0,a
    call scr_card               ; (its rows blank, the dice 5s)
    mov r1,#tt_t0&0FFh
    mov r4,#L_TOP
    call tt_line
    mov r1,#tt_t1&0FFh
    mov r4,#L_BOT
tt_line:                        ; T-line r4 = the 16 at r1 of this page
    mov r0,#LINEBUF
    mov r2,#TXT_LEN
tt_c:
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    djnz r2,tt_c
    jmp say_line
tt_t0:  db "    FUJITZEE    "
tt_t1:  db "FUJINET ODYSSEY2"

    if $ > 900h
    error "page $8 has run out"
    endif

; ===========================================================================
    org 900h
    include "disp.inc"

; ===========================================================================
    org 0A00h
    include "text.inc"

s_rnd:   db "R",0
s_of13:  db "/13",0

; t0_draw: T0 = the card's seat: an arrow when it's to move, its name, and
; the round -- unless that's what it shows (T0SIG). Clobbers A, r0-r7.
t0_draw:
    call lb_clear
    mov r0,#ACTV                ; to move: the arrow (bit 7: tan)
    movx a,@r0
    mov r6,a
    mov r0,#VIEW
    movx a,@r0
    xrl a,r6
    jnz t0_name
    mov r0,#LINEBUF
    mov a,#80h|CH_ARROW
    movx @r0,a
t0_name:
    mov r0,#VIEW
    movx a,@r0
    mov r2,#GAME_SEATS+SE_NAME
    call seat_at
    mov r1,#LINEBUF+1
    mov r6,#8
t0_n:
    mov a,r7
    call rx_get
    jnz t0_c
    mov a,#' '
t0_c:
    movx @r1,a
    inc r1
    inc r7
    djnz r6,t0_n
    mov r0,#LINEBUF+10          ; "R n/13"
    mov a,#s_rnd&0FFh
    call lb_at
    mov r0,#ROUND
    movx a,@r0
    mov r6,#'0'-1               ; two digits, a leading 0 a space
t0_10:
    inc r6
    add a,#256-10
    jc t0_10
    add a,#'0'+10
    mov r0,#LINEBUF+12
    movx @r0,a
    mov a,r6
    xrl a,#'0'
    jz t0_of
    mov a,r6
    mov r0,#LINEBUF+11
    movx @r0,a
t0_of:
    mov r0,#LINEBUF+13
    mov a,#s_of13&0FFh
    call lb_at
    mov r0,#TXCOL
    mov a,#C_WHITE
    movx @r0,a
    inc r0
    mov a,#C_TAN
    movx @r0,a
    mov r4,#L_TOP
    jmp line_say

; t1_prompt: T1 = the server's prompt, its first 16 characters.
; Clobbers A, r0-r7.
t1_prompt:
    call lb_clear
    clr a
    mov r2,a
    call rx_winr
    mov r1,#LINEBUF
    mov r6,#TXT_LEN
    mov r7,#GAME_PROMPT
tp_l:
    mov a,r7
    call rx_get
    jz tp_end
    movx @r1,a
    inc r1
    inc r7
    djnz r6,tp_l
tp_end:
    mov r0,#TXCOL
    mov a,#C_WHITE
    movx @r0,a
    inc r0
    movx @r0,a
    mov r4,#L_BOT
    jmp line_say

    if $ > 0B00h
    error "page $A has run out"
    endif

; ===========================================================================
    org 0B00h
CARDSCR equ 1
    include "screen.inc"

    include "num3.inc"

    if $ > 0C00h
    error "page $B has run out"
    endif

; ===========================================================================
    org 0C00h
    include "card.inc"
    include "rx.inc"

; ===========================================================================
    org 0E00h

; row_build: row ROWN of the card shown (VIEW): LINEBUF's ten characters,
; ROWC their values' colours; A = a signature of it all. Clobbers A,
; r0-r7.
row_build:
    mov r0,#VIEW
    movx a,@r0
    mov r2,#GAME_SEATS+SE_SCORES
    call seat_at                ; r7: slot 0 in the window
    mov r0,#ROWN
    movx a,@r0
    mov r6,a                    ; the left: slot r
    mov r1,#LINEBUF
    call cell
    mov r0,#ROWC
    movx @r0,a
    mov r0,#ROWN
    movx a,@r0
    add a,#8
    mov r6,a                    ; the right: slot 8 + r
    mov r1,#LINEBUF+5
    call cell
    mov r0,#ROWC+1
    movx @r0,a
    mov r0,#LINEBUF             ; the signature
    mov r2,#10
    clr a
rbs_l:
    rl a
    mov r3,a
    movx a,@r0
    add a,r3
    inc r0
    djnz r2,rbs_l
    mov r0,#ROWC
    rl a
    mov r3,a
    movx a,@r0
    add a,r3
    rl a
    mov r3,a
    inc r0
    movx a,@r0
    add a,r3
    ret

; cell: slot r6 of the card into LINEBUF at r1 (its label, its value: 5
; characters); A = the value's colour. Its scores are in the window from
; r7 (seat_at). Clobbers A, r0-r5, r1 moved on 5.
cell:
    mov a,r6                    ; the label
    rl a
    add a,#lbls&0FFh
    mov r2,a
    movp a,@a
    movx @r1,a
    inc r1
    inc r2
    mov a,r2
    movp a,@a
    movx @r1,a
    inc r1
    mov a,r6                    ; its score
    rl a
    add a,r7
    mov r2,a
    call rx_get
    mov r4,a
    mov a,r2
    inc a
    call rx_get
    mov r5,a
    inc a
    jz ce_unset
    mov r3,#C_WHITE             ; filled: white, or tan just taken
    mov r0,#REVSL
    movx a,@r0
    xrl a,r6
    jnz ce_num
    mov r0,#REVTK
    movx a,@r0
    jz ce_num
    mov r3,#C_TAN
    jmp ce_num
ce_unset:
    mov a,r6
    xrl a,#SL_TOTAL
    jz ce_total
    mov r0,#MYTURN              ; our move, our card, dice out: what it scores
    movx a,@r0
    jz ce_blank
    mov r0,#VIEW
    movx a,@r0
    jnz ce_blank
    mov r0,#DICE
    movx a,@r0
    jz ce_blank
    mov a,r6
    add a,#256-NVALID
    jc ce_blank
    mov a,r6
    add a,#VALID
    mov r0,a
    movx a,@r0
    mov r4,a
    inc a
    jz ce_blank                 ; closed
    dec a
    jz ce_blank                 ; 0: T1 says it
    mov r5,#0
    mov r3,#C_GREEN
ce_num:
    mov a,r3
    mov r0,#TMP0
    movx @r0,a
    call num3
    mov r0,#TMP0
    movx a,@r0
    ret
ce_blank:
    mov a,#' '
    movx @r1,a
    inc r1
    movx @r1,a
    inc r1
    movx @r1,a
    inc r1
    mov a,#C_WHITE
    ret
ce_total:
    mov r4,#0                   ; the total: slots 6-14 as filled
    mov r5,#0
    mov r2,#SL_UP
ct_l:
    mov a,r2
    rl a
    add a,r7
    inc a
    call rx_get
    mov r3,a                    ; its high byte
    inc a
    jz ct_n                     ; unset
    mov a,r2
    rl a
    add a,r7
    call rx_get
    add a,r4
    mov r4,a
    mov a,r3
    addc a,r5
    mov r5,a
ct_n:
    inc r2
    mov a,r2
    xrl a,#SL_TOTAL
    jnz ct_l
    mov r3,#C_WHITE
    jmp ce_num

lbls:                           ; each slot's label
    db "1S2S3S4S5S6SUPBN3K4KFHSMLGCHFZTT"

    if $ > 0F00h
    error "page $E has run out"
    endif

    PAGE_TAIL

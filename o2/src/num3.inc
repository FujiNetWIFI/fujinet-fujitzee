; num3.inc -- num3: r5:r4 into LINEBUF at r1 as 3 characters,
; right-aligned, at most 999; r1 moved on 3. A loop, not calls: page 6
; reaches it four deep. Clobbers A, r0, r2-r6.
num3:
    mov a,r5                    ; at most 999 (03E7h)
    add a,#256-3
    jnc n3_ok
    jnz n3_max
    mov a,r4
    add a,#256-0E8h
    jnc n3_ok
n3_max:
    mov r5,#3
    mov r4,#0E7h
n3_ok:
    mov r2,#0                   ; a digit has gone out
    mov r0,#100
n3_d:                           ; r5:r4 / r0 -> a digit, r5:r4 the rest
    mov r3,#'0'
n3_l:
    mov a,r0                    ; r5:r4 - r0
    cpl a
    inc a
    add a,r4
    mov r6,a
    mov a,r5
    addc a,#0FFh                ; (the borrow: r0 > r4 takes one from r5)
    jnc n3_e                    ; it went below 0
    mov r5,a
    mov a,r6
    mov r4,a
    inc r3
    jmp n3_l
n3_e:
    mov a,r3                    ; a leading 0: a space
    xrl a,#'0'
    orl a,r2
    jz n3_sp
    mov r2,#1
    mov a,r3
    jmp n3_put
n3_sp:
    mov a,#' '
n3_put:
    movx @r1,a
    inc r1
    mov a,r0                    ; the next: 100, 10, then 1 (always shown)
    xrl a,#1
    jz n3_x
    xrl a,#1!100
    mov r0,#10
    jz n3_d
    mov r0,#1
    mov r2,#1
    jmp n3_d
n3_x:
    ret

; stick.inc -- joy_read: the stick, through the BIOS. Main program,
; P1 = P1_MAIN.


; joy_read: r4 = the stick's J_* bits. The keyboard scanner shares P2 with
; the program address, so only the BIOS's own code can read it ($38F
; getjoystick: R1 = stick; R3 up/down, R2 left/right as $FF/$01, F0 the
; button). The interrupts save and restore P1, so the stick's P1 survives
; them. Clobbers A, r1-r4, F0.
joy_read:
    mov a,#P1_JOY
    outl p1,a
    mov r1,#0
    FCALL getjoystick
    mov a,#P1_MAIN
    outl p1,a
    clr a
    mov r4,a
    jf0 jr_fire
    jmp jr_v
jr_fire:
    mov r4,#J_FIRE
jr_v:
    mov a,r3
    jz jr_h
    jb7 jr_up
    mov a,r4
    orl a,#J_DOWN
    mov r4,a
    jmp jr_h
jr_up:
    mov a,r4
    orl a,#J_UP
    mov r4,a
jr_h:
    mov a,r2
    jz jr_done
    jb7 jr_left
    mov a,r4
    orl a,#J_RIGHT
    mov r4,a
    ret
jr_left:
    mov a,r4
    orl a,#J_LEFT
    mov r4,a
jr_done:
    ret


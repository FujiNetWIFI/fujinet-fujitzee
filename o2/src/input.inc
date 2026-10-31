; input.inc -- the stick, the keyboard, the frame clock. Main program,
; P1 = P1_MAIN.

; joy_poll: the left stick into JOY, what was pressed since last time into
; JOYEV, and where the page defines JOYREP a direction held repeating
; (REP_FIRST passes, then every REP_NEXT: CONFIG's). Clobbers A, r0-r4, F0.
joy_poll:
    call joy_read
    mov r0,#JOYP                ; JOYEV = JOY & ~JOYP; JOYP = JOY
    movx a,@r0
    cpl a
    anl a,r4
    mov r3,a
    mov a,r4
    mov r0,#JOYP
    movx @r0,a
    mov r0,#JOY
    movx @r0,a
    ifdef JOYREP
    anl a,#J_DIRS               ; a direction held: its repeat
    jz jp_put
    mov r2,a
    mov a,r3
    anl a,#J_DIRS
    jnz jp_arm                  ; just pressed: the delay first
    mov r0,#REP
    movx a,@r0
    dec a
    movx @r0,a
    jnz jp_put
    mov a,#REP_NEXT
    movx @r0,a
    mov a,r3
    orl a,r2
    mov r3,a
    jmp jp_put
jp_arm:
    mov r0,#REP
    mov a,#REP_FIRST
    movx @r0,a
    endif
jp_put:
    mov a,r3
    mov r0,#JOYEV
    movx @r0,a
    ret

    include "stick.inc"

; key_poll: KEY = a key pressed since the last call (K_NONE: none). The
; BIOS scans the matrix (readkey: A = a new key 0-47, $C0 + key while it is
; held, $FF none). It keeps the last key in bank 0's r7 and comes back in
; bank 1. Call it in the quiet stretch (frame_sync): it holds the vblank off
; for up to ~400 cycles while it debounces, and runs in register bank 1,
; which a band interrupt would trample. Clobbers A, r0-r7.
key_poll:
    mov r0,#KLAST
    movx a,@r0
    mov r7,a
    FCALL readkey
    sel rb0
    mov r6,a
    mov a,r7
    mov r0,#KLAST
    movx @r0,a
    mov a,r6
    jb7 kp_none
    jb6 kp_none
    jmp kp_put
kp_none:
    mov a,#K_NONE
kp_put:
    mov r0,#KEY
    movx @r0,a
    ret

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

; frame_sync: until the frame's last band has run -- the stretch with no
; band due before the next frame's, where the keyboard scan belongs.
; Clobbers A, r1, r2.
frame_sync:
    mov r1,#QUIET
    mov a,@r1
    mov r2,a
fs_l:
    mov a,@r1
    xrl a,r2
    jz fs_l
    ret

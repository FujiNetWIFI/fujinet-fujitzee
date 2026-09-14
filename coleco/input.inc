; input.inc -- hand controller scanning, entirely through OS7.
;
; POLLER does the scanning and debouncing for us, from the NMI, into the
; controller data area the cartridge header declares. So the whole input layer
; is: read a struct, fold the joystick and the keypad into one code space, and
; turn levels into edges. No port banging, no debounce table, no scan of our
; own.
;
; The code space is the astrocade client's, deliberately: 'U' 'D' 'L' 'R' for
; the stick, 'T' for the trigger, '0'-'9' for the keypad digits, 'E' for
; cancel and '.' for accept. Keeping it identical is what let card.inc,
; turn.inc, lobby.inc and nament.inc come across from that port with their
; input handling untouched.
;
;   joystick  -> U D L R      either fire button -> T
;   keypad 0-9 -> '0'-'9'     keypad *  -> 'E'   keypad #  -> '.'
;
; AUTO-REPEAT IS PACED BY VBLANKS, NOT BY CALLS. The poll loop calls KEYSCN
; thousands of times a second, so a per-call counter runs the repeat at loop
; speed and throws the cursor across a whole page in one press. The NMI bumps
; FRAMES and the repeat ages only when that changes. (This was audible before
; it was visible: a single held direction produced a burst of key clicks.)
; Only the four directions repeat; a held trigger or keypad key reports once.

; Controller data area, as OS7 lays it out (see ControllerData in os7lib).
P1LBTN  EQU     2
P1JOY   EQU     3
P1SPIN  EQU     4
P1RBTN  EQU     5
P1KEY   EQU     6

JOYUP   EQU     01H
JOYRGHT EQU     02H
JOYDOWN EQU     04H
JOYLEFT EQU     08H
BTNFIRE EQU     40H

KEYNONE EQU     0CH             ; OS7 reports "no key" as 0CH or above
KEYSTAR EQU     0AH
KEYHASH EQU     0BH

RPTFRST EQU     20              ; vblanks before a held direction repeats
RPTNEXT EQU     6               ; ...and between repeats after that

; IINIT: enable both controllers and clear the edge state.
; The NMI handler is vectored from the cartridge header, so there is nothing
; to install here. Clobbers A,HL.
IINIT:  LD      A,0FFH
        LD      (CTLDAT),A              ; player 1 enable
        LD      (CTLDAT+1),A            ; player 2 enable
        XOR     A
        LD      (V_KEY),A
        LD      (V_RPT),A
        LD      (V_LFRM),A
        RET

; KEYSCN: A = a freshly pressed code, or 0. Clobbers A,B,C,D,E,H,L.
KEYSCN: CALL    KEYRAW
        LD      HL,V_KEY
        CP      (HL)
        JR      NZ,KSNEW
        OR      A
        RET     Z                       ; nothing held
        CALL    KSISDIR
        JR      NZ,KSHELD               ; only directions repeat
        LD      A,(FRAMES)              ; age the delay once per vblank
        LD      HL,V_LFRM
        CP      (HL)
        JR      Z,KSHELD
        LD      (HL),A
        LD      HL,V_RPT
        DEC     (HL)
        JR      NZ,KSHELD
        LD      (HL),RPTNEXT
        LD      A,(V_KEY)
        RET
KSHELD: XOR     A
        RET
KSNEW:  LD      (HL),A                  ; V_KEY = the new code
        PUSH    AF
        LD      A,(FRAMES)
        LD      (V_LFRM),A
        LD      A,RPTFRST
        LD      (V_RPT),A
        POP     AF
        OR      A
        RET

; KSISDIR: Z when A is one of the four direction codes. Clobbers B,HL.
KSISDIR:
        LD      HL,JOYMAP
        LD      B,4
KSID1:  CP      (HL)
        RET     Z
        INC     HL
        DJNZ    KSID1
        OR      A                       ; NZ: A is nonzero and not a direction
        RET

; KEYRAW: A = the code currently pressed, or 0. Stick first, then the
; trigger, then the keypad -- first match wins.
; Clobbers A,B,H,L.
KEYRAW: LD      A,(CTLDAT+P1JOY)
        AND     0FH
        JR      Z,KRFIRE
        LD      B,A                     ; keep only the lowest set bit, so a
        NEG                             ; diagonal cannot read as two keys
        AND     B
        LD      HL,JOYMAP
KRJ1:   SRL     A
        JR      C,KRJ2
        INC     HL
        JR      KRJ1
KRJ2:   LD      A,(HL)
        RET
KRFIRE: LD      A,(CTLDAT+P1LBTN)
        LD      HL,CTLDAT+P1RBTN
        OR      (HL)
        AND     BTNFIRE
        JR      Z,KRKEY
        LD      A,'T'
        RET
KRKEY:  LD      A,(CTLDAT+P1KEY)
        CP      KEYNONE
        JR      NC,KRNONE
        CP      KEYSTAR
        JR      NC,KRSPEC
        ADD     A,'0'
        RET
KRSPEC: JR      NZ,KRHASH               ; flags still from CP KEYSTAR
        LD      A,'E'                   ; keypad * -- cancel, and nothing else
        RET
KRHASH: LD      A,'.'                   ; keypad # -- accept
        RET
KRNONE: XOR     A
        RET

; Joystick bit order, lowest bit first: up, right, down, left.
JOYMAP: DB      'U','R','D','L'

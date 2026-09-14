; sound.inc -- the SN76489, written directly.
;
; Direct port writes, not OS7's SOUND_INIT / PLAY_IT / SOUND_MAN. That trio
; wants a note list and a per-sound RAM data area, which is a great deal to
; spend on a cursor click; every cue here is two or three register writes.
; This is the one place in the client where OS7 costs more than it saves.
;
; A latch byte is 1-cc-t-dddd: channel in bits 6:5, type in bit 4 (1 =
; attenuation, 0 = tone), low four data bits. A tone's upper six bits follow
; in a bare 0-dddddd byte. Attenuation 15 is silence.
; Period N gives 3579545 / (32 * N) Hz, so a HIGHER N is a LOWER note.
;
; SNDOFF IS NOT OPTIONAL POLITENESS. All four channels power up at attenuation
; 0 with their frequency registers at zero -- an immediate, permanent buzz.
; Every commercial cartridge is spared it because the BIOS title screen
; silences the chip first, and our header skips that screen. So START calls
; SNDOFF before it does anything else.
;
; Cues block on vblank rather than on a delay loop: every caller is already in
; an input or animation loop, the longest cue is under half a second, and
; waiting on frames keeps the NMI -- and so POLLER -- serviced throughout.

ATTOFF  EQU     0FH             ; attenuation 15: silent

; SNDOFF: silence all four channels. Clobbers A.
SNDOFF: LD      A,9FH           ; channel 0 attenuation 15
        OUT     (PSGPORT),A
        LD      A,0BFH
        OUT     (PSGPORT),A
        LD      A,0DFH
        OUT     (PSGPORT),A
        LD      A,0FFH          ; the noise channel too
        OUT     (PSGPORT),A
        RET

; SBEEP: tone on channel 0 at period HL, held B vblanks, then silence.
; Clobbers A,B,C,D,E,H,L.
SBEEP:  LD      A,(PREFMUT)
        OR      A
        RET     NZ
        PUSH    BC
        LD      A,L
        AND     0FH
        OR      80H             ; channel 0 tone latch, low nibble
        OUT     (PSGPORT),A
        LD      A,L
        RRA
        RRA
        RRA
        RRA
        AND     0FH
        LD      C,A
        LD      A,H
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        OR      C
        AND     3FH             ; the upper six bits
        OUT     (PSGPORT),A
        LD      A,92H           ; channel 0 attenuation 2
        OUT     (PSGPORT),A
        POP     BC
SBEEP1: CALL    WAITVB
        DJNZ    SBEEP1
        LD      A,9FH
        OUT     (PSGPORT),A
        RET

; ---- the cues ---------------------------------------------------------
; Named and pitched to match the astrocade and arcadia clients, so the game
; sounds the same on every cartridge in the family.

SNDKEY: LD      HL,0060H        ; cursor click: high and very short. It fires
        LD      B,2             ; on auto-repeat too, so anything longer
        JP      SBEEP           ; would drag the cursor.

SNDHLD: LD      HL,0048H        ; a die kept or released -- a dry tick,
        LD      B,2             ; distinct from the cursor because it
        JP      SBEEP           ; changes state

SNDSEL: LD      HL,00A0H        ; select / join / confirm
        LD      B,5
        JP      SBEEP

SNDBAD: LD      HL,01C0H        ; rejected input: a dead ROLL tile, a closed
        LD      B,6             ; row, not your turn
        JP      SBEEP

SNDTICK: LD     HL,0050H        ; one of the last seconds going by
        LD      B,1
        JP      SBEEP

SNDTRN: LD      HL,0140H        ; your turn: low, then high
        LD      B,6
        CALL    SBEEP
        LD      HL,0080H
        LD      B,8
        JP      SBEEP

SNDSCOR: LD     HL,0140H        ; a row scored: a rising chirp
        LD      B,3
        CALL    SBEEP
        LD      HL,00C0H
        LD      B,4
        JP      SBEEP

; SNDBOT: somebody else took a row. The same two notes as SNDSCOR, FALLING --
; so a bot's move and your own are told apart without looking up.
SNDBOT: LD      HL,00C0H
        LD      B,3
        CALL    SBEEP
        LD      HL,0140H
        LD      B,4
        JP      SBEEP

SNDFUJI: LD     HL,0180H        ; FUJITZEE: five rising notes
        LD      B,4
        CALL    SBEEP
        LD      HL,0140H
        LD      B,4
        CALL    SBEEP
        LD      HL,0100H
        LD      B,4
        CALL    SBEEP
        LD      HL,00C0H
        LD      B,4
        CALL    SBEEP
        LD      HL,0090H
        LD      B,10
        JP      SBEEP

SNDOVER: LD     HL,0180H        ; game over
        LD      B,5
        CALL    SBEEP
        LD      HL,0140H
        LD      B,5
        CALL    SBEEP
        LD      HL,0100H
        LD      B,10
        JP      SBEEP

; SNDROLL: start the dice rattle -- white noise at the pitch in A (0-3FH).
; NON-BLOCKING, and deliberately so: the animation owns its own timing, because
; a cue that returns at once when the sound is muted must not be the only thing
; pacing what you can see. SNDRLOF stops it.
; The noise control byte is E0H | fb<<2 | rate; rate 3 clocks the LFSR from
; tone generator 2, so the pitch can be swept, and fb set picks white noise.
; Clobbers A.
SNDROLL: PUSH   AF
        LD      A,(PREFMUT)
        OR      A
        JR      NZ,SNROL9
        LD      A,0C0H          ; tone 2 sets the noise rate
        OUT     (PSGPORT),A
        POP     AF
        PUSH    AF
        AND     3FH
        OUT     (PSGPORT),A
        LD      A,0E7H          ; white noise, clocked by tone 2
        OUT     (PSGPORT),A
        LD      A,0F4H          ; noise channel on, a little below full
        OUT     (PSGPORT),A
SNROL9: POP     AF
        RET

; SNDRLOF: silence the noise channel. Clobbers A.
SNDRLOF: LD     A,0FFH
        OUT     (PSGPORT),A
        RET

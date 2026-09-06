; sound.inc -- the 2637's single tone voice.
;
; Both sound registers are SHARED with the display, which is the whole
; gotcha here:
;   $18FD (PITCH)  b0-6 frequency, b7 is the MULTICOLOR bit. The other
;                  Arcadia clients OR it back in on every write to keep
;                  multicolor on; this one runs NORMAL colour mode, so
;                  b7 must stay CLEAR and the write is the bare pitch.
;                  Setting it here would flip the whole screen's palette.
;   $18FE (VOLUME) b0-2 loudness, b3 enable, b4 noise, b5-7 HORIZONTAL
;                  SHIFT -- volume writes touch b0-3 only so the shift
;                  stays 0 and the screen does not jump.
;
; Frequency relation (arcadia_v.cpp): 1/Freq = 2*(pitch+1)*line_period,
; so a HIGHER pitch value is a LOWER tone.
;
; Every cue is blocking and tail-calls into BEEP, so none of them costs
; a return-stack entry beyond BEEP's own. Nothing outside this file
; writes either register after DINIC.
;
; This module sits in BLOCK 2 purely for the ROM budget, so both
; registers are reached through the pointer words below.

PPITCH: DWBE    PITCH           ; sound.inc lives in block 2 for the ROM
PVOLUM: DWBE    VOLUME          ;   budget; the registers are page-0

; BEEP: frequency R1 (0-$7F), duration R3 frames.
; Calls one down (WAITVB). Clobbers R0,R3.
BEEP:   LODZ    R1
        ANDI,R0 $7F             ; b7 is MULTICOLOR: never set it
        STRA,R0 *PPITCH
        LODI,R0 $0F             ; enable + full loudness, shift 0
        STRA,R0 *PVOLUM
BEEP1:  BSTA,UN WAITVB
        BDRR,R3 BEEP1
        EORZ    R0
        STRA,R0 *PVOLUM
        STRA,R0 *PPITCH
        RETC,UN

; SNDKEY: the cursor stepped one cell -- a very short, high click.
; Deliberately 1 frame: it fires on every move including the auto-repeat,
; so anything longer would drag the cursor.
SNDKEY: LODI,R1 $06
        LODI,R3 1
        BCTA,UN BEEP

; SNDSEL: a menu selection, or a mode change between dice and scorecard.
SNDSEL: LODI,R1 $10
        LODI,R3 4
        BCTA,UN BEEP

; SNDERR: rejected input -- a dead ROLL tile, a closed row, not your turn.
SNDERR: LODI,R1 $60
        LODI,R3 12
        BCTA,UN BEEP

; SNDHLD: a die was held or released -- a small dry tick, distinct from
; the cursor click because it changes state.
SNDHLD: LODI,R1 $2C
        LODI,R3 3
        BCTA,UN BEEP

; SNDTICK: one die landing during the roll animation, and the last five
; seconds of the move clock.
SNDTICK: LODI,R1 $38
        LODI,R3 1
        BCTA,UN BEEP

; SNDROLL: the roll going out -- a two-tone tumble.
; Calls one down. Clobbers R0,R1,R3.
SNDROLL: LODI,R1 $30
        LODI,R3 3
        BSTA,UN BEEP
        LODI,R1 $1C
        LODI,R3 4
        BCTA,UN BEEP

; SNDSCOR: a category was taken.
; Calls one down. Clobbers R0,R1,R3.
SNDSCOR: LODI,R1 $18
        LODI,R3 5
        BSTA,UN BEEP
        LODI,R1 $0C
        LODI,R3 7
        BCTA,UN BEEP

; SNDFUJI: five of a kind, scored as FUJI -- the fanfare, and the only
; cue in the game that is longer than a few frames.
; Calls one down. Clobbers R0,R1,R3.
SNDFUJI: LODI,R1 $18
        LODI,R3 5
        BSTA,UN BEEP
        LODI,R1 $10
        LODI,R3 5
        BSTA,UN BEEP
        LODI,R1 $0C
        LODI,R3 5
        BSTA,UN BEEP
        LODI,R1 $06
        LODI,R3 14
        BCTA,UN BEEP

; SNDBOT: another seat just took a row. DESCENDING, where SNDSCOR and
; SNDTRN both rise -- so the ear can tell "someone else scored" from
; "you scored" without looking.
; Calls one down. Clobbers R0,R1,R3.
SNDBOT: LODI,R1 $0A
        LODI,R3 4
        BSTA,UN BEEP
        LODI,R1 $2C
        LODI,R3 8
        BCTA,UN BEEP

; SNDTRN: your turn just began -- a two-tone rising cue.
; Calls one down. Clobbers R0,R1,R3.
SNDTRN: LODI,R1 $18
        LODI,R3 6
        BSTA,UN BEEP
        LODI,R1 $0C
        LODI,R3 8
        BCTA,UN BEEP

; SNDOVER: the game ended.
; Calls one down. Clobbers R0,R1,R3.
SNDOVER: LODI,R1 $0C
        LODI,R3 10
        BSTA,UN BEEP
        LODI,R1 $18
        LODI,R3 10
        BSTA,UN BEEP
        LODI,R1 $30
        LODI,R3 20
        BCTA,UN BEEP

; chrome.inc -- everything on the screen that is not the card or the tray:
; the status row, the server's prompt, the lobby's ready list, the standings
; panel and the game-over banner.
;
; The lobby, the standings and the game-over screen are all the same picture
; with a different right-hand field, so they are one routine: a seat's name in
; eight columns, a gap, and three columns of whatever that screen is about.

RSEAT0  EQU     2               ; the seat list, on every panel screen
RMORE   EQU     11              ; "+N MORE" when the table outgrows the panel
RHINT   EQU     18              ; the one line of advice a screen gets
NSEATR  EQU     RMORE-RSEAT0    ; nine, which is also PCREND

; ---------------------------------------------------------------------------
; STATROW -- row 0. What it says depends on the class, and in play it is
; synthesised rather than taken from the wire: the server's prompt is for the
; panel screens, and this row has twelve columns to say whose turn it is and
; how long they have left.
STATROW: lda   #RSTAT
        jsr     FNROWA
        lda     FZCLS
        cmp     #CLPLAY
        beq     STAT1
        cmp     #CLOVER
        beq     STATOV
        cmp     #CLSTAND
        beq     STATST
; The lobby: the server's own name, straight out of the window.
        ldx     #GOSRVNM
        ldy     #FNTCOL
        jsr     FNRPLA
        jmp     FNENDW
STATOV: ldx     #SOVER&$FF
        ldy     #SOVER>>8
        jsr     FNSTRP
        jmp     FNENDW
STATST: ldx     #SSTAND&$FF
        ldy     #SSTAND>>8
        jsr     FNSTRP
        jmp     FNENDW

STAT1:  lda     #'R'
        jsr     FNCHR
        lda     FZRND
        sta     FZNUM0
        lda     #0
        sta     FZNUM1
        lda     #2
        jsr     FZDEC
        ldx     #SOF13&$FF
        ldy     #SOF13>>8
        jsr     FNSTRP
        lda     #2
        jsr     FNSPC
        lda     #'T'
        jsr     FNCHR
; The clock is FZCLK and not the wire's moveTime: the reply window is the
; cartridge's, so counting down in place would go nowhere, and a client that
; waited for the server to tell it the time had run out would never learn --
; the server force-scores and moves on.
        lda     FZCLK
        sta     FZNUM0
        lda     #0
        sta     FZNUM1
        lda     #3
        jsr     FZDEC
        jmp     FNENDW

SOF13:  DB      "/13",0
SOVER:  DB      "GAME OVER",0
SSTAND: DB      "STANDINGS",0

; ---------------------------------------------------------------------------
; SEATNM -- eight columns of seat X's name, padded. The record pointer is set
; here rather than by the caller, because every caller wants it set.
SEATNM: txa
        jsr     PLRECP
        ldy     #PLNAME
        ldx     #NAMEMAX
SNM1:   lda     (FNPTRL),y
        beq     SNM2
        jsr     FNCHR
        iny
        dex
        bne     SNM1
        rts
SNM2:   txa
        jmp     FNSPC

; ---------------------------------------------------------------------------
; PANEL -- the seat list. A = 0 for the lobby's ready flags, nonzero for the
; running totals the standings and the game-over screen want.
;
; scores[0] DOUBLES AS THE LOBBY READY FLAG, which is why the lobby needs no
; field of its own on the wire at all.
; FZCIX and FZCMOD, not FZIDX and FZTMP: RUNTOT builds its sixteen-bit sum in
; those two, and the running total is exactly what this loop asks it for.
; ONE SEAT PER CALL, and the caller comes back next frame for the next. A
; running total walks nine score words through a sixteen-bit pointer, and nine
; seats of that is thousands of cycles against a vblank of 2812: composing the
; whole panel in one hook ran the frame to 270 lines.
;
; Returns carry SET when the panel is finished.
PANEL:  sta     FZCMOD
PAN1:   ldx     FZCIX
        cpx     FZPCNT
        bcs     PAN6
        cpx     #NSEATR
        bcs     PAN6
        txa
        clc
        adc     #RSEAT0
        jsr     FNROWA
        ldx     FZCIX
        jsr     SEATNM
        lda     #1
        jsr     FNSPC
        lda     FZCMOD
        beq     PANRDY
; A running total: summed from slots 6-14, never read from slot 15, which is
; filled only at game over.
        jsr     RUNTOT
        lda     #CVALW
        jsr     FZDEC
        jmp     PAN5
PANRDY: ldx     #SCONES
        jsr     RDSCOR
        bcs     PANNO
        lda     FZNUM0
        cmp     #1
        bne     PANNO
        ldx     #SRDY&$FF
        ldy     #SRDY>>8
        jsr     FNSTRP
        jmp     PAN5
PANNO:  lda     #CVALW
        jsr     FNSPC
PAN5:   jsr     FNENDW
        inc     FZCIX
        clc                     ; more to do: the caller will be back
        rts

; The seats that did not fit. Twelve seats can sit at a table and nine rows
; can show them, and a panel that silently stopped at nine would be a lie
; about how many people are playing.
PAN6:   lda     FZPCNT
        cmp     #NSEATR+1
        bcc     PAN8
        sec
        sbc     #NSEATR
        sta     FZNUM0
        lda     #0
        sta     FZNUM1
        lda     #RMORE
        jsr     FNROWA
        lda     #'+'
        jsr     FNCHR
        lda     #2
        jsr     FZDEC
        ldx     #SMORE&$FF
        ldy     #SMORE>>8
        jsr     FNSTRP
        jsr     FNENDW
        sec
        rts
PAN8:   sec
        rts

; PANRST -- start a panel.
PANRST: lda     #0
        sta     FZCIX
        rts

SRDY:   DB      "RDY",0
SMORE:  DB      " MORE   ",0

; ---------------------------------------------------------------------------
; PROMPTW -- the server's prompt, word-wrapped over NPRMPTR rows of twelve.
;
; It is the only thing on the wire that names the winner, so the game-over
; screen is mostly this. Breaking at spaces rather than at column twelve is
; the difference between "ALICE WINS" and "ALICE WIN / S".
PROMPTW: lda   #0
        sta     FZIDX           ; offset into the prompt
        ldx     #0
PW1:    txa
        clc
        adc     #RPRMPT0
        jsr     FNROWA
        stx     FZTMP           ; the row
; How much of what is left fits? Walk to column FNTCOL and then back to the
; last space, unless the whole rest fits.
        ldy     FZIDX
        ldx     #0
PW2:    cpx     #FNTCOL
        bcs     PW4
        lda     FNRPLY+GOPRMPT,y
        beq     PW4             ; the end: what is left fits
        iny
        inx
        bne     PW2
PW4:    stx     FZNUM0          ; how many characters this row takes
        lda     FNRPLY+GOPRMPT,y
        beq     PW6             ; ended mid-row: no break needed
        cmp     #' '
        beq     PW6             ; broke exactly on a space
        cpx     #FNTCOL
        bcc     PW6
; Walk back to the last space in what we were going to take.
PW5:    dey
        dex
        beq     PW6             ; no space at all: take the twelve and chop
        lda     FNRPLY+GOPRMPT,y
        cmp     #' '
        bne     PW5
        stx     FZNUM0
PW6:    ldy     FZIDX
        ldx     #0
PW7:    cpx     FZNUM0
        beq     PW8
        lda     FNRPLY+GOPRMPT,y
        beq     PW8
        jsr     FNCHR
        iny
        inx
        bne     PW7
PW8:    txa                     ; pad the row out: a shorter line must not
        eor     #$FF            ;   leave the last one showing behind it
        sec
        adc     #FNTCOL
        beq     PW9
        jsr     FNSPC
; SAVE Y BEFORE RENDERING, AND RELOAD IT AFTER. FNENDW counts its bounded wait
; down in Y, so the cursor into the prompt does not survive it. The first cut
; stored Y afterwards and reloaded nothing: the first row wrapped correctly
; and every row after it started from a count of 128, which reads as a prompt
; that is one line long.
PW9:    sty     FZIDX
        jsr     FNENDW
        ldy     FZIDX
        lda     FNRPLY+GOPRMPT,y
        beq     PWPAD           ; done: blank whatever rows are left
        cmp     #' '
        bne     PW10
        inc     FZIDX           ; swallow the space we broke on
PW10:   ldx     FZTMP
        inx
        cpx     #NPRMPTR
        bcc     PW1
        rts
PWPAD:  ldx     FZTMP
PWP1:   inx
        cpx     #NPRMPTR
        bcs     PWP2
        txa
        clc
        adc     #RPRMPT0
        jsr     FNROWA
        lda     #FNTCOL
        jsr     FNSPC
        jsr     FNENDW
        jmp     PWP1
PWP2:   rts

; ---------------------------------------------------------------------------
; HINT -- one line of advice, at RHINT. X/Y is the string.
; The string is X (lo) / Y (hi), and it need not be twelve characters:
; vcs_render_row pads a short row with spaces of its own accord, so nothing
; here has to count.
HINT:   lda     #RHINT
        jsr     FNROWA
        jsr     FNSTRP
        jmp     FNENDW

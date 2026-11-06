; screen.inc -- what the table looks like.
;
;   play      row 0     the card's owner (inverse: it is their turn), Rnn, clock
;             rows 1-8  slot r (left, cols 0-4) and slot 8+r (right, cols 6-10):
;                       two cells of label, three of value
;             rows 1-5  the dice, cols 12-13, and the cursor at col 14
;             row 9     the keys on our turn, else the server's prompt
;   lobby     row 0 the table, rows 1-6 the seats in two columns (inverse:
;             ready), row 8 the prompt, row 9 the keys
;   standings the same columns: NAME TTT
;
; A label is inverse when we may score that slot now; the cursor makes the
; value inverse too. Every routine here is FCALLed and may clobber R8, R9, RB,
; RC and RA.0.

; grender -- the screen for what the window holds.
        FIT     64
grender:
        LDI     HROUND
        FCALL   rpeek
        BZ      grn1
        XRI     99
        BZ      grn2
        GETV    V_STAND
        BNZ     grn3
        LDI     2
        FCALL   gclr
        FJMP    gplay
grn1:   LDI     1
        FCALL   gclr
        FJMP    globby
grn2:   LDI     3
        FCALL   gclr
        LDR     R8, s_gover
        FJMP    gstand
grn3:   LDI     3
        FCALL   gclr
        LDR     R8, s_stand
        FJMP    gstand

; gclr -- clear the screen if it was showing another kind (D = 1 lobby,
; 2 play, 3 standings).
        FIT     24
gclr:   PLO     R9
        GETV    V_SCR
        STR     R2
        GLO     R9
        XOR
        BZ      gcl9
        GLO     R9
        PUTV    V_SCR
        FJMP    d_cls
gcl9:   FRET

; prec -- RB = player record D.
        FIT     24
prec:   PLO     RC
        LDR     RB, FN_REPLY+GPLYRS
prc1:   GLO     RC
        BZ      prc9
        LDI     PSTRIDE
        FCALL   radd
        DEC     RC
        BR      prc1
prc9:   FRET

; ptot -- R8 = the total of the record at RB: slots 6-14 that are set. (The
; server fills slot 15 only at the end.) Spoils RB and RC.
        FIT     40
ptot:   LDI     PSCORES+2*SL_UPTOT
        FCALL   radd
        LDI     0
        PLO     R8
        PHI     R8
        LDI     NSLOTS-SL_UPTOT
        PLO     RC
pt1:    LDA     RB
        PLO     R9
        LDA     RB
        XRI     0FFH
        BZ      pt2                     ; unset, or watching
        GLO     R9
        STR     R2
        GLO     R8
        ADD
        PLO     R8
        GHI     R8
        ADCI    0
        PHI     R8
pt2:    DEC     RC
        GLO     RC
        BNZ     pt1
        FRET

; gpos -- cursor to the cell of seat D in the two-column lists: six rows
; from row 1, the second six in the right half.
        FIT     24
gpos:   SMI     6
        BNF     gp1
        ADI     1
        SHL
        SHL
        SHL
        SHL
        ADI     8
        FJMP    d_at
gp1:    ADI     7
        SHL
        SHL
        SHL
        SHL
        FJMP    d_at

; ---- play ----

        FIT     40
gplay:  GETV    V_VSEAT                 ; the card on show: its record and total
        FCALL   prec
        GLO     RB
        PUTV    V_PRECL
        GHI     RB
        PUTV    V_PRECH
        GETV    V_VSEAT
        FCALL   prec
        FCALL   ptot
        GLO     R8
        PUTV    V_TOT
        GHI     R8
        PUTV    V_TOT+1
        FCALL   gtop
        FCALL   grows
        FCALL   gdice
        FJMP    gmsg

; gtop -- row 0.
        FIT     120
gtop:   AT      00H
        GETV    V_ACT
        STR     R2
        GETV    V_VSEAT
        XOR
        BNZ     gt1
        OP      TOP_INV_ON              ; their turn
gt1:    GETV    V_VSEAT
        FCALL   prec
        LDI     0
        PHI     R8
        LDI     8
        FCALL   rpu
        OP      TOP_INV_OFF
        LDI     ' '
        FCALL   d_putc
        LDI     'R'
        FCALL   d_putc
        GETV    V_RND
        PLO     R8
        LDI     0
        PHI     R8
        LDI     2
        FCALL   dnum
        LDI     ' '
        FCALL   d_putc
        GETV    V_ACT
        XRI     0FFH
        BZ      gt3
        LDI     HCLOCK
        FCALL   rpeek
        BZ      gt3
        PLO     R8
        LDI     0
        PHI     R8
        LDI     ':'
        FCALL   d_putc
        LDI     2
        FJMP    dnum
gt3:    LDI     3
        FJMP    blanks

; grows -- rows 1-8: the card.
        FIT     80
grows:  SETV    V_ROW, 0
grw1:   GETV    V_ROW
        ADI     1
        SHL
        SHL
        SHL
        SHL
        FCALL   d_at
        GETV    V_ROW
        PUTV    V_CTR
        FCALL   gcell
        GETV    V_ROW
        ADI     1
        SHL
        SHL
        SHL
        SHL
        ADI     6
        FCALL   d_at
        GETV    V_ROW
        ADI     8
        PUTV    V_CTR
        FCALL   gcell
        GETV    V_ROW
        ADI     1
        STR     RA
        XRI     8
        BNZ     grw1
        FRET

; gcell -- slot V_CTR at the cursor: label, then value.
        FIT     200
gcell:  SETV    V_CAND, 0               ; one we may score now?
        GETV    V_VSEAT
        BNZ     gce1
        GETV    V_CTR
        FCALL   cvalid
        BZ      gce1
        SETV    V_CAND, 1
gce1:   SETV    V_ISCUR, 0              ; and the cursor is on it?
        GETV    V_CAND
        BZ      gce2
        GETV    V_CUR
        STR     R2
        GETV    V_CTR
        XOR
        BNZ     gce2
        SETV    V_ISCUR, 1
gce2:   GETV    V_CAND
        BZ      gce3
        OP      TOP_INV_ON
gce3:   LDR     RB, labels
        GETV    V_CTR
        SHL
        FCALL   radd
        LDA     RB
        FCALL   d_putc
        LDN     RB
        FCALL   d_putc
        GETV    V_ISCUR
        BNZ     gce4
        OP      TOP_INV_OFF
gce4:   GETV    V_CAND
        BZ      gce5
        GETV    V_CTR                   ; the potential
        ADI     LO(FN_REPLY+HVALID)
        PLO     RB
        LDI     HI(FN_REPLY+HVALID)
        PHI     RB
        LDN     RB
        PLO     R8
        LDI     0
        PHI     R8
        BR      gcev
gce5:   GETV    V_CTR
        XRI     SL_TOTAL
        BNZ     gce6
        LDR     RB, V_TOT               ; the running total
        FCALL   rword
        BR      gcev
gce6:   VAR     V_PRECL                 ; the record's own word
        LDA     RA
        PLO     RB
        LDN     RA
        PHI     RB
        LDI     PSCORES
        FCALL   radd
        GETV    V_CTR
        SHL
        FCALL   radd
        INC     RB
        LDN     RB
        XRI     0FFH
        BNZ     gce7
        LDI     ' '                     ; unset, or watching: a dash
        FCALL   d_putc
        LDI     ' '
        FCALL   d_putc
        LDI     '-'
        FCALL   d_putc
        BR      gce9
gce7:   DEC     RB
        FCALL   rword
gcev:   LDI     3
        FCALL   dnum
gce9:   OP      TOP_INV_OFF
        FRET

; gmsg -- row 9 on the play screen.
        FIT     80
gmsg:   AT      90H
        GETV    V_ME
        BZ      gm5
        GETV    V_CUR
        SMI     16
        BDF     gm1
        LDR     R8, s_score
        BR      gm2
gm1:    LDR     R8, s_hold
gm2:    FCALL   d_puts
        GETV    V_RL
        ADI     '0'
        FJMP    d_putc
gm5:    LDR     RB, FN_REPLY+HPROMPT
        LDI     0
        PHI     R8
        LDI     16
        FJMP    rpu

; ---- lobby ----

; globby -- the seats, who is ready, what we are waiting for.
        FIT     200
globby: AT      00H
        LDR     RB, FN_REPLY+HSNAME
        LDI     0
        PHI     R8
        LDI     16
        FCALL   rpu
        SETV    V_ROW, 0
gl1:    GETV    V_ROW
        FCALL   gpos
        GETV    V_ROW
        STR     R2
        GETV    V_PC
        SM                              ; seats - this
        BNF     gl4
        BZ      gl4
        GETV    V_ROW                   ; ready if its first score word is 1
        FCALL   prec
        LDI     PSCORES
        FCALL   radd
        LDN     RB
        XRI     1
        BNZ     gl2
        OP      TOP_INV_ON
gl2:    GETV    V_ROW
        FCALL   prec
        LDI     0
        PHI     R8
        LDI     8
        FCALL   rpu
        OP      TOP_INV_OFF
        BR      gl5
gl4:    LDI     8
        FCALL   blanks
gl5:    GETV    V_ROW
        ADI     1
        STR     RA
        XRI     12
        BNZ     gl1
        AT      80H
        LDR     RB, FN_REPLY+HPROMPT
        LDI     0
        PHI     R8
        LDI     16
        FCALL   rpu
        PUTS    90H, s_rdyleg
        FRET

; ---- standings, and the end of a game ----

; gstand -- R8 = the title. NAME TTT for each seat; the prompt (who won) at
; the end of a game, a key legend on the standings.
        FIT     200
gstand: AT      00H
        FCALL   d_puts
        OP      TOP_CLREOL
        SETV    V_ROW, 0
gs1:    GETV    V_ROW
        FCALL   gpos
        GETV    V_ROW
        STR     R2
        GETV    V_PC
        SM
        BNF     gs4
        BZ      gs4
        GETV    V_ROW
        FCALL   prec
        LDI     2
        PHI     R8
        LDI     4
        FCALL   rpu
        LDI     ' '
        FCALL   d_putc
        GETV    V_ROW
        FCALL   prec
        FCALL   ptot
        LDI     3
        FCALL   dnum
        BR      gs5
gs4:    LDI     8
        FCALL   blanks
gs5:    GETV    V_ROW
        ADI     1
        STR     RA
        XRI     12
        BNZ     gs1
        AT      90H
        GETV    V_STAND
        BZ      gs6
        LDR     R8, s_back
        FCALL   d_puts
        OP      TOP_CLREOL
        FRET
gs6:    LDR     RB, FN_REPLY+HPROMPT
        LDI     0
        PHI     R8
        LDI     16
        FJMP    rpu

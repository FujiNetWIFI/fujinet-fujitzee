; dice.inc -- the app glyphs and the dice column.
;
; A die is two glyph cells (tools/mkdice.py draws the faces into
; build/glyphs.inc). The dice stand in cells 12-13 of rows 1-5; a held die is
; drawn inverse, and column 14 carries the cursor when it is on a die.

; glyphs -- teach the cart the app glyphs.
        FIT     32
glyphs: LDR     RB, gtab
gly1:   LDA     RB
        BZ      gly3
        FCALL   d_par                   ; the code
        LDI     12
        PLO     RC
gly2:   LDA     RB
        FCALL   d_par                   ; 12 scanlines, bit 3 the leftmost
        DEC     RC
        GLO     RC
        BNZ     gly2
        LDI     TOP_GLYPH
        FCALL   d_op
        BR      gly1
gly3:   FRET

        INCLUDE "glyphs.inc"

; gdice -- the five dice. A die the server has not rolled ('0' or nothing)
; is two blanks.
        FIT     120
gdice:  SETV    V_CTR, 0
gdi1:   GETV    V_CTR
        ADI     1
        SHL
        SHL
        SHL
        SHL
        ADI     12
        FCALL   d_at
        GETV    V_CTR
        FCALL   dheld                   ; D = 1: drawn inverse
        BZ      gdi2
        OP      TOP_INV_ON
gdi2:   GETV    V_CTR
        ADI     LO(FN_REPLY+HDICE)
        PLO     RB
        LDI     HI(FN_REPLY+HDICE)
        PHI     RB
        LDN     RB
        SMI     '1'
        BNF     gdi4                    ; not a die
        SMI     6
        BDF     gdi4
        ADI     6                       ; face 0-5
        SHL
        ADI     G_DIE
        PLO     R9
        FCALL   d_putc
        GLO     R9
        ADI     1
        FCALL   d_putc
        BR      gdi5
gdi4:   LDI     ' '
        FCALL   d_putc
        LDI     ' '
        FCALL   d_putc
gdi5:   OP      TOP_INV_OFF
        GETV    V_CUR                   ; the cursor, when it is on this die
        SMI     16
        STR     R2
        GETV    V_CTR
        XOR
        BNZ     gdi6
        GETV    V_ME
        BZ      gdi6
        GETV    V_STAND
        BNZ     gdi6
        LDI     SYM_LEFT
        BR      gdi7
gdi6:   LDI     ' '
gdi7:   FCALL   d_putc
        GETV    V_CTR
        ADI     1
        STR     RA
        XRI     5
        BNZ     gdi1
        FRET

; dheld -- D = 1 if die D (0-4) is drawn inverse: on our turn the dice we
; hold; on anyone else's the ones the server says were kept.
        FIT     48
dheld:  PLO     R9
        GETV    V_ME
        BZ      dhe1
        GLO     R9
        ADI     LO(V_HELD)
        PLO     RB
        LDI     HI(V_HELD)
        PHI     RB
        LDN     RB
        FRET
dhe1:   GLO     R9
        ADI     LO(FN_REPLY+HKEEP)
        PLO     RB
        LDI     HI(FN_REPLY+HKEEP)
        PHI     RB
        LDN     RB
        XRI     '0'                     ; '0' kept
        BNZ     dhe2
        LDI     1
        FRET
dhe2:   LDI     0
        FRET

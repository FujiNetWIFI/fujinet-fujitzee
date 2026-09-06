; disp.inc -- 2637 UVI display core for Fujitzee.
;
; Adapted from battleship/arcadia's, which adapted 5 Card Stud's, which
; adapted the bring-up's fujidisp.inc. Two changes, and the first one is
; the whole character of this port:
;
;   - The game runs the 2637's NORMAL colour mode, not MULTICOLOR. The
;     other Arcadia clients take multicolor's two inks over two
;     backgrounds; Fujitzee needs more hues than backgrounds -- filled
;     scores, potentials, the round number and a closed row all want to
;     differ -- and normal mode gives FOUR foreground colours over one
;     global background. From arcadia_v.cpp's draw_char():
;
;         fg = (($19F9>>3)&1) | ((cell>>5)&6)      bg = $19F9 & 7
;
;     so the cell's top two bits supply fg bits 2 and 1 while $19F9 bit 3
;     supplies bit 0, shared by all four. With bit 3 CLEAR and the
;     palette's inverted encoding, the four inks are white / cyan /
;     magenta / blue:
;
;       attr  bits  ink      used for
;       $00   00    WHITE    labels, filled scores, ordinary text
;       $40   01    CYAN     potential scores, the cursor'd ROLL tile
;       $80   10    MAGENTA  round, YOUR TURN, clock, active seat, held die
;       $C0   11    BLUE     closed rows, a dead ROLL tile -- the page's own
;                            colour, so they are INVISIBLE rather than dim.
;                            That is the intv port's COL_DASH trick exactly,
;                            and it is why nothing that must be READ may be
;                            drawn in ADIM.
;
;     Two consequences. There is no second background, so no inverse cell
;     -- the scorecard cursor is a SPRITE (sprites carry their own colour,
;     which is a fifth one for free). And $18FD bit 7, the multicolor
;     enable, must stay CLEAR: sound.inc's pitch writes preserve it clear
;     where the other ports' preserve it set.
;
;   - The $C0 hazard is handled more broadly here. A cell of exactly $C0
;     -- a space with attrs 11 -- flips the rest of the row into block
;     graphics, and $40 flips it back. Black-on-blue IS an attribute this
;     port paints with, so $C0 spaces would otherwise be common. But a
;     blank cell shows only the global background, so every attribute
;     renders one identically: DPUTC forces the attribute to $00 whenever
;     the glyph is 0, which defuses both codes at once and costs nothing.
;
; $19F9 bit 6 is ALSO the pot mux, so input.inc toggles this register
; between $86 (vertical) and $C6 (horizontal) -- see BGC26V/BGC26H.
;
; Output goes through the *SCRP pointer so one code path serves both
; screen halves (rows 0-12 at $1800, rows 13-25 at $1A00). No field
; crosses the boundary: the scorecard and the dice sit wholly in the
; upper half, the markers and everything below in the lower.
;
; DXLAT NEVER emits UDC codes: $38-$3B are the cursor and the dice pips,
; so a stray '-' or ':' in a server string must not render as a die face.
; '/' maps to the built-in full-cell diagonal $02; the rest fold to '.'.
;
; Conventions: R0 data, R1/R2 arguments, R3 counters; PSL = $02 (COM=1,
; logical compares, WC off -- NUMFLD flips WC internally and restores it).

; ---- hardware ---------------------------------------------------------
SCREEN  EQU     $1800           ; upper screen, 16 cols x 13 rows
SCREENL EQU     $1A00           ; lower screen, rows 13-25 (26-line mode)
UDC     EQU     $1980           ; 8 user-defined characters, 8 bytes each
GFXMOD  EQU     $19F8           ; b7 gfx mode, b6 26-line; b0-5 are the
                                ;   alt colours, unused in normal mode
BGCOL   EQU     $19F9           ; b7 char size (1 = 8x8), b6 pot mux,
                                ;   b3 the shared ink LSB, b0-2 background
SPRC23  EQU     $19FA           ; b6/b7 size obj3/obj2, colours obj3/obj2
SPRC01  EQU     $19FB           ; b6/b7 size obj1/obj0, colours obj1/obj0
SPRPOS  EQU     $18F0           ; 4 x (y,x); y raw = 256-yint, x raw = xint+43
PITCH   EQU     $18FD           ; b7 MULTICOLOR (kept CLEAR), b0-6 frequency
VOLUME  EQU     $18FE           ; b0-2 loudness, b3 snd enable, b5-7 h-shift

CWHITE  EQU     0               ; the palette (arcadia.cpp arcadia_colors)
CYELLOW EQU     1
CCYAN   EQU     2
CGREEN  EQU     3
CMGNTA  EQU     4
CRED    EQU     5
CBLUE   EQU     6
CBLACK  EQU     7

INKLSB  EQU     $00             ; $19F9 b3: the low bit of all four inks.
                                ;   Clear -> white/cyan/magenta/blue;
                                ;   set -> yellow/green/red/black.
GFXM26  EQU     $40             ; 26-line refresh, graphics mode off
BGC26V  EQU     $80|INKLSB|CBLUE        ; $86: 8x8 cells, mux CLEAR (vert)
BGC26H  EQU     BGC26V|$40              ; $C6: mux SET (horizontal pot)
GFXM13  EQU     $00             ; 13-line doublescan
BGC13V  EQU     INKLSB|CBLUE            ; $06: 8x16 cells, mux clear

ANORM   EQU     $00             ; attribute nibbles (cell bits 7-6)
APOT    EQU     $40
AHOT    EQU     $80
ADIM    EQU     $C0             ; the page's own blue: INVISIBLE, not dim

; ---- glyphs -----------------------------------------------------------
; Built-in graphics shapes: a complete box-drawing kit, free of charge.
GDIAG   EQU     $01             ; '/' full-cell diagonal. arcadia_v.cpp
                                ;   labels $01 '\' and $02 '/', but its own
                                ;   bitmaps say otherwise: $01 runs from the
                                ;   top-right pixel down to the bottom-left.
GBSLSH  EQU     $02             ; '\'
GBLOCK  EQU     $03             ; solid block -- the die body
GFTOP   EQU     $04             ; top edge
GFRGT   EQU     $05             ; right edge
GFBOT   EQU     $06             ; bottom edge -- also serves as the dash
GFLFT   EQU     $07             ; left edge
GFTR    EQU     $08             ; top-right corner
GFTL    EQU     $09             ; top-left corner
GFBL    EQU     $0A             ; bottom-left corner
GFBR    EQU     $0B             ; bottom-right corner
GDASH   EQU     GFBOT           ; the closed-row marker, drawn in ADIM

; The eight UDCs. $38-$3B double as the sprite images; sprite 0 is the
; scorecard cursor, so $38 must hold the chevron. Sprites 1-3 are parked
; for the program's whole life, so $39-$3B are free to be ordinary
; glyphs -- which is where the dice live.
GCHEVR  EQU     $38             ; cursor chevron = sprite 0's image
GPIP    EQU     $39             ; die cell: one pip punched out
GPIPCL  EQU     $3A             ; die cell: left half of the centre pip
GPIPCR  EQU     $3B             ; die cell: right half of the centre pip

; ---- macros -----------------------------------------------------------
DWBE    MACRO   val             ; big-endian word: what indirection reads
        DB      ((val)>>8)&$FF, (val)&$FF
        ENDM

SETSTR  MACRO   adr             ; point DPRINT at a string (clobbers R0)
        LODI,R0 (adr)>>8
        STRA,R0 DSTRP
        LODI,R0 (adr)&$FF
        STRA,R0 DSTRP+1
        ENDM

DAT     MACRO   row,col         ; fixed screen position (clobbers R0,R2)
    IF (row)<13
        LODI,R0 SCREEN>>8
        STRA,R0 SCRP
        LODI,R2 ((row)*16+(col)-1)&$FF
    ELSEIF
        LODI,R0 SCREENL>>8
        STRA,R0 SCRP
        LODI,R2 (((row)-13)*16+(col)-1)&$FF
    ENDIF
        ENDM

DCOL    MACRO   attr            ; set the attribute for what follows
        LODI,R0 attr
        STRA,R0 DCOLOR
        ENDM

; ---- mode setup -------------------------------------------------------
; DINI13 / DINI26: 13- or 26-line NORMAL-mode text, sound off. Leaf.
; Clobbers R0.
DINI13: LODI,R0 GFXM13
        STRA,R0 GFXMOD
        LODI,R0 BGC13V          ; 8x16 doublescan: 13 big rows fill the frame
        BCTR,UN DINIC
DINI26: LODI,R0 GFXM26          ; 26-line refresh
        STRA,R0 GFXMOD
        LODI,R0 BGC26V          ; 8x8 cells, pot mux clear
DINIC:  STRA,R0 BGCOL
        STRA,R0 BGCBAS          ; input.inc re-arms the pot mux from this
        EORZ    R0
        STRA,R0 PITCH           ; multicolor OFF, frequency 0
        STRA,R0 VOLUME
        STRA,R0 SCRP+1          ; both halves are page-aligned
        STRA,R0 DCOLOR
        RETC,UN

; DCLSU / DCLSL: clear one half to the page (208 cells of $00). Leaf.
; Clobbers R0,R3.
DCLSU:  LODI,R3 207
DCU1:   EORZ    R0
        STRA,R0 SCREEN,R3
        BDRR,R3 DCU1
        EORZ    R0
        STRA,R0 SCREEN
        RETC,UN

DCLSL:  LODI,R3 207
DCL1:   EORZ    R0
        STRA,R0 SCREENL,R3
        BDRR,R3 DCL1
        EORZ    R0
        STRA,R0 SCREENL
        RETC,UN

; ---- positioning ------------------------------------------------------
; DPOS: runtime position. R1 = row 0-25, R2 = col. Sets SCRP and leaves
; R2 = cell-minus-one for the auto-increment stores. Leaf. Clobbers
; R0,R1,R2.
DPOS:   LODI,R0 SCREEN>>8
        COMI,R1 13
        BCTR,LT DPOS1
        SUBI,R1 13
        LODI,R0 SCREENL>>8
DPOS1:  STRA,R0 SCRP
        LODZ    R1
        RRL,R0
        RRL,R0
        RRL,R0
        RRL,R0                  ; row*16 (row < 13: no wrap)
        ADDZ    R2
        SUBI,R0 1
        STRZ    R2
        RETC,UN

; ---- writing ----------------------------------------------------------
; DPUTC: write glyph R0 at the next cell with the DCOLOR attribute. The
; block-graphics guard lives here and only here: a blank cell shows only
; the global background, so storing it unattributed looks identical and
; makes both $C0 (into block graphics) and $40 (out of it) unreachable.
; Leaf. Clobbers R0,R2(+1).
DPUTC:  COMI,R0 0
        BCFR,EQ DPUT1
        STRA,R0 *SCRP,R2,+      ; a blank: attribute $00, never a switch
        RETC,UN
DPUT1:  IORA,R0 DCOLOR
        STRA,R0 *SCRP,R2,+
        RETC,UN

; DFILL: write R3 cells of glyph R1 with the DCOLOR attribute.
; Calls one down (DPUTC). Clobbers R0,R2,R3.
DFILL:  LODZ    R1
        BSTR,UN DPUTC
        BDRR,R3 DFILL
        RETC,UN

; DXLAT: ASCII in R0 -> glyph $00-$37, never a UDC. Lowercase folds.
; Leaf. Clobbers R0,R1.
DXLAT:  ANDI,R0 $7F
        SUBI,R0 $20
        BCTR,LT DXLBAD          ; control character
        STRZ    R1
        LODA,R0 *PXLAT,R1       ; the table is in block 2 (ROM budget);
        RETC,UN                 ;   indirect reaches it, indexed does not
DXLBAD: LODI,R0 $34             ; '.'
        RETC,UN

; DPRINT: print the 0-terminated ASCII string at (DSTRP) from the next
; cell, translated and attributed. Calls one down (DXLAT/DPUTC).
; Clobbers R0,R1,R2,R3.
DPRINT: LODI,R3 $FF
DPRT1:  LODA,R0 *DSTRP,R3,+    ; str[++R3]; a load sets CC
        BCTR,EQ DPRT9
        BSTR,UN DXLAT
        BSTR,UN DPUTC
        BCTR,UN DPRT1
DPRT9:  RETC,UN

; DPRTN: like DPRINT but exactly R1 cells: the string is truncated or
; space-padded to the field, so fields overdraw opaquely.
; Calls one down (DXLAT/DPUTC). Clobbers R0,R1,R2,R3, V_TMP2.
DPRTN:  LODZ    R1
        STRA,R0 V_TMP2          ; cells remaining
        LODI,R3 $FF
DPTN1:  LODA,R0 V_TMP2
        BCTR,EQ DPTN9
        SUBI,R0 1
        STRA,R0 V_TMP2
        LODA,R0 *DSTRP,R3,+
        BCTR,EQ DPTNP           ; NUL: pad the rest with spaces
        BSTR,UN DXLAT
        BSTA,UN DPUTC
        BCTR,UN DPTN1
DPTNP:  EORZ    R0
        BSTA,UN DPUTC
        LODA,R0 V_TMP2
        BCTR,EQ DPTN9
        SUBI,R0 1
        STRA,R0 V_TMP2
        BCTR,UN DPTNP
DPTN9:  RETC,UN

; NUMFLD: print the u16 at V_NUM right-aligned in an R1-cell field
; (R1 = 1..5) from the next cell, leading zeros blanked, DCOLOR applied.
; The field width picks the starting power of ten, so a scorecard cell
; (3) and a standings total (4) share one routine; a value too wide for
; its field keeps its low digits. Flips WC on for the 16-bit repeated
; subtraction and restores WC=0 before returning.
; Calls one down (DPUTC). Clobbers R0,R1,R3, V_NUM, V_TMP.
NUMFLD: LODI,R0 5
        SUBZ    R1
        RRL,R0                  ; (5-width)*2 = the PW10 byte index
        STRZ    R1
        EORZ    R0
        STRA,R0 V_TMP           ; nonzero-seen flag
        COMI,R1 8               ; width 1 starts past the table: the ones
        BCTA,EQ NUMONE          ;   digit is the whole field
NUMP1:  LODI,R3 0               ; digit
NUMP2:  PPSL    $09             ; WC on, C=1 (no borrow in)
        LODA,R0 V_NUM+1
        SUBA,R0 PW10+1,R1
        STRA,R0 V_NUM+1
        LODA,R0 V_NUM
        SUBA,R0 PW10,R1
        STRA,R0 V_NUM
        CPSL    $08             ; WC off (C preserved)
        TPSL    $01
        BCFR,EQ NUMP3           ; C=0: borrowed, undo
        ADDI,R3 1
        BCTR,UN NUMP2
NUMP3:  PPSL    $08             ; WC on, C=0: plain 16-bit add-back
        LODA,R0 V_NUM+1
        ADDA,R0 PW10+1,R1
        STRA,R0 V_NUM+1
        LODA,R0 V_NUM
        ADDA,R0 PW10,R1
        STRA,R0 V_NUM
        CPSL    $09             ; WC off, C clear
        LODZ    R3
        BCFR,EQ NUMPD           ; a digit: print it
        LODA,R0 V_TMP
        BCFR,EQ NUMPZD          ; zero after a digit: print '0'
        EORZ    R0              ; leading zero: blank cell
        BSTA,UN DPUTC
        BCTR,UN NUMPN
NUMPD:  LODI,R0 1
        STRA,R0 V_TMP
NUMPZD: LODZ    R3
        IORI,R0 $10
        BSTA,UN DPUTC
NUMPN:  ADDI,R1 2
        COMI,R1 8
        BCFA,EQ NUMP1
NUMONE: LODA,R0 V_NUM+1         ; ones digit: always printed
        IORI,R0 $10
        BSTA,UN DPUTC
        RETC,UN

PW10:   DWBE    10000
        DWBE    1000
        DWBE    100
        DWBE    10

; RXPRNT: print exactly R1 cells from the state cursor (RXNEXT bytes),
; truncating at the field or padding past a NUL -- the zero-copy string
; renderer for names and the server prompt. The cursor is left wherever
; the NUL (or the field edge) put it; callers position it per field.
; Calls two down (RXNEXT -> MSLICE). Clobbers R0,R1,R3, V_TMP2.
RXPRNT: LODZ    R1
        STRA,R0 V_TMP2
RXP1:   LODA,R0 V_TMP2
        BCTR,EQ RXP9
        SUBI,R0 1
        STRA,R0 V_TMP2
        BSTA,UN RXNEXT
        BCTR,EQ RXPPAD          ; NUL: pad the rest
        BSTA,UN DXLAT
        BSTA,UN DPUTC
        BCTR,UN RXP1
RXPPAD: EORZ    R0
        BSTA,UN DPUTC
        LODA,R0 V_TMP2
        BCTR,EQ RXP9
        SUBI,R0 1
        STRA,R0 V_TMP2
        BCTR,UN RXPPAD
RXP9:   RETC,UN

PXLAT:  DWBE    XLAT            ; the translate table lives in block 2

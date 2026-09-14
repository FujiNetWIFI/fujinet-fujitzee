; disp.inc -- the 32x24 renderer, on TMS9918 Graphics II through OS7.
;
; Mode 2 is the whole reason this port looks like the other ones. Graphics 1
; colours eight consecutive pattern codes together, which is why the CONFIG
; client in the ColecoVision bring-up has to keep a second copy of its whole
; charset just to get one selection bar. Mode 2 gives a foreground and a
; background per 8-pixel row of each pattern -- so a light-green potential
; score, a yellow held die and white body text can share a screen row.
;
; The cost is that colour belongs to the PATTERN, not the cell. Two cells
; showing the same code in the same screen third are always the same colour.
; The answer here is banks: the font is uploaded three times, at three code
; ranges, with three inks, and drawing coloured text is picking a base.
;
;   0x00-0x59   art tiles: dice in four colour banks, the roll tile, the
;               fujiTZEE logo, board chrome, icons        (see assets/)
;   0x70-0xAF   font, WHITE on dark blue      code = ascii + DBWHITE
;   0xB0-0xEF   font, CYAN on dark blue       code = ascii + DBCYAN
;   0xF0-0xFF   digits, LIGHT GREEN           code = ascii + DBGREEN
;
; All three screen thirds get identical uploads, so a code means the same
; thing everywhere and nothing has to know which third it is drawing into.
;
; VRAM WRITE BUDGET. Drawing a cell is ONE name-table byte -- colour rides the
; code, so nothing here ever touches the colour table after DINIT. That is
; what keeps the per-frame cost down: a score cell is 3 bytes, the timer 2, a
; die 9, the prompt one row of 32. The rules:
;
;   - anything that repaints wholesale calls DHIDE first and DSHOW after, so
;     the big writes happen with the display off;
;   - no single burst outside that exceeds one row, 32 bytes;
;   - the NMI handler never writes VRAM at all (see fujitzee.asm).

WIDTH   EQU     32
HEIGHT  EQU     24

; Bank bases. Adding one of these to an ascii code gives the pattern code for
; that ink; the font banks cover ascii 20H-5FH, the green bank 30H-3FH only
; (it exists for potential scores, which are numbers).
DBWHITE EQU     50H
DBCYAN  EQU     90H
DBGREEN EQU     0C0H

DSPACE  EQU     20H+DBWHITE     ; a blank cell

; Colour-table bytes for the three font banks: fg<<4 | bg, every row.
CFWHITE EQU     (CWHITE*16)+CDBLUE
CFCYAN  EQU     (CCYAN*16)+CDBLUE
CFGREEN EQU     (CLGREEN*16)+CDBLUE

; Pattern-table offsets of each bank within a screen third.
POTILE  EQU     0
POWHITE EQU     DBWHITE*8+(20H*8)       ; code 70H
POCYAN  EQU     DBCYAN*8+(20H*8)        ; code 0B0H
POGREEN EQU     DBGREEN*8+(30H*8)       ; code 0F0H
FONTLEN EQU     FONTCNT*8
GRNLEN  EQU     16*8
GRNSRC  EQU     FONTPAT+((30H-20H)*8)   ; the digits, inside the font block


; DINIT: Graphics II, the charset, a cleared screen, interrupt enabled.
;
; MODE_1 is called for its table layout, which is already the standard Mode 2
; map -- patterns 0000H, names 1800H, colours 2000H. What it does NOT do is
; unmask them: it leaves R3 = 80H and R4 = 00H, and in Mode 2 those are masks,
; not just bases. Left alone, all three screen thirds alias onto the first 2K
; of patterns and the colour table collapses to 64 bytes -- the screen fills
; with garbage that looks like a mode problem and is really a mask problem.
;
; R1 is written LAST, and nothing may call MODE_1 again afterwards: R1 bit 5
; is the vblank interrupt enable, and re-running MODE_1 clears it. The client
; stays alive and goes deaf, because POLLER rides that interrupt.
;
; Clobbers everything.
DINIT:
        CALL    VDPLOCK         ; MODE_1 writes registers of its own
        CALL    MODE_1
        CALL    VDPFREE
        LD      BC,01A0H        ; R1 = A0H: 16K, display OFF, vblank int on.
        CALL    VWREG         ; B IS THE REGISTER NUMBER -- writing 00C0H
                                ; here means R0, not R1, and R0 bit 1 is M3:
                                ; it put the VDP straight back into Graphics I
                                ; one instruction after selecting Graphics II,
                                ; where R4 = 03H points the pattern generator
                                ; at 1800H -- the name table. The screen came
                                ; up a flat blue and every name-table byte was
                                ; still perfectly correct.
        LD      BC,0002H        ; R0 = 2: M3 set, Graphics II
        CALL    VWREG
        LD      BC,03FFH        ; R3 = FFH: colours at 2000H, full 6K, unmasked
        CALL    VWREG
        LD      BC,0403H        ; R4 = 03H: patterns at 0000H, full 6K
        CALL    VWREG
        LD      BC,0705H        ; R7 = border, one step off the table felt
        CALL    VWREG

        LD      A,3             ; every screen third gets the same upload
        LD      (DTHIRD),A
        LD      HL,0
        LD      (DBASE),HL
DIN1:   LD      HL,TILEPAT      ; art tiles: pattern and colour both real
        LD      DE,POTILE
        LD      BC,TILECNT*8
        CALL    DIWR
        LD      HL,TILECOL
        LD      DE,VCOLOUR+POTILE
        LD      BC,TILECNT*8
        CALL    DIWR

        LD      HL,FONTPAT      ; the font, three times over
        LD      DE,POWHITE
        LD      BC,FONTLEN
        CALL    DIWR
        LD      HL,FONTPAT
        LD      DE,POCYAN
        LD      BC,FONTLEN
        CALL    DIWR
        LD      HL,GRNSRC
        LD      DE,POGREEN
        LD      BC,GRNLEN
        CALL    DIWR

        LD      DE,VCOLOUR+POWHITE      ; ...and its colours, which are flat:
        LD      BC,FONTLEN              ; one ink per bank, so a fill, not a
        LD      A,CFWHITE               ; table. 1536 bytes of ROM saved.
        CALL    DIFILL
        LD      DE,VCOLOUR+POCYAN
        LD      BC,FONTLEN
        LD      A,CFCYAN
        CALL    DIFILL
        LD      DE,VCOLOUR+POGREEN
        LD      BC,GRNLEN
        LD      A,CFGREEN
        CALL    DIFILL

        LD      HL,(DBASE)      ; next third, 2K along in both tables
        LD      BC,800H
        ADD     HL,BC
        LD      (DBASE),HL
        LD      HL,DTHIRD
        DEC     (HL)
        JR      NZ,DIN1

        CALL    DCLS
        LD      BC,01E0H        ; R1 = E0H: display on, 16K, vblank int on
        JP      VWREG

; DIWR: copy BC bytes from HL to VRAM at (DBASE + DE), in small chunks.
;
; OS7's WRITE_VRAM DOES NOT HONOUR A FULL 16-BIT COUNT. Handed 720 it copies
; 464 and returns as though it had copied all of it; handed 512 it copies 512.
; PUT_VRAM has the documented version of the same disease -- it truncates its
; count to eight bits -- so this is the family trait rather than a surprise.
;
; The symptom was a perfect blue screen: the tile block is the first thing
; uploaded, 720 bytes, so codes 3AH-59H -- the logo, the icons and every piece
; of board chrome -- silently never arrived, while the font that follows them
; did. Reading the name table back showed everything in its right place, which
; is exactly why the screen has to be checked as PIXELS and not as codes.
;
; Rather than work out which counts are safe, nothing goes over in more than
; DIWRMAX bytes at a time. That also keeps every burst inside the frame budget
; the rest of this file is written to.
; Clobbers everything.
DIWRMAX EQU     128

DIWR:   LD      (DIWSRC),HL
        LD      (DIWREM),BC
        LD      HL,(DBASE)
        ADD     HL,DE
        LD      (DIWDST),HL
DIWR1:  LD      BC,(DIWREM)
        LD      A,B
        OR      C
        RET     Z
        LD      A,B
        OR      A
        JR      NZ,DIWRF        ; 256 or more left: take a full chunk
        LD      A,C
        CP      DIWRMAX
        JR      C,DIWRP
DIWRF:  LD      BC,DIWRMAX
        JR      DIWRG
DIWRP:  LD      B,0             ; less than a chunk: take what is left
        LD      C,A
DIWRG:  LD      HL,(DIWSRC)
        LD      DE,(DIWDST)
        PUSH    BC
        CALL    VWRITE
        POP     BC
        LD      HL,(DIWSRC)     ; advance both ends and count down
        ADD     HL,BC
        LD      (DIWSRC),HL
        LD      HL,(DIWDST)
        ADD     HL,BC
        LD      (DIWDST),HL
        LD      HL,(DIWREM)
        OR      A
        SBC     HL,BC
        LD      (DIWREM),HL
        JR      DIWR1

; DIFILL: FILL_VRAM with the current third's base folded in. A = value,
; DE = offset, BC = count. OS7 wants HL = address, DE = count, A = value.
DIFILL: PUSH    AF
        LD      HL,(DBASE)
        ADD     HL,DE
        LD      D,B
        LD      E,C
        POP     AF
        JP      VFILL

; DHIDE / DSHOW: display off and on, around a wholesale repaint. The vblank
; interrupt enable (bit 5) stays set through both, so the frame counter and
; POLLER keep running while the screen is dark.
DHIDE:  LD      BC,01A0H        ; R1 bit 6 is the display enable, so blanking
        JP      VWREG         ; is A0H; C0H leaves it on and merely drops
                                ; the interrupt
DSHOW:  LD      BC,01E0H
        JP      VWREG

; DCLS: blank the whole name table. 768 bytes, so callers do it behind DHIDE
; or at a moment where a frame drop does not matter. Clobbers A,B,C,D,E,H,L.
DCLS:   LD      HL,VNAMETB
        LD      DE,WIDTH*HEIGHT
        LD      A,DSPACE
        JP      VFILL


; ---- guarding the VDP against the NMI ---------------------------------
;
; THE VDP ADDRESS SETUP IS NOT ATOMIC AGAINST THE VBLANK NMI, and on this
; machine that interrupt cannot be masked.
;
; An address is two bytes written to the control port, sequenced by a flip-flop
; inside the VDP. Reading the VDP status register -- which is how the interrupt
; gets cleared, and therefore the one thing the handler must do -- ALSO RESETS
; THAT FLIP-FLOP. So an NMI landing between the two address bytes leaves the
; second byte taken as a first, and the write that follows lands somewhere else
; entirely: one cell, anywhere on screen, holding a code meant for another.
; That is a break in a scorecard rule, a cyan block inside the ROLL tile, a die
; with its pips smeared -- rare, random, and worse the more there is to draw.
;
; The NMI cannot be stopped, so it is asked to defer instead. While VDPBUSY is
; set it does everything except the status read and records that it still owes
; one; VDPFREE makes that read as soon as the sequence finishes. Nothing is
; lost: the VDP holds its interrupt line asserted until the status is read and
; the NMI is edge-triggered, so no second interrupt can arrive in the meantime
; -- it is only delayed by the few microseconds of one cell write.
;
; Every VDP access in this client goes through the three wrappers below.

; Both of these must be TRANSPARENT to every register, A included. FILL_VRAM
; takes its fill value in A, so a lock that used A as scratch painted VRAM with
; the flag instead of the colour -- the whole screen in ink 1. Cheap to get
; wrong, loud when it happens.
VDPLOCK:
        PUSH    AF
        LD      A,1
        LD      (VDPBUSY),A
        POP     AF
        RET

VDPFREE:
        PUSH    AF
        XOR     A
        LD      (VDPBUSY),A
        LD      A,(VDPPEND)
        OR      A
        JR      Z,VDPF1
        XOR     A
        LD      (VDPPEND),A
        CALL    READREG         ; the read the NMI could not make
        LD      (VDPSTAT),A
VDPF1:  POP     AF
        RET

; VWRITE / VFILL / VWREG: WRITE_VRAM, FILL_VRAM and WRITE_REGISTER, bracketed.
; Same registers in; A is not meaningful on return.
VWRITE: CALL    VDPLOCK
        CALL    WRITVRM
        JP      VDPFREE
VFILL:  CALL    VDPLOCK
        CALL    FILLVRM
        JP      VDPFREE
VWREG:  CALL    VDPLOCK
        CALL    WRITREG
        JP      VDPFREE


; ---- cell addressing --------------------------------------------------

; DVADDR: HL = the name-table address of (D = row, E = col).
; Preserves DE. Clobbers A,B,C,H,L.
DVADDR: LD      L,D
        LD      H,0
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL           ; row * 32
        LD      A,E
        ADD     A,L
        LD      L,A
        JR      NC,DVA1
        INC     H
DVA1:   LD      BC,VNAMETB
        ADD     HL,BC
        RET

; DPUTC: write pattern code A at (D = row, E = col).
; Preserves BC and DE. Clobbers A,H,L.
;
; BC IS SAVED FOR A REASON. Five callers here run DJNZ loops around this --
; DPRINT, DPRTN, DFILL, DDIE, the logo blit -- and OS7's WRITE_VRAM returns
; with BC at zero because it counts down with it. Without the push, every one
; of those loops has its counter reset to zero on each pass, so DJNZ decrements
; to 255 and never terminates. The logo blit did exactly that on the first run
; of this port: it wrote the same 256-cell window forever, and the screen filled
; with the ROM's own 0FFH padding as HL walked off the end of the tile list.
DPUTC:  LD      (DCHR),A
        LD      A,D             ; a row off the bottom of the screen is refused
        CP      HEIGHT          ; rather than written. DVADDR is arithmetic --
        RET     NC              ; row 79 is a perfectly valid address, it just
                                ; happens to be inside the COLOUR table -- so
                                ; one bad row index would repaint the charset's
                                ; colours instead of drawing a cell.
        LD      A,(DCHR)
        PUSH    BC
        CALL    DVADDR
        EX      DE,HL           ; DE = VRAM dest...
        PUSH    HL              ; ...HL was row/col; keep it for the caller
        LD      HL,DCHR
        LD      BC,1
        CALL    VWRITE
        POP     DE
        POP     BC
        RET

; DTILE: like DPUTC but takes an art tile code, which needs no bank added.
DTILE   EQU     DPUTC


; ---- text -------------------------------------------------------------

; DPRINT  : the NUL-terminated string at HL, white,       at (D,E).
; DPRINTA : the same, cyan -- the accent the shared engine calls drawTextAlt.
; DPRINTG : the same, light green -- potential scores only, so digits only.
;
; Clipped to the row: a string that would run off column 31 stops there rather
; than wrapping into the next line's cells.
; Clobbers A,H,L and advances E past the text.
DPRINT: LD      A,DBWHITE
        JR      DPSET
DPRINTA:
        LD      A,DBCYAN
        JR      DPSET
DPRINTG:
        LD      A,DBGREEN
DPSET:  LD      (DBANK),A
DPLOOP: LD      A,E
        CP      WIDTH
        RET     NC              ; clipped at the right edge
        LD      A,(HL)
        OR      A
        RET     Z
        PUSH    HL
        CALL    DFOLD
        LD      HL,DBANK
        ADD     A,(HL)
        POP     HL
        PUSH    HL
        CALL    DPUTC
        POP     HL
        INC     HL
        INC     E
        JR      DPLOOP

; DPRTN: B characters from HL, in the current bank, at (D,E). Stops early on a
; NUL but pads the rest of the field with blanks, so a short name cannot leave
; a longer one's tail on screen.
;
; This is what draws straight out of the reply window: server strings are
; fixed-width and NUL-padded, and nothing is copied to RAM first.
; Clobbers A,B,H,L; advances E.
DPRTN:  LD      A,B
        OR      A
        RET     Z
DPRTN1: LD      A,E
        CP      WIDTH
        RET     NC
        LD      A,(HL)
        OR      A
        JR      Z,DPRTPAD
        PUSH    HL
        CALL    DFOLD
        LD      HL,DBANK
        ADD     A,(HL)
        POP     HL
        PUSH    HL
        CALL    DPUTC
        POP     HL
        INC     HL
        INC     E
        DJNZ    DPRTN1
        RET
DPRTPAD:
        LD      A,E             ; ran out of string: blank the rest
        CP      WIDTH
        RET     NC
        LD      A,DSPACE
        CALL    DPUTC
        INC     E
        DJNZ    DPRTPAD
        RET

; DFOLD: ascii in A, folded into the font's range 20H-5FH.
;
; THE NAMCO FONT HAS NO LOWERCASE -- its letterforms ARE the capitals, which is
; why the MS-DOS port keeps them in its lowercase slots and uses ascii case as
; a colour escape instead. Every string the server sends is lowercased, so this
; sits on the path of every wire string that reaches the screen: without it a
; table list renders as punctuation ("ai room" comes out as "!) 2//-").
;
; Anything still outside the range after folding becomes a space rather than
; indexing off the end of the bank into an art tile.
; Clobbers nothing but A and the flags.
DFOLD:  CP      'a'
        JR      C,DFO1
        CP      'z'+1
        JR      NC,DFO1
        SUB     20H
DFO1:   CP      20H
        JR      C,DFOSP
        CP      60H
        RET     C
DFOSP:  LD      A,' '
        RET

; DBANKW / DBANKA / DBANKG: pick the bank DPRTN and NUMFLD will use.
; A only -- NUMFLD's callers keep the field width in C across these.
DBANKW: LD      A,DBWHITE
        LD      (DBANK),A
        RET
DBANKA: LD      A,DBCYAN
        LD      (DBANK),A
        RET
DBANKG: LD      A,DBGREEN
        LD      (DBANK),A
        RET

; DFILL: B blank cells at (D,E). Clobbers A,B,H,L; advances E.
DFILL:  LD      A,B
        OR      A
        RET     Z
DFIL1:  LD      A,E
        CP      WIDTH
        RET     NC
        LD      A,DSPACE
        CALL    DPUTC
        INC     E
        DJNZ    DFIL1
        RET


; ---- numbers ----------------------------------------------------------

; U16STR: HL -> decimal in LINBUF, NUL-terminated, no leading zeros.
; Returns B = length, HL = LINBUF. Clobbers A,C,D,E.
;
; The output cursor lives in RAM, not in DE, because DE carries the power of
; ten each digit is divided out by. Holding both there is the obvious way to
; write this and it is silently wrong: the digits get stored at address 10000
; and LINBUF is never touched at all, so every number on screen renders as
; blanks -- which is how the round counter came out as "R  /13".
;
; url.inc's BYTDEC rides on this, so it must touch nothing but RAM and
; registers: it runs in the middle of a mailbox transaction, where a stray read
; of the hotspot pages would corrupt the TX stream.
U16STR: PUSH    HL
        LD      HL,LINBUF
        LD      (U16P),HL
        POP     HL
        LD      B,0             ; digits emitted so far
        LD      C,0             ; a nonzero digit has been seen
        LD      DE,10000
        CALL    U16DIG
        LD      DE,1000
        CALL    U16DIG
        LD      DE,100
        CALL    U16DIG
        LD      DE,10
        CALL    U16DIG
        LD      A,L             ; the units digit always prints
        ADD     A,'0'
        CALL    U16PUT
        XOR     A
        PUSH    HL
        LD      HL,(U16P)
        LD      (HL),A          ; terminate
        POP     HL
        LD      HL,LINBUF
        RET

; U16DIG: one decimal place. DE = the power of ten, HL = the remainder.
U16DIG: LD      A,'0'-1
U16D1:  INC     A
        OR      A               ; clear the carry SBC would fold in
        SBC     HL,DE
        JR      NC,U16D1
        ADD     HL,DE           ; undo the borrow
        CP      '0'
        JR      NZ,U16D2        ; nonzero: always print
        LD      A,C
        OR      A
        RET     Z               ; a leading zero: skip it
        LD      A,'0'
U16D2:  LD      C,1
        JP      U16PUT

; U16PUT: append A at the LINBUF cursor. Preserves DE and HL.
U16PUT: PUSH    HL
        LD      HL,(U16P)
        LD      (HL),A
        INC     HL
        LD      (U16P),HL
        POP     HL
        INC     B
        RET

; NUMFLD: HL as a decimal right-aligned in a C-wide field ending at column E,
; row D, in the current bank. The field is blank-filled first, so a two-digit
; score replacing a three-digit one does not leave a stray column.
; Clobbers everything but D.
NUMFLD: LD      A,C
        LD      (DNFW),A
        PUSH    DE
        CALL    U16STR          ; B = length, HL = LINBUF
        LD      A,B
        LD      (DNFL),A
        POP     DE
        LD      A,(DNFL)
        LD      B,A
        LD      A,(DNFW)
        SUB     B               ; A = width - length; carry if it overflows
        JR      NC,NUMF1
        XOR     A               ; longer than the field: print it all
NUMF1:  OR      A
        JR      Z,NUMF2
        LD      B,A
        PUSH    HL
        CALL    DFILL           ; leading blanks, advancing E
        POP     HL
NUMF2:  LD      A,(DNFL)
        LD      B,A
        JP      DPRTN


; ---- dice -------------------------------------------------------------

; DDIE: draw a 3x3 die at (D = row, E = col).
; A = face index into DICETAB's banks: 0-5 are the six faces, and the bank is
; already folded in by the caller (dice.inc), because which bank a die belongs
; to -- normal, held, fujitzee -- is game state, not geometry.
;
; Nine name-table bytes, three rows of three, which is why this never needs
; the display blanked: a whole hand of five dice is 45 bytes.
; Clobbers everything.
DDIE:   CP      DICECNT         ; a bad entry would index off the end of
        JR      C,DDIE0         ; DICETAB and paint the ROM's own padding
        LD      A,DICEBLK       ; across the panel -- which is exactly what a
DDIE0:  LD      L,A             ; clobbered bank did once. Draw nothing instead.
                                ; entry * 9 = entry*8 + entry
        LD      H,0
        LD      B,H
        LD      C,L
        ADD     HL,HL           ; *2
        ADD     HL,HL           ; *4
        ADD     HL,HL           ; *8
        ADD     HL,BC           ; *9
        LD      BC,DICETAB
        ADD     HL,BC
        LD      (DDPTR),HL
        LD      B,3             ; three tile rows
DDIE1:  PUSH    DE
        LD      C,3
DDIE2:  PUSH    BC
        LD      HL,(DDPTR)
        LD      A,(HL)
        INC     HL
        LD      (DDPTR),HL
        CALL    DTILE
        INC     E
        POP     BC
        DEC     C
        JR      NZ,DDIE2
        POP     DE
        INC     D               ; next tile row
        DJNZ    DDIE1
        RET

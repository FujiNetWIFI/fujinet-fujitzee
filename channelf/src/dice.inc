; ---------------------------------------------------------------------------
; dice.inc -- five dice, three rows tall.
;
; A die is a solid body in its state colour with the pips PUNCHED THROUGH to
; the ltgray ground -- the Arcadia port's inversion, which is also simply what
; a die looks like. Sixteen pixels wide on a nineteen-pixel pitch is 95 across
; for five of them, which is the safe width exactly.
;
; THERE IS NO ART FILE. The Arcadia port pre-composited its faces into UDCs and
; battleship pre-composites its reticle into a second bank of cells, both
; because their blitters carry the colour IN the image. DFILL takes a colour
; argument, so here a face is nine bits -- one per pip position in a 3x3 grid
; -- and the six of them are twelve bytes of table. The three state colours
; then cost nothing at all, where a pre-composited set would have been six
; faces times three banks of 72-byte cells.
;
;   pip position i = row*3 + col, bit i of the face mask
;   columns at x+1, x+6, x+11   rows at y+1, y+7, y+13   each pip 4x4
; ---------------------------------------------------------------------------

DICEY	EQU DORGY+5*DCELLH	; the band: rows 5-7, pixel rows 34-51
DIEW	EQU 16
DIEH	EQU 18
DIEPTCH	EQU 19			; 16 of die and 3 of gap
PIPSZ	EQU 4
NDICE	EQU 5

DCOLNRM	EQU CVAL1		; an ordinary die: blue body, ltgray pips
DCOLHLD	EQU CVAL2		; held: red
DCOLCUR	EQU CVAL3		; under the cursor: green

; ---------------------------------------------------------------------------
; DIEFACE -- A = the face of die VDIE as 0-5, or $FF if there is none.
;   The wire carries five ascii '1'-'6', and NULs at the top of a turn before
;   anything has been rolled -- which is a blank space, not a one.
;   Leaf. Clobbers A, r0, DC0.
; ---------------------------------------------------------------------------
DIEFACE: DCI VDIE
	LM
	LR 0,A
	DCI FNREPLY+GODICE
	LR A,0
	ADC
	LM
	CI '0'
	BP DIEFX		; A <= '0', the NUL included: no die here yet
	CI '6'
	BM DIEFX		; A > '6'
	AI 0CFH			; -'1'
	POP
DIEFX:	LI 0FFH
	POP

; ---------------------------------------------------------------------------
; DIEHLD -- A = 1 if die VDIE will be KEPT through the next roll.
;
;   On our own turn this reads the LOCALLY staged mask, not the wire. The
;   wire's keepRoll is an echo of what was last actually re-rolled, so it does
;   not move until the next roll goes out -- reading it to decide what to draw
;   made a die you had just held look exactly like one you had not. For every
;   other seat the echo is all there is, and it is the right thing: it is what
;   they kept.
;
;   Either way '1' means RE-ROLL, so a kept die is the '0'.
;   Leaf. Clobbers A, r0, DC0.
; ---------------------------------------------------------------------------
DIEHLD:	DCI VDIE
	LM
	LR 0,A
	DCI VMYTRN
	LM
	OI 0
	BZ DIEH1
	DCI VKEEP
	BR DIEH2
DIEH1:	DCI FNREPLY+GOKEEP
DIEH2:	LR A,0
	ADC
	LM
	CI '0'
	BZ DIEHY
	CLR
	POP
DIEHY:	LIS 1
	POP

; ---------------------------------------------------------------------------
; DIECOL -- VDCOL = the state colour of die VDIE.
;   The cursor overrides the hold, so the markers have to carry the held state
;   independently -- which they do, because a held die keeps its red body the
;   moment the cursor leaves it.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
DIECOL:	ENTER
	LI DCOLNRM
	DCI VDCOL
	ST
	PI DIEHLD
	OI 0
	BZ DIEC1
	LI DCOLHLD
	DCI VDCOL
	ST
DIEC1:	DCI VMYTRN
	LM
	OI 0
	BZ DIECX
	DCI VUIMOD
	LM
	OI 0
	BNZ DIECX		; the cursor is up among the score rows
	DCI VDCUR
	LM
	OI 0
	BZ DIECX		; it is on the ROLL tile
	AI 0FFH			; cursor 1-5 selects die 0-4
	LR 0,A
	DCI VDIE
	LM
	XS 0
	BNZ DIECX
	LI DCOLCUR
	DCI VDCOL
	ST
DIECX:	LEAVE

; ---------------------------------------------------------------------------
; DIEX -- VDX = the left pixel column of die VDIE.
;   Leaf. Clobbers A, r0, r1, DC0.
; ---------------------------------------------------------------------------
DIEX:	DCI VDIE
	LM
	LR 0,A
	LI DORGX
	LR 1,A
	LR A,0
	OI 0
	BZ DIEXX
DIEX1:	LR A,1
	AI DIEPTCH
	LR 1,A
	DS 0
	BNZ DIEX1
DIEXX:	DCI VDX
	LR A,1
	ST
	POP

; ---------------------------------------------------------------------------
; DIEBLK -- blank die VDIE's whole footprint.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
DIEBLK:	ENTER
	PI DIEX
	LI DIEW
	LR 0,A
	DCI VDX
	LM
	LR 1,A
	LI DICEY
	LR 2,A
	LI CVAL0
	LR 3,A
	LI DIEH
	LR 9,A
	PI DFILL
	LEAVE

; ---------------------------------------------------------------------------
; DIEDRW -- draw die VDIE: body, then pips punched out of it.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
DIEDRW:	ENTER
	PI DIEFACE
	DCI VDFACE
	ST
	CI 0FFH
	BNZ DIED1
	PI DIEBLK		; no dice this turn yet
	LEAVE

DIED1:	PI DIEX
	PI DIECOL

	LI DIEW			; the body
	LR 0,A
	DCI VDX
	LM
	LR 1,A
	LI DICEY
	LR 2,A
	DCI VDCOL
	LM
	LR 3,A
	LI DIEH
	LR 9,A
	PI DFILL

	; the face mask, two bytes per face
	DCI VDFACE
	LM
	SL 1
	LR 0,A
	DCI DFACES
	LR A,0
	ADC
	LM
	DCI VDM0
	ST
	DCI VDFACE
	LM
	SL 1
	LR 0,A
	DCI DFACES+1
	LR A,0
	ADC
	LM
	DCI VDM1
	ST

	DCI VDPOS
	CLR
	ST
DIED2:	DCI VDPOS
	LM
	CI 9
	BZ DIEDX
	PI DPIPQ		; is there a pip at this position?
	OI 0
	BZ DIED3
	PI DPIP
DIED3:	DCI VDPOS
	LM
	INC
	DCI VDPOS
	ST
	BR DIED2
DIEDX:	LEAVE

; ---------------------------------------------------------------------------
; DPIPQ -- A = 1 if the face mask has a pip at position VDPOS.
;   Leaf. Clobbers A, r0, r1, DC0.
; ---------------------------------------------------------------------------
DPIPQ:	DCI VDPOS
	LM
	LR 0,A
	LI 1
	LR 1,A
	LR A,0
	CI 7
	BM DPQHI		; A > 7: position 8, the only high-byte bit
	LR A,0
	OI 0
	BZ DPQLS
DPQLL:	LR A,1
	SL 1
	LR 1,A
	DS 0
	BNZ DPQLL
DPQLS:	DCI VDM0
	BR DPQT
DPQHI:	LI 1			; position 8 is the only bit in the high byte
	LR 1,A
	DCI VDM1
DPQT:	LM
	NS 1
	OI 0
	BZ DPQNO
	LIS 1
	POP
DPQNO:	CLR
	POP

; ---------------------------------------------------------------------------
; DPIP -- punch the pip at position VDPOS out of die VDIE's body.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
DPIP:	ENTER
	DCI VDPOS
	LM
	LR 0,A
	CLR
	LR 1,A			; column index
DPIP1:	LR A,0
	CI 2
	BP DPIP2		; A <= 2: what is left IS the column index
	LR A,0
	AI 0FDH			; -3
	LR 0,A
	LR A,1
	INC
	LR 1,A			; ...and r1 counts the rows
	BR DPIP1
DPIP2:	; r0 = column 0-2, r1 = row 0-2
	DCI DPIPX
	LR A,0
	ADC
	LM
	LR 2,A			; the x offset
	DCI DPIPY
	LR A,1
	ADC
	LM
	LR 3,A			; the y offset

	DCI VDX
	LM
	AS 2
	DCI VPIPX
	ST
	LI DICEY
	AS 3
	DCI VPIPY
	ST

	LI PIPSZ
	LR 0,A
	DCI VPIPX
	LM
	LR 1,A
	DCI VPIPY
	LM
	LR 2,A
	LI CVAL0		; punched through to the band's own ground
	LR 3,A
	LI PIPSZ
	LR 9,A
	PI DFILL
	LEAVE

; ---------------------------------------------------------------------------
; DICERW -- repaint all five dice.
;   Reached by PI; calls two levels down.
; ---------------------------------------------------------------------------
DICERW:	ENTER
	DCI VDIE
	CLR
	ST
DICR1:	PI DIEDRW
	DCI VDIE
	LM
	INC
	DCI VDIE
	ST
	CI NDICE
	BNZ DICR1
	LEAVE

; ---------------------------------------------------------------------------
; ROLLANM -- rattle the dice a roll has just re-rolled.
;
;   WHICH dice rattle comes from the WIRE's keepRoll echo, not the local mask:
;   the echo is what was actually re-rolled, so the animation is right for
;   everybody's rolls and not just ours. The faces shown are the ones that
;   landed -- there is nothing else to show, and a die flickering between its
;   neighbours' values reads as a roll perfectly well.
;
;   Reached by PI; calls two levels down.
; ---------------------------------------------------------------------------
ROLLANM: ENTER
	DCI VANM
	LIS 6
	ST
RANM1:	DCI VDIE
	CLR
	ST
RANM2:	; a die the wire says was KEPT does not move
	DCI VDIE
	LM
	LR 0,A
	DCI FNREPLY+GOKEEP
	LR A,0
	ADC
	LM
	CI '1'
	BNZ RANM3
	DCI VANM
	LM
	NI 1
	BZ RANM4
	PI DIEBLK		; off-phase: the die is in the air
	BR RANM3
RANM4:	PI DIEDRW
RANM3:	DCI VDIE
	LM
	INC
	DCI VDIE
	ST
	CI NDICE
	BNZ RANM2
	LI 6
	LR 3,A
	PI DELAYMS
	DCI VANM
	LM
	AI 0FFH
	DCI VANM
	ST
	CI 0
	BNZ RANM1
	PI DICERW		; land on the real faces whatever the phase
	LEAVE

; The six faces, nine bits each: bit (row*3 + col). Low byte first, and the
; high byte only ever carries position 8.
;         1      2      3      4      5      6
DFACES:	DB 010H,000H		; centre
	DB 001H,001H		; two corners
	DB 011H,001H		; two corners and the centre
	DB 045H,001H		; four corners
	DB 055H,001H		; four corners and the centre
	DB 06DH,001H		; two columns of three

DPIPX:	DB 1,6,11
DPIPY:	DB 1,7,13

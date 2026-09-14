; ---------------------------------------------------------------------------
; card.inc -- the scorecard, four columns by four rows.
;
; The Intellivision fits this game's sixteen slots into two columns of eight
; and still has rows left for dice, a seat strip and a prompt. This console has
; NINE rows in total, so the card gets four of them and the geometry has to be
; the densest arrangement that is still a grid: four columns of five cells --
; a two-character label and a three-character value -- with a one-cell gap
; between them, which is 4*5 + 3 = 23 and therefore the screen exactly.
;
; Laid out that way the grid IS THE WIRE'S OWN ORDER, read across:
;
;     1S 2S 3S 4S        slots  0  1  2  3    ones .. fours
;     5S 6S UP BN        slots  4  5  6  7    fives, sixes, upper total, bonus
;     3K 4K FH SR        slots  8  9 10 11    3-kind, 4-kind, house, small run
;     LR CH FZ TT        slots 12 13 14 15    large run, chance, fujitzee, total
;
; so both coordinates are arithmetic rather than a lookup: the row is
; (slot>>2)+1 and the label column is (slot&3)*6, which is also how the cursor
; finds its cell. The two-character labels are the price of the fourth column
; and they are affordable because the row ORDER is the conventional one -- a
; player who knows the game reads the grid, not the abbreviations.
;
; COLOUR IS THE STATE. The card band runs palette 2, where value 0 is the
; LTGRAY ground itself, so a closed row painted in CVAL0 is INVISIBLE rather
; than dim and simply reads as absent -- the Intellivision's COL_DASH trick on
; a machine that has no black to spend. The corollary is the rule the Arcadia
; port learned twice: nothing that must be read may be drawn in CVAL0.
; ---------------------------------------------------------------------------

CDCOLW	EQU 6*DCELLW		; label + value + the gap

; ---------------------------------------------------------------------------
; CDPREC -- DC0 = the base of player r0's record in the reply window.
;   The stride is constant here, unlike battleship's: this serializer always
;   emits the full 42-byte record.
;   Leaf. Clobbers A, r0, DC0.
; ---------------------------------------------------------------------------
CDPREC:	DCI FNREPLY+GOPLYRS
	LR A,0
	OI 0
	BZ CDPRX
CDPR1:	LI PLSTRID
	ADC
	DS 0
	BNZ CDPR1
CDPRX:	POP

; ---------------------------------------------------------------------------
; RDSCOR -- score slot r0 of player r1, into r0 (low) and r1 (high).
;   The slot goes into r9 on the first instruction because everything after
;   this owns DC0, and reading it back would need a DCI that wipes the pointer
;   CDPREC just built.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
RDSCOR:	ENTER
	LR A,0
	LR 9,A			; the slot
	LR A,1
	LR 0,A			; the player
	PI CDPREC
	LR A,9
	SL 1			; two bytes per score
	AI PLSCORS
	ADC
	LM
	LR 0,A
	LM
	LR 1,A
	LEAVE

; ---------------------------------------------------------------------------
; SCFILL -- is the score word in r0:r1 a real score? Returns A = 0 if so.
;   Both sentinels ($FFFF unset, $FFFE spectator) carry $FF in the high byte,
;   and no real score comes close, so one test covers them.
;   Leaf. Clobbers A.
; ---------------------------------------------------------------------------
SCFILL:	LR A,1
	CI 0FFH
	BZ SCFN
	CLR
	POP
SCFN:	LIS 1
	POP

; ---------------------------------------------------------------------------
; CDLBLP -- DC0 = the two-character label of slot r0.
;   Leaf. Clobbers A, r0, DC0.
; ---------------------------------------------------------------------------
CDLBLP:	DCI CDLBLS
	LR A,0
	OI 0
	BZ CDLBX
CDLB1:	LIS 3
	ADC
	DS 0
	BNZ CDLB1
CDLBX:	POP

; ---------------------------------------------------------------------------
; CDGEOM -- VCX and VCY = the pixel origin of slot VSLOT's cell.
;   Leaf-ish: pure arithmetic, all of it in RAM because the caller is about to
;   spend the register file on a draw.
;   Clobbers A, r0, r1, DC0.
; ---------------------------------------------------------------------------
CDGEOM:	DCI VSLOT
	LM
	NI 3			; column = slot & 3
	LR 0,A
	LI DORGX
	LR 1,A
	LR A,0
	OI 0
	BZ CDG2
CDG1:	LR A,1
	AI CDCOLW
	LR 1,A
	DS 0
	BNZ CDG1
CDG2:	DCI VCX
	LR A,1
	ST

	DCI VSLOT
	LM
	SR 1
	SR 1			; row = (slot >> 2) + 1
	INC
	LR 0,A
	LI DORGY
	LR 1,A
CDG3:	LR A,1
	AI DCELLH
	LR 1,A
	DS 0
	BNZ CDG3
	DCI VCY
	LR A,1
	ST
	POP

; ---------------------------------------------------------------------------
; CDNUM -- draw VCV0:VCV1 right-aligned in slot VSLOT's three value cells, in
;   colour VCCOL.
;
;   Clamped to 999 first. Three cells is what the four-column grid leaves, and
;   a real Fujitzee total does not reach four figures -- but the value is read
;   straight out of a reply window that is never cleared, so a validation
;   escape must not be allowed to run the digits off the safe area.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
CDNUM:	ENTER
	DCI VCV1
	LM
	CI 3
	BM CDNC			; A > 3: at least 1024, clamp
	CI 3
	BNZ CDN1
	DCI VCV0
	LM
	CI 0E7H
	BP CDN1			; 0x03E7 = 999 or less
CDNC:	DCI VCV0
	LI 0E7H
	ST
	LIS 3
	ST

CDN1:	DCI VCV0
	LM
	LR 0,A
	DCI VCV1
	LM
	LR 1,A
	PI DEC5

	; measure, so the field can be right-aligned in three cells
	DCI VDIGIT
	CLR
	LR 0,A
CDN2:	LM
	OI 0
	BZ CDN3
	LR A,0
	INC
	LR 0,A
	CI 3
	BNZ CDN2		; never walk past the field width
CDN3:	DCI VCLEN
	LR A,0
	ST

	; Right-align in the three value cells: the field starts two cells in
	; from the label, so a len-digit number starts at (5 - len) cells.
	DCI VCLEN
	LM
	COM
	INC			; -len
	AI 5			; 5 - len, in cells
	LR 0,A
	DCI VCX
	LM
	LR 1,A			; the running pixel column
CDN4:	LR A,0
	OI 0
	BZ CDN5
	LR A,1
	AI DCELLW
	LR 1,A
	LR A,0
	AI 0FFH
	LR 0,A
	BR CDN4
CDN5:

	DCI VCY
	LM
	LR 2,A
	DCI VCCOL
	LM
	LR 3,A
	DCI VDIGIT
	PI DSTR
	LEAVE

; ---------------------------------------------------------------------------
; SCELL -- draw the one cell for slot VSLOT of the viewed card.
;
;   The colour decision is a chain, most specific first: the cursor wins over
;   the bot-score reveal, which wins over a closed row, which wins over a
;   potential. A filled row is the default ink.
;   Reached by PI; calls two levels down.
; ---------------------------------------------------------------------------
SCELL:	ENTER
	PI CDGEOM

	; ---- the value, and whether there is one -------------------------
	DCI VSLOT
	LM
	LR 0,A
	DCI VVIEWPL
	LM
	LR 1,A
	PI RDSCOR
	DCI VCV0
	LR A,0
	ST
	DCI VCV1
	LR A,1
	ST
	PI SCFILL
	DCI VCFILL
	ST

	; ---- the default ink --------------------------------------------
	LI CVAL1
	DCI VCCOL
	ST

	; ---- an unset row: closed, or carrying a potential ---------------
	DCI VCFILL
	LM
	OI 0
	BZ SCE_DRW		; already filled: nothing else to decide

	DCI VSLOT
	LM
	CI SL_TOTAL
	BZ SCE_DRW		; the grand total is computed, never "closed"
	CI SL_UPTOT
	BZ SCE_DRW
	CI SL_BONUS
	BZ SCE_DRW

	; validScores[] is ALL ZEROS in the lobby -- it is a nil slice there,
	; not a row of -1 -- so only play-class code may read it. Reading it in
	; the lobby paints an open board as a wall of zeros.
	DCI VCLASS
	LM
	CI CLPLAY
	BNZ SCE_DRW

	DCI VSLOT
	LM
	LR 0,A
	DCI FNREPLY+GOVALID
	LR A,0
	ADC
	LM
	LR 9,A			; the potential, in a register no DCI reaches
	CI 0FFH
	BNZ SCE_POT
	LI CVAL0		; closed: the whole row reads as ABSENT
	DCI VCCOL
	ST
	BR SCE_DRW

SCE_POT: DCI VPOT
	LM
	OI 0
	BZ SCE_DRW		; not our card, or not our turn: show nothing
	LR A,9
	OI 0
	BNZ SCE_PY
	; A potential of exactly zero is shown ONLY on the row the cursor is
	; sitting on, matching the reference clients: otherwise an open board
	; is a wall of zeros before the player has decided anything.
	PI SCCURQ
	OI 0
	BZ SCE_DRW
SCE_PY:	DCI VCV0
	LR A,9
	ST
	CLR
	ST			; VCV1: a potential is one byte
	DCI VCFILL
	CLR
	ST			; there IS a number to draw
	LI CVAL2
	DCI VCCOL
	ST

	; ---- the bot-score reveal, then the cursor, both overriding -------
SCE_DRW: DCI VREVSL
	LM
	LR 0,A
	DCI VSLOT
	LM
	XS 0
	BNZ SCE_C
	LI CVAL2
	DCI VCCOL
	ST
SCE_C:	PI SCCURQ
	OI 0
	BZ SCE_P
	LI CVAL3
	DCI VCCOL
	ST

	; ---- paint ------------------------------------------------------
SCE_P:	LIS 5
	LR 0,A
	DCI VCX
	LM
	LR 1,A
	DCI VCY
	LM
	LR 2,A
	PI DCLRR

	DCI VSLOT
	LM
	LR 0,A
	PI CDLBLP
	XDC
	DCI VCX
	LM
	LR 1,A
	DCI VCY
	LM
	LR 2,A
	DCI VCCOL
	LM
	LR 3,A
	XDC
	PI DSTR

	DCI VCFILL
	LM
	OI 0
	BNZ SCEX		; no number in this cell
	PI CDNUM
SCEX:	LEAVE

; ---------------------------------------------------------------------------
; SCCURQ -- is the score cursor on slot VSLOT right now? A = 1 if so.
;   It is only ever on our own card, on our own turn, in SCORE mode.
;   Leaf. Clobbers A, r0, DC0.
; ---------------------------------------------------------------------------
SCCURQ:	DCI VMYTRN
	LM
	OI 0
	BZ SCCNO
	DCI VVIEWPL
	LM
	OI 0
	BNZ SCCNO		; we are looking at somebody else's card
	DCI VUIMOD
	LM
	OI 0
	BZ SCCNO		; the cursor is down among the dice
	DCI VSCUR
	LM
	LR 0,A
	DCI VSLOT
	LM
	XS 0
	BNZ SCCNO
	LIS 1
	POP
SCCNO:	CLR
	POP

; ---------------------------------------------------------------------------
; RUNTOT -- VCV0:VCV1 = the running grand total of the viewed card.
;
;   Summed locally over slots 6-14, because the server only fills the wire's
;   own grand-total slot (15) at game over -- so reading slot 15 during play
;   shows nothing for twelve rounds.
;
;   The subject is VRTPL rather than VVIEWPL, because the standings panel wants
;   every seat's total while the card view stays where the player left it.
;   Reached by PI; calls two levels down.
; ---------------------------------------------------------------------------
RUNTOT:	ENTER
	DCI VRUNT0
	CLR
	ST
	ST
	DCI VTMP
	LI SL_UPTOT
	ST
RT1:	DCI VTMP
	LM
	LR 0,A
	DCI VRTPL
	LM
	LR 1,A
	PI RDSCOR
	LR A,1
	CI 0FFH
	BZ RTNEXT		; unset or spectator: contributes nothing
	LR 4,A			; v high
	LR A,0
	LR 5,A			; v low
	DCI VRUNT0
	LM
	AS 5
	LR 6,A			; new low, and the carry we need
	LI 0
	LNK			; capture it before anything else can
	LR 7,A
	DCI VRUNT1
	LM
	AS 4
	AS 7
	LR 8,A
	DCI VRUNT0
	LR A,6
	ST
	LR A,8
	ST
RTNEXT:	DCI VTMP
	LM
	INC
	DCI VTMP
	ST
	CI SL_FUJI+1
	BNZ RT1
	DCI VRUNT0
	LM
	DCI VCV0
	ST
	DCI VRUNT1
	LM
	DCI VCV1
	ST
	LEAVE

; ---------------------------------------------------------------------------
; CARDRW -- repaint the whole card for the player in VVIEWPL.
;   Reached by PI; calls three levels down.
; ---------------------------------------------------------------------------
CARDRW:	ENTER
	PI CDPOTQ
	DCI VSLOT
	CLR
	ST
CDR1:	DCI VSLOT
	LM
	CI SL_TOTAL
	BZ CDRTOT
	PI SCELL
	DCI VSLOT
	LM
	INC
	DCI VSLOT
	ST
	BR CDR1

	; The grand total is the one cell whose value does not come off the
	; wire, so it is drawn here rather than inside SCELL's chain.
CDRTOT:	PI CDGEOM
	LIS 5
	LR 0,A
	DCI VCX
	LM
	LR 1,A
	DCI VCY
	LM
	LR 2,A
	PI DCLRR
	LI CVAL1
	DCI VCCOL
	ST
	LI SL_TOTAL
	LR 0,A
	PI CDLBLP
	XDC
	DCI VCX
	LM
	LR 1,A
	DCI VCY
	LM
	LR 2,A
	LI CVAL1
	LR 3,A
	XDC
	PI DSTR
	DCI VVIEWPL
	LM
	DCI VRTPL
	ST
	PI RUNTOT
	PI CDNUM
	LEAVE

; ---------------------------------------------------------------------------
; CDPOTQ -- VPOT = may this card show potential scores?
;   Only our own card, only on our own turn, only once something has been
;   rolled. Reached by PI; calls nothing.
;   Clobbers A, DC0.
; ---------------------------------------------------------------------------
CDPOTQ:	DCI VPOT
	CLR
	ST
	DCI VMYTRN
	LM
	OI 0
	BZ CDPQX
	DCI VVIEWPL
	LM
	OI 0
	BNZ CDPQX
	DCI FNREPLY+GODICE
	LM
	OI 0
	BZ CDPQX		; the turn has not rolled yet: nothing to offer
	DCI VPOT
	LIS 1
	ST
CDPQX:	POP

; ---------------------------------------------------------------------------
; SCSCAN -- which row did the seat in r0 just take?
;
;   The wire has no "last move" field: every endpoint returns the same blob and
;   nothing in it says which row a player has just filled. So the client works
;   it out, by walking that seat's sixteen score words into a one-bit-per-slot
;   mask and diffing it against the mask this routine stored last time.
;
;   SLOTS 6, 7 AND 15 CAN NEVER BE THE ANSWER. The upper total, the bonus and
;   the grand total fill as a SIDE EFFECT of the row actually chosen -- slot 6
;   the moment any upper row is taken -- so without excluding them slot 6 wins
;   every time, being the lowest.
;
;   THE SHADOW IS PER PLAYER, so it is STAGED for whoever is up at the bottom
;   of every poll (SCSTAGE) and DIFFED only on the turn edge (SCDIFF). While
;   seat P is up, every poll stores P's mask; when the turn moves on, P's card
;   now differs from that mask by exactly the row they took. Diffing against a
;   shadow staged for somebody else would compare two different cards.
;
;   Leaves VREVSL = the lowest slot that is filled now and was not last time,
;   or $FF; and restages the shadow either way.
;   Reached by PI; calls two levels down.
; ---------------------------------------------------------------------------
SCDIFF:	LIS 1
	DCI VSCDIF
	ST
	JMP SCSCAN		; a tail call: SCSCAN's ENTER saves OUR return

SCSTAGE: CLR
	DCI VSCDIF
	ST
	JMP SCSCAN

SCSCAN:	ENTER
	DCI VSCPL
	LR A,0
	ST
	DCI VNEWM0
	CLR
	ST
	ST
	DCI VSCDIF
	LM
	OI 0
	BZ SCS0
	DCI VREVSL
	LI 0FFH
	ST
SCS0:	DCI VTMP
	CLR
	ST

SCS1:	DCI VTMP
	LM
	LR 0,A
	DCI VSCPL
	LM
	LR 1,A
	PI RDSCOR
	PI SCFILL
	OI 0
	BNZ SCSNXT		; this slot is not filled: leave its bit clear

	; set bit VTMP of the new mask
	DCI VTMP
	LM
	LR 0,A
	LI 1
	LR 1,A			; the walking bit
	LR A,0
	CI 7
	BM SCSHI		; A > 7: this slot's bit is in the high byte
	; low byte
	LR A,0
	OI 0
	BZ SCSLS
SCSLL:	LR A,1
	SL 1
	LR 1,A
	DS 0
	BNZ SCSLL
SCSLS:	DCI VNEWM0
	LM
	AS 1			; the bits are disjoint, so add is or
	DCI VNEWM0
	ST
	BR SCSCMP
SCSHI:	LR A,0
	AI 0F8H			; slot - 8
	LR 0,A
	OI 0
	BZ SCSHS
SCSHL:	LR A,1
	SL 1
	LR 1,A
	DS 0
	BNZ SCSHL
SCSHS:	DCI VNEWM1
	LM
	AS 1
	DCI VNEWM1
	ST

	; ---- newly filled? Only if the old mask had this bit clear --------
SCSCMP:	DCI VSCDIF
	LM
	OI 0
	BZ SCSNXT		; staging only: there is nothing to compare against
	DCI VREVSL
	LM
	CI 0FFH
	BNZ SCSNXT		; we already have an answer; keep the lowest
	DCI VTMP
	LM
	CI SL_UPTOT
	BZ SCSNXT		; a side effect, never the answer
	CI SL_BONUS
	BZ SCSNXT
	CI SL_TOTAL
	BZ SCSNXT
	LR A,1
	LR 2,A			; the bit, before a DCI can reach past it
	DCI VTMP
	LM
	CI 7
	BM SCSCH
	DCI VPRVSC0
	BR SCSCT
SCSCH:	DCI VPRVSC1
SCSCT:	LM
	NS 2
	OI 0
	BNZ SCSNXT		; it was already set last poll
	DCI VTMP
	LM
	DCI VREVSL
	ST

SCSNXT:	DCI VTMP
	LM
	INC
	DCI VTMP
	ST
	CI NSLOTS
	BZ SCSDON
	JMP SCS1		; the body is past a relative branch's reach

SCSDON:	DCI VNEWM0
	LM
	DCI VPRVSC0
	ST
	DCI VNEWM1
	LM
	DCI VPRVSC1
	ST
	LEAVE

; ---------------------------------------------------------------------------
; SCCLR -- park the shadow full, so no bit can have turned ON and nothing is
;   revealed from a comparison that was never valid. Used whenever the shadow
;   might belong to a different player than it did last poll: a class change,
;   or a seat count change.
;   Leaf. Clobbers A, DC0.
; ---------------------------------------------------------------------------
SCCLR:	DCI VPRVSC0
	LI 0FFH
	ST
	LI 0FFH
	ST
	DCI VREVSL
	LI 0FFH
	ST
	POP

; The labels, one NUL-terminated pair each so CDLBLP can step by three.
; Read across, these are the wire's own slot order.
CDLBLS:	DB "1S",0		; 0-5   ones .. sixes
	DB "2S",0
	DB "3S",0
	DB "4S",0
	DB "5S",0
	DB "6S",0
	DB "UP",0		; 6     upper total     (server-computed)
	DB "BN",0		; 7     upper bonus     (server-computed)
	DB "3K",0		; 8-14  three of a kind ..
	DB "4K",0
	DB "FH",0
	DB "SR",0
	DB "LR",0
	DB "CH",0
	DB "FZ",0		;       .. fujitzee
	DB "TT",0		; 15    grand total     (computed here)

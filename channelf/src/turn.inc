; ---------------------------------------------------------------------------
; turn.inc -- the play-class input slice.
;
; A BOUNDED SLICE THAT RETURNS, not a loop that blocks until the move. The
; Intellivision client blocks in its ti_loop until the player acts or the clock
; runs out; the ColecoVision and Arcadia ports slice instead, and they are
; right -- a game that ends while you are still thinking is then noticed within
; a slice, and the poll loop never stops running underneath the cursor.
;
; An ACCEPTED press restarts the full slice, so a player still choosing is
; never interrupted. A REJECTED one earns only a short one: a trigger bounced
; off a dead ROLL tile must not be able to starve the poll loop if it is
; mashed.
;
; The clock counts down LOCALLY from the wire's moveTime and the slice simply
; returns at zero. There is nothing to send: the server has already
; force-scored, and only the next poll learns where the game went.
; ---------------------------------------------------------------------------

TISLICE	EQU 220			; OUR turn: idle passes of about 18 ms, so
				;   roughly four seconds of thinking time before
				;   we interrupt the player to poll
PLYWAIT	EQU 85			; SOMEBODY ELSE'S turn: about a second and a
				;   half, because the only thing happening is on
				;   the wire and the screen should follow it
TISHORT	EQU 55			; what a rejected press earns instead
TISEC	EQU 55			; passes to a second of move clock
IDLEMS	EQU 2			; DELAYMS units per pass -- about 18 ms

; What TURNIN decided, in VFLAG:
TF_POLL	EQU 0			; the slice ran out; go and poll
TF_ROLL	EQU 1			; a roll is staged
TF_SCORE EQU 2			; a score is staged
TF_LEAVE EQU 3			; the player is leaving the table
TF_CLOCK EQU 4			; the move clock expired

; ---------------------------------------------------------------------------
; TURNIN -- run one slice. Leaves the answer in VFLAG.
;   Reached by PI; calls four levels down.
; ---------------------------------------------------------------------------
TURNIN:	ENTER
	LI TF_POLL
	DCI VFLAG
	ST
	PI TISET
	LI TISEC
	DCI VCLKF
	ST

TI1:	PI INDRAIN
	LR A,0
	DCI VEVT
	ST
	CI EV_NONE
	BNZ TIKEY

	; ---- idle: pace the pass, run the clock, expire the slice --------
	LI IDLEMS
	LR 3,A
	PI DELAYMS
	PI GREVTK		; a revealed row retires on the play path too, not
				;   only in the lobby's idle loop
	PI TICLK
	DCI VFLAG
	LM
	CI TF_POLL
	BNZ TIX			; the clock decided for us
	DCI VSLICE
	LM
	AI 0FFH
	DCI VSLICE
	ST
	CI 0
	BNZ TI1
TIX:	LEAVE

; ---- a key ---------------------------------------------------------------
TIKEY:	CI EV_BACK
	BNZ TIK1
	LI TF_LEAVE
	DCI VFLAG
	ST
	BR TIX
TIK1:	CI EV_PGUP
	BNZ TIK2
	BR TIX			; TIME: poll now, VFLAG already TF_POLL
TIK2:	CI EV_PGDN
	BNZ TIK3
	PI SHOWST		; HOLD: the standings, straight from the cache
	PI RENDER
	JMP TIRST
TIK3:	; every other key needs it to actually be our turn
	DCI VMYTRN
	LM
	OI 0
	BZ TIVIEW
	JMP TIACT

; ---- not our turn: the only thing the stick does is page the card --------
TIVIEW:	DCI VEVT
	LM
	CI EV_LEFT
	BNZ TIV1
	PI CDPREV
	PI RENDER
	JMP TIRST
TIV1:	CI EV_RIGHT
	BNZ TIV2
	PI CDNEXT
	PI RENDER
	JMP TIRST
TIV2:	JMP TI1

; ---- our turn ------------------------------------------------------------
TIACT:	DCI VUIMOD
	LM
	OI 0
	BZ TIACD		; invert and JMP: both arms are out of reach
	JMP TISCOR
TIACD:	JMP TIDICE

; ---- DICE mode: the ROLL tile and the five dice --------------------------
TIDICE:	DCI VEVT
	LM
	CI EV_LEFT
	BNZ TID1
	DCI VDCUR
	LM
	OI 0
	BZ TID1A
	AI 0FFH
	BR TID1B
TID1A:	LI NDICE
TID1B:	DCI VDCUR
	ST
	PI SNDKEY
	JMP TIRDDI
TID1:	CI EV_RIGHT
	BNZ TID2
	DCI VDCUR
	LM
	CI NDICE
	BZ TID2A
	INC
	BR TID2B
TID2A:	CLR
TID2B:	DCI VDCUR
	ST
	PI SNDKEY
	JMP TIRDDI
TID2:	CI EV_UP
	BNZ TID3
	LIS 1			; cross to the score rows
	DCI VUIMOD
	ST
	DCI VSCUR
	LM
	DCI VSCPRV
	ST
	PI TIRDSC
	JMP TIRDDI
TID3:	CI EV_DOWN
	BNZ TID4
	DCI VDCUR
	CLR
	ST			; drop onto the ROLL tile
	PI SNDKEY
	JMP TIRDDI
TID4:	CI EV_SEL
	BNZ TIJ1
	DCI VDCUR
	LM
	OI 0
	BZ TIROLL
	JMP TIHOLD
TIJ1:	JMP TI1

; ---- hold or release the die under the cursor ---------------------------
TIHOLD:	DCI VDCUR
	LM
	AI 0FFH			; cursor 1-5 selects die 0-4
	LR 0,A
	DCI VKEEP
	LR A,0
	ADC
	LM
	LR 1,A
	DCI VKEEP
	LR A,0
	ADC
	LR A,1
	CI '1'
	BNZ TIH1
	LI '0'			; it was going to re-roll; keep it instead
	BR TIH2
TIH1:	LI '1'
TIH2:	ST
	PI SNDKEY
	JMP TIRDDI

; ---- roll ---------------------------------------------------------------
TIROLL:	DCI VRLEFT
	LM
	OI 0
	BNZ TIRL1
	JMP TIBAD		; no rolls left: the cursor belongs on a row
TIRL1:	PI KEEPANY		; "00000" would re-roll nothing at all
	OI 0
	BNZ TIRL2
	JMP TIBAD
TIRL2:	LI TF_ROLL
	DCI VFLAG
	ST
	PI SNDSEL
	JMP TIX

; ---- SCORE mode: the four-by-four grid ----------------------------------
TISCOR:	DCI VEVT
	LM
	CI EV_LEFT
	BNZ TIS1
	LI 0FFH			; -1: one column left, wrapping in the row
	DCI VSTEP
	ST
	JMP TIMOVE
TIS1:	CI EV_RIGHT
	BNZ TIS2
	LIS 1
	DCI VSTEP
	ST
	JMP TIMOVE
TIS2:	CI EV_UP
	BNZ TIS3
	LI 0FCH			; -4: one row up, wrapping in the column
	DCI VSTEP
	ST
	JMP TIMOVE
TIS3:	CI EV_DOWN
	BNZ TIS4
	; Down from the bottom row drops back to the dice, but only while
	; there is still a roll to spend: with none left the only legal move
	; is a row, and the cursor has no business leaving them.
	DCI VSCUR
	LM
	CI 11
	BM TIS3A		; A > 11: the bottom row
	LIS 4
	DCI VSTEP
	ST
	JMP TIMOVE
TIS3A:	DCI VRLEFT
	LM
	OI 0
	BNZ TIS3B
	JMP TIBAD
TIS3B:	DCI VUIMOD
	CLR
	ST
	DCI VDCUR
	CLR
	ST
	DCI VSCUR
	LM
	DCI VSCPRV
	ST
	PI TIRDSC
	JMP TIRDDI
TIS4:	CI EV_SEL
	BNZ TIJ2
	JMP TITAKE
TIJ2:	JMP TI1

; ---- move the score cursor, skipping the three computed rows ------------
; Slots 6, 7 and 15 are filled by the server as a side effect of the row
; actually chosen, so they are not selectable and the cursor steps straight
; over them. Stepping again in the SAME direction is what makes that work in
; every case: right from slot 5 walks 6, 7 and wraps to 4, and down from 11
; walks 15 and wraps to 3.
TIMOVE:	DCI VSCUR
	LM
	DCI VSCPRV
	ST			; the origin, saved ONCE and not inside the loop:
				;   a skip steps two or three times on the way
				;   past 6 and 7, and saving per step would leave
				;   the cell the cursor actually CAME from still
				;   painted green -- a second cursor that never
				;   goes away
TIMOV1:	PI TISTEP
	DCI VSCUR
	LM
	CI SL_UPTOT
	BZ TIMOV1
	CI SL_BONUS
	BZ TIMOV1
	CI SL_TOTAL
	BZ TIMOV1
	PI SNDKEY
	PI TIRDSC
	JMP TIRST

; TISTEP -- one step of VSTEP, wrapping within the row for +-1 and within the
;   column for +-4. Leaf. Clobbers A, r0, r1, r2, DC0.
TISTEP:	DCI VSTEP
	LM
	LR 2,A
	DCI VSCUR
	LM
	LR 0,A
	LR A,2
	CI 1
	BZ TIST1
	CI 0FFH
	BZ TIST1
	; a row step: add and keep it inside four rows
	LR A,0
	AS 2
	NI 0FH
	BR TIST2
TIST1:	; a column step: the row is preserved, the column wraps in two bits
	LR A,0
	NI 0CH
	LR 1,A
	LR A,0
	AS 2
	NI 3
	AS 1
TIST2:	DCI VSCUR
	ST
	POP

; ---- take the row under the cursor --------------------------------------
TITAKE:	DCI VSCUR
	LM
	LR 0,A
	DCI FNREPLY+GOVALID
	LR A,0
	ADC
	LM
	CI 0FFH
	BZ TIBAD		; a closed row: the server would refuse it
	DCI VSCUR
	LM
	DCI VSCORIX
	ST
	LI TF_SCORE
	DCI VFLAG
	ST
	PI SNDSEL
	JMP TIX

; ---- a refused press: a short slice, so mashing cannot starve the poll ---
TIBAD:	PI SNDERR
	DCI VSLICE
	LM
	LR 0,A
	LI TISHORT
	COM
	INC
	AS 0
	BNC TIB1		; the slice is already shorter than TISHORT
	LI TISHORT
	DCI VSLICE
	ST
TIB1:	JMP TI1

; ---- an accepted press: repaint what actually moved ---------------------
;
; A full CARDRW is sixteen cells and sixteen decimal conversions -- about a
; third of a second of plotted pixels -- so no keypress is allowed to cost one.
; A cursor move inside the card touches the cell it left and the cell it
; landed on; anything in the dice band repaints the band and the ROLL tile.
; This is ui.inc's "redrawing the gutter is what makes moving the cursor free"
; applied to a screen that has no gutter.
TIRDDI:	PI DICERW
	PI TIROLT
TIRST:	PI TISET
	JMP TI1

; TISET -- (re)charge the slice for whose turn it is.
;   Leaf. Clobbers A, DC0.
TISET:	DCI VMYTRN
	LM
	OI 0
	BZ TISET1
	LI TISLICE
	BR TISET2
TISET1:	LI PLYWAIT
TISET2:	DCI VSLICE
	ST
	POP

; TIRDSC -- repaint the score cell the cursor left and the one it arrived at.
;   VPOT has to be re-derived first: SCELL reads it, and CARDRW is normally
;   what sets it.
;   Reached by PI; calls three levels down.
TIRDSC:	ENTER
	PI CDPOTQ
	DCI VSCPRV
	LM
	DCI VSLOT
	ST
	PI SCELL
	DCI VSCUR
	LM
	DCI VSLOT
	ST
	PI SCELL
	LEAVE

; ---------------------------------------------------------------------------
; TICLK -- one pass of the local move clock. At zero the slice ends with
;   TF_CLOCK: the server force-scored some time ago and there is nothing to
;   send, only a poll to make.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
TICLK:	ENTER
	DCI VMYTRN
	LM
	OI 0
	BZ TICX			; somebody else's clock is not ours to run
	DCI VCLKF
	LM
	AI 0FFH
	DCI VCLKF
	ST
	CI 0
	BNZ TICX
	LI TISEC
	DCI VCLKF
	ST
	DCI VCLOCK
	LM
	OI 0
	BZ TICZ
	AI 0FFH
	DCI VCLOCK
	ST
	PI CDCLK		; repaint the three clock cells
	DCI VCLOCK
	LM
	OI 0
	BNZ TICX
TICZ:	LI TF_CLOCK
	DCI VFLAG
	ST
TICX:	LEAVE

; ---------------------------------------------------------------------------
; KEEPANY -- A = 1 if the staged mask would actually re-roll something.
;   "00000" is rejected here rather than sent: it asks the server to re-roll
;   nothing, which is not a move, and the reference clients refuse it too.
;   Leaf. Clobbers A, r0, DC0.
; ---------------------------------------------------------------------------
KEEPANY: DCI VKEEP
	LIS 5
	LR 0,A
KANY1:	LM
	CI '1'
	BZ KANYY
	DS 0
	BNZ KANY1
	CLR
	POP
KANYY:	LIS 1
	POP

; ---------------------------------------------------------------------------
; TIROLT -- the ROLL tile and its caret, on the footer row.
;
;   The footer runs palette 0, where every non-zero pixel value is the same
;   white, so the cursor CANNOT be shown there as a colour the way it is on the
;   card and the dice. It is a caret in the gap row underneath instead -- the
;   name editor's trick, and the only one available.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
TIROLT:	ENTER
	LIS 8
	LR 0,A
	LI DORGX
	LR 1,A
	LI DORGY+8*DCELLH
	LR 2,A
	PI DCLRR

	DCI VMYTRN
	LM
	OI 0
	BZ TIRTX		; no tile at all when it is not our turn

	LI CVAL1
	LR 3,A
	LI DORGX
	LR 1,A
	LI DORGY+8*DCELLH
	LR 2,A
	DCI SROLL
	PI DSTR
	DCI VRLEFT
	LM
	AI '0'
	DCI VLINE
	ST
	CLR
	ST
	LI CVAL1
	LR 3,A
	LI DORGX+6*DCELLW
	LR 1,A
	LI DORGY+8*DCELLH
	LR 2,A
	DCI VLINE
	PI DSTR

	DCI VUIMOD
	LM
	OI 0
	BNZ TIRTX
	DCI VDCUR
	LM
	OI 0
	BNZ TIRTX
	LI 7*DCELLW		; the caret, under "ROLL xN"
	LR 0,A
	LI DORGX
	LR 1,A
	LI DORGY+8*DCELLH+5
	LR 2,A
	LIS 1
	LR 9,A
	LI CVAL1
	LR 3,A
	PI DFILL
TIRTX:	LEAVE

; ---------------------------------------------------------------------------
; stand.inc -- the standings, on a screen that has no room for them.
;
; The Arcadia port has twenty-six rows and simply keeps every seat's name and
; live total on the play screen. Nine rows cannot, so this is the
; Intellivision's answer instead: an overlay, opened with HOLD, served
; ENTIRELY FROM THE CACHED REPLY. Every seat's record arrived in the same poll
; that drew the card, so there is no round trip and it can be opened and
; dismissed in the middle of somebody else's turn without costing anything.
;
; Seven rows of names against a cap of twelve seats, so the footer carries the
; overflow rather than paging: a player who is thirteenth in a twelve-seat game
; does not exist, and one who is eighth wants to know their total, not to
; scroll for it.
; ---------------------------------------------------------------------------

STNROW	EQU 7			; ui.inc gives us exactly NROWS of them

; ---------------------------------------------------------------------------
; SHOWST -- draw the overlay, wait for any key, then hand back. The caller
;   repaints the play screen from the same cached reply.
;   Reached by PI; calls four levels down.
; ---------------------------------------------------------------------------
SHOWST:	ENTER
	PI CLSTXT
	LI CVAL1
	LR 3,A
	DCI SSTAND
	XDC
	PI UITITLE
	PI UICLRL

	DCI VSEAT
	CLR
	ST
STD1:	DCI VSEAT
	LM
	LR 0,A
	DCI VPC
	LM
	COM
	INC
	AS 0			; seat - playerCount
	BC STDOVF		; no borrow: we have run out of seats
	DCI VSEAT
	LM
	CI STNROW
	BZ STDOVF
	PI STROW
	DCI VSEAT
	LM
	INC
	DCI VSEAT
	ST
	BR STD1

	; ---- the overflow line, if twelve seats did not fit in seven ------
	; Drawn in the FOOTER, which runs palette 0: every non-zero value is
	; white there, so this cannot accidentally be painted in a ground
	; colour and vanish the way a card cell can.
STDOVF:	DCI VPC
	LM
	CI STNROW
	BP STDF			; A <= 7: everybody is on screen
	DCI VPC
	LM
	AI 256-STNROW
	DCI VTMP3
	ST
	LI CVAL1
	LR 3,A
	LI DORGX
	LR 1,A
	LI DORGY+8*DCELLH
	LR 2,A
	DCI SPLUS
	PI DSTR
	PI STMORE
	BR STDW

STDF:	LI CVAL1
	LR 3,A
	DCI SANYKEY
	XDC
	PI UIFOOT

	DCI VSTICK		; a latched press belongs to the screen we just
	CLR			;   left, and here it would dismiss this one
	ST			;   before it had been read
STDW:	PI INSCAN
	LR A,0
	CI EV_NONE
	BZ STDW

	; Put the caller's screen back. This is NOT optional and it is not just
	; the pixels: CLSTXT above set palette 0 on all 64 rows, so returning
	; without re-banding leaves the card and the dice in the monochrome
	; palette -- every ink the same white, a closed row suddenly legible,
	; and a held die indistinguishable from an ordinary one. GCLSEL picks
	; the right clear for whichever class we came from; the caller's RENDER
	; then repaints over it.
	PI GCLSEL
	LEAVE

; ---------------------------------------------------------------------------
; STMORE -- finish the overflow line: the count, then the word.
;   Split out of SHOWST only because SHOWST is already four levels deep at
;   this point and the digits need DEC5, which needs one more.
;   Reached by PI; calls two levels down.
; ---------------------------------------------------------------------------
STMORE:	ENTER
	DCI VTMP3
	LM
	LR 0,A
	CLR
	LR 1,A
	PI DEC5
	LI CVAL1
	LR 3,A
	LI DORGX+DCELLW
	LR 1,A
	LI DORGY+8*DCELLH
	LR 2,A
	DCI VDIGIT
	PI DSTR
	LI CVAL1
	LR 3,A
	LI DORGX+4*DCELLW
	LR 1,A
	LI DORGY+8*DCELLH
	LR 2,A
	DCI SMORE
	PI DSTR
	LEAVE

; ---------------------------------------------------------------------------
; STROW -- one seat: its name, and its live running total right-aligned.
;
;   The name is drawn in the ink that says who it is -- but only two of them
;   carry meaning here, because this screen runs palette 0 where every
;   non-zero value is the same white. So the row the player is LOOKING at is
;   marked instead with the cursor gutter, which UIROW leaves alone.
;   Reached by PI; calls four levels down.
; ---------------------------------------------------------------------------
STROW:	ENTER
	DCI VSEAT
	LM
	LR 0,A
	PI CDPREC
	XDC
	DCI VLINE
	XDC
	LI NAMEMAX
	LR 0,A
	PI GUPCPY

	DCI VSEAT
	LM
	LR 0,A
	LI CVAL1
	LR 3,A
	DCI VLINE
	XDC
	PI UIROW

	DCI VSEAT
	LM
	DCI VRTPL
	ST
	PI RUNTOT
	DCI VCV0
	LM
	LR 0,A
	DCI VCV1
	LM
	LR 1,A
	PI DEC5
	DCI VSEAT
	LM
	LR 0,A
	LI CVAL1
	LR 3,A
	DCI VDIGIT
	XDC
	PI UIROWR
	LEAVE

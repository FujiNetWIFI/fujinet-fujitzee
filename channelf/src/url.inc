; ---------------------------------------------------------------------------
; url.inc -- building the devicespec straight into the TX page.
;
; Nothing is assembled in RAM: the cart counts the payload itself, so a write
; ANYWHERE in the TX page appends and the whole request is a run of stores.
; The worst case is /score/14 with the query, about 90 bytes against the
; 320-byte TX ceiling, so the 256-store rule in fujilib.inc is never in reach.
;
; Every helper re-points DC0 at the TX page itself rather than trusting the
; caller to have left it there. A DCI between FNBEG and an append -- reading a
; variable, say -- otherwise redirects the bytes into RAM: the command goes out
; with no payload AND scribbles over whatever the pointer was aimed at. The
; write cursor lives in the cart, so re-pointing cannot rewind the stream.
;
; VTABLE and VNAME are NUL-terminated copies of fixed-width wire fields, pushed
; up to the NUL and never through it. The URL's length is a byte count, not a
; NUL-terminated string, so an embedded $00 corrupts the request on the wire.
; ---------------------------------------------------------------------------

; UTXB -- append the single byte in r0. Leaf. Clobbers A, DC0.
UTXB:	DCI FNTX
	LR A,0
	ST
	POP

; ---------------------------------------------------------------------------
; BLDURL -- stream the devicespec for the request in VREQ.
;   Reached by PI; calls one level down (FNTXS, UTXB, UDEC).
; ---------------------------------------------------------------------------
BLDURL:	ENTER
	DCI UNPFX
	XDC
	PI FNTXS
	DCI UENDPT
	XDC
	PI FNTXS

	DCI VREQ
	LM
	CI RQTABLE
	BNZ BU1
	DCI UTABLS		; tables takes neither a table nor a player,
	XDC			;   so it carries its own ?bin=1
	PI FNTXS
	LEAVE

BU1:	CI RQSTATE
	BNZ BU2
	DCI USTATE
	XDC
	PI FNTXS
	BR BUQ

BU2:	CI RQREADY
	BNZ BU3
	; The BARE /ready, which TOGGLES -- matching the Intellivision and
	; ColecoVision clients. This server has no /ready/1 form; the toggle is
	; edge-detected in the input path instead, so a press between two polls
	; is never missed and never doubled.
	DCI UREADY
	XDC
	PI FNTXS
	BR BUQ

BU3:	CI RQROLL
	BNZ BU4
	DCI UROLL
	XDC
	PI FNTXS
	; The staged mask needs no conversion: VKEEP already holds the five
	; ascii '0'/'1' bytes the wire wants, so it streams verbatim.
	DCI VKEEP
	XDC
	PI FNTXS
	BR BUQ

BU4:	CI RQSCORE
	BNZ BU5
	DCI USCORE
	XDC
	PI FNTXS
	DCI VSCORIX
	LM
	DCI VTMP3
	ST
	PI UDEC
	BR BUQ

BU5:	DCI ULEAVE
	XDC
	PI FNTXS

BUQ:	DCI UQTAB
	XDC
	PI FNTXS
	DCI VTABLE
	XDC
	PI FNTXS
	DCI UQPLR
	XDC
	PI FNTXS
	DCI VNAME
	XDC
	PI FNTXS
	DCI UQBIN
	XDC
	PI FNTXS
	LEAVE

; ---------------------------------------------------------------------------
; UDEC -- append VTMP3 (0-255) as one to three decimal digits, no leading
;   zeros. Takes its byte in RAM, not a register: BLDURL's own dispatch has
;   already used the accumulator and FNTXS writes both data counters.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
UDEC:	ENTER
	DCI VTMP3
	LM
	LR 1,A			; the running remainder
	CLR
	LR 2,A			; hundreds
UD1:	LR A,1
	AI 156			; -100
	BNC UD2
	LR 1,A
	LR A,2
	INC
	LR 2,A
	BR UD1
UD2:	CLR
	LR 3,A			; tens
UD3:	LR A,1
	AI 246			; -10
	BNC UD4
	LR 1,A
	LR A,3
	INC
	LR 3,A
	BR UD3
UD4:	LR A,2
	OI 0
	BZ UD5
	AI '0'
	LR 0,A
	PI UTXB
	LIS 1
	LR 4,A			; something has been emitted
	BR UD6
UD5:	CLR
	LR 4,A
UD6:	LR A,3
	OI 0
	BNZ UD7
	LR A,4
	OI 0
	BZ UD8			; nothing emitted yet and tens 0: skip
UD7:	LR A,3
	AI '0'
	LR 0,A
	PI UTXB
UD8:	LR A,1
	AI '0'
	LR 0,A
	PI UTXB
	LEAVE

	INCLUDE "endpoint.inc"
UNPFX:	DB "N:",0
UTABLS:	DB "tables?bin=1",0
USTATE:	DB "state",0
UREADY:	DB "ready",0
UROLL:	DB "roll/",0
USCORE:	DB "score/",0
ULEAVE:	DB "leave",0
UQTAB:	DB "?table=",0
UQPLR:	DB "&player=",0
UQBIN:	DB "&bin=1",0

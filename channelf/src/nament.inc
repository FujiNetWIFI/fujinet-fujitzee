; ---------------------------------------------------------------------------
; nament.inc -- who you are, and which table you were sent to.
;
; The name comes from the FujiNet SHARED USERNAME APPKEY (creator 1, app 1,
; key 0), the same one every other client in the family reads, so a player who
; has typed their name into 5 Card Stud or CONFIG never types it again here.
; Only when that key is empty does the keyboard come up -- and what is typed is
; written back, so it is empty exactly once per adapter.
;
; Appkey 1/1/3 is the LOBBY ROOM HANDOFF: the FujiNet lobby writes
; "<endpoint>?table=<id>" there when it sends a player into a game, so a client
; that finds one can sit straight down without showing a table list at all. It
; is cleared on a deliberate leave, and a malformed value is simply ignored --
; the table screen is always a correct fallback.
;
; TWO GOTCHAS, both of which cost other ports real time. The OPEN struct is SIX
; bytes -- creator lo, creator hi, app, key, mode, reserved -- and sending five
; blocks the firmware, which then reads back as a TIMEOUT rather than as a
; protocol error. And an appkey READ reply is PREFIXED WITH A 2-BYTE LENGTH; a
; network read is not.
;
; THE KEYBOARD. Three rows of thirteen, driven by the stick, with the plunger
; picking. There is no way to invert a cell -- VRAM cannot be read back at all
; -- so the selection is a caret drawn in the gap row underneath, which is one
; 3-pixel bar to erase and one to draw per keypress.
; ---------------------------------------------------------------------------

KBCOLS	EQU 13
KBROWS	EQU 3
KBX	EQU DORGX+5*DCELLW	; the grid, centred: cells 5..17
KBY	EQU DORGY+3*DCELLH
KBDEL	EQU 11			; the two action keys, on the bottom row
KBOK	EQU 12

; ---------------------------------------------------------------------------
; SNAME -- fetch the shared username, and only ask if there is not one.
; ---------------------------------------------------------------------------
SNAME:	PI CLSTXT
	LI CVAL1
	LR 3,A
	DCI STITLE
	XDC
	PI UITITLE
	LI CVAL1
	LR 3,A
	DCI SFUJI
	XDC
	PI UIFOOT

	LI AKKEY
	LR 0,A
	LI AKRDMOD
	LR 1,A
	PI AKOPEN
	OI 0
	BNZ SNTYPE
	PI AKREAD
	OI 0
	BNZ SNTYPE
	DCI FNREPLY		; the length word, then the value; an empty key
	LM			;   answers with a length of zero
	OI 0
	BZ SNTYPE
	DCI FNREPLY+2
	XDC
	DCI VNAME
	XDC
	LI NAMEMAX
	LR 4,A
SNCP:	LM			; the SOURCE first: the XDC dance has a direction
	XDC
	ST
	XDC
	OI 0
	BZ SNGO
	DS 4
	BNZ SNCP
SNGO:	DCI VNAME+NAMEMAX
	CLR
	ST
	DCI VNAME
	LM
	OI 0
	BZ SNTYPE
	JMP SNROOM

SNTYPE:	DCI VENTRY
	CLR
	ST
	DCI VEDLEN
	CLR
	ST
	ST			; VEDCASE: unused, the server lowercases anyway
	DCI VEDX
	CLR
	ST
	ST			; VEDY
	LI ST_EDIT
	LR 0,A
	JMP GOTO

; ---------------------------------------------------------------------------
; SNROOM -- did the lobby send us somewhere? Appkey 1/1/3 holds
;   "<endpoint>?table=<id>"; all this client wants is the id after "?table=".
;   Anything it cannot parse falls through to the table list, which is always
;   a correct answer.
; ---------------------------------------------------------------------------
SNROOM:	LI AKKROOM
	LR 0,A
	LI AKRDMOD
	LR 1,A
	PI AKOPEN
	OI 0
	BNZ SNRNO
	PI AKREAD
	OI 0
	BNZ SNRNO
	DCI FNREPLY
	LM
	OI 0
	BZ SNRNO		; an empty key: no handoff waiting

	; scan the value for '=', which only appears in "?table="
	DCI FNREPLY+2
	LI 200
	LR 4,A
SNR1:	LM
	OI 0
	BZ SNRNO		; end of the value, and no query in it
	CI '='
	BZ SNR2
	DS 4
	BNZ SNR1
	BR SNRNO
SNR2:	; DC0 is now on the first character of the id
	XDC
	DCI VTABLE
	XDC
	LI NAMEMAX
	LR 4,A
SNR3:	LM
	CI '&'
	BZ SNR4			; a query we do not use starts here
	OI 0
	BZ SNR4
	XDC
	ST
	XDC
	DS 4
	BNZ SNR3
SNR4:	XDC
	CLR
	ST
	DCI VTABLE
	LM
	OI 0
	BZ SNRNO		; "?table=" with nothing after it
	PI TBCLR		; the shadows belong to whatever we played last
	LI ST_GAME
	LR 0,A
	JMP GOTO

SNRNO:	LI ST_TABLE
	LR 0,A
	JMP GOTO

; ---------------------------------------------------------------------------
; AKOPEN -- OPEN_APPKEY(creator, app, key r0, mode r1). SIX one-byte params:
;   the creator is a 16-bit value split across two of them, and the sixth is
;   reserved. Five parameters block the firmware outright.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
AKOPEN:	ENTER
	LR A,0
	LR 9,A			; the key, past anything FNBEG touches
	LR A,1
	LR 10,A			; and the mode
	LI FDEV
	LR 0,A
	LI FC_AKOPN
	LR 1,A
	LIS 6
	LR 2,A
	PI FNBEG
	LI AKCREAT
	LR 0,A
	PI FNPB
	LI AKCREAH
	LR 0,A
	PI FNPB
	LI AKAPP
	LR 0,A
	PI FNPB
	LR A,9
	LR 0,A
	PI FNPB
	LR A,10
	LR 0,A
	PI FNPB
	CLR
	LR 0,A
	PI FNPB			; reserved
	PI FNGO
	PI FNACK
	LEAVE

; AKREAD -- READ_APPKEY. The reply is a 2-byte little-endian length and then
;   the value; a network read carries no such prefix.
AKREAD:	ENTER
	LI FDEV
	LR 0,A
	LI FC_AKRD
	LR 1,A
	CLR
	LR 2,A
	PI FNBEG
	PI FNGO
	PI FNACK
	LEAVE

; AKWRITE -- WRITE_APPKEY with VNAME: a length parameter, then the value as a
;   fixed-width field. Best effort -- an adapter that will not store it is not
;   a reason to refuse to play.
AKWRITE: ENTER
	LI AKKEY
	LR 0,A
	LI AKWRMOD
	LR 1,A
	PI AKOPEN
	OI 0
	BNZ AKWX

	LI FDEV
	LR 0,A
	LI FC_AKWR
	LR 1,A
	LIS 1
	LR 2,A
	PI FNBEG
	DCI VEDLEN
	LM
	LR 0,A
	PI FNPB
	DCI VNAME
	XDC
	LI NAMEMAX+1
	LR 0,A
	PI FNFIX
	PI FNGO
	PI FNACK
AKWX:	LEAVE

; ---------------------------------------------------------------------------
; AKWROOM -- write the room handoff, "<endpoint>?table=<id>", into appkey
;   1/1/3, so the FujiNet lobby and any client that reads it knows where this
;   player went. AKCROOM clears it again on a deliberate leave.
;
;   Both are BEST EFFORT. An adapter that will not store an appkey is not a
;   reason to refuse to play, so neither failure is ever propagated.
;   Reached by PI; calls two levels down.
; ---------------------------------------------------------------------------
AKWROOM: ENTER
	DCI VROOM
	XDC
	DCI UENDPT
	PI SBCAT
	DCI UQTAB
	PI SBCAT
	DCI VTABLE
	PI SBCAT
	PI SBEND
	DCI VROOM		; the byte count the WRITE wants
	CLR
	LR 9,A
AKWR1:	LM
	OI 0
	BZ AKWR2
	LR A,9
	INC
	LR 9,A
	CI 250
	BNZ AKWR1
AKWR2:	LR A,9
	LR 10,A
	LI AKKROOM
	LR 0,A
	LI AKWRMOD
	LR 1,A
	PI AKOPEN
	OI 0
	BNZ AKWRX
	LI FDEV
	LR 0,A
	LI FC_AKWR
	LR 1,A
	LIS 1
	LR 2,A
	PI FNBEG
	LR A,10
	LR 0,A
	PI FNPB
	DCI VROOM
	XDC
	LR A,10
	LR 0,A
	PI FNFIX
	PI FNGO
AKWRX:	LEAVE

AKCROOM: ENTER
	LI AKKROOM
	LR 0,A
	LI AKWRMOD
	LR 1,A
	PI AKOPEN
	OI 0
	BNZ AKCRX
	LI FDEV
	LR 0,A
	LI FC_AKWR
	LR 1,A
	LIS 1
	LR 2,A
	PI FNBEG
	CLR
	LR 0,A
	PI FNPB			; a zero-length value: the room is vacated
	PI FNGO
AKCRX:	LEAVE

; ---------------------------------------------------------------------------
; SEDIT -- the on-screen keyboard.
; ---------------------------------------------------------------------------
SEDIT:	PI CLSTXT
	LI CVAL1
	LR 3,A
	DCI SYNAME
	XDC
	PI UITITLE
	LI CVAL1
	LR 3,A
	DCI SNMKEY
	XDC
	PI UIFOOT

	DCI VTMP		; the three key rows
	CLR
	ST
SED1:	DCI VTMP
	LM
	LR 0,A
	PI EDROWY		; r2 = the pixel row
	LI CVAL1
	LR 3,A
	LI KBX
	LR 1,A
	DCI VTMP		; EDROWP sets DC0, so it goes LAST -- reading the
	LM			;   row number needs a DCI of its own and would
	LR 0,A			;   otherwise wipe the pointer it just built
	PI EDROWP
	PI DSTR
	DCI VTMP
	LM
	INC
	DCI VTMP
	ST
	CI KBROWS
	BNZ SED1

	PI EDECHO
	PI EDCARD

SED2:	PI INSCAN
	LR A,0
	CI EV_NONE
	BZ SED2
	LR A,0
	CI EV_UP
	BNZ SEK1
	DCI VEDY
	LM
	OI 0
	BZ SED2
	AI 0FFH
	DCI VEDY
	ST
	JMP SEDCUR
SEK1:	CI EV_DOWN
	BNZ SEK2
	DCI VEDY
	LM
	CI KBROWS-1
	BZ SED2
	INC
	DCI VEDY
	ST
	JMP SEDCUR
SEK2:	CI EV_LEFT
	BNZ SEK3
	DCI VEDX
	LM
	OI 0
	BZ SED2
	AI 0FFH
	DCI VEDX
	ST
	JMP SEDCUR
SEK3:	CI EV_RIGHT
	BNZ SEK4
	DCI VEDX
	LM
	CI KBCOLS-1
	BZ SED2
	INC
	DCI VEDX
	ST
	JMP SEDCUR
SEK4:	CI EV_BACK
	BZ SEDDEL
	CI EV_PGUP
	BZ SEDOK
	CI EV_SEL
	BNZ SED2
	; the bottom row's last two cells are DELETE and ACCEPT
	DCI VEDY
	LM
	CI KBROWS-1
	BNZ SEDINS
	DCI VEDX
	LM
	CI KBDEL
	BZ SEDDEL
	CI KBOK
	BZ SEDOK

SEDINS:	DCI VEDLEN
	LM
	CI NAMEMAX
	BZ SEDFUL
	PI EDCHAR		; A = the character under the caret
	LR 1,A
	DCI VEDLEN
	LM
	LR 0,A
	DCI VENTRY
	LR A,0
	ADC
	LR A,1
	ST
	CLR
	ST
	DCI VEDLEN
	LM
	INC
	DCI VEDLEN
	ST
	PI EDECHO
	JMP SED2
SEDFUL:	PI SNDERR
	JMP SED2

SEDDEL:	DCI VEDLEN
	LM
	OI 0
	BZ SEDFUL
	AI 0FFH
	DCI VEDLEN
	ST
	LR 0,A
	DCI VENTRY
	LR A,0
	ADC
	CLR
	ST
	PI EDECHO
	JMP SED2

SEDCUR:	PI EDCARD
	JMP SED2

; ---------------------------------------------------------------------------
SEDOK:	DCI VEDLEN		; an empty name is not a name
	LM
	OI 0
	BZ SEDDEF
	DCI VENTRY
	XDC
	DCI VNAME
	XDC
	LI NAMEMAX+1
	LR 4,A
SEDCP:	LM
	XDC
	ST
	XDC
	OI 0
	BZ SEDW
	DS 4
	BNZ SEDCP
SEDW:	PI AKWRITE		; best effort: an unwritable appkey is not fatal
	LI ST_TABLE
	LR 0,A
	JMP GOTO

SEDDEF:	DCI SDEFNM		; the default, so a player who just presses OK
	XDC			;   still gets a seat
	DCI VNAME
	XDC
	LI NAMEMAX+1
	LR 4,A
SEDD1:	LM
	XDC
	ST
	XDC
	OI 0
	BZ SEDD2
	DS 4
	BNZ SEDD1
SEDD2:	LI ST_TABLE
	LR 0,A
	JMP GOTO

; ---------------------------------------------------------------------------
; EDROWP -- DC0 = the characters of key row r0.
;   Leaf. Clobbers A, r0, DC0.
; ---------------------------------------------------------------------------
EDROWP:	DCI KBCHRS
	LR A,0
	OI 0
	BZ EDRPX
EDRP1:	LI KBCOLS+1
	ADC
	DS 0
	BNZ EDRP1
EDRPX:	POP

; EDROWY -- r2 = the pixel row of key row r0. Leaf. Clobbers A, r0, r2.
EDROWY:	CLR
	LR 2,A
	LR A,0
	OI 0
	BZ EDRYX
EDRY1:	LR A,2
	AI DCELLH
	LR 2,A
	DS 0
	BNZ EDRY1
EDRYX:	LR A,2
	AI KBY
	LR 2,A
	POP

; ---------------------------------------------------------------------------
; EDCHAR -- A = the character under the caret.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
EDCHAR:	ENTER
	DCI VEDX		; the column first, into a register no DCI can
	LM			;   reach, because EDROWP owns DC0 from here on
	LR 9,A
	DCI VEDY
	LM
	LR 0,A
	PI EDROWP
	LR A,9
	ADC
	LM
	LEAVE

; ---------------------------------------------------------------------------
; EDECHO -- redraw the name being typed.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
EDECHO:	ENTER
	LI DCOLS
	LR 0,A
	LI DORGX
	LR 1,A
	LI DORGY+DCELLH
	LR 2,A
	PI DCLRR
	LI CVAL1
	LR 3,A
	LI KBX
	LR 1,A
	LI DORGY+DCELLH
	LR 2,A
	DCI VENTRY
	PI DSTR
	LEAVE

; ---------------------------------------------------------------------------
; EDCARD -- move the caret: blank the whole gap row under the keyboard, then
;   draw one 3-pixel bar under the selected cell. Redrawing the strip rather
;   than tracking the old position is 39 pixels against the bookkeeping, and
;   it cannot drift.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
EDCARD:	ENTER
	DCI VTMP
	CLR
	ST
EDC1:	DCI VTMP
	LM
	LR 0,A
	PI EDROWY
	LR A,2
	AI 5			; the gap row inside the 4x6 cell
	LR 2,A
	LI KBCOLS*DCELLW
	LR 0,A
	LI KBX
	LR 1,A
	LIS 1
	LR 9,A
	LI CVAL0
	LR 3,A
	PI DFILL
	DCI VTMP
	LM
	INC
	DCI VTMP
	ST
	CI KBROWS
	BNZ EDC1

	DCI VEDY
	LM
	LR 0,A
	PI EDROWY
	LR A,2
	AI 5
	LR 2,A
	DCI VEDX
	LM
	SL 1
	SL 1
	AI KBX
	LR 1,A
	LIS 3
	LR 0,A
	LIS 1
	LR 9,A
	LI CVAL1
	LR 3,A
	PI DFILL
	LEAVE

; The grid, one NUL-terminated row each so EDROWP can step by KBCOLS+1.
; '<' deletes and '>' accepts; MODE and TIME do the same thing for a player
; who would rather not walk the caret over there.
KBCHRS:	DB "ABCDEFGHIJKLM",0
	DB "NOPQRSTUVWXYZ",0
	DB "0123456789-<>",0

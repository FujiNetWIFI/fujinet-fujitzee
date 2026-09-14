; ---------------------------------------------------------------------------
; strdata.inc -- every literal that reaches the screen.
;
; Collected in one place because they are the easiest thing to overrun: the
; screen is 23 cells wide and DSTR clips nothing, so a string that is too long
; simply walks off the safe area and, past column 124, into the columns that
; carry the PALETTE. The widths are noted where they are close.
;
; Upper case throughout. The font's lower case is the capitals squashed into
; four rows -- legible enough to show a player which case they just typed, and
; not legible enough for anything else.
; ---------------------------------------------------------------------------

; --- the two screens before a table ---------------------------------------
STITLE:	DB "FUJITZEE",0
SFUJI:	DB "READING APPKEY",0
SYNAME:	DB "YOUR NAME",0
SNMKEY:	DB "FIRE PICK  MODE DEL",0		; 19
SDEFNM:	DB "CHANF",0			; a player who just presses OK

; --- the table list --------------------------------------------------------
SCHOOSE: DB "CHOOSE A TABLE",0
SJOINK:	DB "FIRE JOIN  MODE NAME",0	; 20
SNOTBL:	DB "NO TABLES",0

; --- the lobby -------------------------------------------------------------
SLOBBY:	DB "WAITING ROOM",0
SREADY:	DB "READY",0

; --- the play screen -------------------------------------------------------
; SROUND sits at cell 9 and SOF13 at cell 13, with the round's two digits
; between them: "R nn/13" without ever building the string.
SROUND:	DB "R",0
SOF13:	DB "/13",0
; SROLL occupies cells 0-5 and TIROLT puts the rolls-left digit at cell 6, so
; the tile reads "ROLL X2" and the caret under it is seven cells wide.
SROLL:	DB "ROLL X",0

; --- the standings overlay -------------------------------------------------
SSTAND:	DB "STANDINGS",0
SPLUS:	DB "+",0
SMORE:	DB "MORE",0

; --- failure ---------------------------------------------------------------
SNETER:	DB "NET ERR",0
SANYKEY: DB "ANY KEY TO GO BACK",0	; 18
SNOFN:	DB "NO FUJINET CART",0

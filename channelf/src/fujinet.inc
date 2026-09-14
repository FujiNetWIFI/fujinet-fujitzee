; ---------------------------------------------------------------------------
; fujinet.inc -- N: device commands and the Fujitzee binary wire format.
;
; EQUs only; no code. The offsets are the ?bin=1 packed serialization, checked
; against the shared C client's packed structs (../src/misc.h), the
; Intellivision client's byte-exact map (../intv/state.bas) and the
; ColecoVision port's fujinet.inc, which all agree.
;
; Unlike every sibling console the whole reply is FLAT: GMAXLEN is 599 and the
; cart's reply window at $F800 is 1K, so a player record is one DCI and the
; score array is a straight walk. No slices, no 16-bit cursor, no bounce
; buffer -- the Arcadia port needed all three for the same job.
; ---------------------------------------------------------------------------

; --- N: device ------------------------------------------------------------
NDEV	EQU 071H		; N1:
NCOPEN	EQU 04FH		; 'O'
NCCLOSE	EQU 043H		; 'C'
NCREAD	EQU 052H		; 'R'
NCSTAT	EQU 053H		; 'S'
NMHTTPG	EQU 00CH		; open mode: HTTP GET
NTRNONE	EQU 000H		; translation: none

; NET_STATUS reply, 4 bytes at the head of the reply window
NSAVLO	EQU 0			; bytes waiting, low
NSAVHI	EQU 1			; bytes waiting, high
NSCONN	EQU 2			; connected flag
NSDEVST	EQU 3			; nDevStatus_t; 1 = SUCCESS

; --- Fuji device, for the lobby appkeys -----------------------------------
FDEV	EQU 070H
FC_AKOPN EQU 0DCH		; OPEN_APPKEY
FC_AKRD	EQU 0DDH		; READ_APPKEY
FC_AKWR	EQU 0DEH		; WRITE_APPKEY
; The OPEN struct is SIX bytes -- creator lo, creator hi, app, key, mode,
; reserved. Sending five blocks the firmware and reads back as a TIMEOUT
; rather than a protocol error, which is a long way from the real cause.
AKCREAT	EQU 1			; the shared lobby username every client reads
AKCREAH	EQU 0
AKAPP	EQU 1
AKKEY	EQU 0			; key 0: the username
AKKROOM	EQU 3			; key 3: "<endpoint>?table=<id>" room handoff
AKRDMOD	EQU 0
AKWRMOD	EQU 1

; --- request ids (VREQ) ---------------------------------------------------
; RQTABLE is 0, so a cleared "nothing staged" cell must never be read as a
; request id. VPEND uses 0 for "nothing staged" and the poll substitutes
; RQSTATE; battleship shipped the other way once and refetched the table
; list forever.
RQTABLE	EQU 0
RQSTATE	EQU 1
RQREADY	EQU 2
RQROLL	EQU 3
RQSCORE	EQU 4
RQLEAVE	EQU 5

; --- GameState: a 95-byte header, then playerCount x 42 -------------------
; Returned identically by /state, /ready, /roll, /score and /leave. A staged
; action rides the next request in place of /state and its reply IS the next
; state; there is never a second round trip.
GOPC	EQU 0			; playerCount, spectators included
GOSRVNM	EQU 1			; serverName[21]
GOPRMPT	EQU 22			; prompt[41]
GORND	EQU 63			; 0 lobby, 1-13 play, 99 gameover
GORLEFT	EQU 64			; rolls left this turn, 3 down to 0
GOACTPL	EQU 65			; int8; $FF = nobody, 0 = YOUR turn --
				;   the server rotates you to index 0
GOMVTIM	EQU 66			; seconds left on the move clock, 0-250
GOVIEW	EQU 67			; 1 = you are a spectator: input disabled
GODICE	EQU 68			; dice[6]: five ascii '1'-'6' + NUL.
				;   NULs at the top of a turn, before any roll.
GOKEEP	EQU 74			; keepRoll[6]: five ascii '0'/'1' + NUL.
				;   '1' means RE-ROLL, not hold -- and it is an
				;   ECHO of what was last re-rolled, so it moves
				;   only when a roll goes out. See DIEHLD.
GOVALID	EQU 80			; validScores[15], signed: $FF = closed row,
				;   else the points that row would earn now.
				;   ALL ZERO IN THE LOBBY -- only play-class code
				;   may read it, or the card paints as zeros.
GOPLYRS	EQU 95			; playerCount x PLSTRID

PLSTRID	EQU 42
PLNAME	EQU 0			; name[9], lowercased on the wire
PLALIAS	EQU 9			; index of the highlight char; always 0 live
PLSCORS	EQU 10			; scores[16], u16 LITTLE-ENDIAN each

; Score-word sentinels. These are u16 on the wire; -1 and -2 signed.
SCUNSET	EQU 0FFFFH		; row not taken
SCVIEW	EQU 0FFFEH		; this seat is a spectator
; In the LOBBY scores[0] carries the ready flag instead: 1 ready, 0 not.

; --- the sixteen score slots ----------------------------------------------
; Slots 6, 7 and 15 are computed by the server and are NOT selectable. They
; also can never be the answer to "which row did that bot just take": slot 6
; fills the moment ANY upper row is taken, so a lowest-set-bit search that
; does not exclude them returns 6 every time.
SL_ONES	EQU 0
SL_SIXES EQU 5
SL_UPTOT EQU 6
SL_BONUS EQU 7
SL_SET3	EQU 8
SL_FUJI	EQU 14
SL_TOTAL EQU 15
NSLOTS	EQU 16

; --- /tables --------------------------------------------------------------
; A COUNT byte, then that many records -- the records start at offset 1.
; Dividing the reply length by the stride looks equivalent and is not:
; 181 bytes is 1 + 5*36, and the division says 5 only by accident.
TBSTRID	EQU 36
TBID	EQU 0			; table id[9]
TBNAME	EQU 9			; name[21]
TBPLYRS	EQU 30			; players[6], the literal "cur / max" the
				;   server has already formatted -- just print it

; --- lengths --------------------------------------------------------------
PCMAX	EQU 12			; the serializer's seat + spectator cap
GMINLEN	EQU 95			; the fixed header
GMAXLEN	EQU 599			; GOPLYRS + PCMAX*PLSTRID
TMAXLEN	EQU 361			; 1 + 10*TBSTRID; the server caps the list at 10

; --- round values, and the three screen classes they map to ---------------
RNLOBBY	EQU 0
RNFINAL	EQU 13
RNOVER	EQU 99

CLLOBBY	EQU 0
CLPLAY	EQU 1
CLOVER	EQU 2

NAMEMAX	EQU 8

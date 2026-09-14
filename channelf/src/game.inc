; ---------------------------------------------------------------------------
; game.inc -- the poll loop, and everything that decides what a poll means.
;
; THERE IS NO STATE VARIABLE AND NO DISPATCH TABLE: the server's ROUND is the
; state. GCLASS maps it to one of three screen classes -- 0 lobby, 1-13 play,
; 99 over -- and a change of class is the only thing that clears the screen.
; Every other poll updates cells in place, because a full clear on this machine
; is most of a second of plotted pixels.
;
; A STAGED ACTION RIDES THE NEXT REQUEST in place of /state, and its reply IS
; the next state. There is never a second round trip, which is also why VPEND
; holding 0 must mean "nothing staged" and not RQTABLE: request 0 is the table
; list, and a cleared cell read as a request id refetches it forever.
;
; The six invariants below are carried verbatim from the Intellivision client
; and were re-learned by every port in between. They are not style:
;
;   1. THE TURN EDGE. activePlayer stays 0 for EVERY poll of your turn, so the
;      shadow advances at the bottom of every successful poll -- lobby included
;      -- and the your-turn cue fires BEFORE the input slice.
;   2. THE ROLL EDGE IS A DECREASE. A fresh turn resets rollsLeft upward, so
;      only a decrease means a roll landed. $FF in VPRVRLS mutes the animation
;      across a screen boundary.
;   3. THE READY TOGGLE LIVES IN THE INPUT PATH, sampled every pass and
;      edge-detected. A toggle read once per poll feels dead.
;   4. validScores[] IS ALL ZEROS IN THE LOBBY -- a nil slice, not a row of -1
;      -- so only play-class code may read it.
;   5. THE CLOCK BAILS AT ZERO. The server has already force-scored.
;   6. REJOINS LAND ANYWHERE. Your turn can arrive with the rolls already
;      spent, so the my-turn edge restages the wire's mask when dice exist and
;      drops straight into the score rows when rollsLeft is 0.
; ---------------------------------------------------------------------------

; The play-class interval lives in turn.inc, where the slice that implements
; it does: PLYWAIT when somebody else is up, TISLICE when we are.
LOBWAIT	EQU 110			; input passes between polls in the lobby
OVRWAIT	EQU 165			; ...once it is over
FAILWT	EQU 140			; ...and after a failed call

; queued cues, played after the draw
; Queued during the poll and played after the draw. Taking a row ourselves is
; NOT on this list: that cue fires on the spot in TITAKE, because we already
; know what we did and there is nothing to wait for the screen to show.
CUENONE	EQU 0
CUETURN	EQU 1
CUEBOT	EQU 2
CUEOVER	EQU 3
CUEJOIN	EQU 4

REVHOLD	EQU 80			; passes to hold a revealed row: about 1.5 s

; ---------------------------------------------------------------------------
; SGAME -- the state. Entered by JMP, so it may PI one level without ENTER:
;   nothing will ever POP back into it.
; ---------------------------------------------------------------------------
SGAME:	DCI VPRVCLS
	LI 0FFH
	ST			; force a full repaint on the first poll

GAMELP:	PI GFETCH
	OI 0
	BZ GLOK
	; A failed call keeps the screen it has: the old reply is still on
	; display and still true enough to look at. Only the pacing changes.
	DCI VTICK
	LI FAILWT
	ST
	JMP GWAIT

GLOK:	PI VALID8
	OI 0
	BNZ GLBAD
	PI GHDR			; lift the header out of the reply window
	PI GEDGE		; work out what changed, and queue any cue
	PI RENDER
	PI GPLAY		; and only now play it
	PI GSHADW		; advance every shadow, at the bottom of the poll
	JMP GINPUT

GLBAD:	DCI VTICK
	LI FAILWT
	ST
	JMP GWAIT

; ---------------------------------------------------------------------------
; GFETCH -- one round trip. The staged action replaces /state; there is no
;   second call to see what it did.
;   Reached by PI; calls three levels down.
; ---------------------------------------------------------------------------
GFETCH:	ENTER
	DCI VPEND
	LM
	OI 0
	BNZ GF1
	LI RQSTATE		; zero means NOTHING staged. It cannot mean
GF1:	DCI VREQ		;   RQTABLE, which is request 0.
	ST
	DCI VPEND
	CLR
	ST			; consumed either way

	; The settle floor is the length the LAST poll's seat count implies,
	; which is the best guess available before this reply exists. The
	; header alone is always safe.
	LI GMINLEN&0FFH
	DCI VMIN0
	ST
	LI GMINLEN>>8
	DCI VMIN1
	ST
	PI INLATCH
	LI GMAXLEN&0FFH
	LR 0,A
	LI GMAXLEN>>8
	LR 1,A
	PI NFETCH
	PI INLATCH
	LEAVE

; ---------------------------------------------------------------------------
; VALID8 -- believe nothing until the length agrees with the seat count.
;
;   The reply window is NEVER CLEARED, so a short read leaves the tail of a
;   previous, bigger reply exactly where the fixed offsets are about to look.
;   Four gates, in order: the header must be there, the seat count must be
;   inside the serializer's cap, the round must be on its enum, and the length
;   must be at least 95 + 42*playerCount.
;
;   A /state for an N-seat game reads back EXACTLY that -- one number that
;   confirms the wire format and this routine at the same time -- but the test
;   is a MINIMUM, because a longer reply harms nothing.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
VALID8:	ENTER
	DCI VRXHI
	LM
	OI 0
	BNZ VAL1		; over 255 bytes: certainly past the header
	DCI VRXLO
	LM
	CI GMINLEN-1
	BM VAL1			; A > 94, i.e. the header is all there
	BR VALBAD

VAL1:	DCI FNREPLY+GOPC
	LM
	OI 0
	BZ VALBAD		; A table with nobody at it is not a reply -- and
				;   this gate is LOAD-BEARING downstream. Every
				;   "past the last seat" test in this port negates
				;   playerCount and reads the borrow, and negating
				;   zero gives zero, which reads backwards. VPC is
				;   written only by GHDR, and GHDR runs only after
				;   this returns.
	CI PCMAX
	BM VALBAD		; A > 12: more seats than the serializer has
	LR 9,A

	DCI FNREPLY+GORND
	LM
	CI RNOVER
	BZ VAL2
	CI RNFINAL
	BM VALBAD		; A > 13 and not 99: not a round at all

	; ---- 95 + 42*playerCount, built up in VTMP/VTMP2 ------------------
VAL2:	LI GMINLEN
	DCI VTMP
	ST
	CLR
	DCI VTMP2
	ST
VAL3:	LR A,9
	OI 0
	BZ VAL4
	DCI VTMP
	LM
	AI PLSTRID
	DCI VTMP
	ST
	BNC VAL3A
	DCI VTMP2
	LM
	INC
	DCI VTMP2
	ST
VAL3A:	LR A,9
	AI 0FFH
	LR 9,A
	BR VAL3

	; ---- and compare it against what actually arrived -----------------
VAL4:	DCI VTMP
	LM
	COM
	INC			; -expected, low
	LR 4,A
	DCI VTMP2
	LM
	COM
	LNK			; -expected, high, with that carry
	LR 5,A
	DCI VRXLO
	LM
	AS 4
	DCI VRXHI
	LM
	LNK			; the carry out of the low half
	AS 5
	BNC VALBAD		; borrow: the reply is shorter than it claims
	CLR
	LEAVE
VALBAD:	LI 0FFH
	LEAVE

; ---------------------------------------------------------------------------
; GHDR -- lift this poll's header into RAM.
;
;   Copied rather than read in place because almost every consumer of it runs
;   inside a draw, and a DCI at the wrong moment costs a pointer somebody else
;   was using. The reply window itself is still the source for everything
;   variable-length -- names, the prompt, the score arrays.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
GHDR:	ENTER
	DCI FNREPLY+GOPC
	LM
	DCI VPC
	ST
	DCI FNREPLY+GORND
	LM
	DCI VRND
	ST
	DCI FNREPLY+GORLEFT
	LM
	DCI VRLEFT
	ST
	DCI FNREPLY+GOACTPL
	LM
	DCI VACT
	ST
	DCI FNREPLY+GOMVTIM
	LM
	DCI VMVT
	ST
	DCI FNREPLY+GOVIEW
	LM
	DCI VVIEW
	ST
	PI GCLASS

	; MYTRN is the one derived flag, and it is what every input path
	; gates on: activePlayer 0 means us, because the server rotates the
	; caller to index 0 -- but a spectator is never up whatever it says.
	DCI VMYTRN
	CLR
	ST
	DCI VCLASS
	LM
	CI CLPLAY
	BNZ GHX
	DCI VVIEW
	LM
	OI 0
	BNZ GHX
	DCI VACT
	LM
	OI 0
	BNZ GHX
	DCI VMYTRN
	LIS 1
	ST
GHX:	LEAVE

; ---------------------------------------------------------------------------
; GCLSEL -- clear to the screen this class wants. The LOBBY is a text screen
;   and gets palette 0 over the whole display, where the only white this
;   machine has lives; play and game over get the three bands, because the card
;   and the dice need the four inks palette 2 carries and the lobby does not.
;
;   Called ONLY on a class edge. A clear is most of a second of plotted pixels
;   -- doing it per poll would blank the screen every cycle, which is the trap
;   the Intellivision port names explicitly.
;   Reached by PI; calls two levels down.
; ---------------------------------------------------------------------------
GCLSEL:	ENTER
	DCI VCLASS
	LM
	CI CLLOBBY
	BNZ GCS1
	PI CLSTXT
	LEAVE
GCS1:	PI CLSGAM
	LEAVE

; GCLASS -- VCLASS from VRND. Leaf. Clobbers A, DC0.
GCLASS:	DCI VRND
	LM
	OI 0
	BNZ GCL1
	LI CLLOBBY
	BR GCLS
GCL1:	CI RNOVER
	BZ GCLO
	LI CLPLAY
	BR GCLS
GCLO:	LI CLOVER
GCLS:	DCI VCLASS
	ST
	POP

; ---------------------------------------------------------------------------
; GEDGE -- everything that is true only because of what the LAST poll said.
;   Runs before the draw, so the reveal and the animation are already decided
;   by the time anything is painted.
;   Reached by PI; calls three levels down.
; ---------------------------------------------------------------------------
GEDGE:	ENTER
	DCI VCUEQ
	CLR
	ST

	; ---- a class change wipes the screen and every shadow with it -----
	DCI VCLASS
	LM
	LR 0,A
	DCI VPRVCLS
	LM
	XS 0
	BZ GE1
	PI GCLSEL
	PI SCCLR
	DCI VVIEWPL
	CLR
	ST
	BR GE2

GE1:	; A seat count change invalidates the shadow too: it may belong to a
	; different player than it did last poll.
	DCI VPC
	LM
	LR 0,A
	DCI VPRVPC
	LM
	XS 0
	BZ GE2
	PI SCCLR
	DCI VPRVPC
	LM
	OI 0
	BZ GE2			; the first poll at a table is not an arrival
	LI CUEJOIN
	DCI VCUEQ
	ST

	; ---- the turn edge ------------------------------------------------
GE2:	DCI VACT
	LM
	LR 0,A
	DCI VPRVACT
	LM
	XS 0
	BZ GE5			; nobody's turn changed

	; Somebody just finished a turn. Which row did they take? The wire
	; does not say, so SCSCAN diffs their filled rows against the shadow.
	DCI VPRVACT
	LM
	CI 0FFH
	BZ GE3			; there was nobody up to have scored
	OI 0
	BZ GE3			; it was OUR turn: we know what we did
	DCI VPRVACT
	LM
	LR 0,A
	PI SCDIFF
	DCI VREVSL
	LM
	CI 0FFH
	BZ GE3
	; Show it: point the card at them and hold it long enough to read.
	DCI VPRVACT
	LM
	DCI VVIEWPL
	ST
	LI REVHOLD
	DCI VREVTK
	ST
	LI CUEBOT
	DCI VCUEQ
	ST
	BR GE4

	; No SCCLR here. The shadow is staged for whoever is up at the bottom of
	; every poll, so parking it full on a turn that revealed nothing would
	; mean no bit could ever turn on again and the feature would work
	; exactly once.
GE3:	; Auto-follow the roller, so the card is always the one in play.
	DCI VACT
	LM
	CI 0FFH
	BZ GE4
	DCI VACT
	LM
	DCI VVIEWPL
	ST

GE4:	; Our turn just began. Restage the keep mask -- and a REJOIN CAN LAND
	; ANYWHERE, so the wire's own mask is used whenever dice already
	; exist, and the cursor starts on the rows when the rolls are spent.
	DCI VMYTRN
	LM
	OI 0
	BZ GE5
	LI CUETURN
	DCI VCUEQ
	ST
	DCI VUIMOD
	CLR
	ST
	DCI VDCUR
	CLR
	ST
	DCI FNREPLY+GODICE
	LM
	OI 0
	BZ GE4A
	PI KEEPSYN		; mid-turn: adopt what the wire says we kept
	BR GE4B
GE4A:	PI KEEPALL
GE4B:	DCI VRLEFT
	LM
	OI 0
	BNZ GE5
	LIS 1			; no rolls left: the only move is a row
	DCI VUIMOD
	ST
	PI TIBEST

	; ---- the roll edge, which is a DECREASE ---------------------------
; ADDING THE ONE'S COMPLEMENT, not subtracting the two's. prev + ~now carries
; out exactly when prev > now, and it is right for every pair. The obvious
; version -- negate and add, then read the borrow -- is NOT: negating zero
; gives zero, the add then produces no carry, and the comparison comes out
; backwards for exactly the operand that matters here. rollsLeft reaching 0 IS
; the edge this is looking for, so that version missed every one of them and
; left the cursor on a dead ROLL tile until the server force-scored.
GE5:	DCI VPRVRLS
	LM
	CI 0FFH
	BZ GE6			; muted across a screen boundary
	LR 0,A			; prev
	DCI VRLEFT
	LM
	COM
	LR 1,A			; ~now
	LR A,0
	AS 1			; prev + ~now
	BNC GE6			; no carry: rollsLeft did not go DOWN
	PI ROLLANM

	; After the LAST roll the only legal move left is a row, so the cursor
	; crosses to the card by itself and parks on the highest-paying open
	; one. This belongs on the ROLL edge and not only on the turn edge:
	; activePlayer does not change when we spend our own third roll, so a
	; version that only crossed on the turn edge left the cursor sitting on
	; a dead ROLL tile for the rest of the turn, refusing every trigger
	; until the server force-scored.
	DCI VMYTRN
	LM
	OI 0
	BZ GE6
	DCI VRLEFT
	LM
	OI 0
	BNZ GE6
	LIS 1
	DCI VUIMOD
	ST
	PI TIBEST

	; ---- game over ----------------------------------------------------
GE6:	DCI VCLASS
	LM
	CI CLOVER
	BNZ GEX
	DCI VPRVCLS
	LM
	CI CLOVER
	BZ GEX
	LI CUEOVER
	DCI VCUEQ
	ST
GEX:	LEAVE

; ---------------------------------------------------------------------------
; GSHADW -- advance every shadow. At the BOTTOM of the poll, and on every
;   successful poll including the lobby's: activePlayer does not change during
;   your own turn, so a shadow advanced only when something happened would
;   never catch the edge that ends it.
;   Reached by PI; calls three levels down.
; ---------------------------------------------------------------------------
GSHADW:	ENTER
	; Stage the score shadow for whoever is up NOW, so the next turn edge
	; has this seat's card to diff against. A seat that does not exist --
	; the lobby's $FF, or a count that just shrank -- is skipped: reading a
	; record past the end of the reply would stage sixteen bits of whatever
	; the last bigger reply left there.
	DCI VACT
	LM
	CI 0FFH
	BZ GSH1
	LR 0,A
	DCI VPC
	LM
	COM
	INC
	AS 0
	BC GSH1			; no borrow: activePlayer is past the last seat
	DCI VACT
	LM
	LR 0,A
	PI SCSTAGE
GSH1:	DCI VCLASS
	LM
	DCI VPRVCLS
	ST
	DCI VPC
	LM
	DCI VPRVPC
	ST
	DCI VACT
	LM
	DCI VPRVACT
	ST
	DCI VRND
	LM
	DCI VPRVRND
	ST
	DCI VRLEFT
	LM
	DCI VPRVRLS
	ST
	LEAVE

; ---------------------------------------------------------------------------
; KEEPSYN -- adopt the wire's keep mask as our own staged one.
;   Used only on the my-turn edge when dice already exist, which is what a
;   rejoin looks like: the server held the seat and the turn comes back with
;   the rolls partly spent.
;   Leaf. Clobbers A, r0, DC0, DC1.
; ---------------------------------------------------------------------------
KEEPSYN: DCI FNREPLY+GOKEEP
	XDC
	DCI VKEEP
	XDC
	LIS 5
	LR 0,A
KSY1:	LM			; the source first
	XDC
	ST
	XDC
	DS 0
	BNZ KSY1
	XDC
	CLR
	ST
	XDC
	POP

; ---------------------------------------------------------------------------
; GPLAY -- play the queued cue, AFTER the draw. A cue is the best part of a
;   fifth of a second of blocking delay, and played where it is decided it
;   would announce a board that is not on screen yet.
;   Reached by PI; calls two levels down.
; ---------------------------------------------------------------------------
GPLAY:	ENTER
	DCI VCUEQ
	LM
	CI CUETURN
	BNZ GP1
	PI SNDTRN
	LEAVE
GP1:	CI CUEBOT
	BNZ GP2
	PI SNDEND		; somebody else scored: the falling mirror of
	LEAVE			;   our own rising cue
GP2:	CI CUEOVER
	BNZ GP3
	PI SNDEND
	LEAVE
GP3:	CI CUEJOIN
	BNZ GPX
	PI SNDKEY
GPX:	LEAVE

; ---------------------------------------------------------------------------
; RENDER -- paint whatever class we are in.
;   Reached by PI; calls four levels down.
; ---------------------------------------------------------------------------
RENDER:	ENTER
	DCI VCLASS
	LM
	CI CLLOBBY
	BNZ RND1
	PI GLOBBY
	LEAVE
RND1:	PI CDHEAD
	PI INLATCH
	PI CARDRW
	PI INLATCH
	PI DICERW
	PI TIROLT
	PI GPROMP
	LEAVE

; ---------------------------------------------------------------------------
; GLOBBY -- the waiting room: who is here, who is ready, and the countdown the
;   server puts in its prompt.
;   Reached by PI; calls three levels down.
; ---------------------------------------------------------------------------
GLOBBY:	ENTER
	LI CVAL1
	LR 3,A
	DCI SLOBBY
	XDC
	PI UITITLE
	PI UICLRL

	DCI VSEAT
	CLR
	ST
GLB2:	DCI VSEAT
	LM
	LR 0,A
	DCI VPC
	LM
	COM
	INC
	AS 0
	BC GLB3			; no borrow: past the last seat
	DCI VSEAT
	LM
	CI NROWS
	BZ GLB3
	PI GLBROW
	DCI VSEAT
	LM
	INC
	DCI VSEAT
	ST
	BR GLB2

GLB3:	PI GPROMP
	LEAVE

; GLBROW -- one lobby seat: the name, and READY if they have said so.
;   In the lobby the wire reuses scores[0] as the ready flag: 1 ready, 0 not.
;   Reached by PI; calls three levels down.
GLBROW:	ENTER
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

	CLR
	LR 0,A			; scores[0]
	DCI VSEAT
	LM
	LR 1,A
	PI RDSCOR
	LR A,0
	CI 1
	BNZ GLBRX
	DCI VSEAT
	LM
	LR 0,A
	LI CVAL1
	LR 3,A
	DCI SREADY
	XDC
	PI UIROWR
GLBRX:	LEAVE

; ---------------------------------------------------------------------------
; CDHEAD -- the header row: whose card this is, the round, and the clock.
;   Runs in the palette-0 band, so every ink here is the same white and the
;   text itself has to carry the meaning.
;   Reached by PI; calls two levels down.
; ---------------------------------------------------------------------------
CDHEAD:	ENTER
	LI UICOLS
	LR 0,A
	LI DORGX
	LR 1,A
	LI DORGY
	LR 2,A
	PI DCLRR

	DCI VVIEWPL
	LM
	LR 0,A
	PI CDPREC
	XDC
	DCI VLINE
	XDC
	LI NAMEMAX
	LR 0,A
	PI GUPCPY
	LI CVAL1
	LR 3,A
	LI DORGX
	LR 1,A
	LI DORGY
	LR 2,A
	DCI VLINE
	PI DSTR

	; "R4/13" or "R12/13", built as one string. Drawing the three pieces at
	; fixed cells instead left a gap in the middle of the field whenever the
	; round was one digit, because DEC5 suppresses leading zeros.
	DCI VRND
	LM
	LR 0,A
	CLR
	LR 1,A
	PI DEC5
	DCI VLINE
	XDC
	DCI SROUND
	PI SBCAT
	DCI VDIGIT
	PI SBCAT
	DCI SOF13
	PI SBCAT
	PI SBEND
	LI CVAL1
	LR 3,A
	LI DORGX+9*DCELLW
	LR 1,A
	LI DORGY
	LR 2,A
	DCI VLINE
	PI DSTR

	; seed the local clock from the wire, then let TICLK run it down
	DCI VMVT
	LM
	DCI VCLOCK
	ST
	PI CDCLK
	LEAVE

; ---------------------------------------------------------------------------
; CDCLK -- the three clock cells, repainted once a second by TICLK.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
CDCLK:	ENTER
	LIS 3
	LR 0,A
	LI DORGX+20*DCELLW
	LR 1,A
	LI DORGY
	LR 2,A
	PI DCLRR
	DCI VMYTRN
	LM
	OI 0
	BZ CDCKX		; only our own move is on a clock we can show
	DCI VCLOCK
	LM
	LR 0,A
	CLR
	LR 1,A
	PI DEC5
	LI CVAL1
	LR 3,A
	LI DORGX+20*DCELLW
	LR 1,A
	LI DORGY
	LR 2,A
	DCI VDIGIT
	PI DSTR
CDCKX:	LEAVE

; ---------------------------------------------------------------------------
; GPROMP -- the server's prompt, on the footer row.
;
;   The wire carries 41 characters and this row has 15 once the ROLL tile has
;   taken its half, so it is CLIPPED rather than wrapped: there is no second
;   row to wrap onto, and the prompts that matter ("your turn", "starting in
;   5") say what they mean in their first few words.
;   Reached by PI; calls two levels down.
; ---------------------------------------------------------------------------
GPROMP:	ENTER
	DCI VCLASS
	LM
	CI CLLOBBY
	BZ GPRL
	LI 15
	LR 9,A
	LI DORGX+8*DCELLW
	LR 10,A
	BR GPR1
GPRL:	LI UICOLS		; the lobby footer has the whole row
	LR 9,A
	LI DORGX
	LR 10,A
GPR1:	LR A,9
	LR 0,A
	LR A,10
	LR 1,A
	LI DORGY+8*DCELLH
	LR 2,A
	PI DCLRR

	DCI FNREPLY+GOPRMPT
	XDC
	DCI VLINE
	XDC
	LR A,9
	LR 0,A
	PI GUPCPY
	LI CVAL1
	LR 3,A
	LR A,10
	LR 1,A
	LI DORGY+8*DCELLH
	LR 2,A
	DCI VLINE
	PI DSTR
	LEAVE

; ---------------------------------------------------------------------------
; GINPUT -- hand the slice to whichever class we are in, then act on what it
;   decided. Entered by JMP from GAMELP.
; ---------------------------------------------------------------------------
GINPUT:	DCI VCLASS
	LM
	CI CLPLAY
	BZ GIPLAY
	JMP GIWAIT

GIPLAY:	PI TURNIN
	DCI VFLAG
	LM
	CI TF_ROLL
	BNZ GI1
	LI RQROLL
	DCI VPEND
	ST
	JMP GAMELP
GI1:	CI TF_SCORE
	BNZ GI2
	LI RQSCORE
	DCI VPEND
	ST
	JMP GAMELP
GI2:	CI TF_LEAVE
	BZ GILEAV
	JMP GAMELP		; TF_POLL and TF_CLOCK both just go and poll

; ---------------------------------------------------------------------------
; GILEAV -- leave the table. The reply is not parsed: we are no longer at the
;   table it describes.
; ---------------------------------------------------------------------------
GILEAV:	LI RQLEAVE
	DCI VREQ
	ST
	DCI VMIN0
	LIS 1
	ST
	CLR
	ST
	LI GMAXLEN&0FFH
	LR 0,A
	LI GMAXLEN>>8
	LR 1,A
	PI NFETCH
	PI AKCROOM		; the room is vacated; tell the lobby
	LI ST_TABLE
	LR 0,A
	JMP GOTO

; ---------------------------------------------------------------------------
; GIWAIT -- the lobby and game-over slice.
;
;   The READY TOGGLE IS SAMPLED HERE, every pass, and not once per poll: a
;   toggle a player has to hold down for two seconds to be noticed feels
;   broken. The bare /ready the server offers is itself a toggle, so one press
;   is one request.
; ---------------------------------------------------------------------------
GIWAIT:	DCI VCLASS
	LM
	CI CLLOBBY
	BNZ GIW0
	LI LOBWAIT
	BR GIW0A
GIW0:	LI OVRWAIT
GIW0A:	DCI VTICK
	ST

GIW1:	PI INDRAIN
	LR A,0
	DCI VEVT
	ST
	CI EV_NONE
	BZ GIWIDL

	DCI VEVT
	LM
	CI EV_SEL
	BNZ GIW2
	DCI VCLASS
	LM
	CI CLLOBBY
	BNZ GIW1
	LI RQREADY
	DCI VPEND
	ST
	PI SNDSEL
	JMP GAMELP
GIW2:	CI EV_BACK
	BNZ GIW3
	JMP GILEAV
GIW3:	CI EV_PGDN
	BNZ GIW4
	PI SHOWST
	PI RENDER
	BR GIW1
GIW4:	CI EV_PGUP
	BNZ GIW1
	JMP GAMELP		; TIME: poll now

GIWIDL:	LI IDLEMS
	LR 3,A
	PI DELAYMS
	PI GREVTK
	DCI VTICK
	LM
	AI 0FFH
	DCI VTICK
	ST
	CI 0
	BNZ GIW1
	JMP GAMELP

; ---------------------------------------------------------------------------
; GWAIT -- the pause after a failed call, with the old screen left standing.
; ---------------------------------------------------------------------------
GWAIT:	PI INSCAN
	LR A,0
	CI EV_BACK
	BZ GWLV
	LI IDLEMS
	LR 3,A
	PI DELAYMS
	DCI VTICK
	LM
	AI 0FFH
	DCI VTICK
	ST
	CI 0
	BNZ GWAIT
	JMP GAMELP
GWLV:	JMP GILEAV

; ---------------------------------------------------------------------------
; GREVTK -- retire a revealed row once it has been up long enough. Setting
;   VREVSL back to $FF is the whole of disarming it: SCELL simply stops
;   matching, so the reveal is a colour rather than an animation.
;   Reached by PI; calls four levels down.
; ---------------------------------------------------------------------------
GREVTK:	ENTER
	DCI VREVSL
	LM
	CI 0FFH
	BZ GRVX
	DCI VREVTK
	LM
	OI 0
	BZ GRVX
	AI 0FFH
	DCI VREVTK
	ST
	CI 0
	BNZ GRVX
	; Time is up. Disarm, and hand the view back to whoever is up now --
	; the reveal borrowed it to show somebody else's card, and leaving it
	; there would mean a player spent their own turn looking at a bot.
	DCI VREVSL
	LI 0FFH
	ST
	DCI VACT
	LM
	CI 0FFH
	BZ GRV1
	DCI VACT
	LM
	BR GRV2
GRV1:	CLR
GRV2:	DCI VVIEWPL
	ST
	PI CDHEAD
	PI CARDRW
GRVX:	LEAVE

; ---------------------------------------------------------------------------
; CDNEXT / CDPREV -- page the viewed scorecard while somebody else rolls.
;   Served entirely from the cached reply: every seat's record arrived in the
;   same poll, so this costs no round trip at all.
;   Leaf. Clobbers A, r0, DC0.
; ---------------------------------------------------------------------------
CDNEXT:	DCI VVIEWPL
	LM
	INC
	LR 0,A
	DCI VPC
	LM
	COM
	INC
	AS 0
	BNC CDNX1		; borrow: still inside the seat count
	CLR
	LR 0,A
CDNX1:	DCI VVIEWPL
	LR A,0
	ST
	POP

CDPREV:	DCI VVIEWPL
	LM
	OI 0
	BNZ CDPV1
	DCI VPC
	LM
CDPV1:	AI 0FFH
	DCI VVIEWPL
	ST
	POP

; ---------------------------------------------------------------------------
; TIBEST -- park the score cursor on the highest-paying open row.
;   Run on the my-turn edge once the rolls are spent, so that a player who
;   only ever presses the trigger still plays a legal, if artless, game.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
TIBEST:	ENTER
	DCI VTMP3
	CLR
	ST			; the best score seen so far
	DCI VTMP2
	LI 0FFH
	ST			; and which slot had it; $FF = nothing yet
	DCI VTMP
	CLR
	ST
TIB2:	DCI VTMP
	LM
	CI SL_UPTOT
	BZ TIB3
	CI SL_BONUS
	BZ TIB3
	DCI VTMP
	LM
	LR 0,A
	DCI FNREPLY+GOVALID
	LR A,0
	ADC
	LM
	CI 0FFH
	BZ TIB3			; a closed row
	LR 1,A

	; Nothing taken yet? Then take this, whatever it pays -- an open row
	; that scores nothing still beats leaving the cursor on a closed one,
	; where every trigger is refused.
	DCI VTMP2
	LM
	CI 0FFH
	BZ TIBTAK

	; candidate > best? Again by adding the one's complement, for the reason
	; spelt out at GE5: seeded with best = 0, the two's-complement form
	; carries only when the candidate is non-zero, so every comparison
	; against the seed came out backwards and this routine picked nothing at
	; all -- leaving the cursor on whatever row it started on, usually a
	; closed one, where every trigger is refused.
	DCI VTMP3
	LM
	COM
	LR 2,A			; ~best
	LR A,1
	AS 2			; candidate + ~best
	BNC TIB3		; no carry: candidate <= best, so no better

TIBTAK:	DCI VTMP3
	LR A,1
	ST
	DCI VTMP
	LM
	DCI VTMP2
	ST
TIB3:	DCI VTMP
	LM
	INC
	DCI VTMP
	ST
	CI SL_FUJI+1
	BNZ TIB2
	DCI VTMP2
	LM
	CI 0FFH
	BZ TIBX			; every row is closed: leave the cursor alone
	DCI VSCUR
	ST
TIBX:	LEAVE

; ---------------------------------------------------------------------------
; INLATCH / INDRAIN -- not missing a press made during the blind window.
;
; Input is only read inside an input slice. A poll and the repaint that follows
; it are about a second on a four-second cycle, and NOTHING IS LISTENING for
; that second: a press that begins and ends inside it is simply never seen.
; That is not theoretical -- it swallowed two deliberate button presses in
; testing before it was noticed, and from the player's seat a button that does
; nothing is a broken button.
;
; So the seams of the render and both ends of the fetch sample the controls and
; REMEMBER one event, and the slice drains that before it scans. A press in the
; gap then arrives late rather than not at all. The first press wins: a latch
; that kept overwriting itself would hand the slice whatever the player did
; last, which is not what they asked for first.
;
; INLATCH is only ever called between whole routines, never inside one, because
; INSCAN writes r0-r3 and DC0.
;   Both reached by PI; both call one level down.
; ---------------------------------------------------------------------------
INLATCH: ENTER
	DCI VSTICK
	LM
	OI 0
	BNZ INLX		; already holding one
	PI INSCAN
	LR A,0
	CI EV_NONE
	BZ INLX
	LR A,0
	DCI VSTICK
	ST
INLX:	LEAVE

; INDRAIN -- r0 = the latched event if there is one, else a fresh scan.
INDRAIN: ENTER
	DCI VSTICK
	LM
	OI 0
	BZ INDR1
	LR 0,A
	DCI VSTICK
	CLR
	ST
	LEAVE
INDR1:	PI INSCAN
	LEAVE

; ---------------------------------------------------------------------------
; GUPCPY -- copy up to r0 characters from DC0 to DC1, folding to upper case
;   and terminating.
;
;   The font covers full ASCII, but every string on this wire arrives
;   LOWERCASED and the screen is 23 cells wide: capitals are simply more
;   legible at three pixels. The font's own lower case is the capitals
;   squashed into four rows, which exists for the name editor's echo and for
;   nothing else.
;   Leaf. Clobbers A, r0, r1, r2, DC0, DC1.
; ---------------------------------------------------------------------------
GUPCPY:	LR A,0
	LR 1,A
GUP1:	LM			; the source first: the XDC dance has a direction
	CI 0
	BZ GUPZ
	LR 2,A
	CI 060H			; 'a'-1, so BP means A is not a lower-case letter
	BP GUP2
	LR A,2
	CI 07AH			; 'z', so BM means A is past one
	BM GUP2
	LR A,2
	AI 0E0H			; -32
	LR 2,A
GUP2:	LR A,2
	XDC
	ST
	XDC
	DS 1
	BNZ GUP1
GUPZ:	XDC
	CLR
	ST
	XDC
	POP

; ---------------------------------------------------------------------------
; UIROWR -- the right-hand column of a list row: the string at DC1, drawn on
;   list row r0 in colour r3, starting at cell 15.
;   The closing JMP into DSTR is a tail call -- JMP leaves PC1 alone, so DSTR's
;   POP returns to OUR caller.
;   Clobbers A, r0, r1, r2, r4-r8, DC0, DC1.
; ---------------------------------------------------------------------------
UIROWR:	LR A,0
	INC
	LR 6,A
	LI DCELLH
	LR 5,A
	CLR
	LR 2,A
UIRR1:	LR A,2
	AS 5
	LR 2,A
	DS 6
	BNZ UIRR1
	LR A,2
	AI DORGY
	LR 2,A
	LI DORGX+15*DCELLW
	LR 1,A
	XDC
	JMP DSTR

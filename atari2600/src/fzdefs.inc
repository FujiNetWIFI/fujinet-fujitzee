; fzdefs.inc -- banks, entry codes, the wire format, the RAM ledger, colours.
;
; EQUATES ONLY, like fujinet.inc: included before the ORG.

; ---------------- banks ----------------
; Seven 2K banks and the fixed half: 16384 bytes, one of the four sizes MAME's
; cart slot accepts. A bank switch replaces every byte of $1000-$17FF, so each
; bank carries its own copy of every module it calls; only zero page, the text
; planes, the reply window and the cartridge path buffers cross.
BANKLOB EQU     0               ; the cold start and the table list
BANKGAM EQU     1               ; the kernel, the dice, the cursors, the cues
BANKNET EQU     2               ; one request, with the picture up, and back
BANKMNU EQU     3               ; the SELECT menu: resume, help, leave
BANKNAM EQU     4               ; the keyboard and the shared username
BANKCRD EQU     5               ; compose the name row and the 15 card rows
BANKCHR EQU     6               ; compose the chrome: status, prompt, panels
BANKEDG EQU     7               ; what the reply CHANGED: the edges, the
                                ;   reveal, the dice, the cues
NBANKS  EQU     15              ; the image is (N+1)*2048 and MAME's cart slot
                                ;   takes only 4K, 8K, 16K and 32K, so the
                                ;   step above seven banks is fifteen. Eight
                                ;   are real and the rest are stubs back to
                                ;   bank 0: a bank is 2048 bytes whatever the
                                ;   image is, and the composer would not fit
                                ;   in one however much room was left over.

; The bank trampoline. Not a label: fzcore.inc ORGs to this, so the address
; here and the code there cannot drift. It must be the FIRST thing in the
; tail, because it is the one address a bank has to know before
; build/tail.inc -- which is generated from the tail's own listing -- exists.
FZGOTO  EQU     FNTAIL

; ---------------- entry codes ----------------
; What the bank being entered should do. FZENT is read once on entry and the
; bank dispatches on it; a bank reached by the trampoline never returns.
ENCOLD  EQU     0               ; bank 0: power-on
ENLOBBY EQU     1               ; bank 0: the table list is in the window
ENGCOLD EQU     2               ; bank 1: a table was just joined
ENGAME  EQU     3               ; bank 5/6: back from a poll: recompose
ENFETCH EQU     4               ; bank 2: /state, or the staged request
ENTABLE EQU     5               ; bank 2: /tables, then the lobby
ENMENU  EQU     6               ; bank 3: the SELECT menu
ENLEAVE EQU     7               ; bank 2: /leave, /tables, then the lobby
ENNAME  EQU     8               ; bank 4: the keyboard
ENGRUN  EQU     9               ; bank 1: composed: finish the frame, run
ENGNEXT EQU     10              ; bank 1: composed, between frames: run
ENRESUME EQU    11              ; bank 5: the menu closed, mid-frame
ENCHROME EQU    12              ; bank 6: the chrome pass, mid-frame
ENEDGE  EQU     16              ; bank 7: what the reply changed
ENGRUND EQU     13              ; bank 1: as ENGRUN, after a compose: DINIT

; ---------------- requests ----------------
RQTABLE EQU     0               ; /tables
RQSTATE EQU     1               ; /state
RQREADY EQU     2               ; /ready -- TOGGLES
RQROLL  EQU     3               ; /roll/MMMMM
RQSCORE EQU     4               ; /score/N
RQLEAVE EQU     5               ; /leave

; ---------------- the cartridge's path buffers ----------------
; Where the strings console RAM cannot hold live. They SURVIVE A CONSOLE
; RESET, so the cold start empties all four.
PBTABLE EQU     FP_SEL0         ; the joined table's id
PBNAME  EQU     FP_SEL1         ; the player name
PBROOM  EQU     FP_SEL2         ; "<endpoint>?table=<id>" for lobby appkey 3
PBEDIT  EQU     FP_SEL3         ; what the keyboard is typing into

; ---------------- the wire ( ?bin=1 ) ----------------
; /state, /ready, /roll/.., /score/.. and /leave all return the SAME state
; blob; a staged action rides the next request in place of /state and its
; reply IS the next state. /tables returns Tables. All strings are lowercased
; and NUL-padded, one byte longer than their content width.
;
; Header, always present (95 bytes):
GOPC    EQU     0               ; playerCount (spectators appended)
GOSRVNM EQU     1               ; serverName[21]
GOPRMPT EQU     22              ; prompt[41]
GORND   EQU     63              ; round: 0 lobby, 1-13 play, 99 gameover
GORLEFT EQU     64              ; rollsLeft this turn (3 down to 0)
GOACTPL EQU     65              ; signed; $FF = -1 nobody, 0 = your turn
                                ;   (the server rotates you to index 0)
GOMVTIM EQU     66              ; seconds left, 0-250
GOVIEW  EQU     67              ; 1 = YOU are a spectator: input disabled
GODICE  EQU     68              ; 5 ascii '1'-'6' + NUL; NULs at turn start
GOKEEP  EQU     74              ; 5 ascii '0'/'1' + NUL; '1' = RE-ROLL
GOVALID EQU     80              ; validScores[15], signed bytes: $FF = -1
                                ;   closed row, else the points that row
                                ;   would earn. ALL ZERO IN THE LOBBY --
                                ;   only rounds 1-13 may read it, because
                                ;   zero is itself a legal potential.
GOPLYRS EQU     95              ; playerCount x 42-byte records
PLSTRID EQU     42
PLNAME  EQU     0               ; name[9]
PLALIAS EQU     9               ; index of the highlight char in name
PLSCORS EQU     10              ; scores[16], u16 LE each: $FFFF = -1 unset,
                                ;   $FFFE = -2 spectator. A slot is FILLED iff
                                ;   its HIGH byte is not $FF -- those two are
                                ;   the only non-scores, and both carry $FF
                                ;   there, so the test is one byte.
                                ;   scores[0] doubles as the lobby ready flag.

; Score slots. 0-5 ones..sixes and 8-14 are the 13 SELECTABLE categories;
; 6, 7 and 15 are server-computed, never selectable, and must be excluded
; from the bot-reveal diff -- slot 6 fills the moment ANY upper row is taken,
; so being the lowest it would win every comparison.
SCONES  EQU     0
SCSIXES EQU     5
SCUPTOT EQU     6               ; upper total
SCBONUS EQU     7               ; upper bonus (35 at 63+)
SCSET3  EQU     8
SCSET4  EQU     9
SCHOUSE EQU     10
SCSRUN  EQU     11
SCLRUN  EQU     12
SCCHANC EQU     13
SCFUJI  EQU     14              ; five of a kind
SCGRAND EQU     15              ; grand total -- filled only at gameover
NSLOTS  EQU     16

; Round values.
RNLOBBY EQU     0
RNFINAL EQU     13
RNOVER  EQU     99

; /tables -- byte-identical to the card games' Tables struct.
TBSTRID EQU     36
TBID    EQU     0               ; table[9]
TBNAME  EQU     9               ; name[21]
TBPLYRS EQU     30              ; players[6], literal "cur / max"

GMINLEN EQU     95              ; the fixed header
GMAXLEN EQU     599             ; 95 + 12*42, the NREAD clamp
TMAXLEN EQU     361
PCMAX   EQU     12              ; the serializer's seat + spectator cap

; THE SEAT RENDER CAP, and why it is 9. The reply window is ONE 512-byte
; slice (FN_R_SLICE_LEN); the firmware sized it so the flagship client, whose
; worst case is 418, never pages. Fujitzee's 599 would cross. Record 8 ends at
; GOPLYRS + 9*PLSTRID = 473, so capping what we RENDER at nine seats keeps
; every offset we read below 473 -- inside slice 0, which the cartridge
; republishes after every transaction -- and FR_RXSL is never written. A
; "+N MORE" line covers bigger tables, and the server rotates you to index 0
; so your own card is always readable. VALID8 still checks the full length.
PCREND  EQU     9
NAMEMAX EQU     8               ; the wire field is nine with the NUL

; ---------------- zero page ----------------
; $80-$CF, and the stack must stay above $D0. $80-$8C is the transport's, as
; is $A8/$A9 (fujinet.inc); the rest is ours.
;
; THE STACK MOVES. Both 2600 siblings put variables in $80-$BF and gave the
; stack 64 bytes for a measured depth of about sixteen. This client also
; carries five dice pointer PAIRS and a 16-bit score shadow, which will not
; fit in 51 bytes. 48 bytes of stack is still twenty-four levels, and
; emu/stack.lua watermarks it across a played game.
PAD3    EQU     $81             ; the display kernels' 3-cycle pad target
SAVSP   EQU     $82             ; the stack pointer, parked across a text row
FZIDX   EQU     $84             ; a render loop's index. Its own cell: FZTMP
                                ;   is not safe across a call
FZTMP   EQU     $85             ; scratch; any routine you call may clobber it
INRPT   EQU     $8D             ; INREPT's auto-repeat countdown
FZDGT   EQU     $8E             ; FZDEC's digit and leading-zero latch
FZREQ   EQU     $8F             ; the request in flight (RQ*)
FZTRIES EQU     $90             ; the settle loop's bound
FZDLY   EQU     $91             ; its inter-poll delay, in memory because
                                ;   NFRAME clobbers X and Y on both its paths
FZCIX   EQU     $90             ; a composer loop's seat index, and
FZCMOD   EQU    $91             ;   which panel it is composing.
                                ;   THE SETTLE LOOP'S TWO CELLS: a compose
                                ;   never runs inside a transaction, and these
                                ;   two have to survive RUNTOT, which uses
                                ;   FZIDX and FZTMP as its own accumulator
FZINP   EQU     $92             ; THIS FRAME'S input mask
FZAVL0  EQU     $92             ; NET_STATUS bytes waiting -- only ever live
FZAVL1  EQU     $93             ;   inside a transaction, when no input runs
FZPRV0  EQU     $94             ; the settle loop's previous reading
FZPRV1  EQU     $95
FZNUM0  EQU     $94             ; FZDEC's 16-bit number: never live with the
FZNUM1  EQU     $95             ;   settle loop, which composes nothing
FZRXL0  EQU     $96             ; the READ's reply length, captured at once
FZRXL1  EQU     $97
FZMIN0  EQU     $98             ; the length this request will settle for
FZMIN1  EQU     $99
FZERR   EQU     $9A             ; the last failure
FZSTEP  EQU     $9B             ; where it happened: 1 open 2 status 3 read
                                ;   4 validate
FZROW   EQU     $9C             ; kernel: the row being drawn
FZERR2  EQU     $9C             ; APICALL's result, parked across a switch --
                                ;   the bank that issues requests has no kernel
FZLINE  EQU     $9D             ; kernel: ink lines left, or dice band lines
FZHL    EQU     $9E             ; kernel: the highlighted row, $FF for none.
FZHLC   EQU     $9F             ; kernel: that row's ink colour.
                                ;   THESE TWO SERVE BOTH the score cursor and
                                ;   the bot reveal, and the alias is safe
                                ;   because the cursor exists only on your
                                ;   turn and the reveal only on someone
                                ;   else's. It is what makes "highlight the
                                ;   row a bot took" cost two bytes and no
                                ;   kernel time at all.
FZGRM0  EQU     $A0             ; the potential mask, MASTER: bit 15 = row 1
FZGRM1  EQU     $A1             ;   ... bit 0 = row 16
FZGRW0  EQU     $A2             ; the kernel's WORKING copy, destroyed by the
FZGRW1  EQU     $A3             ;   seam shifts, refreshed from the master in
                                ;   the top pad every frame
FZFRAME EQU     $A4             ; a free-running frame counter
FZENT   EQU     $A5             ; EN*: what the bank being entered should do
FZPOLL  EQU     $A6             ; frames until the next request
FZREVHD EQU     $A6             ; frames the bot reveal is held. DELIBERATE:
                                ;   the reveal's hold IS the poll being held
                                ;   off, so there is one counter, not two
FZCLS   EQU     $A7             ; the screen class (CL*)
FZSEL   EQU     $AA             ; list cursor / menu item / keyboard cell
FZDIFF  EQU     $AA             ; the latched difficulty level -- GAME BANK
                                ;   ONLY; FZSEL belongs to the lobby, menu and
                                ;   name banks, which are different 2K images
FZCNT   EQU     $AB             ; rows on the table list  -- LOBBY BANK ONLY
SNDCNT  EQU     $AB             ; frames left on a cue    -- GAME BANK ONLY
FZNLEN  EQU     $AB             ; characters typed        -- NAME BANK ONLY
SNDPTR  EQU     $AC             ; the sound script step, 0 = silent
FZPCNT  EQU     $AD             ; playerCount from the last reply
FZRND   EQU     $AE             ; round: 0 lobby, 1-13, 99 over
FZACT   EQU     $AF             ; activePlayer, $FF = nobody
FZMYTRN EQU     $B0             ; ...and whether it is ours
FZVIEW  EQU     $B1             ; whose card is on screen
FZUIMD  EQU     $B2             ; 0 = the dice cursor, 1 = the score cursor
FZDCUR  EQU     $B3             ; 0-4 the dice, 5 = the ROLL tile
FZSCUR  EQU     $B4             ; the score slot under the cursor
FZKEEP  EQU     $B5             ; the STAGED re-roll mask, bit i = die i
                                ;   re-rolls. SAME SENSE AS THE WIRE, where
                                ;   '1' means re-roll; expanded to five ASCII
                                ;   bytes only on the way into the URL, so no
                                ;   six-byte buffer exists anywhere
FZRLEFT EQU     $B6             ; rollsLeft from the last reply
FZCLK   EQU     $B7             ; the local move clock, in seconds
FZTICK  EQU     $B8             ; frames until it ticks
FZPRVAC EQU     $B9             ; previous activePlayer -- the TURN edge
FZPRVRL EQU     $BA             ; previous rollsLeft -- the ROLL edge, which
                                ;   is a DECREASE; $FF mutes it
FZPRVPC EQU     $BB             ; previous playerCount -- the full-clear watch
FZPRVCL EQU     $BC             ; previous class
FZSCM0  EQU     $BD             ; the filled-slot shadow of whoever is up,
FZSCM1  EQU     $BE             ;   slots 0-7 then 8-15. One BIT per slot: a
                                ;   full shadow of sixteen 16-bit scores is 32
                                ;   bytes and this machine has not got them.
FZREVSL EQU     $BF             ; the slot a bot just took, $FF = none
FZANM   EQU     $C0             ; roll-animation frames left, 0 = the dice on
                                ;   screen are the truth
FZRSD0  EQU     $C1             ; the LFSR the animation shakes them with
FZRSD1  EQU     $C2
FZSCIX  EQU     $C3             ; the staged category for /score/N
FZREQ2  EQU     $C4             ; the request staged for the next fetch
FZREDRW EQU     $C5             ; the composer's pass flags

; The five dice pointers, for the kernel's `lda (DPTRn),y`. The LOW byte is
; the die's whole state -- held*DIEVAR + face*DIEH -- and the high byte is
; written once by DINIT and never again, which is why the art must live at
; the same address in every bank that draws it. See FZART below.
DPTR0   EQU     $C6
DPTR1   EQU     $C8
DPTR2   EQU     $CA
DPTR3   EQU     $CC
DPTR4   EQU     $CE
; Two cells the kernel needs that would not fit below, taken off the top of
; the stack rather than aliased onto something live during the picture.
FZROWE  EQU     $D0             ; the row the kernel stops at. The dice class
                                ;   draws in two segments -- rows 0-16 and
                                ;   rows RHOLD-20 -- so the stop row is data
FZMCUR  EQU     $D1             ; 2 to show the dice cursor bar, 0 to hide it
FZILAT  EQU     $D2             ; THE INPUT LATCH: newly-pressed bits, OR'd in
                                ;   once a frame by DFRAME -- in EVERY bank --
                                ;   and drained by whichever hook cares.
                                ;   Nothing else keeps input alive while a
                                ;   poll or a compose is mapped, and those run
                                ;   for up to twenty-four frames at a stretch.
STKLOW  EQU     $D3             ; the stack must never reach below this

; ---------------- the dice art ----------------
; 6 faces x DIEH lines x two variants (ordinary, held) = 144 bytes, PAGE
; ALIGNED so the highest low byte is 155 and `lda (DPTRn),y` with Y = 0..11
; can never cross a page: every load is 5 cycles, and the kernel's schedule
; is built on that.
;
; FZART IS AN ABSOLUTE ADDRESS AND BOTH BANKS THAT DRAW DICE MUST PUT THE ART
; THERE. DPTRn+1 is set once by DINIT, and the network bank is entered at
; DFRAME2 and never runs DINIT -- so if its copy of the art sat anywhere else
; the dice would render that bank's own code. tools/checkart.py compares the
; 144 bytes between banks and fails the build if they differ.
; THE TOP 256 BYTES OF EVERY DRAWING BANK ARE RESERVED, and the code is
; required at assembly time to stop before them. The first cut put the art at
; $1600 with an ORG after the includes, which quietly assembled it ON TOP OF
; the code that had already run past $1600 -- the listing said the bank fitted,
; because it did.
FZART   EQU     $1750           ; 144 bytes: $1750-$17DF, no page crossed
FZBGTA  EQU     $17E0           ; the per-row background table, 22 bytes
DIEH    EQU     12              ; scanlines in a die
DIEVAR  EQU     72              ; 6 faces x DIEH: the held variant's offset
NDICE   EQU     5

; ---------------- colours (NTSC) ----------------
; Blue page, and the inks that read against it. A closed row is painted in
; CCLOSED, which IS the background -- so it is invisible rather than dim, the
; Intellivision's COL_DASH trick. The corollary is a rule: NOTHING THAT MUST
; BE READ MAY BE DRAWN IN CCLOSED.
CBLACK  EQU     $00
CWHITE  EQU     $0E
CBG     EQU     $82             ; the page: blue
CCLOSED EQU     CBG             ; a closed row: absent, not dim
CGREEN  EQU     $C8             ; an open row with a potential
CCURSOR EQU     $1C             ; the cursor row, and the row a bot just took
CDIE    EQU     $0E             ; an ordinary die
CDIEHLD EQU     $1C             ; ...the hold markers under a kept one
CSTATUS EQU     $1C             ; the status row
CNAME   EQU     $0E             ; the viewed player's name
CTITLE  EQU     $1C
PFEDGE  EQU     $F0             ; the playfield side border. Load-bearing:
                                ;   the playfield draws OVER the background,
                                ;   which is what gives a seam line until
                                ;   cycle ~32 to write COLUBK instead of
                                ;   22.67. It is also why the dice cursor is
                                ;   MISSILE 0 and not the ball -- the ball
                                ;   would take COLUPF and tint this.

; ---------------- the screen ----------------
; 12 columns x 21 rows. One category per row, because the kernel writes
; COLUP0 and COLUP1 from the SAME value and colour granularity is therefore
; one colour per ROW -- which is exactly what buys a green potential.
RSTAT   EQU     0               ; round, whose turn, the clock
RNAME   EQU     1               ; the viewed player's name + turn marker
RCARD0  EQU     2               ; the 15 card rows: 2-16
NCARDR  EQU     15
RUPPER  EQU     8               ; the two computed upper rows: "UP nnn  B nn"
RTOTAL  EQU     16              ; the grand total
RDICE   EQU     17              ; the dice band starts here (art, not planes)
RHOLD   EQU     19              ; hold markers under each die
RROLL   EQU     20              ; the ROLL tile and rolls left
; At game over there is no dice band and the prompt wraps over rows 17-20.
RPRMPT0 EQU     17
NPRMPTR EQU     4               ; 4 x 12 = 48 >= the 41-byte prompt

; A row is an eight-column label, a gap, and a three-column right-aligned
; value. Three is wider than the thirteen selectable categories need -- they
; cap at 50 -- but the totals reach three digits, and a uniform field deletes
; the class of bug where a value outgrows its column. The standings panel uses
; the same two widths, which is why they live here and not in card.inc.
CLABW   EQU     8
CVALW   EQU     3

; ---------------- screen classes ----------------
CLLOBBY EQU     0
CLPLAY  EQU     1
CLOVER  EQU     2
CLSTAND EQU     3               ; the left difficulty switch is UP

; ---------------- timing, in frames ----------------
POLLFRM EQU     90              ; between /state polls in play, ~1.5s
LOBFRM  EQU     120             ; in the lobby, ~2s
OVERFRM EQU     180             ; at game over, ~3s: the server resets
FAILFRM EQU     150             ; after a failed poll
TURNFRM EQU     4               ; after a staged action: its reply IS the
                                ;   next state, so there is no second trip
REVFRM  EQU     90              ; how long a bot's chosen row is held up
ANMFRM  EQU     24              ; the roll animation: 8 flickers of 3 frames
ANMSTEP EQU     3

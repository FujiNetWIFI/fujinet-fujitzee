; disp.inc -- the 48-pixel text display, the dice band, and the frame.
;
; The cartridge composes the glyphs (firmware/src/vcs_render.c) and publishes
; them as six 128-byte planes at $1800-$1AFF; this is the console half.
;
; THE 53-CYCLE INK BODY BELOW IS TRANSCRIBED FROM
; fujinet-firmware/pico/atari-2600/testrom/fujidisp.inc AND MUST NOT BE
; RETIMED. Its four constants are the coordinates of a ONE-PIXEL-WIDE window
; found by byte-comparing the raster against the renderer -- every wrong
; setting still looks like text.
;
; WHAT THIS PORT ADDS to battleship's copy is two things.
;
; 1. THE SEAM LINE PICKS THE ROW'S INK OUT OF A POTENTIAL MASK. vcs_render_row
;    writes zero to scanline 5 of every cell, so no glyph can be drawn there
;    and the line is free of DEADLINES (not of cycles). Battleship spent it on
;    a background and a bar; here it also shifts a 16-bit mask whose carry says
;    "the row about to be drawn is an open category with something to score",
;    and paints that row green. Colour granularity on this machine is ONE
;    COLOUR PER ROW -- COLUP0 and COLUP1 are written from the same value and
;    there is no time to differ -- which is the whole reason the scorecard is
;    one category per row.
;
; 2. A DICE BAND. Five dice drawn from the client's own ROM art, in the middle
;    of a screen whose other rows come from the cartridge. It needs no
;    repositioning: the text kernel's six 8-clock objects become five when
;    NUSIZ1 goes from three copies to two, and the five land at clocks 52-91 --
;    exactly text columns 0-9, so the hold markers on row RHOLD line up under
;    them for free.
;
; THE VISIBLE BAND IS TIMED, NOT COUNTED, because the dice class is two
; scanlines longer than the others and a counted band would make it a 264-line
; frame. The timer makes a gap line an editorial decision instead of a bug.

; ---------------- kernel timing constants ----------------
; Not taste. See the file header.
RNOP    EQU     16              ; NOPs before RESP0 (2 cycles each)...
R3      EQU     1               ; ...plus this, so RESP0 lands on cycle 38
HM0     EQU     15              ; HMP0 = $F0: move P0 right one pixel...
HM1     EQU     0               ; ...and leave P1, closing the gap to eight
VDEL    EQU     1               ; vertical delay on both players

; ---------------- text plane bases ----------------
TP0     EQU     FNTEXT+$000
TP1     EQU     FNTEXT+$080
TP2     EQU     FNTEXT+$100
TP3     EQU     FNTEXT+$180
TP4     EQU     FNTEXT+$200
TP5     EQU     FNTEXT+$280

TINK    EQU     FNTCLH-1        ; 5 ink lines, then the seam
TLINES  EQU     FNTROW*FNTCLH   ; 126 scanlines of text

; NTSC: 3 vsync + 37 vblank + 192 visible + 30 overscan = 262.
VBLINES EQU     37
OSLINES EQU     30
PADLINE EQU     (192-TLINES)/2  ; 33 above; below is whatever the timer leaves

; THE BLANKED BANDS ARE TIMED, NOT COUNTED, and that is not a refinement --
; counting them is what made the picture jump. The two counts are in 64-cycle
; units and are one less than the arithmetic suggests, because the timer has
; to expire INSIDE the last line rather than on its boundary.
VBTIM   EQU     43              ; ~37 scanlines of vblank
OSTIM   EQU     35              ; ~30 scanlines of overscan
; ...and so is the VISIBLE band, which battleship's copy counted. 192 lines is
; 14592 cycles, 228 ticks of the 64-cycle timer; 227 expires inside the last
; line and the WSYNC after it closes the band at exactly 192. Without this the
; play screen -- whose dice band is two lines longer than the two text rows it
; replaces -- would be a 264-line frame.
VITIM   EQU     227

; ---------------- DINIT: TIA setup and player positioning ----------------
DINIT:
; Two players, three copies each, eight pixels apart: P0 draws groups 0,2,4
; and P1 groups 1,3,5, interleaving to 48 contiguous pixels. VDELP is what
; makes the six-write sequence land in the right copies.
        lda     #$03            ; NUSIZ: three copies, close
        sta     NUSIZ0
        sta     NUSIZ1
        lda     #VDEL
        sta     VDELP0
        sta     VDELP1
        lda     #CWHITE
        sta     COLUP0
        sta     COLUP1
        lda     #CBG
        sta     COLUBK
        sta     COLUPF
        lda     #$01            ; CTRLPF: reflect, so one nibble draws both
        sta     CTRLPF          ;   edges
        lda     #PFEDGE
        sta     PF0
        sta     PF1
        lda     #0
        sta     PF2
        sta     ENAM0
        sta     ENAM1
        sta     GRP0
        sta     GRP1
        sta     GRP0
        lda     #$FF
        sta     FZHL            ; no highlighted row until something asks

; Player positioning, transcribed from batari Basic's std_kernel.asm rather
; than re-derived. RESP0 strobed at cycle 43, RESP1 three cycles later, then
; HMP0/HMP1 pull them to eight apart.
        sta     WSYNC
        REPT    RNOP
        nop
        ENDM
        IF      R3
        sta     PAD3            ; 3 cycles, so odd delays are reachable
        ENDIF
        sta     RESP0
        sta     RESP1
        lda     #HM0*16
        sta     HMP0
        lda     #(HM1*16)&$FF
        sta     HMP1
; An EARLY HMOVE -- WSYNC, then strobe -- rather than batari's late one at
; cycle 73. A late HMOVE does not apply the motion nibbles cleanly.
        sta     WSYNC
        sta     HMOVE
; AND THEN CLEAR THE MOTION REGISTERS. The dice cursor strobes a SECOND HMOVE
; later in the frame to place missile 0, and HMOVE moves every object that has
; a nibble set -- so leaving HMP0 at $F0 would slide the whole text block one
; pixel right per frame. It drifts off the screen in about three seconds.
;
; THE WSYNC BETWEEN THE TWO IS NOT PADDING. HMOVE does not move anything at
; the moment it is strobed: it drives the objects from the HM registers
; through the following hblank, so clearing them three cycles later stops the
; motion HALF DONE. That is not a subtle failure -- P0 kept 8 of its 9-clock
; gap from P1 instead of closing it to 8, which put every P1 object one clock
; right, merged dice 1 with 2 and 3 with 4, and stretched the text block by a
; pixel.
        sta     WSYNC
        sta     HMCLR
        rts

; ---------------- DLOOP: draw frames, forever ----------------
; DFRESH is the entry for a FRESH start -- a screen change, not a frame being
; finished or followed -- and arms the overscan timer DFRAME waits on, because
; a stale count could be seconds.
DFRESH: lda     #OSTIM
        sta     TIM64T
DLOOP:  jsr     DFRAME
        jmp     DLOOP

; ---------------- DFRAME: draw ONE frame, then return ----------------
; Split out of DLOOP so a bank that is not running the display loop can still
; keep the picture alive: net.inc waits for the cartridge a frame at a time
; through here, which is what stops a poll from blanking the screen.
;
; THE OVERSCAN IS WAITED OUT HERE, at the start of the next frame, and not at
; the end of the one before. DFRAME2 arms the timer and returns at once, so
; whatever a bank does between two frames -- build a URL into the TX page,
; recompose the card, switch banks -- is spent INSIDE the overscan and moves
; nothing. Waiting at the end instead put every one of those between the
; overscan and the next VSYNC, and a played game measured 96 frames in 2,400
; that were 273 to 310 lines.
DFRAME: bit     TIMINT
        bpl     DFRAME
        sta     WSYNC
        lda     #2
        sta     VSYNC
        sta     WSYNC
        sta     WSYNC
        sta     WSYNC
        lda     #0
        sta     VSYNC

        lda     #2
        sta     VBLANK
        lda     #VBTIM
        sta     TIM64T
; The client's per-frame hook, called while the screen is still blanked and
; the raster is nowhere near the text. Input scanning belongs here; so does
; STARTING a transaction, which is a bank switch out of this loop -- the bank
; that talks keeps the picture alive by calling DFRAME, one frame per wait.
        inc     FZFRAME
; THE INPUT SCAN IS HERE, NOT IN THE HOOK, and that is the whole point of it:
; DFRAME runs in every bank that draws -- including the network bank, which
; calls it once per wait while a transaction is in flight, and the composer
; banks, which call it while they fill the card in. Scanning in the hooks
; instead left input dead for up to twenty-four frames after every poll.
;
; Newly-pressed bits are OR'd into a latch rather than handed straight over,
; so a press made while a bank that does not act on input was mapped is still
; there when one that does comes back. Whichever hook cares drains it.
        jsr     INREPT
        ora     FZILAT
        sta     FZILAT
        jsr     APPVBL

; ---------------- DFRAME2: finish the frame the hook was called in ----------
; A bank entered from another bank's hook carries on from here, so the frame
; the switch interrupted is still 262 lines.
DFRAME2:
        IF      FZHASDIC
; Place the dice cursor while the screen is still blanked. HMOVE draws a black
; comb down the left of whatever line it is strobed on, so it must not happen
; in the visible band.
        jsr     DMPOS
        ENDIF
; Wait out the rest of the band, then land the unblank on a LINE BOUNDARY.
; TIMINT, not INTIM: past zero the timer free-runs at a tick a cycle and a
; seven-cycle `lda INTIM / bne` walks over the single cycle it reads zero on.
; TIMINT's bit 7 latches on the underflow, so an overrun costs exactly the
; overrun.
DVBLP:  bit     TIMINT
        bpl     DVBLP
        sta     WSYNC
        lda     #0
        sta     VBLANK
        lda     #VITIM          ; the visible band, timed; see VITIM
        sta     TIM64T

; The top band is felt, and its LAST line hands over to row 0.
        ldx     #PADLINE-1
DPADA:  sta     WSYNC
        dex
        bne     DPADA
        sta     WSYNC

; The kernel's working copy of the potential mask. It is DESTROYED by the seam
; line's shifts, so it is refreshed from the master here, every frame. Forget
; this and the greens appear on the first frame only.
        lda     FZGRM0
        sta     FZGRW0
        lda     FZGRM1
        sta     FZGRW1

        lda     #CBG            ; row 0's background
        sta     COLUBK
        lda     #CSTATUS        ; ...and its ink
        sta     COLUP0
        sta     COLUP1

        lda     #0
        sta     FZROW
        tay                     ; Y is the ABSOLUTE SCANLINE, 0-125

        IF      FZHASDIC
        lda     FZCLS
        cmp     #CLPLAY
        bne     DNODICE         ; every other class is 21 plain text rows
        lda     #RDICE
        sta     FZROWE
        jsr     DRUN            ; rows 0-16
        jsr     DDICEB
; Rows RHOLD-20. Y is set from the ROW, not carried through the band: the band
; reads its own art and cannot advance the plane index for us.
        lda     #RHOLD
        sta     FZROW
        ldy     #RHOLD*FNTCLH
        lda     #FNTROW
        sta     FZROWE
        jsr     DRUN
        jmp     DBOT
        ENDIF

DNODICE:
        lda     #FNTROW
        sta     FZROWE
        jsr     DRUN

DBOT:
; The bottom pad is whatever the visible band's timer has left. That is what
; lets the dice class run two scanlines longer than the others without the
; frame growing.
DPADB:  sta     WSYNC
        bit     TIMINT
        bpl     DPADB

        lda     #2
        sta     VBLANK
        lda     #OSTIM          ; the next DFRAME waits this out; see above
        sta     TIM64T
; Returns with VBLANK SET and the overscan timer running: a bank may do up to
; about 2,200 cycles of work before the next DFRAME for free.
        rts

; ---------------- DRUN: text rows FZROW .. FZROWE-1 ----------------
; A SUBROUTINE, which battleship's copy could not be: the stack pointer is a
; spare register inside the ink body, so nothing between the `txs` and the
; restore may touch the stack. Here both live INSIDE DRUN, and the JSR's
; return address is pushed before the first of them -- so calling it twice, as
; the dice class does, is safe.
;
; On entry FZROW is the first row to draw, Y is its first scanline, and its
; COLOUR IS ALREADY SET: a seam line programmes the row AFTER the one it ends.
DRUN:   tsx
        stx     SAVSP

DROWL:  lda     #TINK
        sta     FZLINE

DINK:   sta     WSYNC
        lda     TP0,y           ; 4
        sta     GRP0            ; 3   ->  7   new GRP0 = group 0
        lda     TP1,y           ; 4   -> 11
        sta     GRP1            ; 3   -> 14   displays group 0
        lda     TP2,y           ; 4   -> 18
        sta     GRP0            ; 3   -> 21   displays group 1
        lda     TP4,y           ; 4   -> 25
        tax                     ; 2   -> 27
        txs                     ; 2   -> 29   park group 4 in S
        lda     TP3,y           ; 4   -> 33
        tax                     ; 2   -> 35
        lda     TP5,y           ; 4   -> 39
        stx     GRP1            ; 3   -> 42   displays group 2
        tsx                     ; 2   -> 44   recover group 4
        stx     GRP0            ; 3   -> 47   displays group 3
        sta     GRP1            ; 3   -> 50   displays group 4
        sty     GRP0            ; 3   -> 53   displays group 5; value unused
        iny                     ; 2   -> 55
        dec     FZLINE          ; 5   -> 60
        bne     DINK            ; 3   -> 63

; ---- the seam line ----
; Line 5 of the cell. The cartridge stores zero here, so no glyph can be drawn
; and the kernel does something useful instead.
;
; IT HAS TO FIT IN ONE LINE. This is a WSYNC-bounded loop, so a seam that runs
; past cycle 76 does not glitch -- it silently costs a SECOND scanline,
; because the next `sta WSYNC` is then already on the following line. Every
; row would become seven scanlines, the text 147 lines, the picture would
; roll, and nothing about it would look wrong on screen. `make frames` is the
; only thing that sees it.
;
; The worst path -- a row that is BOTH a potential and the highlighted row,
; which the cursor sitting on an open category makes an ordinary case --
; reaches the next WSYNC at cycle 72 of 76. FOUR CYCLES SPARE. So: FZBGT is
; page-aligned in every bank (a straddling table costs a fifth cycle on the
; indexed load) and the two branches below must not cross a page either.
        sta     WSYNC
        lda     #0              ; 2
        sta     GRP0            ; 3   ->  5   three writes, not two: with
        sta     GRP1            ; 3   ->  8     VDELP set a write to GRP0
        sta     GRP0            ; 3   -> 11     reloads the DELAYED GRP1
        ldx     FZROW           ; 3   -> 14
        inx                     ; 2   -> 16   the row these colours are FOR
        lda     FZBGT,x         ; 4   -> 20
        sta     COLUBK          ; 3   -> 23   by cycle 32 at the latest, while
                                ;   the playfield border is still drawing over
                                ;   the background
        stx     FZROW           ; 3   -> 26   X still holds it, and `inc`
                                ;   costs five cycles this line has not got
        iny                     ; 2   -> 28   step past the blank plane byte
; The potential mask, one bit consumed per seam. The shifts run from the TOP:
; `asl` lifts FZGRW1's bit 7 into carry and `rol` lifts FZGRW0's bit 7 out
; through it, so the carry left here is the OLD bit 15 -- and the seam that
; programmes row R is the Rth, so row R's bit is bit 16-R. The card occupies
; rows RCARD0..RTOTAL, so GRNBLD sets bits 16-2 down to 16-16 and leaves the
; rest zero; every other row falls out white. Off by one here and the whole
; card is green one row up.
        asl     FZGRW1          ; 5   -> 33
        rol     FZGRW0          ; 5   -> 38
        lda     #CWHITE         ; 2   -> 40
        bcc     DSM1            ; 2/3 -> 42
        lda     #CGREEN         ; 2   -> 44
; The highlighted row: the score cursor on your own turn, or the row a bot
; just took during a reveal. They are the same two cells because they are
; never both live -- the cursor exists only on your turn and the reveal only
; on somebody else's -- which is what makes "hold up the row a bot chose"
; cost two bytes of RAM and no kernel time at all.
DSM1:   cpx     FZHL            ; 3   -> 47
        bne     DSM2            ; 2/3 -> 49
        lda     FZHLC           ; 3   -> 52
DSM2:   sta     COLUP0          ; 3   -> 55
        sta     COLUP1          ; 3   -> 58
        cpx     FZROWE          ; 3   -> 61
        bne     DROWL           ; 3   -> 64   + DROWL's 5 and the WSYNC's 3

        ldx     SAVSP
        txs
        rts

        IF      FZHASDIC
; ---------------- DDICEB: the five dice ----------------
; Entered mid-line, after DRUN's last seam. The scanline it costs to get in
; sync again is the gap between the card and the tray, and it is wanted.
;
; NUSIZ1 GOES TO TWO COPIES HERE AND BACK TO THREE ON THE WAY OUT. Leave it at
; three and P1 draws a sixth object at clock 92 showing whatever GRP1 last
; held -- die 3 reappears at the right-hand end of the tray, and nothing about
; it looks like a bug.
DDICEB: sta     WSYNC
        lda     #0
        sta     GRP0
        sta     GRP1
        sta     GRP0
        lda     #$01            ; two copies close: five objects, not six
        sta     NUSIZ1
        lda     #CDIE
        sta     COLUP0
        sta     COLUP1
        lda     #CBG
        sta     COLUBK
        ldy     #0

; Five loads, SIX GRP writes, and THE WRITES ARE SPREAD, not bunched.
;
; With VDELP set, writing one player's register buffers that value AND
; transfers the OTHER player's buffer into its displayed register. So write
; k+2 is what puts object k on the screen, and it has to land before the beam
; reaches it -- not merely before the end of the line.
;
; A pixel P is drawn at cycle (P+68)/3, so the three P0 copies at pixels 52,
; 68 and 84 are drawn at cycles 40.0, 45.3 and 50.7 and the two P1 copies at
; 60 and 76 at 42.7 and 48.0.
;
; EVERY WRITE HAS A WINDOW, NOT A DEADLINE, and that is the whole difficulty.
; A write to GRP1 replaces what P0 is DISPLAYING, so it must land after the
; copy it would overwrite has already been drawn as well as before the one it
; is for. Writing die 2 into P0 at cycle 39 is one cycle too EARLY: P0 copy 0
; is drawn at 40, and it comes out as die 2.
;
; Two cuts were lost to that. The first ran all six writes by cycle 43 and the
; tray came out as dice 2, 3, 4, 3, 4 -- five dice, in the right places, all
; the right shapes, three of them the wrong ones. The second moved the fourth
; write to 39 and produced BYTE-FOR-BYTE THE SAME PICTURE, which is what
; finally made the point: the pointers were never wrong, and reading them back
; out of memory is what proved it.
;
; The windows, and where the writes land:
;
;   w2  GRP1  P0 <- die 0     <= 40.0                 at 16
;   w3  GRP0  P1 <- die 1     <= 42.7                 at 24
;   w4  GRP1  P0 <- die 2     in (40.0, 45.3]         at 42
;   w5  GRP0  P1 <- die 3     in (42.7, 48.0]         at 45
;   w6  GRP1  P0 <- die 4     in (45.3, 50.7]         at 48
;
; So the last three writes are back to back at the end, exactly as the text
; body's are, and die 3 and die 4 are preloaded into X and A to make that
; possible. Only two need preloading here, not three, so the text body's
; stack-parking is not needed -- and the three cycles that buys are spent as
; the pad that puts w4 on the far side of cycle 40.
DDICE:  sta     WSYNC
        lda     (DPTR0),y       ; 5   ->  5
        sta     GRP0            ; 3   ->  8   w1: buffer die 0
        lda     (DPTR1),y       ; 5   -> 13
        sta     GRP1            ; 3   -> 16   w2: P0 <- die 0, drawn at 40.0
        lda     (DPTR2),y       ; 5   -> 21
        sta     GRP0            ; 3   -> 24   w3: P1 <- die 1, drawn at 42.7
        lda     (DPTR3),y       ; 5   -> 29
        tax                     ; 2   -> 31
        lda     (DPTR4),y       ; 5   -> 36
        sta     PAD3            ; 3   -> 39   past cycle 40; A survives a store
        stx     GRP1            ; 3   -> 42   w4: P0 <- die 2, drawn at 45.3
        sta     GRP0            ; 3   -> 45   w5: P1 <- die 3, drawn at 48.0
        sty     GRP1            ; 3   -> 48   w6: P0 <- die 4, drawn at 50.7
        iny                     ; 2   -> 50
        cpy     #DIEH           ; 2   -> 52
        bne     DDICE           ; 3   -> 55

; The cursor bar: two lines with the sprites blanked, so COLUP0 -- which
; missile 0 borrows -- is free for a colour the dice do not use. This is why
; the cursor is a missile and NOT the ball: the ball takes COLUPF, and COLUPF
; is the playfield border, which is what gives every seam line until cycle 32
; to write COLUBK instead of 22.67.
        sta     WSYNC
        lda     #0
        sta     GRP0
        sta     GRP1
        sta     GRP0
        lda     #$30            ; one copy, an 8-clock missile
        sta     NUSIZ0
        lda     #CCURSOR
        sta     COLUP0
        lda     FZMCUR          ; 2 to show the bar, 0 to hide it
        sta     ENAM0
        sta     WSYNC
        sta     WSYNC
        lda     #0
        sta     ENAM0
        lda     #$03            ; three copies again, for the text rows
        sta     NUSIZ0
        sta     NUSIZ1
        lda     #CWHITE
        sta     COLUP0
        sta     COLUP1
        rts

; ---------------- DMPOS: place missile 0 under the cursor die ----------------
; Called in the vblank, because HMOVE draws a black comb down the left of the
; line it is strobed on.
;
; The five dice are 8 colour clocks apart and a strobe loop steps in 15, so
; the position is coarse-then-fine: DMCRS picks the loop count, DMFINE the
; HMOVE nibble. A missile strobed by RESM0 completing at cycle N lands at
; clock 3N-64 (a player lands one clock later), so the pad below puts the
; Y=0 case at clock 50 and each loop iteration adds 15: 50, 65, 65, 80, 80
; against targets 52, 60, 68, 76, 84, which the fine nibbles close.
; WHETHER THE BAR SHOWS IS DECIDED HERE TOO, and that is the point of putting
; it here. It used to be computed in the game bank's hook and left in FZMCUR
; for whichever bank drew the frame to find -- and the picture is drawn by the
; network bank and by all three composer banks as well, so the flag and the
; POSITION were being taken from the cursor at two different moments. The bar
; came out under die 0 when the cursor was on die 4, and most frames it did
; not come out at all.
;
; One routine, one read of FZDCUR, one answer.
; THE MISSILE IS POSITIONED ON EVERY FRAME WHETHER IT IS SHOWN OR NOT, and
; that is deliberate. An earlier cut bailed out before the strobe when the bar
; was hidden, to save the two WSYNCs -- and the bar then never appeared at
; all, because the object it was enabling had never been given a position on
; any frame it was going to be seen on. Deciding and placing are two things,
; and only the decision is conditional.
DMPOS:  lda     #0
        sta     FZMCUR
        lda     FZCLS
        cmp     #CLPLAY
        bne     DMP0
        lda     FZMYTRN         ; not your turn: nothing to point at
        beq     DMP0
        lda     FZUIMD
        bne     DMP0            ; on the card, not the tray
        ldx     FZDCUR
        cpx     #NDICE
        bcs     DMP0            ; on the ROLL tile: the '>' says so instead
        lda     #2              ; ENAM0
        sta     FZMCUR
DMP0:   ldx     FZDCUR
        cpx     #NDICE
        bcc     DMP2
        ldx     #0              ; parked under die 0 while it is hidden
DMP2:   lda     DMFINE,x
        sta     HMM0
        ldy     DMCRS,x
        sta     WSYNC
        REPT    14
        nop
        ENDM
        sta     PAD3            ; 14 nops and a 3-cycle store = 31 cycles
DMP1:   dey                     ; 2
        bpl     DMP1            ; 3 taken, 2 not: 5 cycles = 15 clocks a step
        sta     RESM0
        sta     WSYNC
        sta     HMOVE
; CLEAR THE MOTION REGISTERS AGAIN -- a WSYNC later, for the reason DINIT
; gives. Leaving HMM0 armed would walk the missile across the tray a frame.
        sta     WSYNC
        sta     HMCLR
        rts

DMCRS:  DB      0,1,1,2,2       ; strobe-loop iterations
DMFINE: DB      $E0,$50,$D0,$40,$C0  ; +2, -5, +3, -4, +4 clocks
        ENDIF

; ---------------------------------------------------------------------------
; FZBGT is NOT here. Each bank supplies its own TWENTY-TWO byte table -- one
; per row plus one for the row past the last, because the seam at the bottom
; of row 20 programmes a row 21 that does not exist and making that entry the
; page colour is what turns the handover to the bottom band into a write the
; kernel was going to make anyway. It must be PAGE-ALIGNED: see the seam.

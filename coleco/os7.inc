; os7.inc -- the ColecoVision BIOS, as equates.
;
; Everything platform-level in this client goes through OS7: the VDP, the
; hand controllers, the timer manager and the random generator. That is a
; deliberate choice and not just convenience -- OS7 is in every console, it
; costs no ROM, and its VRAM routines already pace their writes.
;
; Entry addresses and register conventions are transcribed from
; ~/Workspace/os7lib (src/os7.h and the wrapper per routine), which is the
; documented binding for these calls.
;
; Two of them come with warnings, both learned elsewhere and both load-bearing
; here:
;
;   MODE_1 resets VDP register 1, and register 1 bit 5 is the vblank interrupt
;   enable. Anything that re-runs it after NMIINIT silently kills the interrupt
;   POLLER rides on -- the client stays alive and goes deaf. DINIT is the only
;   caller, and it writes R1 last.
;
;   PUT_VRAM truncates its count to 8 bits and returns as though it worked.
;   Nothing warns. This client does not use it; WRITE_VRAM takes a real 16-bit
;   count in BC and is what every write here goes through.

; ---- VDP -------------------------------------------------------------
MODE_1  EQU     1F85H           ; lay out the standard tables (see above)
WRITREG EQU     1FD9H           ; B = register, C = value
READREG EQU     1FDCH           ; -> A = VDP status (reading it clears vblank)
WRITVRM EQU     1FDFH           ; HL = source, DE = VRAM dest, BC = count
READVRM EQU     1FE2H           ; HL = VRAM source, DE = dest, BC = count
FILLVRM EQU     1F82H           ; HL = VRAM address, DE = count, A = value

; ---- controllers -----------------------------------------------------
POLLER  EQU     1FEBH           ; scan + debounce into the controller area
DECODER EQU     1F79H           ; H = controller, L = segment
                                ; -> H buttons, L control, E spinner

; ---- timers ----------------------------------------------------------
INITTMR EQU     1FC7H           ; HL = timer table, DE = timer data block
REQSIG  EQU     1FCDH           ; A = table entry -> A = signal number
TESTSIG EQU     1FD0H           ; A = signal number -> Z when it has elapsed
FREESIG EQU     1FCAH           ; A = signal number
TIMEMGR EQU     1FD3H           ; call once per NMI

; ---- misc ------------------------------------------------------------
RANDGEN EQU     1FFDH           ; -> RANDNUM

; ---- OS7's own RAM cells (fixed addresses; do not reuse) --------------
VDPSTAT EQU     73C5H           ; where OS7 keeps the last VDP status read
DEFERWR EQU     73C6H
MUXSPRT EQU     73C7H
RANDNUM EQU     73C8H           ; RAND_GEN's output, 16 bits

; ---- TMS9918 inks ----------------------------------------------------
CTRANSP EQU     0
CBLACK  EQU     1
CMGREEN EQU     2
CLGREEN EQU     3
CDBLUE  EQU     4
CLBLUE  EQU     5
CDRED   EQU     6
CCYAN   EQU     7
CMRED   EQU     8
CLRED   EQU     9
CDYELLW EQU     10
CLYELLW EQU     11
CDGREEN EQU     12
CMAGENT EQU     13
CGRAY   EQU     14
CWHITE  EQU     15

; ---- VDP table bases (what MODE_1 sets, and the standard Mode 2 map) --
VPATTRN EQU     0000H           ; pattern generator, 3 x 2048
VNAMETB EQU     1800H           ; name table, 768
VSPRATT EQU     1B00H           ; sprite attributes (unused here)
VCOLOUR EQU     2000H           ; colour table, 3 x 2048
VSPRGEN EQU     3800H           ; sprite generator (unused here)

; ---- sound -----------------------------------------------------------
PSGPORT EQU     0FFH            ; SN76489

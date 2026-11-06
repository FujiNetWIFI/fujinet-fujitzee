; state.inc -- the wire format and the device numbers, as equates.
;
; The server's ?bin=1 layout (little-endian): fixed-length strings padded with
; NULs and one byte longer than their text, u16 little-endian. Nothing is
; parsed into RAM: the screens index the reply window.

; ---- /state and the actions' replies (roll, score, ready) ----
HPCNT   EQU     0               ; playerCount
HSNAME  EQU     1               ; serverName[21]
HPROMPT EQU     22              ; prompt[41]
HROUND  EQU     63              ; 0 lobby, 1-13, 99 game over
HROLLS  EQU     64              ; rolls left
HACT    EQU     65              ; the player to act, 0 = us, $FF nobody
HCLOCK  EQU     66              ; seconds that player has left
HSPEC   EQU     67              ; 1: we are only watching
HDICE   EQU     68              ; 5 ASCII digits and a NUL
HKEEP   EQU     74              ; the last re-roll, '1' = re-rolled
HVALID  EQU     80              ; 15 signed scores, $FF = not offered
HDRLEN  EQU     95
GPLYRS  EQU     95              ; players, PSTRIDE apart; we are index 0
PSTRIDE EQU     42
PNAME   EQU     0               ; name[9], lower case
PALIAS  EQU     9
PSCORES EQU     10              ; 16 x u16: $FFFF unset, $FFFE watching
GMAXPL  EQU     12
GMINLEN EQU     HDRLEN+PSTRIDE  ; one seat at least
GMAXLEN EQU     HDRLEN+GMAXPL*PSTRIDE

; the card
SL_UPTOT EQU    6
SL_BONUS EQU    7
SL_TOTAL EQU    15
NSLOTS   EQU    15              ; slots a score can go in are 0-14

; ---- /tables: a count byte, then Tables ----
TSTRIDE EQU     36
TID     EQU     0               ; id[9]
TNAME   EQU     9               ; name[21]
TPLAY   EQU     30              ; players[6], literally "cur / max"
TMAXN   EQU     28
TCAP    EQU     1+TMAXN*TSTRIDE

; ---- the requests url.inc builds ----
REQ_TABLES EQU  0
REQ_STATE  EQU  1
REQ_READY  EQU  2
REQ_ROLL   EQU  3
REQ_SCORE  EQU  4
REQ_LEAVE  EQU  5

; ---- N1: (lib/device/NDevice) ----
NDEV    EQU     71H
NCOPEN  EQU     4FH             ; 'O'
NCCLOSE EQU     43H             ; 'C'
NCREAD  EQU     52H             ; 'R'
NCSTAT  EQU     53H             ; 'S'
NMHTTPG EQU     0CH             ; open mode: HTTP GET
NTRNONE EQU     0               ; no EOL translation
NSDEVST EQU     3               ; STATUS byte 3, nDevStatus_t: 1 is SUCCESS
NETTMO  EQU     100             ; seconds: past the cart's own 90 s for N:

; ---- appkeys on the FUJI device: the username every client in the family
; shares (creator 1, app 1, key 0). OPEN takes a 6-byte struct appkey as
; payload; READ answers a u16 length and then the value (rs232Fuji). ----
FNCAKOP EQU     0DCH
FNCAKRD EQU     0DDH
FNCAKWR EQU     0DEH
AKREAD  EQU     0
AKWRITE EQU     1
NAMEMAX EQU     8               ; the wire's name[9] holds 8

; ---- failure codes the screens show ----
FE_OPEN EQU     0E1H
FE_STAT EQU     0E2H
FE_READ EQU     0E3H
FE_LEN  EQU     0E4H

; ---- app glyphs ($80-$9F, dice.inc): die face f (0-5) is G_DIE+2f, +2f+1 ----
G_DIE   EQU     80H

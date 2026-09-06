; lobby.inc -- table selection (13-line doublescan mode).
;
; Up to 8 rows straight out of the /tables reply window: name in cols
; 1-10, the server's literal "cur / max" field after it -- the '/'
; renders through the built-in diagonal ($01), no UDC spent. The
; selected row is drawn in green; normal colour mode has no second
; background to invert against, and the one sprite this game has is the
; scorecard chevron, which means nothing here.
;
; This is table SELECTION. The game's own lobby -- the ready-up phase --
; is STATUS_LOBBY inside the poll loop (see game.inc), because that is
; where the server drives it.

LOBBY:  BSTA,UN KEYRAW          ; no leaking presses
        STRA,R0 *PVKEY
        BSTA,UN DINI13
        BSTA,UN DCLSU
        BSTA,UN SPRINI
        DAT1    0,1
        DCOL1   ANORM
        SETST1  TCHOOS
        BSTA,UN DPRINT
        DAT1    11,1
        SETST1  TJOINK
        BSTA,UN DPRINT
        DAT1    12,1
        SETST1  TBACKK
        BSTA,UN DPRINT
TBLRF:  LODI,R0 RQTABLE
        BSTA,UN APICALL
        BCTR,EQ TBLOK
        DAT1    9,1
        DCOL1   AHOT
        SETST1  TENET
        BSTA,UN DPRINT
        BSTA,UN SNDERR
TBLRW:  BSTA,UN KEYSCN
        COMI,R0 KNONE
        BCTR,EQ TBLRW           ; any key: try again
        BCTR,UN TBLRF
TBLOK:  EORZ    R0              ; the count byte
        STRA,R0 *PRXSLC
        STRA,R0 *PRXOFF
        BSTA,UN RXNEXT
        BCTR,EQ TBLRW           ; none yet: wait for a key, refetch
        COMI,R0 8
        BCTR,LT TBOK2
        LODI,R0 8               ; eight rows fit
TBOK2:  STRA,R0 *PVCNT
        EORZ    R0
        STRA,R0 *PVSEL
        BSTA,UN TBDRAW
TBKEY:  BSTA,UN GETKEY
        COMI,R0 KUP
        BCFR,EQ TBK1
        LODA,R0 *PVSEL
        BCTR,EQ TBKEY
        SUBI,R0 1
        STRA,R0 *PVSEL
        BCTR,UN TBMOVE
TBK1:   COMI,R0 KDOWN
        BCFR,EQ TBK2
        LODA,R0 *PVSEL
        ADDI,R0 1
        COMA,R0 *PVCNT
        BCFA,LT TBKEY
        STRA,R0 *PVSEL
TBMOVE: BSTA,UN SNDKEY 
        BSTA,UN TBDRAW
        BCTA,UN TBKEY
TBK2:   COMI,R0 KCLR            ; Clear: back to the name screen
        BCFR,EQ TBK3
        BSTA,UN NAMESCR
        BCTA,UN LOBBY
TBK3:   COMI,R0 KENT            ; Enter or FIRE joins
        BCTA,EQ TBJOIN
        COMI,R0 2
        BCTA,EQ TBJOIN
        COMI,R0 0               ; 0 refetches
        BCTA,EQ TBLRF
        COMI,R0 1               ; digits 1-8: row shortcut
        BCTA,LT TBKEY
        COMI,R0 8
        BCTA,GT TBKEY
        SUBI,R0 1
        COMA,R0 *PVCNT
        BCFA,LT TBKEY           ; row >= count: reject
        STRA,R0 *PVSEL
; falls into the join
TBJOIN: BSTA,UN SNDSEL
        LODA,R0 *PVSEL           ; copy the id, up to its NUL
        LODI,R1 TBID
        BSTA,UN TBOFF
        EORZ    R0
        STRA,R0 *PVTMP2
TBJC:   BSTA,UN RXNEXT
        BCTR,EQ TBJC9
        LODA,R1 *PVTMP2
        STRA,R0 *PTBLB,R1
        LODA,R0 *PVTMP2
        ADDI,R0 1
        STRA,R0 *PVTMP2
        COMI,R0 9
        BCTR,LT TBJC
TBJC9:  LODA,R1 *PVTMP2
        EORZ    R0
        STRA,R0 *PTBLB,R1
        BSTA,UN GJOINI          ; the edge sentinels and the cursors --
        BCTA,UN GAMEIN          ;   block-1 cells, so block-1 code sets

; TBOFF: state cursor = table record R0 + field R1; record base =
; 1 + rec*36, and row 7's name/players fields cross into slice 1 -- the
; TPSL after the ADDZ reads that carry out.
; Leaf. Clobbers R0,R1,R3.
TBOFF:  RRL,R0
        RRL,R0                  ; *4
        STRZ    R3
        RRL,R0
        RRL,R0
        RRL,R0                  ; *32
        ADDZ    R3              ; *36
        ADDI,R0 1               ; + the count byte
        ADDZ    R1              ; + the field (may carry into slice 1)
        STRA,R0 *PRXOFF
        LODI,R0 0
        TPSL    $01
        BCFR,EQ TBOF1
        LODI,R0 1
TBOF1:  STRA,R0 *PRXSLC
        RETC,UN

; TBDRAW: the list rows: name then "cur / max", the selected row
; inverse. Calls four down (RXPRNT -> RXNEXT -> MSLICE).
; Clobbers R0-R3, DCOLOR, V_ROW, V_TMP2.
TBDRAW: EORZ    R0
        STRA,R0 *PVROW           ; row index (V_TMP is APICALL's)
TBD1:   LODA,R0 *PVROW
        COMA,R0 *PVCNT
        BCFA,LT TBD9
        LODA,R0 *PVROW
        LODI,R1 TBNAME
        BSTA,UN TBOFF
        LODA,R0 *PVROW
        ADDI,R0 2               ; rows 2-9
        STRZ    R1
        LODI,R2 1
        BSTA,UN DPOS
        LODA,R0 *PVROW
        COMA,R0 *PVSEL
        BCFR,EQ TBD2
        LODI,R0 APOT            ; the selected row, in green
        BCTR,UN TBD3
TBD2:   LODI,R0 ANORM 
TBD3:   STRA,R0 *PDCOL
        LODI,R1 10
        BSTA,UN RXPRNT
        LODA,R0 *PVROW           ; the players field, right after
        LODI,R1 TBPLYRS
        BSTA,UN TBOFF
        LODI,R1 5
        BSTA,UN RXPRNT
        LODA,R0 *PVROW
        ADDI,R0 1
        STRA,R0 *PVROW
        BCTA,UN TBD1
TBD9:   RETC,UN


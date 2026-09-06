; state.inc -- reading the reply where it lies.
;
; The reply stays painted in the cart's 256-byte slice window until the
; next transaction, so nothing is buffered: code sets the flat cursor
; (RXPTR = slice, RXPTR+1 = offset) and then calls RXNEXT per byte.
; RXNEXT switches slices only when the cursor's slice differs from the
; cached CURSLC; every transaction ends with the cart republishing slice
; 0 and net.inc re-homing the cache.
;
; Fujitzee needs the 16-bit form: a player record lives at
; GOPLYRS + PLSTRID*i, and the twelfth seat's last byte is offset 598 --
; three slices in. RXSEEK sets the cursor from a flat 16-bit offset and
; lets RXNEXT stream from there, so a record's sixteen score words are a
; purely sequential read, which is what RXNEXT is fast at.

; RXSEEK: set the reply cursor to the flat offset R1 (hi) : R2 (lo).
; Leaf. Clobbers R0. Preserves R1,R2.
RXSEEK: LODZ    R1
        STRA,R0 *PRXSLC
        LODZ    R2
        STRA,R0 *PRXOFF
        RETC,UN

; RXNEXT: R0 = the reply byte at the cursor; the cursor advances.
; PRESERVES R2 (render loops keep their screen cell there). Calls one
; down (MSLICE, on a slice cross only). Clobbers R0,R1,R3.
RXNEXT: LODA,R0 *PRXSLC
        COMA,R0 *PCURSL
        BCTR,EQ RXN1
        STRA,R2 *PSAV2          ; shelter the caller's R2
        STRZ    R2              ; wanted slice
        STRA,R0 *PCURSL
        BSTA,UN MSLICE
        LODA,R2 *PSAV2
RXN1:   LODA,R1 *PRXOFF
        LODA,R0 FNRDATA,R1      ; the byte
        STRZ    R3
        ADDI,R1 1
        STRA,R1 *PRXOFF
        BCFR,EQ RXN2            ; wrapped to 0: next slice
        LODA,R0 *PRXSLC
        ADDI,R0 1
        STRA,R0 *PRXSLC
RXN2:   LODZ    R3
        RETC,UN

; RXHDR: R0 = the header byte at offset R1 (0-94, always slice 0).
; Calls one down (RXNEXT). Clobbers R0,R1,R3. Preserves R2.
RXHDR:  LODZ    R1
        STRA,R0 *PRXOFF
        LODI,R0 0
        STRA,R0 *PRXSLC
        BCTA,UN RXNEXT          ; tail call

; PLSEEK: point the cursor at player record R1's field R2.
; offset = GOPLYRS + PLSTRID*R1 + R2, which for the twelfth seat reaches
; 598 -- three slices in -- so it is done 16-bit off a base table rather
; than by repeated addition. The carry-out of the low add is read with
; TPSL $01 (STRA and LODI leave C alone; only arithmetic sets it).
; Leaf. Clobbers R0,R1,R3. R2 must hold the field offset on entry.
PLSEEK: LODZ    R1
        RRL,R0                  ; idx*2 (WC=0: circular, safe for idx<12)
        STRZ    R1
        LODA,R0 PLBTAB+1,R1     ; base low
        ADDZ    R2              ; + field: C is the carry into the slice
        STRA,R0 *PRXOFF
        LODI,R0 0
        TPSL    $01
        BCFR,EQ PSK1
        LODI,R0 1
PSK1:   ADDA,R0 PLBTAB,R1       ; + base high
        STRA,R0 *PRXSLC
        RETC,UN

; The twelve player-record bases, high byte first: GOPLYRS + 42*i.
PLBTAB: DB      (GOPLYRS+0*PLSTRID)>>8, (GOPLYRS+0*PLSTRID)&$FF
        DB      (GOPLYRS+1*PLSTRID)>>8, (GOPLYRS+1*PLSTRID)&$FF
        DB      (GOPLYRS+2*PLSTRID)>>8, (GOPLYRS+2*PLSTRID)&$FF
        DB      (GOPLYRS+3*PLSTRID)>>8, (GOPLYRS+3*PLSTRID)&$FF
        DB      (GOPLYRS+4*PLSTRID)>>8, (GOPLYRS+4*PLSTRID)&$FF
        DB      (GOPLYRS+5*PLSTRID)>>8, (GOPLYRS+5*PLSTRID)&$FF
        DB      (GOPLYRS+6*PLSTRID)>>8, (GOPLYRS+6*PLSTRID)&$FF
        DB      (GOPLYRS+7*PLSTRID)>>8, (GOPLYRS+7*PLSTRID)&$FF
        DB      (GOPLYRS+8*PLSTRID)>>8, (GOPLYRS+8*PLSTRID)&$FF
        DB      (GOPLYRS+9*PLSTRID)>>8, (GOPLYRS+9*PLSTRID)&$FF
        DB      (GOPLYRS+10*PLSTRID)>>8, (GOPLYRS+10*PLSTRID)&$FF
        DB      (GOPLYRS+11*PLSTRID)>>8, (GOPLYRS+11*PLSTRID)&$FF

; ----------------------------------------------------------------------
; SCSCAN: which row did this seat just take?
;
; The wire has no "last move" field, so the only way to know is to
; compare the seat's filled rows against what they were last poll. A
; full shadow of sixteen 16-bit scores is 32 bytes and this machine has
; 84 in total -- so the shadow is one BIT per slot, and PRVSCM is the
; whole cost.
;
; R1 = seat. Builds that seat's filled mask into V_NUM and reports in
; REVSLT the lowest slot filled NOW that was not filled in PRVSCM --
; $FF when none. Both masks are two bytes reached through ONE
; indexed-indirect pointer, high byte first to match V_NUM: index 1 is
; slots 0-7, index 0 is slots 8-15, and BITTAB picks the bit within.
; That is cheaper than rotating a 16-bit word through the carry, and it
; does not have to fight RXNEXT for the carry flag.
;
; Slots 6, 7 and 15 can never be the answer: the upper total, the bonus
; and the grand total fill as a SIDE EFFECT of the row actually chosen,
; and slot 6 in particular fills the moment ANY upper row is taken -- so
; without this it would win every time, being the lowest.
;
; Only the HIGH byte of each score decides: $FFFF (unset) and $FFFE
; (spectator) are the only non-scores, and both carry $FF there.
; Calls two down (RXNEXT -> MSLICE). Clobbers R0-R3, V_NUM, V_TMP,
; V_TMP2, REVSLT.
SCSCAN: LODI,R2 PLSCORS
        BSTA,UN PLSEEK
        EORZ    R0
        STRA,R0 *PVNUM0
        STRA,R0 *PVNUM1
        LODI,R0 $FF
        STRA,R0 *PREVSL
        LODI,R2 0               ; the slot index; R2 survives RXNEXT
SCS1:   BSTA,UN RXNEXT          ; the low byte decides nothing
        BSTA,UN RXNEXT
        COMI,R0 $FF
        BCTR,EQ SCS2
        LODI,R0 1               ; filled
        BCTR,UN SCS3
SCS2:   LODI,R0 0
SCS3:   STRA,R0 *PVTMP          ; the new bit
        LODZ    R2              ; this slot's bit within its byte
        ANDI,R0 7
        STRZ    R3
        LODA,R0 BITTAB,R3
        STRA,R0 *PVTMP2
        LODZ    R2              ; and which byte of the pair
        ANDI,R0 8
        BCTR,EQ SCS4
        LODI,R1 0               ; slots 8-15: the high byte
        BCTR,UN SCS5
SCS4:   LODI,R1 1               ; slots 0-7: the low byte
SCS5:   LODA,R0 *PPRVS0,R1      ; was it already filled last poll?
        ANDA,R0 *PVTMP2
        BCFR,EQ SCS7
        LODA,R0 *PVTMP          ; is it filled now?
        BCTR,EQ SCS7
        LODA,R0 *PREVSL         ; keep the first one found
        COMI,R0 $FF
        BCFR,EQ SCS7
        LODZ    R2              ; not one of the server's own three
        COMI,R0 SCUPTOT
        BCTR,EQ SCS7
        COMI,R0 SCBONUS
        BCTR,EQ SCS7
        COMI,R0 SCGRAND
        BCTR,EQ SCS7
        LODZ    R2
        STRA,R0 *PREVSL
SCS7:   LODA,R0 *PVTMP          ; record it in the mask being built
        BCTR,EQ SCS8
        LODA,R0 *PVNUM0,R1
        IORA,R0 *PVTMP2
        STRA,R0 *PVNUM0,R1
SCS8:   LODZ    R2
        ADDI,R0 1
        STRZ    R2
        COMI,R2 16
        BCTA,LT SCS1
        RETC,UN

BITTAB: DB      $01,$02,$04,$08,$10,$20,$40,$80

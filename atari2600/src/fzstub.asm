; fzstub.asm -- the filler banks.
;
; The image is (N+1) x 2048 and MAME's cart slot takes only 4K, 8K, 16K and
; 32K, so the step above seven banks is fifteen; eight of them are real. A
; bank is 2048 bytes whatever the image is, which is why splitting the
; composer could not be avoided by making the image bigger -- the size is a
; consequence of the split, not a cure for it.
;
; Nothing ever selects one of these. They are a stub back to bank 0 rather
; than filler bytes so that if something ever does, it lands somewhere with a
; defined meaning instead of executing $FF.

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "fzdefs.inc"
FZBANK  EQU     BANKLOB
FZHASDIC EQU    0

        ORG     $1000

        lda     #ENCOLD
        sta     FZENT
        lda     #BANKLOB
        jmp     FZGOTO

        END

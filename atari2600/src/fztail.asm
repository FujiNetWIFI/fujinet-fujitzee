; fztail.asm -- the fixed half: the trampoline, the shared transport, and the
; cold stub. See fzcore.inc for why each of them has to live at an address
; that does not move.

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "fzdefs.inc"
; The tail has no bank identity -- it IS the shared copy -- but it is
; assembled next to the same equates every bank uses.
FZBANK  EQU     BANKLOB
FZHASDIC EQU    0

        INCLUDE "fzcore.inc"

        END

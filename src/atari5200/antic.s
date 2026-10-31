; antic.s -- the Atari 5200's display: the 8-bit client's display list and
; charset (src/atari/graphics.c, src/atari/charset.s), in ROM where ANTIC
; reads them, and the RAM they draw into.

        .export         _displayList, _charset
        .export         _screen, _screenBak, _pmBase
        .import         __STACK_START__, __STACK_SIZE__

; The top 4K of RAM, which the Makefile keeps from the linker
; (__RESERVED_MEMORY__). Single-line player/missile graphics use only the
; block's top 1.25K, so the backup's tail shares its unused bottom.
_screen    = $3000              ; 40 x 26, inside one 4K block
_screenBak = $3440              ; resetScreen() writes a row past the screen
_pmBase    = $3800              ; 2K-aligned

.assert _screen >= __STACK_START__ + __STACK_SIZE__, error, "screen below the reserved RAM"
.assert (_screen & $F000) = ((_screen + 40 * 26 - 1) & $F000), error, "screen crosses a 4K boundary"
.assert _screenBak + 40 * 26 <= _pmBase + $300, error, "backup runs into the missiles"
.assert (_pmBase & $07FF) = 0, error, "player/missile base not 2K-aligned"

        .segment "DLIST"
_displayList:
        .byte   $70, $70                        ; 16 blank lines
        .byte   $44, <_screen, >_screen         ; mode 4, from the screen
        .repeat 25
        .byte   $04
        .endrepeat
        .byte   $41, <_displayList, >_displayList

; Generated with Atari Fontmapper: https://github.com/matosimi/atari-fontmaker
        .segment "FONT"
_charset:
        .incbin "support/atari/charset.fnt"

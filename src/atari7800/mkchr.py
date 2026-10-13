#!/usr/bin/env python3
"""mkchr.py -- the Atari 7800 tile set for Fujitzee.

The art is the MS-DOS client's, src/msdos/charset.c: 256 glyphs of 8x8 CGA
2bpp, which is a MARIA 320B tile. The engine (maria.h) holds 128 tiles, so
only the glyphs the game draws are carried over, identical bitmaps sharing a
tile, and graphics.c maps an MS-DOS glyph number through sheetTile[].

The CGA colours are renumbered so BLUE, the table, is pixel 0 (BACKGRND):

  CGA 0 blue  -> 0     palette 0: black cyan white
  CGA 1 cyan  -> 2     palette 1: blue  cyan cyan
  CGA 2 black -> 1
  CGA 3 white -> 3

Palette 1 is the MS-DOS MASK_ALT (white -> cyan, black -> blue), so a glyph's
alt colour is the same tile in palette 1: the cyan text, the highlighted dice
and the pushed roll button cost no tiles.

A table entry is the tile number, plus 0x80 for palette 1. Also generated:
  - altTile[]: drawTextAlt()'s cyan text.
  - hlFrom[]/hlTo[]: the board pieces of a player column's two dividers, and
    the same in white: the active player's column outline (graphics.c
    setHighlight), the Adam client's way of marking it.
  - T_LOGO_NOBAR: glyph 0x38 without the score-box border, as MS-DOS
    drawFujitzee() erases it for a standalone logo.
  - T_CURSOR_L/R: the MS-DOS dice cursor's checkerboard sides, for the gap
    columns either side of a die (the engine has no sprites).

Writes src/atari7800/tiles.h, support/atari7800/tilemap.lua (tile ->
character, for the MAME smoke test) and the tiles as NES 2bpp CHR, which
mktiles.py --chr turns into src/atari7800/tiles.s. The Makefile runs both
before every build.

Usage: mkchr.py out.chr
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, '..', '..')
SHEET = os.path.join(ROOT, 'src', 'msdos', 'charset.c')
TILES_H = os.path.join(HERE, 'tiles.h')
TILEMAP = os.path.join(ROOT, 'support', 'atari7800', 'tilemap.lua')

TILES = 128
PAL1 = 0x80
CGA_TO_PX = {0: 0, 1: 2, 2: 1, 3: 3}
CYAN, BLACK = 2, 1

# What the game draws: text (ASCII 32-64 are glyphs 0-32; letters are the
# lower-case slots), the board, the logo and clock, the icons in vars.h, and
# every glyph of graphics.c's diceChars[].
TEXT = list(range(0x00, 0x21)) + list(range(0x61, 0x7B))
BOARD = [0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x57, 0x58, 0x5A, 0x5B,
         0x5F, 0x60, 0x7C]
# What graphics.c drawBoard() puts in the divider columns.
OUTLINE = [0x7C, 0x5B, 0x51, 0x55, 0x50, 0x60, 0x4F, 0x5F, 0x58, 0x5A]
ART = list(range(0x37, 0x3E)) + [0x3E, 0xBE, 0xBF, 0xD9, 0xDC]
DICE = (list(range(0x20, 0x31)) + list(range(0x40, 0x4E)) + [0x50] +
        list(range(0xA0, 0xB1)) + list(range(0xC0, 0xCE)) + [0xD0])


def load_sheet():
    src = open(SHEET).read()
    body = src[src.index('{') + 1:]
    groups = re.findall(r'\{([^{}]*)\}', body)
    glyphs = [bytes(int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]{2})', g)) for g in groups]
    if len(glyphs) != 256 or any(len(g) != 16 for g in glyphs):
        raise SystemExit('mkchr: %s is not 256 glyphs of 16 bytes' % SHEET)
    tiles = []
    for b in glyphs:
        px = []
        for r in range(8):
            w = (b[r * 2] << 8) | b[r * 2 + 1]
            px.append(tuple(CGA_TO_PX[(w >> (14 - 2 * i)) & 3] for i in range(8)))
        tiles.append(tuple(px))
    return tiles


def white(px):
    return tuple(tuple(3 if p == CYAN else p for p in row) for row in px)


def chr_bytes(px):
    p0 = bytes(sum(((px[r][c] & 1) << (7 - c)) for c in range(8)) for r in range(8))
    p1 = bytes(sum((((px[r][c] >> 1) & 1) << (7 - c)) for c in range(8)) for r in range(8))
    return p0 + p1


def cursor_side(cols):
    """A gap cell crossed by the cursor frame's two checkerboard columns."""
    return tuple(tuple((CYAN if (r + c) & 1 == 0 else BLACK) if c in cols else 0
                       for c in range(8)) for r in range(8))


def glyph_char(g):
    if g < 0x20:
        return chr(g + 0x20)
    if 0x61 <= g <= 0x7A:
        return chr(g - 0x20)
    return '#'


def main():
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    sheet = load_sheet()
    tiles, chars, index = [], {}, {}

    def add(px, ch):
        if px not in index:
            index[px] = len(tiles)
            tiles.append(px)
            chars[len(tiles) - 1] = ch
        return index[px]

    # Glyph 0 (all blue) first, so a blank reads as ' ', then the font, so a
    # text tile reads as its character even when an art tile shares it.
    sheetTile = [0] * 256
    for g in [0] + TEXT:
        sheetTile[g] = add(sheet[g], glyph_char(g))

    # The alt-colour dice and roll button (0xA0-0xD0) are their plain glyph
    # in palette 1; the few pixels MS-DOS keeps black there show blue.
    for g in BOARD + ART + DICE:
        if g in TEXT:
            continue
        if g >= 0x80 and g in DICE:
            sheetTile[g] = sheetTile[g - 0x80] | PAL1
        else:
            sheetTile[g] = add(sheet[g], '#')

    altTile = list(sheetTile[:128])
    for g in TEXT:
        altTile[g] = sheetTile[g] | PAL1

    hlFrom = [sheetTile[g] for g in OUTLINE]
    hlTo = [add(white(sheet[g]), '#') for g in OUTLINE]

    logo = tuple(tuple([0, 0, 0, 0] + list(row[4:])) for row in sheet[0x38])
    defs = [('T_BLANK', sheetTile[0], ''),
            ('T_LOGO_NOBAR', add(logo, '#'), 'glyph 0x38 without the score-box border'),
            ('T_CURSOR_L', add(cursor_side((5, 6)), '#'), 'dice cursor, gap cell left of the die'),
            ('T_CURSOR_R', add(cursor_side((1, 2)), '#'), 'dice cursor, gap cell right of the die')]

    used = len(tiles)
    if used > TILES:
        raise SystemExit('mkchr: %d tiles, the engine holds %d' % (used, TILES))
    blank = tuple((0,) * 8 for _ in range(8))
    while len(tiles) < TILES:
        tiles.append(blank)
        chars[len(tiles) - 1] = ' '

    def table(name, values, per=16):
        out = ['static const unsigned char %s[%d] = {' % (name, len(values))]
        for i in range(0, len(values), per):
            out.append('    ' + ', '.join('0x%02X' % v for v in values[i:i + per]) + ',')
        out.append('};')
        return '\n'.join(out)

    with open(TILES_H, 'w') as f:
        f.write('/* GENERATED by src/atari7800/mkchr.py from src/msdos/charset.c: tile\n'
                ' * numbers in the engine\'s tile set, 0x80 for palette 1; %d of %d\n'
                ' * used. Include from graphics.c only (it defines tables). Do not edit. */\n'
                '#ifndef TILES_H\n#define TILES_H\n\n' % (used, TILES))
        for name, v, note in defs:
            f.write('#define %-16s 0x%02X%s\n' % (name, v, ('  /* %s */' % note) if note else ''))
        f.write('#define PAL1             0x80\n\n')
        f.write('/* MS-DOS glyph -> tile, as plotted with MASK_WHITE. */\n')
        f.write(table('sheetTile', sheetTile) + '\n\n')
        f.write('/* MS-DOS glyph 0-127 -> tile, as plotted with MASK_ALT (cyan text). */\n')
        f.write(table('altTile', altTile) + '\n\n')
        f.write('/* A divider column\'s board pieces, and the same in white. */\n')
        f.write('#define HL_PIECES %d\n' % len(OUTLINE))
        f.write(table('hlFrom', hlFrom) + '\n')
        f.write(table('hlTo', hlTo) + '\n\n')
        f.write('#endif /* TILES_H */\n')

    os.makedirs(os.path.dirname(os.path.abspath(sys.argv[1])), exist_ok=True)
    with open(sys.argv[1], 'wb') as f:
        for px in tiles:
            f.write(chr_bytes(px))

    os.makedirs(os.path.dirname(TILEMAP), exist_ok=True)
    with open(TILEMAP, 'w') as f:
        f.write('-- GENERATED by src/atari7800/mkchr.py: what each tile reads as.\n'
                '-- "#" is game art, " " a blank.\n'
                'return {\n')
        for i in range(TILES):
            ch = chars[i]
            f.write('  [%d] = %s,\n' % (i, '"\\""' if ch == '"' else '"%s"' % ch.replace('\\', '\\\\')))
        f.write('}\n')
    print('mkchr: %d of %d tiles' % (used, TILES))


if __name__ == '__main__':
    main()

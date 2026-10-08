#!/usr/bin/env python3
"""mktiles.py -- build the Master System tile set for Fujitzee.

The art is the MS-DOS client's, as on the NES: src/msdos/charset.c, 256
glyphs of 8x8 CGA 2bpp, read by src/nes/mkchr.py's load_sheet(). Every glyph
is carried over (identical bitmaps share a tile), so graphics.c can draw any
MS-DOS glyph number through sheetTile[] -- the shared code's ICON_* values
and the MS-DOS dice table work unchanged.

Mode 4 tiles are 4bpp, so the CGA colours keep their own numbers and the
palette is the MS-DOS client's VGA remap, which the Master System's 6-bit
CRAM holds exactly:

  0  blue  $20  (0,0,170)     palette 1: black -- the active player's column
  1  cyan  $3D  (85,255,255)
  2  black $00
  3  white $3F
  4  blue  $20                palette 1: still blue ("B", fixed blue)

Palette 1 is the active column (graphics.c setHighlight): the MS-DOS client
turns that column's blue background black, pixel by pixel, from the right
half of its left divider to its right divider, rows 0-19, and the top half
of the bottom rule on row 20. A name table entry chooses its palette per
cell, so the column is cells hx-1..hx+2 in palette 1, and the few blue
pixels inside those cells that MS-DOS leaves blue are drawn in B instead:

  - the left half (columns 0-3) of the junction glyphs on the divider
    column: 0x51 and 0x5B on row 0, and 0x58 on row 20 (whose lower half is
    below the board as well);
  - T_RULE_BOTTOM: the thick rule 0x52 with its lower half (rows 4-7) in B,
    for row 20 only -- 0x52 itself is the top and header rules, where MS-DOS
    blackens the whole cell.

B renders exactly as blue in palette 0, so these glyphs look unchanged
everywhere else.

Two pieces of dice art the MS-DOS sheet lost are restored from the Atari
original, whose alternate dice are the same glyphs in another colour:
  - the highlighted dice (a Fujitzee, and every die flashing when all five
    are kept), 0xC0-0xCB and 0xA0-0xAB, are white dice drawn cyan -- the
    sheet's copies are exactly that, but with the black pips gone cyan too.
    The script checks that the pips are the only difference.
  - the roll button's "cannot roll" X, the Atari's glyph 0x50, whose slot the
    MS-DOS sheet gave to a board cross. It goes into two glyphs the sheet
    leaves empty: 0xCF (graphics.c's dice table points there) and 0xD0,
    which the table already uses for the pushed button.

Also generated, as mkchr.py does for the NES:
  - altTile[]: drawTextAlt()'s cyan text, the MS-DOS MASK_ALT (glyph & 0x55:
    white -> cyan, black -> blue) applied to the font glyphs.
  - T_LOGO_NOBAR: glyph 0x38 without the score-box border.
  - cursorSprite[]: the MS-DOS dice cursor -- a 30x30 two-pixel cyan/black
    checkerboard frame one pixel outside the die -- cut into the twelve
    border tiles of a 4x4 sprite grid. Sprites take palette 1, where 0 is
    transparent, 1 cyan and 2 black.

Every tile is below 256: crt0 sets the sprite pattern base to tile 0 (R6 =
$FB), and byte tables are cheaper on the Z80. The script fails rather than
truncate, and fails if the art outgrows the 12K of bank 2 that the
cartridge's mailbox arena ($B000-$BFFF) leaves.

Writes src/sms/tiles.asm (the art, in SECTION BANK_02 -- the 48K image's
third bank, which the Sega mapper keeps in slot 2 at $8000), src/sms/tiles.h
and support/sms/tilemap.lua (tile -> character, for the MAME smoke test).
The Makefile runs it before every SMS build; each file is rewritten only when
its content changes, so the build does not redo work for nothing.
"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, '..', '..')
ATARI_FONT = os.path.join(ROOT, 'support', 'atari', 'charset.fnt')
TILES_ASM = os.path.join(HERE, 'tiles.asm')
TILES_H = os.path.join(HERE, 'tiles.h')
TILEMAP = os.path.join(ROOT, 'support', 'sms', 'tilemap.lua')

# src/nes/__pycache__ is in the tree; importing must not rewrite it.
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.join(ROOT, 'src', 'nes'))
from mkchr import load_sheet, masked, cursor_frame, ALT_GLYPHS  # noqa: E402

B = 4                       # fixed blue
SPR_CYAN, SPR_BLACK = 1, 2  # sprite pixel values (palette 1)

PAL0 = [0x20, 0x3D, 0x00, 0x3F, 0x20] + [0x00] * 11
PAL1 = [0x00, 0x3D, 0x00, 0x3F, 0x20] + [0x00] * 11

# Atari colour -> CGA colour: the MS-DOS sheet drew the Atari's colour 1 (the
# rules' shadow, the pips) black and colour 2 (the rules) cyan.
ATARI_TO_CGA = {0: 0, 1: 2, 2: 1, 3: 3}
NOROLL, NOROLL_HI = 0xCF, 0xD0

TILE_MAX = 256
BANK2_MAX = 0x3000          # $8000-$AFFF; the arena covers $B000-$BFFF

# How the smoke test reads board chrome, so a text dump shows the grid.
CHROME = {0x52: '-', 0x54: '-', 0x7C: '|', 0x4F: '+', 0x50: '+', 0x51: '+',
          0x53: '+', 0x55: '+', 0x57: '+', 0x58: '+', 0x5A: '+', 0x5B: '+',
          0x5F: '+', 0x60: '+'}


def planar(px):
    """One Mode 4 tile: per row, bitplanes 0-3, leftmost pixel in bit 7."""
    out = bytearray()
    for row in px:
        for plane in range(4):
            out.append(sum(((p >> plane) & 1) << (7 - c) for c, p in enumerate(row)))
    return bytes(out)


def fixed_blue(px, where):
    """Blue pixels for which where(row, col) holds, redrawn in B."""
    return [[B if p == 0 and where(r, c) else p for c, p in enumerate(row)]
            for r, row in enumerate(px)]


def white_to_cyan(px):
    return [[1 if p == 3 else p for p in row] for row in px]


def atari_glyph(g):
    """Atari ANTIC mode 4 glyph g (four 2-bit pixels a byte, each two wide)."""
    data = open(ATARI_FONT, 'rb').read()[g * 8:g * 8 + 8]
    return [[ATARI_TO_CGA[(b >> (6 - 2 * (c >> 1))) & 3] for c in range(8)] for b in data]


def lit_dice(sheet, art):
    """Highlighted dice: the white dice in cyan, black pips kept."""
    for base, lit in ((0x40, 0xC0), (0x20, 0xA0)):
        for k in range(12):
            want = white_to_cyan(sheet[base + k])
            for r in range(8):
                for c in range(8):
                    if sheet[base + k][r][c] != 2 and sheet[lit + k][r][c] != want[r][c]:
                        raise SystemExit('mktiles: glyph 0x%02X is not glyph 0x%02X in cyan'
                                         % (lit + k, base + k))
            art[lit + k] = want


def write_if_changed(path, text):
    try:
        if open(path).read() == text:
            return False
    except FileNotFoundError:
        pass
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w') as f:
        f.write(text)
    return True


def main():
    sheet = load_sheet()

    art = list(sheet)
    art[0x51] = fixed_blue(sheet[0x51], lambda r, c: c < 4)
    art[0x5B] = fixed_blue(sheet[0x5B], lambda r, c: c < 4)
    art[0x58] = fixed_blue(sheet[0x58], lambda r, c: c < 4 or r >= 4)

    lit_dice(sheet, art)
    for g in (NOROLL, NOROLL_HI):
        if any(p for row in sheet[g] for p in row):
            raise SystemExit('mktiles: glyph 0x%02X is no longer empty' % g)
    art[NOROLL] = atari_glyph(0x50)
    art[NOROLL_HI] = white_to_cyan(art[NOROLL])

    tiles, chars, index = [], {}, {}
    defs = []

    def add(px, ch):
        data = planar(px)
        if data in index:
            return index[data]
        tiles.append(data)
        chars[len(tiles) - 1] = ch
        index[data] = len(tiles) - 1
        return len(tiles) - 1

    def define(name, idx, note=''):
        defs.append('#define %-16s 0x%02X%s' % (name, idx, ('  /* %s */' % note) if note else ''))

    def glyph_char(g):
        if g < 0x20:
            return chr(g + 0x20)
        if 0x61 <= g <= 0x7A:
            return chr(g - 0x20)
        return CHROME.get(g, '#')

    # Glyph 0 (all blue) first, so a blank is tile 0 and reads as ' ', then
    # the font, so a text tile reads as its character even when an art tile
    # shares it.
    sheetTile = [0] * 256
    for g in [0] + ALT_GLYPHS + list(range(256)):
        sheetTile[g] = add(art[g], glyph_char(g))
    define('T_BLANK', sheetTile[0])

    altTile = [0] * 128
    for g in range(128):
        altTile[g] = add(masked(sheet[g], 0x55), glyph_char(g)) if g in ALT_GLYPHS else sheetTile[g]

    logo = [row[:] for row in sheet[0x38]]
    for row in logo:
        row[0:4] = [0, 0, 0, 0]
    define('T_LOGO_NOBAR', add(logo, '#'), 'glyph 0x38 without the score-box border')
    define('T_RULE_BOTTOM', add(fixed_blue(sheet[0x52], lambda r, c: r >= 4), '-'),
           'the bottom rule (row 20): 0x52, lower half fixed blue')
    define('T_TEE_BOTTOM', sheetTile[0x58], 'a divider meeting the bottom rule')

    # The cursor frame on a 4x4 sprite grid whose top left is (-4, -4) from
    # the die's. The four middle tiles are empty and not emitted.
    frame = cursor_frame()
    sprTiles = []
    for ty in range(4):
        for tx in range(4):
            if 1 <= tx <= 2 and 1 <= ty <= 2:
                continue
            px = [[0] * 8 for _ in range(8)]
            for r in range(8):
                for c in range(8):
                    p = frame.get((tx * 8 - 4 + c, ty * 8 - 4 + r))
                    if p:
                        px[r][c] = SPR_CYAN if p == 1 else SPR_BLACK
            sprTiles.append((tx, ty, add(px, '#')))

    used = len(tiles)
    if used > TILE_MAX:
        raise SystemExit('mktiles: %d tiles, sprites can only reach the first %d' % (used, TILE_MAX))
    bank2 = 3 + 32 * used + 3
    if bank2 > BANK2_MAX:
        raise SystemExit('mktiles: %d bytes of art, bank 2 has %d before the arena' % (bank2, BANK2_MAX))
    define('T_COUNT', used, 'tiles uploaded, from tile 0')

    # ---- tiles.asm: one vdp_runs() run from tile 0, then the end mark ----
    asm = ['; GENERATED by src/sms/mktiles.py from src/msdos/charset.c -- do not edit.',
           '; %d tiles, %d bytes, in the third 16K bank: the Sega mapper keeps it in' % (used, bank2),
           '; slot 2 ($8000), the 12K below the cartridge\'s mailbox arena.',
           '; vdp_runs() format (src/sms/vdp.asm): { first tile lo, hi, count } and',
           '; count * 32 bytes of 4bpp planar rows; a zero count ends it.',
           '',
           '    SECTION BANK_02',
           '',
           '    PUBLIC  _smsTiles',
           '',
           '_smsTiles:',
           '    defb    $00,$00,$%02X' % used]
    for i, t in enumerate(tiles):
        asm.append('    defb    %s ; tile $%02X' % (','.join('$%02X' % b for b in t), i))
    asm.append('    defb    $00,$00,$00')
    asm_changed = write_if_changed(TILES_ASM, '\n'.join(asm) + '\n')

    # ---- tiles.h ----
    def table(name, values, per=16):
        out = ['static const unsigned char %s[%d] = {' % (name, len(values))]
        for i in range(0, len(values), per):
            out.append('    ' + ', '.join('0x%02X' % v for v in values[i:i + per]) + ',')
        out.append('};')
        return '\n'.join(out)

    h = ['/* GENERATED by src/sms/mktiles.py from src/msdos/charset.c. Tile numbers',
         ' * in VRAM from $0000; %d of %d used.' % (used, TILE_MAX),
         ' * Include from graphics.c only (it defines tables). Do not edit. */',
         '#ifndef TILES_H', '#define TILES_H', '']
    h += defs
    h += ['',
          '/* The art, in bank 2 (src/sms/tiles.asm): readable only while bank 2 is',
          ' * in slot 2, which mapperInit() (src/sms/vdp.asm) sets and nothing',
          ' * changes. */',
          'extern const unsigned char smsTiles[];',
          '',
          '/* CRAM: background palette 0, then palette 1 (the active column, and',
          ' * the sprites\'). */',
          table('smsPalette', PAL0 + PAL1), '',
          '/* MS-DOS glyph -> tile, as plotted with MASK_WHITE. */',
          table('sheetTile', sheetTile), '',
          '/* MS-DOS glyph 0-127 -> tile, as plotted with MASK_ALT (cyan text). */',
          table('altTile', altTile), '',
          '/* Dice cursor sprites: grid column, grid row (8px steps from 4px',
          ' * above and left of the die), tile. */',
          '#define CURSOR_SPRITES %d' % len(sprTiles),
          table('cursorSprite', [v for t in sprTiles for v in t], per=3), '',
          '#endif /* TILES_H */']
    h_changed = write_if_changed(TILES_H, '\n'.join(h) + '\n')

    # ---- tilemap.lua ----
    lua = ['-- GENERATED by src/sms/mktiles.py: what each background tile reads as.',
           '-- "#" is game art, " " a blank, "-" "|" "+" the board\'s rules.',
           'return {']
    for i in range(used):
        ch = chars[i]
        lua.append('  [%d] = %s,' % (i, '"\\""' if ch == '"' else '"%s"' % ch.replace('\\', '\\\\')))
    lua.append('}')
    lua_changed = write_if_changed(TILEMAP, '\n'.join(lua) + '\n')

    changed = [n for n, c in (('tiles.asm', asm_changed), ('tiles.h', h_changed),
                              ('tilemap.lua', lua_changed)) if c]
    print('mktiles: %d of %d tiles, %d bytes in bank 2%s'
          % (used, TILE_MAX, bank2, (' (wrote %s)' % ', '.join(changed)) if changed else ''))


if __name__ == '__main__':
    main()

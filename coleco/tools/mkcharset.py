#!/usr/bin/env python3
"""Generate coleco/assets/charset.inc from the MS-DOS port's graphics.

Source: src/msdos/charset.c -- 256 glyphs of 8x8 CGA 2bpp (2 bytes per pixel
row, byte 0 = left four pixels), where the palette is

    0 = dark blue (table background)   2 = black (dice pips, shadow)
    1 = cyan (chrome, highlights)      3 = white (text, dice bodies)

and which carries, besides the dice and chrome, the NAMCO ARCADE FONT: digits
at 0x10-0x19 and A-Z at 0x61-0x7A. The letterforms live in the *lowercase*
slots because in that port ASCII case selects a colour, not a glyph -- its
uppercase slots hold dice art.

NOT support/msdos/charset.dat. That file is stale: it still holds the
pre-Namco block font (commit 5786998 changed only charset.c), so re-running
support/msdos/create_charset would silently regress the font. Diffed here to
be sure: 3801 of 4096 bytes match, and the 36 that differ are exactly the
digits and A-Z.

TMS9918 Graphics II gives one foreground and one background colour per
8-pixel row of each pattern, which is why every glyph below is emitted as an
8-byte pattern plus, for the art tiles, an 8-byte colour block. Text needs no
colour table at all: a whole bank is one ink, so disp.inc fills it.

Code space (256 codes, replicated identically in all three screen thirds):

    0x00-0x6F  112  art tiles: dice in three colour banks, the roll tile,
                    the fujiTZEE logo, board chrome, icons
    0x70-0xAF   64  font, WHITE on dark blue        code = ascii + 0x50
    0xB0-0xEF   64  font, CYAN on dark blue         code = ascii + 0x90
    0xF0-0xFF   16  digits, LIGHT GREEN             code = ascii + 0xC0

Run from anywhere:  python3 coleco/tools/mkcharset.py
The generated assets/charset.inc is committed; tweak this generator and
regenerate rather than hand-editing the output.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
MSDOS = os.path.join(REPO, "src", "msdos", "charset.c")
OUT = os.path.join(REPO, "coleco", "assets", "charset.inc")

# TMS9918 inks
BLACK, LGREEN, DBLUE, CYAN, DYELLOW, LYELLOW, WHITE = 1, 3, 4, 7, 10, 11, 15

# MS-DOS CGA palette entry -> TMS ink
INK = {0: DBLUE, 1: CYAN, 2: BLACK, 3: WHITE}

BG = DBLUE

FONT_LO, FONT_HI = 0x20, 0x5F          # the ASCII span the font banks cover
TILE_MAX = 0x70                        # art tiles must fit below this code


# ---------------------------------------------------------------- sources

def parse_charset(path):
    body = open(path).read().split("charset[256][16] =", 1)[1].split("};", 1)[0]
    data = [int(v, 16) for v in re.findall(r"0x[0-9a-fA-F]{2}", body)]
    if len(data) != 256 * 16:
        sys.exit("%s: expected 4096 bytes, got %d" % (path, len(data)))
    return data


def pixels(data, idx):
    """The glyph's 8 rows, each a list of eight 2bpp palette indices."""
    rows = []
    for r in range(8):
        b0, b1 = data[idx * 16 + r * 2], data[idx * 16 + r * 2 + 1]
        row = [(b0 >> (6 - 2 * p)) & 3 for p in range(4)]
        row += [(b1 >> (6 - 2 * p)) & 3 for p in range(4)]
        rows.append(row)
    return rows


# ------------------------------------------------------------- conversion

def mono(data, idx):
    """1bpp pattern: a bit for every non-background pixel.

    Every Namco glyph is drawn entirely in colour 3, so this is lossless for
    text -- 7x7 inside an 8x8 cell, two-pixel stroke, right column and bottom
    row empty.
    """
    out = []
    for row in pixels(data, idx):
        bits = 0
        for p, v in enumerate(row):
            if v:
                bits |= 0x80 >> p
        out.append(bits)
    return out


def art(data, idx, remap=None, name="?", clear_left=False):
    """2bpp glyph -> (pattern, colour) for Graphics II.

    Graphics II allows exactly two colours per 8-pixel row. A die body plus
    its pips is white + black, and a rounded corner is blue + white; both are
    fine. What is not fine is a rounded corner tile that ALSO carries a pip,
    which is three. Rather than emit something subtly wrong, square the corner
    off on that row alone -- the outside pixels join the body -- and if that
    still leaves three colours, fail the build.
    """
    pattern, colour = [], []
    for y, row in enumerate(pixels(data, idx)):
        if clear_left:
            row = [0] * 4 + row[4:]
        inks = [INK[v] for v in row]
        if remap:
            inks = [remap.get(i, i) for i in inks]
        bits, col = reduce_row(inks, y, name, idx)
        pattern.append(bits)
        colour.append(col)
    return pattern, colour


def reduce_row(inks, y=0, name="?", idx=0):
    """One 8-pixel row of inks -> (pattern byte, fg<<4|bg)."""
    if True:
        distinct = set(inks)
        if len(distinct) > 2 and BG in distinct:
            # Square off the corner: background pixels become the body colour,
            # which is the most common ink that is not the background.
            body = max((i for i in inks if i != BG), key=inks.count)
            inks = [body if i == BG else i for i in inks]
            distinct = set(inks)
        if len(distinct) > 2:
            sys.exit("glyph %s (msdos 0x%02X) row %d needs %d colours: %s"
                     % (name, idx, y, len(distinct), sorted(distinct)))

        if len(distinct) == 1:
            fg = bg = distinct.pop()
        else:
            # Foreground is whichever is not the background where one of them
            # is; otherwise the rarer ink, so the bits describe the detail.
            if BG in distinct:
                bg = BG
                fg = (distinct - {BG}).pop()
            else:
                fg = min(distinct, key=inks.count)
                bg = (distinct - {fg}).pop()

        bits = 0
        for p, ink in enumerate(inks):
            if ink == fg:
                bits |= 0x80 >> p
        return bits, (fg << 4) | bg


# --------------------------------------------------------- synthetic art

def solid(colour_pairs):
    return [0xFF] * 8, list(colour_pairs)


V_LINE = [0x18] * 8                                    # vertical divider
H_THICK = [0, 0, 0, 0xFF, 0xFF, 0, 0, 0]               # rule, centred
H_THIN = [0, 0, 0, 0, 0xFF, 0, 0, 0]
BOX_TOP = [0, 0xFF, 0xFF, 0, 0, 0, 0, 0]
BOX_BOT = [0, 0, 0, 0, 0, 0xFF, 0xFF, 0]
CUR_L = [0x03] * 8                                     # dice cursor, left bar
CUR_R = [0xC0] * 8                                     # dice cursor, right bar


def union(*pats):
    out = list(pats[0])
    for other in pats[1:]:
        out = [x | y for x, y in zip(out, other)]
    return out


def synthesized():
    """The glyphs the Namco font cannot supply, drawn at its own 2px weight.

    Two different problems, both of which render as something wrong rather
    than as nothing:

    '-', '/' and ':' are simply BLANK in that font. Not cosmetic: the round
    counter reads "R n/13", the lobby's seat count is "cur / max", and a closed
    scorecard row is a dash.

    '*' and '#' are worse -- those slots are not punctuation at all. The MS-DOS
    charset keeps its ICONS there: index 0x0A is the little-person marker and
    0x03 is half the connection symbol. So every footer that tells you which
    keypad key to press ("* BACK", "# OK") drew a person and a lightning bolt.
    On a console whose only named keys are * and #, that is the one thing the
    footers exist to say.
    """
    return {
        0x23: [0x66, 0x66, 0xFE, 0x66, 0xFE, 0x66, 0x66, 0],     # #
        0x2A: [0, 0xC6, 0x6C, 0xFE, 0x6C, 0xC6, 0, 0],           # *
        0x2D: [0, 0, 0, 0x7C, 0x7C, 0, 0, 0],                    # -
        0x2F: [0x04, 0x0C, 0x18, 0x30, 0x60, 0x40, 0, 0],        # /
        0x3A: [0, 0x30, 0x30, 0, 0x30, 0x30, 0, 0],              # :
    }


def logo_tiles(data):
    """The fujiTZEE logo, packed into FIVE tiles rather than six.

    MS-DOS draws it as six 8px tiles, but the first of those is that port's
    score-box left border -- its only logo content is the two-pixel tail of the
    "j" -- which is why drawFujitzee() blanks the border away before using it.

    Six cells do not fit here. The scorecard's label column is five cells wide
    (2-6, with the column divider at 7), and slot 14 is labelled by the logo
    itself, so a six-cell logo pokes into the first player's divider. Stripping
    the border leaves exactly forty inked columns, so the whole image is
    shifted flush left and split into five tiles: nothing is lost and it lands
    in the column it has to live in.
    """
    rows = []
    for y in range(8):
        line = []
        for n, idx in enumerate(range(0x38, 0x3E)):
            row = pixels(data, idx)[y]
            if n == 0:
                row = [0] * 4 + row[4:]          # strip the box border
            line += row
        rows.append([INK[v] for v in line])

    inked = [x for r in rows for x, ink in enumerate(r) if ink != BG]
    lead, tail = min(inked), max(inked)
    if tail - lead >= 40:
        sys.exit("logo spans %d columns; it must fit five tiles"
                 % (tail - lead + 1))

    out = []
    for t in range(5):
        pat, col = [], []
        for y in range(8):
            seg = rows[y][lead + t * 8:lead + t * 8 + 8]
            seg += [BG] * (8 - len(seg))
            bits, c = reduce_row(seg, y, "LOGO%d" % t)
            pat.append(bits)
            col.append(c)
        out.append((pat, col))
    return out


# ------------------------------------------------------------------ main

def main():
    data = parse_charset(MSDOS)

    tiles = []            # (name, pattern, colour)
    names = {}

    def add(name, pattern, colour):
        if name in names:
            return names[name]
        code = len(tiles)
        if code >= TILE_MAX:
            sys.exit("art tiles overflow past 0x%02X" % TILE_MAX)
        names[name] = code
        tiles.append((name, pattern, colour))
        return code

    def add_art(name, idx, remap=None):
        p, c = art(data, idx, remap, name)
        return add(name, p, c)

    def add_mono(name, idx, ink):
        return add(name, mono(data, idx), [(ink << 4) | BG] * 8)

    # -- dice ------------------------------------------------------------
    # The MS-DOS diceChars[] table composes each die from 3x3 tiles. Its
    # distinct tiles are 0x40-0x4B (normal bodies and pips), 0x4C/0x4D (the
    # kept-die clamp bars) and 0x20-0x2B (the kept-die bodies). We take all of
    # them and give each colour bank its own codes, because Graphics II colours
    # a pattern, not a cell.
    NORMAL = list(range(0x40, 0x4C))
    CLAMP = [0x4C, 0x4D]
    ROLL = [0x2C, 0x2D, 0x2E, 0x2F, 0x30]

    # The held die is the NORMAL die recoloured, with the clamp bars laid over
    # its top and bottom edges -- NOT the MS-DOS kept-die tiles at 0x20-0x2B.
    # Those are the same shapes with a black drop shadow along the die's outer
    # edge, which that port draws for depth. Recoloured yellow the shadow stops
    # reading as a shadow and starts reading as damage: a two-pixel black notch
    # in the corner of an otherwise clean die, which is exactly what it looked
    # like on screen. The normal tiles have no shadow and keep their rounded
    # corners, so the held die now matches the white ones in every way but its
    # colour and its clamps.

    # Held dice are YELLOW; that is the one colour rule the port was asked for
    # by name. The cursor is not a recolour (see disp.inc) so it costs no bank.
    TO_YELLOW = {WHITE: LYELLOW, CYAN: LYELLOW}
    TO_CYAN = {WHITE: CYAN}

    for i in NORMAL:
        add_art("DW%02X" % i, i)
    for i in NORMAL + CLAMP:
        add_art("DY%02X" % i, i, TO_YELLOW)
    for i in NORMAL:
        add_art("DC%02X" % i, i, TO_CYAN)
    for i in NORMAL + CLAMP:
        add_art("DK%02X" % i, i, TO_CYAN)
    for i in ROLL:
        add_art("RW%02X" % i, i)
    for i in ROLL:
        add_art("RC%02X" % i, i, TO_CYAN)

    # -- the fujiTZEE logo ------------------------------------------------
    logo = logo_tiles(data)
    for n, (p, c) in enumerate(logo):
        add("LOGO%d" % n, p, c)

    # -- icons ------------------------------------------------------------
    add_mono("ICURS", 0x3E, LYELLOW)        # score cursor arrowhead
    add_mono("ICURSA", 0xBE, CYAN)          # ... its alternate frame
    add_mono("IMARK", 0x1D, CYAN)           # ready / turn mark
    add_mono("IMARKA", 0x1C, WHITE)
    add_mono("ICLOCK", 0x37, CYAN)          # turn clock
    add_mono("ICONN1", 0x03, CYAN)          # connection-trouble icon, 2 cells
    add_mono("ICONN2", 0x04, CYAN)
    add_mono("ISPEC", 0x05, CYAN)           # spectator

    # -- board chrome ------------------------------------------------------
    # Synthesized rather than lifted. The MS-DOS chrome is a two-pixel cyan
    # stroke with a one-pixel black drop shadow on blue -- three colours in
    # most of its rows, which Graphics II cannot render in one cell. Drawn
    # fresh as single-ink rules it costs nothing and reads better at this size.
    gold = [(DYELLOW << 4) | BG] * 8
    add("CVERT", V_LINE, gold)
    add("CTHICK", H_THICK, gold)
    add("CTHIN", H_THIN, gold)
    add("CXTHICK", union(V_LINE, H_THICK), gold)
    add("CXTHIN", union(V_LINE, H_THIN), gold)
    add("CBOXT", BOX_TOP, gold)
    add("CBOXB", BOX_BOT, gold)
    add("CBOXTL", union([0x18] * 8, BOX_TOP), gold)
    add("CBOXTR", union([0x18] * 8, BOX_TOP), gold)
    add("CCURL", CUR_L, [(CYAN << 4) | BG] * 8)
    add("CCURR", CUR_R, [(CYAN << 4) | BG] * 8)
    add("CBLANK", [0] * 8, [(BG << 4) | BG] * 8)

    # -- the font ----------------------------------------------------------
    # ASCII 0x20-0x5F. The Namco letterforms are at the msdos lowercase slots;
    # its digits and the four punctuation marks it does have are at
    # (ascii - 0x20). Everything else stays as it comes except the five in
    # synthesized(), which override whatever that slot held.
    synth = synthesized()
    font = []
    for ch in range(FONT_LO, FONT_HI + 1):
        if ch in synth:
            font.append(synth[ch])
        elif 0x41 <= ch <= 0x5A:                 # A-Z
            font.append(mono(data, ch + 0x20))
        else:                                    # digits and punctuation
            font.append(mono(data, ch - 0x20))

    # -- the dice composition table ----------------------------------------
    # The MS-DOS diceChars[] rows, re-indexed into our codes. Six faces per
    # bank, nine tiles each, in the order drawDie writes them.
    def T(prefix, i):
        return names["%s%02X" % (prefix, i)]

    def bank(pfx_body, pfx_clamp, kept):
        """One colour bank's six faces, nine tile codes each."""
        rows = [
                [0x41, 0x40, 0x42, 0x40, 0x45, 0x40, 0x43, 0x40, 0x44],
                [0x41, 0x40, 0x47, 0x40, 0x40, 0x40, 0x48, 0x40, 0x44],
                [0x41, 0x40, 0x47, 0x40, 0x45, 0x40, 0x48, 0x40, 0x44],
                [0x46, 0x40, 0x47, 0x40, 0x40, 0x40, 0x48, 0x40, 0x49],
                [0x46, 0x40, 0x47, 0x40, 0x45, 0x40, 0x48, 0x40, 0x49],
                [0x46, 0x40, 0x47, 0x4A, 0x40, 0x4B, 0x48, 0x40, 0x49],
        ]
        if kept:                    # the clamp bars replace the two edge tiles
            for row in rows:
                row[1], row[7] = -1, -2
        out = []
        for row in rows:
            for v in row:
                if v == -1:
                    out.append(T(pfx_clamp, 0x4C))
                elif v == -2:
                    out.append(T(pfx_clamp, 0x4D))
                else:
                    out.append(T(pfx_body, v))
        return out

    dicetab = []
    dicetab += bank("DW", "DW", False)                 # +0    normal, white
    dicetab += bank("DY", "DY", True)                  # +54   held, YELLOW
    dicetab += [names["CBLANK"]] * 9                   # +108  blank
    # The roll tile borrows the normal die's frame and fills the middle row
    # with "ROLL"; its bottom-centre cell is the rolls-left digit, which is
    # the only thing that changes between the three variants.
    # Rolls left: "1", "2", and none. The MS-DOS table puts a chrome cross in
    # the last slot, which converts to a cyan-and-black blot sitting inside the
    # tile and reads as corruption; a blank count says the same thing, and the
    # ROLLS counter beside it says it in words.
    for variant in ("RW30", "RW2F", "DW40"):           # +117  roll tile x3
        dicetab += [names["DW41"], names["DW40"], names["DW42"],
                    names["RW2C"], names["RW2D"], names["RW2E"],
                    names["DW43"], names[variant], names["DW44"]]
    dicetab += bank("DC", "DC", False)                 # +144  fujitzee, cyan
    dicetab += bank("DK", "DK", True)                  # +198  held + fujitzee

    # ---------------------------------------------------------------- emit
    def block(label, rows, comment):
        flat = [b for row in rows for b in row]
        out = ["; %s" % comment, "%s:" % label]
        for i in range(0, len(flat), 8):
            out.append("        DB      " +
                       ",".join("0%02XH" % b for b in flat[i:i + 8]))
        return "\n".join(out) + "\n"

    with open(OUT, "w") as f:
        f.write("; charset.inc -- GENERATED by tools/mkcharset.py; do not"
                " hand-edit.\n;\n"
                "; Converted from src/msdos/charset.c: the NAMCO font, the\n"
                "; dice, the fujiTZEE logo and the icons, decoded from CGA\n"
                "; 2bpp into TMS9918 Graphics II pattern/colour pairs.\n"
                "; Regenerate with `make assets`.\n;\n"
                "; %d art tiles at 0x00-0x%02X; the font is %d glyphs covering\n"
                "; ascii 0x%02X-0x%02X, uploaded three times by DINIT into the\n"
                "; white, cyan and light-green banks.\n\n"
                % (len(tiles), len(tiles) - 1, len(font), FONT_LO, FONT_HI))

        f.write("TILECNT EQU     %d\n" % len(tiles))
        f.write("DICECNT EQU     %d\n" % (len(dicetab) // 9))
        f.write("LOGOCNT EQU     %d\n" % len(logo))
        f.write("DICEBLK EQU     %d\n" % 12)
        f.write("FONTCNT EQU     %d\n" % len(font))
        f.write("FONTLO  EQU     0%02XH\n" % FONT_LO)
        f.write("\n; Named tile codes.\n")
        for name, _, _ in tiles:
            if not name[:2] in ("DW", "DY", "DC", "DK", "RW", "RC"):
                f.write("T%-7s EQU     0%02XH\n" % (name, names[name]))
        f.write("\n")
        f.write(block("TILEPAT", [p for _, p, _ in tiles],
                      "Art tile patterns, 8 bytes each."))
        f.write("\n")
        f.write(block("TILECOL", [c for _, _, c in tiles],
                      "Art tile colours: one fg<<4|bg per pixel row."))
        f.write("\n")
        f.write(block("FONTPAT", font,
                      "Font patterns, ascii 0x%02X-0x%02X. No colour table:"
                      " each bank is one ink,\n; so DINIT fills it."
                      % (FONT_LO, FONT_HI)))
        f.write("\n")
        f.write(block("DICETAB", [dicetab[i:i + 9]
                                  for i in range(0, len(dicetab), 9)],
                      "Nine tile codes per die face, in drawDie's order.\n"
                      "; Banks: +0 normal, +54 held(yellow), +108 blank,\n"
                      "; +117 roll tile, +144 fujitzee, +198 held+fujitzee."))

    print("wrote %s: %d tiles, %d font glyphs, %d dice entries"
          % (OUT, len(tiles), len(font), len(dicetab)))


if __name__ == "__main__":
    main()

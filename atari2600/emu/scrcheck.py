#!/usr/bin/env python3
"""scrcheck.py -- read a layout-ROM snapshot back, cell by cell.

Not "does the screenshot look right". The 48-pixel text block is decoded back
through the SAME 3x5 font the cartridge composes with, and every cell is
required to be the character tools/mklayout.py put there; then every card row's
INK COLOUR is required to be the one the potential mask implies, and each die's
bitmap the one tools/mkdice.py generates for the face and held state it was
staged with.

At 3x5 an 'S' and a '5' are one picture and so are 'O' and '0', which is why
this compares against the font's own rendering rather than trying to recognise
text: a cell matches if its three-by-five bit pattern is the pattern that
character renders to, and nothing else is consulted.

    python3 emu/scrcheck.py build/snap/a2600/0000.png
"""

import sys, os
from collections import Counter
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "tools"))
sys.path.insert(0, os.path.join(
    os.environ.get("FUJI_FIRMWARE", os.path.expanduser("~/Workspace/fujinet-firmware")),
    "pico/atari-2600/tools"))
import vcsfont                                                   # noqa: E402
import mklayout                                                  # noqa: E402

COLS, CELL_W, INK_H, CELL_H = 12, 4, 5, 6
CARD0, NCARD = 2, 15

# The NTSC colours disp.inc paints with, as MAME renders them.
WHITE, GREEN, CURSOR = "white", "green", "cursor"


def classify(rgb, bg):
    if rgb == bg:
        return None
    r, g, b = rgb
    if r > 200 and g > 200 and b > 200:
        return WHITE
    if g > r and g > b:
        return GREEN
    if r > 200 and g > 180 and b < 200:      # NTSC hue 1: yellow, not white
        return CURSOR
    return "?%d,%d,%d" % rgb


def main(path):
    im = Image.open(path).convert("RGB")
    W, H = im.size
    px = im.load()
    bg = Counter(px[x, y] for y in range(H) for x in range(W)).most_common(1)[0][0]

    ink = [(x, y) for y in range(H) for x in range(40, 140) if px[x, y] != bg
           and px[x, y] != (0, 0, 0)]
    if not ink:
        sys.exit("scrcheck: nothing on screen at all")
    x0 = min(x for x, _ in ink)
    y0 = min(y for _, y in ink)

    want = mklayout.screen()
    mask = mklayout.mask()
    bad = 0

    for r in range(CARD0 + NCARD):          # rows 0..16: the card and its head
        top = y0 + r * CELL_H
        got = []
        colours = set()
        for c in range(COLS):
            bits = []
            for ln in range(INK_H):
                v = 0
                for dx in range(3):
                    p = px[x0 + c * CELL_W + dx, top + ln]
                    if p != bg:
                        v |= 4 >> dx
                        colours.add(classify(p, bg))
                bits.append(v)
            got.append(tuple(bits))
        exp = [vcsfont.glyph(ch) for ch in want[r].ljust(COLS)]
        exp = [tuple(int(row.replace('.', '0').replace('#', '1'), 2)
                     if isinstance(row, str) else row for row in g) for g in exp]
        if got != exp:
            bad += 1
            print("row %2d: text differs" % r)
            for ln in range(INK_H):
                print("   got %s" % " ".join("%03d" % g[ln] for g in got))
                print("   exp %s" % " ".join("%03d" % e[ln] for e in exp))
                break
        if CARD0 <= r < CARD0 + NCARD:
            green = bool(mask & (1 << (16 - r)))
            colours.discard(None)
            wantc = {GREEN} if green else {WHITE}
            if r == CARD0 + 10:             # the cursor row overrides both
                wantc = {CURSOR}
            if colours and colours != wantc:
                bad += 1
                print("row %2d: ink %s, wanted %s" % (r, sorted(colours), sorted(wantc)))

    # The five dice, against the generator's own art.
    sys.path.insert(0, os.path.join(HERE, "..", "tools"))
    src = open(os.path.join(HERE, "..", "tools", "mkdice.py")).read()
    ns = {"__name__": "notmain"}
    exec(src, ns)
    # The band's top line is solid in BOTH variants -- an ordinary die's body
    # and a held die's bracket bar -- so the first row below the card where
    # all five die columns are lit is the top of the tray. Searching for "any
    # ink" instead finds the last line of row 16's text.
    dtop = None
    for y in range(y0 + 17 * CELL_H - 6, H):
        if all(px[x0 + i * 8 + dx, y] != bg
               for i in range(5) for dx in (0, 3, 6)):
            dtop = y
            break
    if dtop is None:
        sys.exit("scrcheck: no dice band")
    for i, (f, h) in enumerate(zip(mklayout.DICE, mklayout.HELD)):
        exp = ns["grid"](ns["FACES"][f - 1], h)
        for ln in range(ns["H"]):
            got = [px[x0 + i * 8 + dx, dtop + ln] != bg for dx in range(7)]
            if got != exp[ln]:
                bad += 1
                print("die %d line %2d: got %s exp %s" % (
                    i, ln,
                    "".join("#" if c else "." for c in got),
                    "".join("#" if c else "." for c in exp[ln])))
                break

    if bad:
        sys.exit("scrcheck: %d checks FAILED" % bad)
    print("scrcheck: %d text rows and %d dice agree with the picture"
          % (CARD0 + NCARD, len(mklayout.DICE)))


if __name__ == "__main__":
    main(sys.argv[1])

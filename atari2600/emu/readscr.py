#!/usr/bin/env python3
"""readscr.py -- decode a snapshot back into the twenty-one text rows.

Through the SAME 3x5 font the cartridge composes with, because at that size an
'S' and a '5' are one picture and so are 'O' and '0'. A cell is whatever
character renders to its bit pattern, and nothing else is consulted.
"""
import sys, os
from collections import Counter
from PIL import Image

sys.path.insert(0, os.path.join(
    os.environ.get("FUJI_FIRMWARE", os.path.expanduser("~/Workspace/fujinet-firmware")),
    "pico/atari-2600/tools"))
import vcsfont                                                   # noqa: E402

COLS, CELL_W, INK_H, CELL_H, ROWS = 12, 4, 5, 6, 21

REV = {}
for ch in vcsfont._F:
    g = tuple(int(r.replace('.', '0').replace('#', '1'), 2)
              for r in vcsfont._F[ch])
    REV.setdefault(g, ch)


def rows(path):
    im = Image.open(path).convert("RGB")
    W, H = im.size
    px = im.load()
    bg = Counter(px[x, y] for y in range(H) for x in range(W)).most_common(1)[0][0]
    ink = [(x, y) for y in range(H) for x in range(40, 140)
           if px[x, y] != bg and px[x, y] != (0, 0, 0)]
    if not ink:
        return [], None
    x0, y0 = min(x for x, _ in ink), min(y for _, y in ink)
    out = []
    for r in range(ROWS):
        line = ""
        for c in range(COLS):
            g = []
            for ln in range(INK_H):
                v = 0
                for dx in range(3):
                    yy = y0 + r * CELL_H + ln
                    if yy < H and px[x0 + c * CELL_W + dx, yy] != bg:
                        v |= 4 >> dx
                g.append(v)
            line += REV.get(tuple(g), "?")
        out.append(line)
    return out, (x0, y0)


if __name__ == "__main__":
    rs, org = rows(sys.argv[1])
    for i, l in enumerate(rs):
        print("%2d |%s|" % (i, l))

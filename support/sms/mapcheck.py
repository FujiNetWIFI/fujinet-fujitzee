#!/usr/bin/env python3
"""mapcheck.py -- fail the build unless the cartridge will run this image
on the Sega mapper.

    mapcheck.py image.sms

The FujiNet SMS cartridge looks an image up in MAME's software list by
CRC-32 and, failing that, guesses its mapper the way MAME's sega8 slot does
(fujinet-firmware pico/sms/firmware/src/smsmap.c smsmap_heuristic(),
transcribed below): it counts LD (nn),A instructions -- the byte $32 and a
little-endian address -- in the first 32K, by target. Fujitzee is an image
over 32K whose art is paged with LD ($FFFF),A (src/sms/vdp.asm mapperInit),
and a single stray $32 $FE $3F anywhere in code or data would make it a 4 PAK
cartridge, which maps every slot to bank 0 at power-on.
"""

import sys


def heuristic(img):
    kind = "rom"
    if len(img) >= 0x8000:
        count = {a: 0 for a in ("0002", "8000", "a000", "ffff", "3ffe", "4000", "6000")}
        i = 0
        while i < 0x8000:
            if img[i] == 0x32:
                lo = img[i + 1] if i + 1 < len(img) else 0
                hi = img[i + 2] if i + 2 < len(img) else 0
                addr = lo | hi << 8
                key = {0xFFFF: "ffff", 0x0002: "0002", 0x0003: "0002", 0x0004: "0002",
                       0x8000: "8000", 0xA000: "a000", 0x3FFE: "3ffe", 0x4000: "4000",
                       0x6000: "6000"}.get(addr)
                if key:
                    count[key] += 1
                    i += 3
                    continue
            i += 1
        c = count
        if len(img) > 0x10000 and (c["0002"] > c["ffff"] + 2 or (c["0002"] > 0 and c["ffff"] == 0)):
            kind = "zemina"
        elif c["8000"] > c["ffff"] + 2 or (c["8000"] > 0 and c["ffff"] == 0):
            kind = "codemasters"
        elif c["a000"] > c["ffff"] + 2 or (c["a000"] > 0 and c["ffff"] == 0):
            kind = "korean"
        elif c["3ffe"] > c["ffff"] + 2 or c["3ffe"] > 0:
            kind = "4pak"
        elif c["4000"] > 0 and c["6000"] > 0 and c["8000"] > 0 and c["a000"] > 0:
            kind = "janggun"
        return kind, c
    return kind, {}


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    img = open(sys.argv[1], "rb").read()
    kind, counts = heuristic(img)
    seen = ", ".join("$%s x%d" % (k.upper(), v) for k, v in counts.items() if v)
    if kind != "rom":
        sys.exit("mapcheck: %s would run on the %s mapper (LD (nn),A by target: %s)"
                 % (sys.argv[1], kind, seen or "none"))
    print("mapcheck: %s runs on the Sega mapper (%s)" % (sys.argv[1], seen or "no LD (nn),A"))


if __name__ == "__main__":
    main()

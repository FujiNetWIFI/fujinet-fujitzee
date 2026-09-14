#!/usr/bin/env python3
"""checksize.py -- per-module ROM budget from the zmac listing.

Reads the MB_* fence labels out of the symbol table and reports how many
bytes each module contributes to the 30,720-byte client window, so a module
that quietly doubles is visible at build time rather than when the image stops
fitting.

Usage: checksize.py build/fujitzee.lst
"""
import re
import sys

ORG = 0x8000
ROM_TOP = 0x8000 + 0x7800       # console address of the mailbox pages
WARN_SPARE = 1024


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__.strip())
    text = open(sys.argv[1]).read()

    # zmac's symbol table lists "name  value" in columns; fences are MB_*.
    fences = {}
    for name, val in re.findall(r"\b(mb_[a-z0-9_]+)\s+(?:=)?\s*([0-9a-f]{4})\b",
                                text, re.I):
        fences.setdefault(name.upper(), int(val, 16))
    if not fences:
        print("checksize: no MB_* fences found", file=sys.stderr)
        return 0

    ordered = sorted(fences.items(), key=lambda kv: kv[1])
    print("  module            bytes")
    for i, (name, addr) in enumerate(ordered):
        end = ordered[i + 1][1] if i + 1 < len(ordered) else None
        if end is None:
            continue
        print("  %-16s %6d" % (name[3:].lower(), end - addr))

    total = ordered[-1][1] - ORG
    spare = (ROM_TOP - ORG) - total
    print("  %-16s %6d of %d, %d spare" %
          ("TOTAL", total, ROM_TOP - ORG, spare))
    if spare < 0:
        print("checksize: over the ROM top by %d bytes" % -spare,
              file=sys.stderr)
        return 1
    if spare < WARN_SPARE:
        print("checksize: only %d bytes spare" % spare, file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())

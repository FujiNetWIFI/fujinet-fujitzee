#!/usr/bin/env python3
"""checkart.py -- the dice art must be identical in every bank that draws it.

The kernel reads a die through `lda (DPTRn),y`, and DPTRn's HIGH byte is
written once by DINIT and never again. The network bank is entered at DFRAME2
and never runs DINIT, so it inherits the pointers the game bank staged -- which
means its copy of the art has to be at the same address, holding the same
bytes. If it is not, a poll draws the dice out of whatever that bank happens to
have at $1600, which is its own code.

Nothing about that failure looks like a pointer problem: the card is right, the
frame is right, and the tray fills with garbage for the fifth of a second a
poll takes. So it is a build gate, not a thing to remember.

    python3 tools/checkart.py build/fujitzee.bin
"""

import sys

BANK_LEN = 0x800
ART_LEN = 144


def art_addr():
    """FZART, read out of fzdefs.inc rather than repeated here.

    It was repeated here once, and when the reserved page moved this tool went
    on comparing 144 bytes of whatever was at the old address -- in four banks
    at once, so they agreed, and it passed."""
    import os, re
    d = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                     "..", "src", "fzdefs.inc")
    m = re.search(r"^FZART\s+EQU\s+\$([0-9A-Fa-f]+)", open(d).read(), re.M)
    if not m:
        sys.exit("checkart: no FZART in fzdefs.inc")
    return int(m.group(1), 16)
DRAWS_DICE = {1: "fzgame", 2: "fznet", 5: "fzcard", 6: "fzchrome"}


def main(path):
    FZART = art_addr()
    img = open(path, "rb").read()
    nbanks = len(img) // BANK_LEN - 1       # the last part is the fixed half
    off = FZART - 0x1000
    ref = None
    for bank, name in sorted(DRAWS_DICE.items()):
        if bank >= nbanks:
            sys.exit("checkart: bank %d (%s) is past the end of %s"
                     % (bank, name, path))
        art = img[bank * BANK_LEN + off: bank * BANK_LEN + off + ART_LEN]
        if len(set(art)) == 1:
            sys.exit("checkart: bank %d (%s) has no art at $%04X -- is the "
                     "ORG missing?" % (bank, name, FZART))
        if ref is None:
            ref, refname = art, name
        elif art != ref:
            bad = next(i for i in range(ART_LEN) if art[i] != ref[i])
            sys.exit("checkart: %s and %s differ at art byte %d ($%02X vs "
                     "$%02X)" % (refname, name, bad, ref[bad], art[bad]))
    print("checkart: %d banks carry the same %d bytes of dice art at $%04X"
          % (len(DRAWS_DICE), ART_LEN, FZART))


if __name__ == "__main__":
    main(sys.argv[1])

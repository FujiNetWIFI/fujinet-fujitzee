#!/usr/bin/env python3
"""endgame.py -- game over in seconds, on the private rig's server.

    endgame.py [--server URL] [--table ID] [--name NAME] [--after S] [--winners N]

Waits for TABLE's game to be under way (round 1-13), AFTER seconds more,
then asks for /state as NAME (the O2's own player, so nobody joins) with
skipToEnd=N: the server plays every turn out at once and gives the first N
seats 500 in Chance. Only a server without GO_PROD honours it -- never the
production one.
"""
import argparse
import sys
import time
import urllib.request


def get(url):
    with urllib.request.urlopen(url, timeout=5) as r:
        return r.read()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--server", default="http://localhost:8091/")
    ap.add_argument("--table", default="ai2")
    ap.add_argument("--name", default="o2")
    ap.add_argument("--after", type=float, default=5)
    ap.add_argument("--winners", type=int, default=1)
    a = ap.parse_args()
    base = "%sstate?table=%s&player=%s&bin=1" % (a.server, a.table, a.name)
    while True:
        b = get(base)
        if 1 <= b[63] <= 13:
            break
        time.sleep(0.5)
    print("round %d: skipping to the end in %g s" % (b[63], a.after), flush=True)
    time.sleep(a.after)
    b = get(base + "&skipToEnd=%d" % a.winners)
    print("round %d: %s" % (b[63], b[22:63].split(b"\0")[0].decode()), flush=True)


if __name__ == "__main__":
    sys.exit(main())

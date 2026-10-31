#!/usr/bin/env python3
"""player.py -- a second human for the rig: plays Fujitzee over HTTP.

    player.py [--server URL] [--table ID] [--name NAME] [--delay S]
              [--games N]

Joins TABLE as NAME, says it's ready in the lobby, and on its turn, DELAY
seconds in, rolls once more (the server rolls the first) and scores the
slot that pays most. Reads the binary state, the same bytes the O2 does
(src/equ.inc). Prints a line per change of round or turn; leaves after N
games.
"""
import argparse
import sys
import time
import urllib.request


def get(server, path, table, name):
    url = "%s%s?table=%s&player=%s&bin=1" % (server, path, table, name)
    with urllib.request.urlopen(url, timeout=5) as r:
        return r.read()


def cstr(b):
    return b.split(b"\0")[0].decode()


def parse(b):
    n = b[0]
    seats = []
    for k in range(n):
        rec = b[95 + 42 * k:95 + 42 * (k + 1)]
        scores = [rec[10 + 2 * i] | rec[11 + 2 * i] << 8 for i in range(16)]
        seats.append({"name": cstr(rec[0:9]), "scores": scores})
    valid = [v - 256 if v > 127 else v for v in b[80:95]]
    return {"prompt": cstr(b[22:63]), "round": b[63], "rolls": b[64],
            "active": b[65], "dice": cstr(b[68:74]), "valid": valid, "seats": seats}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--server", default="http://localhost:8091/")
    ap.add_argument("--table", default="kit")
    ap.add_argument("--name", default="py")
    ap.add_argument("--delay", type=float, default=1.5)
    ap.add_argument("--games", type=int, default=1)
    a = ap.parse_args()
    seen, turn_at, games, last_round, rolled = None, None, 0, None, False
    while True:
        s = parse(get(a.server, "state", a.table, a.name))
        key = (s["round"], s["active"])
        if key != seen:
            print("round %d active %d %s %s" % (s["round"], s["active"],
                  [x["name"] for x in s["seats"]], s["prompt"]), flush=True)
            seen, turn_at, rolled = key, time.time(), False
        if last_round == 99 and s["round"] != 99:
            games += 1
            if games >= a.games:
                get(a.server, "leave", a.table, a.name)
                print("left", flush=True)
                return
        last_round = s["round"]
        if s["round"] == 0 and s["seats"] and s["seats"][0]["scores"][0] != 1:
            get(a.server, "ready", a.table, a.name)
            print("  ready", flush=True)
        elif 1 <= s["round"] <= 13 and s["active"] == 0 and time.time() - turn_at >= a.delay:
            if not rolled and s["rolls"] > 0:
                get(a.server, "roll/11111", a.table, a.name)
                print("  roll", flush=True)
                rolled = True
            else:
                slots = [i for i, v in enumerate(s["valid"]) if v >= 0 and i not in (6, 7)]
                if slots:
                    best = max(slots, key=lambda i: s["valid"][i])
                    get(a.server, "score/%d" % best, a.table, a.name)
                    print("  score %d (%d)" % (best, s["valid"][best]), flush=True)
                    seen = None
        time.sleep(0.5)


if __name__ == "__main__":
    sys.exit(main())

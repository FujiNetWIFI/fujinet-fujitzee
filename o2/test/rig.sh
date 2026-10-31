#!/usr/bin/env bash
# rig.sh -- a private server and FujiNet for a live run, then a scenario.
#
#   test/rig.sh SCENARIO OUTDIR [SECS [IMAGE]]
#
# Builds and starts the Fujitzee server (~/Workspace/servers/fujinet-game-
# system/fujitzee/server, SERVER_SRC=) on FZ_PORT (8091; without GO_PROD it
# never posts to the lobby) and a private copy of fujinet-pc-rs232 on BoIP
# port BOIP (19974), its name appkey seeded with NAME (O2; NAME= for none:
# the boot asks) and its room appkey with ROOM ("<server>?table=<id>"). Then runs tools/o2sim.py on
# SCENARIO against it, throttled (the server's clocks are wall time).
# Kills only what it started. Never the production server, never the
# everyday fujinet-pc on 9995.
#
# The image must point at the same server: build with
# ENDPOINT=http://localhost:8091/ (make rig does). HOLD=1 leaves the rig up
# after the scenario, for a window (./run.sh), until ^C. PLAYER="--table kit
# ..." plays a second human (test/player.py) alongside; build/rig/player.log.
# ENDGAME="--table ai2 ..." skips the game to its end (test/endgame.py).
set -euo pipefail
cd "$(dirname "$0")/.."
HERE=$(pwd)
SCEN=$1
OUT=$2
SECS=${3:-120}
FZ_PORT=${FZ_PORT:-8091}
BOIP=${BOIP:-19974}
NAME=${NAME-O2}
SERVER_SRC=${SERVER_SRC:-$HOME/Workspace/servers/fujinet-game-system/fujitzee/server}
FNPC_DIST=${FNPC_DIST:-$HOME/Workspace/fujinet-pc-rs232/build/dist}
RIG="${RIG:-$HERE/build/rig}"

pids=()
cleanup() {
    for p in "${pids[@]}"; do kill "$p" 2>/dev/null || true; done
}
trap cleanup EXIT

# The image has to talk to this rig's server: one built for any other
# (make, not make rig) would play the live server from here.
IMAGE=${4:-build/fujitzee.bin}
if ! grep -aq "http://localhost:$FZ_PORT/" "$IMAGE"; then
    echo "rig: $IMAGE isn't built for http://localhost:$FZ_PORT/ (make rig)" >&2
    exit 1
fi

# (a rig just ended may still be letting go of them: wait a few seconds)
for port in "$FZ_PORT" "$BOIP" "$((BOIP - 10000))"; do
    for _ in 1 2 3 4 5 6 7 8 9 10; do
        ss -ltn | grep -q ":$port " || break
        sleep 0.5
    done
    if ss -ltn | grep -q ":$port "; then echo "rig: port $port is taken" >&2; exit 1; fi
done

mkdir -p "$RIG"
( cd "$SERVER_SRC" && go build -o "$RIG/fzserver" . )
( cd "$RIG" && PORT=$FZ_PORT exec ./fzserver ) > "$RIG/server.log" 2>&1 &
pids+=($!)

rm -rf "$RIG/fn"; mkdir -p "$RIG/fn"
# the binary by link, the rest copied: its config is ours to change
ln -s "$FNPC_DIST/fujinet" "$RIG/fn/fujinet"
cp -a "$FNPC_DIST/data" "$FNPC_DIST/fnconfig.ini" "$RIG/fn"/
mkdir -p "$RIG/fn/SD/FujiNet"
python3 - "$RIG/fn/fnconfig.ini" "$BOIP" <<'PY'
import re, sys
s = open(sys.argv[1]).read()
s = re.sub(r"(\[BOIP\][^\[]*?\bport=)\d*", r"\g<1>" + sys.argv[2], s, flags=re.S)
s = re.sub(r"(\[BOIP\][^\[]*?\benabled=)\d*", r"\g<1>1", s, flags=re.S)
open(sys.argv[1], "w").write(s)
PY
[ -n "$NAME" ] && printf '%s' "$NAME" > "$RIG/fn/SD/FujiNet/00010100.key"
[ -n "${ROOM:-}" ] && printf '%s' "$ROOM" > "$RIG/fn/SD/FujiNet/00010103.key"
( cd "$RIG/fn" && exec ./fujinet -u http://127.0.0.1:$((BOIP - 10000)) -c fnconfig.ini -s SD ) \
    > "$RIG/fn.log" 2>&1 &
pids+=($!)
for _ in 1 2 3 4 5 6 7 8 9 10; do
    ss -ltn | grep -q ":$BOIP " && ss -ltn | grep -q ":$FZ_PORT " && break
    sleep 0.5
done
ss -ltn | grep -q ":$BOIP " || { echo "rig: no BoIP listener on $BOIP" >&2; exit 1; }
ss -ltn | grep -q ":$FZ_PORT " || { echo "rig: no server on $FZ_PORT" >&2; exit 1; }

# PLAYER="args": a second human (test/player.py) at the same server.
if [ -n "${PLAYER:-}" ]; then
    # shellcheck disable=SC2086
    python3 test/player.py --server "http://localhost:$FZ_PORT/" $PLAYER \
        > "$RIG/player.log" 2>&1 &
    pids+=($!)
fi
# ENDGAME="args": game over in seconds (test/endgame.py, skipToEnd).
if [ -n "${ENDGAME:-}" ]; then
    # shellcheck disable=SC2086
    python3 test/endgame.py --server "http://localhost:$FZ_PORT/" $ENDGAME \
        > "$RIG/endgame.log" 2>&1 &
    pids+=($!)
fi

O2SIM_FUJINET="127.0.0.1:$BOIP" O2SIM_SECONDS=$SECS \
    python3 tools/o2sim.py "$SCEN" "$OUT" "${@:4}"
if [ -n "${HOLD:-}" ]; then
    echo "rig: up (server $FZ_PORT, BoIP $BOIP); ^C ends it"
    wait
fi

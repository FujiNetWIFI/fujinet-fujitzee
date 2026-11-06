#!/usr/bin/env bash
# rig.sh -- a private server and FujiNet for a live run, then a scenario.
#
#   test/rig.sh SCENARIO OUTDIR [SECS]
#
# Builds the image for the private server into build/rigimg (never over
# build/fz.st2, which run.sh boots against production), starts the Fujitzee
# server (no GO_PROD: it posts nothing to the lobby) on FZ_PORT (8097) and a
# private copy of fujinet-pc-rs232 on BoIP port BOIP (19976; web UI on
# BOIP - 10000) with the name appkey NAME seeded, then runs tools/s2sim.py on
# SCENARIO, throttled (the server's clocks are wall time). Kills only what it
# started. Never the production server, never the everyday fujinet-pc on
# 9995. The copied fujinet-pc config loses its cloud sections (tokens).
set -euo pipefail
cd "$(dirname "$0")/.."
HERE=$(pwd)
SCEN=$1
OUT=$2
SECS=${3:-120}
FZ_PORT=${FZ_PORT:-8097}
BOIP=${BOIP:-19976}
NAME=${NAME-S2}
SERVER_SRC=${SERVER_SRC:-$HOME/Workspace/servers/fujinet-game-system/fujitzee/server}
FNPC_DIST=${FNPC_DIST:-$HOME/Workspace/fujinet-pc-rs232/build/dist}
RIG="$HERE/build/rig"

for p in $FZ_PORT $BOIP $((BOIP - 10000)); do
    if ss -ltn | grep -q ":$p "; then echo "rig: port $p is taken" >&2; exit 1; fi
done
pids=()
cleanup() {
    for p in "${pids[@]}"; do kill -9 "$p" 2>/dev/null || true; done
}
trap cleanup EXIT

mkdir -p "$RIG"
ENDPOINT="http://127.0.0.1:$FZ_PORT/" OUT=build/rigimg ./build.sh > "$RIG/build.log"
( cd "$SERVER_SRC" && go build -o "$RIG/fzserver" . )
( cd "$SERVER_SRC" && PORT=$FZ_PORT exec "$RIG/fzserver" ) > "$RIG/server.log" 2>&1 &
pids+=($!)

rm -rf "$RIG/fn"; mkdir -p "$RIG/fn"
ln -s "$FNPC_DIST/fujinet" "$RIG/fn/fujinet"
cp -a "$FNPC_DIST/data" "$RIG/fn"/
python3 - "$FNPC_DIST/fnconfig.ini" "$RIG/fn/fnconfig.ini" "$BOIP" <<'PY'
import re, sys
out, skip = [], False
for line in open(sys.argv[1]).read().splitlines():
    m = re.match(r"\[(.*)\]", line)
    if m:
        skip = m.group(1) in ("GoogleDrive", "S3", "OneDrive")
    if not skip:
        out.append(line)
s = "\n".join(out) + "\n"
s = re.sub(r"(\[BOIP\][^\[]*?\bport=)\d*", r"\g<1>" + sys.argv[3], s, flags=re.S)
s = re.sub(r"(\[BOIP\][^\[]*?\benabled=)\d*", r"\g<1>1", s, flags=re.S)
open(sys.argv[2], "w").write(s)
PY
mkdir -p "$RIG/fn/SD/FujiNet"
[ -n "$NAME" ] && printf '%s' "$NAME" > "$RIG/fn/SD/FujiNet/00010100.key"
( cd "$RIG/fn" && exec ./fujinet -u http://127.0.0.1:$((BOIP - 10000)) -c fnconfig.ini -s SD ) \
    > "$RIG/fn.log" 2>&1 &
pids+=($!)
for _ in $(seq 20); do ss -ltn | grep -q ":$BOIP " && break; sleep 0.5; done
if ! ss -ltn | grep -q ":$BOIP "; then echo "rig: no BoIP listener on $BOIP" >&2; exit 1; fi
for _ in $(seq 20); do ss -ltn | grep -q ":$FZ_PORT " && break; sleep 0.5; done

S2SIM_FUJINET="127.0.0.1:$BOIP" S2SIM_SECONDS=$SECS S2SIM_THROTTLE=1 \
    python3 tools/s2sim.py "$SCEN" "$OUT" "$HERE/build/rigimg/fz.st2"

#!/usr/bin/env bash
# run.sh -- run a Fujitzee client ROM in a FujiNet-patched MAME.
#
#   ./run.sh [rom] [lua-script]
#
# With a script it runs headless and exits; without one it opens a window.
# `rom` is a basename in build/ and defaults to fujitzee.
#
# Five environment facts this wraps, each of which costs time to rediscover:
#   - MAME must run FROM ITS OWN TREE or -autoboot_script is silently ignored.
#   - SDL_VIDEODRIVER=dummy is required wherever there is no DISPLAY, because
#     SDL comes up before the video backend is chosen.
#   - fujinet-pc's BoIP listener takes ONE client, so a MAME left running
#     starves the next run and the symptom is a hang, not an error.
#   - THROTTLED IS NOT A PERFORMANCE CHOICE. The server's start countdown and
#     its move clock are WALL CLOCK, so an unthrottled run sits at "starting
#     in 2" forever and never gets a turn. Set FAST=1 only for a screenshot
#     of something that waits on nothing -- the layout ROM, for instance.
#   - The layout ROM has no mailbox in it at all, so it runs in the STOCK
#     cart slot; SLOT=a26_2k_4k is the default for it.

set -euo pipefail
cd "$(dirname "$0")"
HERE=$(pwd)

ROM=${1:-fujitzee}
SCRIPT=${2:-}
MAME=${MAME:-$HOME/Workspace/mame}

if [ -z "${SLOT:-}" ]; then
    case "$ROM" in
        layout) SLOT=a26_2k_4k ;;
        *)      SLOT=fujinet ;;
    esac
fi

pkill -f "mame a2600" 2>/dev/null || true

args=(a2600 -cartslot "$SLOT" -cart "$HERE/build/$ROM.bin"
      -snapshot_directory "${SNAP:-$HERE/build/snap}")

if [ -n "$SCRIPT" ]; then
    args+=(-autoboot_script "$HERE/emu/$SCRIPT.lua"
           -video none -sound none
           -seconds_to_run "${SECS:-20}")
    [ -n "${FAST:-}" ] && args+=(-nothrottle)
fi

[ -n "${DISPLAY:-}" ] || export SDL_VIDEODRIVER=dummy
export FUJINET_TCP="${FUJINET_TCP:-127.0.0.1:9995}"
export A2600_EMU="$HERE/emu"

cd "$MAME"
exec ./mame "${args[@]}"

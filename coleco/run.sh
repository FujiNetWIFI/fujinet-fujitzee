#!/usr/bin/env bash
# run.sh -- run the client in the patched MAME against a live fujinet-pc.
#
#   ./run.sh                     interactive
#   ./run.sh --headless "TEXT"   headless; print the screen, PASS/FAIL on TEXT
#   ./run.sh --drive "TEXT"      headless; play the controller through DRIVE=,
#                                then print the screen and verdict
#
# Needs a MAME tree with the FujiNet cartridge device grafted in
# (fujinet-firmware/pico/coleco/emu/apply.sh) and, for anything past the
# splash, a fujinet-pc BoIP listener on FUJINET_TCP.
#
# The emulated clock is THROTTLED to real time by default, which matters more
# here than anywhere else in this family: the game server is a real host taking
# real seconds to move its bots, and MAME left unthrottled runs at 1500-3000%,
# so an "SCREEN_AT=40" sample lands about two real seconds in and every turn
# still belongs to whoever had it at boot. Set NOTHROTTLE=1 for tests that
# never leave the console.
#
# Env: MAME_DIR, FUJI_PICO, FUJINET_TCP (default 127.0.0.1:9995),
#      FUJINET_DEBUG=1 to log every mailbox transaction,
#      SCREEN_AT=<seconds> when to sample the screen headlessly.

set -euo pipefail
cd "$(dirname "$0")"

MAME_DIR="${MAME_DIR:-$HOME/Workspace/mame}"
FUJI_PICO="${FUJI_PICO:-$HOME/Workspace/fujinet-firmware/pico}"
export FUJINET_TCP="${FUJINET_TCP:-127.0.0.1:9995}"

BIN="$PWD/build/fujitzee.bin"
[ -f "$BIN" ] || ./build.sh

SCRIPT="$PWD/emu/screen.lua"   # this port's own: the charset is banked
[ "${1:-}" = "--drive" ] && SCRIPT="$PWD/emu/drive.lua"

# MAME resolves rompath, pluginspath and its Lua search path against its OWN
# working directory: run it from anywhere else and -autoboot_script is
# silently ignored, with no error and no output. Everything handed to it is
# absolute for the same reason.
cd "$MAME_DIR"

if [ "${1:-}" = "--headless" ] || [ "${1:-}" = "--drive" ]; then
    [ -n "${2:-}" ] && export SCREEN_EXPECT="$2"
    secs=$(( ${SCREEN_AT:-3} + 8 ))
    throttle="-nothrottle"
    [ -z "${NOTHROTTLE:-}" ] && throttle=""
    # shellcheck disable=SC2086
    exec ./mame coleco -cartslot fujinet -cart "$BIN" \
        -video none -sound none $throttle -seconds_to_run "$secs" \
        -autoboot_script "$SCRIPT"
fi

exec ./mame coleco -cartslot fujinet -cart "$BIN" -window -nomax "$@"

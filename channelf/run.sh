#!/usr/bin/env bash
# run.sh -- run the client in the patched MAME against a live fujinet-pc.
#
# Needs the MAME tree with the Channel F FujiNet cart device grafted in
# (fujinet-firmware/pico/channelf/emu/apply.sh) and a fujinet-pc BoIP listener
# on FUJINET_TCP (default 127.0.0.1:9995).
#
# MAME must run FROM ITS OWN TREE or its relative paths resolve against the
# wrong place, so every path below is absolute.
#
# THROTTLED, ALWAYS. The server's start countdown and its move clock are WALL
# CLOCK, so an unthrottled run sits at "starting in 2" forever because barely
# any real time passes.
set -euo pipefail
cd "$(dirname "$0")"
HERE="$PWD"

MAME_DIR="${MAME_DIR:-$HOME/Workspace/mame}"
export FUJINET_TCP="${FUJINET_TCP:-127.0.0.1:9995}"

[ -f build/fujitzee.bin ] || ./build.sh

# MAME writes its own per-machine config here rather than into $HOME. The
# directory is deliberately not committed: the battleship port's cfg carries an
# absolute cart path that is only right on the machine that generated it, and
# nothing here needs one -- the cart is passed on the command line.
mkdir -p cfg build/snap

args=(channelf -bios sl31253 -cartslot fujinet -cart "$HERE/build/fujitzee.bin"
      -snapshot_directory "$HERE/build/snap" -cfg_directory "$HERE/cfg"
      -window -nomax)

cd "$MAME_DIR"
exec ./mame "${args[@]}" "$@"

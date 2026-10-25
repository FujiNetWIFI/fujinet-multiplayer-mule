#!/usr/bin/env bash
# run.sh -- play it: MAME's RCA Studio II with the FujiNet cart, in a window,
# against a live fujinet-pc and the deployed server.
#
#   ./run.sh                       rebuild build/mule.st2 for ENDPOINT, run it
#   ./run.sh path/to/image.st2     another image as it is (more MAME options after it)
#   ENDPOINT=http://host:port ./run.sh   another server (default: the deployed one)
#
# Every run without an image rebuilds, so it is never an AUTOPLAY or a test
# rig's build. The server is ENDPOINT unless that FujiNet's lobby room appkey
# names one: the room wins (and the table chosen is saved there). The name
# comes from its lobby name appkey; without it the game asks.
#
# Needs ~/Workspace/mame-studio2 with the cart's protocol 2 grafted in
# (fujinet-firmware PR #2187, pico/studio2/emu/apply.sh; the binary
# studio2-p2) and a fujinet-pc BoIP listener on FUJINET_TCP (default
# 127.0.0.1:9995).
#
# Controls (MAME's defaults): the numeric pad is keypad B (8 up, 2 down, 4
# left, 6 right, 7 9 1 3 the diagonals, 5 the button, 0 the menu); keypad A
# is Q W E / A S D / Z X C, left Alt its 0.
#
# MAME runs from its own tree, so every path here is absolute.
set -euo pipefail
cd "$(dirname "$0")"
HERE="$PWD"

MAME_DIR="${MAME_DIR:-$HOME/Workspace/mame-studio2}"
MAME_BIN="${MAME_BIN:-studio2-p2}"
ROMS="${ROMS:-$HOME/Workspace/mame/roms}"
export FUJINET_TCP="${FUJINET_TCP:-127.0.0.1:9995}"
ENDPOINT="${ENDPOINT:-http://fujinet.online:9900}"

if [ $# -eq 0 ]; then
    mkdir -p build
    ENDPOINT="$ENDPOINT" AUTOPLAY=0 DEBUG=0 ./build.sh > build/run.log 2>&1 \
        || { cat build/run.log >&2; exit 1; }
    IMAGE="$HERE/build/mule.st2"
else
    IMAGE="$1"
    [ "${IMAGE#/}" = "$IMAGE" ] && IMAGE="$HERE/$IMAGE"
fi
[ -f "$IMAGE" ] || { echo "run.sh: no $IMAGE" >&2; exit 1; }
[ -x "$MAME_DIR/$MAME_BIN" ] || { echo "run.sh: no MAME at $MAME_DIR/$MAME_BIN" >&2; exit 1; }
export FUJINET_BOOT="$IMAGE"

mkdir -p "$HERE/build/snap" "$HERE/cfg"
cd "$MAME_DIR"
exec "./$MAME_BIN" studio2 -rompath "$ROMS" \
    -snapshot_directory "$HERE/build/snap" -cfg_directory "$HERE/cfg" \
    -skip_gameinfo -window -nomax "${@:2}"

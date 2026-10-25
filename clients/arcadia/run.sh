#!/usr/bin/env bash
# run.sh -- play it: MAME's Arcadia 2001 with the FujiNet cart, in a window,
# against a live fujinet-pc and the deployed server.
#
#   ./run.sh                       rebuild build/mule.bin for ENDPOINT, run it
#   ./run.sh path/to/image.bin     another image as it is (more MAME options after it)
#   ENDPOINT=http://host:port ./run.sh   another server (default: the deployed one)
#
# Every run without an image rebuilds (a second), so it is never an AUTOPLAY
# or a test rig's build. The server is ENDPOINT unless that FujiNet's lobby
# room appkey names one: the room wins (and the table chosen is saved there).
# The name comes from its lobby name appkey; without it the game asks.
#
# Needs ~/Workspace/mame with the Arcadia FujiNet cart device grafted in
# (fujinet-firmware PR #2180, pico/arcadia/emu/apply.sh: cart protocol 2,
# the APPBANK pages, cart RAM and the tick) and a fujinet-pc BoIP listener on
# FUJINET_TCP (default 127.0.0.1:9995).
#
# Controls (MAME's defaults): the arrow keys are the disc, keypad 2 on the
# numeric pad is FIRE, the numeric pad is the keypad (- Clear, + Enter);
# the console: 1 Start (end your turn), O Option (leave the table), 5 Select
# (sound off and on).
#
# MAME runs from its own tree, so every path here is absolute.
set -euo pipefail
cd "$(dirname "$0")"
HERE="$PWD"

MAME_DIR="${MAME_DIR:-$HOME/Workspace/mame}"
export FUJINET_TCP="${FUJINET_TCP:-127.0.0.1:9995}"
ENDPOINT="${ENDPOINT:-http://fujinet.online:9900}"

if [ $# -eq 0 ]; then
    mkdir -p build
    ENDPOINT="$ENDPOINT" AUTOPLAY=0 DEBUG=0 ./build.sh > build/run.log 2>&1 \
        || { cat build/run.log >&2; exit 1; }
    IMAGE="$HERE/build/mule.bin"
else
    IMAGE="$1"
    [ "${IMAGE#/}" = "$IMAGE" ] && IMAGE="$HERE/$IMAGE"
fi
[ -f "$IMAGE" ] || { echo "run.sh: no $IMAGE" >&2; exit 1; }
[ -x "$MAME_DIR/mame" ] || { echo "run.sh: no MAME at $MAME_DIR" >&2; exit 1; }

mkdir -p "$HERE/build/snap" "$HERE/cfg"
cd "$MAME_DIR"
exec ./mame arcadia -cartslot fujinet -cart "$IMAGE" \
    -snapshot_directory "$HERE/build/snap" -cfg_directory "$HERE/cfg" \
    -skip_gameinfo -window -nomax "${@:2}"

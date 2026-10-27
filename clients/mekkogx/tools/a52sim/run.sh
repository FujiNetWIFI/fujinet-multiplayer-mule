#!/usr/bin/env bash
# run.sh -- play it: MAME's Atari 5200 with the FujiNet 5200 cart, in a
# window, against a live fujinet-pc and the deployed server.
#
#   tools/a52sim/run.sh                      rebuild r2r/atari5200/mule.a52 for ENDPOINT, run it
#   tools/a52sim/run.sh path/to/image.a52    another image as it is (more MAME options after it)
#   tools/a52sim/run.sh - [MAME options]      rebuild, run it with those options
#   ENDPOINT=http://host:port/ tools/a52sim/run.sh   another server (default: the deployed one)
#   SYSTEM=a5200a tools/a52sim/run.sh        the 2-port console
#
# Every run without an image rebuilds (make atari5200), so it is never an
# AUTOPLAY or a test rig's build (those build in copies: rig.sh). The
# server is ENDPOINT unless that FujiNet's lobby room appkey names one: the
# room wins (and the table chosen is saved there). The name comes from its
# lobby name appkey; without it the game asks.
#
# Needs ~/Workspace/mame-a5200 with the 5200 FujiNet cart grafted in
# (fujinet-firmware pico/atari-5200/emu/apply.sh; the binary a5200) and a
# fujinet-pc BoIP listener on FUJINET_TCP (default 127.0.0.1:9995). MAME
# serves the image from power-on (it carries the cart's claim), so that
# fujinet-pc needn't know .a52; it carries the network and the appkeys.
#
# Controls (MAME's defaults): the arrow keys are the stick, left Ctrl the
# top fire button, left Alt the lower one (rub out); 1 is START (end your
# turn); the numeric pad is the keypad: + is *, leave the table (and, at
# the table list, quit to CONFIG), 0 sits out an auction, Enter is #, the
# sound off and on.
#
# MAME runs from its own tree, so every path here is absolute.
set -euo pipefail
cd "$(dirname "$0")/../.."
HERE="$PWD"                                 # clients/mekkogx

MAME_DIR="${MAME_DIR:-$HOME/Workspace/mame-a5200}"
MAME_BIN="${MAME_BIN:-a5200}"
ROMS="${ROMS:-$HOME/Workspace/mame/roms}"
SYSTEM="${SYSTEM:-a5200}"
export FUJINET_TCP="${FUJINET_TCP:-127.0.0.1:9995}"
ENDPOINT="${ENDPOINT:-http://fujinet.online:9900/}"

if [ $# -eq 0 ] || [ "$1" = - ]; then
    mkdir -p build
    make atari5200 ENDPOINT="$ENDPOINT" > build/run-a52.log 2>&1 \
        || { tail -30 build/run-a52.log >&2; exit 1; }
    IMAGE="$HERE/r2r/atari5200/mule.a52"
else
    IMAGE="$1"
    [ "${IMAGE#/}" = "$IMAGE" ] && IMAGE="$OLDPWD/$IMAGE"
fi
[ -f "$IMAGE" ] || { echo "run.sh: no $IMAGE" >&2; exit 1; }
[ -x "$MAME_DIR/$MAME_BIN" ] || { echo "run.sh: no MAME at $MAME_DIR/$MAME_BIN" >&2; exit 1; }

RUN="$HERE/build/a52run"
mkdir -p "$RUN/snap" "$RUN/cfg"
cd "$MAME_DIR"
exec "./$MAME_BIN" "$SYSTEM" -rompath "$ROMS" -cartslot fujinet -cart "$IMAGE" \
    -snapshot_directory "$RUN/snap" -cfg_directory "$RUN/cfg" -nvram_directory "$RUN/cfg" \
    -skip_gameinfo -window -nomax "${@:2}"

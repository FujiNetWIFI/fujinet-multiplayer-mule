#!/usr/bin/env bash
# run.sh -- play it: MAME's Odyssey 2 with the FujiNet cart, in a window,
# against a live fujinet-pc.
#
#   ./run.sh                       build/mule.bin (built if missing)
#   ./run.sh path/to/image.bin     another image (more MAME options after it)
#   ENDPOINT=http://host:port ./run.sh   rebuild for that server first
#
# Needs ~/Workspace/mame with the O2 FujiNet cart device grafted in
# (fujinet-firmware PR #2177, pico/o2/emu/apply-mame.sh: protocol 2, the
# APPBANK pages) and a fujinet-pc BoIP listener on FUJINET_TCP (default
# 127.0.0.1:9995). The name comes from that FujiNet's lobby appkey; without
# it the game asks.
#
# Controls (MAME's defaults): the arrow keys are the left joystick, Left
# Ctrl its button.
#
# MAME runs from its own tree, so every path here is absolute.
set -euo pipefail
cd "$(dirname "$0")"
HERE="$PWD"

MAME_DIR="${MAME_DIR:-$HOME/Workspace/mame}"
export FUJINET_TCP="${FUJINET_TCP:-127.0.0.1:9995}"
IMAGE="${1:-$HERE/build/mule.bin}"
[ "${IMAGE#/}" = "$IMAGE" ] && IMAGE="$HERE/$IMAGE"

if [ -n "${ENDPOINT:-}" ] || [ ! -f build/mule.bin ]; then
    ./build.sh
fi
[ -f "$IMAGE" ] || { echo "run.sh: no $IMAGE" >&2; exit 1; }
[ -x "$MAME_DIR/mame" ] || { echo "run.sh: no MAME at $MAME_DIR" >&2; exit 1; }

mkdir -p "$HERE/build/snap" "$HERE/cfg"
cd "$MAME_DIR"
exec ./mame odyssey2 -cartslot fujinet -cart "$IMAGE" \
    -snapshot_directory "$HERE/build/snap" -cfg_directory "$HERE/cfg" \
    -window -nomax "${@:2}"

#!/usr/bin/env bash
# run.sh -- run the client in the FujiNet-patched MAME.
#
#   ./run.sh [lua-script]      headless with a script from emu/, else a window
#
# Needs a MAME tree with fn-2600's pico/atari-2600/emu/apply.sh applied.
#   - MAME must run FROM ITS OWN TREE or -autoboot_script is ignored.
#   - The cart model talks to a fujinet-pc over BoIP at FUJINET_TCP; it takes
#     ONE client, so never leave a MAME running.
#   - Other sessions run MAME too: this never pkills by name.
#   - Throttled is not a choice: the server's clock is wall time. FAST=1 only
#     for pictures of things that wait on nothing.
#
# Env: MAME (tree), SECS, FAST, FUJINET_TCP (default 127.0.0.1:19996), ROM.

set -euo pipefail
cd "$(dirname "$0")"
HERE=$(pwd)
SCRIPT=${1:-}
MAME=${MAME:-$HOME/Workspace/mame}
ROM=${ROM:-mule2600}

args=(a2600 -window -cartslot fujinet -cart "$HERE/build/$ROM.bin"
      -snapshot_directory "$HERE/build/snap" -nvram_directory "$HERE/build/nvram"
      -cfg_directory "$HERE/build/cfg")
if [ -n "$SCRIPT" ]; then
    args+=(-autoboot_script "$HERE/emu/$SCRIPT.lua" -video none -sound none
           -seconds_to_run "${SECS:-20}")
    [ -n "${FAST:-}" ] && args+=(-nothrottle)
fi
[ -n "${DISPLAY:-}" ] || export SDL_VIDEODRIVER=dummy
export FUJINET_TCP="${FUJINET_TCP:-127.0.0.1:9995}"
export M26_EMU="$HERE/emu" M26_BUILD="$HERE/build"
cd "$MAME"
exec ./mame "${args[@]}"

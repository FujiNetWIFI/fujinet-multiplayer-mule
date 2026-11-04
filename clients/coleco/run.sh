#!/usr/bin/env bash
# run.sh -- the client in a MAME window, with the FujiNet cartridge device,
# against a fujinet-pc's BoIP listener (FUJINET_TCP, default 127.0.0.1:9995).
#
# MAME needs fujinet-firmware's pico/coleco/emu/apply.sh grafted in, with
# APPBANK (the cart serves this banked image). It runs from its own tree:
# from anywhere else -autoboot_script and its rompath go missing.
set -euo pipefail
cd "$(dirname "$0")"
MAME_DIR=${MAME_DIR:-$HOME/Workspace/mame}
ROM="$PWD/build/mule.rom"
[ -f "$ROM" ] || ./build.sh
export FUJINET_TCP=${FUJINET_TCP:-127.0.0.1:9995}
cd "$MAME_DIR"
exec ./mame coleco -cartslot fujinet -cart "$ROM" -window -nomax "$@"

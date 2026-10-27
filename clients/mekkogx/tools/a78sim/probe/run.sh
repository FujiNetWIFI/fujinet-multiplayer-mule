#!/usr/bin/env bash
# run.sh -- build the M0 probe, run it in MAME (NTSC, then PAL), check it.
#   tools/a78sim/probe/run.sh [a7800|a7800p ...]
set -euo pipefail
cd "$(dirname "$0")"
HERE=$(pwd)
B=$HERE/build
MAME=${MAME:-$HOME/Workspace/mame-a7800}
mkdir -p "$B"
python3 gen.py "$B/probe_data.s" "$B/probe.json"
ca65 -o "$B/probe.o" probe.s
ca65 -o "$B/probe_data.o" "$B/probe_data.s"
ld65 -C probe.cfg -m "$B/probe.map" -o "$B/probe.bin" "$B/probe.o" "$B/probe_data.o"
python3 ../../atari7800/mka78.py "$B/probe.bin" "$B/probe.a78" PROBE
CNT=FC
for sys in ${@:-a7800 a7800p}; do
    rm -rf "$B/snap-$sys"; mkdir -p "$B/snap-$sys" "$B/cfg"
    ( cd "$MAME" && SDL_VIDEODRIVER=dummy FUJINET_TCP=127.0.0.1:1 \
      PROBE_FRAMES=120 PROBE_CNT=${CNT#00} \
      ./a7800 "$sys" -rompath "$HOME/Workspace/mame/roms" -cartslot fujinet -cart "$B/probe.a78" \
        -snapshot_directory "$B/snap-$sys" -cfg_directory "$B/cfg" -nvram_directory "$B/cfg" \
        -autoboot_script "$HERE/probe.lua" -video none -sound none -nothrottle -seconds_to_run 30 \
        2>&1 | grep -v '^$' | tail -5 )
    python3 check.py "$B/probe.json" "$(find "$B/snap-$sys" -name '*.png' | head -1)" "$sys"
done

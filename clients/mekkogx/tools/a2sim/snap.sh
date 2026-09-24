#!/bin/sh
# snap.sh DISK OUTDIR FRAMES -- boot DISK in MAME's //e (enhanced), save PNG
# snapshots at the listed frames (e.g. "400,700") to OUTDIR, then quit.
HERE=$(cd "$(dirname "$0")" && pwd)
DISK=$(realpath "$1"); OUT=$(realpath -m "$2"); FRAMES=${3:-600}
MAME=${MAME:-$HOME/Workspace/mame}
mkdir -p "$OUT"
cd "$MAME" && A2SNAP="$FRAMES" timeout 300 ./mame apple2ee -rompath roms -flop1 "$DISK" \
  -video none -sound none -nothrottle -snapshot_directory "$OUT" \
  -autoboot_script "$HERE/snap.lua" -skip_gameinfo >/dev/null 2>&1
ls "$OUT"/apple2ee

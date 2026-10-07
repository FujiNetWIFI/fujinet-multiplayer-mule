#!/usr/bin/env bash
# clips_2600.sh OUTDIR [SECS] -- film the Atari 2600 client in a live game.
#
# The 2600 has no offline test mode, so this plays a whole game on the
# client's rig (test/rig.sh: a private muleserver and fujinet-pc, MAME in real
# time, emu/auto.lua playing) with a2600/film.lua filming each new screen.
# About an hour. Frames land in OUTDIR/raw/NNNNN.png; OUTDIR/film.log maps
# each to its screen ("FILM k key bank:phase:round ...").
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=$(mkdir -p "$1" && cd "$1" && pwd)
SECS=${2:-4500}
D=$HERE/../../clients/atari-2600
cp "$HERE/a2600/film.lua" "$D/emu/film.lua"
sed 's/LUA=auto/LUA=film/' "$D/test/rig.sh" > "$D/build/rig-film.sh"
rm -rf "$D/build/snap"
(cd "$D" && bash build/rig-film.sh auto "$SECS" > "$OUT/film.log" 2>&1) || true
rm -f "$D/emu/film.lua"
rm -rf "$OUT/raw"
mkdir -p "$OUT/raw"
# MAME numbers the snapshots 0000.png on; keep them in order, 5 digits
i=0
for f in $(ls "$D/build/snap/a2600" | sort -n); do
    mv "$D/build/snap/a2600/$f" "$OUT/raw/$(printf %05d $i).png"
    i=$((i + 1))
done
echo "clips_2600: $i frames in $OUT/raw"

#!/bin/sh
# mkfnlib.sh SRC OUT -- build fujinet-lib-experimental (SRC, its add-nes
# branch: bus/nes, the cartridge's mailbox) for the NES into
# OUT/fujinet-nes.lib, headers in OUT/include, compiled here (not in SRC's
# own build).
set -e
src=$1
out=$2
[ -d "$src/bus/nes" ] || { echo "mkfnlib.sh: no $src/bus/nes (fujinet-lib-experimental's add-nes branch)" >&2; exit 1; }
rm -rf "$out"
mkdir -p "$out/obj" "$out/include"
cp -r "$src"/include/. "$out/include/"
( cd "$src" && git rev-parse --short HEAD 2>/dev/null || echo unknown ) > "$out/VERSION"
objs=
for c in "$src"/common/*.c "$src"/bus/nes/*.c; do
    o="$out/obj/$(basename "${c%.c}").o"
    cl65 -t nes -O --cpu 6502 -DBUILD_NES \
         -I "$out/include" -I "$src/bus/nes" -c -o "$o" "$c"
    objs="$objs $o"
done
ar65 a "$out/fujinet-nes.lib" $objs

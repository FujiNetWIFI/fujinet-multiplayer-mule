#!/bin/sh
# mkfnlib.sh SRC OUT -- build fujinet-lib-experimental (SRC) for the Lynx
# into OUT/fujinet-lynx.lib, headers in OUT/include, compiled here (not in
# SRC's own build) with the bus packet buffer cut to 256 bytes: the game's
# biggest transfer is a 64-byte appkey, and main RAM is tight.
set -e
src=$1
out=$2
rm -rf "$out"
mkdir -p "$out/obj" "$out/include"
cp -r "$src"/include/. "$out/include/"
( cd "$src" && git rev-parse --short HEAD 2>/dev/null || echo unknown ) > "$out/VERSION"
objs=
for c in "$src"/common/*.c "$src"/bus/lynx/*.c; do
    o="$out/obj/$(basename "${c%.c}").o"
    cl65 -t lynx -O --cpu 6502 -DBUILD_LYNX -DLYNX_PACKET_MAX=256 \
         -I "$out/include" -I "$src/bus/lynx" -c -o "$o" "$c"
    objs="$objs $o"
done
ar65 a "$out/fujinet-lynx.lib" $objs

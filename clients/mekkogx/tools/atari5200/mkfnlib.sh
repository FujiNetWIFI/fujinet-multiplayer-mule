#!/bin/sh
# mkfnlib.sh SRC OUT -- build fujinet-lib-experimental (SRC, its
# add-atari5200 branch: bus/atari5200, the cartridge's mailbox) for the 5200
# into OUT/fujinet-a52.lib, headers in OUT/include, compiled here (not in
# SRC's own build). The lib's crt0 is left out (src/atari5200/crt0.s is the
# game's).
set -e
src=$1
out=$2
[ -d "$src/bus/atari5200" ] || { echo "mkfnlib.sh: no $src/bus/atari5200 (fujinet-lib-experimental's add-atari5200 branch)" >&2; exit 1; }
rm -rf "$out"
mkdir -p "$out/obj" "$out/include"
cp -r "$src"/include/. "$out/include/"
cp "$src"/bus/atari5200/*.h "$out/include/"
( cd "$src" && git rev-parse --short HEAD 2>/dev/null || echo unknown ) > "$out/VERSION"
objs=
for c in "$src"/common/*.c "$src"/bus/atari5200/*.c "$src"/bus/atari5200/*.s; do
    b=$(basename "$c")
    [ "$b" = crt0.s ] && continue
    o="$out/obj/${b%.*}.o"
    # what only the game's boot calls (the appkeys, finding the cartridge)
    # goes in its boot overlay's bank (mule-a52.cfg OVERLAY1): every byte
    # left in REPL is there four times
    seg=
    case "${b%.*}" in
        appkey_atari5200|fuji_appkey|fuji_atari5200) seg="--code-name OVERLAY1 --rodata-name OVL1RO" ;;
    esac
    case "$c" in
        *.s) ca65 -t atari5200 -o "$o" "$c" ;;
        *)   cl65 -t atari5200 -O --cpu 6502 -DBUILD_ATARI5200 $seg \
                  -I "$out/include" -I "$src/bus/atari5200" -c -o "$o" "$c" ;;
    esac
    objs="$objs $o"
done
ar65 a "$out/fujinet-a52.lib" $objs

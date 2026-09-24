#!/bin/sh
# mkfnlib.sh SRC OUT -- build fujinet-lib-experimental (SRC) for MS-DOS in
# the medium memory model (the game's) into OUT/fujinet-msdos.lib, headers
# in OUT/include. The released lib is small model: far calls into it would
# not link. Sources are copied in first so the container can see them.
set -e
src=$1
out=$2
ow=$(dirname "$0")/ow
rm -rf "$out"
mkdir -p "$out/src" "$out/obj" "$out/include"
cp "$src"/common/*.c "$src"/bus/msdos/*.c "$out/src/"
cp "$src"/bus/msdos/*.h "$out/src/"
cp -r "$src"/include/. "$out/include/"
( cd "$src" && git rev-parse --short HEAD 2>/dev/null || echo unknown ) > "$out/VERSION"
objs=
for c in "$out"/src/*.c; do
    o="$out/obj/$(basename "${c%.c}").o"
    objs="$objs $o"
done
# one container run for the whole library
cmds=
for c in "$out"/src/*.c; do
    o="$out/obj/$(basename "${c%.c}").o"
    cmds="$cmds wcc -zq -mm -0 -bt=dos -os -s -D__MSDOS__ -DBUILD_MSDOS -I$out/include -I$out/src -fo=$o $c || exit 1;"
done
cmds="$cmds wlib -q -n $out/fujinet-msdos.lib $(for o in $objs; do printf ' +%s' "$o"; done)"
"$ow" sh -c "$cmds"

#!/bin/bash
# make-shots.sh -- regenerate screenshots/<target>/*.png from the offline
# test mode (src/teststart.c's fake round-1 state), one scene per screen.
#
# usage: [ONLY=08-develop-map] [OLD_MAME=dir] screenshots/make-shots.sh [TARGET...]
# Build the clients first (top-level `make clients`). The Atari 2600 has
# no offline mode: `make-shots.sh atari-2600` plays a live game (see a2600()).
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd)
SHOTS=$ROOT/screenshots
TMP=${TMPDIR:-/tmp}/mule-shots.$$
MX=$ROOT/clients/mekkogx
# the PCjr and the CoCo only boot in the old MAME (0.263: the new one fails
# the PCjr's -ram and wants a CoCo charset dump): OLD_MAME = a dir with
# `mame` -> mamed and `roms`
OLD_MAME=${OLD_MAME:-}

# shoot TARGET DIR CMD... : run every scene with CMD SCEN OUT from DIR
shoot() {
    local t=$1 dir=$2 s n out
    shift 2
    mkdir -p "$SHOTS/$t"
    for s in "$SHOTS"/scenes/${ONLY:-*}.scen; do
        n=$(basename "$s" .scen)
        # a target's own version of a scene, where its input differs
        [ -f "$SHOTS/scenes/$t/$n.scen" ] && s=$SHOTS/scenes/$t/$n.scen
        # "skip": the target's test mode can't show this screen
        grep -q '^skip' "$s" && continue
        out=$TMP/$t/$n
        mkdir -p "$out"
        (cd "$dir" && timeout 300 "$@" "$s" "$out" >"$out/log" 2>&1)
        if [ -f "$out/$n.png" ]; then
            cp "$out/$n.png" "$SHOTS/$t/"
        else
            echo "$t: no $n.png (see $out/log)"
        fi
    done
}

# The 2600 has no offline mode: play a live game (test/rig.sh's private
# server and fujinet-pc, emu/auto.lua playing) with a2600-shots.lua taking
# a snapshot on each new screen. About an hour, to game over; the shots
# land in clients/atari-2600/build/snap/a2600/ to be picked by hand.
a2600() {
    local d=$ROOT/clients/atari-2600
    cp "$SHOTS/a2600-shots.lua" "$d/emu/shots.lua"
    sed 's/LUA=auto/LUA=shots/' "$d/test/rig.sh" > "$d/build/rig-shots.sh"
    rm -rf "$d/build/snap"
    (cd "$d" && bash build/rig-shots.sh auto 4500 > build/shots.log 2>&1)
    rm -f "$d/emu/shots.lua"
    echo "atari-2600: pick from $d/build/snap/a2600 (see build/shots.log)"
}

run() {
    case $1 in
    apple2) shoot apple2 "$MX" python3 tools/a2sim/a2sim.py r2r/apple2/mule.po build/mule-apple2.map ;;
    coco) MAME=${OLD_MAME:-$MAME_DEFAULT} shoot coco "$MX" python3 tools/cocosim/cocosim.py r2r/coco/mule.dsk build/mule-coco.map ;;
    msdos-cga) DOSSIM_MACHINE=ibm5150 DOSSIM_NOFN=1 \
        shoot msdos-cga "$MX" python3 tools/dossim/dossim.py r2r/msdos/mule.exe build/mule-msdos.map ;;
    # the PCjr runs MULEJR.EXE (MULESEL picks it): its own name on the
    # disk, where its overlays are read from (DOS 2 tells it no other)
    msdos-pcjr) MAME=${OLD_MAME:-$MAME_DEFAULT} DOSSIM_RAMOPT=-ramsize DOSSIM_RAM=128K DOSSIM_NOFN=1 \
        DOSSIM_FILES="r2r/msdos/mulejr.exe r2r/msdos/mulesel.com" DOSSIM_AUTOEXEC=MULESEL \
        shoot msdos-pcjr "$MX" python3 tools/dossim/dossim.py r2r/msdos/mule.exe build/mulejr-msdos.map ;;
    adam) shoot adam "$MX" python3 tools/adam/adamsim/adamsim.py r2r/adam/mule.ddp r2r/adam/mule.map ;;
    msx) shoot msx "$MX" python3 tools/msx/msxsim/msxsim.py r2r/msxrom/mule.rom r2r/msxrom/mule.map ;;
    atari) shoot atari "$MX" python3 tools/atarisim/atarisim.py r2r/atari/mule.atr build/mule-atari.map ;;
    lynx) shoot lynx "$MX" python3 tools/lynxsim/lynxsim.py r2r/lynx/mule.lnx build/mule-lynx.map ;;
    nes) shoot nes "$MX" python3 tools/nessim/nessim.py r2r/nes/mule.nes build/mule-nes.map ;;
    sms) shoot sms "$MX" python3 tools/smssim/smssim.py r2r/sms/mule.sms r2r/sms/mule.map ;;
    intv) shoot intv "$ROOT/clients/intv" python3 tools/intvsim.py mule.rom ;;
    astrocade) shoot astrocade "$ROOT/clients/astrocade" python3 tools/astrosim.py ;;
    atari-2600) a2600 ;;
    *) echo "unknown target $1"; return 1 ;;
    esac
}

MAME_DEFAULT=${MAME:-$HOME/Workspace/mame}
TARGETS=${*:-apple2 coco msdos-cga msdos-pcjr adam msx atari lynx nes sms intv astrocade}
for t in $TARGETS; do
    run "$t" &
done
wait
rm -rf "$TMP"

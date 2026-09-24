#!/bin/sh
# snap.sh DISK OUTDIR FRAMES -- boot DISK in MAME's CoCo 2 (Disk BASIC), type
# LOADM"MULE":EXEC, save PNG snapshots at the listed frames (e.g. "400,700")
# to OUTDIR, then quit. COCO_BIOS, COCO_ARTIFACT, COCO_BECKER and DISK "-"
# as for cocosim.py.
HERE=$(cd "$(dirname "$0")" && pwd)
DISK="$1"; [ "$DISK" != "-" ] && DISK=$(realpath "$1"); OUT=$(realpath -m "$2"); FRAMES=${3:-600}
MAME=${MAME:-$HOME/Workspace/mame}
EXT=fdc; [ -n "$COCO_BIOS" ] && EXT="fdc,bios=$COCO_BIOS"
mkdir -p "$OUT/cfg"
cat > "$OUT/cfg/coco2b.cfg" <<CFG
<?xml version="1.0"?>
<mameconfig version="10"><system name="coco2b"><input>
<port tag=":beckerport" type="CONFIG" mask="1" defvalue="0" value="$([ -n "$COCO_BECKER" ] && echo 1 || echo 0)" />
<port tag=":vdg:artifacting" type="CONFIG" mask="3" defvalue="1" value="${COCO_ARTIFACT:-1}" />
</input></system></mameconfig>
CFG
[ "$DISK" = "-" ] && export COCO_TYPE=0     # HDB-DOS runs AUTOEXEC.BAS itself
cd "$MAME" && A2SNAP="$FRAMES" timeout 300 ./mame coco2b -rompath roms -ext "$EXT" $([ "$DISK" != "-" ] && echo -flop1 "$DISK") \
  -video none -sound none -nothrottle -snapshot_directory "$OUT" \
  -cfg_directory "$OUT/cfg" -nvram_directory "$OUT/nvram" \
  -autoboot_script "$HERE/snap.lua" -skip_gameinfo >/dev/null 2>&1
ls "$OUT"/coco2b

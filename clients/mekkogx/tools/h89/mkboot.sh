#!/bin/sh
# mkboot.sh MULE.COM OUT.h8d -- a bootable H17 disk with M.U.L.E. on it.
#
# r2r/h89-cpm/mule.h8d is a data disk: no system on its boot tracks, no
# BIOS.SYS (Heath's CP/M 2.2.04 loader reads the BIOS from that file). This
# starts from fujinet-go-h89-desktop's 64K CP/M 2.2.04 system disk (its
# tools/disks/boot/cpm-2.2.04-64k.h8d: Heath's distribution disk after
# MOVCPM17 64 * and SYSGEN) and lets CP/M itself, in that project's
# headless H89, make the room and copy the game on, as a user would: ERA the
# development tools (MULE.COM is 47K of the disk's 90K), PIP MULE.COM over
# from a data disk in drive 2. What stays: BIOS.SYS, PIP, STAT, SYSGEN,
# FORMAT and DUP (to make copies), and MULE.COM. Boot it, then MULE.
#
# H89_RIG: fujinet-go-h89-desktop (~/Workspace/fujinet-go-h89-desktop), built.
set -e
com=$(realpath "$1")
out=$(realpath -m "$2")
rig=${H89_RIG:-$HOME/Workspace/fujinet-go-h89-desktop}
run=$rig/build/core/h89_session_run
sys=$rig/tools/disks/boot/cpm-2.2.04-64k.h8d
here=$(cd "$(dirname "$0")" && pwd)
for f in "$com" "$run" "$sys"; do
    [ -e "$f" ] || { echo "mkboot.sh: missing $f" >&2; exit 1; }
done
z88dk=$here/../z80/z88dk
work=$(mktemp -d "$(dirname "$out")/mkboot.XXXXXX")
trap 'rm -rf "$work"' EXIT

# the game on a data disk, for PIP to read
mkdir -p "$work/data"
cp "$com" "$work/data/MULE.COM"
(cd "$work/data" && "$z88dk" z88dk-appmake +cpmdisk -f hz89 --container raw --no-com-file \
    -b MULE.COM -o disk >/dev/null)
python3 -I "$here/cpm-dir-fixup.py" "$work/data/disk"
cp "$sys" "$work/boot.h8d"

steps="until H:
type B\\r
until A>
wait 800"
for f in ASM DDT ED LOAD MOVCPM17 SUBMIT XSUB CONFIGUR ASSIGN; do
    steps="$steps
prompt A>
type ERA $f.COM\\r
wait 1500"
done
steps="$steps
prompt A>
type PIP A:=B:MULE.COM[V]\\r
until A>
wait 30000
prompt A>
type STAT *.*\\r
wait 4000
prompt A>
screen"
# (the rig's ports are fixed: in a network namespace of its own when it can)
ns=
unshare -rn true 2>/dev/null && ns="unshare -rn"
printf '%s\n' "$steps" | $ns "$run" --roms "$rig/tools/roms" --home "$work/home" --timeout 120 \
    --disk "h17:1=$work/boot.h8d" --disk "h17:2=$work/data/disk" --stdin > "$work/log" 2>&1
sed -n '/---- screen ----/,/^\(\[fujinet\] \)\?----------------/p' "$work/log" | sed 's/^\[fujinet\] //'
grep -q 'A:MULE.COM' "$work/log" && ! grep -q 'NO DIRECTORY SPACE\|DISK WRITE ERROR\|VERIFY ERROR' "$work/log" || {
    echo "mkboot.sh: MULE.COM didn't get onto the disk (see above)" >&2; exit 1; }
cp "$work/boot.h8d" "$out"
echo "mkboot.sh: $out"

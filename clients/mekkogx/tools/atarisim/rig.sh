#!/usr/bin/env bash
# rig.sh -- the Atari client live: a private server, FujiNet and asim over
# NetSIO, all in a network namespace of their own (unshare -rn).
#
#   tools/atarisim/rig.sh ASIM_SCRIPT OUTDIR
#
# Builds the disk from a scratch copy of SRC (clients/mekkogx, or another
# tree: a worktree of an older commit for a baseline) with ENDPOINT
# http://localhost:9900/, so the rig's endpoint never reaches gen/ or r2r/;
# builds the M.U.L.E. server from SRC's repo and starts it on 9900 (stream
# 9902, the image's default); starts a copy of fujinet-pc-atari (NetSIO
# 9997, web UI 8013) with the disk mounted in slot 1 from its SD and the
# appkeys seeded: name NAME (ATR), room ROOM (the local server's irata
# table). Then runs asim on ASIM_SCRIPT, in real time.
#
# The script is asim's own language (asim.c), not atarisim's: it starts
# with "wait 120" and "coldstart" (FujiNet is there by then), and
# @symbol[+n] stands for that address in the rig's link map, in hex
# ("trace t.csv @phase,@rnd,@st+388", "peek @frame_count 2"); OUTDIR gets
# asim's files and log. AUTOPLAY=1 builds the image that plays by itself;
# BUILD=0 runs the image RIG already has; XL=1 an 800XL. The ports are the
# namespace's alone: never the production server, never the everyday
# fujinet-pc. The copied fujinet-pc config loses its cloud sections (they
# hold tokens). Kills what it started when asim ends.
set -euo pipefail
HERE=$(cd "$(dirname "$0")/../.." && pwd)          # clients/mekkogx
SRC=$(realpath "${SRC:-$HERE}")
ROOT=$(cd "$SRC/../.." && pwd)
SCRIPT=$(realpath "$1")
OUT=$(realpath -m "$2")
RIG=$(realpath -m "${RIG:-$HERE/build/rig-atari}")
NAME=${NAME-ATR}
ROOM=${ROOM-http://localhost:9900/?table=irata}
FNPC_DIST=${FNPC_DIST:-$HOME/Workspace/fujinet-pc-atari/build/dist}
ASIM=$HERE/tools/atarisim/asim
mkdir -p "$RIG" "$OUT"

# the disk, from a copy, and the server
if [ "${BUILD:-1}" = 1 ]; then
    rsync -a --delete --exclude build --exclude r2r "$SRC/" "$RIG/src/"
    cp "$SRC/../mule_proto.h" "$RIG/"            # (the build's "..")
    ( cd "$RIG/src" && rm -f gen/endpoint && make atari ENDPOINT=http://localhost:9900/ \
          ${AUTOPLAY:+AUTOPLAY=1} > "$RIG/build.log" 2>&1 ) || { tail -20 "$RIG/build.log"; exit 1; }
    ( cd "$ROOT" && go build -o "$RIG/muleserver" ./cmd/muleserver )
fi

# the script, its symbols resolved
python3 - "$RIG/src/build/mule-atari.map" "$SCRIPT" "$OUT/script.txt" <<'PY'
import re, sys
t = open(sys.argv[1]).read().split("Exports list by name:")[1].split("Exports list by value:")[0]
s = {n: int(a, 16) for n, a in re.findall(r"(\S+)\s+([0-9A-F]{6})\s+\S+", t)}
def addr(m):
    n = m.group(1)
    return "%x" % ((s["_" + n] if "_" + n in s else s[n]) + int(m.group(2) or "0", 0))
src = open(sys.argv[2]).read()
open(sys.argv[3], "w").write(re.sub(r"@(\w+)(?:\+(\w+))?", lambda m: addr(m), src))
PY

# fujinet-pc
rm -rf "$RIG/fn"; mkdir -p "$RIG/fn/SD/FujiNet"
cp -a "$FNPC_DIST/fujinet" "$FNPC_DIST/run-fujinet" "$FNPC_DIST/data" "$RIG/fn/"
python3 - "$FNPC_DIST/fnconfig.ini" "$RIG/fn/fnconfig.ini" <<'PY'
import re, sys
out, sec = [], ""
for line in open(sys.argv[1]).read().splitlines():
    m = re.match(r"\[(.*)\]", line)
    if m:
        sec = m.group(1)
    if sec in ("GoogleDrive", "S3", "OneDrive"):
        continue
    if sec == "General" and line.startswith("configenabled="):
        line = "configenabled=0"
    if sec == "Mount1" and line.startswith("hostslot="):
        line = "hostslot=1"                     # (1-based)
    if sec == "Mount1" and line.startswith("path="):
        line = "path=/mule.atr"
    out.append(line)
open(sys.argv[2], "w").write("\n".join(out) + "\n")
PY
cp "$RIG/src/r2r/atari/mule.atr" "$RIG/fn/SD/mule.atr"
[ -n "$NAME" ] && printf '%s' "$NAME" > "$RIG/fn/SD/FujiNet/00010100.key"
[ -n "$ROOM" ] && printf '%s' "$ROOM" > "$RIG/fn/SD/FujiNet/0001010a.key"

# all of it in a namespace; fujinet-pc restarts when the emulator resets
# (run-fujinet), in a process group of its own so it all goes at the end
export RIG OUT ASIM XL
exec unshare -rn bash -c '
    ip link set lo up
    PORT=9900 MULE_STREAM_ADDR=:9902 "$RIG/muleserver" > "$RIG/server.log" 2>&1 &
    srv=$!
    ( cd "$RIG/fn" && exec setsid ./run-fujinet -u http://127.0.0.1:8013 -c fnconfig.ini -s SD ) \
        > "$RIG/fn.log" 2>&1 &
    fn=$!
    trap "kill -- -$fn $srv 2>/dev/null; true" EXIT
    for i in $(seq 40); do ss -ltn | grep -q ":9900 " && break; sleep 0.25; done
    sleep 1
    "$ASIM" ${XL:+-xl} -netsio 9997 - "$OUT/script.txt" "$OUT" > "$OUT/asim.log" 2>&1 || true
    grep -v "^$" "$OUT/asim.log" | tail -40
'

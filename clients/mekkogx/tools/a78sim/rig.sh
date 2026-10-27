#!/usr/bin/env bash
# rig.sh -- the 7800 client live: a private server, FujiNet and MAME.
#
#   tools/a78sim/rig.sh SCENARIO OUTDIR [SECS]
#
# Builds the image from a scratch copy of clients/mekkogx (so the rig's
# endpoint never reaches gen/ or r2r/) with ENDPOINT
# http://127.0.0.1:MULE_PORT/ and the stream on STREAM; starts the M.U.L.E.
# server built from this repo (or SERVER_SRC) on MULE_PORT (8087), its
# stream on STREAM (19987), and a private copy of fujinet-pc-rs232 on BoIP
# port BOIP (19988; its web UI on BOIP - 10000), appkeys seeded: name NAME
# (A78), room ROOM (the local server's irata table). Then runs
# tools/a78sim/a78sim.py on SCENARIO, throttled (the server's clock is wall
# time). AUTOPLAY=1 builds the image that plays by itself; A78SIM_SYSTEM
# =a7800p a PAL console; SLOW=ms puts the server behind a delay proxy on
# STREAM + 1. Kills only what it started. Never the production server,
# never the everyday fujinet-pc on 9995. The copied fujinet-pc config loses
# its cloud sections (they hold tokens).
set -euo pipefail
HERE=$(cd "$(dirname "$0")/../.." && pwd)          # clients/mekkogx
ROOT=$(cd "$HERE/../.." && pwd)
SCEN=$(realpath "$1")
OUT=$(realpath -m "$2")
SECS=${3:-300}
MULE_PORT=${MULE_PORT:-8087}
STREAM=${STREAM:-19987}
BOIP=${BOIP:-19988}
NAME=${NAME-A78}
ROOM=${ROOM-http://127.0.0.1:$MULE_PORT/?table=irata}
FNPC_DIST=${FNPC_DIST:-$HOME/Workspace/fujinet-pc-rs232/build/dist}
RIG=${RIG:-$HERE/build/rig-a78}

for p in $MULE_PORT $STREAM $BOIP $((BOIP - 10000)); do
    if ss -ltn | grep -q ":$p "; then echo "rig: port $p is taken" >&2; exit 1; fi
done
pids=()
cleanup() {
    for p in "${pids[@]}"; do kill -9 "$p" 2>/dev/null || true; done
}
trap cleanup EXIT
mkdir -p "$RIG" "$OUT"

# the image, from a copy
rsync -a --delete --exclude build --exclude r2r --exclude _cache \
    "$HERE/" "$RIG/src/"
cp "$HERE/../mule_proto.h" "$RIG/"            # (the build's "..")
mkdir -p "$RIG/src/build"
[ -d "$HERE/build/fnlib-a78" ] && cp -a "$HERE/build/fnlib-a78" "$RIG/src/build/"
( cd "$RIG/src" && make atari7800 ENDPOINT=http://127.0.0.1:$MULE_PORT/ \
      CFLAGS_EXTRA="-DNS_PORT=$STREAM ${AUTOPLAY:+-DMULE_AUTOPLAY}" ${AUTOPLAY:+AUTOPLAY=1} \
      > "$RIG/build.log" 2>&1 ) || { tail -20 "$RIG/build.log"; exit 1; }

# the server
( cd "${SERVER_SRC:-$ROOT}" && go build -o "$RIG/muleserver" ./cmd/muleserver )
PORT=$MULE_PORT MULE_STREAM_ADDR=:$STREAM "$RIG/muleserver" > "$RIG/server.log" 2>&1 &
pids+=($!)
if [ -n "${SLOW:-}" ]; then
    echo "rig: SLOW is not wired for the stream yet" >&2
fi

# fujinet-pc
rm -rf "$RIG/fn"; mkdir -p "$RIG/fn"
ln -s "$FNPC_DIST/fujinet" "$RIG/fn/fujinet"
cp -a "$FNPC_DIST/data" "$RIG/fn"/
python3 - "$FNPC_DIST/fnconfig.ini" "$RIG/fn/fnconfig.ini" "$BOIP" <<'PY'
import re, sys
out, skip = [], False
for line in open(sys.argv[1]).read().splitlines():
    m = re.match(r"\[(.*)\]", line)
    if m:
        skip = m.group(1) in ("GoogleDrive", "S3", "OneDrive")
    if not skip:
        out.append(line)
s = "\n".join(out) + "\n"
s = re.sub(r"(\[BOIP\][^\[]*?\bport=)\d*", r"\g<1>" + sys.argv[3], s, flags=re.S)
s = re.sub(r"(\[BOIP\][^\[]*?\benabled=)\d*", r"\g<1>1", s, flags=re.S)
open(sys.argv[2], "w").write(s)
PY
mkdir -p "$RIG/fn/SD/FujiNet"
[ -n "$NAME" ] && printf '%s' "$NAME" > "$RIG/fn/SD/FujiNet/00010100.key"
[ -n "$ROOM" ] && printf '%s' "$ROOM" > "$RIG/fn/SD/FujiNet/0001010a.key"
( cd "$RIG/fn" && exec ./fujinet -u http://127.0.0.1:$((BOIP - 10000)) -c fnconfig.ini -s SD ) \
    > "$RIG/fn.log" 2>&1 &
pids+=($!)
for _ in $(seq 20); do ss -ltn | grep -q ":$BOIP " && break; sleep 0.5; done
if ! ss -ltn | grep -q ":$BOIP "; then echo "rig: no BoIP listener on $BOIP" >&2; exit 1; fi

FUJINET_TCP="127.0.0.1:$BOIP" A78SIM_THROTTLE=1 A78SIM_SECONDS=$SECS \
    python3 "$HERE/tools/a78sim/a78sim.py" "$RIG/src/r2r/atari7800/mule.a78" \
    "$RIG/src/build/mule-a78.map" "$SCEN" "$OUT"

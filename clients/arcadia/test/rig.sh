#!/usr/bin/env bash
# rig.sh -- a private server and FujiNet for a live run, then a scenario.
#
#   test/rig.sh SCENARIO OUTDIR [SECS [IMAGE]]
#
# Starts the M.U.L.E. server (built from this repo, or SERVER_SRC) on
# MULE_PORT (8099) and a private copy of fujinet-pc-rs232 on BoIP port BOIP
# (19981; its web UI on BOIP - 10000), appkeys seeded: name NAME (ARC), room
# ROOM (the local server's irata table). Then runs tools/arcsim.py on
# SCENARIO, throttled (the server's clock is wall time). Kills only what it
# started. Never the production server, never the everyday fujinet-pc on
# 9995. The copied fujinet-pc config loses its cloud sections (they hold
# tokens).
#
# The build must point at the same server: ENDPOINT=http://localhost:8099
# ./build.sh (or the room appkey says so, which wins). SLOW=ms puts the
# server behind a delay proxy on MULE_PORT+1.
set -euo pipefail
cd "$(dirname "$0")/.."
HERE=$(pwd)
SCEN=$1
OUT=$2
SECS=${3:-120}
MULE_PORT=${MULE_PORT:-8099}
BOIP=${BOIP:-19981}
NAME=${NAME-ARC}
ROOM=${ROOM-http://localhost:$MULE_PORT/?table=irata}
FNPC_DIST=${FNPC_DIST:-$HOME/Workspace/fujinet-pc-rs232/build/dist}
RIG="$HERE/build/rig"
SERVER_BIN="$RIG/muleserver"

for p in $MULE_PORT $BOIP $((BOIP - 10000)); do
    if ss -ltn | grep -q ":$p "; then echo "rig: port $p is taken" >&2; exit 1; fi
done
pids=()
cleanup() {
    for p in "${pids[@]}"; do kill -9 "$p" 2>/dev/null || true; done
}
trap cleanup EXIT

mkdir -p "$RIG"
( cd "${SERVER_SRC:-../..}" && go build -o "$SERVER_BIN" ./cmd/muleserver )
PORT=$MULE_PORT "$SERVER_BIN" -stream "" > "$RIG/server.log" 2>&1 &
pids+=($!)
if [ -n "${SLOW:-}" ]; then
    python3 ../atari-2600/test/slowproxy.py $((MULE_PORT + 1)) $MULE_PORT "$SLOW" \
        > "$RIG/proxy.log" 2>&1 &
    pids+=($!)
    ROOM=${ROOM_SLOW:-http://localhost:$((MULE_PORT + 1))/?table=irata}
fi

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

ARCSIM_FUJINET="127.0.0.1:$BOIP" ARCSIM_SECONDS=$SECS ARCSIM_THROTTLE=1 \
    python3 tools/arcsim.py "$SCEN" "$OUT" "${@:4}"

#!/usr/bin/env bash
# rig.sh -- a private server and FujiNet for a live run, then a scenario.
#
#   test/rig.sh SCENARIO OUTDIR [SECS [IMAGE]]
#
# Starts the M.U.L.E. server (built from this repo) on MULE_PORT (8098) and a
# private copy of fujinet-pc-rs232 on BoIP port BOIP (19993), with its
# appkeys seeded: name NAME (O2), room ROOM (the local server; ROOM= with
# ?table=ai3 plays alone against three computers). Then runs tools/o2sim.py
# on SCENARIO against it, throttled (the server's clock is wall time).
# Kills only what it started. Never the production server, never the
# everyday fujinet-pc on 9995.
#
# The build must point at the same server: ENDPOINT=http://localhost:8098
# ./build.sh (or the room appkey says so, which wins). SERVER_SRC=dir builds
# the server from a copy instead (a short game: NumRounds patched).
set -euo pipefail
cd "$(dirname "$0")/.."
HERE=$(pwd)
SCEN=$1
OUT=$2
SECS=${3:-120}
MULE_PORT=${MULE_PORT:-8098}
BOIP=${BOIP:-19993}
NAME=${NAME-O2}
ROOM=${ROOM-http://localhost:$MULE_PORT/}
FNPC_DIST=${FNPC_DIST:-$HOME/Workspace/fujinet-pc-rs232/build/dist}
RIG="$HERE/build/rig"
SERVER_BIN="$RIG/muleserver"

pids=()
cleanup() {
    for p in "${pids[@]}"; do kill "$p" 2>/dev/null || true; done
}
trap cleanup EXIT

mkdir -p "$RIG"
( cd "${SERVER_SRC:-../..}" && go build -o "$SERVER_BIN" ./cmd/muleserver )
PORT=$MULE_PORT "$SERVER_BIN" -stream "" > "$RIG/server.log" 2>&1 &
pids+=($!)
# SLOW=ms: the server behind ../atari-2600/test/slowproxy.py on MULE_PORT+1,
# replies late and in pieces (the internet's way); the room points there.
if [ -n "${SLOW:-}" ]; then
    python3 ../atari-2600/test/slowproxy.py $((MULE_PORT + 1)) $MULE_PORT "$SLOW" \
        > "$RIG/proxy.log" 2>&1 &
    pids+=($!)
    ROOM=${ROOM_SLOW:-http://localhost:$((MULE_PORT + 1))/}
fi

rm -rf "$RIG/fn"; mkdir -p "$RIG/fn"
# the binary by link, the rest copied: its config is ours to change
ln -s "$FNPC_DIST/fujinet" "$RIG/fn/fujinet"
cp -a "$FNPC_DIST/data" "$FNPC_DIST/fnconfig.ini" "$RIG/fn"/
mkdir -p "$RIG/fn/SD/FujiNet"
python3 - "$RIG/fn/fnconfig.ini" "$BOIP" <<'PY'
import re, sys
s = open(sys.argv[1]).read()
s = re.sub(r"(\[BOIP\][^\[]*?\bport=)\d*", r"\g<1>" + sys.argv[2], s, flags=re.S)
s = re.sub(r"(\[BOIP\][^\[]*?\benabled=)\d*", r"\g<1>1", s, flags=re.S)
open(sys.argv[1], "w").write(s)
PY
[ -n "$NAME" ] && printf '%s' "$NAME" > "$RIG/fn/SD/FujiNet/00010100.key"
[ -n "$ROOM" ] && printf '%s' "$ROOM" > "$RIG/fn/SD/FujiNet/0001010a.key"
( cd "$RIG/fn" && exec ./fujinet -u http://127.0.0.1:$((BOIP - 10000)) -c fnconfig.ini -s SD ) \
    > "$RIG/fn.log" 2>&1 &
pids+=($!)
sleep 2
if ! ss -ltn | grep -q ":$BOIP "; then echo "rig: no BoIP listener on $BOIP" >&2; exit 1; fi

O2SIM_FUJINET="127.0.0.1:$BOIP" O2SIM_SECONDS=$SECS \
    python3 tools/o2sim.py "$SCEN" "$OUT" "${@:4}"

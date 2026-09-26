#!/usr/bin/env bash
# rig.sh -- a private server and FujiNet for a live run, then a scenario.
#
#   test/rig.sh SCENARIO [SECS]      (SCENARIO "auto": emu/auto.lua plays)
#
# Starts the M.U.L.E. server (built from this repo) on MULE_PORT (8096) and a
# private copy of fujinet-pc-rs232 on BoIP port BOIP (19996), with its
# appkeys seeded: name NAME (VCS), room ROOM (the local server, no table). Then
# runs MAME headless with emu/sim.lua on SCENARIO, throttled (the server's
# clock is wall time). Kills only what it started.
set -euo pipefail
cd "$(dirname "$0")/.."
HERE=$(pwd)
SCEN=$1
SECS=${2:-120}
MULE_PORT=${MULE_PORT:-8096}
BOIP=${BOIP:-19996}
NAME=${NAME-VCS}
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
( cd ../.. && go build -o "$SERVER_BIN" ./cmd/muleserver )
PORT=$MULE_PORT "$SERVER_BIN" -stream "" > "$RIG/server.log" 2>&1 &
pids+=($!)
# SLOW=ms: the server behind test/slowproxy.py on MULE_PORT+1, replies late
# and in pieces (the internet's way); the room points there.
if [ -n "${SLOW:-}" ]; then
    python3 test/slowproxy.py $((MULE_PORT + 1)) $MULE_PORT "$SLOW" > "$RIG/proxy.log" 2>&1 &
    pids+=($!)
    ROOM=${ROOM_SLOW:-http://localhost:$((MULE_PORT + 1))/}
fi

rm -rf "$RIG/fn"; mkdir -p "$RIG/fn"; cp -a "$FNPC_DIST"/. "$RIG/fn"/
python3 - "$RIG/fn/fnconfig.ini" "$BOIP" <<'PY'
import re, sys
s = open(sys.argv[1]).read()
s = re.sub(r"(\[BOIP\][^\[]*?\bport=)\d*", r"\g<1>" + sys.argv[2], s, flags=re.S)
s = re.sub(r"(\[BOIP\][^\[]*?\benabled=)\d*", r"\g<1>1", s, flags=re.S)
open(sys.argv[1], "w").write(s)
PY
rm -f "$RIG/fn/SD/FujiNet/00010100.key" "$RIG/fn/SD/FujiNet/0001010a.key"
[ -n "$NAME" ] && printf '%s' "$NAME" > "$RIG/fn/SD/FujiNet/00010100.key"
[ -n "$ROOM" ] && printf '%s' "$ROOM" > "$RIG/fn/SD/FujiNet/0001010a.key"
( cd "$RIG/fn" && exec ./fujinet ) > "$RIG/fn.log" 2>&1 &
pids+=($!)
sleep 2
if grep -q "bind failed" "$RIG/fn.log"; then echo "rig: BoIP $BOIP taken" >&2; exit 1; fi

# A scenario file, or "auto": emu/auto.lua plays by itself.
LUA=sim
[ "$SCEN" = auto ] && LUA=auto
M26_SCEN="$HERE/$SCEN" SECS=$SECS FUJINET_TCP="127.0.0.1:$BOIP" \
    ./run.sh $LUA < /dev/null 2>&1 | grep -v "^Average speed" || true

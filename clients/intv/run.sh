#!/bin/sh
# run.sh -- build (if needed) and start M.U.L.E. in the FujiNet-patched
# jzIntv, talking to a fujinet-firmware (fujinet-pc, RS232/BoIP build).
#
#   ./run.sh                      # FujiNet at localhost:9995
#   FUJINET_TARGET=localhost:9997 ./run.sh --fujinet-debug
#
# Extra arguments go straight to jzIntv.
set -e
cd "$(dirname "$0")"
export SDL_AUDIODRIVER=${SDL_AUDIODRIVER:-pulseaudio}
JZINTV_DIR=${JZINTV_DIR:-$HOME/Workspace/jzintv-20200712-src}
if [ -z "$JZINTV" ]; then
    JZINTV=$JZINTV_DIR/bin/jzintv
    [ -x "$JZINTV" ] || JZINTV=$JZINTV_DIR/bin/linux/jzintv
fi
EXEC_BIN=${EXEC_BIN:-$JZINTV_DIR/rom/exec.bin}
GROM_BIN=${GROM_BIN:-$JZINTV_DIR/rom/grom.bin}
FUJINET_TARGET=${FUJINET_TARGET:-localhost:9995}

if [ ! -x "$JZINTV" ]; then
    echo "jzIntv not found at $JZINTV; set JZINTV_DIR or JZINTV." >&2
    exit 1
fi
make
exec "$JZINTV" -z 4 -e "$EXEC_BIN" -g "$GROM_BIN" --fujinet="$FUJINET_TARGET" "$@" mule.rom

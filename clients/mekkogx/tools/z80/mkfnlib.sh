#!/bin/sh
# mkfnlib.sh SRC OUT PLATFORM -- build fujinet-lib-experimental (SRC) for a
# z88dk platform (adam, msx, sms or h89) with zsdcc, the game's compiler, into
# OUT/fujinet-PLATFORM.lib, headers in OUT/include. The released libs are
# sccz80 builds. Sources are copied in first so the container can see them,
# and the copies get these local fixes:
#   - bus/msx: portio.s and timeout.s are replaced by tools/z80/msx-portio.c
#     (the asm uses IX and IY, which zsdcc keeps, and port_get_until left a
#     word on the stack when it timed out); VDP_IS_PAL now reads the byte at
#     002Bh (it tested the address).
#   - bus/msx: network_bus_read/write use the renamed NETCMD_READ/WRITE and
#     report a failed call as 0 bytes, as bus/msdos does; it has no appkey
#     calls, so bus/msdos's (FujiBus too) are added.
#   - bus/msx: fuji_bus_call reads its 8-bit aux arguments with
#     va_arg(ap, uint8_t), but zsdcc passes a uint8_t variable promoted to
#     int and a value cast to (uint8_t) as one byte (as NATIVE_SPLIT_U16
#     makes them), so the rest were read from the wrong place (the network
#     OPEN copied from address 0 over RAM; a READ asked for 19750 bytes).
#     Every aux argument is now passed as an int (ezvar.h, fujinet-endian.h)
#     and read as (uint8_t) va_arg(ap, int).
#   - msx, sms: network_open sends the URL with its NUL (NETWORK_OPEN_LEN
#     is strlen + 1): the firmware (rs232) takes the string without a
#     length and ran on into what its buffer held before ("&bin=1=1"). The
#     SMS cartridge hands its requests to the same rs232 code. (Firmware
#     that zeroes the buffer first trims the NUL.)
#   - sms: SRC is the add-sms branch (fujinet-lib-experimental's worktree
#     ~/Workspace/fnlib-sms); its bus is plain stores to the cartridge's
#     mailbox, and fuji_bus_call has fixed arguments (no va_arg fix).
#   - h89: SRC is fujinet-lib (its bus/h89: FujiBus over the H89-ESP32
#     card's 82C55, fixed-argument fuji_bus_call); the card runs the rs232
#     code, so the URL goes with its NUL too. Its fuji_bus_call.c has a
#     static copy of common/fuji_field.c's table, which the header declares
#     extern (sccz80 lets it pass, zsdcc doesn't): the copy goes.
#   - no <stdio.h> (debug only).
set -e
src=$1
out=$2
plat=$3
case $plat in
    adam) target="+coleco -subtype=adam"; define=BUILD_ADAM ;;
    msx)  target="+msx -subtype=rom";     define=BUILD_MSX ;;
    sms)  target="+sms";                  define=BUILD_SMS ;;
    h89)  target="+cpm";                  define="BUILD_H89 -D__H89__" ;;
    *) echo "mkfnlib.sh: unknown platform $plat" >&2; exit 1 ;;
esac
z88dk=$(dirname "$0")/z88dk
rm -rf "$out"
mkdir -p "$out/src" "$out/obj" "$out/include"
cp "$src"/common/*.c "$out/src/"
cp "$src"/bus/$plat/* "$out/src/"
[ "$plat" = msx ] && cp "$src"/bus/msdos/appkey.c "$out/src/"
cp -r "$src"/include/. "$out/include/"
( cd "$src" && git rev-parse --short HEAD 2>/dev/null || echo unknown ) > "$out/VERSION"
if [ "$plat" = msx ]; then
    rm -f "$out/src/portio.s" "$out/src/timeout.s"
    cp "$(dirname "$0")/msx-portio.c" "$out/src/portio.c"
    cat > "$out/src/portio.h" <<'H'
/* portio.h, for tools/z80/msx-portio.c (mkfnlib.sh). */
#include <stdint.h>
extern int port_getc(void);
extern int port_getc_timeout(uint16_t timeout);
extern int port_discard_until(uint8_t c, uint16_t timeout);
extern uint16_t port_getbuf(void *buf, uint16_t len, uint16_t timeout);
extern uint16_t port_get_until(void *buf, uint16_t maxlen, uint8_t c, uint16_t timeout);
extern void port_putc(uint8_t c);
extern uint16_t port_putbuf(void *buf, uint16_t len);
#define VDP_IS_PAL (*(unsigned char *) 0x002b & 0x80)
H
    sed -i -e 's/NETCALL_B12_RV(FUJICMD_READ,/if (!NETCALL_B12_RV(NETCMD_READ,/' \
           -e 's/NETCALL_B12_D(FUJICMD_WRITE,/if (!NETCALL_B12_D(NETCMD_WRITE,/' \
           -e 's/^\(  if (!NETCALL_B12_[RVD]*(.*, buffer, length)\);$/\1)\n    return 0;/' \
           -e 's/va_arg(ap, uint8_t)/(uint8_t) va_arg(ap, int)/' \
           "$out/src/fujinet-bus-msx.c"
    python3 - "$out/include" <<'P'
import re, sys
d = sys.argv[1]
p = d + "/fujinet-bus-ezvar.h"
s = open(p).read()
s = "\n".join(re.sub(r"\b(a[1-4])\b", r"(int)(\1)", l) if "fuji_bus_call(" in l or l.lstrip().startswith(("a", "(")) and "reply" not in l else l
              for l in s.split("\n"))
open(p, "w").write(s)
p = d + "/fujinet-endian.h"
s = open(p).read()
s = s.replace("#define NATIVE_SPLIT_U16(w) U16_LSB(w), U16_MSB(w)",
              "#define NATIVE_SPLIT_U16(w) (int)U16_LSB(w), (int)U16_MSB(w)")
s = s.replace("  U16_LSB(U32_LSW(l)), U16_MSB(U32_LSW(l)),                             \\\n"
              "    U16_LSB(U32_MSW(l)), U16_MSB(U32_MSW(l))",
              "  (int)U16_LSB(U32_LSW(l)), (int)U16_MSB(U32_LSW(l)),                   \\\n"
              "    (int)U16_LSB(U32_MSW(l)), (int)U16_MSB(U32_MSW(l))")
open(p, "w").write(s)
P
fi
if [ "$plat" = h89 ]; then
    sed -i '/^static const uint8_t fuji_field_numbytes_table\[\]/d' "$out/src/fuji_bus_call.c"
fi
if [ "$plat" = msx ] || [ "$plat" = sms ] || [ "$plat" = h89 ]; then
    sed -i 's/^#define NETWORK_OPEN_LEN strlen(devicespec)$/#define NETWORK_OPEN_LEN (strlen(devicespec) + 1)/' \
        "$out/include/fujinet-network.h"
    grep -q 'strlen(devicespec) + 1' "$out/include/fujinet-network.h" || {
        echo "mkfnlib.sh: NETWORK_OPEN_LEN not patched" >&2; exit 1; }
fi
sed -i -e '/#include <stdio.h>/d' "$out"/src/*.c
cmds=
objs=
for c in "$out"/src/*.c "$out"/src/*.s; do
    [ -e "$c" ] || continue
    o="$out/obj/$(basename "$c").o"
    objs="$objs $o"
    cmds="$cmds zcc $target -compiler=sdcc -SO3 -D$define -I$out/include -I$out/src -c -o $o $c || exit 1;"
done
cmds="$cmds z80asm -x$out/fujinet-$plat.lib $objs"
"$z88dk" sh -c "$cmds"

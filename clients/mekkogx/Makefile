# clients/mekkogx/Makefile -- M.U.L.E. for FujiNet 8-bit computers (MekkoGX).
#
#   make apple2                        r2r/apple2/mule.po (ProDOS, 128K //e, IIc, IIgs)
#   make coco                          r2r/coco/mule.dsk (DECB, 64K CoCo 1/2)
#   make msdos                         r2r/msdos/mule.img (MULE.EXE: 128K PC with CGA, or PCjr)
#   make adam                          r2r/adam/mule.ddp and mule.dsk (Coleco Adam)
#   make msxrom                        r2r/msxrom/mule.rom (MSX, a FujiNet user ROM: ASCII8)
#   make atari                         r2r/atari/mule.atr (48K Atari 400/800, XL/XE)
#   make lynx                          r2r/lynx/mule.lnx (Atari Lynx cart, FujiNet-Lynx)
#   make ATR="path/to/M.U.L.E. (1983)(Electronic Arts)(US).atr"
#                                      graphics from the original disk (never committed)
#   make ENDPOINT=http://host:8080     server used when the lobby gave none (remembered)
#   make AUTOPLAY=1                    plays by itself (soak tests; rebuild clean)
#
# src/ is platform neutral; src/platform-specific/*.h is the API each platform
# implements in src/<platform>/. Generated files go to gen/ (gitignored).

PRODUCT = mule
PLATFORMS = apple2 coco msdos adam msxrom atari lynx

SRC_DIRS = src src/%PLATFORM% gen/%PLATFORM%
EXTRA_INCLUDE = src src/platform-specific .. gen

# fujinet-lib from the sibling checkout (prebuilt libs in its build/).
FUJINET_LIB ?= $(HOME)/Workspace/fujinet-lib

PLATFORM_COMBOS = \
  apple2enh+=apple2 \
  c64+=commodore \
  atarixe+=atari \
  msxrom+=tms9918,msx \
  msxdos+=msx \
  adam_cpm+=adam \
  adam+=tms9918

# ---- Apple II -------------------------------------------------------------
# HGR page 1 sits at $2000-$3FFF: the linker config keeps code and data out
# of it. ProDOS stays resident (overlays and assets are read with it).
# Stream mode over an N:TCP socket (src/nsnet.c, src/nstrans_fn.c): an HTTP
# poll held the machine for the whole request.
CFLAGS_EXTRA_APPLE2 += -Os -Cl -DPLAT_NETSTREAM
ifeq ($(AUTOPLAY),1)
  CFLAGS_EXTRA += -DMULE_AUTOPLAY
endif
LDFLAGS_EXTRA_APPLE2 += -C src/apple2/mule-hgr.cfg -m build/mule-apple2.map
EXECUTABLE_EXTRA_DEPS_APPLE2 += src/apple2/mule-hgr.cfg
DISK_BASIC_SYSTEM =

include mekkogx/toplevel-rules.mk

# ---- Apple II memory: where each module goes (see src/apple2/mule-hgr.cfg)
# Phases are overlays kept in auxiliary RAM; the strings live there too.
A2OBJ = build/$(PRODUCT)/apple2/src
$(A2OBJ)/boot.o:    CFLAGS += --code-name OVERLAY1 --rodata-name OVERLAY1
$(A2OBJ)/teststart.o: CFLAGS += --code-name OVERLAY1 --rodata-name OVERLAY1
$(A2OBJ)/status.o:  CFLAGS += --code-name OVERLAY2 --rodata-name OVERLAY2
$(A2OBJ)/grant.o:   CFLAGS += --code-name OVERLAY3 --rodata-name OVERLAY3
$(A2OBJ)/events.o:  CFLAGS += --code-name OVERLAY3 --rodata-name OVERLAY3
$(A2OBJ)/auction.o: CFLAGS += --code-name OVERLAY4 --rodata-name OVERLAY4
$(A2OBJ)/develop.o: CFLAGS += --code-name OVERLAY5 --rodata-name OVERLAY5
$(A2OBJ)/strings.o: CFLAGS += --rodata-name AUXTXT
$(A2OBJ)/text.o:    CFLAGS += --code-name LOWCODE
$(A2OBJ)/map.o:     CFLAGS += --code-name LOWCODE
$(A2OBJ)/fx.o:      CFLAGS += --code-name LC
$(A2OBJ)/apple2/input.o: CFLAGS += --code-name LC
$(A2OBJ)/apple2/sound.o: CFLAGS += --code-name LC
$(A2OBJ)/apple2/plat.o:  CFLAGS += --code-name LC

# The overlays and aux data go on the disk as MULE.1 .. MULE.7 (binary).
apple2/disk-post::
	for n in 1 2 3 4 5 6 7; do \
	  ac -p $(DISK) MULE.$$n BIN 0x0000 < $(EXECUTABLE).$$n || exit 1; done

# ---- CoCo 1/2 ---------------------------------------------------------------
# 64K, all-RAM once running (src/coco/bank.c). CMOC has no section pragma:
# tools/coco/lwasm-sect moves a module's code and constants to the section
# named by MULE_SECT, and tools/coco/lwlink-mule.py links with
# src/coco/mule.link and splits the result into mule.bin (LOADM'd),
# mulehi.bin and the overlays mule1..5.bin.
COCO_TOOLS = $(CURDIR)/tools/coco
# -O1: CMOC 0.1.100's -O2 drops index reloads (a[i]; b[i]; read b[a[i]])
# Stream mode over N:TCP; a status call only every other frame (DriveWire
# masks the IRQ while it talks).
CFLAGS_EXTRA_COCO += -O1 -funsigned-char -fomit-frame-pointer -Wno-const -DPLAT_BIG_ENDIAN \
                     -DPLAT_NETSTREAM -DNT_POLL_EVERY=2 \
                     --lwasm=$(COCO_TOOLS)/lwasm-sect
ASFLAGS_EXTRA_COCO += --lwasm=$(COCO_TOOLS)/lwasm-sect
LDFLAGS_EXTRA_COCO += --lwlink=$(COCO_TOOLS)/lwlink-mule.py --initial-s=8000
CFLAGS_EXTRA_COCO += -DKEY_END_NAME='"ENTER"' -DQUIT_HINT='"BREAK: QUIT"' -DLEAVE_KEY_NAME='"BREAK"' \
                     -DLOBBY_HINT='"<- -> SPECIES  SPACE READY  BREAK LEAVE"'
EXTRA_INCLUDE_COCO += src/coco/include
EXECUTABLE_EXTRA_DEPS_COCO += src/coco/mule.link $(COCO_TOOLS)/lwlink-mule.py
DECB ?= $(firstword $(wildcard $(HOME)/Workspace/toolshed/build/unix/decb/decb) decb)
ifeq ($(PLATFORM),coco)
  DISK_TOOL := $(DECB)
endif

COOBJ = build/$(PRODUCT)/coco
$(COOBJ)/src/boot.o $(COOBJ)/src/teststart.o: export MULE_SECT = ov1
$(COOBJ)/src/status.o:                        export MULE_SECT = ov2
$(COOBJ)/src/grant.o $(COOBJ)/src/events.o:   export MULE_SECT = ov3
$(COOBJ)/src/auction.o:                       export MULE_SECT = ov4
$(COOBJ)/src/develop.o:                       export MULE_SECT = ov5
$(COOBJ)/src/text.o $(COOBJ)/src/map.o $(COOBJ)/src/fx.o $(COOBJ)/src/test.o \
  $(COOBJ)/src/nstrans_fn.o \
  $(COOBJ)/src/coco/gfx.o $(COOBJ)/src/coco/input.o \
  $(COOBJ)/src/coco/sound.o $(COOBJ)/gen/coco/gfx.o:   export MULE_SECT = hi

# The loader: HDB-DOS runs AUTOEXEC.BAS, which LOADMs the low core. BASIC
# programs start at $2601, where MULE.BIN goes: PCLEAR1 moves this one down
# out of the way first (to $1601, the future PMODE 4 screen).
coco/disk-post::
	printf '10 PCLEAR1:LOADM"MULE":EXEC\n' > build/autoexec.bas
	$(DECB) copy -0 -a -l -r build/autoexec.bas $(DISK),AUTOEXEC.BAS
	$(DECB) copy -b -2 -r r2r/coco/$(PRODUCT)hi.bin $(DISK),MULEHI.BIN
	for n in 1 2 3 4 5; do \
	  $(DECB) copy -b -2 -r r2r/coco/$(PRODUCT)$$n.bin $(DISK),MULE$$n.BIN || exit 1; done

# ---- MS-DOS (CGA; PCjr 16 colours) ------------------------------------------
# Open Watcom, medium model (far code, near data): the phases are overlays
# (tools/dos/wlink-mule.py) so the game fits a 128K PCjr in its 16-colour
# mode. Watcom runs from defoogi unless installed (tools/dos/ow). fujinet-lib
# is fujinet-lib-experimental, rebuilt medium model (tools/dos/mkfnlib.sh).
# FUJINET.SYS may run at 9600 bps (a PCjr): a status call is 25 ms there, so
# the socket is asked every 6th frame, and the server is told the link is
# slow (PLAT_NS_SLOW: a push at most every 300 ms).
FNLIB_EXP ?= $(HOME)/Workspace/fujinet-lib-experimental
DOS_TOOLS = $(CURDIR)/tools/dos
ifeq ($(PLATFORM),msdos)
  FUJINET_LIB := __UNDEFINED__
  CC_OW2 = $(DOS_TOOLS)/ow wcc
  AS_OW2 = $(DOS_TOOLS)/ow wasm
  AR_OW2 = $(DOS_TOOLS)/ow wlib
  LD_OW2 = $(DOS_TOOLS)/wlink-mule.py OPTION quiet
endif
CFLAGS_EXTRA_MSDOS += -zq -mm -os -s -of -zp1 -DKEY_END_NAME='"ENTER"' -DPLAT_NET_PROBE -DPLAT_NETSTREAM \
                      -DPLAT_NET_GUARD -DQUIT_HINT='"ESC: LOBBY"' -DNT_POLL_EVERY=6 -DPLAT_NS_SLOW -DNT_BUS_READ
EXTRA_INCLUDE_MSDOS += build/fnlib-msdos/include
# (-ad names the object in the .d without its directory: -adt names it)
CFLAGS_EXTRA_MSDOS += -adt=$@
EXECUTABLE_EXTRA_DEPS_MSDOS += $(DOS_TOOLS)/wlink-mule.py

msdos/disk-post::
	printf '@ECHO OFF\r\nMULE.EXE\r\n' > build/autoexec.bat
	mcopy -o -i $(DISK) build/autoexec.bat "::AUTOEXEC.BAT"

# A boot floppy with the game on it, for a PCjr whose MULE.EXE would
# otherwise come over FUJINET.SYS (an overlay, 4.6K, is 5 s at 9600 bps,
# and the 128K PCjr reloads one at every phase change). PCJR_BOOT is a DOS
# disk with FUJINET.SYS and a CONFIG.SYS for the PCjr: the 86Box rig's.
PCJR_BOOT ?= $(HOME)/Vintage/IBM 5160 PCXT/fn-dos21.img
.PHONY: pcjr-floppy
pcjr-floppy: r2r/msdos/$(PRODUCT)
	cp "$(PCJR_BOOT)" r2r/msdos/pcjr-boot.img
	mcopy -o -i r2r/msdos/pcjr-boot.img r2r/msdos/mule.exe ::MULE.EXE
	mcopy -o -i r2r/msdos/pcjr-boot.img build/autoexec.bat ::AUTOEXEC.BAT
	@echo "r2r/msdos/pcjr-boot.img: boot it (fdd_01_fn in 86box.cfg) and MULE runs from A:"

# ---- Coleco Adam (and the MSX): z88dk --------------------------------------
# zcc with zsdcc, from defoogi unless installed (tools/z80/z88dk). The
# TMS9918A code is src/tms9918 (a PLATFORM_COMBOS entry). fujinet-lib is
# fujinet-lib-experimental built with zsdcc (tools/z80/mkfnlib.sh).
Z80_TOOLS = $(CURDIR)/tools/z80
ifneq ($(filter adam msxrom,$(PLATFORM)),)
  FUJINET_LIB := __UNDEFINED__
  CC_Z88DK = $(Z80_TOOLS)/z88dk zcc
  AS_Z88DK = $(Z80_TOOLS)/z88dk z80asm
  LD_Z88DK = $(Z80_TOOLS)/z88dk zcc
endif
# (zcc hands the defines on through a shell: a string is quoted twice)
CFLAGS_EXTRA_ADAM += -compiler=sdcc -SO3 -DPLAT_NET_PROBE -DPLAT_NETSTREAM -DQUIT_HINT='"\"ESCAPE: QUIT\""' -DLEAVE_KEY_NAME='"\"ESCAPE\""'
EXTRA_INCLUDE_ADAM += src/tms9918 build/fnlib-adam/include
# The memory map: src/adam/mule.asm. The NMI is the frame clock
# (src/adam/nmi.asm); the variables are not part of the image.
LDFLAGS_EXTRA_ADAM += -compiler=sdcc -m -pragma-define:CRT_ENABLE_NMI=2 \
                      -pragma-define:CRT_INITIALIZE_BSS=1
ifeq ($(PLATFORM),adam)
  LIBS += -Lbuild/fnlib-adam -lfujinet-adam
endif
EXECUTABLE_EXTRA_DEPS_ADAM += build/fnlib-adam/fujinet-adam.lib src/adam/mule.asm

# The phases are overlays (src/adam/bank.c), linked at the same address.
ADOBJ = build/$(PRODUCT)/adam
$(ADOBJ)/src/boot.o $(ADOBJ)/src/teststart.o: CFLAGS += --codeseg=OV1 --constseg=OV1
$(ADOBJ)/src/status.o:                        CFLAGS += --codeseg=OV2 --constseg=OV2
$(ADOBJ)/src/grant.o $(ADOBJ)/src/events.o:   CFLAGS += --codeseg=OV3 --constseg=OV3
$(ADOBJ)/src/auction.o:                       CFLAGS += --codeseg=OV4 --constseg=OV4
$(ADOBJ)/src/develop.o:                       CFLAGS += --codeseg=OV5 --constseg=OV5

# The overlays go on the tape (and a disk) after the program.
adam/r2r-post::
	python3 tools/adam/mkimage.py r2r/adam

# ---- MSX: a ROM the FujiNet cartridge runs --------------------------------
# An ASCII8 megaROM (src/msx/mule.asm): the program fixed at 4000h-9FFFh,
# the phases, the art and the strings taking turns at A000h.
CFLAGS_EXTRA_MSXROM += -compiler=sdcc -SO3 -DPLAT_NET_PROBE -DKEY_END_NAME='"\"RETURN\""'
EXTRA_INCLUDE_MSXROM += src/tms9918 build/fnlib-msx/include
LDFLAGS_EXTRA_MSXROM += -compiler=sdcc -m -pragma-define:CRT_MSX_CUSTOM_HEADER=1 -Ca-I$(CURDIR)/src/msx/crt
ifeq ($(PLATFORM),msxrom)
  LIBS += -Lbuild/fnlib-msx -lfujinet-msx
endif
EXECUTABLE_EXTRA_DEPS_MSXROM += build/fnlib-msx/fujinet-msx.lib src/msx/crt/msx_rom_header.asm
MXOBJ = build/$(PRODUCT)/msxrom
$(MXOBJ)/src/boot.o $(MXOBJ)/src/teststart.o: CFLAGS += --codeseg=OV1 --constseg=OV1
$(MXOBJ)/src/status.o:                        CFLAGS += --codeseg=OV2 --constseg=OV2
$(MXOBJ)/src/grant.o $(MXOBJ)/src/events.o:   CFLAGS += --codeseg=OV3 --constseg=OV3
$(MXOBJ)/src/auction.o:                       CFLAGS += --codeseg=OV4 --constseg=OV4
$(MXOBJ)/src/develop.o:                       CFLAGS += --codeseg=OV5 --constseg=OV5
$(MXOBJ)/src/strings.o:                       CFLAGS += --constseg=STR
$(MXOBJ)/gen/tms9918/gfx.o:                   CFLAGS += --constseg=ART

msxrom/r2r-post::
	python3 tools/msx/mkrom.py r2r/msxrom

# ---- Atari 400/800 (48K), XL/XE -------------------------------------------
# cc65, no DOS: picoboot loads MULE.COM, whose first chunk (src/atari/
# start.s) switches BASIC off; the phases are overlays PHASE.1 .. PHASE.5 read
# by sector (src/atari/bank.c). Memory: src/atari/mule-atari.cfg.
CFLAGS_EXTRA_ATARI += -Os -Cl -DPLAT_NETSTREAM
LDFLAGS_EXTRA_ATARI += -C src/atari/mule-atari.cfg -m build/mule-atari.map
EXECUTABLE_EXTRA_DEPS_ATARI += src/atari/mule-atari.cfg
ifeq ($(PLATFORM),atari)
  # jhallen/atari-tools' atr, which mekkogx/platforms/atari.mk wants
  export PATH := $(HOME)/Workspace/atari-tools:$(PATH)
endif

ATOBJ = build/$(PRODUCT)/atari/src
$(ATOBJ)/boot.o $(ATOBJ)/teststart.o: CFLAGS += --code-name OVERLAY1 --rodata-name OVERLAY1
$(ATOBJ)/status.o:                    CFLAGS += --code-name OVERLAY2 --rodata-name OVERLAY2
$(ATOBJ)/grant.o $(ATOBJ)/events.o:   CFLAGS += --code-name OVERLAY3 --rodata-name OVERLAY3
$(ATOBJ)/auction.o:                   CFLAGS += --code-name OVERLAY4 --rodata-name OVERLAY4
$(ATOBJ)/develop.o:                   CFLAGS += --code-name OVERLAY5 --rodata-name OVERLAY5
# variables in low memory ($0400-$09FF, cleared by src/atari/sprite.s)
$(ATOBJ)/text.o $(ATOBJ)/strings.o:   CFLAGS += --bss-name LOWBSS
$(ATOBJ)/atari/gfx.o $(ATOBJ)/atari/bank.o: CFLAGS += --bss-name BSS2

# The overlays go on the disk beside the program as PHASE.1 .. PHASE.5
# (after MULE.COM: picoboot runs the first file): rebuild it with them.
atari/disk-post::
	for n in 1 2 3 4 5; do cp $(EXECUTABLE).$$n $(CACHE_PLATFORM)/disk/PHASE.$$n || exit 1; done
	$(RM) $(DISK)
	$(DISK_TOOL) $(DISK_TOOL_FLAGS_ATARI) $(DISK) $(CACHE_PLATFORM)/disk

# ---- Atari Lynx ---------------------------------------------------------------
# cc65, a cart image (.lnx): the program at $0200, the phases as overlays in
# the cart's directory loaded into a window at the top of RAM (src/lynx/
# mule-lynx.cfg, bank.c). FujiNet-Lynx on ComLynx through fujinet-lib-
# experimental (tools/lynx/mkfnlib.sh); the game then talks netstream: the
# stream can't be paused, so nothing else goes over the bus once it is on.
ifeq ($(PLATFORM),lynx)
  FUJINET_LIB := __UNDEFINED__
  LIBS += build/fnlib-lynx/fujinet-lynx.lib
endif
CFLAGS_EXTRA_LYNX += -Os -Cl -DPLAT_NETSTREAM -DPLAT_NS_ONEWAY -DPLAT_NO_KEYBOARD \
                     -DKEY_END_NAME='"OPT1"' -DBUTTON_NAME='"A"' -DLEAVE_KEY_NAME='"OPT2"' -DQUIT_HINT='"OPT2: QUIT"' \
                     -DLOBBY_HINT='"<- -> SPECIES   A READY   OPT2 LEAVE"'
EXTRA_INCLUDE_LYNX += build/fnlib-lynx/include
LDFLAGS_EXTRA_LYNX += -C src/lynx/mule-lynx.cfg -m build/mule-lynx.map
EXECUTABLE_EXTRA_DEPS_LYNX += src/lynx/mule-lynx.cfg build/fnlib-lynx/fujinet-lynx.lib

LXOBJ = build/$(PRODUCT)/lynx/src
$(LXOBJ)/boot.o $(LXOBJ)/teststart.o: CFLAGS += --code-name OVERLAY1 --rodata-name OVERLAY1
$(LXOBJ)/status.o:                    CFLAGS += --code-name OVERLAY2 --rodata-name OVERLAY2
$(LXOBJ)/grant.o $(LXOBJ)/events.o:   CFLAGS += --code-name OVERLAY3 --rodata-name OVERLAY3
$(LXOBJ)/auction.o:                   CFLAGS += --code-name OVERLAY4 --rodata-name OVERLAY4
$(LXOBJ)/develop.o:                   CFLAGS += --code-name OVERLAY5 --rodata-name OVERLAY5

# ---- generated files (top-level pass only) ---------------------------------
ifeq ($(PLATFORM),)
PYTHON   ?= python3
ENDPOINT ?= $(shell cat gen/endpoint 2>/dev/null || echo https://mule.fujinet.online)
ATR      ?= $(wildcard mule.atr)

.PHONY: gen-apple2 gen-coco gen-msdos gen-adam gen-msxrom gen-atari gen-lynx FORCE
r2r/apple2/$(PRODUCT): gen-apple2
r2r/coco/$(PRODUCT): gen-coco
r2r/msdos/$(PRODUCT): gen-msdos
r2r/adam/$(PRODUCT): gen-adam
r2r/msxrom/$(PRODUCT): gen-msxrom
r2r/atari/$(PRODUCT): gen-atari
r2r/lynx/$(PRODUCT): gen-lynx

gen-apple2: gen/config.h gen/apple2/gfx.c
gen-coco: gen/config.h gen/coco/gfx.c
gen-msdos: gen/config.h gen/msdos/gfx.c build/fnlib-msdos/fujinet-msdos.lib
gen-adam: gen/config.h gen/tms9918/gfx.c build/fnlib-adam/fujinet-adam.lib
gen-msxrom: gen/config.h gen/tms9918/gfx.c build/fnlib-msx/fujinet-msx.lib
gen-atari: gen/config.h gen/atari/gfx.c
gen-lynx: gen/config.h gen/lynx/gfx.c build/fnlib-lynx/fujinet-lynx.lib

gen/config.h: FORCE
	@mkdir -p gen
	@echo "$(ENDPOINT)" > gen/endpoint.new
	@cmp -s gen/endpoint.new gen/endpoint || mv gen/endpoint.new gen/endpoint
	@rm -f gen/endpoint.new
	@printf '/* GENERATED by the Makefile. */\n#define DEFAULT_ENDPOINT "%s"\n' "$$(cat gen/endpoint)" > gen/config.h.new
	@cmp -s gen/config.h.new gen/config.h || mv gen/config.h.new gen/config.h
	@rm -f gen/config.h.new

gen/apple2/gfx.c: tools/mkgfx_hgr.py $(ATR)
	@mkdir -p gen/apple2
	@if [ -z "$(ATR)" ]; then \
	  echo "*** WARNING: no ATR (make ATR=... or a mule.atr link): using STAND-IN art"; fi
	$(PYTHON) tools/mkgfx_hgr.py $(if $(ATR),--atr "$(ATR)") --preview gen/preview $@

gen/coco/gfx.c: tools/mkgfx_pmode4.py tools/mkgfx_hgr.py $(ATR)
	@mkdir -p gen/coco
	@if [ -z "$(ATR)" ]; then \
	  echo "*** WARNING: no ATR (make ATR=... or a mule.atr link): using STAND-IN art"; fi
	$(PYTHON) tools/mkgfx_pmode4.py $(if $(ATR),--atr "$(ATR)") --preview gen/preview/coco $@

gen/msdos/gfx.c: tools/mkgfx_dos.py tools/tms/namco.bin tools/mkgfx_hgr.py $(ATR)
	@mkdir -p gen/msdos
	@if [ -z "$(ATR)" ]; then \
	  echo "*** WARNING: no ATR (make ATR=... or a mule.atr link): using STAND-IN art"; fi
	$(PYTHON) tools/mkgfx_dos.py $(if $(ATR),--atr "$(ATR)") --preview gen/preview/msdos $@

gen/tms9918/gfx.c: tools/mkgfx_tms.py tools/tms/namco.bin tools/mkgfx_dos.py tools/mkgfx_pmode4.py tools/mkgfx_hgr.py $(ATR)
	@mkdir -p gen/tms9918
	@if [ -z "$(ATR)" ]; then \
	  echo "*** WARNING: no ATR (make ATR=... or a mule.atr link): using STAND-IN art"; fi
	$(PYTHON) tools/mkgfx_tms.py $(if $(ATR),--atr "$(ATR)") --preview gen/preview/tms9918 $@

gen/atari/gfx.c: tools/mkgfx_atari.py tools/mkgfx_hgr.py $(ATR)
	@mkdir -p gen/atari
	@if [ -z "$(ATR)" ]; then \
	  echo "*** WARNING: no ATR (make ATR=... or a mule.atr link): using STAND-IN art"; fi
	$(PYTHON) tools/mkgfx_atari.py $(if $(ATR),--atr "$(ATR)") $@

gen/lynx/gfx.c: tools/mkgfx_lynx.py tools/mkgfx_dos.py tools/mkgfx_hgr.py $(ATR)
	@mkdir -p gen/lynx
	@if [ -z "$(ATR)" ]; then \
	  echo "*** WARNING: no ATR (make ATR=... or a mule.atr link): using STAND-IN art"; fi
	$(PYTHON) tools/mkgfx_lynx.py $(if $(ATR),--atr "$(ATR)") --preview gen/preview/lynx $@

build/fnlib-lynx/fujinet-lynx.lib: tools/lynx/mkfnlib.sh $(wildcard $(FNLIB_EXP)/common/*.c $(FNLIB_EXP)/bus/lynx/* $(FNLIB_EXP)/include/*.h)
	tools/lynx/mkfnlib.sh $(FNLIB_EXP) build/fnlib-lynx

build/fnlib-adam/fujinet-adam.lib: tools/z80/mkfnlib.sh $(wildcard $(FNLIB_EXP)/common/*.c $(FNLIB_EXP)/bus/adam/* $(FNLIB_EXP)/include/*.h)
	tools/z80/mkfnlib.sh $(FNLIB_EXP) build/fnlib-adam adam

build/fnlib-msx/fujinet-msx.lib: tools/z80/mkfnlib.sh tools/z80/msx-portio.c $(wildcard $(FNLIB_EXP)/common/*.c $(FNLIB_EXP)/bus/msx/* $(FNLIB_EXP)/include/*.h)
	tools/z80/mkfnlib.sh $(FNLIB_EXP) build/fnlib-msx msx

build/fnlib-msdos/fujinet-msdos.lib: tools/dos/mkfnlib.sh $(wildcard $(FNLIB_EXP)/common/*.c $(FNLIB_EXP)/bus/msdos/*.c $(FNLIB_EXP)/include/*.h)
	tools/dos/mkfnlib.sh $(FNLIB_EXP) build/fnlib-msdos

clean::
	rm -rf gen/apple2 gen/coco gen/msdos gen/tms9918 gen/atari gen/lynx gen/preview gen/config.h
endif

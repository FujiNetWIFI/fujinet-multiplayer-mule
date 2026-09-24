# clients/mekkogx/Makefile -- M.U.L.E. for FujiNet 8-bit computers (MekkoGX).
#
#   make apple2                        r2r/apple2/mule.po (ProDOS, 128K //e, IIc, IIgs)
#   make coco                          r2r/coco/mule.dsk (DECB, 64K CoCo 1/2)
#   make msdos                         r2r/msdos/mule.img (MULE.EXE: 128K PC with CGA, or PCjr)
#   make ATR="path/to/M.U.L.E. (1983)(Electronic Arts)(US).atr"
#                                      graphics from the original disk (never committed)
#   make ENDPOINT=http://host:8080     server used when the lobby gave none (remembered)
#   make AUTOPLAY=1                    plays by itself (soak tests; rebuild clean)
#
# src/ is platform neutral; src/platform-specific/*.h is the API each platform
# implements in src/<platform>/. Generated files go to gen/ (gitignored).

PRODUCT = mule
PLATFORMS = apple2 coco msdos

SRC_DIRS = src src/%PLATFORM% gen/%PLATFORM%
EXTRA_INCLUDE = src src/platform-specific .. gen

# fujinet-lib from the sibling checkout (prebuilt libs in its build/).
FUJINET_LIB ?= $(HOME)/Workspace/fujinet-lib

PLATFORM_COMBOS = \
  apple2enh+=apple2 \
  c64+=commodore \
  atarixe+=atari \
  msxrom+=msx \
  msxdos+=msx \
  adam_cpm+=adam

# ---- Apple II -------------------------------------------------------------
# HGR page 1 sits at $2000-$3FFF: the linker config keeps code and data out
# of it. ProDOS stays resident (overlays and assets are read with it).
CFLAGS_EXTRA_APPLE2 += -Os -Cl
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
CFLAGS_EXTRA_COCO += -O1 -funsigned-char -fomit-frame-pointer -Wno-const -DPLAT_BIG_ENDIAN \
                     --lwasm=$(COCO_TOOLS)/lwasm-sect
ASFLAGS_EXTRA_COCO += --lwasm=$(COCO_TOOLS)/lwasm-sect
LDFLAGS_EXTRA_COCO += --lwlink=$(COCO_TOOLS)/lwlink-mule.py --initial-s=8000
CFLAGS_EXTRA_COCO += -DKEY_END_NAME='"ENTER"' -DQUIT_HINT='"BREAK: QUIT"' \
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
FNLIB_EXP ?= $(HOME)/Workspace/fujinet-lib-experimental
DOS_TOOLS = $(CURDIR)/tools/dos
ifeq ($(PLATFORM),msdos)
  FUJINET_LIB := __UNDEFINED__
  CC_OW2 = $(DOS_TOOLS)/ow wcc
  AS_OW2 = $(DOS_TOOLS)/ow wasm
  AR_OW2 = $(DOS_TOOLS)/ow wlib
  LD_OW2 = $(DOS_TOOLS)/wlink-mule.py OPTION quiet
endif
CFLAGS_EXTRA_MSDOS += -zq -mm -os -s -of -zp1 -DKEY_END_NAME='"ENTER"' -DPLAT_NET_PROBE \
                      -DQUIT_HINT='"ESC: LOBBY"'
EXTRA_INCLUDE_MSDOS += build/fnlib-msdos/include
# (-ad names the object in the .d without its directory: -adt names it)
CFLAGS_EXTRA_MSDOS += -adt=$@
EXECUTABLE_EXTRA_DEPS_MSDOS += $(DOS_TOOLS)/wlink-mule.py

msdos/disk-post::
	printf '@ECHO OFF\r\nMULE.EXE\r\n' > build/autoexec.bat
	mcopy -o -i $(DISK) build/autoexec.bat "::AUTOEXEC.BAT"

# ---- generated files (top-level pass only) ---------------------------------
ifeq ($(PLATFORM),)
PYTHON   ?= python3
ENDPOINT ?= $(shell cat gen/endpoint 2>/dev/null || echo https://mule.fujinet.online)
ATR      ?= $(wildcard mule.atr)

.PHONY: gen-apple2 gen-coco gen-msdos FORCE
r2r/apple2/$(PRODUCT): gen-apple2
r2r/coco/$(PRODUCT): gen-coco
r2r/msdos/$(PRODUCT): gen-msdos

gen-apple2: gen/config.h gen/apple2/gfx.c
gen-coco: gen/config.h gen/coco/gfx.c
gen-msdos: gen/config.h gen/msdos/gfx.c build/fnlib-msdos/fujinet-msdos.lib

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

gen/msdos/gfx.c: tools/mkgfx_dos.py tools/mkgfx_hgr.py $(ATR)
	@mkdir -p gen/msdos
	@if [ -z "$(ATR)" ]; then \
	  echo "*** WARNING: no ATR (make ATR=... or a mule.atr link): using STAND-IN art"; fi
	$(PYTHON) tools/mkgfx_dos.py $(if $(ATR),--atr "$(ATR)") --preview gen/preview/msdos $@

build/fnlib-msdos/fujinet-msdos.lib: tools/dos/mkfnlib.sh $(wildcard $(FNLIB_EXP)/common/*.c $(FNLIB_EXP)/bus/msdos/*.c $(FNLIB_EXP)/include/*.h)
	tools/dos/mkfnlib.sh $(FNLIB_EXP) build/fnlib-msdos

clean::
	rm -rf gen/apple2 gen/coco gen/msdos gen/preview gen/config.h
endif

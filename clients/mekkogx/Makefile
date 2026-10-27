# clients/mekkogx/Makefile -- M.U.L.E. for FujiNet 8-bit computers (MekkoGX).
#
#   make apple2                        r2r/apple2/mule.po (ProDOS, 128K //e, IIc, IIgs)
#   make coco                          r2r/coco/mule.dsk (DECB, 64K CoCo 1/2)
#   make msdos                         r2r/msdos/mule.img (MULE.EXE: 128K PC with CGA, or PCjr;
#                                      MULEJR.EXE: the PCjr's own)
#   make adam                          r2r/adam/mule.ddp and mule.dsk (Coleco Adam)
#   make msxrom                        r2r/msxrom/mule.rom (MSX, a FujiNet user ROM: ASCII8)
#   make atari                         r2r/atari/mule.atr (48K Atari 400/800, XL/XE)
#   make lynx                          r2r/lynx/mule.lnx (Atari Lynx cart, FujiNet-Lynx)
#   make nes                           r2r/nes/mule.nes (NES, the FujiNet NES cartridge: MMC3)
#   make sms                           r2r/sms/mule.sms (Sega Master System, the FujiNet SMS cartridge)
#   make atari7800                     r2r/atari7800/mule.a78 (Atari 7800, the FujiNet 7800 cartridge)
#   make h89-cpm                       r2r/h89-cpm/mule.h8d (Heathkit H89, CP/M: MULE.COM on an H17 disk)
#   make h89-boot                      r2r/h89-cpm/mule-boot.h8d (the same on a bootable 64K CP/M disk)
#   make ATR="path/to/M.U.L.E. (1983)(Electronic Arts)(US).atr"
#                                      graphics from the original disk (never committed)
#   make ENDPOINT=http://host:8080     server used when the lobby gave none (remembered)
#   make AUTOPLAY=1                    plays by itself (soak tests; rebuild clean)
#
# src/ is platform neutral; src/platform-specific/*.h is the API each platform
# implements in src/<platform>/. Generated files go to gen/ (gitignored).

PRODUCT = mule
PLATFORMS = apple2 coco msdos adam msxrom atari lynx nes sms atari7800 h89-cpm
# The PC's disk has two: MULE.EXE, and MULEJR.EXE for the PCjr (MS-DOS below)
ifeq ($(PLATFORM),msdos)
  PRODUCTS = mule mulejr
endif

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
  adam+=tms9918 \
  sms+=tms9918 \
  h89-cpm+=h89

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
  $(COOBJ)/src/nstrans_fn.o $(COOBJ)/src/autoplay.o \
  $(COOBJ)/src/coco/gfx.o $(COOBJ)/src/coco/input.o \
  $(COOBJ)/gen/coco/gfx.o:                      export MULE_SECT = hi
# (sound.o is in the low core: nstrans_fn.c's own DriveWire calls took the
# high core's last bytes, and fujinet-lib's status, read and write, which
# they replace, left the low core room; autoplay.o, empty unless AUTOPLAY=1,
# is in the high core so that build fits too)

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
# The stream's packets go over the serial port FUJINET.SYS uses, driven by
# its interrupt (src/msdos/fbport.c, uartisr.asm; PLAT_FUJIBUS), so no call
# is waited on; FB_RESENDS asks the FujiNet for a garbled reply again. If no
# port answers, FUJINET.SYS carries them, each call waited on (at 9600 bps,
# a PCjr, a status is 25 ms: asked every 6th frame). The server is told
# the link may be slow (PLAT_NS_SLOW: a push at most every 300 ms).
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
                      -DPLAT_NET_GUARD -DQUIT_HINT='"ESC: LOBBY"' -DPLAT_NS_SLOW \
                      -DPLAT_FUJIBUS -DFB_PROBE -DFB_RESENDS=2 -DFB_RBUF=128 -DFB_QBUF=64
EXTRA_INCLUDE_MSDOS += build/fnlib-msdos/include
# (-ad names the object in the .d without its directory: -adt names it)
CFLAGS_EXTRA_MSDOS += -adt=$@
EXECUTABLE_EXTRA_DEPS_MSDOS += $(DOS_TOOLS)/wlink-mule.py

# MULEJR.EXE is the PCjr's own build (src/msdos/machine.h): only its
# 16-colour mode, the CGA, EGA/VGA and PC keyboard and speaker code left
# out, so the 128K PCjr has that much more room for the overlays. It
# hands any other machine to MULE.EXE. MULESEL.COM (tools/dos/mulesel.asm),
# AUTOEXEC.BAT's last line, types the one for the machine as the
# keyboard's: nothing of it, or of the batch file, is left when the game
# runs.
mulejr_CFLAGS += -DPLAT_JRONLY
mulejr_ASFLAGS += -dJRONLY
DISK_EXTRA_FILES_MSDOS += r2r/msdos/mulesel.com
r2r/msdos/mulesel.com: tools/dos/mulesel.asm | r2r/msdos
	$(DOS_TOOLS)/ow wasm -zq -fo=build/mulesel.o $<
	$(DOS_TOOLS)/ow wlink option quiet format dos com name $@ file build/mulesel.o

msdos/disk-post::
	printf '@ECHO OFF\r\nMULESEL\r\n' > build/autoexec.bat
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
	mcopy -o -i r2r/msdos/pcjr-boot.img r2r/msdos/mulejr.exe ::MULEJR.EXE
	mcopy -o -i r2r/msdos/pcjr-boot.img r2r/msdos/mulesel.com ::MULESEL.COM
	mcopy -o -i r2r/msdos/pcjr-boot.img build/autoexec.bat ::AUTOEXEC.BAT
	@echo "r2r/msdos/pcjr-boot.img: boot it (fdd_01_fn in 86box.cfg) and MULE runs from A:"

# ---- Coleco Adam (and the MSX, the SMS): z88dk -----------------------------
# zcc with zsdcc, from defoogi unless installed (tools/z80/z88dk). The
# TMS9918A code is src/tms9918 (a PLATFORM_COMBOS entry). fujinet-lib is
# fujinet-lib-experimental built with zsdcc (tools/z80/mkfnlib.sh).
Z80_TOOLS = $(CURDIR)/tools/z80
ifneq ($(filter adam msxrom sms h89-cpm,$(PLATFORM)),)
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
CFLAGS_EXTRA_MSXROM += -compiler=sdcc -SO3 -DPLAT_NET_PROBE -DPLAT_NETSTREAM -DPLAT_FUJIBUS -DKEY_END_NAME='"\"RETURN\""'
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

# ---- Sega Master System: the FujiNet SMS cartridge -------------------------
# A Sega-mapper image (src/sms/mule.asm, tools/sms/mkrom.py): the program
# fixed in banks 0-1 (0000h-7FD7h), the phases, the art and the strings
# taking turns in slot 2 at 8000h-AFFFh (B000h-BFFFh is the cartridge's
# mailbox, over every bank). The screen is the Adam's cells shown in Mode 4
# through a tile cache (src/tms9918, src/sms/vram_sms.h). The stream's calls
# go to the mailbox unwaited (src/sms/nstrans.c). fujinet-lib is the add-sms
# branch (FNLIB_SMS: fujinet-lib-experimental's worktree) built with zsdcc.
FNLIB_SMS ?= $(HOME)/Workspace/fnlib-sms
CFLAGS_EXTRA_SMS += -compiler=sdcc -SO3 -DPLAT_NETSTREAM -DPLAT_NO_KEYBOARD \
                    -DBUTTON_NAME='"\"1\""' -DBACK_KEY_NAME='"\"2\""' -DKEY_END_NAME='"\"PAUSE\""' \
                    -DLEAVE_KEY_NAME='"\"2+PAUSE\""' -DQUIT_HINT='"\"2+PAUSE: QUIT\""' \
                    -DLOBBY_HINT='"\"<->SPECIES 1 READY 2+PAUSE LEAVE\""'
EXTRA_INCLUDE_SMS += src/tms9918 src/sms build/fnlib-sms/include
LDFLAGS_EXTRA_SMS += -compiler=sdcc -m -pragma-define:CRT_ENABLE_STDIO=0 -pragma-define:CLIB_FOPEN_MAX=0
ifeq ($(PLATFORM),sms)
  LIBS += -Lbuild/fnlib-sms -lfujinet-sms
endif
EXECUTABLE_EXTRA_DEPS_SMS += build/fnlib-sms/fujinet-sms.lib src/sms/mule.asm
SMOBJ = build/$(PRODUCT)/sms
$(SMOBJ)/src/boot.o $(SMOBJ)/src/teststart.o: CFLAGS += --codeseg=OV1 --constseg=OV1
$(SMOBJ)/src/status.o:                        CFLAGS += --codeseg=OV2 --constseg=OV2
$(SMOBJ)/src/grant.o $(SMOBJ)/src/events.o:   CFLAGS += --codeseg=OV3 --constseg=OV3
$(SMOBJ)/src/auction.o:                       CFLAGS += --codeseg=OV4 --constseg=OV4
$(SMOBJ)/src/develop.o:                       CFLAGS += --codeseg=OV5 --constseg=OV5
$(SMOBJ)/src/strings.o:                       CFLAGS += --constseg=STR
$(SMOBJ)/gen/tms9918/gfx.o:                   CFLAGS += --constseg=ART

sms/r2r-post::
	python3 tools/sms/mkrom.py r2r/sms

# ---- Heathkit H89: CP/M 2.2.04, the H19 terminal --------------------------
# MULE.COM, alone on a 100K H17 disk (hz89, the format Heath's CP/M gives a
# hard-sectored disk). The H89-ESP32 FujiNet card is an 82C55 at D0h that
# speaks FujiBus: the stream's calls go to it unwaited (src/h89/fbport.c,
# src/nstrans_fb.c); fujinet-lib (FNLIB_H89: the one with bus/h89) opens and
# closes, built with zsdcc. The screen is the H19's characters, sent by
# difference (src/h89/h19.c). No overlays: a 64K CP/M has room for it all.
FNLIB_H89 ?= $(HOME)/Workspace/fujinet-lib
CFLAGS_EXTRA_H89-CPM += -compiler=sdcc -SO3 -D__H89__ -DPLAT_NET_PROBE -DPLAT_NETSTREAM -DPLAT_FUJIBUS \
                        -DPLAT_NS_SLOW -DFB_RBUF=128 -DFB_QBUF=64
ifeq ($(H89_RIG_KEYS),1)
  CFLAGS_EXTRA_H89-CPM += -DH89_RIG_KEYS
endif
EXTRA_INCLUDE_H89-CPM += src/h89 build/fnlib-h89/include
LDFLAGS_EXTRA_H89-CPM += -compiler=sdcc -m -pragma-define:CRT_ENABLE_STDIO=0 -pragma-define:CLIB_FOPEN_MAX=0 \
                         -pragma-define:CRT_ENABLE_COMMANDLINE=0 -pragma-define:CRT_INITIALIZE_BSS=1 \
                         -pragma-define:noprotectmsdos=1
ifeq ($(PLATFORM),h89-cpm)
  LIBS += -Lbuild/fnlib-h89 -lfujinet-h89
endif
EXECUTABLE_EXTRA_DEPS_H89-CPM += build/fnlib-h89/fujinet-h89.lib
# What runs once in a while is made small, for the 62K CP/M FNDISK needs.
H8OBJ = build/$(PRODUCT)/h89-cpm/src
$(H8OBJ)/boot.o $(H8OBJ)/teststart.o $(H8OBJ)/test.o $(H8OBJ)/net.o $(H8OBJ)/status.o \
  $(H8OBJ)/events.o $(H8OBJ)/grant.o $(H8OBJ)/strings.o: CFLAGS += --opt-code-size

h89-cpm/r2r-post::
	@mkdir -p $(CACHE_PLATFORM)/disk
	python3 tools/h89/trimcom.py $(R2R_PD)/$(PRODUCT).map $(R2R_PD)/$(PRODUCT).com
	cp $(R2R_PD)/$(PRODUCT).com $(CACHE_PLATFORM)/disk/MULE.COM
	cd $(CACHE_PLATFORM)/disk && rm -f disk && $(Z80_TOOLS)/z88dk z88dk-appmake +cpmdisk -f hz89 \
	  --container raw --no-com-file -b MULE.COM -o disk
	python3 -I tools/h89/cpm-dir-fixup.py $(CACHE_PLATFORM)/disk/disk
	mv $(CACHE_PLATFORM)/disk/disk $(R2R_PD)/$(PRODUCT).h8d

# A disk that boots: fujinet-go-h89-desktop's 64K CP/M system disk with the
# development tools taken off and MULE.COM put on, by CP/M itself in that
# project's headless H89 (tools/h89/mkboot.sh; it needs the rig, built).
.PHONY: h89-boot
h89-boot: r2r/h89-cpm/$(PRODUCT)
	tools/h89/mkboot.sh r2r/h89-cpm/$(PRODUCT).com r2r/h89-cpm/$(PRODUCT)-boot.h8d

# ---- Atari 400/800 (48K), XL/XE -------------------------------------------
# cc65, no DOS: picoboot loads MULE.COM, whose first chunk (src/atari/
# start.s) switches BASIC off; the phases are overlays PHASE.1 .. PHASE.5 read
# by sector (src/atari/bank.c). Memory: src/atari/mule-atari.cfg.
CFLAGS_EXTRA_ATARI += -Os -Cl -DPLAT_NETSTREAM
LDFLAGS_EXTRA_ATARI += -C src/atari/mule-atari.cfg -m build/mule-atari.map
# AUTOPLAY: its turn walker makes PHASE.5 the biggest overlay; the offline
# test mode's resident half (test.c) makes the room for the larger window
ifeq ($(AUTOPLAY),1)
  CFLAGS_EXTRA_ATARI += -DMULE_NOTEST
  LDFLAGS_EXTRA_ATARI += -Wl -D,__OVLSIZE__=0x1BC0
endif
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

# ---- NES ---------------------------------------------------------------------
# cc65, an MMC3 cart image (.nes) for the FujiNet NES cartridge, which the
# NES CONFIG boots: 128K PRG, 16K CHR-RAM, 8K WRAM (src/nes/mule-nes.cfg).
# The program is in the fixed banks and R7; the phases, the art and the
# strings take turns in the window at $8000 (src/nes/bank.c). The screen
# is the Adam's 256 x 192 bitmap, a pattern table to each third of it,
# switched by the scanline interrupt (src/nes/crt0.s, gfx.c). fujinet-lib
# is fujinet-lib-experimental's nes target (tools/nes/mkfnlib.sh); the
# stream's calls go to the cartridge's mailbox unwaited (src/nes/nstrans.c).
ifeq ($(PLATFORM),nes)
  FUJINET_LIB := __UNDEFINED__
  LIBS += build/fnlib-nes/fujinet-nes.lib
endif
CFLAGS_EXTRA_NES += -Os -Cl -DPLAT_NETSTREAM -DPLAT_NO_KEYBOARD \
                    -DKEY_END_NAME='"START"' -DBUTTON_NAME='"A"' -DLEAVE_KEY_NAME='"SELECT"' \
                    -DQUIT_HINT='"SELECT: QUIT"' -DLOBBY_HINT='"<->SPECIES A READY SELECT LEAVE"'
EXTRA_INCLUDE_NES += build/fnlib-nes/include
# AUTOPLAY: its turn walker goes in the fixed bank (src/develop.c), in the
# room of the offline test mode's resident half (test.c)
ifeq ($(AUTOPLAY),1)
  CFLAGS_EXTRA_NES += -DMULE_NOTEST
endif
LDFLAGS_EXTRA_NES += -C src/nes/mule-nes.cfg -m build/mule-nes.map
EXECUTABLE_EXTRA_DEPS_NES += src/nes/mule-nes.cfg build/fnlib-nes/fujinet-nes.lib

NSOBJ = build/$(PRODUCT)/nes
$(NSOBJ)/src/boot.o $(NSOBJ)/src/teststart.o: CFLAGS += --code-name OVERLAY1 --rodata-name OVERLAY1
$(NSOBJ)/src/status.o:                        CFLAGS += --code-name OVERLAY2 --rodata-name OVERLAY2
$(NSOBJ)/src/grant.o $(NSOBJ)/src/events.o:   CFLAGS += --code-name OVERLAY3 --rodata-name OVERLAY3
$(NSOBJ)/src/auction.o:                       CFLAGS += --code-name OVERLAY4 --rodata-name OVERLAY4
$(NSOBJ)/src/develop.o:                       CFLAGS += --code-name OVERLAY5 --rodata-name OVERLAY5
$(NSOBJ)/src/strings.o:                       CFLAGS += --rodata-name STRS
$(NSOBJ)/gen/nes/gfx.o:                       CFLAGS += --rodata-name ART
# R7's bank: the rest of the program, never switched out (fx.c's scripts
# and text.c's tables with it: the fixed bank is full)
$(NSOBJ)/src/nsnet.o $(NSOBJ)/src/text.o $(NSOBJ)/src/map.o \
  $(NSOBJ)/src/test.o $(NSOBJ)/src/fx.o:      CFLAGS += --code-name CODE2
$(NSOBJ)/src/text.o $(NSOBJ)/src/fx.o:        CFLAGS += --rodata-name CODE2
# net.c is only the appkeys here (stream mode), which only boot calls: its
# overlay (so are the lib's appkeys, tools/nes/mkfnlib.sh)
$(NSOBJ)/src/net.o:                           CFLAGS += --code-name OVERLAY1 --rodata-name OVERLAY1

# The claim, the vectors and no read-modify-write of the mailbox.
nes/executable-post::
	python3 tools/nes/checkrom.py build/mule-nes.map $(EXECUTABLE)

# ---- Atari 7800 ----------------------------------------------------------------
# cc65, a SuperGame image with the cart's RAM for the FujiNet 7800 cartridge,
# which the 7800 CONFIG boots: 128K in 16K banks (src/atari7800/mule-a78.cfg).
# The program is in the fixed bank and in REPL, the first 8K of the window,
# which every bank has (tools/atari7800/mkrom.py copies it); the phases, the
# art and the strings take turns in the window's other 8K
# (src/atari7800/bank.c). The screen is the Atari 800 port's, rows of the
# original's characters and its sprites, drawn by MARIA (src/atari7800/
# maria.s, nmi.s). fujinet-lib is fujinet-lib-experimental's atari7800
# target (tools/atari7800/mkfnlib.sh); the stream's calls go to the
# cartridge's mailbox unwaited (src/atari7800/nstrans.c).
FNLIB_7800 ?= $(HOME)/Workspace/fnlib-7800
ifeq ($(PLATFORM),atari7800)
  FUJINET_LIB := __UNDEFINED__
  LIBS += build/fnlib-a78/fujinet-a78.lib
endif
CFLAGS_EXTRA_ATARI7800 += -Os -Cl -DPLAT_NETSTREAM -DPLAT_NO_KEYBOARD \
                          -DKEY_END_NAME='"PAUSE"' -DBUTTON_NAME='"FIRE"' -DLEAVE_KEY_NAME='"SELECT"' \
                          -DBACK_KEY_NAME='"BUTTON 2"' -DQUIT_HINT='"SELECT: QUIT"' \
                          -DLOBBY_HINT='"<- -> SPECIES  FIRE READY  SELECT LEAVE"'
EXTRA_INCLUDE_ATARI7800 += build/fnlib-a78/include
ifeq ($(AUTOPLAY),1)
  CFLAGS_EXTRA_ATARI7800 += -DMULE_NOTEST
endif
LDFLAGS_EXTRA_ATARI7800 += -C src/atari7800/mule-a78.cfg -m build/mule-a78.map
EXECUTABLE_EXTRA_DEPS_ATARI7800 += src/atari7800/mule-a78.cfg build/fnlib-a78/fujinet-a78.lib

A7OBJ = build/$(PRODUCT)/atari7800
$(A7OBJ)/src/boot.o $(A7OBJ)/src/teststart.o: CFLAGS += --code-name OVERLAY1 --rodata-name OVL1RO
$(A7OBJ)/src/status.o:                        CFLAGS += --code-name OVERLAY2 --rodata-name OVL2RO
$(A7OBJ)/src/grant.o $(A7OBJ)/src/events.o:   CFLAGS += --code-name OVERLAY3 --rodata-name OVL3RO
$(A7OBJ)/src/auction.o:                       CFLAGS += --code-name OVERLAY4 --rodata-name OVL4RO
$(A7OBJ)/src/develop.o:                       CFLAGS += --code-name OVERLAY5 --rodata-name OVL5RO
$(A7OBJ)/src/strings.o:                       CFLAGS += --rodata-name STRS
$(A7OBJ)/gen/atari7800/gfx.o:                 CFLAGS += --rodata-name ART
# REPL: in every bank, so as good as fixed
$(A7OBJ)/src/nsnet.o $(A7OBJ)/src/text.o $(A7OBJ)/src/map.o \
  $(A7OBJ)/src/test.o $(A7OBJ)/src/fx.o \
  $(A7OBJ)/src/autoplay.o:                    CFLAGS += --code-name REPL --rodata-name REPLRO
# net.c is only the appkeys here (stream mode), which only boot calls
$(A7OBJ)/src/net.o:                           CFLAGS += --code-name OVERLAY1 --rodata-name OVL1RO

# REPL into every bank, the checks, the .a78 (r2r/atari7800/mule.a78)
atari7800/executable-post::
	python3 tools/atari7800/mkrom.py build/mule-a78.map $(EXECUTABLE) $(EXECUTABLE:.bin=.a78)

# ---- generated files (top-level pass only) ---------------------------------
ifeq ($(PLATFORM),)
PYTHON   ?= python3
ENDPOINT ?= $(shell cat gen/endpoint 2>/dev/null || echo https://mule.fujinet.online)
ATR      ?= $(wildcard mule.atr)

.PHONY: gen-apple2 gen-coco gen-msdos gen-adam gen-msxrom gen-atari gen-lynx gen-nes gen-sms gen-atari7800 gen-h89-cpm FORCE
r2r/apple2/$(PRODUCT): gen-apple2
r2r/coco/$(PRODUCT): gen-coco
r2r/msdos/$(PRODUCT): gen-msdos
r2r/adam/$(PRODUCT): gen-adam
r2r/msxrom/$(PRODUCT): gen-msxrom
r2r/atari/$(PRODUCT): gen-atari
r2r/lynx/$(PRODUCT): gen-lynx
r2r/nes/$(PRODUCT): gen-nes
r2r/sms/$(PRODUCT): gen-sms
r2r/atari7800/$(PRODUCT): gen-atari7800
r2r/h89-cpm/$(PRODUCT): gen-h89-cpm

gen-apple2: gen/config.h gen/apple2/gfx.c
gen-coco: gen/config.h gen/coco/gfx.c
gen-msdos: gen/config.h gen/msdos/gfx.c build/fnlib-msdos/fujinet-msdos.lib
gen-adam: gen/config.h gen/tms9918/gfx.c build/fnlib-adam/fujinet-adam.lib
gen-msxrom: gen/config.h gen/tms9918/gfx.c build/fnlib-msx/fujinet-msx.lib
gen-atari: gen/config.h gen/atari/gfx.c
gen-lynx: gen/config.h gen/lynx/gfx.c build/fnlib-lynx/fujinet-lynx.lib
gen-nes: gen/config.h gen/nes/gfx.c build/fnlib-nes/fujinet-nes.lib
gen-sms: gen/config.h gen/tms9918/gfx.c build/fnlib-sms/fujinet-sms.lib
gen-atari7800: gen/config.h gen/atari7800/gfx.c build/fnlib-a78/fujinet-a78.lib
gen-h89-cpm: gen/config.h build/fnlib-h89/fujinet-h89.lib

gen/config.h: FORCE
	@mkdir -p gen
	@echo "$(ENDPOINT)" > gen/endpoint.new
	@cmp -s gen/endpoint.new gen/endpoint || mv gen/endpoint.new gen/endpoint
	@rm -f gen/endpoint.new
	@printf '/* GENERATED by the Makefile. */\n#define DEFAULT_ENDPOINT "%s"\n' "$$(cat gen/endpoint)" > gen/config.h.new
	@cmp -s gen/config.h.new gen/config.h || mv gen/config.h.new gen/config.h
	@rm -f gen/config.h.new

gen/apple2/gfx.c: tools/mkgfx_hgr.py tools/tms/namco.bin $(ATR)
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

gen/nes/gfx.c: tools/mkgfx_nes.py tools/mkgfx_tms.py tools/tms/namco.bin tools/mkgfx_dos.py tools/mkgfx_pmode4.py tools/mkgfx_hgr.py $(ATR)
	@mkdir -p gen/nes
	@if [ -z "$(ATR)" ]; then \
	  echo "*** WARNING: no ATR (make ATR=... or a mule.atr link): using STAND-IN art"; fi
	$(PYTHON) tools/mkgfx_nes.py $(if $(ATR),--atr "$(ATR)") --preview gen/preview/nes $@

# The 7800 shows the Atari 800's graphics (src/atari7800/gfx.c converts them)
gen/atari7800/gfx.c: tools/mkgfx_atari.py tools/mkgfx_hgr.py $(ATR)
	@mkdir -p gen/atari7800
	@if [ -z "$(ATR)" ]; then \
	  echo "*** WARNING: no ATR (make ATR=... or a mule.atr link): using STAND-IN art"; fi
	$(PYTHON) tools/mkgfx_atari.py $(if $(ATR),--atr "$(ATR)") $@

build/fnlib-a78/fujinet-a78.lib: tools/atari7800/mkfnlib.sh $(wildcard $(FNLIB_7800)/common/*.c $(FNLIB_7800)/bus/atari7800/* $(FNLIB_7800)/include/*.h)
	tools/atari7800/mkfnlib.sh $(FNLIB_7800) build/fnlib-a78

build/fnlib-nes/fujinet-nes.lib: tools/nes/mkfnlib.sh $(wildcard $(FNLIB_EXP)/common/*.c $(FNLIB_EXP)/bus/nes/* $(FNLIB_EXP)/include/*.h)
	tools/nes/mkfnlib.sh $(FNLIB_EXP) build/fnlib-nes

build/fnlib-lynx/fujinet-lynx.lib: tools/lynx/mkfnlib.sh $(wildcard $(FNLIB_EXP)/common/*.c $(FNLIB_EXP)/bus/lynx/* $(FNLIB_EXP)/include/*.h)
	tools/lynx/mkfnlib.sh $(FNLIB_EXP) build/fnlib-lynx

build/fnlib-adam/fujinet-adam.lib: tools/z80/mkfnlib.sh $(wildcard $(FNLIB_EXP)/common/*.c $(FNLIB_EXP)/bus/adam/* $(FNLIB_EXP)/include/*.h)
	tools/z80/mkfnlib.sh $(FNLIB_EXP) build/fnlib-adam adam

build/fnlib-msx/fujinet-msx.lib: tools/z80/mkfnlib.sh tools/z80/msx-portio.c $(wildcard $(FNLIB_EXP)/common/*.c $(FNLIB_EXP)/bus/msx/* $(FNLIB_EXP)/include/*.h)
	tools/z80/mkfnlib.sh $(FNLIB_EXP) build/fnlib-msx msx

build/fnlib-sms/fujinet-sms.lib: tools/z80/mkfnlib.sh $(wildcard $(FNLIB_SMS)/common/*.c $(FNLIB_SMS)/bus/sms/* $(FNLIB_SMS)/include/*.h)
	tools/z80/mkfnlib.sh $(FNLIB_SMS) build/fnlib-sms sms

build/fnlib-h89/fujinet-h89.lib: tools/z80/mkfnlib.sh $(wildcard $(FNLIB_H89)/common/*.c $(FNLIB_H89)/bus/h89/* $(FNLIB_H89)/include/*.h)
	tools/z80/mkfnlib.sh $(FNLIB_H89) build/fnlib-h89 h89

build/fnlib-msdos/fujinet-msdos.lib: tools/dos/mkfnlib.sh $(wildcard $(FNLIB_EXP)/common/*.c $(FNLIB_EXP)/bus/msdos/*.c $(FNLIB_EXP)/include/*.h)
	tools/dos/mkfnlib.sh $(FNLIB_EXP) build/fnlib-msdos

clean::
	rm -rf gen/apple2 gen/coco gen/msdos gen/tms9918 gen/atari gen/lynx gen/nes gen/atari7800 gen/preview gen/config.h
endif

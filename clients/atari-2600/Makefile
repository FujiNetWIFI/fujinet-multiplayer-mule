# Makefile -- a wrapper; build.sh is the build.
#
# 6502 for Macroassembler AS on the FujiNet 2600 cartridge (fn-2600), so it
# stands beside intv/ and astrocade/ rather than in the MekkoGX build.

ENDPOINT ?= http://fujinet.online:9900
FUJI_FIRMWARE ?= $(HOME)/Workspace/fn-2600
MAME_DIR ?= $(HOME)/Workspace/mame
SECS ?= 300

SRC := $(wildcard src/*.asm src/*.inc tools/*.py)

.PHONY: all layout run shot frames live hosttest emu clean

all: build/mule2600.bin

build/mule2600.bin: $(SRC) build.sh
	ENDPOINT="$(ENDPOINT)" FUJI_FIRMWARE="$(FUJI_FIRMWARE)" ./build.sh

# The map kernel with no network: the model's test map baked into ROM.
layout: $(SRC) build.sh
	FUJI_FIRMWARE="$(FUJI_FIRMWARE)" ./build.sh layout

run: all
	MAME=$(MAME_DIR) ./run.sh

shot: layout
	MAME=$(MAME_DIR) ROM=layout FAST=1 SECS=6 ./run.sh shot

# Live: a private server (8096) and fujinet-pc (BoIP 19996), a scenario.
# SCEN=test/round.scen by default; M26_FRAMES=1 reports every long frame.
SCEN ?= test/round.scen
live: all
	MAME=$(MAME_DIR) test/rig.sh $(SCEN) $(SECS)

# The cartridge's composer (FN_BLIT_MULEMAP among them), byte for byte.
hosttest:
	$(MAKE) -C $(FUJI_FIRMWARE)/pico/atari-2600/firmware/host_test

# Re-graft the cartridge into MAME after a firmware change.
emu:
	$(FUJI_FIRMWARE)/pico/atari-2600/emu/apply.sh $(MAME_DIR)
	$(MAKE) -C $(MAME_DIR) -j$$(nproc) NOWERROR=1

clean:
	rm -rf build

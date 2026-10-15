# M.U.L.E. for the Fairchild Channel F (Macroassembler AS, FujiNet cartridge
# with the APPBANK firmware). The build is build.sh; this is the convenience
# wrapper. See README.md.

SRCS := mule.asm $(wildcard src/*.inc) $(wildcard tools/*.py)

all: build/mule.bin

build/mule.bin: $(SRCS) build.sh
	./build.sh

# Headless: an offline test scenario (test/scen/*.scen), snapshots in build/sim.
sim: build/mule.bin
	python3 tools/chfsim.py $(SCEN) build/sim

clean:
	rm -rf build gen

.PHONY: all sim clean

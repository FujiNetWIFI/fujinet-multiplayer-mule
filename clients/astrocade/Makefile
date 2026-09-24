# M.U.L.E. for the Bally Astrocade (zmac, FujiNet cartridge). The build is
# build.sh; this is the convenience wrapper. See README.md.

SRCS := mule.asm HVGLIB.H $(wildcard *.inc) tools/mkgfx_astro.py

all: build/mule.bin

build/mule.bin: $(SRCS) build.sh tools/checkrom.py tools/checksize.py
	./build.sh

run: build/mule.bin
	./run.sh

# Headless: the offline test scenarios, snapshots in build/snap/.
sim: build/mule.bin
	python3 tools/astrosim.py $(SCEN)

clean:
	rm -rf build gen

.PHONY: all run sim clean

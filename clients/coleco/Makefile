# M.U.L.E. for the ColecoVision (zmac, FujiNet cartridge). The build is
# build.sh; this is the convenience wrapper. See README.md.

SRCS := mule.asm $(wildcard *.inc) tools/mkgfx_coleco.py tools/mkstrings.py

all: build/mule.rom

build/mule.rom: $(SRCS) build.sh tools/mkimage.py tools/checksize.py
	./build.sh

run: build/mule.rom
	./run.sh

clean:
	rm -rf build gen

.PHONY: all run clean

# clients/intv/Makefile -- M.U.L.E. for the Intellivision (IntyBASIC + FujiNet).
#
#   make                         build mule.rom (jzIntv) and mule.bin/.cfg (carts)
#   make ATR="path/to/M.U.L.E. (1983)(Electronic Arts)(US).atr"
#                                use the original's graphics (never committed)
#   make ENDPOINT=http://host:8080   default server when the lobby gave none
#   make check                   sanity checks (string lengths, ROM layout)
#   make run                     build and start jzIntv (see run.sh)
#
# IntyBASIC 1.5 is built from source ($(IB_SRC)), and its own prologue and
# epilogue are used with it. The IntyBASIC on PATH may be 1.4, which lacks
# OPTION MAP.

IB_SRC   ?= $(HOME)/Workspace/IntyBASIC
INTYBASIC = build/intybasic
AS1600   ?= as1600
PYTHON   ?= python3
# The server built in when the lobby gives none. Remembered in gen/endpoint,
# so a later plain `make` (run.sh runs one) keeps what you last chose.
ENDPOINT ?= $(shell cat gen/endpoint 2>/dev/null || echo https://mule.fujinet.online)
ATR      ?= $(wildcard mule.atr)
ifeq ($(strip $(ATR)),)
$(warning *** No M.U.L.E. ATR (ATR= or mule.atr): gen/gfx.bas uses STAND-IN ART, not the Atari graphics ***)
endif

PRODUCT = mule
SOURCES = mule.bas memory.bas fujinet.bas input.bas net.bas screen.bas isr.bas \
          fx.bas sound.bas map.bas grant.bas develop.bas auction.bas events.bas \
          status.bas boot.bas text.bas textlong.bas ids.bas grant.bas test.bas constants.bas
GEN     = gen/proto.bas gen/gfx.bas gen/gfxconst.bas gen/config.bas

.PHONY: all clean run check proto assets FORCE

all: $(PRODUCT).rom $(PRODUCT).bin check

$(INTYBASIC): $(wildcard $(IB_SRC)/*.cpp $(IB_SRC)/*.h)
	@mkdir -p build
	$(CXX) -O2 -o $@ $(IB_SRC)/node.cpp $(IB_SRC)/microcode.cpp $(IB_SRC)/code.cpp $(IB_SRC)/IntyBASIC.cpp

gen/proto.bas: ../mule_proto.h tools/gen_proto.py
	@mkdir -p gen
	$(PYTHON) tools/gen_proto.py $< $@

gen/gfx.bas gen/gfxconst.bas: tools/mkgfx.py $(ATR)
	@mkdir -p gen
	$(PYTHON) tools/mkgfx.py $(if $(ATR),--atr "$(ATR)") gen/gfx.bas

gen/config.bas: tools/gen_config.py FORCE
	@mkdir -p gen
	$(PYTHON) tools/gen_config.py "$(ENDPOINT)" $@
	@echo "$(ENDPOINT)" > gen/endpoint

proto: gen/proto.bas
assets: gen/gfx.bas

# IntyBASIC's prologue and epilogue, copied next to the compiler. The 1.5.2
# prologue has one comment written with ' instead of ; (line 251), which
# as1600 rejects; the copy fixes it.
build/intybasic_prologue.asm: $(IB_SRC)/intybasic_prologue.asm
	@mkdir -p build
	sed "s/  ' Max internal index/  ; Max internal index/" $< > $@

build/intybasic_epilogue.asm: $(IB_SRC)/intybasic_epilogue.asm
	@mkdir -p build
	cp $< $@

$(PRODUCT).asm: $(SOURCES) $(GEN) $(INTYBASIC) build/intybasic_prologue.asm build/intybasic_epilogue.asm
	$(INTYBASIC) $(PRODUCT).bas $(PRODUCT).asm build/

# .bin + .cfg: for carts (PiRTO II / Minty) from the SD card.
$(PRODUCT).bin: $(PRODUCT).asm
	$(AS1600) -o $(PRODUCT).bin -l $(PRODUCT).lst $(PRODUCT).asm

# .rom: for jzIntv.
$(PRODUCT).rom: $(PRODUCT).asm
	$(AS1600) -o $(PRODUCT).rom -s $(PRODUCT).sym $(PRODUCT).asm

check: $(PRODUCT).bin
	$(PYTHON) tools/strlen.py text.bas textlong.bas
	$(PYTHON) tools/checkcfg.py $(PRODUCT).cfg

run: $(PRODUCT).rom
	./run.sh

clean:
	rm -f $(PRODUCT).asm $(PRODUCT).bin $(PRODUCT).cfg $(PRODUCT).rom $(PRODUCT).lst $(PRODUCT).sym
	rm -rf gen build

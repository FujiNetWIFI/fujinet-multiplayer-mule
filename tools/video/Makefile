# The Intellivision new-player intro video (about 3 minutes, 1080p60).
# Needs the built ROM with the converted art: make -C ../../clients/intv
# (with mule.atr present), plus jzIntv for the clips.
ROOT  := ../..
INTV  := $(ROOT)/clients/intv
BUILD := $(ROOT)/videos/build
OUT   := $(ROOT)/videos/intv-intro.mp4

all: $(OUT)

$(BUILD)/music.wav: chip.py
	mkdir -p $(BUILD)
	python3 chip.py $@

$(BUILD)/clips/.done: clips.py $(INTV)/mule.rom
	python3 clips.py $(BUILD)/clips
	touch $@

$(OUT): render.py romgfx.py $(BUILD)/music.wav $(BUILD)/clips/.done $(INTV)/mule.rom
	python3 romgfx.py --check
	python3 render.py $(BUILD) $@

stills:
	python3 render.py $(BUILD) --still 5 18 26 40 55 64 75 85 92 100 110 125 138 152 170 178 190 200 208 218 224 228 236

clean:
	rm -rf $(BUILD) $(OUT)

.PHONY: all stills clean

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

$(OUT): render.py romgfx.py vidlib.py $(BUILD)/music.wav $(BUILD)/clips/.done $(INTV)/mule.rom
	python3 romgfx.py --check
	python3 render.py $(BUILD) $@

stills:
	python3 render.py $(BUILD) --still 5 18 26 40 55 64 75 85 92 100 110 125 138 152 170 178 190 200 208 218 224 228 236

clean:
	rm -rf $(BUILD) $(OUT)

.PHONY: all stills clean

# ---- the Atari 8-bit video: videos/atari-intro.mp4 ------------------------
# Needs the Atari client (make -C ../../clients/mekkogx atari, with mule.atr)
# and its libatari800 harness (clients/mekkogx/tools/atarisim/asim).
MX    := $(ROOT)/clients/mekkogx
ABLD  := $(BUILD)/atari
AOUT  := $(ROOT)/videos/atari-intro.mp4

atari: $(AOUT)

$(ABLD)/music.wav: pokey.py chip.py
	mkdir -p $(ABLD)
	python3 pokey.py $@

$(ABLD)/clips/.done: clips_atari.py $(MX)/r2r/atari/mule.atr
	python3 clips_atari.py $(ABLD)/clips
	touch $@

$(AOUT): render_atari.py mgx_scenes.py atarigfx.py vidlib.py $(ABLD)/music.wav $(ABLD)/clips/.done
	python3 atarigfx.py --check $(ABLD)/clips/gfx
	python3 render_atari.py $(ABLD) $@

atari-stills:
	python3 render_atari.py $(ABLD) --still 5 18 26 40 55 64 75 85 92 100 110 125 138 152 170 178 190 200 208 218 224 228 236

.PHONY: atari atari-stills

# ---- the Apple II video: videos/apple2-intro.mp4 ---------------------------
# Needs the Apple II client (make -C ../../clients/mekkogx apple2, with
# mule.atr) and MAME for clients/mekkogx/tools/a2sim.
PBLD  := $(BUILD)/apple2
POUT  := $(ROOT)/videos/apple2-intro.mp4

apple2: $(POUT)

$(PBLD)/music.wav: mockingboard.py chip.py
	mkdir -p $(PBLD)
	python3 mockingboard.py $@

$(PBLD)/clips/.done: clips_a2.py clips_atari.py $(MX)/r2r/apple2/mule.po
	python3 clips_a2.py $(PBLD)/clips
	touch $@

$(POUT): render_apple2.py mgx_scenes.py a2gfx.py vidlib.py $(PBLD)/music.wav $(PBLD)/clips/.done
	python3 a2gfx.py --check
	python3 render_apple2.py $(PBLD) $@

apple2-stills:
	python3 render_apple2.py $(PBLD) --still 5 18 26 40 55 64 75 85 92 100 110 125 138 152 170 178 190 200 208 218 224 228 236

.PHONY: apple2 apple2-stills

# ---- the other clients: videos/<target>-intro.mp4 ---------------------------
# MekkoGX (msdos coco adam msx lynx) and the Astrocade: clips_mgx.py films
# them in their sims (the clients must be built), chips.py voices the tune,
# render_mgx.py renders. The Atari 2600 has no test mode: clips_2600.sh plays
# a whole live game on the client's rig (about an hour), a26clips.py cuts it.
MGX := msdos coco adam msx lynx astrocade
CHIP_msdos := sn76489
CHIP_adam := sn76489
CHIP_coco := coco
CHIP_msx := msx
CHIP_lynx := lynx
CHIP_astrocade := astrocade

$(addprefix $(BUILD)/,$(addsuffix /music.wav,$(MGX))): $(BUILD)/%/music.wav: chips.py chip.py
	mkdir -p $(dir $@)
	python3 chips.py $(CHIP_$*) $@

$(BUILD)/msdos/clips/.done: clips_mgx.py clips_atari.py
	python3 clips_mgx.py msdos-cga $(BUILD)/msdos/clips
	python3 clips_mgx.py msdos-pcjr $(BUILD)/msdos/pcjr develop map
	touch $@

$(addprefix $(BUILD)/,$(addsuffix /clips/.done,$(filter-out msdos,$(MGX)))): $(BUILD)/%/clips/.done: clips_mgx.py clips_atari.py
	python3 clips_mgx.py $* $(BUILD)/$*/clips
	touch $@

$(ROOT)/videos/%-intro.mp4: render_mgx.py mgx_scenes.py mgxgfx.py astrogfx.py vidlib.py $(BUILD)/%/music.wav $(BUILD)/%/clips/.done
	python3 render_mgx.py $* $(BUILD)/$* $@

$(MGX): %: $(ROOT)/videos/%-intro.mp4

$(BUILD)/a2600/music.wav: chips.py chip.py
	mkdir -p $(dir $@)
	python3 chips.py tia $@

$(BUILD)/a2600/clips/.done: clips_2600.sh a2600/film.lua a26clips.py
	./clips_2600.sh $(BUILD)/a2600/clips
	python3 a26clips.py $(BUILD)/a2600/clips
	touch $@

$(ROOT)/videos/atari-2600-intro.mp4: render_mgx.py mgx_scenes.py a26gfx.py vidlib.py $(BUILD)/a2600/music.wav $(BUILD)/a2600/clips/.done
	python3 render_mgx.py a2600 $(BUILD)/a2600 $@

atari-2600: $(ROOT)/videos/atari-2600-intro.mp4

everything: $(OUT) $(AOUT) $(POUT) $(MGX) atari-2600

.PHONY: $(MGX) atari-2600 everything

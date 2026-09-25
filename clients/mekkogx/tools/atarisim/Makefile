# tools/atarisim: build asim against libatari800 (atari800 configured with
# --target=libatari800). A800 is that source tree.
A800 ?= $(HOME)/Workspace/atari800-lib
WRAP = -Wl,--wrap=POKEY_PutByte,--wrap=GTIA_PutByte,--wrap=ANTIC_PutByte

asim: asim.c
	gcc -O1 -I$(A800)/src -I$(A800)/src/libatari800 -o $@ $< $(WRAP) \
	  $(A800)/src/libatari800.a -lm -lpthread -lmp3lame -lz -lpng

# atari800 capture harness

This harness plays M.U.L.E. on the atari800 emulator from a script and saves
screenshots and memory dumps. I used it to find the addresses and colours
that `../extract.py` relies on, and you can use it to check a new extraction
against the real game. You don't need it for normal use.

## Build

Build libatari800 from the atari800 source, then compile the harness:

    ./configure --target=libatari800 && make CFLAGS="-O2 -Wno-error"
    gcc -O1 -I$A800/src -I$A800/src/libatari800 -o cap cap.c \
        $A800/src/libatari800.a -lm -lpthread -lmp3lame -lz -lpng

`$A800` is the path to the atari800 source tree.

## Run

    mkdir out && ./cap mule.atr tournament.txt out
    python3 topng.py out/*.scr

Each `shot NAME` line in the script writes three files:

- `NAME.scr`: the 384x240 frame buffer, one Atari colour index per pixel.
- `NAME.mem`: a 64 KB RAM dump.
- `NAME.state`: the full emulator state.

`python3 dl.py out/NAME.mem` decodes the display list in a dump. It shows
which ANTIC mode each screen uses and where its screen memory is.

## Input conventions

libatari800 reads input differently from a real Atari:

- For triggers, `1` means pressed.
- Joystick bits are active-high: up=1, down=2, left=4, right=8, and 0 means centred.

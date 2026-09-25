# M.U.L.E. for FujiNet 8-bit computers (MekkoGX)

A C client for the M.U.L.E. Tournament server in this repository, built with the [MekkoGX](https://github.com/fozzTexx/MekkoGX) template and [fujinet-lib](https://github.com/FujiNetWIFI/fujinet-lib). It is a port of the Intellivision client (`../intv`), and plays the same way:
- it polls `/state?bin=1` and draws what comes back the way the Atari 800 original does;
- it sends the player's moves;
- it plays every animation itself at the original's speed.

There are six platforms:
- the **Apple II**: a 128K enhanced //e, //c or IIgs, in HGR colour, with speaker sound;
- the **TRS-80 Color Computer 1 or 2** with 64K, in PMODE 4 artifact colour, with 1-bit sound;
- the **IBM PC** with 128K and CGA (4 colours, PC speaker), or the **PCjr** (16 colours, its SN76496), under MS-DOS with FUJINET.SYS;
- the **Coleco Adam**, booted from a FujiNet disk or tape, with its SN76489;
- the **MSX** (MSX1 and up), as a ROM the FujiNet cartridge runs, with its PSG;
- the **Atari 400/800** with 48K (and the XL/XE), with the original's own graphics, music and sound effects, from its disk.

The Adam and the MSX share their graphics code (`src/tms9918`): both have a TMS9918A.

The code is split so other MekkoGX targets can follow:

| Where | What |
|---|---|
| `src/*.c` | the game, platform neutral: protocol, phases, walking, the sprite scripts, text |
| `src/platform-specific/*.h` | what a platform provides: screen and sprites (`gfx.h`), input, sound, the 60 Hz clock (`plat.h`), far memory and overlays (`bank.h`) |
| `src/apple2/` | the Apple II implementation |
| `src/coco/` | the CoCo 1/2 implementation (CMOC) |
| `src/msdos/` | the PC / PCjr implementation (Open Watcom) |
| `src/tms9918/` | the TMS9918A screen and sprites, for the Adam and the MSX (z88dk) |
| `src/adam/`, `src/msx/` | the rest of the Adam and MSX implementations |
| `src/atari/` | the Atari implementation (cc65, much of it 6502 assembly) |

A new platform is a `src/<platform>/` directory implementing those headers, plus its line in `PLATFORMS`.

## Building

    make apple2                               # r2r/apple2/mule.po (ProDOS, 140K)
    make apple2 ATR="M.U.L.E. (1983)(Electronic Arts)(US).atr"
    make apple2 ENDPOINT=http://myhost:8080   # server used when the lobby didn't choose one (remembered)

**What the build needs:**
- cc65 (2.19), AppleCommander (`ac`, `acx`), Python 3 with Pillow;
- fujinet-lib, built, in `~/Workspace/fujinet-lib` (or `make FUJINET_LIB=/path`).

**Graphics.**
- **From the disk.** The font, map characters, mountains, species, map figures, town, ships and event sprites are converted from the original Atari disk (CRC32 F76D2896) by `tools/mkgfx_hgr.py`. The disk comes from `ATR=`, or from a `mule.atr` next to the Makefile.
- **Copyright.** The converted graphics belong to their copyright holders. They are written only to `gen/`, which is ignored by git; never commit them.
- **Without the disk** the build uses stand-in art and warns you. It plays, but it looks nothing like M.U.L.E.
- **Previews.** The conversion also writes NTSC-style previews of the town and the plots to `gen/preview/`.

## Running

Mount `r2r/apple2/mule.po` on a FujiNet (or in an emulator with FujiNet SmartPort, e.g. AppleWin's SmartPort-over-SLIP card with fujinet-pc) and boot it.

The FujiNet lobby passes the game its details in appkeys (creator 1, app 1):

| Key | Contents |
|---|---|
| 0 | the player's name, shared by every FujiNet game |
| 10 | the table's URL: `https://host/?table=<id>` |

**If the lobby didn't set them:**
- the game asks for a name (type it, RETURN);
- it lists the default server's tables.

The chosen table is written back to key 10, so restarting rejoins it.

## Controls

| Key | Does |
|---|---|
| arrows, or I J K M | walk; push up/down in auctions; left/right in the lobby |
| space, or a joystick button / apple key | the button |
| RETURN | end your development turn |
| 0 | declare: sit this good out |
| ESC | leave the table (lobby); at the table list, quit |
| ctrl-S | sound off / on |

A joystick works too if one is centred when the game starts.

## The Apple II version

**The screen: HGR page 1.**
- **Scaling.** Atari colour clocks become colour pixels at 7/8 (160 across becomes 140). A map plot is exactly 4 bytes wide and starts on a byte; scanlines are kept 1:1.
- **Colours.** Players are blue, violet, green and orange, which are HGR's four colours. White text is 40 columns. Coloured text is double width (20 columns), because single-width coloured text doesn't survive NTSC artifacting.
- **Sprites.** They are software sprites with save-under (`src/apple2/gfx.c`, `hires.s`), drawn a colour pixel at a time.

**Memory: 128K is used.**
- **Main RAM** holds the core. HGR page 1 sits at $2000-$3FFF. The language card holds a little more code.
- **Overlays.** Each phase's code is an overlay (boot, lobby and summary, grant and production, auctions, development).
- **Aux RAM** holds all five overlays, plus the sprites, the town and the strings.
- **Loading.** Everything is read from `MULE.1`…`MULE.7` at start-up. At each phase change the phase's overlay is copied into an 8K window with AUXMOVE.
- **Memory map.** See `src/apple2/mule-hgr.cfg`.

**Timing.**
- **Counting frames.** The clock is the vertical blank ($C019, or the //c's VBL flag). The drawing routines check it every few milliseconds, so a busy frame still counts every blank.
- **Catching up.** The game logic (walking, the grant cursor, auction lines, the scripts) runs once for every frame that passed. The game keeps the Atari's speeds even when drawing can't keep up.
- **Network calls block** while they run. The server clock (`elapsed`) resynchronises the phase timers on every reply.

## Testing

**Offline, in MAME.** `tools/a2sim/a2sim.py` boots the disk in MAME's enhanced //e (headless), drives the game from a scenario and saves screenshots. The input and the test mode go through hooks in the game (`input.c`, `test.c`), found from the linker map.

    python3 tools/a2sim/a2sim.py r2r/apple2/mule.po build/mule-apple2.map scenario.txt out/

- A scenario that starts with `test <phase> [arg [event]]` runs the game on a fake state with no server. It covers walking the town and map, every colony event (`test 4 <event>`), the auctions, the summary and game over.
- **Other commands:** `wait`, `snap`, `hold`, `press`, `key`, `peek`, `poke`, `pc`, `dump`, and `trace` (MAME's debugger trace).
- **Frame budget.** `tools/a2sim/gaps.py` reads a trace and reports the longest stretches between clock checks, i.e. where frames get missed.
- MAME needs the `apple2ee` ROMs.

**Soak tests against a real server.** `make apple2 AUTOPLAY=1` (after `make clean`) builds a client that plays by itself: it readies, claims, takes its turns and sits out goods. Run it in AppleWin (headless) with fujinet-pc against a private server, and watch the game go round.

## The CoCo version

**Building.**

    make coco                                  # r2r/coco/mule.dsk (Disk BASIC, 160K)
    make coco ENDPOINT=http://myhost:8080

- **What it needs:** CMOC 0.1.100 and lwtools (`lwasm`, `lwlink`), ToolShed's `decb` (found in `~/Workspace/toolshed/build/unix/decb/`, or `make DECB=...`), Python 3 with Pillow, and fujinet-lib built for `coco`.
- **Graphics:** `tools/mkgfx_pmode4.py` converts them from the same Atari disk (`ATR=`, or `mule.atr`), into `gen/coco/`. The Atari extraction is `mkgfx_hgr.py`'s. As on the Apple, there is stand-in art without the disk.
- **Optimisation:** the build uses `-O1`. CMOC 0.1.100's `-O2` has a bug: after `x = a[i]`, a following `b[i]` is indexed by `x`.
- **Rebuilding:** a module's section (below) is set by the Makefile. Changing it doesn't rebuild the module: `make clean` first.

**Running.** Mount `mule.dsk` on the FujiNet and boot. HDB-DOS runs `AUTOEXEC.BAS`, which does `PCLEAR1:LOADM"MULE":EXEC`. The `PCLEAR1` moves the BASIC program out of the way of `MULE.BIN`.

**The screen: PMODE 4 at $0E00.**
- **Colours:** two neighbouring pixels are one artifact colour pixel (128 across). The players are blue, orange, white, and blue/orange stripes (blue on even lines, orange on odd).
- **Colour phase:** a CoCo 1/2 comes up with blue and orange either way round, so the game asks at start-up ("IS THE BAR BLUE?"), and CLEAR swaps them at any time.
- **Scaling:** Atari colour clocks x become colour pixels x - x/4 + 2 (160 -> 120).
  - A map plot is 12 colour pixels, 3 bytes on a byte.
  - Text is 40 columns of 6-pixel cells; the Atari font is squeezed from 7 pixels to 5.
  - Coloured text is double width, as on the Apple.
  - Characters' sprites keep their 8 colour pixels; the ship, cloud and plot cursor are squeezed like the map.
- **Drawing:** sprites are software sprites with save-under (`src/coco/gfx.c`). The inner loops (sprite rows, glyphs, plots) are 6809 assembly.

**Memory: 64K, all RAM once running.** See `src/coco/mule.link`.

| Where | What |
|---|---|
| $0000-$0DFF | BASIC/DOS variables, kept: DSKCON and DriveWire use them |
| $0E00-$25FF | the screen |
| $2600-$7BFF | the low core, `MULE.BIN` (LOADM'd by BASIC): everything that runs before the rest is loaded, all variables, the strings, the town |
| $7C00-$7FFF | the stack (1K) |
| $8000-$BFEF | the high core, `MULEHI.BIN`: graphics code and data, text, input, sound |
| $BFF0-$BFFF | the CPU vectors (in all-RAM mode the SAM fetches them from here) |
| $C000-$DFFF | the Disk ROM, copied to RAM: DSKCON, and the DriveWire routines that fujinet-lib calls through $D93F/$D941 |
| $E000-$F9FF | the overlay window: `MULE1`...`MULE5.BIN`, one phase's code at a time |
| $FA00-$FEFF | the sprites' save-unders |

How it gets there:
- **Start-up:** `bank_init` copies the Disk ROM and the vectors to RAM, switches the SAM to all-RAM, and reads `MULEHI.BIN` with DSKCON.
- **Overlays:** at a phase change, `bank_overlay` reads that phase's overlay the same way. Over DriveWire that takes about a second.
- **Placing modules:** CMOC has no section pragma, so:
  - `tools/coco/lwasm-sect` renames a module's sections, as the Makefile's `MULE_SECT` says;
  - `tools/coco/lwlink-mule.py` links with the memory map, checks every region's limit, and splits the output into the files above.

**Timing.**
- **The clock:** the VDG's field-sync interrupt counts the frames and scans the keyboard, so a quick key press isn't lost while the game is busy.
- **Network calls:** the DriveWire ROM masks interrupts while it talks to the FujiNet. The server's clock (`elapsed`) resynchronises the game after each call.

**Testing.**
- **Offline:** `tools/cocosim/cocosim.py` is `a2sim.py` for MAME's CoCo 2 (`coco2b`, Disk BASIC), with the same scenario language and test hooks:

      python3 tools/cocosim/cocosim.py r2r/coco/mule.dsk build/mule-coco.map scenario.txt out/

  - `tools/cocosim/snap.sh` takes plain timed screenshots.
  - `addr2sym.py` turns `pc` samples into function names.
  - The stack's unused part is filled with $A5 at start-up; `dump 7C00 7FFF` shows how deep it has been.
- **Against a server:**
  1. Start fujinet-pc (CoCo build) with BoIP on 65504 and the disk mounted.
  2. Run cocosim with `COCO_BECKER=1 COCO_BIOS=hdbk12` and `-` as the disk. It boots HDB-DOS over the Becker port and runs in real time.
  - `make coco AUTOPLAY=1` plays by itself.
  - MAME's Becker port connects at start-up even when it is off, so an offline run waits while a FujiNet is listening on 65504.

**Controls.**

| Key | Does |
|---|---|
| arrows (or the right joystick) | walk, push, choose |
| SPACE (or the joystick button) | the button |
| ENTER | end your development turn |
| BREAK | leave the table (lobby); at the table list, quit (a cold start) |
| 0 | declare: sit this good out |
| LEFT | also rubs out when typing a name |
| CLEAR | swap the artifact colours |
| SHIFT-CLEAR | sound off / on |

## The MS-DOS version

**Building.**

    make msdos                                 # r2r/msdos/mule.img (360K, MULE.EXE + AUTOEXEC.BAT)
    make msdos ENDPOINT=http://myhost:8080

- **What it needs:** Open Watcom 2. If `wcc` isn't installed, `tools/dos/ow` runs it in the [defoogi](https://github.com/FozzTexx/defoogi) container, which sees only this repository.
- **fujinet-lib:** fujinet-lib-experimental, from `~/Workspace/fujinet-lib-experimental` (or `make FNLIB_EXP=/path`). `tools/dos/mkfnlib.sh` builds it into `build/fnlib-msdos/` in the medium memory model; its release is small model and wouldn't link.
- **Graphics:** `tools/mkgfx_dos.py` converts them from the same Atari disk into `gen/msdos/`, with previews of the town in both modes in `gen/preview/msdos/`.
- **Rebuilding:** `make clean` after changing `AUTOPLAY`.

**Running.** Boot DOS with FUJINET.SYS (for example `DEVICE=FUJINET.SYS FUJI_PORT=2 FUJI_BPS=115200` on a PCjr, `FUJI_PORT=1` elsewhere), mount `mule.img` and run `MULE`. ESC at the table list mounts the lobby (`ec.tnfs.io`, `msdos/lobby.img`) in device slot 0 and runs its `AUTOEXEC.BAT` when the game exits: the game types it into the keyboard buffer rather than spawning COMMAND.COM, so all its memory is free for the lobby.

**The screen.** 320 x 200 is exactly two pixels a colour clock, so the Atari art is drawn unscaled: 40 columns of 8-pixel text, the 192 lines 4 down.
- **CGA:** mode 4, palette 1 (cyan, magenta, white). The players are cyan, magenta, white, and cyan/magenta stripes. Colour 0 is the background register: grey behind the map, as on the Atari, black elsewhere. On EGA, VGA or a PCjr in this mode the BIOS also sets the palette registers, and cyan and magenta become light blue and light magenta. A real CGA ignores that.
- **PCjr:** mode 9, 16 colours, each Atari colour its nearest RGBI: light blue, light magenta, green and orange (light red) players, a grey map with black mountains, the town in its own colours.
- **The PCjr's screen memory:** in 16 colours the game draws straight into the 32K of RAM the screen is shown from (the CPU page, from INT 10h AX=0580h). The B800 window is only 16K wide on a real PCjr. MAME maps all 32K there, so it doesn't show the difference.
- **Drawing:** sprites are software sprites with save-under (`src/msdos/gfx.c`, the CoCo's slot logic). Plots and sprite rows are drawn a word at a time through nibble-to-mask tables. The 8088's shifts by a count are too slow for anything else.

**Memory.** Open Watcom's medium model: far code, near data. Each phase is an overlay (the Apple's groups), loaded from `MULE.EXE` by Watcom's dynamic overlay manager when it's first called (`tools/dos/wlink-mule.py`). The game is about 54K resident, with a 1K stack (about 300 bytes are used).
- **The tight case is a 128K PCjr in 16 colours.** Its screen is the last 32K of the 128K (from 18000h). The BIOS keeps only the top 16K from DOS, so the game claims the rest itself, and everything must end below 18000h. With PC-DOS 2.1 and FUJINET.SYS that leaves about 5K, which goes to the overlay manager. If there isn't room (a bigger DOS, another resident program), the PCjr uses the 4-colour mode instead.
- **Elsewhere** the overlay manager gets 16K more, so every overlay stays loaded after its first use.

**Timing.**
- **The clock:** the PIT runs at 60 Hz, and the timer interrupt counts the frames. Every 3.3rd tick goes on to the BIOS, so the time of day and the floppy motor still work.
- **The keyboard:** INT 9 is hooked to know which keys are held. On a PC the scan code comes from port 60h; on a PCjr the BIOS passes it in AL. The BIOS still queues typed characters.
- **Network calls** go through FUJINET.SYS (INT F5h) and block. The server's clock resynchronises the game after each one.
- **Without FUJINET.SYS**, INT F5h may point anywhere (on the PCjr it's left over from BASIC). FUJINET.SYS is recognised by its device header (attributes 6002h) in the vector's segment. If it isn't there, the game answers INT F5h itself and says NO FUJINET FOUND.

**Testing.** `tools/dossim/dossim.py` is `cocosim.py` for MAME's `ibmpcjr` (128K) or `ibm5150` (CGA). It boots a copy of a DOS disk (`DOSSIM_BOOT`, by default PC-DOS 2.1 with FUJINET.SYS) with `MULE.EXE` on it, and uses the same scenario language and test hooks:

    python3 tools/dossim/dossim.py r2r/msdos/mule.exe build/mule-msdos.map scenario.txt out/
    DOSSIM_MACHINE=ibm5150 DOSSIM_NOFN=1 python3 tools/dossim/dossim.py ...

- The game's data segment is found at run time, from DS once the title has written `3C C3`.
- `peek plat_late 2` counts the frames the game missed.
- `stack` says how much of the stack (filled with A5h at start-up) was never used.
- `DOSSIM_EARLYPC=frame` samples CS:IP before the game is found.
- `DOSWAV=file.wav` records the sound.
- **Against a server:** start fujinet-pc (RS232 build) with BoIP on a free port, then `DOSSIM_FUJINET=127.0.0.1:port`. MAME's serial port becomes a null modem to it, and the run is in real time. `make msdos AUTOPLAY=1` plays by itself.

**Controls.**

| Key | Does |
|---|---|
| arrows, keypad (7 9 1 3 diagonals), or joystick A | walk, push, choose |
| SPACE (or a joystick button) | the button |
| ENTER | end your development turn |
| ESC | leave the table (lobby); at the table list, back to the FujiNet lobby |
| 0 | declare: sit this good out |
| BACKSPACE | rub out, when typing a name |
| F1 | sound off / on |

## The Coleco Adam and MSX versions

**Building.**

    make adam                                  # r2r/adam/mule.ddp (tape) and mule.dsk (disk)
    make msxrom                                # r2r/msxrom/mule[ASCII8].rom (and mule.rom, the same)
    make adam msxrom ENDPOINT=http://myhost:8080

- **What it needs:** z88dk (zcc with zsdcc). If `zcc` isn't installed, `tools/z80/z88dk` runs it in the defoogi container, which sees only this repository.
- **fujinet-lib:** fujinet-lib-experimental (`FNLIB_EXP`, as for MS-DOS). `tools/z80/mkfnlib.sh` builds it with zsdcc into `build/fnlib-adam/` and `build/fnlib-msx/`. The released libraries are sccz80 builds, and the copies get a few local fixes, which the script lists (mostly in the MSX bus: see *Known limits*).
- **Graphics:** `tools/mkgfx_tms.py` converts them from the same Atari disk into `gen/tms9918/gfx.c`, one file for both machines, with previews in `gen/preview/tms9918/`.
- **Rebuilding:** `make clean` after changing `AUTOPLAY`.

**The screen** (`src/tms9918/gfx.c`). Both machines use Graphics II, a 256 x 192 bitmap in which each 8-pixel row of a cell has two colours.
- **x is scaled 3/2 from the Atari's colour clocks, from 4 pixels in.** A plot's 16 clocks become exactly three cells, so plots are made whole, cell by cell. Text is 40 columns of 6-pixel glyphs (the CoCo's squeeze of the Atari font), and big text is 12 pixels a character. Lines are the Atari's own.
- **Drawing** reads a cell's 8 pattern and 8 colour bytes, paints them and writes them back. A colour a row doesn't have replaces the one with fewer pixels left, and those pixels change colour: that is the colour clash.
- **The town** is converted cell by cell by `tools/mkgfx_tms.py`: each row keeps its two commonest colours, and the rest go to the nearer one (96 pixels in all).
- **The colours** are the Atari's nearest, from TMS9918's 15. The players are light blue, magenta, green and orange (light red). The map is grey, with black mountains and a light green river.
- **Sprites** are the VDP's own: 16 x 16, one colour each, with no save-under. A slot is up to four of them, with its own pattern names, and a pattern is sent only when its frame changes. The shapes stay 1 bit a colour clock and are widened as they are sent (`wide12`). With a fifth sprite on a line the order rotates a frame at a time, so they flicker rather than vanish.

**The Adam.**
- **Memory** (`src/adam/mule.asm`):

  | Range | Use |
  |---|---|
  | 0000h | the program, about 31K, then its variables |
  | A000h-B7FFh | the overlay window |
  | down from D390h | the stack, below EOS |

  z88dk's bootstrap loads the program a 1K block at a time. Each phase is an overlay that `bank.c` reads from the boot medium, in whole blocks, at a phase change. `tools/adam/mkimage.py` puts the overlays after the program, fills in the program's table of where they are, and checks the map.
- **The clock** is the VDP's frame interrupt, which is the Z80's NMI (`nmi.asm`). The NMI can't be masked, so while the game is at the VDP it only notes that the interrupt came (`vdp_busy`), and `VDP_UNLOCK` reads the status then. The clock keeps counting while AdamNet calls wait.
- **EOS** is called directly (`eoscall.asm`): block reads, the keyboard and the controllers. z88dk's `eos.lib` is an sccz80 build, whose arguments come in the other order from zsdcc's.
- **Keys:** the arrows walk, SPACE or a fire button is the button, RETURN ends a turn, ESCAPE leaves, 0 sits out, BACKSPACE rubs out, and SmartKey VI turns the sound off and on. On the controller's keypad, * ends a turn and 0 sits out. The keyboard sends characters, not keys held, so an arrow counts as held for a third of a second after each one.
- **ESCAPE at the table list** sets the FujiNet to boot CONFIG and restarts the Adam.

**The MSX.**
- **Memory: an ASCII8 megaROM** (`src/msx/mule.asm`, built by `tools/msx/mkrom.py`):

  | Range | Use |
  |---|---|
  | 4000h-9FFFh | banks 0-2, fixed: the program |
  | A000h-BFFBh | the window: overlays, the art and the strings take turns, each a single bank write |
  | BFFCh-BFFFh | the FujiNet's registers, in every bank |
  | C000h | RAM: the variables, and the stack below HIMEM |

  Drawing and `bank_get` switch the window themselves and put the phase's bank back. The header (`src/msx/crt/msx_rom_header.asm`) maps banks 1-3 before anything outside 4000h-5FFFh runs.
- **The FujiNet runs it.** Mount `mule[ASCII8].rom` in a device slot and boot: the firmware sends the ROM into the cartridge and starts it, choosing the mapper by that end of the name. This needs firmware with user ROMs (the `feat/msx-mapper-types` work; the master build of fujinet-pc-msx here mounts it as a disk instead).
- **The clock** is the BIOS's interrupt, through the H.TIMI hook. The BIOS reads the VDP's status, so VRAM bursts run with interrupts off.
- **Keys:** the arrows or joystick 1 walk, SPACE or the trigger is the button, RETURN ends a turn, ESC leaves, 0 sits out, BS rubs out, and F1 turns the sound off and on.
- **ESC at the table list** switches the cartridge back to the FujiNet's own ROM and restarts.

**Testing.**
- **The Adam, offline:** `tools/adam/adamsim/adamsim.py` is `cocosim.py` for MAME's `adam` driver, with the tape (or the disk). MAME's tape is slow, so allow about 40 s of emulated time to boot and a long wait before the first snapshot:

      python3 tools/adam/adamsim/adamsim.py r2r/adam/mule.ddp r2r/adam/mule.map scenario.txt out/

  It adds `vdump FILE` (saves VRAM) and `vwatch FROM TO` (prints the PC of each VRAM write in the range).
- **The MSX, offline:** `tools/msx/msxsim/msxsim.py` runs openMSX (C-BIOS by default; `MSXSIM_MACHINE`) with the renderer off. Each `snap` saves VRAM, and `tools/tmsshot.py` draws it:

      python3 tools/msx/msxsim/msxsim.py r2r/msxrom/mule.rom r2r/msxrom/mule.map scenario.txt out/

- **Against a server** (with `AUTOPLAY=1`, plus a name and a table in the FujiNet's `SD/FujiNet/00010100.key` and `0001010a.key`):
  - **Adam:** ADAMEm (`adamem -fujinet -if 60`) first, then fujinet-pc (ADAM build) with `mule.ddp` mounted and CONFIG off.
  - **MSX:** fujinet-pc (RS232 build with user ROMs) with `mule[ASCII8].rom` mounted, then the Workspace openMSX with `-ext FujiNet`. Choose boot at CONFIG.
  - Both have played whole rounds against the server this way.

## The Atari version

The Atari is the machine M.U.L.E. was written for, so this version draws and sounds the way the original does, with the original's own data. `tools/mkgfx_atari.py` and `tools/mksnd_atari.py` copy it from the disk (`ATR=`, as for the other versions) into `gen/atari/`, which is never committed.

**Building.**

    make atari                                 # r2r/atari/mule.atr (boots with picoboot)
    make atari ENDPOINT=http://myhost:8080

- **What it needs:** cc65 2.19, `dir2atr` (AtariSIO) and `atr` (jhallen/atari-tools, built in `~/Workspace/atari-tools`; the Makefile puts it on the path), Python 3, and fujinet-lib's Atari library in `~/Workspace/fujinet-lib/build`. The first build downloads `picoboot.bin` into `_cache/atari`.
- **The disk** is `MULE.COM` and the overlays `PHASE.1`-`PHASE.5`. picoboot runs the first file, so the overlays are named to sort after it.

**Memory** (a 48K 400/800; `src/atari/mule-atari.cfg`, `src/atari/atari.h`):

| Range | Use |
|---|---|
| $0400-$09FF | variables that need not be loaded (picoboot's area; cleared at start) |
| $0A00 | the program: code, data, then its variables |
| $8DE0-$A67F | the overlay window: one phase at a time, read from the disk by sector |
| $A680-$A7FF | the C stack |
| $A800 | the drawing characters (mode 4 cells made on the fly) |
| $AC00, $AE00 | the original's font and map characters (64 each) |
| $B000 | the original's town characters; its unused ones hold the small sprites |
| $B400 | the screen: 24 rows of 40 bytes |
| $B800 | PMG: the display list and the row tables below the missiles and players |

- **The load chunk** (`src/atari/start.s`) runs first. It turns BASIC off on an XL/XE, checks for RAM up to $BFFF, blanks the screen, and lowers MEMTOP to put the C stack under the graphics.
- **Overlays:** there is no DOS. `bank.c` finds `PHASE.n` in the DOS 2 directory and follows the sector links with DSKINV.
- **The overlay window** is fixed below the stack, so the program can grow up to it. The link fails if it doesn't fit.

**The screen** (`gfx.c`, `gfxa.s`, `draw.s`, `cells.s`, `mapgfx.s`, `sprite.s`) is a character screen, as the original's are.
- **Rows:** the display list is 24 rows of 8 scanlines. Each row has its own ANTIC mode, and a DLI loads the next row's CHBASE, colours, player positions, player colours and sizes from row tables, but only what changes. The immediate VBI loads row 0's registers and their shadows. Registers a row doesn't use take the next row's values, and colour slots avoid changing between rows, because a change lands a few cycles into the row's first scanline.
- **Text** uses the original font in mode 2. Big text is mode 6, in up to four colours a row.
- **Drawing:** fills, dotted lines and pictures make a row mode 4 and draw into characters taken from a pool of 127. A row with both big and small text becomes a drawing row too: big characters are exactly two cells, and small ones are squeezed to every other pixel.
- **The map** is mode 6 with the original's map characters: plot borders, icons, production dots and the time bar. Player 2 draws the mountains, one per display row, repositioned row by row as the original's DLI does. Player 3 draws the river, and the store in its rows at double width in black.
- **The town** is the original's own screen and characters, with its colours. The corral shows its M.U.L.E.s as the original draws them.
- **Sprites** are players, single line. A player can show several sprites whose rows don't meet: the DLI moves it between them. On the map players 0 and 1 are free; in town, 0-2, with missile 3 the time bar.
- **Shapes** come from the original: the species, the map figures, the M.U.L.E., mountains, store, ships, pest, cloud and bolt. The wampus, meteor and the ship's hull are still the Intellivision's.

**Sound** (`pokey.s`, `sound.c`). The original's sounds, reverse-engineered and played by a driver in the immediate VBI:
- **The title theme** plays in the lobby, and **the round's tune** on the status screens (its length grows every 4 rounds, as the original's does). The driver is the original's four-voice engine: note streams with a held attack and a decay per voice. It matches the original frame for frame.
- **The voice-4 effects** use the original's tables: the player blip, the message beep (four times when an auction ends), and the production chirp.
- **Other effects:**
  - each species' footstep;
  - the time bar's tick, faster as time runs out;
  - the colony ship's drone;
  - the colony events: pest, pirates, acid rain, planetquake, meteorite, radiation, fire;
  - the good and bad news jingles;
  - outfitting a M.U.L.E.

  These are scripts, written from how the original plays them. They are started through `SND_X()` in the shared code (`sound.h`), which is empty on the other platforms.
- **During SIO,** which uses voices 3 and 4 and AUDCTL, only voices 1 and 2 are written.
- **Where the data lives:** the tunes are in the status overlay, the colony events in production's and the jingles in develop's. They stop when their overlay goes.

**Keys:** the stick or the arrow keys (with CONTROL) walk, and the trigger or SPACE is the button. RETURN ends a turn, ESC leaves, 0 sits out, and CONTROL-S turns the sound off and on.

**Testing** (`tools/atarisim/`). `asim` is a scripted libatari800: atari800's source configured with `--target=libatari800` and built (`~/Workspace/atari800-lib`, a copy of the atari800 tree, so the emulator build stays as it is):

    make -C tools/atarisim A800=~/Workspace/atari800-lib
    python3 tools/atarisim/atarisim.py r2r/atari/mule.atr build/mule-atari.map scenario.txt out/

- **Scenarios** use `tools/a2sim`'s language, plus `wav` and `pokey`, which record the sound and the POKEY writes. `asim` itself also logs GTIA and ANTIC writes, dumps registers with each shot, and can boot from FujiNet.
- **Against a server:** run fujinet-pc (Atari build) with `[BOIP] port=` set to asim's NetSIO port, CONFIG off, and `mule.atr` mounted. Then:

      asim -netsio 9995 - script.txt out/

  In that script, `coldstart` once FujiNet is there. With NetSIO on, asim keeps to 60 frames a second and turns the SIO patch off.

## Known limits

- **Assay results in town only**, as in the Intellivision version: the server doesn't track where anyone stands.
- **Other players' walks are reconstructed.** Computer turns are replayed as walks.
- **Grey.** HGR has no grey: the plains are black and the mountains white.
- **Network pauses.** Animations pause briefly while a network call runs, because the Apple II has nothing to run them during a SmartPort call (the CoCo, during a DriveWire call).
- **CoCo: phase changes** pause about a second while the phase's overlay is read.
- **CoCo: grey** is black or white, and the fourth player is striped.
- **MS-DOS: phase changes** pause while an overlay is read the first time; on a 128K PCjr in 16 colours, every time. Over FUJINET.SYS at 9600 bps that is a few seconds, and every server poll stalls the animations (about 13 frames a second get drawn): use 115200.
- **MS-DOS, CGA:** the fourth player is striped, and the fence around the town's corral is white.
- **Adam: phase changes** read the phase's overlay (up to 6 blocks). That is quick from a FujiNet, and slow from a real tape.
- **Adam: arrow keys** repeat rather than hold (the keyboard sends characters): walking is smoother with a controller.
- **TMS9918: colour clash.** Text or a sprite-free drawing next to another colour can recolour a few pixels of its cell's row.
- **MSX: fujinet-lib-experimental's MSX bus** needed fixes for zsdcc, all in `tools/z80/mkfnlib.sh`:
  - its port routines used IX and IY, which zsdcc keeps;
  - its variadic `fuji_bus_call` read 8-bit arguments with `va_arg(ap, uint8_t)`;
  - `network_bus_read` and `network_bus_write` used the renamed `FUJICMD_READ`/`FUJICMD_WRITE`;
  - it has no appkey calls (MS-DOS's are used);
  - the URL is sent with its NUL, because the firmware read past its end.
- **MSX: 50 Hz machines** count 50 frames a second. The server's clock keeps the game's timing, but local animations run 5/6 as fast.
- **Atari: phase changes** read the phase's overlay (up to 50 sectors) from the FujiNet at 19200 bps: about 3 s.
- **Atari: text on the map or town** is only in its text rows. The corral's price is under the town, not in the corral.
- **Atari: colours per row.** A drawing or big-text row has four colours at most, and a row's colours change a few cycles late, so a pixel on the first scanline of a row can show the row above's colour.
- **Atari: sprites** share four players. One that finds no player free in its rows isn't shown.
- **Atari: the M.U.L.E.** faces left as a mirror image, not the original's own left frames.


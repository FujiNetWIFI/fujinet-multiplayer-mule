# M.U.L.E. for FujiNet 8-bit computers (MekkoGX)

A C client for the M.U.L.E. Tournament server in this repository, built with the [MekkoGX](https://github.com/fozzTexx/MekkoGX) template and [fujinet-lib](https://github.com/FujiNetWIFI/fujinet-lib). It is a port of the Intellivision client (`../intv`), and plays the same way:
- it polls `/state?bin=1` and draws what comes back the way the Atari 800 original does;
- it sends the player's moves;
- it plays every animation itself at the original's speed.

There are three platforms:
- the **Apple II**: a 128K enhanced //e, //c or IIgs, in HGR colour, with speaker sound;
- the **TRS-80 Color Computer 1 or 2** with 64K, in PMODE 4 artifact colour, with 1-bit sound;
- the **IBM PC** with 128K and CGA (4 colours, PC speaker), or the **PCjr** (16 colours, its SN76496), under MS-DOS with FUJINET.SYS.

The code is split so other MekkoGX targets can follow:

| Where | What |
|---|---|
| `src/*.c` | the game, platform neutral: protocol, phases, walking, the sprite scripts, text |
| `src/platform-specific/*.h` | what a platform provides: screen and sprites (`gfx.h`), input, sound, the 60 Hz clock (`plat.h`), far memory and overlays (`bank.h`) |
| `src/apple2/` | the Apple II implementation |
| `src/coco/` | the CoCo 1/2 implementation (CMOC) |
| `src/msdos/` | the PC / PCjr implementation (Open Watcom) |

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

## Known limits

- **Assay results in town only**, as in the Intellivision version: the server doesn't track where anyone stands.
- **Other players' walks are reconstructed.** Computer turns are replayed as walks.
- **Grey.** HGR has no grey: the plains are black and the mountains white.
- **Network pauses.** Animations pause briefly while a network call runs, because the Apple II has nothing to run them during a SmartPort call (the CoCo, during a DriveWire call).
- **CoCo: phase changes** pause about a second while the phase's overlay is read.
- **CoCo: grey** is black or white, and the fourth player is striped.
- **MS-DOS: phase changes** pause while an overlay is read the first time; on a 128K PCjr in 16 colours, every time. Over FUJINET.SYS at 9600 bps that is a few seconds, and every server poll stalls the animations (about 13 frames a second get drawn): use 115200.
- **MS-DOS, CGA:** the fourth player is striped, and the fence around the town's corral is white.

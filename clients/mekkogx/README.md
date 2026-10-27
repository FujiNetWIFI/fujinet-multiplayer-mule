# M.U.L.E. for FujiNet 8-bit computers (MekkoGX)

A C client for the M.U.L.E. Tournament server in this repository, built with the [MekkoGX](https://github.com/fozzTexx/MekkoGX) template and [fujinet-lib](https://github.com/FujiNetWIFI/fujinet-lib). It is a port of the Intellivision client (`../intv`), and plays the same way:
- it holds a stream-mode connection to the server (`docs/PROTOCOL.md`) and draws what comes back the way the Atari 800 original does;
- it sends the player's moves;
- it plays every animation itself at the original's speed.

There are eleven platforms:
- the **Apple II**: a 128K enhanced //e, //c or IIgs, in HGR colour, with speaker sound;
- the **TRS-80 Color Computer 1 or 2** with 64K, in PMODE 4 artifact colour, with 1-bit sound;
- the **IBM PC** with 128K and CGA (4 colours, PC speaker), or the **PCjr** (16 colours, its SN76496), under MS-DOS with FUJINET.SYS;
- the **Coleco Adam**, booted from a FujiNet disk or tape, with its SN76489;
- the **MSX** (MSX1 and up), as a ROM the FujiNet cartridge runs, with its PSG;
- the **Atari 400/800** with 48K (and the XL/XE), with the original's own graphics, music and sound effects, from its disk;
- the **Atari Lynx**, as a cart image, with FujiNet-Lynx on ComLynx, and the Atari's music and effects on Mikey;
- the **NES**, as an MMC3 image the FujiNet NES cartridge boots, with the Adam's pictures and effects on its pulse channel;
- the **Sega Master System**, as an image the FujiNet SMS cartridge runs, with the Adam's pictures in Mode 4 and its effects on the SMS's own SN76489;
- the **Atari 7800**, as a SuperGame image the FujiNet 7800 cartridge runs, with the Atari 800's graphics (the original's) drawn by MARIA and the Atari's effects on the cartridge's POKEY;
- the **Heathkit H89** under CP/M, with the H89-ESP32 FujiNet card, drawn on its H19 terminal in the terminal's own characters (and its one beep).

The Adam, the MSX and the SMS share their graphics code (`src/tms9918`): the first two have a TMS9918A, and the SMS shows the same cells in its own mode.

The code is split so other MekkoGX targets can follow:

| Where | What |
|---|---|
| `src/*.c` | the game, platform neutral: protocol, phases, walking, the sprite scripts, text |
| `src/platform-specific/*.h` | what a platform provides: screen and sprites (`gfx.h`), input, sound, the 60 Hz clock (`plat.h`), far memory and overlays (`bank.h`) |
| `src/apple2/` | the Apple II implementation |
| `src/coco/` | the CoCo 1/2 implementation (CMOC) |
| `src/msdos/` | the PC / PCjr implementation (Open Watcom) |
| `src/tms9918/` | the TMS9918A screen and sprites, for the Adam and the MSX (z88dk); the SMS draws with it too |
| `src/adam/`, `src/msx/` | the rest of the Adam and MSX implementations |
| `src/atari/` | the Atari implementation (cc65, much of it 6502 assembly) |
| `src/lynx/` | the Lynx implementation (cc65; its sound is the Atari's `pokey.s`) |
| `src/nes/` | the NES implementation (cc65; the screen is the TMS9918A's, in the PPU's terms) |
| `src/sms/` | the Sega Master System implementation (z88dk; the TMS9918A's cells as Mode 4 tiles) |
| `src/atari7800/` | the Atari 7800 implementation (cc65, the display in 6502 assembly; the Atari's rows and sprites as MARIA's display lists, its sound the Atari's `snd.s`) |
| `src/h89/` | the Heathkit H89 implementation (z88dk, CP/M; the H19 terminal's characters, sent by difference) |

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
- **The font** is the Namco arcade font of the other clients (`tools/tms/namco.bin`), narrowed to 6 pixels and a gap so that a character is one HGR byte; the glyphs that would keep lone (coloured) pixels are redrawn with 2-pixel strokes (`FONT_FIX`).
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
| ESC | leave the table: at once in the lobby; in a game it asks first (ESC again leaves, any other key carries on) and the computer takes over your seat. At the table list, quit |
| ctrl-S | sound off / on |

A joystick works too if one is centred when the game starts.

**Spectating.** If you join a table whose game has already started (or you leave a game), you watch it as a spectator. "SPECTATOR" stays in a corner of every screen. It sits on row 0, except under the big titles of the auctions (row 3) and the summaries (row 1). Every key but the leave key is ignored. The leave key leaves at once, without asking, as there is no seat to lose (`main.c`, `spec_tag` in `text.c`).

## The Apple II version

**The screen: HGR page 1.**
- **Scaling.** Atari colour clocks become colour pixels at 7/8 (160 across becomes 140). A map plot is exactly 4 bytes wide and starts on a byte; scanlines are kept 1:1.
- **Colours.** Players are blue, violet, green and orange, which are HGR's four colours. White text is 40 columns. Coloured text is double width (20 columns), because single-width coloured text doesn't survive NTSC artifacting.
- **Sprites.** They are software sprites with save-under (`src/apple2/gfx.c`, `hires.s`), drawn a colour pixel at a time. The save-unders are rings of rows (a screen line's bytes in row line % rows), so a flush does each line at once, top to bottom: the old row back, the new one saved, the sprite drawn. A sprite is never off the screen, so nothing flickers; at worst the beam shows one frame torn between a sprite's old place and its new one. The same goes for the CoCo and MS-DOS.

**Memory: 128K is used.**
- **Main RAM** holds the core. HGR page 1 sits at $2000-$3FFF. The language card holds a little more code.
- **Overlays.** Each phase's code is an overlay (boot, lobby and summary, grant and production, auctions, development).
- **Aux RAM** holds all five overlays, plus the sprites, the town and the strings.
- **Loading.** Everything is read from `MULE.1`…`MULE.7` at start-up. At each phase change the phase's overlay is copied into a window ($9D00-$B61F) with AUXMOVE. Above it, to $BAFF, are seven sprites' save-unders (`SPRSAVE`).
- **The network's state being built** (550 bytes) lives in aux RAM at $8900 (`AUXSTAGE`), and the code only the boot screens use (the table list, the appkeys) is in the boot overlay: main RAM is full.
- **Memory map.** See `src/apple2/mule-hgr.cfg`.

**Timing.**
- **Counting frames.** The clock is the vertical blank ($C019, or the //c's VBL flag). The drawing routines check it every few milliseconds, so a busy frame still counts every blank.
- **Catching up.** The game logic (walking, the grant cursor, auction lines, the scripts) runs once for every frame that passed. The game keeps the Atari's speeds even when drawing can't keep up.
- **The network: stream mode over N:TCP** (`src/nsnet.c`, `src/nstrans_fn.c`), as on the Adam. An HTTP poll held the machine for the whole request (FujiNet makes the GET inside the first status call). A status call a frame is quick; the socket is read only when bytes wait, with `sp_read` itself (fujinet-lib's `network_read_nb` would ask the status again first: two SmartPort calls where one does). The server clock resynchronises the phase timers on every state.

## Testing

**Offline, in MAME.** `tools/a2sim/a2sim.py` boots the disk in MAME's enhanced //e (headless), drives the game from a scenario and saves screenshots. The input and the test mode go through hooks in the game (`input.c`, `test.c`), found from the linker map.

    python3 tools/a2sim/a2sim.py r2r/apple2/mule.po build/mule-apple2.map scenario.txt out/

- A scenario that starts with `test <phase> [arg [event]]` runs the game on a fake state with no server. It covers walking the town and map, every colony event (`test 4 <event>`), the auctions, the summary and game over.
- **Other commands:** `wait`, `snap`, `hold`, `press`, `key`, `stick` and `pad` (the real controller 1 through MAME's ioport, Adam only), `peek`, `poke`, `pc`, `dump`, and `trace` (MAME's debugger trace).
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
- **Appkeys:** fujinet-lib's CoCo `fuji_write_appkey` always sends 64 bytes, whatever the count. `net.c` zeroes the buffer before each write, so clearing the table (on leaving) stores zeros, not the old URL.

**Running.** Mount `mule.dsk` on the FujiNet and boot. HDB-DOS runs `AUTOEXEC.BAS`, which does `PCLEAR1:LOADM"MULE":EXEC`. The `PCLEAR1` moves the BASIC program out of the way of `MULE.BIN`.

**The screen: PMODE 4 at $0E00.**
- **Colours:** two neighbouring pixels are one artifact colour pixel (128 across). The players are blue, orange, white, and blue/orange stripes (blue on even lines, orange on odd).
- **Colour phase:** a CoCo 1/2 comes up with blue and orange either way round, so the game asks at start-up ("IS THE BAR BLUE?"), and CLEAR swaps them at any time.
- **Scaling:** Atari colour clocks x become colour pixels x - x/4 + 2 (160 -> 120).
  - A map plot is 12 colour pixels, 3 bytes on a byte.
  - Text is 40 columns of 6-pixel cells; the Atari font is squeezed from 7 pixels to 5.
  - Coloured text is double width, as on the Apple.
  - Characters' sprites keep their 8 colour pixels; the ship, cloud and plot cursor are squeezed like the map.
- **Drawing:** sprites are software sprites with save-under (`src/coco/gfx.c`), done a line at a time as on the Apple II. The sprite engine and the inner loops (sprite rows, glyphs, plots) are 6809 assembly: CMOC's code for the engine would not fit.

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
- **Network calls:** stream mode over N:TCP (`src/nsnet.c`, `src/nstrans_fn.c`, in the high core), with a status call every other frame (`NT_POLL_EVERY=2`): the DriveWire ROM masks interrupts while it talks to the FujiNet, and the frames lost are put back from the server's clock. The HELLO asks for big-endian states.
  - **One round trip a call** (`dw_call`): the FujiNet answers DriveWire commands in order and keeps a reply until it is asked for, so the command and the fetch of its reply (or of its error, for a write) go out in one go and one read takes the answer. fujinet-lib asks "ready?" before each part: three round trips for a status, five for a read (which asked the status again), two for a write.

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
| BREAK | leave the table: at once in the lobby; in a game after asking (BREAK again). At the table list, quit (a cold start) |
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

**Running.** Boot DOS with FUJINET.SYS (for example `DEVICE=FUJINET.SYS FUJI_PORT=2 FUJI_BPS=9600` on a PCjr, `FUJI_PORT=1 FUJI_BPS=115200` elsewhere; FUJINET.SYS from fujinet-msdos of October 2026 or later, see below), mount `mule.img` and run `MULE`. ESC at the table list mounts the lobby (`ec.tnfs.io`, `msdos/lobby.img`) in device slot 0 and runs its `AUTOEXEC.BAT` when the game exits: the game types it into the keyboard buffer rather than spawning COMMAND.COM, so all its memory is free for the lobby.

**The screen.** 320 x 200 is exactly two pixels a colour clock, so the Atari art is drawn unscaled: 40 columns of 8-pixel text, the 192 lines 4 down.
- **CGA:** mode 4, palette 1 (cyan, magenta, white). The players are cyan, magenta, white, and cyan/magenta stripes. Colour 0 is the background register: grey behind the map, as on the Atari, black elsewhere. On EGA, VGA or a PCjr in this mode the BIOS also sets the palette registers, and cyan and magenta become light blue and light magenta. A real CGA ignores that.
- **PCjr:** mode 9, 16 colours, each Atari colour its nearest RGBI: light blue, light magenta, green and orange (light red) players, a grey map with black mountains, the town in its own colours.
- **The PCjr's screen memory:** in 16 colours the game draws straight into the 32K of RAM the screen is shown from (the CPU page, from INT 10h AX=0580h). The B800 window is only 16K wide on a real PCjr. MAME maps all 32K there, so it doesn't show the difference.
- **Drawing:** sprites are software sprites with save-under (`src/msdos/gfx.c`, the CoCo's slot logic). Plots and sprite rows are drawn a word at a time through nibble-to-mask tables. The 8088's shifts by a count are too slow for anything else.

**Memory.** Open Watcom's medium model: far code, near data. Each phase is an overlay (the Apple's groups), loaded from `MULE.EXE` by Watcom's dynamic overlay manager when it's first called (`tools/dos/wlink-mule.py`). The game is about 54K resident, with a 1K stack (about 300 bytes are used).
- **The tight case is a 128K PCjr in 16 colours.** Its screen is the last 32K of the 128K (from 18000h). The BIOS keeps only the top 16K from DOS, so the game claims the rest itself, and everything must end below 18000h. With PC-DOS 2.1 and FUJINET.SYS the game's block now ends about 0.7K below 18000h (the stream code took 1.3K, the serial port's own driver 1.3K more; `net.c`'s appkeys go with the boot overlay and `lobby.c` is an overlay of its own to make room for it), and what is left goes to the overlay manager on top of its own area. If there isn't room (a bigger DOS, another resident program), the PCjr uses the 4-colour mode instead.
- **Elsewhere** the overlay manager gets 16K more, so every overlay stays loaded after its first use.

**Timing.**
- **The clock:** the PIT runs at 60 Hz, and the timer interrupt counts the frames. Every 3.3rd tick goes on to the BIOS, so the time of day and the floppy motor still work.
- **The keyboard:** INT 9 is hooked to know which keys are held. On a PC the scan code comes from port 60h; on a PCjr the BIOS passes it in AL. The BIOS still queues typed characters.
- **Network calls are never waited on** (`src/nstrans_fb.c` over `src/msdos/fbport.c` and `uartisr.asm`). FUJINET.SYS (INT F5h) polls the UART inside each call, so the game used to stop for every round trip: 25 ms a status at 9600 bps, a 128-byte read 175 ms, more with the network's delay. The stream's FujiBus packets (FUJINET.SYS's format) now go over the same serial port through its interrupt: a request is put in a ring that the transmitter's interrupt empties, the reply is gathered by the receiver's, and the game looks at it on the next frame. As on the Adam: a WRITE of what nsnet flushed goes first, else a STATUS, and a READ only when bytes wait.
  - **The port** is the one FUJINET.SYS left set up (8N1, DTR, RTS and OUT2 on, its interrupt off), among the BIOS's COM ports and the usual four, with the usual IRQ (4 for 3F8h/3E8h, 3 for 2F8h/2E8h); a 16550's FIFO is turned on. The first one to answer a STATUS is kept. If none does, the same calls go through FUJINET.SYS, each one waited on, the socket asked every 6th frame.
  - **FUJINET.SYS still owns the port** for the rest (opening the socket, appkeys, reading `MULE.EXE`'s overlays from a FujiNet drive): the UART's interrupt is on only while one of our calls is going, and the game's INT 21h hook lets that call finish before DOS goes on.
  - HELLO tells the server the link is slow (`PLAT_NS_SLOW`): it then pushes at most every 300 ms, and the game animates the auction between pushes. The server's clock resynchronises the game.
  - Measured on MAME's PCjr at 9600 bps with 50 ms of network delay: no call held the game (the longest used to be 11 frames), and frames the game was late for went from 59% to 42% (the rest is drawing 16 colours on a 4.77 MHz 8088).
- **A reply cut short:** on the PCjr the keyboard is decoded by the NMI handler, which holds the machine about 9 ms a key, and the 8250 has no FIFO, so the bytes of a reply that come meanwhile are lost. FUJINET.SYS used to wait 15 s for each byte and never retry, so every such key froze the game that long. Since October 2026 it gives a reply up 110 ms after its last byte and asks the FujiNet for it again (FUJICOM command 05h, `FUJICOM-Protocol.md` in fujinet-msdos; fujinet-pc-rs232 answers with its last reply as it went), up to twice. A write that failed even so is sent again with the same seq (the server answers a repeat rather than act twice); a read that failed counts its bytes as taken and asks for the whole state; a read of MULE.EXE (an overlay) that failed is retried by the game's INT 24h handler rather than DOS's prompt. A FujiNet that never answers still costs the driver's 15 s (`plat_net_guard` in `src/msdos/plat.c` can run the BIOS tick faster during a call to shorten that, `NET_TICK_SPEED`, but that shortens the 110 ms the same way, and it is off). `nt_worst` holds the longest call through FUJINET.SYS in frames, for `peek`.
  - The game's own calls do the same: a reply that is garbled, cut short or lost asks for it again with 05h, up to twice (`FB_RESENDS`; a FujiNet without RESEND answers it with a NAK at once). Holding keys on MAME's PCjr took 15 RESENDs in a minute, and the game went on. `fb_lsr` keeps the UART's line errors and `fb_base` the port found, for `peek`.
- **Without FUJINET.SYS**, INT F5h may point anywhere (on the PCjr it's left over from BASIC). FUJINET.SYS is recognised by its device header (attributes 6002h) in the vector's segment. If it isn't there, the game answers INT F5h itself and says NO FUJINET FOUND.

**Testing.** `tools/dossim/dossim.py` is `cocosim.py` for MAME's `ibmpcjr` (128K) or `ibm5150` (CGA). It boots a copy of a DOS disk (`DOSSIM_BOOT`, by default PC-DOS 2.1 with FUJINET.SYS) with `MULE.EXE` on it, and uses the same scenario language and test hooks:

    python3 tools/dossim/dossim.py r2r/msdos/mule.exe build/mule-msdos.map scenario.txt out/
    DOSSIM_MACHINE=ibm5150 DOSSIM_NOFN=1 python3 tools/dossim/dossim.py ...

- The game's data segment is found at run time, from DS once the title has written `3C C3`.
- `peek plat_late 2` counts the frames the game missed.
- `stack` says how much of the stack (filled with A5h at start-up) was never used.
- `DOSSIM_EARLYPC=frame` samples CS:IP before the game is found.
- `DOSWAV=file.wav` records the sound.
- **Against a server:** start fujinet-pc (RS232 build) with BoIP on a free port, then `DOSSIM_FUJINET=127.0.0.1:port`. MAME's serial port becomes a null modem to it, and the run is in real time. `make msdos AUTOPLAY=1` plays by itself. `DOSSIM_AUTOEXEC='C:\nMULE'` runs the game from the FujiNet's drive instead (mount `mule.img` in slot 1), so its overlays are read through FUJINET.SYS.

**Controls.**

| Key | Does |
|---|---|
| arrows, keypad (7 9 1 3 diagonals), or joystick A | walk, push, choose |
| SPACE (or a joystick button) | the button |
| ENTER | end your development turn |
| ESC | leave the table: at once in the lobby; in a game after asking (ESC again). At the table list, back to the FujiNet lobby |
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
- **x is scaled 3/2 from the Atari's colour clocks, from 4 pixels in.** A plot's 16 clocks become exactly three cells, so plots are made whole, cell by cell. Lines are the Atari's own.
- **Text** is the Namco arcade font the other Adam FujiNet games use (`tools/tms/namco.bin`, from fujinet-5cardstud), a whole cell a character: 32 columns. Big text is the same glyphs (the colour sets it apart). A character is written straight into its cell, pattern and colour, with no reading back. The shared screens lay themselves out by `TCOLS`, `BIGW` and `TX()` (`src/platform-specific/gfx.h`): `TX(c)` is the cell over the same spot as the 40-column cell `c`, for text on the art (the town's prices, the auction's). A centred line too wide for 32 columns goes on to the next row.
- **Drawing** reads a cell's 8 pattern and 8 colour bytes, paints them and writes them back. A colour a row doesn't have replaces the one with fewer pixels left, and those pixels change colour: that is the colour clash. Where the cells are kept is `vram_tms.h`'s business (the SMS's: `src/sms/vram_sms.h`).
- **Plots** are worked out whole in `plotcell.asm` (the owner's layer, white or black, the river), under a frame each on the Adam: the whole map in about two thirds of a second. In C it took three frames a plot, and leaving the town froze a turn for over two seconds. The auction's dotted lines touch only their own line of each cell, and their dots are made once.
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
- **Sound** (`sound.c`) is timed by the NMI too (`snd_frame`), so an effect ends on time however long the game is held up (an overlay loading, an AdamNet call).
- **The network: stream mode over N:TCP** (`src/nsnet.c`, `src/nstrans_fn.c`, shared with the CoCo, the Apple II and MS-DOS; `docs/PROTOCOL.md`). An HTTP poll held the Z80 for the whole request, because FujiNet makes the GET inside its AdamNet handler, and the game froze with the clock running. The Adam instead keeps one socket open to `N:TCP://<endpoint host>:9902/`.
  - The socket's status is asked at most once a frame, and it is read only when bytes wait. A read with nothing waiting gets no answer, which costs fujinet-lib 20 timeouts.
  - A read brings everything waiting. The server's 192-byte window keeps that under the 256-byte buffer.
  - EOS puts the length received back in the DCB, but ADAMEm doesn't. A read that seems to fill the buffer takes the status's count instead.
  - Each frame going up is one write. A dropped connection (5 s of silence, or the status says it closed) is opened again at the next poll.
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

- **The network: stream mode, never waited on** (`src/nstrans_fb.c`, `src/fbport.h`). fujinet-lib's `fuji_bus_call` writes a FujiBus packet to the cartridge and then spins until the whole reply is in, and an HTTP poll put the GET inside that wait: half the Z80's time went to the bus, in stalls of up to 4 s. Now the game writes a request (a few register writes) and picks up the reply bytes on the frames after, as the Adam does with its DCB: a WRITE of what nsnet flushed, else a STATUS, and a READ only when bytes wait. The open, the close and the appkeys stay fujinet-lib's (`ns_pause` lets the call going finish first). With 50 ms of network delay the bus code takes 8% of the Z80.

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

**The network: stream mode** (`src/atari/nsnet.c`, `nstream.s`; see `docs/PROTOCOL.md`). The Atari doesn't poll over HTTP as the other versions do: SIO made each poll cost about 0.45 s of blocking. It uses FujiNet netstream instead, which turns the SIO line into a 19200-baud serial link to one TCP connection. The server's stream port is 9902, on the endpoint's host.
- **The server pushes the state** whenever it changes, as patches to the copy the client already has. Actions go up as they happen. Only a clock change costs about 14 bytes every half second.
- **Receiving:** the serial input IRQ fills a 256-byte ring. The main loop takes whole frames out of it once a frame (COBS, Fletcher-16, in assembly). The server never has more than 192 bytes the client hasn't taken, so the ring can't overflow while an overlay loads or a screen draws.
- **Plain SIO** (the overlays, the appkeys) drops the motor line. FujiNet then answers SIO normally and holds the socket's data until the motor comes back.
- **The immediate VBI starts with `CLI`**, so the serial IRQ is not held off while the VBI and the sound run.
- **Quitting** sends a goodbye, then turns FujiNet's netstream off (`$F0` with host `STOP`).
- **Only the latest state is drawn.** Of the states waiting in the ring, the last is kept (the one answering an action goes at once): drawing each in turn put the game ever further behind.
- **Every version talks stream mode.** `PLAT_NETSTREAM` compiles out `net.c`'s HTTP half and keeps its appkeys.

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

**Sound** (`sound.c`, `snd.s`). Short blips on POKEY voice 1, no music, from the table every platform shares (`src/platform-specific/sound.h`). The immediate VBI times them, so a tone ends on time however busy the game is. Voices 3 and 4 are left alone: SIO and the network stream clock the serial port with them.
- **The alert** (`SND_ALERT`) is the only two-note sound and the loudest. It rings whenever the game wants your hand on the stick.
- **The time bar's tick** (`SND_TICK`/`SND_TOCK`, `map.c` `bar_tick`) is the original's. Once fewer than 30 units are left (PTU on a turn, ticks in an auction), it ticks every that-many frames, alternating AUDF `$0A` and `$0F`, so it hurries as the bar empties. It is soft and never cuts another sound. Every MekkoGX platform plays it.

**Keys:** the stick or the arrow keys (with CONTROL) walk, and the trigger or SPACE is the button. RETURN ends a turn, ESC leaves, 0 sits out, and CONTROL-S turns the sound off and on.

**Testing** (`tools/atarisim/`). `asim` is a scripted libatari800: atari800's source configured with `--target=libatari800` and built (`~/Workspace/atari800-lib`, a copy of the atari800 tree, so the emulator build stays as it is):

    make -C tools/atarisim A800=~/Workspace/atari800-lib
    python3 tools/atarisim/atarisim.py r2r/atari/mule.atr build/mule-atari.map scenario.txt out/

- **Scenarios** use `tools/a2sim`'s language, plus `wav` and `pokey`, which record the sound and the POKEY writes. `asim` itself also logs GTIA and ANTIC writes, dumps registers with each shot, and can boot from FujiNet.
- **Against a server:** run fujinet-pc (Atari build) with `[BOIP] port=` set to asim's NetSIO port, CONFIG off, and `mule.atr` mounted. Then:

      asim -netsio 9995 - script.txt out/

  In that script, `coldstart` once FujiNet is there. With NetSIO on, asim keeps to 60 frames a second and turns the SIO patch off. atari800's NetSIO carries netstream too (the motor line and the POKEY serial port). Start the server with `MULE_STREAM_DEBUG=1` to log every frame. Convert shots with `topng.py out/NAME.scr`: it takes `.scr` files only.

## The Lynx version

A cart image for the Atari Lynx with a FujiNet-Lynx on its ComLynx port. It keeps the Atari version's network and sound, and draws the other versions' pictures with Suzy, on a screen half as tall.

**Building.**

    make lynx                                  # r2r/lynx/mule.lnx (a flash cart or an emulator)
    make lynx ENDPOINT=http://myhost:8080      # the server: its host, stream port 9902

- **What it needs:** cc65 2.19, Python 3 with Pillow, and fujinet-lib-experimental in `~/Workspace/fujinet-lib-experimental` (or `make FNLIB_EXP=/path`). `tools/lynx/mkfnlib.sh` compiles it into `build/fnlib-lynx`, with the bus packet buffer at 256 bytes instead of 1K.
- **fujinet-lib-experimental needs** `fuji_enable_netstream()` and the Lynx appkey read and write (`bus/lynx/fuji_enable_netstream.c`, `fuji_bus_appkey_read.c`, `fuji_bus_appkey_write.c`), and `LYNX_PACKET_MAX` in `lynx_bus_call.c`.
- **Graphics** come from the disk as for the others (`tools/mkgfx_lynx.py`, from `mkgfx_dos.py`'s and `mkgfx_hgr.py`'s extraction), into `gen/lynx/`. The sound is `tools/mksnd_atari.py`'s, into `gen/lynx/snd.s`.

**The network: stream mode, one way.** `src/nsnet.c` (shared with the Adam) runs the Atari's protocol over `src/lynx/nstrans.c`: cc65's ComLynx driver (62500 baud, 8 bits, odd parity), which the library opens for its first bus call.
- **Switching on:** `fuji_enable_netstream(9902, host, NETSTREAM_TCP)`. From then on every ComLynx byte goes to the server's socket, and every byte from it comes back.
- **No way back.** The Lynx firmware can't leave stream mode, so nothing else may go over the bus once it is on (`PLAT_NS_ONEWAY` in `net.c`).
  - The player's name (appkey 0) is read, and written after the picker, before the stream starts.
  - The table (appkey 10) is neither read nor kept, so the table list comes up at every start.
  - Quitting only says goodbye: the Lynx can then be switched off.
- **The receive ring** is the driver's 256 bytes, filled by its interrupt. The server's 192-byte window keeps it from overflowing.
- **HELLO and the table list** are asked for again every half second until an answer comes, because FujiNet acknowledges the command before its socket is open.
- **A bad frame** asks for the whole state (RESYNC), at most once every 30 frames. ComLynx is one wire, and our own bytes or a collision must not start a storm.
- **While we send** the driver doesn't receive, and FujiNet waits for the line to go quiet before it sends. Frames going up are short.

**Memory** (`src/lynx/mule-lynx.cfg`):

| Range | Use |
|---|---|
| $0200 | the program: code, data, then its variables |
| $A000-$BFDF | the screen shown (160 x 102, 4 bits a pixel) |
| $C000-$DFDF | the background (what `gfx_*` draw) |
| $E000-$F89F | the overlay window: one phase at a time |
| $F8A0-$FBFF | the C stack |

- **The cart** is the program, then the five phases as entries 1-5 of the cart's directory (`src/lynx/directory.s`). `bank.c` loads a phase into the window with `lynx_load()`.
- **Below $FC00.** Both buffers are kept under $FC00, because the CPU copies between them and can't reach the RAM under Suzy's and Mikey's registers.
- **The pictures** are stored as plain 1-bit rows. Each becomes a Suzy literal sprite in a small buffer as it is drawn, because a literal line's length and pad bytes would triple an 8-pixel-wide shape.

**The screen** (`gfx.c`, `suzy.s`).
- **Across,** a colour clock is a pixel, as on the Atari.
- **Down,** the 192 lines are mapped into 102 (`ly()`):
  - rows 0-1 and 22-23 are 5 lines each;
  - the map and the town (rows 2-21) are at half height: a plot is 16 x 16, and the town is 96 x 72 from line 10;
  - text rows there alternate 5 and 3 lines, with rows 18 and 19 up a line. No screen writes an odd row and the one after it, except the town's corral price (19) with its money (20), which the shift makes room for.
- **Text** is a 3 x 5 font in 4-pixel cells (40 across). Big text is the same, twice as wide. On the grey screens (the map and the town) white text is black, as the Atari prints it.
- **Everything is drawn into the background** by Suzy. What changed is made up again 32 lines at a time in a band buffer (the background copied in, the sprites drawn over it), then copied to the screen, so a sprite needs nothing saved from under it, and each byte of the screen is written once, as it ends up: nothing flickers.
- **ComLynx and Suzy.** While Suzy draws the CPU sleeps and interrupts wait, and ComLynx has one byte of buffering. So fills go in pieces of 8 lines, and the town is unpacked by the CPU.
- **Pens:** the game's colours in order, with pen 14 a second black for sprites (pen 0 may be see-through) and pen 15 the river. The town's colours are the nearest pens.

**Sound** is the Atari's: `src/atari/pokey.s` (assembled again by `src/lynx/pokey.s`) and `src/atari/sound.c`, run in the vertical blank.
- **`mikey.s`** turns the four POKEY voices into Mikey's four channels:
  - **pitch:** the POKEY clock (64 kHz, 15 kHz, 1.79 MHz, joined pairs) becomes a Mikey clock and count;
  - **distortion:** pure tones are a square wave, the 4-bit polys a 15-step LFSR, the others a 4095-step one;
  - **volume:** AUDC's volume times 8.
- **All four voices play online:** ComLynx has its own clock.
- **The VBL** (`irq.s`) counts the 60 Hz clock and runs the sound. Its interruptor has priority 1, so cc65's `clock()` counter (priority 2) sees the VBL first: fujinet-lib's timeouts depend on it. It acknowledges the VBL and re-enables interrupts, so ComLynx isn't held off while the music plays.
- **No TGI:** its VBL handler would fight ours.

**Controls:**

| Button | Does |
|---|---|
| pad | walk; up/down in auctions; left/right in the lobby |
| A | the button |
| B | rub out (the name) |
| OPTION 1 | RETURN: end your turn, finish the name |
| OPTION 2 | ESC: leave the table (asks first in a game); at the table list, quit |
| OPTION 1 + B | 0: sit this good out |
| PAUSE | sound off / on |

The name is picked a letter at a time (`PLAT_NO_KEYBOARD` in `boot.c`): up and down turn the letter, A or right takes it, B or left rubs out, and OPTION 1 ends.

**Testing.**
- **Offline, in MAME** (`tools/lynxsim/lynxsim.py`): `tools/a2sim`'s scenario language, with the cart in MAME's `lynx` (it needs `lynx.zip` in MAME's roms). `LYNXWAV=out.wav` records the sound.

      python3 tools/lynxsim/lynxsim.py r2r/lynx/mule.lnx build/mule-lynx.map scenario.txt out/

  - MAME's ComLynx only echoes back to itself, so only the offline test mode (`test <phase> [arg]`) plays.
  - Without a test step the game waits for FujiNet (`NO FUJINET FOUND` after 10 seconds). Poking `ns_on` to 1 then gets past it, to the name picker and the table list.
  - `key` takes a character: `key 08` is backspace (`key 8` would be the digit).
- **On a Lynx** with a FujiNet-Lynx and a flash cart:
  1. "CONNECTING...", then the name picker the first time (the name is kept).
  2. The table list comes over the stream from the server's port 9902. A local server started with `MULE_STREAM_DEBUG=1` logs every frame.
  3. Join a table and play a round against the computer.

  The build remembers `ENDPOINT` (in `gen/endpoint`): put back `http://fujinet.online:9900/` after testing locally.

## The NES version

An image for the NES on the FujiNet NES cartridge (fujinet-firmware `pico/nes`, the `nes-bringup` branch): the NES CONFIG boots it from TNFS like any other `.nes`. It draws the Adam's pictures, 32 columns, and plays the Adam's effects.

    make nes                                   # r2r/nes/mule.nes
    make nes ENDPOINT=http://myhost:8080       # the server: its host, stream port 9902

- **What it needs:** cc65 2.19, Python 3 with Pillow, and fujinet-lib-experimental's `add-nes` branch in `~/Workspace/fujinet-lib-experimental` (or `make FNLIB_EXP=/path`). `tools/nes/mkfnlib.sh` compiles it into `build/fnlib-nes`.
- **Graphics** are the Adam's (`tools/mkgfx_nes.py`, from `mkgfx_tms.py`): the Namco font, the plot parts, the shapes. Only the town is made again, a 16 x 16 block at a time in the NES's palettes, into `gen/nes/` (previews in `gen/preview/nes/`).
- **The check:** `tools/nes/checkrom.py` runs after the link. It wants the "FUJI" claim at $FFF0 (without it the cartridge turns its mailbox off once the image boots), a reset vector in the fixed banks, and no read-modify-write instruction aimed at the mailbox's write-only pages. A failing image is deleted.

**The cartridge image** (`src/nes/mule-nes.cfg`): MMC3 (mapper 4), 128K PRG, 16K CHR-RAM, 8K WRAM, NES 2.0 header.
- **$C000-$FFFF**, fixed: the program, the runtime and fujinet-lib.
- **$A000-$BFFF**, R7: more of the program (`nsnet.c`, `net.c`, `text.c`, `map.c`, `fx.c`, `test.c`), never switched.
- **$8000-$9FFF**, R6: the phase's overlay (banks 0-4). The art (bank 5) and the strings (bank 6) take turns there too, as on the MSX (`src/nes/bank.c`): the code that switches lives outside the window and puts the phase back. The town and the auction's dotted lines are drawn by their own phases, so their code and the town's tables sit in those overlays.
- **WRAM** holds the data and the variables; the C stack is in the console's RAM at $0500-$07FF.
- Both the fixed banks and R7 are nearly full: under 200 bytes free in each (`build/mule-nes.map`).

**The screen** is the TMS9918A's Graphics II, in the PPU's terms (`src/nes/gfx.c`, `crt0.s`):
- **A bitmap.** All 768 cells have their own tile. Each third of the screen (8 rows) has its own 4K pattern table in CHR-RAM. MMC3's scanline interrupt switches the background's table at lines 79 and 143. The 24 rows are name table rows 2-25. Above them the background is off, and from line 208 the PPU is off altogether.
- **Colour** is a 16 x 16 block's palette (1 of 4), 3 colours and the background each. Each screen has its set: black screens, the map (the plains, white, black, a player's colour), the town (the players' colours, and two building palettes). White is in every palette. Drawing in a colour gives its block that colour's palette, and the last colour drawn in a block wins.
- **The river** on the map is sprites behind the background: the plots' palettes have no room for its blue.
- **Drawing goes through a queue** (`ppuq`). The PPU can only be written while it shows nothing, so the interrupts drain the queue: from line 208 to the vertical blank, then in the NMI after the sprites' DMA, about 280 bytes a frame. `gfx_clear` turns the display off until the next `frame_wait`, so a whole screen is drawn at once and shows complete.
- **Sprites:** 8 x 16, one colour each, from four palettes. A slot is up to six of them, with its own patterns, sent when its frame or colour changes. When a line has a ninth sprite, the order rotates a frame at a time.
- MMC3's bank select register is shared by the game (R6) and the interrupts (R0/R1). The game's choice is kept in `mmc3_sel`, written before the register, and the interrupts put it back.

**The network: stream mode, never waited on.** `src/nes/nstrans.c` writes each STATUS, READ or WRITE straight into the cartridge's mailbox: registers at $5500+n, the request at $5700, a sequence number to launch it. It looks again on the next frames, and copies the reply out of the 1K window at $5000.
- The sequence number is the cartridge's ACKSEQ + 1, never a counter of our own: a console Reset doesn't reset the cartridge.
- Only plain stores touch $5500-$57FF.
- The open, the close and the appkeys are fujinet-lib's: a wait, once. Before an appkey call, `nt_quiesce` lets the call in flight finish.
- `nt_detect` wants the cartridge and an answer from the FujiNet behind it (`fuji_get_wifi_status`).

**Controls** (joypad 1):

| Button | Does |
|---|---|
| pad | walk; up/down in auctions; left/right in the lobby |
| A | the button |
| B | rub out (the name) |
| START | RETURN: end your turn, finish the name |
| SELECT | ESC: leave the table (asks first in a game); at the table list, quit |
| START + B | 0: sit this good out |

The name is picked a letter at a time, as on the Lynx. Quitting says goodbye to the server and asks for the NES to be switched off: Reset doesn't bring CONFIG back, only a power cycle does.

**Testing.**
- **Offline, in MAME** (`tools/nessim/nessim.py`): `tools/a2sim`'s scenario language, with the image on the FujiNet cartridge's MAME device (fujinet-firmware `pico/nes/emu`, grafted into `~/Workspace/mame` by its `apply.sh`). The cartridge puts the image straight into its SRAMs. `NESWAV=out.wav` records the sound. The cartridge is pointed at a port nothing listens on, so a fujinet-pc isn't needed, or disturbed.

      python3 tools/nessim/nessim.py r2r/nes/mule.nes build/mule-nes.map scenario.txt out/

- **Live, in MAME:** the same device, with a fujinet-pc (`fujinet-pc-rs232`, BoIP on 127.0.0.1:9995, or `FUJINET_TCP`) and a server. Run it throttled: under `-nothrottle` the server's countdowns look frozen. To keep clear of other servers on :9902, run the server, a copy of fujinet-pc and MAME together in `unshare -rnm`, with a hosts file mapping fujinet.online to 127.0.0.1. Played that way: lobby, transport, summary, land grant, land auction, with no failed calls.
- **The scanline interrupt's first line:** MAME's cartridge device clocks MMC3's counter from line 0. The hardware also clocks it on the pre-render line, one line earlier. `IRQ_EARLY` in `crt0.s` is 0 for MAME. On a real cartridge it may need to be 1: a one-line seam at the table switches would show it.

## The Atari 7800 version

An image for the Atari 7800 on the FujiNet 7800 cartridge (fujinet-firmware `pico/atari-7800`): the 7800 CONFIG pushes it into the cartridge's SRAM and starts it like any game. It draws what the Atari 800 version draws, the original's own characters, colours and sprites, at the original's 160 colour clocks, and plays the Atari's effects on the cartridge's POKEY.

    make atari7800                             # r2r/atari7800/mule.a78
    make atari7800 ENDPOINT=http://myhost:8080 # the server: its host, stream port 9902

- **What it needs:** cc65 2.19, Python 3 with Pillow, the original disk (as for the Atari), and fujinet-lib-experimental's `add-atari7800` branch in `~/Workspace/fnlib-7800` (or `make FNLIB_7800=/path`; built from 1ebf9d0). `tools/atari7800/mkfnlib.sh` compiles it into `build/fnlib-a78`, without its crt0 (`src/atari7800/crt0.s` is the game's).
- **Graphics** are the Atari's (`tools/mkgfx_atari.py`, into `gen/atari7800/`); `src/atari7800/gfx.c` turns them into MARIA's layout at start-up.
- **The check:** `tools/atari7800/mkrom.py` runs after the link. It copies REPL into every bank, then scans every code segment in its own bank for a read-modify-write aimed at the mailbox, a store to the TIA (it would rewrite INPTCTRL, which the cartridge keeps unlocked), a `JMP ($xxFF)`, and a store into the bank window anywhere but `bank.c`. It wants the claim (kind 5) and a reset vector in the fixed bank, then writes `mule.a78`. Constants sit in segments of their own (`REPLRO`, `OVLnRO`) so only code is scanned.

**The cartridge image** (`src/atari7800/mule-a78.cfg`): SuperGame with the cartridge's RAM (the claim's kind 5), 128K in eight 16K banks. The window at $8000-$BFFF takes banks 0-6; bank 7 sits at $C000.
- **$8000-$9FFF in every bank** is REPL, the same 8K in all seven (`text.c`, `fx.c`, `map.c`, `nsnet.c`, `test.c`, `autoplay.c`, the sprites): as good as fixed.
- **$A000-$BFFF** is the bank's own: a phase's overlay (banks 0-4), the art (5), the strings (6). `bank.c` switches with a store to $8000 and puts the phase back.
- **$C000-$FF6F**, fixed: the rest of the program, the display, the runtime and fujinet-lib; the claim at $FF70.
- **RAM:** the console's holds the display lists (MARIA reads lists in a 2-cycle window); the cartridge's 16K at $4000 holds four 2K character blocks, the screen, the variables and the C stack. All three are nearly full (`build/mule-a78.map`): the fixed bank has about 350 bytes free, REPL about 300, the variables about 120.

**The screen** (`src/atari7800/maria.s`, `nmi.s`, `gfx.c`) is the Atari port's 24 rows of 8 lines, each a MARIA zone with a display list of its own, two slots of it (built in one, shown from the next frame, all together).
- **Rows** keep the Atari's screen bytes (`draw.s`, `cells.s`, `mapgfx.s` are the Atari's with MARIA's character layout). A text row is 320A, the original's font a byte a character; the big text, drawing, map and town rows are 160A. The map's mode 6 characters are made 2-byte codes in three colour values (a row's four colours in two palettes); the drawing cells and the town's characters are 1-byte codes.
- **What the original drew with players** is direct objects in the zones: the mountains, the river, the store, the time bar.
- **Sprites** are 8-line pieces (made again only when the frame or the line within the zone changes) in direct objects, pixel value 3, in P4-P7: four sprite colours a frame.
- **A DLI** where a zone differs from the one above sets CHARBASE, CTRL and the zone's colours (even zones use P0/P1, odd ones P2/P3, so colours are set while the zone above shows). A 320A row under a 160A one is switched two lines on, at its first line's start. The frame's DLI on the blank below keeps the clock (60 jiffies a second, NTSC or PAL), plays the sound and shows what was built. `tools/a78sim/probe` measured the protocol and its DMA limits in MAME.

**The network: stream mode, never waited on.** `src/atari7800/nstrans.c` is the NES's, at the 7800 cartridge's mailbox: registers at $0D00+n, the request at $0F00, the reply in the 1K window at $0800, ACKSEQ at $0C00.

**Controls** (the left joystick, the console's switches):

| Control | Does |
|---|---|
| stick | walk; up/down in auctions; left/right in the lobby |
| button 1 (FIRE) | the button |
| button 2 | rub out (the name); RESET does the same, for a one-button 2600 stick |
| PAUSE | RETURN: end your turn, finish the name |
| SELECT | ESC: leave the table (asks first in a game); at the table list, quit |
| PAUSE + button 2 | 0: sit this good out |

The name is picked a letter at a time. Quitting says goodbye to the server and goes back to CONFIG.

**Testing.**
- **Offline, in MAME** (`tools/a78sim/a78sim.py`): `tools/a2sim`'s scenario language, with the image on the FujiNet 7800 cartridge's MAME device (`~/Workspace/mame-a7800`, grafted by fujinet-firmware's `pico/atari-7800/emu/apply.sh`); `pad` drives MAME's own stick and switches, `zones` prints each zone's DMA, `wcount` counts the main loop's passes. `A78SIM_SYSTEM=a7800p` is a PAL console, `A78WAV=out.wav` records the sound. The cartridge is pointed at a port nothing listens on.

      python3 tools/a78sim/a78sim.py r2r/atari7800/mule.a78 build/mule-a78.map scenario.txt out/

- **Live** (`tools/a78sim/rig.sh SCENARIO OUT [SECS]`): a server built from this repository (HTTP 8087, stream 19987), a private copy of fujinet-pc on BoIP 19988 with the appkeys seeded, and MAME in real time, the image built from a copy with that endpoint. `AUTOPLAY=1` builds the one that plays itself.

## The SMS version

An image for the Sega Master System on the FujiNet SMS cartridge (fujinet-firmware `pico/sms`, the `add-sms` branch): CONFIG pushes it into the cartridge's SRAM and starts it like any game. It draws the Adam's pictures (`src/tms9918`, `gen/tms9918/gfx.c`) with the Namco font at 32 columns, and plays the Adam's effects on the SMS's own SN76489.

    make sms                                   # r2r/sms/mule.sms
    make sms ENDPOINT=http://myhost:8080       # the server: its host, stream port 9902

- **What it needs:** z88dk, as for the Adam (`tools/z80/z88dk`), and fujinet-lib-experimental's `add-sms` branch: the worktree `~/Workspace/fnlib-sms` (or `make FNLIB_SMS=/path`). `tools/z80/mkfnlib.sh` builds it with zsdcc into `build/fnlib-sms`.
- **The image** (`src/sms/mule.asm`, `tools/sms/mkrom.py`): 144K, the Sega mapper.

  | Range | Use |
  |---|---|
  | 0000h-7FD7h | banks 0-1, fixed in slots 0 and 1: the program |
  | 7FDCh, 7FF0h | the "FUJI" claim (the cartridge keeps its mailbox on for a claimed image) and the SEGA header |
  | 8000h-AFFFh | slot 2: the overlays (banks 2-6), the art (7) and the strings (8) take turns there, as on the MSX |
  | B000h-BFFFh | the cartridge's mailbox, over slot 2 whatever its bank |
  | C000h-DFEFh | RAM: the variables (about 6.3K), then the stack |

  `mkrom.py` checks the sizes, stamps the claim and the header's checksum, and makes the cartridge's own mapper guess (it has no CRC for this image). The bank switches in `mule.asm` are each an `ld (FFFFh),a`, which says Sega; a stray `ld (3FFEh),a` would say 4 PAK. A failing image is deleted. The cartridge's RAM isn't used.

**The screen: the TMS9918A's cells in Mode 4** (`src/sms/tiles.c`, `vram_sms.h`, `tileconv.asm`). The SMS can show Graphics II, but only in a fixed palette that turns the map's grey dark. Mode 4 has room for about 450 tiles of 16 colours, and no screen has more than about 120 different cells. So the painter (`src/tms9918/gfx.c`) works on a cell's patterns and colours as on the Adam, and the SMS keeps each different cell as one tile, shared by every cell that looks the same.
- **Storing a cell:** its rows' two colours are spread over the four bit planes, and the tile is hashed. Tiles with the same hash are compared with VRAM. A tile no cell shows any more stays, for the next cell like it. The cell stored last is remembered, so a fill or the plains cost next to nothing.
- **Reading a cell back** decodes its tile into a pattern and two colours (the left pixel's is the background).
- **255 tiles for the cells, 160 for the sprites.** If the cells' run out, a cell keeps what it had and `tile_full` counts it; `tile_most` is the most on screen yet.
- **Colours:** the TMS9918A's 15, as near as the SMS's 64 have them, in both palettes.
- **Sprites:** a TMS sprite (16 x 16, one colour) is two of the SMS's 8 x 16 sprites, the colour drawn into their tiles, sent when the frame or the colour changes. Eight fit on a line; with a ninth, the order rotates. A half left of the screen isn't shown.

**The clock and the sound.** z88dk's crt0 takes the frame interrupt and reads the VDP's status; `smsint.asm` counts the frame, keeps the status bits and times the sound effect. A 50 Hz console (found by timing a frame) counts every fifth frame twice, so the clock is in 60ths there too.

**The network: stream mode, never waited on.** `src/sms/nstrans.c` is the NES's, on the same mailbox at B000h: registers at B500h+n, the request at B700h, the 1K reply at B000h. The open, the close and the appkeys are fujinet-lib's.

**Controls** (pad 1, and the console's Pause):

| Button | Does |
|---|---|
| pad | walk; up/down in auctions; left/right in the lobby |
| 1 | the button |
| 2 | rub out (the name), when let go |
| Pause | end your turn, finish the name |
| 2 held + Pause | leave the table (asks first in a game: 2 + Pause again leaves); at the table list, quit |
| 1 held + Pause | sit this good out |
| Reset (an SMS1's) | leave |

Pause is the Z80's NMI and can't be held, so the button held when it comes says what it means. Button 2 counts when it is let go without a Pause meanwhile, so holding it for 2 + Pause does nothing else (pressed, it would end the "leave?" question before the Pause answered it). Button 1 counts when pressed, as it must for the land grant, and declare ignores it. The name is picked a letter at a time. Quitting says goodbye to the server and goes back to the cartridge's CONFIG.

**Testing.**
- **Offline, in MAME** (`tools/smssim/smssim.py`): the scenario language, with the image on the FujiNet SMS cartridge's MAME device (fujinet-firmware `pico/sms/emu`, grafted by its `apply.sh`), started without a BIOS. `SMSSIM_SYSTEM` picks the console: `sms` is the SMS2, `smspal` and `sms1pal` run at 50 Hz.

      python3 tools/smssim/smssim.py r2r/sms/mule.sms r2r/sms/mule.map scenario.txt out/

- **Live, in MAME:** as for the NES, the same device with a copy of fujinet-pc-rs232 and a server, together in `unshare -rnm`. An `AUTOPLAY=1` build played 25 minutes that way, to round 9 (land grants, auctions, development, production, the goods' auctions), with no failed or dropped call and at most 137 tiles in use. Leaving the table and quitting back to CONFIG were played with the pad and Pause.

## The H89 version

`MULE.COM` for a Heathkit H89 (or Zenith Z-89/Z-90) running Heath/Zenith CP/M 2.2.04, with the H89-ESP32 FujiNet card (fujinet-firmware's `rs232` build for the H89: an 82C55 at D0h speaking FujiBus). The screen is the H89's own H19 terminal: 80 columns, 24 lines and the 25th, its graphics characters, reverse video, no colour, one beep.

    make h89-cpm                               # r2r/h89-cpm/mule.h8d (MULE.COM on an H17 disk) and mule.com
    make h89-cpm ENDPOINT=http://myhost:8080   # the server: its host, stream port 9902

- **What it needs:** z88dk, as for the Adam (`tools/z80/z88dk`), and fujinet-lib with `bus/h89` in `~/Workspace/fujinet-lib` (or `make FNLIB_H89=/path`), which `tools/z80/mkfnlib.sh` builds with zsdcc into `build/fnlib-h89`.
- **The disks** are 100K H17 images (the layout Heath's CP/M gives a hard-sectored disk):
  - `mule-boot.h8d` (`make h89-boot`) boots: a 64K CP/M 2.2.04 system disk with `MULE.COM` on it (and PIP, STAT, SYSGEN, FORMAT and DUP; the development tools are taken off for room). Boot it from drive 1 and type `MULE`. `tools/h89/mkboot.sh` makes it from fujinet-go-h89-desktop's system disk, letting CP/M itself ERA and PIP in that project's headless H89.
  - `mule.h8d` is a data disk with `MULE.COM` alone (`appmake +cpmdisk -f hz89`): from a CP/M already booted, `B:MULE`, or through FNDISK (fujinet-h89) `D:MULE`.
- **Memory:** a 64K system, or the 62K one FNDISK needs (about 360 bytes to spare there). MULE.COM loads at 0100h; the screen's buffer (2,000 bytes) and the stack (384 bytes; 192 measured) go above the program's variables, over the CCP (it ends with a warm boot). The .COM is cut short at its variables (`tools/h89/trimcom.py`): 46K to load, about 20 seconds from an H17.

**The screen** (`src/h89/h19.c`, `gfx.c`). The H19 is a serial terminal at 9600 baud: a byte a millisecond, a whole screen in two seconds. So the game draws into a buffer of cells, and only the cells that changed go out, in the fewest bytes:
- graphics mode stays on (ESC F once): its characters 20h-5Dh are still text, upper case, so the only switch is reverse video (ESC p, ESC q);
- the cursor moves the cheapest way: a few cells across by sending them again, back by BS, down by LF, else ESC Y; the rest of a line gone blank is ESC K; a new screen with much to wipe is ESC E;
- a flush puts at most 56 bytes in the console's ring, so what is queued is on the screen within a few frames, and the rows the sprites are on go first: your figure answers the keys even while a screen is still being drawn behind it.
- A cell has a dirty bit, not a copy of what the terminal shows: 250 bytes instead of 2,000, which the 62K system doesn't have. The cell writes are assembly (`h19cell.asm`); a flush with nothing to send returns at once.
- A step in town costs about 10 bytes; walking out of town onto the map, a few dozen (the title) and a seventh of a second.

| Screen | What it looks like |
|---|---|
| map | plots of 8 x 3 cells; mountains ╱╲, the river ▒▒, an owned plot boxed with its owner's seat number and its M.U.L.E.'s good (F, E, S, C) in the top edge, production dots ● along the bottom edge; your own plots' boxes in reverse; a plot for sale blinks boxed |
| development | the map and the town together (map rows 1-15, the town 16-22): walking into town or out of it moves only your figure. The money is at the top right, the time bar down the right edge |
| town | the four outfitters along the top, the assay office, land office, pub and corral (with the M.U.L.E. price and count) along the bottom, their doors where the Atari's are |
| players | seat numbers 1-4, reversed, wherever they stand; the 25th line says who is who: `1 NAME  2 NAME ...` |
| auctions | the price lines ─, the players' numbers above them, the money and goods under each; the goods' bars in block characters |
| titles | in reverse |

**The console and the clock** (`src/h89/h89io.asm`). The BIOS's console output waits a millisecond a byte, most of a frame for a few cells. Here the bytes go into a ring that empties without waiting: from the main loop whenever it waits, and from the BIOS's 2 ms clock interrupt (the jump at 0008h, which goes through ours first) two bytes at a time, so the screen keeps moving while a FujiBus call holds the CPU. The console's receive interrupt (0018h) is ours while the game runs: the keys go into a ring, and XOFF/XON from the terminal stop and start the output at once. Both vectors are put back on the way out. The clock interrupt also counts the 60 Hz frames.

**The network: stream mode, never waited on.** `src/nstrans_fb.c` drives the card's 82C55 itself (`src/h89/fbport.c`): a request goes out, and its reply is collected over the frames after. The open, the close, the appkeys and the table list are fujinet-lib's functions, but the call they all make, `fuji_bus_call`, is `fbport.c`'s own: fujinet-lib's `bus/h89` is 1.6K that repeats it, and its writes wait for ever on a card that doesn't answer (these time out). Before anything, the 82C55 is looked for (`plat_net_probe`): with no card, port D2h reads FFh.

**Controls** (the keypad is put in shifted mode):

| Key | Does |
|---|---|
| keypad 8 2 4 6 (7 9 1 3 diagonals) | walk; up/down in auctions; left/right in the lobby |
| SPACE or keypad 5 | the button |
| RETURN | end your development turn |
| ESC | leave the table: at once in the lobby; in a game it asks first (ESC again leaves). At the table list, back to CP/M |
| 0 | declare: sit this good out |
| BACKSPACE, DELETE | rub out, when typing a name |
| f1 | the bell off / on |
| f5, ERASE | draw the screen again |

A terminal sends keys, not keys held, and the H19 repeats only with its REPEAT key. So while walking and in the auctions an arrow keeps going until keypad 5, SPACE or another arrow, or until you arrive in town, on the map or come out of a building. Elsewhere an arrow counts as held for a sixth of a second.

**Sound:** the bell, for the one alert that wants your hand (your turn, the land grant, an auction, the summary).

**Testing.**
- **In fujinet-go-h89-desktop's headless H89** (`tools/h89sim/h89sim.py`), the scenario language, step by step through its runner's `--stdin`: CP/M boots from its 64K disk, `B:MULE` runs, and `snap` saves the H19's picture and its text.

      python3 tools/h89sim/h89sim.py r2r/h89-cpm/mule.h8d r2r/h89-cpm/mule.map scenario.txt out/

  `H89SIM_FUJINET=1` is a live game against the runner's in-process FujiNet, with the appkeys `H89SIM_NAME` and `H89SIM_TABLE` written first; with a private server, run both in `unshare -rn`.
  More commands for this target: `keyname NAME` (esc, up, f1... -- keys `type` can't send), `sleep MS` (after the game has quit and its clock stopped), and `profile N` / `pcprof N` / `stallpcs`. Build with `-DH89_PROF=1` (C and `h89io.asm`) for the last two: the clock interrupt keeps the last 256 PCs it interrupted (a fair profile, which the rig's debugger samples are not), and the worst stall's are kept too.
- **The rig's runner** needs fujinet-go-h89-desktop with `--stdin` and the ESC-like key names in `core/tools/h89_headless.c`, and its Z80 core's `EX (SP),IX` and `LD SP,IX` honouring the DD/FD prefix (`core/v89/z80.cpp`): zsdcc's frame entry and exit use both.
- **Live:** an `AUTOPLAY=1` build played 25 minutes against a private server (round 9: land grants and auctions, development, production, the goods' auctions) with no failed or dropped FujiBus call; leaving the table and quitting to CP/M were played with the keys (the BIOS's vectors come back).
- **zsdcc's lost store:** at -SO3 zsdcc can drop the result of `v |= n` on a local kept in the frame (it bit the SMS's input and the H89's bars). `tools/z80/scan-lost-store.py` looks for it in the compiler's assembly.

## Known limits

- **Assay results in town only**, as in the Intellivision version: the server doesn't track where anyone stands.
- **Other players' walks are reconstructed.** Computer turns are replayed as walks.
- **Grey.** HGR has no grey: the plains are black and the mountains white.
- **Network pauses.** Animations pause briefly while a network call runs, because the Apple II has nothing to run them during a SmartPort call (the CoCo, during a DriveWire call).
- **CoCo: phase changes** pause about a second while the phase's overlay is read.
- **CoCo: grey** is black or white, and the fourth player is striped.
- **H89: no colour.** Players are their seat numbers; the 25th line names them.
- **H89: the console line.** At 9600 baud a new screen takes a second or two to fill in; the development screen is drawn once per turn.
- **Lynx: the screen is 102 lines.** Text rows on the map and town screens share lines, as described above. A long message's two rows (22 and 23) are tight.
- **Lynx: untested on hardware** as written: MAME has no ComLynx to the outside, so the network half has only been run on paper and the offline test mode.
- **MS-DOS: phase changes** pause while an overlay is read the first time; on a 128K PCjr in 16 colours, every time. Over FUJINET.SYS that is a sector (512 bytes) a round trip: about 5 s at 9600 bps, half a second at 115200. `make pcjr-floppy` makes a boot floppy with MULE.EXE on it (`r2r/msdos/pcjr-boot.img`, from `PCJR_BOOT`, the rig's DOS disk), from which the overlays load at floppy speed.
- **MS-DOS, CGA:** the fourth player is striped, and the fence around the town's corral is white.
- **Adam: phase changes** read the phase's overlay (up to 6 blocks). That is quick from a FujiNet, and slow from a real tape.
- **Adam: arrow keys** repeat rather than hold (the keyboard sends characters): walking is smoother with a controller.
- **NES: untested on hardware.** No NES cartridge has been built yet. The MMC3 interrupt's line (`IRQ_EARLY`) and the drain budgets in `crt0.s` are set for MAME.
- **NES: colour clash** by 16 x 16 block. Two colours that need different palettes in one block share the last one drawn. On the map, a plot is 3 cells wide, so every other plot shares a block column with its neighbour.
- **NES: big redraws without a clear** (the summary's table, a scrolling message) go through the queue at about 280 bytes a frame. A message row being scrolled can show half its old position for a frame.
- **7800: untested on hardware.** No 7800 cartridge has been built yet, so all of it has run only in MAME, which applies MARIA's read mode and colours a line at a time: on a console the DLIs' writes may land mid-line. The display lists and the characters are in RAM that MARIA reads in 2- and 3-cycle windows.
- **7800: switching modes.** A text row under a graphics row starts a line late if the row above it is busy (over ~200 cycles of DMA), and a text row's last line is shown in the next row's mode: the font leaves both blank but for some punctuation.
- **7800: four sprite colours** a frame; a fifth takes the last.
- **7800: trading** redraws about every second frame (the price lines are drawn cells).
- **SMS: untested on hardware.** No SMS cartridge has been built yet (fujinet-hardware `SMS/FujiNet-SMS-Rev0`).
- **SMS: `.sms` images** are taken for cartridge images only by the fujiversal-sms ESP32 firmware (the `fujiversal-sms-board` work); the `add-sms` branch takes `.ROM` and `.BIN`.
- **TMS9918 (and the SMS, which keeps its cells): colour clash.** Text or a sprite-free drawing next to another colour can recolour a few pixels of its cell's row.
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


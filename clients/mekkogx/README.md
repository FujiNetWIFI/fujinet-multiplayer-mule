# M.U.L.E. for FujiNet 8-bit computers (MekkoGX)

A C client for the M.U.L.E. Tournament server in this repository, built with the [MekkoGX](https://github.com/fozzTexx/MekkoGX) template and [fujinet-lib](https://github.com/FujiNetWIFI/fujinet-lib). It is a port of the Intellivision client (`../intv`), and plays the same way:
- it polls `/state?bin=1` and draws what comes back the way the Atari 800 original does;
- it sends the player's moves;
- it plays every animation itself at the original's speed.

The first platform is the **Apple II**: a 128K enhanced //e, //c or IIgs, in HGR colour, with speaker sound.

The code is split so other MekkoGX targets can follow:

| Where | What |
|---|---|
| `src/*.c` | the game, platform neutral: protocol, phases, walking, the sprite scripts, text |
| `src/platform-specific/*.h` | what a platform provides: screen and sprites (`gfx.h`), input, sound, the 60 Hz clock (`plat.h`), far memory and overlays (`bank.h`) |
| `src/apple2/` | the Apple II implementation |

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
| ESC | leave the table (lobby) |
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

## Known limits

- **Assay results in town only**, as in the Intellivision version: the server doesn't track where anyone stands.
- **Other players' walks are reconstructed.** Computer turns are replayed as walks.
- **Grey.** HGR has no grey: the plains are black and the mountains white.
- **Network pauses.** Animations pause briefly while a network call runs, because the Apple II has nothing to run them during a SmartPort call.

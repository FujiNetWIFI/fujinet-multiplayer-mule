# M.U.L.E. for the Fairchild Channel F

A FujiNet client for the M.U.L.E. Tournament server, in hand-written F8
assembly for the Macroassembler AS (`CPU F3850`). It is a port of the MekkoGX
client (`clients/mekkogx/src/*.c`) by way of the Astrocade one; the comments
name the C routine each piece comes from. The server runs the game. The
client polls `/state?bin=1` (the 550-byte `MuleState` of
`clients/mule_proto.h`), draws it on the Channel F's 102 x 58, and plays the
animations at the Atari's speed. Graphics come from the Atari disk.

## The cartridge it needs

The image is 64K: the fixed 16K at `$0800-$47FF` plus six 8K pages mapped
at `$4800-$67FF`. It needs the FujiNet Channel F cart firmware with
**APPBANK and the tick register (cart protocol 2)**:
fujinet-firmware PR #1988, which stacks on #1987. Older firmware refuses the
image (TOOBIG) and the game never starts. The same branch carries the MAME
device (`pico/channelf/emu`, grafted into `~/Workspace/mame` by `apply.sh`).

- A page is mapped by storing anything to `$FD80+n`; only `SELPG`/`PGPUSH`
  do it. Page 0 boots (title, name, tables, appkeys, test mode); 1 lobby,
  summary, game over; 2 grant, production, colony events; 3 the auctions;
  4 development; 5 the data (strings, art, fx scripts), copied to RAM at
  `$A000` at boot. Pages never call each other: what they share is
  resident.
- The tick (`$FC0E/F`, 60 Hz, little-endian) is the game's clock: the
  Channel F has no vertical-blank interrupt. A slow frame catches up
  (`CATCHN`), so the set pieces and timers keep the Atari's time.
- Everything is drawn through a shadow of the screen at `$8000` (VRAM is
  write-only and slow): a pixel is written only when it changes. Sprites
  are drawn into it with a generation tag in bits 7-2 and taken off again.

## Building

    make                             # build/mule.bin
    make ENDPOINT=http://host:port   # the server when no lobby appkey says
    AUTOPLAY=1 ./build.sh            # plays by itself (soak tests)
    GALTEST=1|2 ./build.sh           # the sprite / set-piece galleries

It needs `~/asl` (Macroassembler AS 1.42, `asl` and `p2bin`) and
`mule.atr`, a link to the Atari disk, for the graphics (never committed;
without it the art is stand-ins and the build says so).

Every build runs `tools/checkrom.py` (the banked layout, page ids, claim),
`tools/checksize.py` (each page and the RAM ledger) and `tools/checkacc.py`
(nothing reads A after `PI`, `JMP` or `ENTER` destroyed it: the F8 trap that
bit most often).

## Playing

| Control | Does |
|---|---|
| stick | moves; up/down chooses a letter in a name |
| plunger (push down) | the button |
| START | ends the turn; finishes the name |
| MODE | leaves the table: at once in the lobby, in a game it asks first (MODE again leaves) |
| HOLD | sits a good out, declaring |
| pull up | rubs out (name entry) |
| TIME | sound off and on |

In MAME (`./run.sh`): arrows, Left Ctrl push, Left Alt pull up, and the
console keys 1 TIME, 2 HOLD, 3 MODE, 4 START.

Watching a game (no seat), "WATCH" sits in the top left corner where the
title leaves room. The appkeys are the FujiNet lobby's (creator 1, app 1):
key 0 the name, key 10 the table's URL, written back when a table is chosen.

## The screen

The 24 x 9 text grid (4 x 6 font) and three bands: the title row and the
message rows black (palette 0), the rest light grey with blue, red and
green inks (palette 2). The fourth player is red and green stripes.

- **Map**: plots 10 x 8 from (6, 10), the time bar at x 97, the message on
  y50/56.
- **Town**: the Atari's town scaled to 60 x 36 at (21, 10); money on y50,
  messages on y56, the corral's price and stock right of the bar.
- **Auctions**: the arena y10-37 (two price steps a row), each seat's field
  of 6 cells for BUY/SELL or its price, money, units; the store's prices
  at the arena's left.
- **Lobby / summary / game over**: a seat a row of 8 pixels with its species
  standing at the left; the summary's prompt on one row.

## Testing

`./run.sh` plays it in a MAME window against the fujinet-pc on
`FUJINET_TCP` (default 127.0.0.1:9995); `ENDPOINT=... ./run.sh` rebuilds for
another server first.

`tools/chfsim.py SCENARIO OUTDIR` runs MAME headless (`-cartslot fujinet`)
from a scenario: `test PHASE [ARG [PEVENT]]` starts the offline test mode,
then `wait`, `snap`, `hold`, `press`, `key`, `peek`, `poke`, `vcheck` (VRAM
against the shadow), `watch ADDR` (who writes there) and `traceon`. The
scenes in `test/scen/` and `screenshots/scenes/` are examples. Offline it
points the cart at `127.0.0.1:9`, never the everyday fujinet-pc.

`test/rig.sh SCENARIO OUTDIR SECS` plays live against a private server
(port 8097) and a private fujinet-pc copy (BoIP 19997); build with
`ENDPOINT=http://localhost:8097`, and give it
`ROOM='http://localhost:8097/?table=irata'` for a game against the
computers.

## Known limits

- Not yet run on hardware: bus timing, the VRAM settle and the tick under
  load are MAME's.
- The sprite flush costs about 1900 instructions a redraw; the transport
  ship's fast pass drops frames (its landing runs ~3 s long). The catch-up
  keeps the game's timers right.

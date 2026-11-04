# M.U.L.E. for the ColecoVision

A FujiNet client for the M.U.L.E. Tournament server, in hand-written Z80
assembly for zmac 1.3. It runs on a stock ColecoVision (1K of RAM) with the
FujiNet cartridge.

It is a port of the Astrocade client (`clients/astrocade`, itself a port of
the MekkoGX C client), with the Coleco Adam's look: its TMS9918 screen
(`clients/mekkogx/src/tms9918/gfx.c`), its art (the Namco font, the plots,
the town, the shapes) and its sound effects. The server runs the game. The
client polls `/state?bin=1` (the 550-byte `MuleState` of
`clients/mule_proto.h`), draws it the Atari's way, and plays the animations
at the Atari's speed.

## Building

    make                        # build/mule.rom, the cartridge image
    make ENDPOINT=http://host:port   (the server when no lobby appkey says)
    AUTOPLAY=1 ./build.sh       # plays by itself (soak tests)

The build needs:

- the zmac 1.3 source (`~/Workspace/zmac-1.3`, or `ZMACSRC=`). `build.sh`
  compiles its own copy with a bigger symbol table.
- `mule.atr`, a link to the Atari disk, for the graphics. It is never
  committed. `tools/mkgfx_coleco.py` runs the Adam's generator
  (`clients/mekkogx/tools/mkgfx_tms.py`) and converts its tables; without
  the disk it gets stand-in art, and the build warns.

`tools/mkimage.py` lays out the image and checks it (the header, the
mailbox pages clear, the claim). `tools/checksize.py` prints each region's
size and the RAM ledger, and fails the build on an overflow or a duplicated
label.

The lobby serves `dist/coleco/mule.col` (top-level `make dist`).

## Playing

| Control | Does |
|---|---|
| stick | walks; picks a letter, a table, a species; moves your auction line |
| either fire button | the button |
| `#` | ends the turn; finishes the name |
| `*` | leaves the table: at once in the lobby; in a game it asks first (`*` again leaves, anything else carries on) and the computer takes over your seat. In name entry: rubs out |
| `0` | sits a good out (declaring) |
| `9` | sound off and on (not in name entry, where digits are typed) |

The appkeys are the FujiNet lobby's (creator 1, app 1): key 0 is the name,
key 10 the table's URL. Without them the game asks for a name (and saves
it) and lists the server's tables. The chosen table is written back to
key 10.

## How it fits

**The cartridge.** The FujiNet cart's merged firmware serves a client only
as a flat 32K image, 30K of it usable. This needs APPBANK, added to
`fujinet-firmware/pico/coleco` (and its MAME device) for this client:

| Console | Holds |
|---|---|
| 8000H-BFFFH | fixed: the header, the frame interrupt, the network, the drawing, the art |
| C000H-DFFFH | the page: one read of `FD80H + n` maps image page n |
| E000H-F7FFH | fixed (free) |
| F800H-FFFFH | the mailbox |

| Page | Holds |
|---|---|
| 0 | boot (title, name, tables), the strings and the message line, the test mode |
| 1 | lobby, summary, game over; land grant, production, events, the fx engine |
| 2 | status per good, declaring, both auctions |
| 3 | development |

Page 0 is image 4000H-5FFFH; pages 1-3 follow the first 32K, so the image
is 56K. zmac 1.3 wants its ORGs in order, so `mule.asm` assembles pages 1-3
first (at 0000H.., each PHASEd to C000H) and `tools/mkimage.py` puts the
window first. A console RESET doesn't reach the cart: `START` selects page
0. Code in a page never maps another page itself; it goes through
`FARCALL` or fixed code.

**OS7.** The BIOS does what it can:
- `POLLER` reads controller 1 in the frame interrupt (`input.inc`);
- its sound engine plays the effects: each is a song of one or two notes,
  all sharing one data area, so a new effect replaces the one playing
  (`sound.inc`);
- `RAND_GEN` makes the random numbers, `WRITE_REGISTER` sets the VDP up.

VRAM bursts are the client's own (`vdp.inc`): OS7's `WRITE_VRAM` mishandles
counts past 256, and cell-by-cell painting needs reads and writes 29 cycles
apart, no faster.

**Interrupts.** The VDP's frame interrupt is the NMI; the BIOS jumps to
8021H. The handler counts `FRAMES`, runs POLLER and the sound engine, and
acknowledges the VDP. It never reads F800H and up, so it can land in the
middle of a mailbox transaction. Reading the VDP status also resets its
address latch, so every burst runs inside `VLOCK`/`VFREE`: the NMI then
leaves the VDP alone and `VFREE` catches the frame up.

**RAM.** 7000H-73B8H, less the BIOS's sound area (7020H-702AH) and 128
bytes of stack (a live round peaked at 80: the NMI's frame and OS7's POLLER
take about 45 of them). The state stays where the cart painted it (the 1K reply at F800H,
read with `RXGETB`); a compact HOT copy holds what ticks need (`ram.inc`).
Each phase lays its own variables over one shared block (`PV`).

**The screen.** Graphics II with the name table counting 0..255 in each
third: a 256 x 192 bitmap where each 8-pixel row of a cell has two colours.
Drawing reads a cell's 8 pattern and 8 colour bytes, paints them (a new
colour takes the place of the one with fewer pixels left) and writes them
back. x is the Atari's colour clocks scaled 3/2, 4 pixels in, so a plot is
exactly three cells; lines are the Atari's own. Text is a whole cell, 32 by
24, in the Namco font; the layout is the Adam's (`status.c`, `auction.c`
and the rest at 32 columns).

**Sprites.** The VDP's own, 16 x 16 and one colour each. A slot is up to
four of them with four pattern names of its own; its patterns are sent when
its shape or frame changes (`SPRSET`), widened 3/2 from the shapes' 1-bit
rows. `SPRFLUSH` writes the attribute table after each `FRWAIT`. With five
on a line the order turns a slot a frame, so they flicker instead of
vanishing. The background is drawn under them without lifting them.

## Testing

`tools/colsim.py SCENARIO OUTDIR` drives MAME (`~/Workspace/mame`, with the
FujiNet cart device and APPBANK) from a scenario, in the other sims'
language: `test`, `wait`, `waitfor`, `snap`, `hold`, `press`, `key`,
`peek`, `poke`, `pad`, `pc`, `show`, `dump`, plus `vdump` (VRAM).

The offline test mode (`test PHASE [ARG [PEVENT]]`) needs no network: in
test mode `RXGETB` asks the test page (`test.inc`), which makes each byte
of `teststart.c`'s state up from its offset. `screenshots/make-shots.sh
coleco` shoots every screen this way.

Live, against a private server and fujinet-pc (never production):

    AUTOPLAY=1 ENDPOINT=http://localhost:8099 ./build.sh
    ROOM='http://localhost:8099/?table=irata' test/rig.sh SCEN OUT SECS build/mule.rom build/mule.lst

`test/rig.sh` starts a muleserver on 8099 and a private copy of
fujinet-pc-rs232 with BoIP on 19999 and the appkeys seeded, and runs
colsim throttled (the server's clock is real time).

`run.sh` opens a MAME window against the fujinet-pc on `FUJINET_TCP`
(default 127.0.0.1:9995).

## Lessons learned

- **zmac 1.3:** ORGs only go up, and the symbol table is one namespace: a
  label in one page clashes with another page's, sometimes reported only as
  a JR "Value error" (`checksize.py` checks for duplicates).
- **OS7's `PLAY_IT`** doesn't restart a song that is still playing, and
  runs from the NMI's sound engine's data: call it inside `VLOCK`.
- **The header skips the BIOS title** (55H AAH), so nothing has silenced the
  sound chip or set the VDP up: `START` does both, and it sets `VDPBUSY`
  first, before the NMI can run the sound engine on uninitialised RAM.
- **Plots are compute-bound:** about a frame each, the bare plains far less
  (the map takes about 33 frames to draw).
- **Measure the stack:** `fill 7339 7388 0xAA` early in a colsim scenario,
  `dump` it at the end.

## Not done yet

- Back to CONFIG: there is no way to leave for the FujiNet's CONFIG but the
  power switch.
- Plot drawing could be faster (precomputed layer rows).

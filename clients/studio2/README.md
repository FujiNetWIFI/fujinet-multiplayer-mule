# M.U.L.E. for the RCA Studio II

A FujiNet client for the M.U.L.E. Tournament server, in CDP1802 assembly for
the Macroassembler AS (`CPU 1802`). It ports the MekkoGX client
(`clients/mekkogx/src/*.c`); the comments name the C routine each piece
comes from. The server runs the game. The client polls `/state?bin=1` (the
550-byte `MuleState` of `clients/mule_proto.h`, little-endian) and draws it
through the cart's graphics layer.

## The cartridge it needs

The Studio II can't write to a cartridge: every write is a read of a
hotspot address the cart decodes. The image needs the FujiNet Studio II cart
firmware with protocol 2 (fujinet-firmware PR #2187), which adds:

- images up to 192 blocks, with a bank window at `$C400-$CFFF` (bank n
  selected by reading `$E980+n`);
- writable image pages (a pointer and a POKE stream) and a cart-side COPY;
- a graphics layer: 96 pictures (8 x 24 at most) stamped at any pixel,
  rectangles with dither patterns, 16 sprites composed over the background
  each frame, double-buffered, the picture shaken or inverted (VIEW), pixel
  text, and HOLD (the picture held while it is redrawn);
- drawing while a FujiNet transaction is under way.

The client stops with "NEEDS NEW CART FIRMWARE" on a protocol 1 cart. The
same PR carries the MAME device (`pico/studio2/emu`, grafted into
`~/Workspace/mame-studio2` by `emu/apply.sh`; the binary `studio2-p2`).

| Range | Holds |
|---|---|
| `$0400-$07FB` | the runtime: entry, display interrupt, SCRT, FujiNet calls |
| `$0A00`, `$0C00-$BFFF` | fixed code: frame loop, network, text, map, phases |
| `$C400-$CFFF` | the bank window: 0 title, boot, name, tables, menu; 1 test mode; 2 pictures; 3-5 strings; 6 AUTOPLAY |
| `$D400-$DBFF` | cart RAM: the state (copied by the cart after each read), the one before, name, table, server |
| `$0800-$09FF` | the console's 512 bytes: variables, a phase union, text, the stack |

## The screen

64 x 128 pixels, 120 lines used; a pixel is ~3 lines wide on a TV.

- **Map**: 9 x 5 plots of 7 x 16 pixels, stamped: plains, 1-3 peaks, the
  river, the store. An owner's frame is its seat's pattern (solid, dotted,
  corners, bars); the M.U.L.E.'s good an icon; production a dot a unit. A
  plot for sale, a computer's land and quake jolts are outlined. The time
  bar runs down the right edge.
- **Town**: the outfitters above the street, assay, land office, pub and
  corral below; our colonist and M.U.L.E. walk it (sprites); the time bar
  across.
- **Auctions**: the store's prices either side of the good's name; the
  players in four columns, feet on their price; the sellers' and buyers'
  lines dashed; each player's BUY, SELL or price, then money and units,
  odd seats' fields inverted so the four read apart. A good's status is a
  bar per player, next month's need dashed beside it.
- **Summary**: the colonists walk in and climb to their bands in rank order;
  cash, land, goods and the total; the colony; FIRST FOUNDER and the rating
  at game over.
- **Set pieces** (`fx.inc`, `prod.inc`): the transport ship lands, leaves at
  the first grant and comes back; pirates raid; the pest crawls its plot;
  acid rain's cloud and bolt cross a row; sunspots spark and flash the
  picture; the planetquake shakes it; the meteorite falls and flashes;
  radiation's M.U.L.E. runs off; flames over the store.

Sound is one fixed tone, so cues are rhythms: select, sample, error, buy,
the alert (at each input moment), auction over, trade, event, founder,
and the soft step, tick and tock (the time bar's, under 30 left).

## Building

    make                             # build/mule.st2
    make ENDPOINT=http://host:port   # the server when no lobby appkey says
    make AUTOPLAY=1                  # plays by itself

It needs `~/asl` (Macroassembler AS). `build.sh` assembles `mule.asm` once a
bank and checks every listing: `checkrom.py` (no 3-cycle opcodes, the
hotspot register read only with LDN), `fitcheck.py` (a routine's short
branches stay on its page: `RFIT n`), `checkfixed.py` (fixed code the same
in every bank), `checkldr.py` (a value lost to a pointer load).
`tools/fitfix.py build/mule.b*.lst` resets every `RFIT` to its routine's size.

## Testing

`tools/s2sim.py SCENARIO OUTDIR` runs a scenario in MAME headless: `wait`,
`hold`, `press`, `down`/`up`, `snap`, `burst`, `text`, `sprites`,
`peek`/`poke`/`speek`, `until`, `watch`, `status` and `test PH ARG PEV` (the
offline test mode, mekkogx `teststart.c`). `test/rig.sh` puts a private
muleserver (port 8093) and a private fujinet-pc (BoIP 19975) behind it;
build with `ENDPOINT=http://localhost:8093`.

## Playing

    ./run.sh                             # MAME, in a window, against the deployed server
    ENDPOINT=http://host:port ./run.sh   # another server

Either keypad plays. In MAME keypad B is the numeric pad.

| Screen | Keys |
|---|---|
| your name | 2 4 6 8 move, 5 types the letter, 1 rubs out, 3 done |
| table list | 2 8 choose, 5 join |
| lobby | 4 6 your species, 5 ready |
| land grant | 5 claims the plot under the cursor |
| plot for sale, summary | 5 seen |
| declaring | 2 seller, 8 buyer, 5 sit it out |
| bidding, trading | hold 2 or 8 to move your line; 5 colludes (goods) |
| development | 5 begins your turn; 1-9 walk (diagonals too); 5 on the map: install, assay, list a plot, catch the wampus |
| game over | 5 the other view |

0 is the menu everywhere: 2 leaves the table (asked first in a game), 3 the
sound, 4 ends your turn. In town a door is its action: the corral (buy or
return a M.U.L.E.), the outfitters (crystite, smithore, energy, food), the
assay office, the land office, the pub (gamble: ends your turn). Walk off
either end of the street to the map, into the store to come back.

## 1802 notes

- No 3-cycle opcodes (long branches, long skips, NOP): the cart serves each
  fetch in time only for 2-cycle ones. Calls are SCRT (`FCALL`, `FRET`,
  `FJMP`); short branches reach their own page only, so routines sit on one
  page (`RFIT`), and one over 256 bytes is split.
- Labels are global across the includes: suffix them per routine.
- `LDR Rn, adr` goes through D: load pointers before the value they use.
- `GETV`/`PUTV` reach page `$08` only (RA.1 stays `$08`).
- `DW` is big-endian; little-endian tables are `DB lo, hi`.
- `jmp_r9` is entered with `SEP R8`, never `FJMP`.

## Screenshots

Every major screen is in [`screenshots/studio2/`](../../screenshots/studio2/),
with the other clients' in [`screenshots/README.md`](../../screenshots/README.md).

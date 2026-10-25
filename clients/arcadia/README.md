# M.U.L.E. for the Emerson Arcadia 2001

A FujiNet client for the M.U.L.E. Tournament server, in hand-written
Signetics 2650 assembly for the Macroassembler AS (`CPU 2650`). It ports the
MekkoGX client (`clients/mekkogx/src/*.c`); the comments name the C routine
each piece comes from. The server runs the game. The client polls
`/state?bin=1&be=1` (the 550-byte `MuleState` of `clients/mule_proto.h`,
big-endian) and draws it with the 2637 UVI's 8x8 cells, its block graphics,
its ROM glyphs, its four sprites and its user glyphs.

## The cartridge it needs

The image is a protocol 2 (APPBANK) image, 48K: the fixed 8K (fixed A at
`$0000-$05FF`, fixed B at `$2000-$28FF`) and 2K pages in the window at
`$0800-$0FFF`, selected by reading `$2D80+n`. It also uses the protocol's
1K of cart RAM (RAM-L `$0600-$07FF`, RAM-H `$2900-$2AFF`, read like ROM,
written through the mailbox) and its 60 Hz tick. It needs the FujiNet
Arcadia cart firmware with protocol 2, fujinet-firmware PR #2180; older
firmware refuses the image ("ROM TOO BIG"), and the client stops with
"NEEDS A FUJINET CART, PROTO 2" on a cart without it. The same branch carries
the MAME device (`pico/arcadia/emu`, grafted into `~/Workspace/mame` by
`emu/apply.sh`).

| Page | Holds |
|---|---|
| 0 | boot: appkeys (name, room), name entry, table list, join, leaving |
| 1 | the lobby |
| 2 | status summary, game over |
| 3 | the land grant |
| 4 | a land auction's plot, a good's status |
| 5, 18, 19 | an auction's arena: its lines and players; its labels, money and clock; its screen and input |
| 6, 7, 8 | the development: the map, the town, what they far-call (a computer's walk, events) |
| 9 | production and its colony event |
| 10 | the sprite scripts (mekkogx `fx.c`): ships, colony events, a computer's walk |
| 11 | the map and the time bar (the main loop keeps it current) |
| 12 | the pictures |
| 13 | the offline test mode |
| 14 | the URL, the clock's sync, the poll rate |
| 15-17 | strings (`src/strings.txt`, `tools/mkstr.py`) |
| 20 | AUTOPLAY builds: plays by itself |
| 21 | the title and its parade |

Pages never call each other directly. The main loop calls the phase's page
with each state and every frame (`PGCALL`, a tail branch: the page returns
to its caller); fixed code far-calls a page and comes back to the caller's
(`PGFAR`); a page hands the phase to another by setting `PHPAGE` and
`PGNEW`. The 2650 keeps return addresses on chip, 8 deep, and a ninth
silently overwrites the oldest: `tools/checkdepth.py` fails the build past 7.

## The screen

26 lines of 16 cells (8x8 pixels), even inks (white, cyan, magenta, blue) on
black. A cell of exactly `$C0` starts block graphics to the row's end (6
blocks a cell, in its ink); the display code never writes one by accident.

- **Map**: 9 x 5 plots, each two cells, one over the other (8 x 16 pixels):
  the land in the upper (1-3 peaks and the river are user glyphs, the river
  flowing), what is on it in the lower (a M.U.L.E.'s letter, production's
  units, an owner's flag), both in the owner's ink. Column 13 is the time
  bar (block graphics, 20 half cells). A plot for sale, a computer's land
  and planetquake jolts outline in white.
- **Sprites**: the walker, its M.U.L.E., the wampus, the grant cursor; the
  ships are all four side by side; each sprite's picture is its user glyph,
  rewritten in the vertical blank (three a frame).
- **Town**: eight buildings from the ROM's edges and corners with page glyphs
  for doors, windows and the corral's fence; our colonist and M.U.L.E. on
  the street; money and the corral's M.U.L.E.s; the time bar in block graphics.
- **Auctions**: the players in four columns, feet on their price; the
  sellers' and buyers' price lines are a row of a user glyph whose bytes
  hold a dotted line at the line's pixel row; each player's BUY, SELL or
  price under its column, then a line per player (name, money, units:
  every number in full, up to 5 digits) and the clock. A good's status is a bar per player in
  block graphics, with next month's need marked.
- **Summary**: the colonists walk in and climb to their bands in rank order;
  each band its name and score, then money, land and goods a row each (5
  digits and more, in full), and a bar against the leader.
- **Set pieces** (page 10): the transport ship lands before round 1, leaves
  at its grant and comes back as an event; the pirate ship raids; pest,
  acid rain, sunspots (the sky flashes), meteorite, radiation, fire each have
  their actors; the planetquake shakes the screen. A computer's turn walks
  its colonist (and M.U.L.E.) out to its plot and back.

Sound is mekkogx's set: select, sample, error, buy, the two-note alert,
auction over, trade, event, founder, and the soft step, tick and tock (the
time bar's, under 30 left). Select mutes.

## Building

    make                             # build/mule.bin
    make ENDPOINT=http://host:port   # the server when no lobby appkey says
    make AUTOPLAY=1                  # plays by itself

It needs `~/asl` (Macroassembler AS, `asl` and `p2bin`). Every build runs
`tools/checkdepth.py` (the deepest call chain, 7 at most) and
`tools/checkclob.py` (a register read after a cart-RAM write or another
call that clobbers it); `tools/fixreach.py` widens relative branches that
fell out of reach.

## Testing

`tools/arcsim.py` runs a scenario in MAME headless (`-cartslot fujinet`,
`-video none`): `wait`, `hold`, `press`, `down`/`up`, `snap`, `burst`, `text`
(the screen as text), `peek`/`poke` (symbols from the listings), `until`,
`watch`, `test PH ARG PEV` (the offline test mode, mekkogx `teststart.c`)
and `pc`. `ARCSIM_WAV=file` records the sound for `tools/sndcheck.py`.
`test/rig.sh` puts a private muleserver (port 8099) and a private
fujinet-pc (BoIP 19981) behind it; build with
`ENDPOINT=http://localhost:8099`; `SLOW=ms` adds a delay proxy.

## Playing

    ./run.sh                             # MAME, in a window, against the deployed server
    ENDPOINT=http://host:port ./run.sh   # another server

It rebuilds `build/mule.bin` for that server first (no autoplay), then runs
`~/Workspace/mame` (with the cart grafted in) against a fujinet-pc BoIP
listener on `FUJINET_TCP` (default 127.0.0.1:9995). A lobby room appkey on
that FujiNet names the server instead. In MAME the arrow keys are the disc,
numeric-pad 2 is FIRE and the numeric pad the keypad (- Clear, + Enter); 1
is Start, O Option, 5 Select.

| Screen | Disc | FIRE (keypad 2) | Other |
|---|---|---|---|
| your name (no name appkey) | up, down: the letter (held, it runs on); right: take it; left: rub out | take the letter | Enter or Start: done; Clear: rub out |
| table list | up, down: choose | join | Enter: join |
| lobby | left, right: your species | ready, or not | |
| status summary | | seen: go on | |
| land grant | | claim the plot under the cursor | |
| land auction, its plot | | skip the wait | |
| bidding, trading | up, down: move your line (let go: hold) | collude (goods) | |
| declaring | up: seller; down: buyer | | 0: sit it out |
| development, your prompt | | begin your turn | |
| development, the map | walk; into the store: town | on the wampus: catch it; leading a M.U.L.E.: install it; after the land office: list the plot; else take an assay sample | Start: end your turn |
| town | walk the street; up or down into a door | | Start: end your turn |
| game over | | the other view | |

Everywhere: Option leaves the table (asked first in a game); Select mutes.
In town a door is its action: M the corral (buy a M.U.L.E., or return
yours), C S E F outfit it (crystite, smithore, energy, food), A the assay
office (reads your sample), L the land office (then list a plot on the
map), P the pub (gamble: ends your turn). Walk off either end of the street
to the map.

## Screenshots

Every major screen is in [`screenshots/arcadia/`](../../screenshots/arcadia/),
with the other clients' in [`screenshots/README.md`](../../screenshots/README.md).

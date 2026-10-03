# M.U.L.E. for the Atari 2600

A FujiNet client for the M.U.L.E. Tournament server, in 6502 assembly for
Macroassembler AS, on the FujiNet 2600 cartridge (fn-2600,
`pico/atari-2600`). The server runs the game. The client polls
`/state?bin=1` (the 550-byte `MuleState` of `clients/mule_proto.h`) and draws
it. Its behaviour and timings are the MekkoGX client's
(`clients/mekkogx/src/*.c`), and its pictures come from the Atari disk.

It runs on a stock 2600 (128 bytes of RAM) with the FujiNet cartridge.

## Building

    make                         # build/mule2600.bin, 32768 bytes
    make ENDPOINT=http://host:port   (the server when no lobby appkey says)
    make layout                  # build/layout.bin: the map kernel, no network
    make hosttest                # the cartridge's composer, byte for byte
    make emu                     # re-graft the cartridge into MAME

The build needs:

- Macroassembler AS (`asl`, `p2bin`) on the PATH or in `~/asl`.
- The fn-2600 tree (`FUJI_FIRMWARE`, default `~/Workspace/fn-2600`), for
  `checkdefs.py`, `checkrom.py`, and the mailbox header.
- `mule.atr`, a link to the Atari disk (CRC32 F76D2896). It is never
  committed. Without it `tools/mkgfx2600.py` uses stand-in art and says so.
- Python 3 (with PIL only for the `--png` previews).

Every build checks:

- `checkdefs.py`: the mailbox equates against `fuji_mailbox.h`.
- `tools/checkbanks.py`: each 2K bank fits (p2bin truncates silently).
- `tools/mktail.py`: the fixed tail fits its 220 bytes.
- `checkrom.py`: no read-modify-write or indirect store on the write-only
  pages. Each bank's tables start at `DSTART`, and `tools/dataranges.py`
  tells the checker they are data.

## Playing

| | |
|---|---|
| stick | walk; choose; move your auction line; declare (up sell, down buy) |
| FIRE | start, join, ready; claim a plot; begin your turn; install, sample, catch the wampus; collude; sit out a declaration |
| SELECT | end your turn |
| RESET, held a second | leave the table (the computer takes the seat) |

If you join a table whose game has already started (or you leave a game), you watch it as a spectator. The row that would show your own things says "SPECTATOR" instead:
- the money under the map;
- your role in an auction;
- the footer of a good's status;
- row 18 of the summaries;
- the ready line in the lobby.

FIRE does nothing; RESET held leaves.
| left difficulty A | mute |

The name is the FujiNet lobby's shared username (appkey 0). Without one a
keyboard types it and writes it back. The room is appkey 10 (creator 1,
app 1), a URL `https://host/?table=id`: its host is the server, and its table
is joined at once. Without it, the build's endpoint and the table list. The
table chosen is written back to key 10.

## What is on the screen

- **The lobby.** Each seat is a band in its colour, with its name, a tick
  when ready, and its species; you are `*`. The stick changes species.
- **The map** is the playfield, 9 x 5 plots of 4 playfield pixels by 16
  entries of two scanlines, at the Atari's colour clocks 8-151 on the Atari
  map's grey. The playfield has one colour a scanline, so what an entry can
  show is decided by its colour:
  - entries 0-3 and 12-15: the owner's bracket, top and bottom, in the
    seat's colour, seat 0 outermost;
  - 4-8: the M.U.L.E.'s good as a white letter (F E S C), the store, or at
    production the units made, as bars;
  - 9-11: the ground, dark: 1-3 mountains, the river's dots.

  The cartridge composes all of it: `FN_BLIT_MULEMAP`, from the state's map
  arrays. `tools/mulemap.py` is its model and makes the firmware's golden
  test. The grant's cursor, and the plot for sale, is player 0 at double
  width: a box a plot big.
- **The status summary**: each seat a band with its rank, name, money,
  land and total, and the colony's total.
- **A good's status**: the Atari's bars as numbers stepping through the
  previous amount, usage, spoilage and production, then the store's stock,
  with each seat's need next round. Then the declaring.
- **The auctions**: the four species in their columns at their price lines
  (two drawn a frame, each 30 Hz), a step every 8 frames as on the Atari;
  the best ask and bid as dashed lines; the clock as bars at both edges.
- **Your turn**: the species' map figure walks the map with its M.U.L.E. a
  step behind, at the Atari's speeds (slower over the river and the
  mountains); the wampus flashes on its mountain. The store is the town:
  the buildings as letters (C S E F, A L P, the corral), the species'
  town figure, the doors, the actions. The time bar runs under the picture.
- **Messages**: the player events, the colony events, whose turn it is, the
  rating -- in the Atari's words (from `strings.c`), scrolled through a row.

Sound: a bell when your input is wanted (your turn, the grant's cursor
setting off, the auction lines about to go, declaring, the summary), and the
timer's tick as your turn and an auction run out.

## How it fits

**The cartridge.** The fixed half (`$1800-$1FFF`) is almost all mailbox:
six 128-byte text planes the cartridge renders into (and composes the map
into), the 512-byte reply window (two slices), the write-only control and TX
pages, the status page. The client's code there is the 220-byte tail: the
bank trampoline, the transport, the cold stub. Everything else is fifteen 2K
banks.

**Every transaction repaints the reply window**, so nothing drawn per frame
reads it. When a state arrives the screens copy out what they need (a few
bytes of zero page, the map into the planes by blit) before the next request
starts; `HFNEW` holds the next request until they are done. The walker reads
the map's terrain and ownership back out of the planes, which hold still.

**The banks** (`mdefs.inc`):

| Bank | Holds |
|---|---|
| 0 | boot: cold start, the title, appkeys, the name keyboard |
| 1 | the lobby |
| 2 | the request machine: one step a visit, and a state's arrival |
| 3 | the summary, game over |
| 4 | the map: the grant, the land for sale, production, others' turns |
| 5 | your turn on the map: the picture (the kernel, the bar, the shapes) |
| 6 | the town: the picture |
| 7 | the auctions' arena |
| 8 | the table list, joining |
| 9 | a good's status, declaring |
| 10 | text rows for the banks with no room (auction, your turn, the town) |
| 11 | your turn on the map: the logic (walking, the button, the wampus) |
| 12 | the town: the logic (the street, the doors, the buildings) |
| 13 | the player events' words |
| 14 | the other messages' words |

A bank switch is a jump (`MGOTO`, which resets the stack): where the entered
bank carries on is `MENT`. Each frame a screen bank draws, then in the
overscan visits at most a message bank, a text-row bank, its helper and the
request machine, each coming back to it. The request machine steps only when
the RIOT timer says the overscan has room, so every frame is 262 lines except
the few at a screen change.

**The request machine** (bank 2) is the family's sequence, non-blocking:
CLOSE the last connection, OPEN (the devicespec streamed into TX over two
visits, out of the path buffers: the endpoint, the table, the name), STATUS
until two readings agree or the whole state is there, READ. Actions are
queued (`QACT`) and ride the next request; each one's answer is the state it
returns (`NREQ` says which request fetched it). `hash=` is not used: the hash
comes back in a response header, which N: cannot read.

**RAM** is `mdefs.inc`'s ledger: the transport's, the system's, the request
machine's, HOT (the state between polls), each screen's own cells laid over
each other (`PV`, `$E8-$F5`), and a 10-byte stack.

## The firmware

This client needs one thing the fn-2600 cartridge did not do:
`FN_BLIT_MULEMAP` (19) in `vcs_render.c`, documented in `fuji_mailbox.h`.
`host_test/mule_gold.h` is `tools/mulemap.py --gold`'s output and
`host_test/test_render.c` compares. After a firmware change, `make emu`
re-grafts MAME. Stella's copy is `src/emucore/fujinet/sync.sh`.

`tools/checkrom.py` also learnt bank-qualified data ranges
(`--data B:$LO-$HI`); it used to report addresses in banks other than 0 at
`$1800`.

## Testing

    make layout && make shot      # the map kernel, no network: a snapshot
    make live SCEN=test/round.scen   # a round against a private server
    make live SCEN=auto SECS=2400    # a whole game, played by emu/auto.lua

`test/rig.sh` starts the server built from this repository on port 8096 and a
private fujinet-pc-rs232 copy with BoIP on 19996 (its appkeys seeded: the
name VCS, the room the local server), then MAME headless with a scenario:

- `emu/sim.lua` runs a scenario file: `wait`, `bank`, `zp`, `press`, `hold`,
  `snap`, `text` (the 21 rows, decoded), `peek`. `M26_FRAMES=1` reports every
  frame that is not 262 lines, `M26_BANKLOG` and `M26_WATCH` trace bank
  switches and writes to a cell.
- `emu/auto.lua` plays by itself and logs phases, rounds, cold restarts and
  long frames.

MAME must be the tree `apply.sh` grafted (`~/Workspace/mame`). The run is
throttled: the server's clock is wall time.

## Not here

- **Real hardware.** Like the family's other 2600 clients, it is checked in
  MAME; the bus timing of the RP2040 cartridge is the thing emulation cannot
  settle.
- **The lobby registration.** `cmd/muleserver/main.go` registers only the
  Atari client with the FujiNet lobby.
- **The Atari's animations** (the ship, the set pieces of the colony
  events, the summary's walk-in): a message stands in for each.
- **PAL.**

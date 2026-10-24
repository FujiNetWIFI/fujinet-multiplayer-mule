# M.U.L.E. for the Magnavox Odyssey 2

A FujiNet client for the M.U.L.E. Tournament server, in hand-written 8048
assembly for the Macroassembler AS (`CPU 8048`). It ports the MekkoGX client
(`clients/mekkogx/src/*.c`); the comments name the C routine each piece
comes from. The server runs the game. The client polls `/state?bin=1` (the
550-byte `MuleState` of `clients/mule_proto.h`) and draws it with the
Intel 8244's characters, quads, sprites and grid.

## The cartridge it needs

The image is a protocol 2 (APPBANK) image: the fixed 1K at `$400-$7FF`
(cart RAM: the display engine and its tables, written through the
mailbox's POKE register) and 2K pages at `$800-$F1F`, selected through
`FN_REG_CODEBANK`. It needs the FujiNet O2 cart firmware with protocol 2,
fujinet-firmware PR #2177; older firmware refuses the image. The same branch
carries the MAME device (`pico/o2/emu`, grafted into `~/Workspace/mame` by
`apply-mame.sh`) and the o2em model.

| Page | Holds |
|---|---|
| 0 | boot: cart check, name (appkey or picker), table list, join |
| 1 | the network: one request at a time through the mailbox |
| 2 | the main loop: a frame, the network, the phase's page |
| 3 | the lobby (and placeholders) |
| 4 | the land grant |
| 5, 9, 14 | strings banks 0-2 (`src/strings.txt`, `tools/mkstr.py`): the say queue |
| 6 | production, colony events, a land auction's plot |
| 7 | status summary, game over, a good's status |
| 8, 10 | an auction's arena, and its text |
| 11, 12, 13 | the development: the map, the town, its words |

The image is 31K: the fixed 1K and 15 pages. Pages never call each other. The main loop calls a phase's page with each
state (and a second page after it where one would not hold it) and every
frame; what a page wants said goes in the say queue, which the main loop
hands to the strings page.

## The screen

The 8244 shows 12 single characters, 4 quads of 4 and 4 sprites a frame.
The display engine (`src/fixed.inc`) re-points them as the beam goes: a
timer-counter interrupt at each band rewrites the characters in the gap
between two rows (foreground off, about 8 lines), from tables the main
program pokes into the fixed half. The 8048 runs its interrupts with A11
forced low, so the engine and every table it reads live in `$400-$7FF`.

- **Map**: the grid is 9 x 5 plots; each plot one character, from the ROM
  font. A mountain is "slope left" + "slope right" (a solid peak, shaded by
  its peaks: grey, cyan, white); every map row has exactly two, so their
  right halves are the 10 static singles. Owned land is a ball in the
  owner's colour, a M.U.L.E. its letter (F E S C), production its units.
  The river is a blue `=`, the store a `$`.
- **Text screens**: six lines of 16 characters (two quads interleaved).
- **Auctions**: the players are sprites in four columns, the sellers' and
  buyers' lines dotted with static dashes, the time bar the grid's right
  column; no text band crosses the arena, so the players never blank.
- **Town**: the buildings are the grid's, their letters two text lines
  (C S E F over A L P M).

Seat colours: red, green, tan, violet (blue is the river's). The font has
no `! , ' #`, and the strings are worded around that: a player event is one
line ("FAIR PRIZE +$75"), a rating four.

Sound is short blips from the VDC's shift register (`beep` in the fixed
half; the vblank ends them): select, alert, error.

## Building

    make                             # build/mule.bin
    make ENDPOINT=http://host:port   # the server when no lobby appkey says

It needs `~/asl` (Macroassembler AS, `asl` and `p2bin`). Every build runs
`tools/checkflags.py` (a jump tests A after an ALU op, not a MOV in
between), `checkobj.py` (pages overlapping), `checklayout.py`, `checkmb.py`
(memory-bank selects), `checkpage.py` (a MOVP table inside its reader's
page) and `checkstack.py` (interrupts plus the deepest call chain within
the 8-level stack).

## Testing

`tools/o2sim.py` runs a scenario in MAME (`-cartslot fujinet`):
`wait`, `hold`, `press`, `snap`, `burst` (a snapshot every frame),
`xram`/`iram`/`cart` peeks, `until`, `watch` (a per-frame change log) and
`auto` (plays from the client's RAM, to game over or a given state;
`O2SIM_SHOTS=N` snaps every N frames). `test/rig.sh` puts a private
muleserver (port 8098) and a private fujinet-pc (BoIP 19993) behind it;
build with `ENDPOINT=http://localhost:8098`; `SERVER_SRC=dir` builds the
server from a copy (one with `NumRounds = 1` gets to game over in four
minutes). `test/scen/soak1.scen` plays such a game through.
`tools/rowcheck.py` counts the frames of a burst whose rows lost pixels.

## Playing

    ./run.sh                         # MAME, in a window, against fujinet-pc
    ENDPOINT=http://host:port ./run.sh   # rebuilt for another server first

It needs `~/Workspace/mame` with the cart grafted in (`apply-mame.sh`) and a
fujinet-pc BoIP listener on `FUJINET_TCP` (default 127.0.0.1:9995). In MAME
the arrow keys are the left joystick and Left Ctrl its button.

| Control | Does |
|---|---|
| stick | walks; in the auctions pushes your line; declares (up seller, down buyer) |
| button | ready, claim, the action on a plot (install, sample, list), begin a turn; sits a good out when declaring |

The pub ends a turn early.

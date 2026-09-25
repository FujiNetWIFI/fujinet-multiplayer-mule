# M.U.L.E. for the Bally Astrocade

A FujiNet client for the M.U.L.E. Tournament server, in hand-written Z80
assembly for zmac 1.3. It is a port of the MekkoGX client
(`clients/mekkogx/src/*.c`); the comments name the C routine each piece
comes from. The server runs the game. The client polls `/state?bin=1` (the
550-byte `MuleState` of `clients/mule_proto.h`), draws it the Atari's way,
and plays the animations at the Atari's speed. Graphics come from the Atari
disk.

It runs on a stock Astrocade (4K of RAM) with the FujiNet cartridge.

## Building

    make                        # build/mule.bin, the cartridge image
    make ENDPOINT=http://host:port   (the server when no lobby appkey says)
    AUTOPLAY=1 ./build.sh       # plays by itself (soak tests)

The build needs:

- the zmac 1.3 source. `build.sh` compiles its own copy into `build/zmac`
  with a bigger symbol table (the stock table holds 2000 symbols, and this
  program has more). It looks in `~/Workspace/zmac-1.3` and the firmware
  tree, or `ZMACSRC=`.
- `mule.atr`, a link to the Atari disk (CRC32 F76D2896), for the graphics.
  It is never committed. Without it `tools/mkgfx_astro.py` falls back to
  stand-in art, and the build warns about it.

Every build runs three checks:

- `tools/checkrom.py --banked`: the cartridge layout (the claim, the page
  headers, the mailbox pages kept clear).
- `tools/checksize.py`: the size of each page and of the resident half, and
  the RAM ledger.
- `tools/checkcalls.py`: no page jumps into another page (see below).

## Playing

The hand controller moves, and its trigger is the button. On the keypad:

| Key | Does |
|---|---|
| = | ends the turn; finishes the name |
| C | leaves the table: at once in the lobby; in a game it asks first (C again leaves, the trigger or any other key carries on) and the computer takes over your seat |
| CE | rubs out (name entry) |
| 0 | sits a good out (declaring) |
| MR | mutes |

A name is entered with the stick: up and down choose a letter, right or
the trigger takes it. The keypad's digits go straight in.

The appkeys are the FujiNet lobby's (creator 1, app 1): key 0 is the name,
key 10 the table's URL. Without them the game asks for a name and lists the
server's tables. The chosen table is written back to key 10.

## How it fits

**The cartridge.** The image is banked (APPBANK, firmware protocol v2),
like gcal's: the 8K window plus 4K pages. One read at `FNBKSEL+page` maps a
page at 2000H-2FFFH. 3000H-3AFFH never moves; the resident core lives there:

- the frame interrupt, paging and `FARCALL`
- the frame loop and a state's arrival
- reading the reply window
- input, sound, the text renderer and font
- thin wrappers that far-call into the pages

The pages:

| Page | Holds |
|---|---|
| 0 | boot: title, name entry, table list |
| 2 | lobby, status summary, game over |
| 3 | land grant, production and colony events, the fx engine |
| 4 | status per good, declaring, both auctions |
| 5, 6 | development |
| 7, 8 | the strings (generated from `strings.c`), the message line |
| 9 | the offline test mode, autoplay |
| 10 | the request engine, appkeys, a state's arrival |
| 11 | the sprites: XOR blitter and shapes |
| 12 | the map's plots and logic, the town, fills |

Code in a page never maps another page itself; it goes through `FARCALL`
or resident code. All pages are PHASEd to 2000H, so a direct `CALL` from
one page into another assembles cleanly and runs whatever is at that
address in the caller's page. `tools/checkcalls.py` fails the build on
that.

**Interrupts.** The FujiNet Astrocade family keeps interrupts off, so that
the I register stays 0 and refresh cycles never land on the cart's hotspot
pages. This client needs a steady 60 Hz clock, so it runs IM 2 with I = 4FH:

- Refresh cycles then land in screen RAM (`fuji_mailbox.h` names 4FH as the
  safe value).
- The vector word sits in RAM at 4F6EH.
- The resident handler counts `FRAMES` on the line interrupt below the
  picture. It reads no cart hotspot, so an interrupt between a REGSEL and
  its REGDATA read does no harm: the cart pairs hotspot reads, not bus
  cycles.

Checked in MAME: `FRAMES` advances 60 a second, and mailbox transactions
complete with interrupts on.

**RAM.** The screen is 78 lines of 40 bytes (4000H-4C2FH). That leaves
830 bytes for everything else, the stack and the vector included. The C
client keeps three copies of the 550-byte state. Here:

- The reply stays where the cart painted it: the 1K window at 3B00H, read
  with `RXGETB`. It is valid from its READ until the next request begins
  (`STVALID`); every transaction repaints the window.
- A compact HOT copy is what code between states reads: the header, the
  players, and the map's terrain, owner and M.U.L.E.s. It also holds the
  phase's own section (grant, auction, or the first 14 bytes of the turn).
- When a state arrives, the phase code compares the window with HOT (st
  against prev). `MAPSRC` says which of the two holds the latest state
  while a phase's ENTER and STATE run. After that, HOT is refreshed.
- Each phase lays its own variables over one shared block (`PV` in
  `ram.inc`). The fx actors sit at its start for the phases that use them.

**The screen.** The Atari's 160 colour clocks become the Astrocade's 160
pixels one for one. Its 192 lines are squeezed to 3/8: a plot's 32 lines
become 12. The art is squeezed in the generator (rows OR-ed together, so
thin lines survive); positions are squeezed by `SPRSET` and `YCONV`.

Text uses the family's 4x6 font through the magic expander, on a grid of 40
columns and 13 rows. Each Atari screen is laid out again on 13 rows; the
C code's row numbers are noted where they change.

**Colours.** One palette: black, blue, orange and white. The players are
blue, orange, white, and blue/orange stripes on alternate lines (the CoCo
scheme). Stripes work for text, fills, plots and sprites.

**Sprites.** There are no hardware sprites. A sprite is drawn into the
screen through magic memory with XOR, expanded from one bit a pixel, and
shifted to its pixel. Drawing it again takes it off. MAME's funcgen
(`astrocde_v.cpp`) is the reference: the shifter's carry is the previous
write's data whatever its address. So columns off the screen are written to
a scratch byte rather than skipped, and the carry is zeroed first.

A background drawn under a sprite must lift the sprites first
(`SPRLIFT`/`SPRDROP`); the auction's price lines do.

## Testing

`tools/astrosim.py SCENARIO OUTDIR` drives MAME (`~/Workspace/mame`, with
the FujiNet cartridge device) from a scenario, in the other sims' language:
`test`, `wait`, `snap`, `hold`, `press`, `key`, `peek`, `poke`, `pad`,
`pc`, `trace`.

The offline test mode (`test PHASE [ARG [PEVENT]]`, phase + 1 in the hook)
needs no network. The state cannot fit in RAM, so in test mode `RXGETB`
asks the test page, which makes each byte of `teststart.c`'s state up from
its offset. The few things actions change are kept in RAM.

Live, against a private server and fujinet-pc:

    PORT=8091 muleserver
    fujinet -c fnconfig.ini -s SD      # (RS232 build, BoIP on 9998)
    ENDPOINT=http://localhost:8091 ./build.sh
    ASTROSIM_FUJINET=127.0.0.1:9998 python3 tools/astrosim.py scen out

Live runs are throttled, because the server's clock is real time.

## Lessons learned

- **zmac 1.3 reserves words:**
  - `PHASE`, `TITLE` and `FILL` (a BIOS equate in HVGLIB.H) can't be labels.
  - A macro parameter can't be named like a register (`d`) or like its own
    macro (case-insensitive).
  - A label can't share a line with a macro call.
  - The symbol table holds only 2000 names.
- **Far calls don't keep IX.** The sprite page works through IX, so its
  wrappers save the caller's. The fx engine lost its actor pointer mid-script
  until they did.
- **`YCONV` (and anything with DE scratch) clobbers a width in D.** Both
  AFILL and HDOTS drew 256-byte "lines" until they saved it.
- **Shared phase RAM needs care.** FXSTOP clears the actors, so it only acts
  when actors ran (`FXON`, in main RAM). Before that fix, it zeroed the
  auction's first variable.
- **The reply window is only good until the next transaction.** Anything a
  tick needs between states must be in HOT. Anything else waits for
  `STVALID`: the production dots, the wampus list, the summary table.
- **Killing test servers:** don't `pkill -f` a pattern that is in your own
  command line; match `/proc/PID/exe` instead.

## Not done yet

- LEAVE goes back to the table list; there is no Astrocade lobby to return to
  yet. The server's lobby registration has no `astrocade` entry either
  (`cmd/muleserver/main.go`, owned by the server side).
- Sounds are the Apple II table on tone A only; no music.
- Sprites are XOR: where two cross, or a sprite crosses coloured
  background, the colours mix.

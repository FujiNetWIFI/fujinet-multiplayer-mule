# M.U.L.E. for the Intellivision (FujiNet)

An Intellivision client for the M.U.L.E. Tournament server in this repository. It is written in IntyBASIC and talks to the server through a FujiNet cartridge, the same way the other FujiNet Intellivision games do (5 Card Stud, Texas Hold'Em, Battleship, Fujitzee).

The server runs the game. The client:
- polls `/state?bin=1` and draws what comes back the way the Atari 800 original does;
- sends the player's moves;
- plays every animation itself at the original's speed.

Those animations include walking, the transport ship arriving and leaving, the pirate ship, the colony events, the status summary walk-in, and FIRST FOUNDER.

[`docs/ANIMATION.md`](docs/ANIMATION.md) has the measured timings behind all of that. The game-rule timings are in [`../../docs/TIMING.md`](../../docs/TIMING.md).

## Building

    make                                  # mule.rom (jzIntv) and mule.bin/.cfg (cartridges)
    make ATR="M.U.L.E. (1983)(Electronic Arts)(US).atr"
    make ENDPOINT=http://myhost:8080      # server used when the lobby didn't choose one (remembered)

**What the build needs:**
- `~/Workspace/IntyBASIC`, the IntyBASIC 1.5 source. The Makefile compiles its own copy of the compiler into `build/`, because the `intybasic` on `PATH` may be 1.4, which has no `OPTION MAP`.
- `as1600` (from jzIntv).
- Python 3. Pillow is only needed for the `ATR=` conversion.

**Graphics.**
- **With `ATR=`** (or a `mule.atr` next to the Makefile), the map icons, species, town, pests, clouds and ship are converted from the original Atari disk by `tools/mkgfx.py`.
- **Copyright.** Those graphics belong to their copyright holders. They are written only to `gen/`, which is ignored by git; never commit them.
- **Without an ATR** the game builds with stand-in art.

**Build checks.** `make check` runs automatically. It confirms:
- every string fits the GROM;
- nothing lands at $7000–$9FFF, where the ECS and the FujiNet RAM and mailbox live;
- nothing lands in the $B800/$F800 GRAM mirrors.

## Running

    ./run.sh                               # jzIntv + FujiNet at localhost:9995
    FUJINET_TARGET=localhost:9997 ./run.sh

This needs the FujiNet-patched jzIntv (`~/Workspace/jzintv-20200712-src`) and a fujinet-firmware PC build with BoIP (the RS232 build).

## Joining a game

The FujiNet lobby passes the game its details in appkeys (creator 1, app 1):

| Key | Contents |
|---|---|
| 0 | the player's name, shared by every FujiNet game |
| 10 | the table's URL: `https://host/?table=<id>` |

**If the lobby didn't set them:**
- the game asks for a name (disc up/down picks a letter, right adds it, left rubs out, the button finishes);
- it lists the default server's tables.

The chosen table is written back to key 10, so resetting the console rejoins it.

## Controls (left hand controller)

If you join a table whose game has already started (or you leave a game), you watch it as a spectator. The title row has no room for a tag beside it, so it shows "SPECTATOR" for a second in every two and then puts the title back (`spec_tick` in `leave.bas`).

| Screen | Control |
|---|---|
| Lobby | Disc left/right: species. Button: ready / not ready. Keypad CLEAR: leave the table |
| Any screen in a game | Keypad CLEAR: leave the game (asks first: CLEAR again leaves, anything else carries on; the computer takes over your seat) |
| Spectating | Keypad CLEAR: leave at once. Nothing else does anything |
| Status summary | Button: continue (every player must press) |
| Land grant | Button: claim the plot under the cursor |
| Land auction | Button: skip the plot display. Disc up/down: raise/lower your bid; let go to hold |
| Development | Button: start your turn. Disc: walk. Keypad ENTER: end the turn |
| In town | Walk into a building: down into the assay office, land office, pub or corral; up into an outfitter (only while leading a M.U.L.E.). The corral sells you a M.U.L.E. (or takes it back) |
| On the map | Button on your own plot: install the M.U.L.E. you lead. Near the flashing wampus: catch it. Anywhere else: take an assay sample (for the assay office). After a visit to the land office: the button on your plot lists it for sale |
| Declare | Disc up: seller. Disc down: buyer. Keypad 0: sit out |
| Trading | Disc up/down: move your line. Button: collude |

## Files

| File | Contents |
|---|---|
| `mule.bas` | main program: includes, poll loop, phase dispatch |
| `memory.bas` | RAM map, colours, globals, helper functions |
| `net.bas` | URLs and actions; reads the 550-byte state in two pieces (the mailbox window is 512 bytes) |
| `netstep.bas`, `netpost.bas` | the main loop's requests, a mailbox transaction at a time: each is posted (`fn_post`) and its answer looked for on the next frames (`fn_check`), so the main loop never waits on the cart. `net_exchange` still waits, for the door in develop, leaving and boot |
| `fujinet.bas`, `input.bas` | the shared FujiNet transport and controller decoding. `input.bas` adds latching and a test hook |
| `isr.bas` | the per-frame handler: walking, grant cursor, auction sprites, sound |
| `fx.bas` | the sprite script engine for the set pieces |
| `events.bas` | ships, colony events, the production phase |
| `map.bas`, `grant.bas`, `develop.bas`, `auction.bas`, `status.bas`, `boot.bas` | the screens |
| `text.bas`, `textlong.bas`, `ids.bas` | strings, in the Atari's wording |
| `test.bas` | offline test mode (see below) |
| `gen/` | generated by `tools/`: `proto.bas` (from `../mule_proto.h`), `gfx.bas`, `gfxconst.bas`, `config.bas` |
| `tools/` | generators, the timing harness (`timing/`) and the headless test tools |

## Testing without a TV

`tools/intvsim.py` runs the ROM in jzIntv's debugger and renders real screenshots from BACKTAB, GRAM and the MOBs. It also injects controller input through a hook in `input.bas`.

**Offline test mode.** A scenario that starts with `test <phase> [n]` puts the game in offline test mode (`test.bas`), on a fake state with no server. You can:
- walk the town and map (`test 3`);
- watch any colony event (`test 4 <event>`);
- see the status and auction screens (`test 5 0`, `test 6 3`);
- see the summary (`test 7`) or game over (`test 8`).

    python3 tools/intvsim.py mule.rom scenario.txt out/

**Against a real server.** `tools/autoplay.py mule.rom out/ 10` plays against a real server for 10 minutes: it readies up, claims land, and buys, outfits and installs a M.U.L.E. each turn, with screenshots along the way.

## Known limits

- **Assay results in town only.** The server doesn't track where anyone stands, so the assay office reports on the plot you last sampled.
- **Other players' walks are reconstructed.** A computer's turn is shown by walking its species to the plot that gained a M.U.L.E. Other humans' turns show only their time bar and name.
- **Short text.** Messages longer than a row scroll, and the Atari's two-line messages are one scrolling line.
- **Colours.** The Intellivision has one grey, so the Atari's darker greys (mountains, the acid-rain cloud) are black.
- **No planetquake or meteorite reference.** Neither turned up in 24 seeded Atari games, so their animations are designed from the other events rather than measured.

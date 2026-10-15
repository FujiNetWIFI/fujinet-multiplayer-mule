# M.U.L.E. timing (Atari 800, NTSC)

These are the timings of the original game, measured frame by frame on the
Atari 800 disk (CRC32 F76D2896). The measurements used the libatari800
harness in `tools/extract/capture/`, with the scripts in
`tools/extract/capture/timing/`, and were checked against the disassembly.

All values are in **jiffies**. A jiffy is one NTSC vertical blank, 1/60 s. The
server clock (`t`, `pe` in the protocol) counts jiffies, and the web client
simulates at 60 Hz.

Sources:
- **M**: measured in an emulator trace.
- **C**: read from the game code (addresses are in the loaded image).
- **RE**: Kroah's reverse-engineering page.

## The game's clock

| Item | Value | Source |
|---|---|---|
| Event processor (BTU) | every 4 jiffies. The immediate VBI at `$5BB5` decrements the timers `$C3–$C6` whenever `RTCLOK & 3 == 0` | C, RE |
| Per-mode VBI handler | jump table at `$5D7E`, indexed by the game mode `$C1` | C |
| PTU (1 pixel of the turn bar) | 7 BTU = 28 jiffies. Flapper: 36 jiffies. Humanoid: 20 jiffies | RE, M (a full-food turn lasted 2828 jiffies) |
| CTU (land-grant cursor step) | 3 BTU = 12 jiffies | RE, M |
| ATU (auction time-bar tick) | 25 jiffies (`$B327`). Kroah's page gives "25 BTU", but the tick measures 25 jiffies | M, C |
| ATU while idle | 25 >> 2 = 6 jiffies | M, C (`$9D1B`) |

Game modes (`$C1`), with each mode's screen:

| Mode | Screen |
|---|---|
| 10 | status summary drawing |
| 2 | "press all player buttons" |
| 5 | ship landing |
| 7 | land auction |
| 4 | map and town walking |
| 14 | a computer's turn on the map |
| 16 | pub result |
| 0 | per-good status screen |
| 1 | declare |
| 3 | trading |
| 255 | transitions and messages |

## Round flow

| Step | Duration | Source |
|---|---|---|
| Status summary drawing | 410 | M |
| "Press all player buttons to go on" | waits forever (no timeout; 9000 jiffies traced) | M |
| Round 1: ship lands, "the ship will be back in 12 months" | 726 | M |
| Button press to the first grant-cursor step | 186 | M (rounds 1 and 2) |

### Land grant

| Item | Value | Source |
|---|---|---|
| Cursor step | 12 (1 CTU). The cursor blanks for 1 frame at each step | M |
| Plots the cursor passes over | the store and plots that are already owned are skipped, and skipping takes no time | M |
| Pause after a claim | 47–48 (4 CTU) | RE, M |
| After the sweep | 187–206, then the first plot is offered | M |

### Land auction

**Plot for sale.** The plot blinks on the map, with the countdown `t1` = 255 BTU (1020 jiffies). The display ends early once every human presses their button. (M)

**Time bar** (`$D3`). (C, M)
- The bar has 70 ticks.
- Each tick lasts 25 jiffies while any participating line's stick is pushed, and 6 jiffies otherwise.
- Participating means a buyer is at or above the start price (flags `$9F`–`$A2`).
- Computers push a simulated stick, so a computer holding its bid at the top keeps the bar slow.

**Line movement.** A pushed line moves 1 step (2 scanlines) every 8 jiffies. A released bid drifts back down at the same rate. (M)

**Scale.** Each step is $4. (M, RE)

| Scanline | Price |
|---|---|
| 151 | start − 7 steps. Buyers wait here |
| 137 | start price |
| 67 | top: start + 35 steps |

A bid can go past the top: the scale scrolls up with it, and only the bidder's money limits the bid.

**End.** "Auction is over" is shown for 191 (91 + 100). (M)

### Development

| Item | Value | Source |
|---|---|---|
| "Press your stick button to start" | waits forever. Ta (the wampus delay) is capped at 255 BTU | M, RE |
| Turn length | (10 + 91·min(1, food/need)) × PTU | RE, M |
| Where a turn starts | in the middle of town | M |
| Movement tick | every 6 jiffies | M |
| Town step | 4 colour clocks (8 px) horizontally, 4 scanlines vertically | M |
| Map step | 2 colour clocks (4 px), 2 scanlines. Diagonals move on both axes at once | M |
| Terrain on the map | plains: a step every 6 jiffies. River: every 14. Mountain: every 21 | M |
| Map bounds | x 55–197 colour clocks, y 63–163 player lines | M |
| Screen change between town and map | 25 | M |
| "Time has run out" | 200 | M |
| Pub result ("won $X gambling!") | 277 (156 + 121) | M |
| Computer turns | end at once; only the result display is shown (pub result, or "time has run out") | IMPL |

### Production

The map shows production and the colony event for 1421 jiffies. (M)

### Goods auction (one good)

**Stages.**

| Stage | Duration | Source |
|---|---|---|
| Status screen: usage, spoilage, production | 561 (127 + 434) | M |
| Declare | 30 ticks at the idle rate = 180 | M |
| Lead into trading | 57 | M |
| Trading | 70 ticks, with the same pushing/idle rule as the land auction | M |
| "Auction is over" | 191 | M |

The trading clock doesn't run while a buyer and a seller are in contact. (M, C `$DB`)

**Scale.**

| Scanline | Line |
|---|---|
| 65 | sellers start (1 step above the store's sell line) |
| 67 | the store's sell line |
| 137 | the store's buy line |
| 157 | buyers start (10 steps below the store's buy line) |

Each step is 2 scanlines: $1 for Food, Energy and Smithore, $4 for Crystite. The two store lines are 35 steps apart, which matches `sell = buy + 35` (or `+ 140` for Crystite). (M)

**Line movement.** 1 step every 8 jiffies. (M)

**Trade cadence.**
- Units change hands after 12, 12, 10, 8, 6, 4, 2, 2… BTU of contact: 48, 48, 40, 32, 24, 16, 8 jiffies.
- The delay is `max(2, $B374)`, and `$B374` drops by 2 per unit.
- It resets when the contact breaks.
- (M, C `$912E`)

**When a line can no longer trade.** It goes back to its starting edge, after a message of about 19 jiffies. (M)

## The time bar's tick

The original's countdown tick runs every jiffy from the VBI (`$5D07` calls `$5E98` while `$B33C` is set). `$D3` is the bar's units left: PTU on a development turn, ticks in an auction or while declaring.

| Item | Value | Source |
|---|---|---|
| When it ticks | while `0 < $D3 < 30` (threshold `$B33A = $1E`, set by `$3101`) | C |
| Interval | `$D3` jiffies: the counter `$B33E` reloads from `$D3` after each tick, so 29 jiffies at 29 left, 1 at 1 | C |
| First tick | the jiffy `$D3` first drops below 30 (`$B33E` starts at 1). Declare (30 ticks) ticks throughout | C |
| The sound | AUDF1 alternates `$0A` (~2906 Hz, first) and `$0F` (~1998 Hz); AUDC1 `$A2`, `$A1`, then `$A0`, plus a 1-jiffy noise click on voice 2 (AUDC2 `$81`) | C |
| Where it runs | development (`$67E3`), land auction and trading (`$9E14`), declare (`$656B`). Not the land grant | C |
| At 0 | voices 1 and 2 silenced, the tick disabled | C |

Every client plays it this way (MekkoGX `map.c` `bar_tick`, the Intellivision's `map.bas` `bar_tick`, the Astrocade's `mappg.inc` `XBARTICK`, the Channel F's `map.inc` `BARTICK`, the 2600's `devbar.inc` and `auction.asm`, the web's `barTick`).

## Where the server departs from these timings

- **Claim latency allowance.** A land-grant claim can name the plot the cursor left up to 24 jiffies earlier. This covers the round trip for a player pressing just as the cursor steps.
- **Computer turn pace.** In the original, a computer's development turn plays out on the map for the PTU it uses (25 + 2·distance per M.U.L.E., 10 + 2·distance per assay). Clients don't animate the computer's walk, so the server ends the turn at once and shows only its result display.
- **Button waits.** The original has no timeouts on its button waits. The server keeps that, but a player who stops polling for 2 minutes hands their seat to the computer, so an idle player can't block a network game.
- **Collusion length.** 10 ticks. This hasn't been measured yet.
- **Early pushes.** A stick pushed during the plot-for-sale display (land) or the lead into trading (goods) is held and applied on the first active jiffy. Without this, a human always starts a poll and a round trip behind the computers.
- **Backing off a trade.** When a trader in contact pushes away from the line they are trading with, their line steps away at once, not at the next 8-jiffy step, and no further unit trades. The push can carry the units-traded count the client last showed. Units traded after that count, within the last 60 jiffies (`TradeUndoWindow`), are given back.

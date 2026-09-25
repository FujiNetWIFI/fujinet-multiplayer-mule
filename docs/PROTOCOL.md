# M.U.L.E. server protocol (version 2)

This document describes the HTTP protocol spoken by `muleserver`. It follows
the conventions of the other FujiNet game servers in fujinet-game-system:

- Every route accepts both `GET` and `POST`.
- The client polls `/state`. (Stream mode, below, is a TCP alternative for
  FujiNet netstream clients: the Atari.)
- Every call carries `?table=<id>&player=<name>`. The player name is the
  identity and is matched without regard to case. The first call from a new
  name at a table in its lobby takes a free seat.
- Every action returns the complete new state, with the action's error
  (`ec`) and result (`res`) filled in.

## Output formats

The same data can be returned in four forms. Select one with a query parameter:

| Query     | Format | Notes |
|-----------|--------|-------|
| (default) | JSON with short keys | See "JSON state" below. |
| `raw=1`   | Flattened JSON: `path\0value\0` pairs | Paths look like `pl/0/n`, the same form as FujiNet JSON queries. Add `lf=1` to use `\n` instead of NUL, and `uc=1` or `lc=1` to force upper or lower case. |
| `bin=1`   | Fixed-layout binary, 550 bytes | See "Binary state". Little-endian by default; add `be=1` for big-endian. |
| `hash=N`  | Skip the state when nothing changed | Every response carries an `X-Mule-Hash` header. Send that value back as `hash`. If nothing has changed, the reply is `{"same":1,"t":..,"pe":..}`, or in binary a 16-byte header with `msgType=3`. |

`clients/mule_proto.h` is generated from the Go structs. Regenerate it with:

    go run ./cmd/muleserver -cheader > clients/mule_proto.h

A running server also serves it at `/mule_proto.h`.

## Time

The server clock counts **jiffies**. A jiffy is 1/60 of a second, the frame rate of the original game. Every duration in the protocol uses this unit. JSON reports the rate as `hz: 60`.

- `t` is the server clock in jiffies.
- `pe` is the number of jiffies since the current phase began.

Clients keep a jiffy clock synced to `t` and animate between polls:

| What | How to animate it |
|------|-------------------|
| Land-grant cursor | It moves to the next free plot once `gr.nx` jiffies have passed, then every `gr.ctu` jiffies. |
| Auction lines | Each line moves one `au.stp` price step every `au.lj` jiffies towards its target. |
| Auction time bar | `au.tk` ticks are left, and the current tick has `au.tl` jiffies left. Each later tick lasts 25 jiffies, or 6 when `au.idl` is set. |
| Turn time bar | It loses one pixel every `tu.ptu` jiffies. |

The `poll` field suggests a poll interval in ms: 100 during real-time phases and up to 500 otherwise. [TIMING.md](TIMING.md) lists every measured timing.

## Endpoints

| Route | Phase | Effect |
|-------|-------|--------|
| `/tables` | any | Lists tables: `[{t,n,p,m,ph,r}]`. With `bin=1`: a count byte, then entries of `char id[8], name[20], players[5]`. With `dev=1`: hidden dev tables. |
| `/state` | any | Joins the table (lobby only; otherwise you watch as a spectator), pings, and returns the state. |
| `/view` | any | Spectator view. Does not join. |
| `/leave` | any | Leaves the table. In a game, the computer takes over your seat. Returns `bye`. A leaving client must stop polling under its name: any later request with the same name at that table takes the seat back from the computer (that is how a dropped player rejoins). |
| `/setup/species/:s` | lobby | Sets your species, by name or number 0–7. |
| `/ready`, `/ready/1`, `/ready/0` | lobby, summary, landauction | The button. In the lobby it toggles or sets ready. On the status summary, every human must press. During a land auction's for-sale display, the display ends once every human has pressed. |
| `/claim/:plot` | grant | Claims the plot under the cursor. Accepted if the plot is under the cursor, or the cursor left it within the last 36 jiffies; the cursor then goes back to it for the claim pause. A computer claims on the jiffy the cursor arrives, but a human claim within 36 jiffies of that takes the plot from it. |
| `/auction/push/:dir[/:seen]` (alias `/auction/move/:dir`) | landauction, trading | The joystick. `1`/`up` means pushed up, `-1`/`down` means pushed down, `0`/`hold` means released. Send one call when the stick changes. A released land bid drifts back down. Pushing keeps the auction clock at its slow rate. A push sent before the lines are released (the plot-for-sale display, or the lead into trading) is held and applied the moment they are. `seen` is the units-traded count (`traded`) the client last showed. When a trader pushes away from the line they are trading with, trading stops at once, and units traded after `seen` in the last 60 jiffies are given back. |
| `/auction/at/:price` | landauction, trading | Moves your line towards a price. |
| `/declare/:role` | declare | `b` for buyer, `s` for seller, `n` to sit out. |
| `/collude` | trading | Press within 25 jiffies of another player to trade privately for 10 ticks. |
| `/turn/begin` | develop | Starts your turn. As in the original, the turn waits until you press. |
| `/turn/buymule` | develop | Buys a M.U.L.E. from the corral. |
| `/turn/outfit/:good` | develop | Outfits your M.U.L.E. with `food`, `energy`, `smithore` or `crystite` (or `f`/`e`/`s`/`c`, or 0–3). |
| `/turn/install/:plot` | develop | Installs your M.U.L.E. `res=1` means it ran away because the plot isn't yours. Installing on a plot that already has a M.U.L.E. swaps them. |
| `/turn/return` | develop | Returns your M.U.L.E. to the corral for a refund of the M.U.L.E. price. |
| `/turn/assay/:plot` | develop | `res` is the plot's crystite level, 0–4. |
| `/turn/sell/:plot`, `/turn/unsell/:plot` | develop | Lists or unlists one of your plots for the next land auction. |
| `/turn/wampus` | develop | Catches the wampus. `res` is the reward. It must be visible (`tu.wp`), and you must not be leading a M.U.L.E. |
| `/turn/pub` | develop | Gambles and ends your turn. `res` is your winnings. |
| `/turn/end` | develop | Ends your turn. |

## JSON state

```jsonc
{
  "v": 2, "tb": "irata", "ph": "develop", "p": 3, "r": 1,  // version, table, phase name/id, round
  "t": 1234, "pe": 56, "cd": -1,                         // clock, phase elapsed, lobby countdown (jiffies)
  "me": 0, "ec": 0, "err": "", "res": 0,                 // your seat (-1 spectator), error code/text, action result
  "pl": [ { "n": "alice", "sp": 5, "c": 0, "ai": false, "rd": false, "dc": false, "on": true,
            "m": 1600, "g": [4,2,0,0], "sc": 2100, "rk": 1, "pr": [0,0,0,0], "sl": [0,0,0,0] } ],
  "map": { "tr": [..45], "mt": [..], "ow": [..], "mu": [..], "cr": [..], "pr": [..], "ls": [..], "fs": [..] },
  "st":  { "s": [8,8,8,0], "pr": [30,25,50,100], "b": [15,10,50,100], "x": [50,45,85,240], "mu": 14, "mp": 100 },
  "gr":  { "cur": 12, "nx": 7, "cl": [-1,3,-1,-1], "ctu": 12, "end": false }, // land grant only
  "au":  { "land": false, "gd": 2, "pt": -1, "sl": -1, "sp": 0, "ak": 0,   // auctions and declare
           "sg": 3, "sgl": 0, "tk": 70, "tl": 25, "idl": false, "lj": 8,          // stage, time bar
           "lo": 40, "hi": 86, "sb": 50, "ss": 85, "stp": 1,
           "dn": false, "w": -1, "pc": 0, "tr": 0, "lt": 0, "col": 0,
           "ln": [ { "r": 1, "p": 40, "t": 40, "u": 0, "ps": 0, "in": false } ] },
  "tu":  { "s": 0, "st": true, "en": false, "len": 2828, "el": 40, "lf": 2788, "ei": 0, "ptu": 28, "ai": false,
           "hm": false, "cy": -1, "ct": false, "ev": 8, "et": "event text",
           "wp": [ { "p": 26, "m": 0, "f": 0, "t": 14 } ] },                          // wampus: only for the active seat
  "co":  { "ev": 1, "tx": "pests have attacked...", "pt": 12, "rt": 0, "rtx": "", "w": -1, "f": false, "tot": 7157 }
}
```

### Value codes

- **Phases (`p`):** 0 lobby, 1 grant, 2 landauction, 3 develop, 4 production, 5 declare, 6 trading, 7 summary, 8 gameover.
- **Goods:** 0 food, 1 energy, 2 smithore, 3 crystite.
- **Terrain (`tr`):** 0 plain, 1 river, 2 mountain, 3 store.
- **Species:** 0 mechtron, 1 gollumer, 2 packer, 3 bonzoid, 4 spheroid, 5 flapper, 6 leggite, 7 humanoid.
- **Roles (`ln[].r`):** 0 none, 1 buyer, 2 seller.
- **Auction stages (`au.sg`):** 0 show (the plot for sale, or the per-good status screen), 1 declare, 2 lead-in, 3 active, 4 done.
- **Colony events (`co.ev`):** 0 none, 1 pest, 2 pirates, 3 acid rain, 4 planetquake, 5 sunspot, 6 meteorite, 7 radiation, 8 fire, 9 ship returns.
- **Player events (`tu.ev`):** 1–22, in the order of `internal/rules/events_player.go`. The text is in `tu.et`. Binary clients should keep their own copy of the table.
- **Error codes (`ec`):** 0 none, 1 phase, 2 not your turn, 3 time up, 4 plot, 5 no M.U.L.E., 6 already have one, 7 out of stock, 8 money, 9 outfit, 10 done, 11 terrain, 12 seat, 13 bad request, 14 table full.

### Map field notes

`cr` is crystite: the values you have assayed, or -1 if you haven't. At game
over it shows the whole map.

## Binary state

`MuleState` is 550 bytes, packed, with fields in this order. See
`clients/mule_proto.h` for the full field list.

| Offset | Size | Struct |
|--------|------|--------|
| 0 | 16 | MuleHeader: `'M'`, version (2), msgType, phase, round, mySeat, err, reserved, `t` (jiffies, low 16 bits), elapsed, countdown, result |
| 16 | 4×29 | MulePlayer: `name[9]`, species, color, flags (`PF_*`), money, `goods[4]`, score, rank, `prod[4]`, `spoil[4]` |
| 132 | 225 | MuleMap: `terrain[45]` (bits 0–1 terrain, bits 2–3 mountains), `owner[45]`, `mule[45]`, `crystite[45]`, `prod[45]` (bits 0–3 units, bit 6 for sale, bit 7 lost) |
| 357 | 31 | MuleStore: stock, price, buy, sell, mules, mulePrice |
| 388 | 8 | MuleGrant: cursor, `claimed[4]`, step, next (jiffies until the next step) |
| 396 | 58 | MuleAuction: kind, good, plot, seller, flags (`AF_*`, including `AF_IDLE`), winner, colluders, stage, step, traded, prices, stageLeft, ticks, tickLeft, `role[4]`, `pos[4]`, `target[4]`, `units[4]`, `push[4]` |
| 454 | 86 | MuleTurn: seat, flags (`TF_*`), carrying, event, length, elapsed, left, endIn, ptu (jiffies), nWampus, `wampus[12]` |
| 540 | 10 | MuleColony: event, plot, rating, winner, failed, total |

A binary client can `read()` the response straight into a `MuleState`.
cc65 does not pad structs, so the header can be used as-is.

On an error that is not tied to a table (for example an unknown table), the
server sends only a `MuleHeader`, with `msgType=0xFF` and `err` set.

## Stream mode

Stream mode is for clients whose network calls are slow. The Atari is one:
it goes through SIO, and each HTTP poll costs about eight SIO frames and
0.45 s of blocking. The client uses FujiNet's netstream, which turns the SIO
line into a raw serial link to a TCP socket. It holds one connection to
`-stream` (default `:9902`, `MULE_STREAM_ADDR`; empty turns it off). The
server pushes the binary state as it changes, and the client sends actions
as they happen. Constants and codes are in `clients/mule_proto.h`
(`SC_*`, `CS_*`, `STREAM_*`), and the implementation is
`internal/proto/stream.go` and `internal/server/stream.go`.

**Framing.** Each frame is COBS-encoded and ends in a 0 byte, so a receiver
that lost or garbled a byte picks up again at the next 0. A decoded frame is
`type, payload…, sum1, sum2`. sum1 and sum2 are Fletcher-16, each mod 255,
over the type and the payload. A decoded frame is at most 128 bytes.

**Client → server**

| Type | Payload | |
|------|---------|--|
| `H` | ver (1), flags (bit 0: big-endian), table[8], player[8] (NUL padded) | join: from now on the connection is a `/state` poll for this player at this table |
| `A` | seq (1–255), act, arg lo, arg hi, seen | an action. `act` is the clients' `ACT_*` (the HTTP route it stands for); `seen` is the units-traded count for `ACT_PUSH`. `ACT_LEAVE` leaves the table (as `/leave`) and the connection then watches as a spectator until the next `H`, which rejoins |
| `T` | (1: the hidden dev tables) | list the tables |
| `R` | | send the whole state again (the client lost a frame) |
| `C` | n | credit: bytes taken out of the receive ring since connecting, mod 256 |
| `P` | | keepalive |
| `B` | | leave the table and close |

**Server → client**

| Type | Payload | |
|------|---------|--|
| `P` | off lo, off hi, bytes… | patch the state being built |
| `C` | ack seq | the state being built is complete: it becomes the client's state. When ack is an action's seq, `err` and `result` in the header answer that action; otherwise both are 0 |
| `T` | index, count, `MuleTable` | one table (count 0: there are none) |
| `E` | `ERR_*` | refused (an unknown table) |

**Pushes.** Every 50 ms, and at once after an action, the server builds the
player's view, exactly as `/state` would. The first push after `H`, and any
push after `R`, sends the whole state; after that only the byte runs that
changed are sent. If only the clocks changed (`t`, elapsed, countdown and the
phase timers), the push waits until 500 ms after the last one. A connection
keeps its seat online just as polling does. A connection that has sent
nothing for 35 s is dropped.

**Flow control.** A small machine's receive ring is small (the Atari's is 256
bytes), and it may go a while without draining it, for example while it
loads an overlay or draws a screen. So the server keeps no more than
`STREAM_WINDOW` (192) encoded bytes unacknowledged. The client reports what
it has taken with `C` frames. The count is cumulative mod 256, so a lost
credit frame is made good by the next one. Set `MULE_STREAM_DEBUG=1` to log
every frame.

**On the Atari** (`clients/mekkogx/src/atari/nstream.s`, `nsnet.c`):
- **Enabling the stream.** The Fuji device's `$70/$F0` command sends a
  64-byte frame, `host\0 flags AUDF3`. `DAUX1/2` carry the port, high byte
  first. The flags are `$05`: TCP, TX clock external. AUDF3 is 39, for
  19200 baud.
- **While the stream is on.** The motor line is asserted, POKEY's serial
  input IRQ fills the ring, and channels 3 and 4 clock the port, so the
  music keeps to voices 1 and 2.
- **Plain SIO.** Loading an overlay or writing an appkey needs normal SIO.
  For that the client drops the motor, and FujiNet holds the socket's data
  until the motor comes back on.
- **Quitting.** The client sends `B`, then `$F0` with host `STOP`.

## Lobby registration

When `GO_PROD=1` and `MULE_APPKEY` are both set, the server posts each
registered table to `https://qalobby.fujinet.online/server` (the QA lobby,
for now). There is one registered table, `irata` ("Planet IRATA"), so the
posted `serverurl` is `MULE_SERVER_URL?table=irata`. The FujiNet lobby client passes
that URL to the game in an appkey, and the game adds `&player=<name>`.

The appkey must be registered on the FujiNet firmware wiki page "SIO Command
$DC Open App Key".

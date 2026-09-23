# M.U.L.E. server protocol (version 2)

This document describes the HTTP protocol spoken by `muleserver`. It follows
the conventions of the other FujiNet game servers in fujinet-game-system:

- Every route accepts both `GET` and `POST`.
- The client polls `/state`. There is no socket.
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
| `/leave` | any | Leaves the table. In a game, the computer takes over your seat. Returns `bye`. |
| `/setup/species/:s` | lobby | Sets your species, by name or number 0–7. |
| `/ready`, `/ready/1`, `/ready/0` | lobby, summary, landauction | The button. In the lobby it toggles or sets ready. On the status summary, every human must press. During a land auction's for-sale display, the display ends once every human has pressed. |
| `/claim/:plot` | grant | Claims the plot under the cursor. Accepted if the plot is under the cursor, or the cursor left it within the last 36 jiffies; the cursor then goes back to it for the claim pause. A computer claims on the jiffy the cursor arrives, but a human claim within 36 jiffies of that takes the plot from it. |
| `/auction/push/:dir` (alias `/auction/move/:dir`) | landauction, trading | The joystick. `1`/`up` means pushed up, `-1`/`down` means pushed down, `0`/`hold` means released. Send one call when the stick changes. A released land bid drifts back down. Pushing keeps the auction clock at its slow rate. |
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
  "v": 2, "tb": "m1", "ph": "develop", "p": 3, "r": 1,  // version, table, phase name/id, round
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

`MuleState` is 543 bytes, packed, with fields in this order. See
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

## Lobby registration

When `GO_PROD=1` and `MULE_APPKEY` are both set, the server posts each
registered table to `https://lobby.fujinet.online/server`. The posted
`serverurl` is `MULE_SERVER_URL?table=<id>`. The FujiNet lobby client passes
that URL to the game in an appkey, and the game adds `&player=<name>`.

The appkey must be registered on the FujiNet firmware wiki page "SIO Command
$DC Open App Key".

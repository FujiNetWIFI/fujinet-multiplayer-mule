# fujinet-multiplayer-mule

A free (GPLv3) multiplayer server for **M.U.L.E.** in **Tournament mode**:
12 rounds, with assay, crystite, collusion and pirates, for up to 4 players.
The computer takes any seat without a human. It is built for FujiNet 8-bit
clients and follows the conventions of the other FujiNet game servers:

- Clients poll over HTTP.
- Every route answers `GET` and `POST` and takes `?table=&player=`.
- The server registers its tables with the FujiNet lobby.

Every response comes as short-key **JSON**, flattened `raw=1` key/value
pairs, or a fixed-layout **binary** struct (`bin=1`). 8-bit clients can read
the binary struct straight into C (see `clients/mule_proto.h`).

The rules follow Kroah's reverse engineering of the Atari 800 version, with
the manual filling the gaps. `docs/RULES.md` lists every rule and its source.
`docs/PROTOCOL.md` describes the protocol.

The game's timing matches the Atari original frame for frame: auction clocks, line speeds, trade cadence, walking speeds, the land-grant cursor and every pause. These were measured on the real disk in an emulator and are documented in `docs/TIMING.md`. The server clock and the web client both run at 60 jiffies per second.

## Run

    go run ./cmd/muleserver            # http://localhost:8080/play/
    go test ./...

The server has these tables:

| Tables | Players |
|--------|---------|
| `irata` ("Planet IRATA") | up to 4 humans; the computer plays any empty seat |
| `dev1`–`dev3` | hidden test tables |

A request without `?table=` goes to `irata`.

A game starts 31 seconds after the first human joins, or 6 seconds after
every human is ready.

## Web test client

`web/` is a playable canvas client, served at `/play/`. It has no build step.
It draws with graphics extracted from the Atari 8-bit disk. Those graphics
are Electronic Arts' copyright and are **not** included here. Generate them
from your own copy of the disk (`M.U.L.E. (1983)(Electronic Arts)(US).atr`,
CRC32 F76D2896):

    make assets ATR="path/to/M.U.L.E. (1983)(Electronic Arts)(US).atr"

Without the extracted files, the client draws plain shapes instead.

`tools/extract/capture/` holds an atari800 (libatari800) harness. It plays
the original game from a script and dumps screens and memory. It was used to
find the addresses and colours the extractor relies on.

The debug panel's "protocol debug" section polls `?bin=1` and decodes it. It
then checks the decoded binary against the JSON state.

### Controls

| Phase | Keys |
|-------|------|
| Lobby | ←/→ choose species; space: ready |
| Land grant | space claims the plot under the cursor |
| Development: map | Arrows walk. Walk into the store to enter town. Space on your plot installs a M.U.L.E.; with no M.U.L.E. it takes an assay sample; near a flashing wampus it catches it. E ends your turn. |
| Development: town | Walk into a building: corral buys or returns a M.U.L.E., shops outfit it, assay analyses your sample, land office lists a plot for sale, pub gambles and ends your turn. |
| Declare | ↑ seller, ↓ buyer, N sit out |
| Auctions | hold ↑/↓ to move your line; C colludes |
| Status screens | space continues |
| Any phase | Esc leaves: at once in the lobby, and in a game after asking (Esc again). The computer takes over your seat. |

## Deploy

`deploy.sh` deploys to Google Cloud Run with `max-instances=1`, because game
state lives in memory. For now the server registers with the QA lobby,
`https://qalobby.fujinet.online/`. Lobby registration needs `GO_PROD=1` plus:

- `MULE_APPKEY`: the lobby appkey id, default 10 (the key every client reads
  the chosen table from); 0 turns registration off
- `MULE_SERVER_URL`: the server's public URL, default `http://fujinet.online:9900/`

## Layout

    cmd/muleserver      entry point (-cheader prints the C header)
    internal/rules      deterministic Tournament engine (60 Hz jiffy clock, Advance(t))
    internal/ai         computer players
    internal/proto      JSON / raw / binary views, C header generator
    internal/server     tables, lobby, HTTP handlers, FujiNet lobby client
    web/                web test client
    tools/extract       ATR -> web/assets extractor (+ atari800 capture harness)
    clients/            generated C header for 8-bit clients
    docs/               RULES.md, PROTOCOL.md, TIMING.md

## License

GPLv3. See `LICENSE`. M.U.L.E. was designed by Dan Bunten (Ozark Softscape)
and published by Electronic Arts in 1983. This project is not affiliated with
Electronic Arts, and it contains none of the original game's code or graphics.

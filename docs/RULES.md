# M.U.L.E. Tournament Rules (implementation spec)

This file lists the rules the server actually implements, and where each one comes from.

- **[RE]** is Kroah's reverse-engineering page, http://bringerp.free.fr/RE/Mule/reverseEngineering.php5. It was made from the Atari 800 ATR with CRC F76D2896, the same dump used to extract the graphics.
- **[MAN]** is the Atari 8-bit manual, https://archive.org/details/Mule_atari8, read through its OCR text.
- **[IMPL]** marks a decision made for this server. The source had nothing to say, or the original real-time interaction had to be adapted to HTTP polling.

The constants are in `internal/rules/constants.go`.

## Time units [RE: Time unit; measured]
All timing follows the Atari original. The measured values are in [TIMING.md](TIMING.md).

- The engine clock counts **jiffies**, which are NTSC frames at 60 per second.
- One BTU is 4 jiffies.
- One PTU is 28 jiffies. It is 36 for the Flapper and 20 for the Humanoid.
- One CTU is 12 jiffies.
- The auction time-bar tick (ATU) is 25 jiffies while a line is being pushed, and 6 jiffies otherwise.

`Game.Advance(t)` replays every jiffy up to `t` in order, so results don't depend on how often clients poll.

## Game setup [RE: Players initial money; Round initialization; MAN]
- There are 12 rounds and 4 seats. Any seat without a human is played by the computer.
- **Species and starting money:**
  - Mechtron, Gollumer, Packer, Bonzoid, Spheroid and Leggite start with $1000.
  - Flapper starts with $1600. Humanoid starts with $600.
  - Computer players are Mechtrons with $1200 (the Tournament bonus).
- Each player starts with 4 Food and 2 Energy.
- The store starts with 8 Food, 8 Energy, 8 Smithore, 0 Crystite and 14 M.U.L.E.s.
- Starting prices are Food $30, Energy $25, Smithore $50, Crystite $100 and M.U.L.E. $100.

## Map [RE: Map generation]
The map is 5 rows by 9 columns, so plot index = row*9+col. The store is plot 22 (row 2, column 4). The river is column 4, which gives 4 river plots.

**Mountains.** In each row, one mountain plot is picked left of the river and one to the right. Each of the 4 mountain "quarters" in the row goes into the left or right plot at random. A plot can hold at most 3 mountains; any extra goes to the other plot.

**Crystite.** 4 High (3) deposits are placed at random. Each spreads in a diamond: Medium (2) on the plots next to it (up, down, left, right), and Low (1) on the ring outside that. Where diamonds overlap, the higher value wins. A meteorite creates Very High (4).

## Round sequence [RE section order; MAN]
1. Land grant
2. Land auctions
3. Development turns
4. Production and colony event
5. Goods auctions: Smithore, Crystite, Food, Energy
6. Corral builds M.U.L.E.s
7. Status summary

After round 12, or when the colony fails, the game ends with the colony rating.

## Land grant [RE: Lands grant]
The cursor sweeps plots 0..44 in order, one plot per CTU. It skips the store and plots that are already owned, and skipping takes no time. After a claim the cursor pauses for 4 CTU. A player presses the button to claim the plot under the cursor, and each player can claim one plot per round.

The phase ends when every seat has claimed a plot or the sweep finishes. If two claims land on the same plot, the lower-ranked player gets it.

**[IMPL]** Clients draw the cursor themselves from the cursor plot and the time to its next step. A claim may also name the plot the cursor left up to 24 jiffies earlier, to allow for network latency.

## Land auctions [RE: Lands auctions; MAN]
- Player-listed plots are auctioned first. Then Normal(1, amp 2) colony plots are auctioned, capped by the number of free plots.
- **Before each auction:** the plot for sale blinks on the map for 255 BTU. The display ends early when every human presses.
- **Length:** the auction runs for 70 time-bar ticks. A tick is 25 jiffies while any bidder pushes their stick, and 6 jiffies otherwise.
- **Bidding:** bids move one $4 step every 8 jiffies. A released bid drifts back down. A bid has no top price, only the bidder's money: the scale scrolls up with the high bid.
- **Starting price:**
  - $160 for the first auction of the game.
  - The first auction of each later round starts at the previous round's average price minus $60.
  - Each later auction in the same round starts at the previous auction's price minus $60.
  - **[IMPL]** The starting price is never below $60.
- **Colony plot:** the highest bid at or above the starting price wins when time runs out, with ties going to the lower-ranked player. If nobody bids, that ends the colony auctions for the round. The price recorded is start/2+52.
- **Player-listed plot:** the plot sells the moment a bid reaches the seller's asking price. The seller can lower the asking price. An unsold plot goes back to its owner.

## Development turn [RE: Available time; Turns order; Outfits price; MAN]
**Turn order.** Players go in rank order, best first. If the store has 7 or fewer M.U.L.E.s, the order is reversed.

**Food needed** in rounds 1–3 / 4–7 / 8–11 / 12 is 3 / 4 / 5 / 5 units.

**Turn length** = 10 + 91·min(1, food/needed) PTU.

**Outfit prices:**

| Food | Energy | Smithore | Crystite |
|------|--------|----------|----------|
| $25 | $50 | $75 | $100 |

**Installing a M.U.L.E.**
- A M.U.L.E. can only be installed on your own plot. Installing it anywhere else loses it.
- Smithore and Crystite can't be installed on the river.
- Installing on a plot that already has a M.U.L.E. swaps them, and you then lead the old one.
- **Returning a M.U.L.E.** to the corral refunds its price. The outfit cost isn't refunded.
- If your turn ends while you're still leading a M.U.L.E., it runs away.

**Assay** reports the plot's crystite level, 0–3. A plot hit by a meteorite reports 4.

**Pub:** you're paid a round bonus plus rand[0, 2·PTU left), capped at $250, and your turn ends.

| Rounds | 1–3 | 4–7 | 8–11 | 12 |
|--------|-----|-----|------|----|
| Round bonus | $50 | $100 | $150 | $200 |

**Wampus** [RE: Wampus hunting]
- **Where and when it shows:**
  - It shows on one of the mountains.
  - The first appearance comes after Ta/4 PTU. Ta is how long the player waited before pressing to start, in BTU, capped at 255.
  - It stays visible for 1–4 PTU, then hides for 4–7 PTU in the same place, or 2 PTU before moving somewhere else.
- **Catching it:**
  - It can be caught once per turn, and only while you aren't leading a M.U.L.E.
  - The reward is $100·(round/4+1).
- **[IMPL]** The server sends the turn's appearance schedule and checks each catch against it.

**[IMPL]** The client runs the walking and tracks travel time. The server enforces the turn deadline and the legality of each action.

## Player events [RE: Random player event]
- **Chance:** an event happens at the start of a turn 27.5% of the time.
- **Size of each event:** m = 25·(round/4+1).
- **Limits:** each event can happen once per game. The top-ranked player gets no good events. The two lowest-ranked players get no bad events.
- `internal/rules/events_player.go` has the full table of 22 events, with each one's text, condition and effect.

## Production [RE: Production]
**Base output by terrain:**

| Terrain | Food | Energy | Smithore | Crystite |
|---------|------|--------|----------|----------|
| Plain | 2 | 3 | 1 | crystite level |
| River | 4 | 2 | – | – |
| Mountain ×1/×2/×3 | 1 | 1 | 2/3/4 | crystite level |

**Capacity** = base output + 1 for every 3 plots of the same kind you own + 1 if you own an adjacent plot of the same kind.

**Output** = Normal(capacity, amp 2), clamped to 0..8.

**Energy:** every M.U.L.E. that isn't producing energy uses 1 Energy. For each unit you're short, one random plot of yours produces nothing.

**Stock** changes in this order: usage, then spoilage, then production. Nothing can go above 255.
- **Food:** subtract what's needed this round, then half of what's left spoils.
- **Energy:** subtract what the M.U.L.E.s use, then a quarter of what's left spoils.
- **Smithore and Crystite:** anything over 50 is lost.

## Colony events [RE: Random round event]
**Odds.** Each event has a lifetime limit per game:

| Pest | Pirates | Acid Rain | Planetquake | Sunspot | Meteorite | Radiation | Fire |
|------|---------|-----------|-------------|---------|-----------|-----------|------|
| 3 | 2 | 3 | 3 | 3 | 2 | 2 | 2 |

The chance of each event is its uses left divided by the total uses left for all events. Round 12 always brings "the ship returns" instead.

**Effects** (Tournament): Pirates steal all Crystite. The event code has the details of every event.

## Goods auctions [RE: Declaring, Trading, Purchase and sale price, Goods price variation; MAN]
**Store buy and sell prices.** P is the store's reference price for the good.

| Good | Store buys at | Store sells at | Step |
|------|---------------|----------------|------|
| Smithore | P | P+35 | $1 |
| Crystite | P rounded down to a multiple of 4 | that +140 | $4 |
| Food | P−15 | P+20 | $1 |
| Energy | P−15 | P+20 | $1 |

**Reference price before each auction:**
- **Food and Energy:** P·(0.25 + 0.75·ratio), where ratio = demand/supply clamped to [0.25, 3].
- **Smithore:** driven by M.U.L.E. demand, plus 7·Normal(0, amp 2).
- **Crystite:** 50 + rand(0..99).
- **Minimums:** Food 30, Energy 25, Smithore 20.
- **[IMPL]** P never goes above 250.

**Declaring.**
- Each player declares Buyer or Seller within 30 ticks. A status screen showing usage, spoilage and production comes first.
- The phase ends early once every human has declared.
- If a player doesn't declare, they're a Seller when they have a surplus.

**Trading.**
- Trading lasts 70 ticks, using the same pushing and idle rates as land auctions. The clock stops while a buyer and a seller are in contact.
- A buyer's line can't go above the lowest seller's line. When the store has stock, buyers also can't go above the store's selling price.
- A seller's line can't go below the highest buyer's line or below the store's buying price.
- While a buyer and a seller meet, units change hands after 12, 12, 10, 8, 6, 4, 2, 2… BTU. Each trade is at the meeting price, and ties go to the lower-ranked player.
- After the auction, P becomes the average price actually traded. After the Crystite auction, the store's Crystite is set to 0.

**Collusion.**
- Two or more players can press collude within the same ATU. They then trade only with each other for 10 ATU.

## Corral [RE: Corral mules building and pricing]
- The corral holds at most 14 M.U.L.E.s. The store builds min(14−current, Smithore/2) new ones, each using 2 Smithore.
- The M.U.L.E. price is ⌊2·P_smithore/10⌋·10.

## Score and rating [RE: Status summary]
**Score** = money + land + goods.
- **Land:** each plot is worth $500. A plot with a M.U.L.E. also counts the outfit cost plus $35.
- **Goods:** each unit counts at the store's reference price.

**Rank** is by score. Ties are broken at random.

**Colony rating** = round((total−10000)/20000), clamped to 0..6.

## Computer players [RE: Computers AI; IMPL]
The land grant, development, assay and deduction logic follows the RE page.

The RE page marks land bidding, declaring and trading as TODO, so these are
designed here:
- **Land bidding:** bid up to min(money−300, 300 + 4·rating + 10·rounds left). A computer only outbids the current leader, after a random pause.
- **Declaring:** declare Seller above critical level + 1, and Buyer below it. Smithore and Crystite are always sold.
- **Trading:**
  - A seller holds near the store's selling price for the first third of the auction. Then it meets the best buyer, or sells to the store.
  - A buyer moves towards the cheapest seller, or the store's selling price, until its need is met.
- **Smithore:** a computer rates Smithore higher while the corral has fewer than 6 M.U.L.E.s.

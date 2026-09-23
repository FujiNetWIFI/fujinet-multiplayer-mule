# Animation timings for the Intellivision client

[`docs/TIMING.md`](../../../docs/TIMING.md) lists the game-rule timings the
server follows. This file adds the timings the **client** owns: screen
geometry, walking, and the animations the original Atari game plays between
phases. They were measured on the Atari 800 disk (CRC32 F76D2896) with
`tools/timing/cap.c`, a copy of the libatari800 harness that also dumps
player/missile registers.

## Units

- **Jiffies.** All times are in jiffies (1/60 s). The Atari and the
  Intellivision both run at 60 Hz NTSC, so one Atari frame is one
  Intellivision frame.
- **Horizontal.** One Atari colour clock (cc) is one Intellivision pixel.
  - An ANTIC mode 6 character (8 cc) is one Intellivision card column.
  - The map's plot columns start at cc 8, which is Intellivision card column 1.
- **Vertical.** Two Atari scanlines are one Intellivision background pixel.
  - The map and town screens both start at Atari PMG line 44. That becomes Intellivision card row 1 (scanline 16), so `inty_scanline = atari_y - 28`.
  - MOBs keep scanline resolution: 0.5x Y scale is one scanline per row.
  - A map plot is 16 cc × 32 lines, which is 2×2 cards.
  - The town is 96 cc × 144 lines, which is 12×9 cards.
- **Sprite X.** An Atari player at HPOS `h` covers cc `h-48`. The Intellivision MOB x is `h - 40` (IntyBASIC's x origin is 8).

## Screen layout (Atari display list)

| Screen | Lines |
|---|---|
| Map | title (mode 2) at y 32–39; map 20 mode-6 lines, y 44–203; two message lines, y 216–231 |
| Town | title y 32–39; 18 mode-4 lines, y 44–187; "your money equals" below |
| Intellivision | row 0 title, rows 1–10 map (or rows 1–9 town), row 11 message |

## Walking

| Item | Value |
|---|---|
| Town step | 4 cc every 6 jiffies (40 cc/s). Vertical: 4 lines per step |
| Town street | player top y 87–95. Pushing past it is blocked, except at a door |
| Doors, lower row (HPOS) | assay 88, land office 108, pub 128, corral 148–164. Pushing down walks in: 4 lines per step, to y 123 |
| Doors, outfitter shops (HPOS) | crystite 88, smithore 112, energy 136, food 160, centred on the signs (checked by outfitting at each). They open only while you lead a M.U.L.E. |
| Leaving town | walking past HPOS 72 (left) or 176 (right). The screen blanks for 20–26 jiffies, then the map shows with you beside the store |
| Map step | 2 cc or 2 lines every 6 jiffies. The wait is set by what the figure touches where it stands (collision test at `$5837`): overlapping a mountain (player 2) with no playfield pixel makes it 21; overlapping the river (player 3) makes it 14. So only brushing a mountain slows you, not the whole plot |
| Map figure | on the map the player is a small figure, not the species sprite: boot `$9215`, 8 species (server order) x 10 frames x 8 bytes (byte 0 a flag). Frames: right 0/1, left 2/3, standing 4/5, down 6/7, up 8/9. Single width, one line per row |
| Map bounds | figure top at HPOS 55–197, player line 45–197 (MOB x 15–157, y 16–92) |
| First step after entering the map | held for 20 jiffies |
| Walk cycle | side frames 0/1 (right), 2/3 (left), switched each step. The body bobs 1 line on alternate steps. Standing uses front frame 5 |

## Ship arrival (round 1, and "the ship is back!" in round 12)

| Jiffy | What happens |
|---|---|
| 0 | map with "TRANSPORT SHIP" title and the message. Round 1 shows a 24-jiffy blue flash first |
| 36–282 | ship enters at the right edge and crosses right to left at 1 cc per jiffy |
| 282–300 | off screen |
| 300–763 | ship re-enters from the left, moving right at 1 cc per 6 jiffies |
| 763–829 | ship descends into the store in 8-jiffy shape steps (nose frames at `$7410`) |

The ship is two-tone: a light upper half and a dark lower half. The nose is 8 cc × 10 lines; the body is 32 cc × 32 lines.

Mode 5 lasts **782** jiffies (round 1) or **721** jiffies.

## Ship departure (after the first status summary)

| Jiffy | What happens |
|---|---|
| 0–45 | map with "THE SHIP WILL BE BACK IN 12 MONTHS." |
| 45–109 | ship rises out of the store in 8-jiffy shape steps |
| 109–742 | ship moves left at 1 cc per 7.9 jiffies until it leaves the screen |

Total: 721 jiffies.

## Status summary walk-in

| Jiffy | What happens |
|---|---|
| 0 | black screen |
| 29–35 | the players appear on the bottom row, 20 cc apart, starting at HPOS 191 |
| 35–315 | they walk left at 4 cc per 6 jiffies, bobbing each step |
| ~230 on | at the left column each one climbs to its row, in rank order |

Total: 410 jiffies. The table then prints, with "press all player buttons to go on".

## Colony events

All of these play during production. The map title changes to the event name, and the message line shows the event text, which the table below gives in quotes where the capture recorded it.

| Event | Animation | Length |
|---|---|---|
| Pest attack | A bug (11 lines, frames at `$778F`) crawls around the food plot, 2 cc per ~5 jiffies with 11-jiffy pauses at each turn. Then the plot's dots vanish | ~1100 |
| Pirate ship | "PIRATE SHIP". A red ship, with the same body as the transport ship and its nose at `$73B5`: fast pass right to left (1 cc per jiffy, ~250 jiffies), slow return from the left (1 cc per 6 jiffies) to land on the store (~816), stays ~150, then leaves left at 1 cc per 8 jiffies. "PIRATES TAKE ALL CRYSTITE!" from ~860 | ~1700 |
| Acid rain storm | A cloud (7 rows × 4 lines, double width, at `$799F`) with lightning (`$7983`) drifts left to right along the storm's row. Its colour cycles grey/blue/black/light blue every ~20 jiffies. "FOOD OUTPUT UP; ENERGY REDUCED." | ~700 |
| Planetquake | Not seen in 24 seeded games; reproduced as a screen shake | ~300 |
| Sunspot activity | 1–3 single-pixel sparks flicker at random places on the map. "ENERGY OUTPUT IS INCREASED" | ~370 |
| Meteorite | Not seen; reproduced as a meteor falling onto the plot | ~300 |
| Radiation | "MULE GOES CRAZY!" The plot's M.U.L.E. flashes (radiation dots at `$7EA1`), then runs off the right edge | ~130 |
| Fire in store | The store flickers with fire colours. "ALL THE STOCK IN THE STORE IS LOST!" | ~640 |
| Ship returns (round 12) | same as the arrival, with "THE SHIP IS BACK!" | ~830 |

After an event the title goes back to "PRODUCTION #n". Each plot then gets its production dots (up to 8), and the message changes to "PRESS ALL PLAYER BUTTONS TO GO ON."

## Game over ("FIRST FOUNDER")

| Jiffy | What happens |
|---|---|
| 0–64 | black screen. The players stand in a column on the left; the winner walks to the title |
| 64–540 | "FIRST FOUNDER" with the winner's sprite pulsing through its colour's luminances every ~32 jiffies |
| 576–700 | the winner walks back and the final status summary prints. "*** OVERALL, ..." scrolls along the bottom |
| ~800 | the final map, then "press all player buttons" |

## Auction blinking

In the Atari original, the players the auction is currently about blink: roughly 16 frames shown, then 16 hidden. The client does the same (`au_blinkers` in `auction.bas`):

| Auction | Who blinks |
|---|---|
| Trading | the lowest-priced seller and the highest-priced buyer, i.e. the two who are about to deal |
| Land auction | the high bidder |

## Auction status: what each player needs

When a food or energy status reaches "STORE HAS n UNITS", the Atari marks what each player needs for the next round. The need is next round's food requirement for food, and one unit per other M.U.L.E. for energy. Players who need nothing get no marks.
- **A dotted line** runs across the player's bar at the needed height. It is light grey above a short bar and black inside a bar that passes it.
- **A number** under the player shows how far short (light grey) or over (black) they are.

The client (`au_need_draw`) differs in a few ways:
- **Line:** it is drawn in the cards either side of the bar, because a card has only one colour.
- **Colours:** short is white and over is black. The Atari's light grey doesn't show on the Intellivision's grey.
- **Sign:** the number carries a "-" or "+" as well as the colour.
- **Bar position:** the bars stand on row 6, clear of the players' heads, like the Atari's.

## A computer's development turn

On the Atari, captured in round 2:
- **Figure:** the computer's small figure stands in the store for the whole turn (HPOS 123, line 118, which is MOB 83, 52). Its colour shows in the store's window.
- **Its land:** as the turn starts, all the computer's plots are outlined in white for about 40 frames.
- **Its work:** each plot it develops is outlined in white, steadily, for about 90 frames, then drawn in its colour.

The client (`dv_ai_walk`, `dv_ai_marks`) does the same, with one addition:
- **The walk:** the figure walks from the store to the plot it worked on, with its M.U.L.E. four steps behind, at map walking speed.
- **At the plot:** the M.U.L.E. stays there. The plot is outlined for the 90 frames the figure spends on it.
- **Back in the store:** the figure walks back and stands there until the next turn.

## Auction lines and prices

The Atari's auction screen:
- **Two dotted lines.** The sellers' line is at the lowest asking price: a player seller's, or the store's sell price while it has stock. The buyers' line is at the highest bid: a player buyer's, or the store's buy price. They follow the players and meet where a deal happens.
- **Store prices.** The sell price sits above the store's cyan icon, the buy price below its brown one.
- **Labels.** Each player's label reads BUY or SELL while they stand at their starting line, then shows the price they have moved to. The labels line up with the MONEY column.

The land auction has one line, at the high bid or at the bottom of the scrolling scale.

The client (`au_lines`, `au_lbl_tick`) draws the same, with these details:
- **Columns 2–16:** the lines stay clear of the icon in column 17.
- **Shared card row:** when the two lines fall in the same card row, a two-row dotted card (`gfx_hline2`) is loaded into GRAM 58.
- **Store prices:** row 1 (cols 14–17) for selling, row 8 (cols 0–3) for buying.
- **Labels:** row 9, in the money row's 5-character fields.

# Timing scripts

These scripts for the capture harness (`../cap`) measured the timings in
`docs/TIMING.md`. Run them in the order of their number prefixes, since
the early scripts create the savestates the later ones load:

1. `01-boot.txt` boots the disk. It selects Tournament mode with one human
   player and a colour, then saves the savestate `sel`.
2. `02-species.txt` picks the Gollumer and saves `chosen`, the "all players
   press buttons" screen.
3. `03-round.txt` traces a whole round. It sets `autopress 1800`, so the harness
   presses the button only after a screen has been waiting for 30 s. Any
   screen that ends sooner than that ended on its own timer.
4. `04-states.txt`, `05-town-state.txt` and `30-food-state.txt` save states at
   the start of the land grant, the land auction, the human's turn, the town
   and the food auction.
5. The `1x`, `2x` and `3x` scripts measure walking, land auctions and goods
   auctions.

    ./cap mule.atr timing/01-boot.txt out
    python3 timing/segs.py out/round.csv          # phase segments (mode $C1, display list)
    python3 timing/walksum.py out/walk2.csv mode,hp0,p0y
    python3 timing/varsum.py out/la2.csv m00d3 p0y,p1y,p2y,p3y
    python3 timing/memscan.py out/la_idle.mem 7153 8642   # find decrementing counters

The frame numbers in `docs/TIMING.md` come from these runs. Emulation is
deterministic, so the same run always gives the same frames.

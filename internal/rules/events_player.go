package rules

import "fmt"

// NumPlayerEvents is the number of random player events. [RE: Random player event]
const NumPlayerEvents = 22

type playerEvent struct {
	good bool
	text string
	x    int // multiplier of m
	cond func(g *Game, seat int) bool
	do   func(g *Game, seat int, m int) int // returns money delta
}

func hasMule(g *Game, seat int) bool {
	for i := range g.Map {
		if g.Map[i].Owner == seat && g.Map[i].Mule != NoGood {
			return true
		}
	}
	return false
}

func countMule(g *Game, seat int, goods ...Good) int {
	n := 0
	for i := range g.Map {
		if g.Map[i].Owner != seat {
			continue
		}
		for _, gd := range goods {
			if g.Map[i].Mule == gd {
				n++
			}
		}
	}
	return n
}

func freePlots(g *Game) []int {
	var f []int
	for i := range g.Map {
		if g.Map[i].Owner < 0 && g.Map[i].Terrain != Town {
			f = append(f, i)
		}
	}
	return f
}

func money(x int) func(*Game, int, int) int {
	return func(g *Game, seat, m int) int { return x * m }
}

var playerEvents = [NumPlayerEvents + 1]playerEvent{
	{}, // 0 unused
	{good: true, text: "you just received a package from your home-world relatives containing 3 food and 2 energy units.",
		do: func(g *Game, s, m int) int {
			p := g.Players[s]
			p.Goods[Food] = min(MaxStock, p.Goods[Food]+3)
			p.Goods[Energy] = min(MaxStock, p.Goods[Energy]+2)
			return 0
		}},
	{good: true, text: "a wandering space traveler repaid your hospitality by leaving two bars of smithore.",
		do: func(g *Game, s, m int) int {
			g.Players[s].Goods[Smithore] = min(MaxStock, g.Players[s].Goods[Smithore]+2)
			return 0
		}},
	{good: true, text: "your M.U.L.E. was judged \"best built\" at the colony fair. you won $%d.", x: 2, cond: hasMule, do: money(2)},
	{good: true, text: "your M.U.L.E. won the colony tap-dancing contest. you collected $%d.", x: 4, cond: hasMule, do: money(4)},
	{good: true, text: "the colony council for agriculture awarded you $%d for each food plot you have developed. the total grant is $%d.",
		cond: func(g *Game, s int) bool { return countMule(g, s, Food) > 0 },
		do:   func(g *Game, s, m int) int { return 2 * m * countMule(g, s, Food) }},
	{good: true, text: "the colony awarded you $%d for stopping the wart worm infestation.", x: 4, do: money(4)},
	{good: true, text: "the museum bought your antique personal computer for $%d.", x: 8, do: money(8)},
	{good: true, text: "you won the colony swamp eel eating contest and collected $%d. (yecch!)", x: 2, do: money(2)},
	{good: true, text: "a charity from your home-world took pity on you and sent $%d.", x: 3, do: money(3)},
	{good: true, text: "your offworld investments in artificial dumbness paid $%d in dividends.", x: 6, do: money(6)},
	{good: true, text: "a distant relative died and left you a vast fortune. but after taxes you only got $%d.", x: 4, do: money(4)},
	{good: true, text: "you found a dead moose rat and sold the hide for $%d.", x: 2, do: money(2)},
	{good: true, text: "you received an extra plot of land to encourage colony development.",
		cond: func(g *Game, s int) bool { return len(freePlots(g)) > 0 },
		do: func(g *Game, s, m int) int {
			f := freePlots(g)
			g.setOwner(f[g.rng.IntN(len(f))], s)
			return 0
		}},
	{text: "mischievous glac-elves broke into your storage shed and stole half your food.",
		do: func(g *Game, s, m int) int { g.Players[s].Goods[Food] -= g.Players[s].Goods[Food] / 2; return 0 }},
	{text: "one of your M.U.L.E.s lost a bolt. repairs cost you $%d.", x: -3, cond: hasMule, do: money(-3)},
	{text: "your mining M.U.L.E.s have deteriorated from heavy use and must be repaired. the cost is $%d.",
		cond: func(g *Game, s int) bool { return countMule(g, s, Smithore, Crystite) > 0 },
		do:   func(g *Game, s, m int) int { return -2 * m * countMule(g, s, Smithore, Crystite) }},
	{text: "the solar collectors on your energy M.U.L.E.s are dirty. cleaning cost you $%d.",
		cond: func(g *Game, s int) bool { return countMule(g, s, Energy) > 0 },
		do:   func(g *Game, s, m int) int { return -m * countMule(g, s, Energy) }},
	{text: "your space gypsy in-laws made a mess of the town. it cost you $%d to clean it up.", x: -6, do: money(-6)},
	{text: "flying cat-bugs ate the roof off your house. repairs cost $%d.", x: -4, do: money(-4)},
	{text: "you lost $%d betting on the two-legged kazinga races.", x: -4, do: money(-4)},
	{text: "your child was bitten by a bat lizard and the hospital bill cost you $%d.", x: -4, do: money(-4)},
	{text: "you lost a plot of land because the claim was not recorded.",
		cond: func(g *Game, s int) bool { return g.lastOwnedPlot(s) >= 0 },
		do: func(g *Game, s, m int) int {
			p := g.lastOwnedPlot(s)
			g.Map[p].Owner = -1
			g.Map[p].Mule = NoGood
			g.Map[p].ForSale = false
			return 0
		}},
}

// lastOwnedPlot returns the most recently acquired plot of seat (tracked by acquisition order).
func (g *Game) lastOwnedPlot(seat int) int {
	best, bestN := -1, -1
	for i := range g.Map {
		if g.Map[i].Owner == seat && g.acquired[i] > bestN {
			best, bestN = i, g.acquired[i]
		}
	}
	return best
}

// setOwner assigns a plot and records the acquisition order.
func (g *Game) setOwner(p, seat int) {
	g.Map[p].Owner = seat
	g.acqSeq++
	g.acquired[p] = g.acqSeq
}

// rollPlayerEvent picks and applies a player event at turn start.
func (g *Game) rollPlayerEvent(seat int) (int, string) {
	if g.rng.IntN(1000) >= PlayerEventChance {
		return 0, ""
	}
	rank := g.Players[seat].Rank
	var cands []int
	for id := 1; id <= NumPlayerEvents; id++ {
		ev := &playerEvents[id]
		if g.usedEvent[id] {
			continue
		}
		if ev.good && rank == 1 {
			continue
		}
		if !ev.good && rank >= NumSeats-1 {
			continue
		}
		if ev.cond != nil && !ev.cond(g, seat) {
			continue
		}
		cands = append(cands, id)
	}
	if len(cands) == 0 {
		return 0, ""
	}
	id := cands[g.rng.IntN(len(cands))]
	// Food rescue: a player without food receives the relatives' package. [RE]
	if g.Players[seat].Goods[Food] == 0 && rank != 1 && !g.usedEvent[1] {
		id = 1
	}
	g.usedEvent[id] = true
	ev := &playerEvents[id]
	m := 25 * RoundFactor(g.Round)
	delta := ev.do(g, seat, m)
	g.Players[seat].Money = max(0, g.Players[seat].Money+delta)
	text := ev.text
	switch id {
	case 5:
		text = fmt.Sprintf(text, 2*m, delta)
	default:
		if delta != 0 {
			text = fmt.Sprintf(text, abs(delta))
		}
	}
	return id, text
}

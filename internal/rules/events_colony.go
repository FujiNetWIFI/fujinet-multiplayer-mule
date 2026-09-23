package rules

// Colony events. [RE: Random round event]
const (
	CENone = iota
	CEPest
	CEPirates
	CEAcidRain
	CEQuake
	CESunspot
	CEMeteorite
	CERadiation
	CEFire
	CEShipReturns
	NumColonyEvents
)

var colonyEventCaps = [NumColonyEvents]int{0, 3, 2, 3, 3, 3, 2, 2, 2, 0}

// topTwo returns the seats ranked 1 and 2.
func (g *Game) topTwo() []int {
	var s []int
	for i, p := range g.Players {
		if p.Rank <= 2 {
			s = append(s, i)
		}
	}
	return s
}

func inSeats(seat int, seats []int) bool {
	for _, s := range seats {
		if s == seat {
			return true
		}
	}
	return false
}

// rollColonyEvent picks and applies a colony event to this round's Plot.Prod
// values (before they are added to stock).
func (g *Game) rollColonyEvent() {
	g.ColonyEvent, g.ColonyText, g.ColonyPlot = CENone, "", -1
	if g.Round == NumRounds {
		g.ColonyEvent = CEShipReturns
		g.ColonyText = "the ship is returning. the colony's twelve months are up!"
		return
	}
	total := 0
	for _, n := range g.colonyLeft {
		total += n
	}
	if total == 0 {
		return
	}
	x := g.rng.IntN(total)
	ev := 0
	for i, n := range g.colonyLeft {
		if x < n {
			ev = i
			break
		}
		x -= n
	}
	if !g.applyColonyEvent(ev) {
		return
	}
	g.colonyLeft[ev]--
	g.ColonyEvent = ev
}

func (g *Game) applyColonyEvent(ev int) bool {
	top := g.topTwo()
	switch ev {
	case CEPest:
		var c []int
		for i, pl := range g.Map {
			if pl.Mule == Food && pl.Prod > 0 && inSeats(pl.Owner, top) {
				c = append(c, i)
			}
		}
		if len(c) == 0 {
			return false
		}
		p := c[g.rng.IntN(len(c))]
		g.Map[p].Prod, g.Map[p].Lost = 0, true
		g.ColonyPlot = p
		g.ColonyText = "pests have attacked a food plot and ruined its entire crop."
	case CEPirates:
		for _, pl := range g.Players {
			pl.Goods[Crystite] = 0
		}
		for i := range g.Map {
			if g.Map[i].Mule == Crystite {
				g.Map[i].Prod = 0
			}
		}
		g.ColonyText = "space pirates have raided the colony and stolen all the crystite."
	case CEAcidRain:
		row := g.rng.IntN(MapRows)
		for i := range g.Map {
			pl := &g.Map[i]
			if pl.Mule == NoGood || pl.Lost {
				continue
			}
			r, _ := PlotRC(i)
			switch {
			case pl.Mule == Food && r == row:
				pl.Prod = clamp(pl.Prod+4, 0, 8)
			case pl.Mule == Food:
				pl.Prod = clamp(pl.Prod+1, 0, 8)
			case pl.Mule == Energy && r == row:
				pl.Prod = clamp(pl.Prod-2, 0, 8)
			case pl.Mule == Energy:
				pl.Prod = clamp(pl.Prod-1, 0, 8)
			}
		}
		g.ColonyPlot = row * MapCols
		g.ColonyText = "an acid rain storm has increased food production and decreased energy production."
	case CEQuake:
		if g.rng.IntN(2) == 0 {
			g.quakeMoveMountain()
		}
		for i := range g.Map {
			if g.Map[i].Mule == Smithore || g.Map[i].Mule == Crystite {
				g.Map[i].Prod /= 2
			}
		}
		g.ColonyText = "a planetquake has reduced mining production by half."
	case CESunspot:
		for i := range g.Map {
			if g.Map[i].Mule == Energy && !g.Map[i].Lost {
				g.Map[i].Prod = min(8, g.Map[i].Prod+3)
			}
		}
		g.ColonyText = "sunspot activity has increased energy production."
	case CEMeteorite:
		var c []int
		for i, pl := range g.Map {
			if pl.Terrain != River && pl.Terrain != Town && pl.Crystite <= 2 {
				c = append(c, i)
			}
		}
		if len(c) == 0 {
			return false
		}
		p := c[g.rng.IntN(len(c))]
		g.Map[p].Crystite = 4
		if g.Map[p].Mule != NoGood {
			g.Map[p].Mule, g.Map[p].Prod = NoGood, 0
		}
		g.ColonyPlot = p
		g.ColonyText = "a meteorite has struck the colony, creating a huge crystite deposit."
	case CERadiation:
		var c []int
		for i, pl := range g.Map {
			if pl.Mule != NoGood && inSeats(pl.Owner, top) {
				c = append(c, i)
			}
		}
		if len(c) == 0 {
			return false
		}
		p := c[g.rng.IntN(len(c))]
		g.Map[p].Mule, g.Map[p].Prod = NoGood, 0
		g.ColonyPlot = p
		g.ColonyText = "radiation from a cosmic storm has destroyed a M.U.L.E."
	case CEFire:
		g.Store.Stock[Food], g.Store.Stock[Energy], g.Store.Stock[Smithore] = 0, 0, 0
		g.ColonyText = "a fire in the store has destroyed all food, energy and smithore."
	default:
		return false
	}
	return true
}

// quakeMoveMountain shifts a mountain in columns 1,2,6,7 one plot sideways,
// destroying the M.U.L.E.s on both plots.
func (g *Game) quakeMoveMountain() {
	var c []int
	for i, pl := range g.Map {
		_, col := PlotRC(i)
		if pl.Terrain == Mountain && (col == 1 || col == 2 || col == 6 || col == 7) {
			c = append(c, i)
		}
	}
	if len(c) == 0 {
		return
	}
	p := c[g.rng.IntN(len(c))]
	d := 1
	if g.rng.IntN(2) == 0 {
		d = -1
	}
	q := p + d
	if g.Map[q].Terrain != Plain {
		q = p - d
		if g.Map[q].Terrain != Plain {
			return
		}
	}
	g.Map[q].Terrain, g.Map[q].Mountains = Mountain, g.Map[p].Mountains
	g.Map[p].Terrain, g.Map[p].Mountains = Plain, 0
	for _, x := range []int{p, q} {
		g.Map[x].Mule, g.Map[x].Prod = NoGood, 0
	}
	g.ColonyPlot = q
}

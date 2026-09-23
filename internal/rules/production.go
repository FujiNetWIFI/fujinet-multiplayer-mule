package rules

// Capacity returns the expected output of plot p for its owner producing g:
// base + learning curve + adjacency. [RE: Production]
func (g *Game) Capacity(p int, good Good, seat int) int {
	pl := &g.Map[p]
	c := pl.BaseYield(good)
	if c == 0 && good != Food && good != Energy {
		return 0
	}
	same := 0
	for i := range g.Map {
		if g.Map[i].Owner == seat && g.Map[i].Mule == good {
			same++
		}
	}
	if pl.Owner != seat || pl.Mule != good {
		same++ // count this plot as if installed
	}
	c += same / 3
	for _, n := range Neighbors4(p) {
		if g.Map[n].Owner == seat && g.Map[n].Mule == good {
			c++
			break
		}
	}
	return c
}

// EnergyNeed returns the energy units required by a seat's M.U.L.E.s.
func (g *Game) EnergyNeed(seat int) int {
	n := 0
	for i := range g.Map {
		if g.Map[i].Owner == seat && g.Map[i].Mule != NoGood && g.Map[i].Mule != Energy {
			n++
		}
	}
	return n
}

// computeProduction fills Plot.Prod for every installed M.U.L.E.
func (g *Game) computeProduction() {
	for i := range g.Map {
		g.Map[i].Prod = 0
		g.Map[i].Lost = false
	}
	for seat := 0; seat < NumSeats; seat++ {
		var plots []int
		for i := range g.Map {
			pl := &g.Map[i]
			if pl.Owner != seat || pl.Mule == NoGood {
				continue
			}
			cap := g.Capacity(i, pl.Mule, seat)
			if cap > 0 {
				pl.Prod = clamp(Normal(g.rng, cap), 0, 8)
			}
			if pl.Mule != Energy {
				plots = append(plots, i)
			}
		}
		short := g.EnergyNeed(seat) - g.Players[seat].Goods[Energy]
		for short > 0 && len(plots) > 0 {
			k := g.rng.IntN(len(plots))
			g.Map[plots[k]].Prod = 0
			g.Map[plots[k]].Lost = true
			plots = append(plots[:k], plots[k+1:]...)
			short--
		}
	}
}

// applyProduction performs usage, spoilage then adds production. [RE: Players status]
func (g *Game) applyProduction() {
	for seat, p := range g.Players {
		var prod [NumGoods]int
		for i := range g.Map {
			if g.Map[i].Owner == seat && g.Map[i].Mule != NoGood {
				prod[g.Map[i].Mule] += g.Map[i].Prod
			}
		}
		// usage
		food := max(0, p.Goods[Food]-FoodRequired(g.Round))
		energy := max(0, p.Goods[Energy]-g.EnergyNeed(seat))
		// spoilage
		p.LastSpoil[Food] = food / 2
		food -= food / 2
		p.LastSpoil[Energy] = energy / 4
		energy -= energy / 4
		p.Goods[Food] = food
		p.Goods[Energy] = energy
		for _, gd := range []Good{Smithore, Crystite} {
			p.LastSpoil[gd] = max(0, p.Goods[gd]-50)
			p.Goods[gd] = min(p.Goods[gd], 50)
		}
		for gd := Good(0); gd < NumGoods; gd++ {
			p.Goods[gd] = min(MaxStock, p.Goods[gd]+prod[gd])
			p.LastProd[gd] = prod[gd]
		}
	}
}

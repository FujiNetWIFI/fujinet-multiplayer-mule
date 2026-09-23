package rules

import "sort"

// updatePrice recomputes the store reference price for g before its auction.
// [RE: Goods price variation]
func (g *Game) updatePrice(gd Good) {
	s := &g.Store
	switch gd {
	case Crystite:
		s.Price[gd] = 50 + g.rng.IntN(100)
		return
	case Smithore:
		supply := float64(s.Mules) + float64(s.Stock[Smithore])/2
		free := min(len(freePlots(g)), 4)
		idle := 0
		for _, pl := range g.Map {
			if pl.Owner >= 0 && pl.Mule == NoGood {
				idle++
			}
		}
		demand := float64(clamp(free+idle, 0, 8))
		p := float64(s.Price[gd]) * (0.25 + 0.75*ratio(demand, supply))
		s.Price[gd] = int(p) + 7*NormalOffset(g.rng)
	case Food, Energy:
		supply := float64(s.Stock[gd])
		demand := 0.0
		next := g.Round + 1
		for seat, p := range g.Players {
			need := FoodRequired(next)
			if gd == Energy {
				need = g.EnergyNeed(seat)
				demand += float64(need)
			}
			supply += float64(max(0, p.Goods[gd]-need)) / 2
		}
		if gd == Food {
			demand = float64(FoodRequired(next) * NumSeats)
		} else {
			demand += NumSeats
		}
		p := float64(s.Price[gd]) * (0.25 + 0.75*ratio(demand, supply))
		s.Price[gd] = int(p + 0.5)
	}
	s.Price[gd] = clamp(s.Price[gd], StoreMinPrice[gd], StoreMaxPrice)
}

func ratio(demand, supply float64) float64 {
	if supply <= 0 {
		return 3
	}
	r := demand / supply
	if r < 0.25 {
		return 0.25
	}
	if r > 3 {
		return 3
	}
	return r
}

// buildMules converts store smithore into M.U.L.E.s and reprices them.
// [RE: Corral mules building and pricing]
func (g *Game) buildMules() {
	s := &g.Store
	n := min(MaxMules-s.Mules, s.Stock[Smithore]/2)
	if n > 0 {
		s.Mules += n
		s.Stock[Smithore] -= 2 * n
	}
	s.MulePrice = (2 * s.Price[Smithore] / 10) * 10
}

// ScoreOf computes money + land + goods for a seat. [RE: Status summary]
func (g *Game) ScoreOf(seat int) (total, money, land, goods int) {
	p := g.Players[seat]
	money = p.Money
	for _, pl := range g.Map {
		if pl.Owner != seat {
			continue
		}
		land += 500
		if pl.Mule != NoGood {
			land += OutfitCost[pl.Mule] + 35
		}
	}
	for gd := Good(0); gd < NumGoods; gd++ {
		goods += p.Goods[gd] * g.Store.Price[gd]
	}
	return money + land + goods, money, land, goods
}

// updateRanks recomputes scores and ranks (ties broken randomly).
func (g *Game) updateRanks() {
	idx := []int{0, 1, 2, 3}
	tie := make([]int, NumSeats)
	for i, p := range g.Players {
		p.Score, _, _, _ = g.ScoreOf(i)
		tie[i] = g.rng.IntN(1000)
	}
	sort.SliceStable(idx, func(a, b int) bool {
		pa, pb := g.Players[idx[a]], g.Players[idx[b]]
		if pa.Score != pb.Score {
			return pa.Score > pb.Score
		}
		return tie[idx[a]] < tie[idx[b]]
	})
	for r, s := range idx {
		g.Players[s].Rank = r + 1
	}
}

// ByRank returns seats ordered best first.
func (g *Game) ByRank() []int {
	idx := []int{0, 1, 2, 3}
	sort.Slice(idx, func(a, b int) bool { return g.Players[idx[a]].Rank < g.Players[idx[b]].Rank })
	return idx
}

// ColonyTotal is the sum of all scores.
func (g *Game) ColonyTotal() int {
	t := 0
	for _, p := range g.Players {
		t += p.Score
	}
	return t
}

// ColonyRating returns 0..6. [RE: Status summary]
func ColonyRating(total int) int {
	r := (total - 10000 + 10000) / 20000
	if total < 10000 {
		r = 0
	}
	return clamp(r, 0, 6)
}

var RatingText = []string{
	"the colony failed. the federation will not be sending another ship. you will all be left to die.",
	"the colony failed. the few survivors are taken off-planet by federation police. you have been sentenced to pay reparations.",
	"the colony barely survived. the federation has decided to keep it running, but with a new set of leaders.",
	"the colony was a success. the federation is pleased and will send new colonists.",
	"the colony was a great success. you will be honored for your work.",
	"the colony was a tremendous success. you will all be named honorary citizens.",
	"the colony was a phenomenal success! the federation is delighted and has declared irata a planetary landmark.",
}

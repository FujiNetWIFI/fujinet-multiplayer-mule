// Package ai implements the computer players. Land grant, development and
// assay follow the RE description of the original AI; land bidding, declaring
// and trading are marked TODO in the RE and are designed here. See docs/RULES.md.
package ai

import (
	"fujinet-multiplayer-mule/internal/rules"
)

// Brain is the default computer player.
type Brain struct{}

// New returns a Brain.
func New() *Brain { return &Brain{} }

var _ rules.Brain = (*Brain)(nil)

// goodRating returns how desirable producing gd is for seat. [RE: Computers turn AI]
func goodRating(g *rules.Game, seat int, gd rules.Good) int {
	var r int
	if gd == rules.Crystite {
		r = (100 + 4*g.Round) / 8
	} else {
		r = g.Store.Price[gd] / 8
	}
	if g.Round <= 1 && gd == rules.Food {
		r = max(r, 10)
	}
	p := g.Players[seat]
	switch gd {
	case rules.Smithore:
		// The colony needs smithore to build M.U.L.E.s.
		if g.Store.Mules < 6 {
			r += 8
		}
	case rules.Food:
		switch {
		case countMules(g, seat, rules.Food) == 0:
			r += 11
		case p.Goods[rules.Food] < rules.FoodRequired(g.Round+1):
			r += 10
		}
	case rules.Energy:
		owned, en := 0, countMules(g, seat, rules.Energy)
		for i := range g.Map {
			if g.Map[i].Owner == seat {
				owned++
			}
		}
		switch {
		case p.Goods[rules.Energy] < g.EnergyNeed(seat)+1:
			r += 11
		case en*4 < owned:
			r += 12
		}
	}
	return r
}

func countMules(g *rules.Game, seat int, gd rules.Good) int {
	n := 0
	for i := range g.Map {
		if g.Map[i].Owner == seat && g.Map[i].Mule == gd {
			n++
		}
	}
	return n
}

// capacity uses only the seat's knowledge of crystite.
func capacity(g *rules.Game, seat, plot int, gd rules.Good) int {
	pl := &g.Map[plot]
	if !pl.CanInstall(gd) {
		return 0
	}
	if gd == rules.Crystite {
		k := int(g.Players[seat].Known[plot])
		if k <= 0 {
			return 0
		}
	}
	return g.Capacity(plot, gd, seat)
}

// bestUse returns the best good and its rating for plot.
func bestUse(g *rules.Game, seat, plot int) (rules.Good, int) {
	best, bv := rules.NoGood, 0
	for gd := rules.Good(0); gd < rules.NumGoods; gd++ {
		v := goodRating(g, seat, gd) * capacity(g, seat, plot, gd)
		if v > bv {
			best, bv = gd, v
		}
	}
	return best, bv
}

// GrantChoice picks a plot at or after from. [RE: Computers land grant AI]
func (b *Brain) GrantChoice(g *rules.Game, seat, from int) int {
	best, bv := -1, -1<<30
	for p := from; p < rules.NumPlots; p++ {
		pl := &g.Map[p]
		if pl.Owner >= 0 || pl.Terrain == rules.Town {
			continue
		}
		_, v := bestUse(g, seat, p)
		v -= rules.StoreDistance[p] / 2
		row, _ := rules.PlotRC(p)
		if row == 3 {
			v--
		} else if row == 4 {
			v -= 2
		}
		v += g.Rng().IntN(4)
		if v > bv {
			best, bv = p, v
		}
	}
	return best
}

// DevTurn performs a computer development turn. [RE: Computers turn AI]
func (b *Brain) DevTurn(g *rules.Game, seat int) {
	t := g.Turn
	p := g.Players[seat]
	ptu := rules.PTUOf(p.Species)
	budget := t.Length / ptu
	used := 0

	// Assay (Tournament).
	high := 0
	for i := range p.Known {
		if p.Known[i] >= 3 && g.Map[i].Owner == seat {
			high++
		}
	}
	if high <= 2 {
		for i := range g.Map {
			pl := &g.Map[i]
			if pl.Terrain == rules.River || pl.Terrain == rules.Town || p.Known[i] >= 0 {
				continue
			}
			if pl.Owner != seat && (pl.Owner >= 0 || g.Rng().IntN(8) != 0) {
				continue
			}
			cost := 10 + 2*rules.StoreDistance[i]
			if budget-used-cost < 22 {
				break
			}
			if _, err := g.Assay(seat, i); err == nil {
				used += cost
				deduce(p, i, int(p.Known[i]))
			}
		}
	}

	// Install M.U.L.E.s on the best empty plots.
	for budget-used >= 35 {
		plot, good, gain := -1, rules.NoGood, 2
		for i := range g.Map {
			if g.Map[i].Owner != seat || g.Map[i].Mule != rules.NoGood {
				continue
			}
			gd, v := bestUse(g, seat, i)
			if gd != rules.NoGood && v > gain {
				plot, good, gain = i, gd, v
			}
		}
		if plot < 0 {
			break
		}
		if p.Money < g.Store.MulePrice+rules.OutfitCost[good] || g.Store.Mules == 0 {
			break
		}
		if g.BuyMule(seat) != nil || g.Outfit(seat, good) != nil {
			break
		}
		if _, err := g.Install(seat, plot); err != nil {
			g.ReturnMule(seat)
			break
		}
		used += 25 + 2*rules.StoreDistance[plot]
	}

	// Spend the rest at the pub.
	t.Deadline = t.Begin + (budget-used)*ptu
	if budget-used >= 3 {
		g.Pub(seat)
	}
}

// deduce infers neighbouring crystite from the diamond pattern.
func deduce(p *rules.Player, plot, v int) {
	if v < 3 {
		return
	}
	for _, n := range rules.Neighbors4(plot) {
		if p.Known[n] < 0 {
			p.Known[n] = 2
		}
	}
}

// Declare decides buyer/seller for a goods auction.
func (b *Brain) Declare(g *rules.Game, seat int, gd rules.Good) int {
	have := g.Players[seat].Goods[gd]
	switch gd {
	case rules.Food, rules.Energy:
		crit := g.Critical(seat, gd) + 1
		if have > crit {
			return rules.RoleSeller
		}
		if have < crit {
			return rules.RoleBuyer
		}
		return rules.RoleNone
	}
	if have > 0 {
		return rules.RoleSeller
	}
	return rules.RoleNone
}

// AuctionTick moves the computer's auction line.
func (b *Brain) AuctionTick(g *rules.Game, seat int) {
	a := g.Auction
	l := &a.Lines[seat]
	if a.Land {
		landTick(g, seat)
		return
	}
	p := g.Players[seat]
	crit := 0
	if a.Good == rules.Food || a.Good == rules.Energy {
		crit = g.Critical(seat, a.Good) + 1
	}
	switch l.Role {
	case rules.RoleSeller:
		l.Floor = crit
		best := a.StoreBuy
		for s := range a.Lines {
			o := &a.Lines[s]
			if s != seat && o.Role == rules.RoleBuyer && o.Pos > best && g.Players[s].Money >= o.Pos {
				best = o.Pos
			}
		}
		// Hold out near the store's price for a while before dumping.
		if a.Ticks > rules.TradeTicks*2/3 {
			best = max(best, a.StoreSell-rules.PriceStep[a.Good])
		}
		l.Target = best
	case rules.RoleBuyer:
		l.Want = max(1, crit-p.Goods[a.Good]+l.Units)
		if l.Units >= l.Want {
			l.Target = a.Lo
			return
		}
		best := a.StoreSell
		if g.Store.Stock[a.Good] == 0 {
			best = a.Hi
		}
		for s := range a.Lines {
			o := &a.Lines[s]
			if s != seat && o.Role == rules.RoleSeller && o.Pos < best && g.Players[s].Goods[a.Good] > o.Floor {
				best = o.Pos
			}
		}
		l.Target = min(best, p.Money)
	}
}

func landTick(g *rules.Game, seat int) {
	a := g.Auction
	l := &a.Lines[seat]
	p := g.Players[seat]
	left := rules.NumRounds - g.Round
	if l.Role == rules.RoleSeller {
		l.Target = max(a.Start, 300+25*left)
		return
	}
	_, v := bestUse(g, seat, a.Plot)
	maxBid := min(p.Money-300, 300+4*v+10*left)
	maxBid -= maxBid % rules.LandStep
	high := a.Start - rules.LandStep
	for s := range a.Lines {
		if s != seat && a.Lines[s].Role == rules.RoleBuyer && a.Lines[s].Pos > high {
			high = a.Lines[s].Pos
		}
	}
	if a.Seller >= 0 {
		high = min(high, a.Ask-rules.LandStep)
	}
	switch {
	case l.Pos > high && l.Pos >= a.Start:
		l.Target = l.Pos // leading: hold the bid
	case high+rules.LandStep <= maxBid:
		l.Target = max(a.Start, high+rules.LandStep) // outbid
	default:
		l.Target = a.Lo // too rich: drop out
	}
}

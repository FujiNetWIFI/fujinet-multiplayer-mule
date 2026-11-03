// Package ai implements the computer players. Land grant, development and
// assay follow the RE description of the original AI; land bidding, declaring
// and trading are marked TODO in the RE and are designed here. See docs/RULES.md.
package ai

import (
	"fujinet-multiplayer-mule/internal/rules"
)

// Brain is the default computer player. One Brain serves every computer seat;
// the seats differ by temperament, so they no longer trade in lockstep.
type Brain struct{}

// temper is how a computer seat trades. [IMPL]
type temper struct {
	reserve  int // extra units of food and energy kept beyond next round's need
	pride    int // % of a good's starting price the store must pay before it sells surplus
	patience int // sixths of the auction it holds out near the store's price before dumping
	markup   int // price steps above the store's price it asks while holding out
}

var tempers = [rules.NumSeats]temper{
	{reserve: 0, pride: 100, patience: 1, markup: 0}, // eager: sells at once
	{reserve: 1, pride: 125, patience: 2, markup: 2}, // careful: keeps a spare, waits for a price
	{reserve: 2, pride: 90, patience: 3, markup: 1},  // cautious: big reserve, still sells
	{reserve: 1, pride: 115, patience: 4, markup: 3}, // proud: holds out longest
}

func temperOf(seat int) temper { return tempers[seat%rules.NumSeats] }

// worthSelling reports whether seat sells surplus gd at the store's price now:
// when the price is at least what its temper wants, or when money is short.
func worthSelling(g *rules.Game, seat int, gd rules.Good) bool {
	if g.Players[seat].Money < 150 {
		return true
	}
	return g.Store.Price[gd]*100 >= rules.StoreInitPrice[gd]*temperOf(seat).pride
}

// keep is how much of gd seat holds back from the auction.
func keep(g *rules.Game, seat int, gd rules.Good) int {
	if gd == rules.Food || gd == rules.Energy {
		return g.Critical(seat, gd) + 1 + temperOf(seat).reserve
	}
	return 0
}

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

// DevTurn performs a computer development turn: development, then assay,
// then the pub. [RE: Computers turn AI]
func (b *Brain) DevTurn(g *rules.Game, seat int) {
	t := g.Turn
	p := g.Players[seat]
	ptu := rules.PTUOf(p.Species)
	left := t.Length / ptu

	// Phase 1: development. Each pass takes the plot whose best use beats
	// its current use by the most (at least 3): an empty plot gets a new
	// M.U.L.E., a working one is outfitted again. Each plot once a turn.
	var done [rules.NumPlots]bool
	for left >= 35 {
		plot, good, gain := -1, rules.NoGood, 2
		for i := rules.NumPlots - 1; i >= 0; i-- {
			pl := &g.Map[i]
			if pl.Owner != seat || done[i] {
				continue
			}
			cost := rules.OutfitCost[0]
			cur := 0
			if pl.Mule == rules.NoGood {
				if g.Store.Mules == 0 || p.Money < g.Store.MulePrice {
					continue
				}
				cost = g.Store.MulePrice
			} else {
				// [IMPL] Our ratings follow prices, so a computer that
				// re-outfitted on any gain of 3 would swing its whole
				// fleet with the market: it needs half again, and keeps
				// its last food and energy M.U.L.E.s.
				if (pl.Mule == rules.Food || pl.Mule == rules.Energy) && countMules(g, seat, pl.Mule) == 1 {
					continue
				}
				cur = goodRating(g, seat, pl.Mule) * capacity(g, seat, i, pl.Mule)
				cost = 0
			}
			gd, v := bestUse(g, seat, i)
			if gd == rules.NoGood || gd == pl.Mule || v-cur <= gain || 2*v < 3*cur {
				continue
			}
			if p.Money < cost+rules.OutfitCost[gd] {
				continue
			}
			plot, good, gain = i, gd, v-cur
		}
		if plot < 0 {
			break
		}
		if !develop(g, seat, plot, good) {
			break
		}
		done[plot] = true
		left -= 25 + 2*rules.StoreDistance[plot]
	}

	// Phase 2: assay (Tournament), own plots then free ones, each one not
	// yet known with a chance of 1 in 8, while 22 PTU are left. Not when
	// it already has more than two High plots.
	high := 0
	for i := range p.Known {
		if p.Known[i] >= 3 && g.Map[i].Owner == seat {
			high++
		}
	}
	for stage := 0; stage < 2 && high <= 2; stage++ {
		for i := rules.NumPlots - 1; i >= 0; i-- {
			pl := &g.Map[i]
			if pl.Terrain == rules.River || pl.Terrain == rules.Town {
				continue
			}
			if (stage == 0 && pl.Owner != seat) || (stage == 1 && pl.Owner >= 0) {
				continue
			}
			if left < 22 {
				break
			}
			if p.Known[i] >= 0 || g.Rng().IntN(8) != 0 {
				continue
			}
			if _, err := g.Assay(seat, i); err == nil {
				deduce(p, i, int(p.Known[i]))
				left -= 10 + 2*rules.StoreDistance[i]
			}
		}
	}

	// Phase 3: the rest at the pub, or out of time.
	left = max(left, 0)
	t.Deadline = t.Begin + left*ptu
	if left >= 3 {
		g.Pub(seat)
	}
}

// develop puts a M.U.L.E. outfitted for good on plot: a new one from the
// corral, or the one already there outfitted again.
func develop(g *rules.Game, seat, plot int, good rules.Good) bool {
	if g.Map[plot].Mule == rules.NoGood {
		if g.BuyMule(seat) != nil {
			return false
		}
	} else if _, err := g.Install(seat, plot); err != nil { // lead it away
		return false
	}
	if g.Outfit(seat, good) != nil {
		g.ReturnMule(seat)
		return false
	}
	if _, err := g.Install(seat, plot); err != nil {
		g.ReturnMule(seat)
		return false
	}
	return true
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
		crit := keep(g, seat, gd)
		if have > crit && worthSelling(g, seat, gd) {
			return rules.RoleSeller
		}
		if have < g.Critical(seat, gd)+1 {
			return rules.RoleBuyer
		}
		return rules.RoleNone
	}
	if have > 0 && worthSelling(g, seat, gd) {
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
	tm := temperOf(seat)
	switch l.Role {
	case rules.RoleSeller:
		l.Floor = keep(g, seat, a.Good)
		best := a.StoreBuy
		for s := range a.Lines {
			o := &a.Lines[s]
			if s != seat && o.Role == rules.RoleBuyer && o.Pos > best && g.Players[s].Money >= o.Pos {
				best = o.Pos
			}
		}
		// Hold out near the store's price for a while before dumping.
		if a.Ticks > rules.TradeTicks*(6-tm.patience)/6 {
			best = max(best, a.StoreSell-(1+tm.markup)*rules.PriceStep[a.Good])
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

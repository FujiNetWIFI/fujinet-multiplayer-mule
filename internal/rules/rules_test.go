package rules

import (
	"math/rand/v2"
	"testing"
)

func TestMapGen(t *testing.T) {
	for seed := uint64(0); seed < 200; seed++ {
		r := rand.New(rand.NewPCG(seed, 1))
		m := GenerateMap(r)
		river, mounts := 0, 0
		for i, p := range m {
			switch p.Terrain {
			case River:
				river++
				if i%MapCols != RiverCol {
					t.Fatalf("river off column at %d", i)
				}
			case Mountain:
				if p.Mountains < 1 || p.Mountains > 3 {
					t.Fatalf("plot %d has %d mountains", i, p.Mountains)
				}
				mounts += p.Mountains
			}
			if p.Crystite < 0 || p.Crystite > 3 {
				t.Fatalf("crystite %d", p.Crystite)
			}
		}
		if river != 4 || m[StorePlot].Terrain != Town {
			t.Fatalf("river=%d store=%v", river, m[StorePlot].Terrain)
		}
		if mounts != 4*MapRows {
			t.Fatalf("mountains=%d want 20", mounts)
		}
		highs := 0
		for i, p := range m {
			if p.Crystite == 3 {
				highs++
				for _, n := range Neighbors4(i) {
					if m[n].Crystite < 2 {
						t.Fatalf("diamond broken next to %d", i)
					}
				}
			}
		}
		if highs == 0 || highs > 4 {
			t.Fatalf("high crystite count %d", highs)
		}
	}
}

func TestNormalDistribution(t *testing.T) {
	r := rand.New(rand.NewPCG(7, 7))
	var h [9]int
	const n = 200000
	for i := 0; i < n; i++ {
		h[NormalOffset(r)+4]++
	}
	if f := float64(h[4]) / n; f < 0.36 || f > 0.395 {
		t.Fatalf("P(0)=%.3f", f)
	}
	if f := float64(h[5]) / n; f < 0.23 || f > 0.255 {
		t.Fatalf("P(+1)=%.3f", f)
	}
}

func testGame() *Game {
	g := &Game{rng: rand.New(rand.NewPCG(1, 2))}
	for i := range g.Map {
		g.Map[i] = Plot{Terrain: Plain, Owner: -1, Mule: NoGood}
	}
	for i := range g.Players {
		g.Players[i] = &Player{Rank: i + 1}
	}
	return g
}

func TestCapacity(t *testing.T) {
	g := testGame()
	g.Map[0] = Plot{Terrain: Plain, Owner: 0, Mule: Energy}
	if c := g.Capacity(0, Energy, 0); c != 3 {
		t.Fatalf("single energy plain = %d want 3", c)
	}
	g.Map[1] = Plot{Terrain: Plain, Owner: 0, Mule: Energy}
	if c := g.Capacity(0, Energy, 0); c != 4 {
		t.Fatalf("adjacent = %d want 4", c)
	}
	g.Map[20] = Plot{Terrain: Plain, Owner: 0, Mule: Energy}
	if c := g.Capacity(0, Energy, 0); c != 5 {
		t.Fatalf("3 plots + adjacency = %d want 5", c)
	}
	g.Map[30] = Plot{Terrain: Mountain, Mountains: 3, Owner: 1, Mule: Smithore}
	if c := g.Capacity(30, Smithore, 1); c != 4 {
		t.Fatalf("3 mountain smithore = %d want 4", c)
	}
	if (&Plot{Terrain: River}).CanInstall(Smithore) {
		t.Fatal("smithore on river")
	}
}

func TestSpoilage(t *testing.T) {
	g := testGame()
	g.Round = 1
	p := g.Players[0]
	p.Goods = [NumGoods]int{11, 9, 60, 3}
	g.applyProduction()
	// food: 11-3=8, spoil 4 -> 4; energy: 9-0=9, spoil 2 -> 7; smithore capped 50.
	if p.Goods != [NumGoods]int{4, 7, 50, 3} {
		t.Fatalf("goods = %v", p.Goods)
	}
}

func TestStorePrices(t *testing.T) {
	s := Store{Price: StoreInitPrice}
	if s.BuyPrice(Food) != 15 || s.SellPrice(Food) != 50 {
		t.Fatalf("food %d/%d", s.BuyPrice(Food), s.SellPrice(Food))
	}
	s.Price[Crystite] = 103
	if s.BuyPrice(Crystite) != 100 || s.SellPrice(Crystite) != 240 {
		t.Fatalf("crystite %d/%d", s.BuyPrice(Crystite), s.SellPrice(Crystite))
	}
	g := testGame()
	g.Store.Price[Smithore] = 57
	g.buildMules()
	if g.Store.MulePrice != 110 {
		t.Fatalf("mule price %d", g.Store.MulePrice)
	}
}

func TestScoreAndRating(t *testing.T) {
	g := testGame()
	g.Store.Price = StoreInitPrice
	g.Players[0].Money = 1000
	g.Players[0].Goods = [NumGoods]int{2, 0, 0, 1}
	g.Map[3] = Plot{Owner: 0, Mule: Food}
	g.Map[4] = Plot{Owner: 0, Mule: NoGood}
	total, _, land, goods := g.ScoreOf(0)
	if land != 500+25+35+500 || goods != 60+100 || total != 1000+land+goods {
		t.Fatalf("score %d land %d goods %d", total, land, goods)
	}
	for _, c := range []struct{ total, want int }{{5000, 0}, {25000, 1}, {61000, 3}, {200000, 6}} {
		if r := ColonyRating(c.total); r != c.want {
			t.Fatalf("rating(%d)=%d want %d", c.total, r, c.want)
		}
	}
}

func TestTurnLengthAndOrder(t *testing.T) {
	g := testGame()
	g.Round = 1
	g.Players[0].Goods[Food] = 3
	if l := g.TurnLengthPTU(0); l != 101 {
		t.Fatalf("full food turn %d", l)
	}
	g.Players[0].Goods[Food] = 0
	if l := g.TurnLengthPTU(0); l != 10 {
		t.Fatalf("no food turn %d", l)
	}
}

func TestPlayerEventGating(t *testing.T) {
	for seed := uint64(0); seed < 500; seed++ {
		g := testGame()
		g.rng = rand.New(rand.NewPCG(seed, 3))
		g.Round = 5
		g.Players[0].Rank = 1
		g.Players[0].Goods[Food] = 5
		id, _ := g.rollPlayerEvent(0)
		if id != 0 && playerEvents[id].good {
			t.Fatalf("leader got good event %d", id)
		}
		g.Players[3].Rank = 4
		g.Players[3].Goods[Food] = 5
		id, _ = g.rollPlayerEvent(3)
		if id != 0 && !playerEvents[id].good {
			t.Fatalf("last place got bad event %d", id)
		}
	}
}

func humanGame(seed uint64, sp Species) *Game {
	var seats [NumSeats]SeatInfo
	seats[0] = SeatInfo{Name: "h", Species: sp}
	for i := 1; i < NumSeats; i++ {
		seats[i] = SeatInfo{AI: true}
	}
	return NewGame(seed, seats, idleBrain{})
}

// runTo advances until cond or fails after max jiffies.
func runTo(t *testing.T, g *Game, max int, cond func() bool) {
	t.Helper()
	for i := 0; i < max; i++ {
		if cond() {
			return
		}
		pressButtons(g)
		g.Advance(g.T + 1)
	}
	t.Fatalf("condition not reached (phase %v round %d)", g.Phase, g.Round)
}

// pressButtons plays a passive human in seat 0: it presses on waiting screens
// and ends its development turns at once.
func pressButtons(g *Game) {
	switch g.Phase {
	case PhaseSummary:
		if g.Elapsed() >= SummaryAnim {
			g.Continue(0)
		}
	case PhaseDevelop:
		if t := g.Turn; t != nil && t.Seat == 0 && !g.Players[0].AI {
			if !t.Started {
				g.BeginTurn(0)
			} else if !t.Ended {
				g.EndTurn(0)
			}
		}
	}
}

// toGrant leaves summary #0 (the human presses its button).
func toGrant(t *testing.T, g *Game) {
	runTo(t, g, SummaryAnim+1, func() bool { return g.Elapsed() >= SummaryAnim })
	g.Continue(0)
	g.Advance(g.T + 1)
	if g.Phase != PhaseLandGrant {
		t.Fatalf("phase %v", g.Phase)
	}
}

func TestLandGrantClaimWindow(t *testing.T) {
	g := humanGame(9, Flapper)
	toGrant(t, g)
	runTo(t, g, 5000, func() bool { return g.GrantCur >= 3 && g.GrantCur < NumPlots })
	cur := g.GrantCur
	if err := g.Claim(0, cur+2); err == nil {
		t.Fatal("claimed plot ahead of cursor")
	}
	if err := g.Claim(0, cur); err != nil {
		t.Fatalf("claim under cursor: %v", err)
	}
	if g.Map[cur].Owner != 0 || g.GrantNext != g.T+GrantClaimPause {
		t.Fatalf("owner %d pause %d", g.Map[cur].Owner, g.GrantNext-g.T)
	}
}

// grantBrain has seat 1 want plot P in the land grant, and once P is gone the
// next free plot. Other computers want nothing.
type grantBrain struct {
	idleBrain
	P int
}

func (b grantBrain) GrantChoice(g *Game, seat, from int) int {
	if seat != 1 {
		return -1
	}
	if b.P >= from && g.grantable(b.P) {
		return b.P
	}
	for p := from; p < NumPlots; p++ {
		if g.grantable(p) {
			return p
		}
	}
	return -1
}

// grantToPlot starts a game where seat 1 (a computer) wants plot p and the other
// computers want nothing, and runs until the cursor lands on p (and seat 1 claims it).
func grantToPlot(t *testing.T, p int) *Game {
	t.Helper()
	g := humanGame(9, Flapper)
	g.SetBrain(grantBrain{P: p})
	toGrant(t, g)
	runTo(t, g, 5000, func() bool { return g.GrantCur >= p })
	if g.GrantCur != p {
		t.Fatalf("cursor %d", g.GrantCur)
	}
	return g
}

func TestLandGrantHumanBumpsAI(t *testing.T) {
	const p = 5
	g := grantToPlot(t, p)
	if g.Map[p].Owner != 1 {
		t.Fatalf("computer did not claim on arrival: owner %d", g.Map[p].Owner)
	}
	g.Advance(g.T + GrantLatency)
	if err := g.Claim(0, p); err != nil {
		t.Fatalf("human claim within the allowance: %v", err)
	}
	if g.Map[p].Owner != 0 || g.Claimed[1] != -1 || g.GrantCur != p {
		t.Fatalf("owner %d computer %d cursor %d", g.Map[p].Owner, g.Claimed[1], g.GrantCur)
	}
	runTo(t, g, 1000, func() bool { return g.Claimed[1] >= 0 })
	if g.Claimed[1] <= p {
		t.Fatalf("computer claimed %d", g.Claimed[1])
	}
}

func TestLandGrantHumanTooLateForAI(t *testing.T) {
	const p = 5
	g := grantToPlot(t, p)
	g.Advance(g.T + GrantLatency + 1)
	if err := g.Claim(0, p); err != ErrPlot {
		t.Fatalf("late human claim: %v", err)
	}
	if g.Map[p].Owner != 1 || g.Claimed[1] != p {
		t.Fatalf("owner %d computer %d", g.Map[p].Owner, g.Claimed[1])
	}
}

func TestLandGrantLateClaim(t *testing.T) {
	g := humanGame(9, Flapper)
	toGrant(t, g)
	runTo(t, g, 5000, func() bool { return g.GrantCur >= 3 && g.GrantCur < NumPlots })
	x := g.GrantCur
	// Run until the cursor has left x and moved two plots on.
	runTo(t, g, 1000, func() bool { st := g.grantStopAt(x); return st != nil && st.left >= 0 && g.T-st.left == 30 })
	if g.GrantCur < x+2 {
		t.Fatalf("cursor %d, from %d", g.GrantCur, x)
	}
	if err := g.Claim(0, x); err != nil {
		t.Fatalf("claim 30 jiffies after the cursor left: %v", err)
	}
	if g.Map[x].Owner != 0 || g.GrantCur != x || g.GrantNext != g.T+GrantClaimPause {
		t.Fatalf("owner %d cursor %d pause %d", g.Map[x].Owner, g.GrantCur, g.GrantNext-g.T)
	}
	// The cursor carries on from x.
	runTo(t, g, 1000, func() bool { return g.GrantCur != x })
	if g.GrantCur <= x || g.GrantCur > x+2 {
		t.Fatalf("cursor went to %d after %d", g.GrantCur, x)
	}
}

func TestLandGrantTooLateClaim(t *testing.T) {
	g := humanGame(9, Flapper)
	toGrant(t, g)
	runTo(t, g, 5000, func() bool { return g.GrantCur >= 3 && g.GrantCur < NumPlots })
	x := g.GrantCur
	runTo(t, g, 1000, func() bool { return g.GrantCur > x })
	g.Advance(g.T + GrantLatency + 1)
	if err := g.Claim(0, x); err != ErrPlot {
		t.Fatalf("claim %d jiffies after the cursor left: %v", GrantLatency+1, err)
	}
}

func TestLandGrantHumanNotBumped(t *testing.T) {
	var seats [NumSeats]SeatInfo
	seats[0] = SeatInfo{Name: "a", Species: Flapper}
	seats[1] = SeatInfo{Name: "b", Species: Flapper}
	for i := 2; i < NumSeats; i++ {
		seats[i] = SeatInfo{AI: true}
	}
	g := NewGame(9, seats, idleBrain{})
	runTo(t, g, SummaryAnim+1, func() bool { return g.Elapsed() >= SummaryAnim })
	g.Continue(0)
	g.Continue(1)
	runTo(t, g, 5000, func() bool { return g.Phase == PhaseLandGrant && g.GrantCur >= 3 && g.GrantCur < NumPlots })
	x := g.GrantCur
	if err := g.Claim(1, x); err != nil {
		t.Fatal(err)
	}
	if err := g.Claim(0, x); err != ErrPlot {
		t.Fatalf("second human claim: %v", err)
	}
	if g.Map[x].Owner != 1 {
		t.Fatalf("owner %d", g.Map[x].Owner)
	}
}

type idleBrain struct{}

func (idleBrain) GrantChoice(g *Game, seat, from int) int { return -1 }
func (idleBrain) DevTurn(g *Game, seat int)               {}
func (idleBrain) Declare(g *Game, seat int, gd Good) int  { return RoleNone }
func (idleBrain) AuctionTick(g *Game, seat int)           {}

func TestHumanTurnAndTrade(t *testing.T) {
	g := humanGame(3, Humanoid)
	toGrant(t, g)
	g.setOwner(0, 0)
	g.Claimed = [NumSeats]int{0, 1, 1, 1}
	runTo(t, g, 20000, func() bool { return g.Phase == PhaseDevelop && g.Turn != nil && g.Turn.Seat == 0 })
	if err := g.BuyMule(0); err != ErrDone {
		t.Fatalf("buy before begin: %v", err)
	}
	must(t, g.BeginTurn(0))
	money := g.Players[0].Money
	must(t, g.BuyMule(0))
	must(t, g.Outfit(0, Food))
	lost, err := g.Install(0, 0)
	if err != nil || lost {
		t.Fatalf("install: %v lost=%v", err, lost)
	}
	if g.Map[0].Mule != Food || g.Players[0].Money != money-100-25 {
		t.Fatalf("mule=%v money=%d", g.Map[0].Mule, g.Players[0].Money)
	}
	g.Advance(g.Turn.Deadline)
	if err := g.BuyMule(0); err == nil {
		t.Fatal("action after deadline accepted")
	}
	runTo(t, g, 40000, func() bool {
		return g.Phase == PhaseTrading && g.Auction.Good == Food && g.Auction.Stage == StageActive
	})
	g.Players[0].Goods[Food] = 10
	a := g.Auction
	a.Lines[0].Role, a.Lines[0].Home, a.Lines[0].Pos = RoleSeller, a.Hi, a.Hi
	must(t, g.Push(0, -1))
	before := g.Players[0].Goods[Food]
	g.Advance(g.T + 600)
	if g.Players[0].Goods[Food] >= before {
		t.Fatalf("no units sold to store: %d", g.Players[0].Goods[Food])
	}
}

func must(t *testing.T, err error) {
	t.Helper()
	if err != nil {
		t.Fatal(err)
	}
}

// A stalled buyer's line stays put and buys nothing; back, it goes on.
func TestStallStopsTrading(t *testing.T) {
	g := humanGame(3, Humanoid)
	toGrant(t, g)
	runTo(t, g, 60000, func() bool {
		return g.Phase == PhaseTrading && g.Auction.Good == Food && g.Auction.Stage == StageActive
	})
	a := g.Auction
	a.Ticks = 1000 // time enough
	g.Store.Stock[Food] = 30
	g.Players[0].Money = 10000
	g.Players[0].Goods[Food] = 0
	a.Lines[0].Role, a.Lines[0].Home, a.Lines[0].Pos, a.Lines[0].Target = RoleBuyer, a.Lo, a.Lo, a.Lo
	for s := 1; s < NumSeats; s++ {
		a.Lines[s].Role = RoleNone
	}
	must(t, g.Push(0, 1))
	runTo(t, g, 3000, func() bool { return g.Players[0].Goods[Food] >= 1 })
	g.Stall(0, true)
	units, pos := g.Players[0].Goods[Food], a.Lines[0].Pos
	g.Advance(g.T + 900)
	if g.Players[0].Goods[Food] != units || a.Lines[0].Pos != pos {
		t.Fatalf("stalled: units %d -> %d, pos %d -> %d", units, g.Players[0].Goods[Food], pos, a.Lines[0].Pos)
	}
	g.Stall(0, false)
	g.Advance(g.T + 900)
	if g.Players[0].Goods[Food] <= units {
		t.Fatalf("no units bought after the stall: %d", g.Players[0].Goods[Food])
	}
}

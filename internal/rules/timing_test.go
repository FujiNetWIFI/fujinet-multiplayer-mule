package rules

import "testing"

// These tests pin the engine to timings measured on the Atari original
// (docs/TIMING.md). All values are jiffies (1/60 s).

// pushBrain is a computer that does nothing except what the test asks.
type pushBrain struct{ idleBrain }

func TestTimeUnits(t *testing.T) {
	cases := map[Species]int{Mechtron: 28, Gollumer: 28, Flapper: 36, Humanoid: 20}
	for sp, want := range cases {
		if got := PTUOf(sp); got != want {
			t.Errorf("PTU %v = %d, want %d", sp, got, want)
		}
	}
	if CTU != 12 || ATU != 25 || ATUIdle != 6 {
		t.Fatalf("CTU %d ATU %d idle %d", CTU, ATU, ATUIdle)
	}
	// Measured: a full-food Gollumer turn lasts 2828 jiffies.
	if l := (TurnBaseTime + TurnFoodTime) * PTUOf(Gollumer); l != 2828 {
		t.Fatalf("turn %d", l)
	}
}

func TestGrantCursorTiming(t *testing.T) {
	g := humanGame(11, Gollumer)
	toGrant(t, g)
	start := g.T
	runTo(t, g, 3000, func() bool { return g.GrantCur >= 0 })
	// Round 1: landing + intro between the button press and the first step.
	if d := g.T - start; d != LandingDelay+GrantIntro {
		t.Fatalf("first step after %d, want %d", d, LandingDelay+GrantIntro)
	}
	// Steps every CTU, skipping owned plots and the store without using time.
	prev, prevT := g.GrantCur, g.T
	for i := 0; i < 20 && g.GrantEnd < 0; i++ {
		runTo(t, g, 200, func() bool { return g.GrantCur != prev })
		d := g.T - prevT
		if d != CTU && d != GrantClaimPause {
			t.Fatalf("cursor %d->%d after %d jiffies", prev, g.GrantCur, d)
		}
		for p := prev + 1; p < g.GrantCur && p < NumPlots; p++ {
			if g.grantable(p) {
				t.Fatalf("cursor skipped free plot %d", p)
			}
		}
		prev, prevT = g.GrantCur, g.T
	}
}

// landAuction runs to an active colony land auction with idle computers.
func landAuction(t *testing.T) *Game {
	g := humanGame(4, Gollumer)
	toGrant(t, g)
	g.Claimed = [NumSeats]int{0, 0, 0, 0}
	g.ColonyLeft = 1
	runTo(t, g, 5000, func() bool { return g.Phase == PhaseLandAuction })
	if g.Auction.Stage != StageShow {
		t.Fatalf("stage %d", g.Auction.Stage)
	}
	return g
}

func TestForSaleDisplayAndIdleAuction(t *testing.T) {
	g := landAuction(t)
	start := g.T
	runTo(t, g, 2000, func() bool { return g.Auction.Stage == StageActive })
	if d := g.T - start; d != ForSaleDisplay {
		t.Fatalf("for-sale display %d, want %d", d, ForSaleDisplay)
	}
	// Nobody pushing: 1 full tick then 69 idle ticks.
	start = g.T
	runTo(t, g, 3000, func() bool { return g.Auction.Stage == StageDone })
	if d := g.T - start; d != ATU+69*ATUIdle {
		t.Fatalf("idle land auction %d, want %d", d, ATU+69*ATUIdle)
	}
	start = g.T
	runTo(t, g, 1000, func() bool { return g.Phase != PhaseLandAuction || g.Auction.Stage != StageDone })
	if d := g.T - start; d != AuctionEndDisplay {
		t.Fatalf("result display %d", d)
	}
}

func TestForSaleSkippedByButton(t *testing.T) {
	g := landAuction(t)
	g.Advance(g.T + 30)
	g.Continue(0)
	g.Advance(g.T + 1)
	if g.Auction.Stage != StageActive {
		t.Fatal("button did not skip the for-sale display")
	}
}

func TestActiveAuctionAndLineSpeed(t *testing.T) {
	g := landAuction(t)
	g.Continue(0)
	g.Advance(g.T + 1)
	a := g.Auction
	must(t, g.Push(0, 1))
	start, pos := g.T, a.Lines[0].Pos
	g.Advance(g.T + 10*LineStep)
	if steps := (a.Lines[0].Pos - pos) / a.Step; steps < 9 || steps > 10 {
		t.Fatalf("line moved %d steps in %d jiffies", steps, 10*LineStep)
	}
	// Holding the stick keeps the timer at the slow rate for the whole auction.
	runTo(t, g, 3000, func() bool { return a.Stage == StageDone })
	// The first ticks run idle until the bid crosses the start price (7 steps). [measured]
	if d := g.T - start; d < 65*ATU || d > 70*ATU+1 {
		t.Fatalf("active land auction %d, want ~%d", d, 70*ATU)
	}
	// Releasing a land bid drifts it back down.
	g2 := landAuction(t)
	g2.Continue(0)
	g2.Advance(g2.T + 1)
	must(t, g2.Push(0, 1))
	g2.Advance(g2.T + 30*LineStep)
	high := g2.Auction.Lines[0].Pos
	must(t, g2.Push(0, 0))
	g2.Advance(g2.T + 5*LineStep)
	if g2.Auction.Lines[0].Pos >= high {
		t.Fatal("released land bid did not drift down")
	}
}

func TestGoodsAuctionTiming(t *testing.T) {
	g := humanGame(6, Gollumer)
	toGrant(t, g)
	runTo(t, g, 60000, func() bool { return g.Phase == PhaseDeclare })
	start := g.T
	runTo(t, g, 2000, func() bool { return g.Auction.Stage == StageDeclare })
	if d := g.T - start; d != GoodsStatus {
		t.Fatalf("status screen %d", d)
	}
	start = g.T
	runTo(t, g, 2000, func() bool { return g.Auction.Stage == StageLead })
	if d := g.T - start; d != DeclareTicks*ATUIdle {
		t.Fatalf("declare %d, want 180", d)
	}
	start = g.T
	runTo(t, g, 2000, func() bool { return g.Phase == PhaseTrading })
	if d := g.T - start; d != DeclareLead {
		t.Fatalf("lead %d", d)
	}
}

func TestTradeCadence(t *testing.T) {
	g := humanGame(6, Gollumer)
	toGrant(t, g)
	runTo(t, g, 60000, func() bool { return g.Phase == PhaseTrading })
	a := g.Auction
	g.Players[0].Money = 10000
	g.Store.Stock[a.Good] = 20
	a.Lines[0].Role, a.Lines[0].Home, a.Lines[0].Pos = RoleBuyer, a.Lo, a.StoreSell
	a.Lines[0].Target = a.Hi
	var times []int
	last := g.Players[0].Goods[a.Good]
	for i := 0; i < 400 && len(times) < 8; i++ {
		g.Advance(g.T + 1)
		if n := g.Players[0].Goods[a.Good]; n != last {
			times = append(times, g.T)
			last = n
		}
	}
	want := []int{48, 40, 32, 24, 16, 8, 8}
	if len(times) < 8 {
		t.Fatalf("only %d units", len(times))
	}
	for i, w := range want {
		if d := times[i+1] - times[i]; d != w {
			t.Fatalf("unit %d after %d jiffies, want %d (times %v)", i+2, d, w, times)
		}
	}
}

func TestComputerTurnDuration(t *testing.T) {
	g := humanGame(8, Gollumer)
	toGrant(t, g)
	runTo(t, g, 30000, func() bool { return g.Phase == PhaseDevelop })
	if !g.Players[g.Turn.Seat].AI {
		return
	}
	tu := g.Turn
	// idleBrain uses no time: the turn lasts its full length, then the display.
	if d := tu.EndAt - tu.Begin; d != tu.Length+TurnOverDisplay {
		t.Fatalf("computer turn %d, want %d", d, tu.Length+TurnOverDisplay)
	}
}

func TestLinesStayOnScale(t *testing.T) {
	g := landAuction(t)
	g.Continue(0)
	g.Advance(g.T + 1)
	a := g.Auction
	a.Lines[1].Target = a.Hi * 3 // a computer asking for too much
	g.Advance(g.T + 400*LineStep)
	if a.Lines[1].Pos > a.Hi {
		t.Fatalf("line at %d above the top %d", a.Lines[1].Pos, a.Hi)
	}
}

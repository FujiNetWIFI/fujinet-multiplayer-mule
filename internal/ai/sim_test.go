package ai

import (
	"testing"

	"fujinet-multiplayer-mule/internal/rules"
)

func allAI() [rules.NumSeats]rules.SeatInfo {
	var s [rules.NumSeats]rules.SeatInfo
	for i := range s {
		s[i] = rules.SeatInfo{Name: "cpu", AI: true}
	}
	return s
}

func runGame(t *testing.T, seed uint64) *rules.Game {
	g := rules.NewGame(seed, allAI(), New())
	for step := 0; g.Phase != rules.PhaseGameOver; step++ {
		if step > 2_000_000 {
			t.Fatalf("seed %d: game did not finish (round %d phase %s)", seed, g.Round, g.Phase)
		}
		stop, claimed := grantStop(g), g.Claimed
		g.Advance(g.T + 1)
		checkInvariants(t, g)
		checkGrantStop(t, g, stop, claimed)
	}
	return g
}

func grantStop(g *rules.Game) int {
	if g.Phase != rules.PhaseLandGrant {
		return -1
	}
	return g.GrantStop()
}

// checkGrantStop checks a computer only claims where GrantStop said the cursor
// would next stop, and that the stop is never behind the cursor.
func checkGrantStop(t *testing.T, g *rules.Game, stop int, claimed [rules.NumSeats]int) {
	t.Helper()
	if g.Phase != rules.PhaseLandGrant || stop < 0 {
		return
	}
	for seat, c := range g.Claimed {
		if c >= 0 && claimed[seat] < 0 && c != stop {
			t.Fatalf("round %d: seat %d claimed %d, GrantStop said %d", g.Round, seat, c, stop)
		}
	}
	if s := g.GrantStop(); g.GrantEnd < 0 && s <= g.GrantCur {
		t.Fatalf("round %d: GrantStop %d not past the cursor %d", g.Round, s, g.GrantCur)
	}
}

func checkInvariants(t *testing.T, g *rules.Game) {
	t.Helper()
	owned := 0
	for _, pl := range g.Map {
		if pl.Owner >= 0 {
			owned++
		}
	}
	if owned > rules.NumPlots-1 {
		t.Fatalf("too many owned plots: %d", owned)
	}
	for i, p := range g.Players {
		if p.Money < 0 {
			t.Fatalf("seat %d negative money %d (round %d phase %s)", i, p.Money, g.Round, g.Phase)
		}
		for gd, n := range p.Goods {
			if n < 0 || n > rules.MaxStock {
				t.Fatalf("seat %d good %d = %d", i, gd, n)
			}
		}
	}
	for gd, n := range g.Store.Stock {
		if n < 0 {
			t.Fatalf("store good %d = %d", gd, n)
		}
	}
	if g.Store.Mules < 0 || g.Store.Mules > rules.MaxMules {
		t.Fatalf("store mules %d", g.Store.Mules)
	}
}

func TestSimulateGames(t *testing.T) {
	for seed := uint64(1); seed <= 20; seed++ {
		g := runGame(t, seed)
		if !g.Failed && g.Round != rules.NumRounds {
			t.Fatalf("seed %d ended in round %d", seed, g.Round)
		}
		mules := 0
		for _, pl := range g.Map {
			if pl.Mule != rules.NoGood {
				mules++
			}
		}
		t.Logf("seed %d: round %d failed=%v colony=%d rating=%d mules=%d scores=%d/%d/%d/%d prices=%v",
			seed, g.Round, g.Failed, g.ColonyTotal(), g.Rating, mules,
			g.Players[0].Score, g.Players[1].Score, g.Players[2].Score, g.Players[3].Score, g.Store.Price)
	}
}

func TestDeterministic(t *testing.T) {
	a, b := runGame(t, 42), runGame(t, 42)
	for i := range a.Players {
		if a.Players[i].Score != b.Players[i].Score {
			t.Fatalf("seat %d score differs %d vs %d", i, a.Players[i].Score, b.Players[i].Score)
		}
	}
	if a.T != b.T {
		t.Fatalf("clock differs")
	}
}

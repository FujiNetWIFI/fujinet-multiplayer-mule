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
		g.Advance(g.T + 1)
		checkInvariants(t, g)
	}
	return g
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

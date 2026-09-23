package rules

import "math/rand/v2"

// GenerateMap builds a Tournament map. [RE: Map generation]
func GenerateMap(r *rand.Rand) [NumPlots]Plot {
	var m [NumPlots]Plot
	for i := range m {
		m[i] = Plot{Terrain: Plain, Owner: -1, Mule: NoGood}
		if i%MapCols == RiverCol {
			m[i].Terrain = River
		}
	}
	m[StorePlot].Terrain = Town

	for row := 0; row < MapRows; row++ {
		left := row*MapCols + r.IntN(RiverCol)
		right := row*MapCols + RiverCol + 1 + r.IntN(MapCols-RiverCol-1)
		var n [2]int
		for q := 0; q < 4; q++ {
			side := r.IntN(2)
			if n[side] == 3 {
				side ^= 1
			}
			n[side]++
		}
		for side, p := range [2]int{left, right} {
			if n[side] > 0 {
				m[p].Terrain = Mountain
				m[p].Mountains = n[side]
			}
		}
	}

	// Crystite: 4 high deposits spreading in diamonds.
	for k := 0; k < 4; k++ {
		c := r.IntN(NumPlots)
		cr, cc := PlotRC(c)
		for p := 0; p < NumPlots; p++ {
			pr, pc := PlotRC(p)
			d := abs(pr-cr) + abs(pc-cc)
			if d <= 2 && 3-d > m[p].Crystite {
				m[p].Crystite = 3 - d
			}
		}
	}
	return m
}

func abs(x int) int {
	if x < 0 {
		return -x
	}
	return x
}

// CanInstall reports whether good g may be produced on plot p.
func (p *Plot) CanInstall(g Good) bool {
	switch p.Terrain {
	case Town:
		return false
	case River:
		return g == Food || g == Energy
	}
	return true
}

// BaseYield returns base production of g on the plot. [RE: Production]
func (p *Plot) BaseYield(g Good) int {
	switch p.Terrain {
	case Plain:
		return [NumGoods]int{2, 3, 1, p.Crystite}[g]
	case River:
		return [NumGoods]int{4, 2, 0, 0}[g]
	case Mountain:
		return [NumGoods]int{1, 1, 1 + p.Mountains, p.Crystite}[g]
	}
	return 0
}

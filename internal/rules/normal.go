package rules

import "math/rand/v2"

// normalWeights is the distribution of the offset produced by the game's
// Normal(N, amplitude 2) generator, per 100000. [RE: Number variation with
// normal distribution] Index 0 is offset -4, index 8 is offset +4.
var normalWeights = [9]int{13, 562, 6248, 24303, 37748, 24303, 6248, 562, 13}

// NormalOffset returns a random offset in [-4, 4] with the Tournament
// (amplitude 2) distribution.
func NormalOffset(r *rand.Rand) int {
	x := r.IntN(100000)
	for i, w := range normalWeights {
		if x < w {
			return i - 4
		}
		x -= w
	}
	return 0
}

// Normal returns n plus a Tournament normal offset.
func Normal(r *rand.Rand, n int) int { return n + NormalOffset(r) }

func clamp(v, lo, hi int) int {
	if v < lo {
		return lo
	}
	if v > hi {
		return hi
	}
	return v
}

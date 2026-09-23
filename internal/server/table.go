// Package server exposes M.U.L.E. tables over HTTP, following the conventions
// of the FujiNet game servers (fujinet-game-system): every route answers GET
// and POST, takes ?table=&player=, and returns JSON, raw (?raw=1) or binary (?bin=1).
package server

import (
	"fmt"
	"strings"
	"sync"
	"time"

	"fujinet-multiplayer-mule/internal/ai"
	"fujinet-multiplayer-mule/internal/proto"
	"fujinet-multiplayer-mule/internal/rules"
)

// Timeouts (package vars so tests can shorten them).
var (
	PlayerPingTimeout = 2 * time.Minute  // human seat handed to the AI
	LobbyPingTimeout  = 30 * time.Second // human removed from the lobby
	AbandonTimeout    = 5 * time.Minute  // no humans left: table resets
	GameOverLinger    = 60 * time.Second
)

type seat struct {
	name     string
	species  rules.Species
	human    bool
	ready    bool
	lastPing time.Time
}

// Table is one game room.
type Table struct {
	mu        sync.Mutex
	ID        string
	Name      string
	MaxHumans int
	Register  bool // register with the FujiNet lobby
	seats     [rules.NumSeats]seat
	game      *rules.Game
	epoch     time.Time // wall time of game BTU 0
	countFrom time.Time // lobby countdown start
	allReady  time.Time
	overAt    time.Time
	lastHuman time.Time
	seed      uint64
}

func newTable(id, name string, maxHumans int, register bool) *Table {
	t := &Table{ID: id, Name: name, MaxHumans: maxHumans, Register: register}
	t.resetLobby(time.Time{})
	return t
}

func (t *Table) resetLobby(now time.Time) {
	t.game = nil
	t.countFrom, t.allReady, t.overAt = time.Time{}, time.Time{}, time.Time{}
	for i := range t.seats {
		s := &t.seats[i]
		if !s.human || now.Sub(s.lastPing) > LobbyPingTimeout {
			*s = seat{name: fmt.Sprintf("cpu%d", i+1), species: rules.Mechtron}
		}
		s.ready = false
	}
}

func (t *Table) humans() int {
	n := 0
	for _, s := range t.seats {
		if s.human {
			n++
		}
	}
	return n
}

// jiffy returns the game clock (1/60 s units) at wall time now.
func (t *Table) jiffy(now time.Time) int {
	return int(now.Sub(t.epoch) * rules.JiffiesPerSecond / time.Second)
}

// seatOf returns the seat for player name, joining if possible. -1 = spectator.
func (t *Table) seatOf(name string, now time.Time, join bool) int {
	if name == "" {
		return -1
	}
	for i := range t.seats {
		if t.seats[i].human && strings.EqualFold(t.seats[i].name, name) {
			t.ping(i, now)
			return i
		}
	}
	if !join || t.game != nil || t.humans() >= t.MaxHumans {
		return -1
	}
	for i := range t.seats {
		s := &t.seats[i]
		if !s.human {
			*s = seat{name: truncName(name), species: rules.Species(1 + (i*2)%int(rules.NumSpecies-1)), human: true, lastPing: now}
			t.countFrom, t.allReady = now, time.Time{}
			t.lastHuman = now
			return i
		}
	}
	return -1
}

func truncName(n string) string {
	if len(n) > 8 {
		return n[:8]
	}
	return n
}

func (t *Table) ping(i int, now time.Time) {
	s := &t.seats[i]
	s.lastPing = now
	t.lastHuman = now
	if t.game != nil && t.game.Players[i].AI && t.game.Players[i].Human {
		t.game.SetAI(i, false) // player came back
	}
}

// update advances the table to now: lobby countdown, game clock, drops.
// Returns true when the lobby should be notified.
func (t *Table) update(now time.Time) (changed bool) {
	if t.game == nil {
		for i := range t.seats {
			if t.seats[i].human && now.Sub(t.seats[i].lastPing) > LobbyPingTimeout {
				t.seats[i] = seat{name: fmt.Sprintf("cpu%d", i+1), species: rules.Mechtron}
				changed = true
			}
		}
		if t.humans() == 0 {
			t.countFrom = time.Time{}
			return
		}
		if t.countdown(now) == 0 {
			t.start(now)
			changed = true
		}
		return
	}
	t.game.Advance(t.jiffy(now))
	humans := false
	for i := range t.seats {
		s := &t.seats[i]
		if !s.human {
			continue
		}
		online := now.Sub(s.lastPing) <= PlayerPingTimeout
		if online {
			humans = true
		}
		if !online && !t.game.Players[i].AI {
			t.game.SetAI(i, true)
			changed = true
		}
	}
	if t.game.Phase == rules.PhaseGameOver {
		if t.overAt.IsZero() {
			t.overAt = now
			changed = true
		}
		if now.Sub(t.overAt) > GameOverLinger {
			t.resetLobby(now)
			changed = true
		}
	} else if !humans && now.Sub(t.lastHuman) > AbandonTimeout {
		t.resetLobby(now)
		changed = true
	}
	return
}

// countdown returns jiffies until the game starts, or -1 if not counting.
func (t *Table) countdown(now time.Time) int {
	if t.game != nil || t.countFrom.IsZero() {
		return -1
	}
	all := true
	for _, s := range t.seats {
		if s.human && !s.ready {
			all = false
		}
	}
	d := time.Duration(rules.LobbyCountdown) * time.Second / rules.JiffiesPerSecond
	from := t.countFrom
	if all {
		if t.allReady.IsZero() {
			t.allReady = now
		}
		rd := time.Duration(rules.LobbyReady) * time.Second / rules.JiffiesPerSecond
		if t.allReady.Add(rd).Before(from.Add(d)) {
			from, d = t.allReady, rd
		}
	} else {
		t.allReady = time.Time{}
	}
	left := from.Add(d).Sub(now)
	if left <= 0 {
		return 0
	}
	return int(left*rules.JiffiesPerSecond/time.Second) + 1
}

func (t *Table) start(now time.Time) {
	var si [rules.NumSeats]rules.SeatInfo
	for i, s := range t.seats {
		si[i] = rules.SeatInfo{Name: s.name, Species: s.species, AI: !s.human}
	}
	t.seed++
	seed := uint64(now.UnixNano()) ^ t.seed<<32
	t.game = rules.NewGame(seed, si, ai.New())
	t.epoch = now
	t.overAt = time.Time{}
}

// view builds the proto table for serialization.
func (t *Table) view(now time.Time, me int) *proto.View {
	pt := &proto.Table{ID: t.ID, Name: t.Name, Game: t.game, Countdown: t.countdown(now)}
	for i, s := range t.seats {
		pt.Lobby[i] = proto.LobbySeat{Name: s.name, Species: s.species, Human: s.human, Ready: s.ready}
		pt.Connected[i] = s.human && now.Sub(s.lastPing) <= PlayerPingTimeout
	}
	return proto.BuildView(pt, me)
}

// leave removes a human from the table.
func (t *Table) leave(i int) {
	if i < 0 {
		return
	}
	if t.game == nil {
		t.seats[i] = seat{name: fmt.Sprintf("cpu%d", i+1), species: rules.Mechtron}
		return
	}
	t.seats[i].lastPing = time.Time{}
	t.game.SetAI(i, true)
}

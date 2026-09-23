package proto

import (
	"bytes"
	"encoding/binary"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"testing"

	"fujinet-multiplayer-mule/internal/rules"
)

type nopBrain struct{}

func (nopBrain) GrantChoice(g *rules.Game, seat, from int) int { return -1 }
func (nopBrain) DevTurn(g *rules.Game, seat int)               {}
func (nopBrain) Declare(g *rules.Game, seat int, gd rules.Good) int {
	return rules.RoleNone
}
func (nopBrain) AuctionTick(g *rules.Game, seat int) {}

func sampleView() *View {
	var seats [rules.NumSeats]rules.SeatInfo
	seats[0] = rules.SeatInfo{Name: "alice", Species: rules.Flapper}
	for i := 1; i < rules.NumSeats; i++ {
		seats[i] = rules.SeatInfo{Name: "cpu", AI: true}
	}
	g := rules.NewGame(5, seats, nopBrain{})
	return BuildView(&Table{ID: "t", Game: g}, 0)
}

func TestBinaryLayout(t *testing.T) {
	sizes := map[string]struct {
		v any
		n int
	}{
		"header": {BinHeader{}, 16}, "player": {BinPlayer{}, 29}, "map": {BinMap{}, 225},
		"store": {BinStore{}, 31}, "grant": {BinGrant{}, 8}, "auction": {BinAuction{}, 58},
		"turn": {BinTurn{}, 86}, "colony": {BinColony{}, 10}, "state": {BinState{}, 550}, "table": {BinTable{}, 33},
	}
	for name, c := range sizes {
		if n := binary.Size(c.v); n != c.n {
			t.Errorf("%s size %d, want %d (update docs/PROTOCOL.md and bump Version)", name, n, c.n)
		}
	}
}

func TestBinaryRoundTrip(t *testing.T) {
	v := sampleView()
	for _, be := range []bool{false, true} {
		b := Encode(ToBin(v), be)
		var order binary.ByteOrder = binary.LittleEndian
		if be {
			order = binary.BigEndian
		}
		var st BinState
		if err := binary.Read(bytes.NewReader(b), order, &st); err != nil {
			t.Fatal(err)
		}
		if st.H.Magic != 'M' || st.H.Phase != uint8(rules.PhaseSummary) || st.Players[0].Money != 1600 {
			t.Fatalf("be=%v header %+v money %d", be, st.H, st.Players[0].Money)
		}
		for i := range v.Map.Owner {
			if int(st.Map.Owner[i]) != v.Map.Owner[i] || int(st.Map.Terrain[i]&3) != v.Map.Terrain[i] {
				t.Fatalf("plot %d mismatch", i)
			}
		}
	}
}

func TestFlatten(t *testing.T) {
	got := Flatten(`{"a":1,"b":[true,"x"],"c":{"d":null}}`, "\n", false, false)
	want := "a\n1\nb/0\n1\nb/1\nx\nc/d\n\n"
	if got != want {
		t.Fatalf("got %q want %q", got, want)
	}
}

// TestCHeader checks the generated header is current and, when a C compiler
// is available, that sizeof() matches the Go layout.
func TestCHeader(t *testing.T) {
	h := CHeader()
	disk, err := os.ReadFile("../../clients/mule_proto.h")
	if err != nil || string(disk) != h {
		t.Fatal("clients/mule_proto.h is stale: run `go run ./cmd/muleserver -cheader > clients/mule_proto.h`")
	}
	cc, err := exec.LookPath("cc")
	if err != nil {
		t.Skip("no C compiler")
	}
	dir := t.TempDir()
	os.WriteFile(filepath.Join(dir, "mule_proto.h"), []byte(h), 0o644)
	src := `#include <stdio.h>
#include "mule_proto.h"
int main(void){printf("%d %d %d", (int)sizeof(MuleState), (int)sizeof(MuleAuction), (int)sizeof(MuleTable));return 0;}`
	os.WriteFile(filepath.Join(dir, "t.c"), []byte(src), 0o644)
	exe := filepath.Join(dir, "t")
	if out, err := exec.Command(cc, "-o", exe, filepath.Join(dir, "t.c")).CombinedOutput(); err != nil {
		t.Fatalf("compile: %v\n%s", err, out)
	}
	out, err := exec.Command(exe).Output()
	if err != nil {
		t.Fatal(err)
	}
	if strings.TrimSpace(string(out)) != "550 58 33" {
		t.Fatalf("C sizes %s", out)
	}
}

// firstBrain has every computer take the first free plot in the land grant.
type firstBrain struct{ nopBrain }

func (firstBrain) GrantChoice(g *rules.Game, seat, from int) int { return from }

func TestGrantCursorGoneAfterSweep(t *testing.T) {
	var seats [rules.NumSeats]rules.SeatInfo
	seats[0] = rules.SeatInfo{Name: "alice", Species: rules.Flapper}
	for i := 1; i < rules.NumSeats; i++ {
		seats[i] = rules.SeatInfo{Name: "cpu", AI: true}
	}
	g := rules.NewGame(5, seats, firstBrain{})
	tb := &Table{ID: "t", Game: g}
	g.Advance(g.T + rules.SummaryAnim + 1)
	g.Continue(0)
	claimed := false
	for i := 0; i < 5000 && (g.Phase == rules.PhaseLandGrant || g.Phase == rules.PhaseSummary); i++ {
		v := BuildView(tb, 0)
		if gr := v.Grant; gr != nil {
			if gr.Ended {
				if gr.Cursor != rules.NumPlots || ToBin(v).Grant.Cursor != rules.NumPlots {
					t.Fatalf("cursor %d after the sweep", gr.Cursor)
				}
				return
			}
			if !claimed && gr.Cursor >= 0 && g.Claim(0, gr.Cursor) == nil {
				claimed = true
			}
		}
		g.Advance(g.T + 1)
	}
	t.Fatalf("sweep did not end (phase %v)", g.Phase)
}

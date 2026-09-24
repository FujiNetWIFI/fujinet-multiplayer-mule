package server

import (
	"bytes"
	"encoding/binary"
	"net/http"
	"net/http/httptest"
	"strconv"
	"strings"
	"testing"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/goccy/go-json"

	"fujinet-multiplayer-mule/internal/proto"
	"fujinet-multiplayer-mule/internal/rules"
)

type harness struct {
	t   *testing.T
	s   *Server
	r   *gin.Engine
	now time.Time
}

func newHarness(t *testing.T) *harness {
	gin.SetMode(gin.TestMode)
	h := &harness{t: t, now: time.Unix(1_800_000_000, 0)}
	h.s = New(nil)
	h.s.Now = func() time.Time { return h.now }
	h.r = gin.New()
	h.s.Routes(h.r)
	return h
}

func (h *harness) get(path string) []byte {
	w := httptest.NewRecorder()
	h.r.ServeHTTP(w, httptest.NewRequest(http.MethodGet, path, nil))
	if w.Code != http.StatusOK {
		h.t.Fatalf("%s: status %d", path, w.Code)
	}
	return w.Body.Bytes()
}

func (h *harness) view(path string) *proto.View {
	var v proto.View
	if err := json.Unmarshal(h.get(path), &v); err != nil {
		h.t.Fatalf("%s: %v", path, err)
	}
	return &v
}

// advance moves the clock forward, ticking the tables like the server loop.
func (h *harness) advance(d time.Duration) {
	for step := 100 * time.Millisecond; d > 0; d -= step {
		h.now = h.now.Add(step)
		h.s.tickAll()
	}
}

func TestJoinSetupAndStart(t *testing.T) {
	h := newHarness(t)
	v := h.view("/state?table=dev1&player=alice")
	if v.Me != 0 || v.Phase != "lobby" || !v.Players[0].Online {
		t.Fatalf("join: me=%d phase=%s", v.Me, v.Phase)
	}
	v = h.view("/setup/species/flapper?table=dev1&player=alice")
	if v.Players[0].Species != int(rules.Flapper) || v.Err != 0 {
		t.Fatalf("species=%d err=%d", v.Players[0].Species, v.Err)
	}
	v = h.view("/state?table=dev1&player=bob")
	if v.Me != 1 {
		t.Fatalf("bob seat %d", v.Me)
	}
	h.view("/ready/1?table=dev1&player=alice")
	h.view("/ready/1?table=dev1&player=bob")
	h.advance(7 * time.Second)
	v = h.view("/state?table=dev1&player=alice")
	if v.Phase != "summary" || v.Round != 0 {
		t.Fatalf("not started: %s", v.Phase)
	}
	// Status summary #0 waits for every human's button.
	h.view("/ready/1?table=dev1&player=alice")
	h.advance(8 * time.Second)
	if v = h.view("/state?table=dev1&player=alice"); v.Phase != "summary" {
		t.Fatalf("summary ended before all pressed: %s", v.Phase)
	}
	h.view("/ready/1?table=dev1&player=bob")
	h.advance(200 * time.Millisecond)
	v = h.view("/state?table=dev1&player=alice")
	if v.Phase != "grant" || v.Round != 1 {
		t.Fatalf("not in grant: %s", v.Phase)
	}
	if v.Players[0].Money != 1600 || v.Players[2].Money != 1200 || !v.Players[2].AI {
		t.Fatalf("money %d / %d", v.Players[0].Money, v.Players[2].Money)
	}
	// Late joiners spectate.
	if v := h.view("/state?table=dev1&player=carol"); v.Me != -1 {
		t.Fatalf("carol seat %d", v.Me)
	}
}

func startSolo(h *harness, table string) *proto.View {
	h.view("/state?table=" + table + "&player=solo")
	h.view("/ready/1?table=" + table + "&player=solo")
	h.advance(7 * time.Second)
	h.advance(7 * time.Second) // summary #0 animation
	h.view("/ready/1?table=" + table + "&player=solo")
	h.advance(200 * time.Millisecond)
	return h.view("/state?table=" + table + "&player=solo")
}

func TestGrantClaimAndErrors(t *testing.T) {
	h := newHarness(t)
	v := startSolo(h, "dev2")
	if v.Grant == nil {
		t.Fatal("no grant block")
	}
	// Claim ahead of the cursor fails with ERR_PLOT.
	v = h.view("/claim/44?table=dev2&player=solo")
	if v.Err != proto.ErrPlot {
		t.Fatalf("err=%d", v.Err)
	}
	// Wait for the cursor to reach a free plot and claim it.
	for i := 0; i < 400; i++ {
		h.advance(50 * time.Millisecond)
		v = h.view("/state?table=dev2&player=solo")
		if v.Grant == nil {
			t.Fatal("grant ended without claim")
		}
		c := v.Grant.Cursor
		if c >= 0 && c < rules.NumPlots && c != rules.StorePlot && v.Map.Owner[c] < 0 {
			v = h.view("/claim/" + strconv.Itoa(c) + "?table=dev2&player=solo")
			if v.Err != 0 {
				t.Fatalf("claim %d: %s", c, v.ErrText)
			}
			if v.Map.Owner[c] != 0 {
				t.Fatal("plot not owned")
			}
			return
		}
	}
	t.Fatal("cursor never reached a free plot")
}

func TestBinaryAndRaw(t *testing.T) {
	h := newHarness(t)
	startSolo(h, "dev3")
	b := h.get("/state?table=dev3&player=solo&bin=1")
	if len(b) != binary.Size(proto.BinState{}) {
		t.Fatalf("binary size %d", len(b))
	}
	var st proto.BinState
	if err := binary.Read(bytes.NewReader(b), binary.LittleEndian, &st); err != nil {
		t.Fatal(err)
	}
	if st.H.Magic != 'M' || st.H.Version != proto.Version || st.H.Phase != uint8(rules.PhaseLandGrant) || st.H.MySeat != 0 {
		t.Fatalf("header %+v", st.H)
	}
	if string(bytes.TrimRight(st.Players[0].Name[:], "\x00")) != "solo" || st.Players[0].Money != 1000 {
		t.Fatalf("player %+v", st.Players[0])
	}
	if st.Map.Terrain[rules.StorePlot]&3 != uint8(rules.Town) {
		t.Fatal("store terrain")
	}
	be := h.get("/state?table=dev3&player=solo&bin=1&be=1")
	if be[16+9+3] != 0x03 || be[16+9+3+1] != 0xE8 { // money 1000 big-endian
		t.Fatalf("big endian money % x", be[28:30])
	}
	raw := string(h.get("/state?table=dev3&player=solo&raw=1&lf=1"))
	if !strings.Contains(raw, "pl/0/n\nsolo\n") || !strings.Contains(raw, "ph\ngrant\n") {
		t.Fatalf("raw: %.200s", raw)
	}
	tb := h.get("/tables?bin=1")
	if tb[0] != 7 || len(tb) != 1+7*33 {
		t.Fatalf("tables bin len %d count %d", len(tb), tb[0])
	}
}

func TestHashUnchanged(t *testing.T) {
	h := newHarness(t)
	h.get("/state?table=m1&player=a")
	w := httptest.NewRecorder()
	h.r.ServeHTTP(w, httptest.NewRequest(http.MethodGet, "/state?table=m1&player=a&hash=0", nil))
	hash := w.Header().Get("X-Mule-Hash")
	if hash == "" {
		t.Fatal("no hash header")
	}
	body := h.get("/state?table=m1&player=a&hash=" + hash)
	if !bytes.Contains(body, []byte(`"same":1`)) {
		t.Fatalf("expected same: %s", body)
	}
}

func TestDropHandsSeatToAIAndGameCompletes(t *testing.T) {
	h := newHarness(t)
	v := startSolo(h, "ai3")
	if v.Phase != "grant" {
		t.Fatalf("phase %s", v.Phase)
	}
	h.advance(PlayerPingTimeout + time.Second)
	v = h.view("/view?table=ai3")
	if !v.Players[0].AI {
		t.Fatal("dropped player not taken over by AI")
	}
	// Rejoin restores control.
	v = h.view("/state?table=ai3&player=solo")
	if v.Me != 0 || v.Players[0].AI {
		t.Fatal("rejoin failed")
	}
	h.get("/leave?table=ai3&player=solo")
	defer func(d time.Duration) { AbandonTimeout = d }(AbandonTimeout)
	AbandonTimeout = 24 * time.Hour // let the computers finish the game
	for i := 0; i < 400; i++ {
		h.advance(10 * time.Second)
		if v = h.view("/view?table=ai3"); v.Phase == "gameover" {
			if v.Colony == nil || v.Colony.RatingText == "" {
				t.Fatal("no final rating")
			}
			return
		}
	}
	t.Fatalf("game did not finish: round %d phase %s", v.Round, v.Phase)
}

func TestPushRouteWithSeen(t *testing.T) {
	h := newHarness(t)
	startSolo(h, "dev2")
	// Both forms route to the auction stick (no auction yet: a phase error).
	for _, p := range []string{"/auction/push/down", "/auction/push/down/3", "/auction/push/up/0"} {
		if v := h.view(p + "?table=dev2&player=solo"); v.Err != proto.ErrPhase {
			t.Fatalf("%s: err=%d", p, v.Err)
		}
	}
}

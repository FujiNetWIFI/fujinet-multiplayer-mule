package server

import (
	"bufio"
	"bytes"
	"encoding/binary"
	"io"
	"net"
	"testing"
	"time"

	"fujinet-multiplayer-mule/internal/proto"
	"fujinet-multiplayer-mule/internal/rules"
)

func TestCOBSAndFrames(t *testing.T) {
	cases := [][]byte{{}, {0}, {0, 0}, {1, 2, 0, 3}, bytes.Repeat([]byte{7}, 300), append(bytes.Repeat([]byte{7}, 254), 0, 1)}
	for _, c := range cases {
		enc := proto.COBSEncode(c)
		if bytes.IndexByte(enc, 0) >= 0 {
			t.Fatalf("%v: 0 in %v", c, enc)
		}
		dec, ok := proto.COBSDecode(enc)
		if !ok || !bytes.Equal(dec, c) {
			t.Fatalf("%v -> %v -> %v", c, enc, dec)
		}
	}
	f := proto.Frame(proto.CSPatch, []byte{0, 0, 9, 0, 8})
	raw, _ := proto.COBSDecode(f[:len(f)-1])
	typ, p, ok := proto.Unframe(raw)
	if !ok || typ != proto.CSPatch || !bytes.Equal(p, []byte{0, 0, 9, 0, 8}) {
		t.Fatal("frame round trip")
	}
	raw[2] ^= 1
	if _, _, ok := proto.Unframe(raw); ok {
		t.Fatal("bad sum passed")
	}
}

func TestPatches(t *testing.T) {
	prev := make([]byte, 550)
	cur := append([]byte(nil), prev...)
	cur[3], cur[5], cur[400] = 1, 1, 1
	runs := proto.Patches(prev, cur)
	if len(runs) != 2 || runs[0] != [2]int{3, 6} || runs[1] != [2]int{400, 401} {
		t.Fatalf("runs %v", runs)
	}
	all := proto.Patches(nil, cur)
	got := make([]byte, 550)
	for _, r := range all {
		if r[1]-r[0] > proto.StreamMaxPatch {
			t.Fatalf("run too long %v", r)
		}
		copy(got[r[0]:], cur[r[0]:r[1]])
	}
	if !bytes.Equal(got, cur) {
		t.Fatal("full patch set")
	}
}

// streamClient plays an 8-bit client: a staged copy, commits, credit.
type streamClient struct {
	t      *testing.T
	c      net.Conn
	br     *bufio.Reader
	stage  [550]byte
	st     [550]byte
	taken  uint8
	ring   int // bytes the server may have outstanding (checked against the window)
	tables [][]byte
	errc   byte
}

func dialStream(t *testing.T, addr string) *streamClient {
	c, err := net.Dial("tcp", addr)
	if err != nil {
		t.Fatal(err)
	}
	t.Cleanup(func() { c.Close() })
	return &streamClient{t: t, c: c, br: bufio.NewReader(c)}
}

func (sc *streamClient) send(typ byte, p ...byte) {
	if _, err := sc.c.Write(proto.Frame(typ, p)); err != nil {
		sc.t.Fatal(err)
	}
}

func (sc *streamClient) hello(table, player string) {
	p := []byte{proto.StreamVersion, 0}
	p = append(p, proto.FixedString(table, 8)...)
	p = append(p, proto.FixedString(player, 8)...)
	sc.send(proto.SCHello, p...)
}

// next reads frames until one of type want; it returns its payload.
func (sc *streamClient) next(want byte) []byte {
	sc.c.SetReadDeadline(time.Now().Add(3 * time.Second))
	var buf []byte
	for {
		b, err := sc.br.ReadByte()
		if err != nil {
			sc.t.Fatalf("waiting for %c: %v", want, err)
		}
		sc.taken++
		sc.ring++
		if sc.ring > proto.StreamWindow {
			sc.t.Fatalf("server overran the window: %d", sc.ring)
		}
		if b != 0 {
			buf = append(buf, b)
			continue
		}
		raw, ok := proto.COBSDecode(buf)
		buf = nil
		if !ok {
			sc.t.Fatal("bad COBS")
		}
		typ, p, ok := proto.Unframe(raw)
		if !ok {
			sc.t.Fatal("bad sum")
		}
		if len(raw) > proto.StreamMaxFrame {
			sc.t.Fatalf("frame %d bytes", len(raw))
		}
		// give the credit back at once, as a client draining every frame does
		sc.ring = 0
		sc.send(proto.SCCredit, sc.taken)
		switch typ {
		case proto.CSPatch:
			off := int(p[0]) | int(p[1])<<8
			copy(sc.stage[off:], p[2:])
		case proto.CSCommit:
			sc.st = sc.stage
		case proto.CSTable:
			if p[1] > 0 {
				sc.tables = append(sc.tables, p[2:])
			}
		case proto.CSError:
			sc.errc = p[0]
		}
		if typ == want {
			return p
		}
	}
}

func (sc *streamClient) state() *proto.BinState {
	var b proto.BinState
	if err := binary.Read(bytes.NewReader(sc.st[:]), binary.LittleEndian, &b); err != nil {
		sc.t.Fatal(err)
	}
	return &b
}

func startStream(t *testing.T) (*harness, string) {
	h := newHarness(t)
	ln, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		t.Fatal(err)
	}
	t.Cleanup(func() { ln.Close() })
	go h.s.ServeStream(ln)
	return h, ln.Addr().String()
}

func TestStreamJoinActAndPush(t *testing.T) {
	h, addr := startStream(t)
	c := dialStream(t, addr)

	c.send(proto.SCTables)
	p := c.next(proto.CSTable)
	for int(p[0])+1 < int(p[1]) {
		p = c.next(proto.CSTable)
	}
	if len(c.tables) != 1 || string(bytes.TrimRight(c.tables[0][:8], "\x00")) != DefaultTable {
		t.Fatalf("tables %q", c.tables)
	}

	c.hello("dev1", "ALICE")
	if seq := c.next(proto.CSCommit)[0]; seq != 0 {
		t.Fatalf("first commit acks %d", seq)
	}
	st := c.state()
	if st.H.Magic != 'M' || st.H.MySeat != 0 {
		t.Fatalf("header %+v", st.H)
	}
	// what the HTTP poll says, byte for byte (but the clock)
	want := h.get("/state?table=dev1&player=ALICE&bin=1")
	if !bytes.Equal(want[16:], c.st[16:]) {
		t.Fatal("stream state differs from /state")
	}

	// an action is answered by a commit carrying its seq
	c.send(proto.SCAct, 7, proto.ActSpecies, 2, 0, 0)
	for {
		if seq := c.next(proto.CSCommit)[0]; seq == 7 {
			break
		}
	}
	if st := c.state(); st.Players[0].Species != 2 || st.H.Err != 0 {
		t.Fatalf("species %d err %d", st.Players[0].Species, st.H.Err)
	}
	// a refused action carries its error
	c.send(proto.SCAct, 8, proto.ActClaim, 3, 0, 0)
	for {
		if seq := c.next(proto.CSCommit)[0]; seq == 8 {
			break
		}
	}
	if st := c.state(); st.H.Err != proto.ErrPhase {
		t.Fatalf("claim in lobby err %d", st.H.Err)
	}

	// someone else joining over HTTP is pushed
	h.view("/state?table=dev1&player=bob")
	deadline := time.Now().Add(3 * time.Second)
	for string(bytes.TrimRight(c.state().Players[1].Name[:], "\x00")) != "bob" {
		if time.Now().After(deadline) {
			t.Fatal("bob never pushed")
		}
		c.next(proto.CSCommit)
	}
}

func TestStreamUnknownTableAndResync(t *testing.T) {
	_, addr := startStream(t)
	c := dialStream(t, addr)
	c.hello("nosuch", "ALICE")
	if e := c.next(proto.CSError); e[0] != proto.ErrBadRequest {
		t.Fatalf("err %d", e[0])
	}
	c.hello("dev2", "ALICE")
	c.next(proto.CSCommit)
	before := c.st
	c.stage = [550]byte{}
	c.send(proto.SCResync)
	for i := 0; ; i++ {
		c.next(proto.CSCommit) // one may have been on its way already
		if bytes.Equal(before[16:], c.st[16:]) {
			break
		}
		if i == 3 {
			t.Fatal("resync did not resend everything")
		}
	}
}

func TestStreamWindowHeld(t *testing.T) {
	// a client that gives no credit gets no more than a window
	_, addr := startStream(t)
	c, err := net.Dial("tcp", addr)
	if err != nil {
		t.Fatal(err)
	}
	defer c.Close()
	p := []byte{proto.StreamVersion, 0}
	p = append(p, proto.FixedString("dev3", 8)...)
	p = append(p, proto.FixedString("ZED", 8)...)
	c.Write(proto.Frame(proto.SCHello, p))
	time.Sleep(300 * time.Millisecond)
	c.SetReadDeadline(time.Now().Add(200 * time.Millisecond))
	buf := make([]byte, 2048)
	n := 0
	for {
		k, err := c.Read(buf[n:])
		n += k
		if err != nil {
			break
		}
	}
	if n == 0 || n > proto.StreamWindow {
		t.Fatalf("got %d bytes without credit", n)
	}
}

func TestStreamActionParams(t *testing.T) {
	cases := []struct {
		id        byte
		arg, seen int
		want      pmap
	}{
		{proto.ActPush, 1, 3, pmap{"dir": "up", "seen": "3"}},
		{proto.ActPush, 2, 0, pmap{"dir": "down", "seen": "0"}},
		{proto.ActPush, 0, 5, pmap{"dir": "hold", "seen": "5"}},
		{proto.ActDeclare, 2, 0, pmap{"role": "s"}},
		{proto.ActDeclare, 0, 0, pmap{"role": "n"}},
		{proto.ActOutfit, 3, 0, pmap{"good": "3"}},
		{proto.ActReady, 0, 0, pmap{"v": "1"}},
	}
	for _, c := range cases {
		f, p := streamAction(c.id, c.arg, c.seen)
		if f == nil {
			t.Fatalf("act %d: no action", c.id)
		}
		for k, v := range c.want {
			if p.Param(k) != v {
				t.Fatalf("act %d: %s=%q, want %q", c.id, k, p.Param(k), v)
			}
		}
	}
	if f, _ := streamAction(proto.ActState, 0, 0); f != nil {
		t.Fatal("ACT_STATE is a poll")
	}
}

// Leaving mid-game hands the seat to the computer for good: the pushes that
// follow must not take it back. HELLO under the same name does.
func TestStreamLeaveInGame(t *testing.T) {
	h, addr := startStream(t)
	startSolo(h, "dev1")
	c := dialStream(t, addr)
	c.hello("dev1", "solo")
	c.next(proto.CSCommit)
	if st := c.state(); st.H.MySeat != 0 {
		t.Fatalf("seat %d", st.H.MySeat)
	}
	c.send(proto.SCAct, 5, proto.ActLeave, 0, 0, 0)
	for {
		if seq := c.next(proto.CSCommit)[0]; seq == 5 {
			break
		}
	}
	if st := c.state(); st.H.MySeat != -1 {
		t.Fatalf("after leave seat %d", st.H.MySeat)
	}
	time.Sleep(5 * StreamPush) // pushes run
	if v := h.view("/view?table=dev1"); !v.Players[0].AI {
		t.Fatal("seat taken back after leave")
	}
	c.hello("dev1", "solo")
	deadline := time.Now().Add(3 * time.Second)
	for c.state().H.MySeat != 0 {
		if time.Now().After(deadline) {
			t.Fatal("HELLO did not rejoin")
		}
		c.next(proto.CSCommit)
	}
	if v := h.view("/view?table=dev1"); v.Players[0].AI {
		t.Fatal("rejoined seat still AI")
	}
}

// auctionView is a goods auction in its active stage at jiffy t.
func auctionView(t int, pos, target [4]int) *proto.View {
	a := &proto.AuctionV{Stage: rules.StageActive, Step: 1, Crit: -1, Ticks: 50, TickLeft: 10}
	for i := range 4 {
		a.Lines = append(a.Lines, proto.LineV{Role: rules.RoleBuyer, Pos: pos[i], Target: target[i]})
	}
	return &proto.View{T: t, Phase: "trading", PhaseID: int(rules.PhaseTrading), Auction: a}
}

// Lines walking to their targets as the client walks them are clocks: no
// push before StreamClockOnly. A new target, or a line held back, goes out.
func TestStreamPredictedLines(t *testing.T) {
	sc := &streamConn{}
	v0 := auctionView(800, [4]int{40, 40, 40, 40}, [4]int{60, 40, 40, 40})
	sc.queueState(v0, false, 0)
	sc.q = nil

	v1 := auctionView(824, [4]int{43, 40, 40, 40}, [4]int{60, 40, 40, 40}) // 3 steps, 24 jiffies
	v1.Auction.TickLeft = 11                                               // 10 then 25: 24 jiffies on is 11
	v1.Auction.Ticks = 49
	if !predicted(v0, v1) {
		t.Fatal("walking line not predicted")
	}
	sc.queueState(v1, false, 0)
	if len(sc.q) != 0 {
		t.Fatalf("pushed a predicted change: %d frames", len(sc.q))
	}

	v2 := auctionView(824, [4]int{42, 40, 40, 40}, [4]int{60, 40, 40, 40}) // held back a step
	v2.Auction.TickLeft, v2.Auction.Ticks = 11, 49
	sc.queueState(v2, false, 0)
	if len(sc.q) == 0 {
		t.Fatal("held-back line not pushed")
	}
	sc.q = nil

	v3 := auctionView(824, [4]int{42, 40, 40, 40}, [4]int{60, 50, 40, 40}) // a new target
	v3.Auction.TickLeft, v3.Auction.Ticks = 11, 49
	sc.queueState(v3, false, 0)
	if len(sc.q) == 0 {
		t.Fatal("new target not pushed")
	}

	v4 := auctionView(832, [4]int{43, 41, 40, 40}, [4]int{60, 50, 40, 40})
	v4.Auction.TickLeft, v4.Auction.Ticks = 11, 49 // the bar stopped (a trade in contact)
	if predicted(v3, v4) {
		t.Fatal("stopped time bar predicted")
	}
}

// A client that is behind gets no new state built, but an answer to its
// action still is.
func TestStreamBehindWaits(t *testing.T) {
	h := newHarness(t)
	a, b := net.Pipe()
	t.Cleanup(func() { a.Close(); b.Close() })
	go io.Copy(io.Discard, b)
	sc := &streamConn{s: h.s, nc: a, table: h.s.Table("dev1"), player: "ALICE"}
	sc.sent, sc.taken = proto.StreamWindow/2+10, 0
	if err := sc.push(); err != nil {
		t.Fatal(err)
	}
	if sc.last != nil || len(sc.q) != 0 {
		t.Fatal("built a state for a client that is behind")
	}
	sc.ack, sc.ackSeq = h.s.apply(sc.table, "ALICE", nil, nil), 9
	if err := sc.push(); err != nil {
		t.Fatal(err)
	}
	if sc.last == nil {
		t.Fatal("answer held back")
	}
}

// A client owing credit and silent isn't counted as heard from by the
// pushes made for it; one reading is. Credit for more than was sent (bytes
// a client counted as lost) doesn't wedge the window.
func TestStreamQuietNotPinged(t *testing.T) {
	h := newHarness(t)
	a, b := net.Pipe()
	t.Cleanup(func() { a.Close(); b.Close() })
	go io.Copy(io.Discard, b)
	tb := h.s.Table("dev1")
	h.s.apply(tb, "ALICE", nil, nil) // seated
	sc := &streamConn{s: h.s, nc: a, table: tb, player: "ALICE", lastIn: h.now}
	ping := func() time.Time {
		tb.mu.Lock()
		defer tb.mu.Unlock()
		return tb.seats[tb.seatNamed("ALICE")].lastPing
	}
	h.now = h.now.Add(2 * time.Second)
	sc.sent, sc.taken = 60, 0 // owes credit, silent 2 s
	if err := sc.push(); err != nil {
		t.Fatal(err)
	}
	if ping().Equal(h.now) {
		t.Fatal("a client not reading was counted as heard from")
	}
	sc.q = nil
	sc.handle(proto.SCCredit, []byte{sc.sent})
	h.now = h.now.Add(100 * time.Millisecond)
	if err := sc.push(); err != nil {
		t.Fatal(err)
	}
	if !ping().Equal(h.now) {
		t.Fatal("a reading client wasn't counted as heard from")
	}
	sc.sent = 10
	sc.handle(proto.SCCredit, []byte{20})
	if sc.taken != sc.sent {
		t.Fatalf("over-credit kept: sent %d taken %d", sc.sent, sc.taken)
	}
}

// A slow link (HELLO flag bit 1) gets a push at most every StreamSlowHold;
// an answer goes at once.
func TestStreamSlowHold(t *testing.T) {
	h := newHarness(t)
	a, b := net.Pipe()
	t.Cleanup(func() { a.Close(); b.Close() })
	go io.Copy(io.Discard, b)
	tb := h.s.Table("dev1")
	sc := &streamConn{s: h.s, nc: a, table: tb, player: "ALICE", slow: true, lastIn: h.now}
	drain := func() { // the client takes all that is queued
		for {
			sc.mu.Lock()
			sc.taken = sc.sent
			empty := len(sc.q) == 0
			sc.mu.Unlock()
			if empty {
				return
			}
			if err := sc.push(); err != nil {
				t.Fatal(err)
			}
		}
	}
	if err := sc.push(); err != nil {
		t.Fatal(err)
	}
	if sc.last == nil {
		t.Fatal("no first state")
	}
	drain()
	first := sc.last
	h.s.apply(tb, "BOB", nil, nil) // a change: Bob sits down
	if err := sc.push(); err != nil {
		t.Fatal(err)
	}
	if !bytes.Equal(sc.last, first) {
		t.Fatal("a slow link was pushed within the hold")
	}
	sc.lastSend = time.Now().Add(-StreamSlowHold)
	if err := sc.push(); err != nil {
		t.Fatal(err)
	}
	if bytes.Equal(sc.last, first) {
		t.Fatal("not pushed once the hold was over")
	}
	drain()
	sc.ack, sc.ackSeq = h.s.apply(tb, "ALICE", nil, nil), 9
	if err := sc.push(); err != nil {
		t.Fatal(err)
	}
	if sc.ack != nil || sc.sent == sc.taken {
		t.Fatal("an answer was held")
	}
}

// An action's seq sent again (a client whose write failed, not knowing
// whether it went) is answered again, not applied again.
func TestStreamDupAct(t *testing.T) {
	_, addr := startStream(t)
	c := dialStream(t, addr)
	c.hello("dev1", "ALICE")
	c.next(proto.CSCommit)
	c.send(proto.SCAct, 7, proto.ActSpecies, 2, 0, 0)
	for c.next(proto.CSCommit)[0] != 7 {
	}
	c.send(proto.SCAct, 7, proto.ActSpecies, 3, 0, 0) // the same seq, not the same arg
	for c.next(proto.CSCommit)[0] != 7 {
	}
	if st := c.state(); st.Players[0].Species != 2 {
		t.Fatalf("a repeated seq was applied: species %d", st.Players[0].Species)
	}
	c.send(proto.SCAct, 8, proto.ActSpecies, 3, 0, 0)
	for c.next(proto.CSCommit)[0] != 8 {
	}
	if st := c.state(); st.Players[0].Species != 3 {
		t.Fatalf("the next seq wasn't applied: species %d", st.Players[0].Species)
	}
}

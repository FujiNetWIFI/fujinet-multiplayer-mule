package server

// Stream mode (proto/stream.go, docs/PROTOCOL.md): one TCP connection per
// client, for FujiNet netstream. The connection is a /state poll that never
// ends: each push is what a poll would have returned, sent as patches to
// the state the client already has, and only when something changed.

import (
	"bufio"
	"bytes"
	"log"
	"net"
	"os"
	"strconv"
	"sync"
	"time"

	"fujinet-multiplayer-mule/internal/proto"
	"fujinet-multiplayer-mule/internal/rules"
)

// Stream timing (package vars so tests can change them).
var (
	StreamPush      = 50 * time.Millisecond  // how often to look for changes
	StreamClockOnly = 500 * time.Millisecond // a change of the clocks alone waits this long
	StreamSlowHold  = 300 * time.Millisecond // a slow link (HELLO): any push but an answer waits this long
	StreamSlowClock = time.Second            // ... and the clocks alone this long
	StreamIdle      = 35 * time.Second       // no frame from the client: drop it
)

// streamDebug logs every frame (MULE_STREAM_DEBUG=1).
var streamDebug = os.Getenv("MULE_STREAM_DEBUG") != ""

// ServeStream accepts stream clients on ln until it fails.
func (s *Server) ServeStream(ln net.Listener) error {
	for {
		c, err := ln.Accept()
		if err != nil {
			return err
		}
		go s.handleStream(c)
	}
}

type streamConn struct {
	s  *Server
	nc net.Conn

	pollMu   sync.Mutex // a push's poll against a leave: the poll must not use the old name
	mu       sync.Mutex
	table    *Table
	player   string
	be       bool
	slow     bool        // HELLO flags bit 1: a slow link (MS-DOS at 9600 bps)
	lastSeq  byte        // the action last applied, and its answer: a repeat
	lastAck  *proto.View // (a client whose write failed) is answered, not applied
	last     []byte      // the state the client has once q is out
	steady   []byte      // last without the clocks
	lastV    *proto.View // the view last sent (what the client predicts from)
	lastSend time.Time
	q        [][]byte    // frames waiting for credit
	sent     uint8       // encoded bytes sent, mod 256
	taken    uint8       // bytes the client took from its ring, mod 256 (SC_CREDIT)
	lastIn   time.Time   // a frame from the client (s.Now())
	ack      *proto.View // an action's answer, to go out next
	ackSeq   byte
	kick     chan struct{}
	done     chan struct{}
}

func (s *Server) handleStream(nc net.Conn) {
	sc := &streamConn{s: s, nc: nc, kick: make(chan struct{}, 1), done: make(chan struct{})}
	defer nc.Close()
	go sc.writer()
	sc.reader()
	close(sc.done)
}

func (sc *streamConn) poke() {
	select {
	case sc.kick <- struct{}{}:
	default:
	}
}

// reader takes frames apart and handles them.
func (sc *streamConn) reader() {
	br := bufio.NewReader(sc.nc)
	var buf []byte
	for {
		sc.nc.SetReadDeadline(time.Now().Add(StreamIdle))
		c, err := br.ReadByte()
		if err != nil {
			return
		}
		if c != 0 {
			if len(buf) < 2*proto.StreamMaxFrame {
				buf = append(buf, c)
			}
			continue
		}
		raw, ok := proto.COBSDecode(buf)
		if streamDebug {
			log.Printf("stream %s: in % x", sc.nc.RemoteAddr(), buf)
		}
		buf = buf[:0]
		if !ok {
			continue // a REGISTER preamble, or noise
		}
		typ, p, ok := proto.Unframe(raw)
		if !ok {
			if streamDebug {
				log.Printf("stream %s: bad sum", sc.nc.RemoteAddr())
			}
			continue
		}
		if !sc.handle(typ, p) {
			return
		}
	}
}

func cstr(b []byte) string {
	if i := bytes.IndexByte(b, 0); i >= 0 {
		b = b[:i]
	}
	return string(b)
}

// handle one frame; false ends the connection.
func (sc *streamConn) handle(typ byte, p []byte) bool {
	s := sc.s
	sc.mu.Lock()
	sc.lastIn = s.Now()
	sc.mu.Unlock()
	switch typ {
	case proto.SCHello:
		if len(p) < 18 {
			return false
		}
		id := cstr(p[2:10])
		if id == "" {
			id = DefaultTable
		}
		t := s.Table(id)
		sc.mu.Lock()
		sc.table, sc.player, sc.be, sc.slow = t, cstr(p[10:18]), p[1]&1 != 0, p[1]&2 != 0
		sc.last, sc.steady, sc.lastV, sc.q = nil, nil, nil, nil
		sc.lastSeq, sc.lastAck = 0, nil
		if t == nil {
			sc.q = append(sc.q, proto.Frame(proto.CSError, []byte{byte(proto.ErrBadRequest)}))
		}
		sc.mu.Unlock()
	case proto.SCTables:
		list := s.tableList(len(p) > 0 && p[0] == 1)
		sc.mu.Lock()
		if len(list) == 0 {
			sc.q = append(sc.q, proto.Frame(proto.CSTable, []byte{0, 0}))
		}
		for i, ti := range list {
			sc.q = append(sc.q, proto.Frame(proto.CSTable, append([]byte{byte(i), byte(len(list))}, binTable(ti)...)))
		}
		sc.mu.Unlock()
	case proto.SCAct:
		if len(p) < 5 {
			return false
		}
		sc.mu.Lock()
		t, player := sc.table, sc.player
		sc.mu.Unlock()
		if t == nil {
			return true
		}
		sc.mu.Lock()
		again := p[0] == sc.lastSeq && sc.lastAck != nil
		if again {
			sc.ack, sc.ackSeq = sc.lastAck, p[0]
		}
		sc.mu.Unlock()
		if again {
			break // sent again because its FujiNet's answer was lost: not twice
		}
		var v *proto.View
		if p[1] == proto.ActLeave {
			sc.pollMu.Lock()
			t.mu.Lock()
			seat := t.seatOf(player, s.Now(), false)
			t.leave(seat)
			t.mu.Unlock()
			s.notifyLobby(t)
			// Watch from now on: the pushes must not carry the name, or the
			// first would take the seat back (seatOf). HELLO rejoins.
			sc.mu.Lock()
			if sc.player == player {
				sc.player = ""
			}
			sc.mu.Unlock()
			sc.pollMu.Unlock()
			v = s.apply(t, "", nil, nil)
		} else {
			f, ps := streamAction(p[1], int(p[2])|int(p[3])<<8, int(p[4]))
			v = s.apply(t, player, f, ps)
		}
		sc.mu.Lock()
		sc.ack, sc.ackSeq = v, p[0]
		sc.lastSeq, sc.lastAck = p[0], v
		sc.mu.Unlock()
	case proto.SCResync:
		sc.mu.Lock()
		sc.last, sc.steady, sc.lastV, sc.q = nil, nil, nil, nil
		sc.mu.Unlock()
	case proto.SCBye:
		sc.mu.Lock()
		t, player := sc.table, sc.player
		sc.mu.Unlock()
		if t != nil {
			t.mu.Lock()
			t.leave(t.seatOf(player, s.Now(), false))
			t.mu.Unlock()
			s.notifyLobby(t)
		}
		return false
	case proto.SCCredit:
		if len(p) > 0 {
			sc.mu.Lock()
			sc.taken = p[0]
			if int(sc.sent-sc.taken) > proto.StreamWindow {
				sc.taken = sc.sent // credit for more than was sent: a client
				// that counted bytes it lost; don't wedge the window
			}
			sc.mu.Unlock()
		}
	case proto.SCPing:
	default:
		return true
	}
	sc.poke()
	return true
}

// streamAction is SC_ACT's action and parameters (the HTTP route's).
func streamAction(id byte, arg, seen int) (action, params) {
	n := strconv.Itoa(arg)
	switch id {
	case proto.ActReady:
		return actReady, pmap{"v": "1"}
	case proto.ActUnready:
		return actReady, pmap{"v": "0"}
	case proto.ActSpecies:
		return actSpecies, pmap{"s": n}
	case proto.ActClaim:
		return actClaim, pmap{"plot": n}
	case proto.ActPush:
		dir := [3]string{"hold", "up", "down"}[min(arg, 2)]
		if arg > 2 {
			dir = "hold"
		}
		return actBidMove, pmap{"dir": dir, "seen": strconv.Itoa(seen)}
	case proto.ActHold:
		return actBidMove, pmap{"dir": "hold", "seen": strconv.Itoa(seen), "at": n}
	case proto.ActDeclare:
		return actDeclare, pmap{"role": [3]string{"n", "b", "s"}[min(arg, 2)]}
	case proto.ActCollude:
		return actCollude, nil
	case proto.ActBegin:
		return actBegin, nil
	case proto.ActBuyMule:
		return actBuyMule, nil
	case proto.ActOutfit:
		return actOutfit, pmap{"good": n}
	case proto.ActInstall:
		return actInstall, pmap{"plot": n}
	case proto.ActReturn:
		return actReturn, nil
	case proto.ActAssay:
		return actAssay, pmap{"plot": n}
	case proto.ActSell:
		return actSell, pmap{"plot": n}
	case proto.ActUnsell:
		return actUnsell, pmap{"plot": n}
	case proto.ActWampus:
		return actWampus, nil
	case proto.ActPub:
		return actPub, nil
	case proto.ActEnd:
		return actEnd, nil
	}
	return nil, nil // ACT_STATE, or unknown: a poll
}

// writer pushes: on each tick or poke, the next state if nothing is
// queued and the client isn't behind, then as much of the queue as the
// credit allows.
func (sc *streamConn) writer() {
	tk := time.NewTicker(StreamPush)
	defer tk.Stop()
	for {
		select {
		case <-sc.done:
			return
		case <-tk.C:
		case <-sc.kick:
		}
		if err := sc.push(); err != nil {
			sc.nc.Close()
			return
		}
	}
}

func (sc *streamConn) push() error {
	sc.pollMu.Lock()
	sc.mu.Lock()
	t, player, ack, seq := sc.table, sc.player, sc.ack, sc.ackSeq
	idle := len(sc.q) == 0
	// A client still holding half a window unread is behind (drawing, or a
	// slow line): a state built now would be stale when it got there, so
	// the next one is built once it has caught up. An answer goes at once.
	behind := int(sc.sent-sc.taken) > proto.StreamWindow/2
	// Reading: a client owing credit (it sends it every 48 bytes taken) and
	// silent a while has stopped taking bytes, and isn't heard from, so a
	// held auction push stops (Table.stallQuiet).
	reading := int(sc.sent-sc.taken) < 48 || sc.s.Now().Sub(sc.lastIn) < PushStale/2
	sc.mu.Unlock()
	if idle && t != nil && (ack != nil || !behind) {
		v := ack
		if v == nil {
			v = sc.s.applyAs(t, player, nil, nil, reading) // what /state would say
		}
		sc.mu.Lock()
		if ack != nil && sc.ack == ack {
			sc.ack = nil
		}
		sc.queueState(v, ack != nil, seq)
		sc.mu.Unlock()
	}
	sc.pollMu.Unlock() // not held through the write: it may wait out a slow client

	sc.mu.Lock()
	inflight := int(sc.sent - sc.taken) // the window is < 256: mod 256 is enough
	var out []byte
	for len(sc.q) > 0 && inflight+len(out)+len(sc.q[0]) <= proto.StreamWindow {
		out = append(out, sc.q[0]...)
		sc.q = sc.q[1:]
	}
	sc.sent += uint8(len(out))
	sc.mu.Unlock()
	if len(out) == 0 {
		return nil
	}
	if streamDebug {
		log.Printf("stream %s: out %d bytes, in flight %d", sc.nc.RemoteAddr(), len(out), inflight+len(out))
	}
	sc.nc.SetWriteDeadline(time.Now().Add(StreamIdle))
	_, err := sc.nc.Write(out)
	return err
}

// queueState queues v as patches and a commit, if the client needs it:
// always for an action's answer, else when something besides the clocks
// changed, or the clocks did a while ago. Auction lines and the auction
// time bar count as clocks while they go the way the client works out on
// its own from the last state (predicted). A slow link (HELLO) gets any
// push but an answer at most every StreamSlowHold: each one costs it a
// blocking read, and it animates between them from what it has. sc.mu held.
func (sc *streamConn) queueState(v *proto.View, isAck bool, ackSeq byte) {
	cur := proto.Encode(proto.ToBin(v), sc.be)
	std := proto.Encode(proto.ToBin(streamSteady(v)), sc.be)
	now := time.Now()
	if !isAck && sc.last != nil {
		if bytes.Equal(cur, sc.last) {
			return
		}
		var hold time.Duration // a change goes at once
		clocks := StreamClockOnly
		if sc.slow {
			hold, clocks = StreamSlowHold, StreamSlowClock
		}
		if bytes.Equal(std, sc.steady) && predicted(sc.lastV, v) {
			hold = clocks
		}
		if hold > 0 && now.Sub(sc.lastSend) < hold {
			return
		}
	}
	for _, r := range proto.Patches(sc.last, cur) {
		p := []byte{byte(r[0]), byte(r[0] >> 8)}
		sc.q = append(sc.q, proto.Frame(proto.CSPatch, append(p, cur[r[0]:r[1]]...)))
	}
	var seq byte
	if isAck {
		seq = ackSeq
	}
	sc.q = append(sc.q, proto.Frame(proto.CSCommit, []byte{seq}))
	sc.last, sc.steady, sc.lastV, sc.lastSend = cur, std, v, now
}

// streamSteady is steady without the auction's line positions and time bar
// (predicted decides whether those still go as the client expects).
func streamSteady(v *proto.View) *proto.View {
	cp := steady(v)
	if cp.Auction != nil {
		ac := *cp.Auction
		ac.Ticks = 0
		ac.Lines = append([]proto.LineV(nil), ac.Lines...)
		for i := range ac.Lines {
			ac.Lines[i].Pos = 0
		}
		cp.Auction = &ac
	}
	return cp
}

// predicted reports whether cur's auction lines and time bar are where a
// client that got prev puts them by itself: each line a price step toward
// its target on every LineStep jiffy of the active stage (the critical
// seller's stays put), and the bar a tick every ATU jiffies (ATUIdle when
// idle or declaring). A line held back (a ceiling, a trade) or a bar
// stopped by a trade is not, and goes out at once.
func predicted(prev, cur *proto.View) bool {
	if prev == nil {
		return false
	}
	pa, ca := prev.Auction, cur.Auction
	if pa == nil || ca == nil {
		return pa == ca
	}
	if pa.Stage != ca.Stage || len(pa.Lines) != len(ca.Lines) || cur.T < prev.T {
		return false
	}
	dt := cur.T - prev.T
	moving := ca.Stage == rules.StageActive
	steps := 0
	if moving {
		steps = cur.T/rules.LineStep - prev.T/rules.LineStep
	}
	for i, l := range ca.Lines {
		want := pa.Lines[i].Pos
		if l.Role != rules.RoleNone && i != ca.Crit {
			for k := 0; k < steps; k++ {
				want = stepToward(want, l.Target, ca.Step)
			}
		}
		if l.Pos != want {
			return false
		}
	}
	if ca.Stage != rules.StageActive && ca.Stage != rules.StageDeclare {
		return ca.Ticks == pa.Ticks
	}
	per := rules.ATU
	if pa.Idle || ca.Stage == rules.StageDeclare {
		per = rules.ATUIdle
	}
	tk, tl := pa.Ticks, pa.TickLeft
	for ; dt > 0; dt-- {
		if tl--; tl <= 0 {
			tk--
			tl = per
		}
	}
	return tk == ca.Ticks && tl == ca.TickLeft
}

func stepToward(pos, target, st int) int {
	switch {
	case pos < target:
		return min(pos+st, target)
	case pos > target:
		return max(pos-st, target)
	}
	return pos
}

// ListenStream serves stream mode on addr (":9902") in the background.
func (s *Server) ListenStream(addr string) error {
	ln, err := net.Listen("tcp", addr)
	if err != nil {
		return err
	}
	log.Printf("stream mode on %s", addr)
	go func() { log.Println("stream:", s.ServeStream(ln)) }()
	return nil
}

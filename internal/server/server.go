package server

import (
	"hash/fnv"
	"net/http"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/goccy/go-json"

	"fujinet-multiplayer-mule/internal/proto"
	"fujinet-multiplayer-mule/internal/rules"
)

// Server holds all tables.
type Server struct {
	tables sync.Map // id -> *Table
	order  []string
	Now    func() time.Time
	Lobby  *LobbyClient
}

// New creates a server with the default tables.
func New(lobby *LobbyClient) *Server {
	s := &Server{Now: time.Now, Lobby: lobby}
	for i := 1; i <= 4; i++ {
		s.AddTable(newTable("m"+strconv.Itoa(i), "colony "+strconv.Itoa(i), 4, true))
	}
	s.AddTable(newTable("ai1", "3 humans + 1 cpu", 3, true))
	s.AddTable(newTable("ai2", "2 humans + 2 cpu", 2, true))
	s.AddTable(newTable("ai3", "solo vs 3 cpu", 1, true))
	for i := 1; i <= 3; i++ {
		s.AddTable(newTable("dev"+strconv.Itoa(i), "dev "+strconv.Itoa(i), 4, false))
	}
	return s
}

// AddTable registers a table.
func (s *Server) AddTable(t *Table) {
	s.tables.Store(t.ID, t)
	s.order = append(s.order, t.ID)
}

// Table returns a table by id.
func (s *Server) Table(id string) *Table {
	v, ok := s.tables.Load(strings.ToLower(id))
	if !ok {
		return nil
	}
	return v.(*Table)
}

// Run advances every table periodically so auctions and computer turns
// progress even when no client is polling.
func (s *Server) Run(interval time.Duration, stop <-chan struct{}) {
	tk := time.NewTicker(interval)
	defer tk.Stop()
	for {
		select {
		case <-stop:
			return
		case <-tk.C:
			s.tickAll()
		}
	}
}

func (s *Server) tickAll() {
	now := s.Now()
	for _, id := range s.order {
		t := s.Table(id)
		t.mu.Lock()
		changed := t.update(now)
		t.mu.Unlock()
		if changed {
			s.notifyLobby(t)
		}
	}
}

func (s *Server) notifyLobby(t *Table) {
	if s.Lobby == nil || !t.Register {
		return
	}
	t.mu.Lock()
	humans := t.humans()
	var result *GameResult
	if t.game != nil && t.game.Phase == rules.PhaseGameOver {
		result = &GameResult{}
		for i, p := range t.game.Players {
			typ := PlayerTypeHuman
			if !p.Human {
				typ = PlayerTypeBot
			}
			result.Players = append(result.Players, GamePlayer{Name: p.Name, Winner: i == t.game.Winner, Type: typ})
		}
	}
	name, id, maxH := t.Name, t.ID, t.MaxHumans
	t.mu.Unlock()
	go s.Lobby.Send(maxH, humans, true, "mule "+name, "?table="+id, result)
}

// Routes installs all HTTP routes on r.
func (s *Server) Routes(r *gin.Engine) {
	route := func(path string, h gin.HandlerFunc) {
		r.GET(path, h)
		r.POST(path, h)
	}
	route("/tables", s.apiTables)
	route("/state", s.act(nil))
	route("/view", s.apiView)
	route("/leave", s.apiLeave)
	route("/ready", s.act(actReady))
	route("/ready/:v", s.act(actReady))
	route("/setup/species/:s", s.act(actSpecies))
	route("/claim/:plot", s.act(actClaim))
	route("/auction/at/:price", s.act(actBidAt))
	route("/auction/move/:dir", s.act(actBidMove))
	route("/auction/push/:dir", s.act(actBidMove))
	route("/declare/:role", s.act(actDeclare))
	route("/collude", s.act(actCollude))
	route("/turn/begin", s.act(turnAct(func(g *rules.Game, seat int, c *gin.Context) (int, error) { return 0, g.BeginTurn(seat) })))
	route("/turn/buymule", s.act(turnAct(func(g *rules.Game, seat int, c *gin.Context) (int, error) { return 0, g.BuyMule(seat) })))
	route("/turn/outfit/:good", s.act(turnAct(func(g *rules.Game, seat int, c *gin.Context) (int, error) {
		gd, ok := parseGood(c.Param("good"))
		if !ok {
			return 0, rules.ErrOutfit
		}
		return 0, g.Outfit(seat, gd)
	})))
	route("/turn/install/:plot", s.act(turnAct(func(g *rules.Game, seat int, c *gin.Context) (int, error) {
		lost, err := g.Install(seat, atoi(c.Param("plot"), -1))
		if lost {
			return 1, err
		}
		return 0, err
	})))
	route("/turn/return", s.act(turnAct(func(g *rules.Game, seat int, c *gin.Context) (int, error) { return 0, g.ReturnMule(seat) })))
	route("/turn/assay/:plot", s.act(turnAct(func(g *rules.Game, seat int, c *gin.Context) (int, error) {
		return g.Assay(seat, atoi(c.Param("plot"), -1))
	})))
	route("/turn/sell/:plot", s.act(turnAct(func(g *rules.Game, seat int, c *gin.Context) (int, error) {
		return 0, g.ListLand(seat, atoi(c.Param("plot"), -1), true)
	})))
	route("/turn/unsell/:plot", s.act(turnAct(func(g *rules.Game, seat int, c *gin.Context) (int, error) {
		return 0, g.ListLand(seat, atoi(c.Param("plot"), -1), false)
	})))
	route("/turn/wampus", s.act(turnAct(func(g *rules.Game, seat int, c *gin.Context) (int, error) { return g.CatchWampus(seat) })))
	route("/turn/pub", s.act(turnAct(func(g *rules.Game, seat int, c *gin.Context) (int, error) { return g.Pub(seat) })))
	route("/turn/end", s.act(turnAct(func(g *rules.Game, seat int, c *gin.Context) (int, error) { return 0, g.EndTurn(seat) })))
}

// action is a table mutation for seat. It returns an action result and error.
type action func(t *Table, seat int, c *gin.Context) (int, error)

func atoi(s string, def int) int {
	v, err := strconv.Atoi(s)
	if err != nil {
		return def
	}
	return v
}

func parseGood(s string) (rules.Good, bool) {
	s = strings.ToLower(s)
	for i, n := range rules.GoodNames {
		if s == n || s == n[:1] || s == strconv.Itoa(i) {
			return rules.Good(i), true
		}
	}
	return rules.NoGood, false
}

func turnAct(f func(g *rules.Game, seat int, c *gin.Context) (int, error)) action {
	return func(t *Table, seat int, c *gin.Context) (int, error) {
		if t.game == nil {
			return 0, rules.ErrPhase
		}
		return f(t.game, seat, c)
	}
}

func actReady(t *Table, seat int, c *gin.Context) (int, error) {
	v := c.Param("v")
	if t.game == nil {
		s := &t.seats[seat]
		switch v {
		case "1":
			s.ready = true
		case "0":
			s.ready = false
		default:
			s.ready = !s.ready
		}
		return 0, nil
	}
	t.game.Continue(seat)
	return 0, nil
}

func actSpecies(t *Table, seat int, c *gin.Context) (int, error) {
	if t.game != nil {
		return 0, rules.ErrPhase
	}
	p := strings.ToLower(c.Param("s"))
	for i, n := range rules.SpeciesNames {
		if p == n || p == strconv.Itoa(i) {
			t.seats[seat].species = rules.Species(i)
			return 0, nil
		}
	}
	return 0, rules.ErrSeat
}

func actClaim(t *Table, seat int, c *gin.Context) (int, error) {
	if t.game == nil {
		return 0, rules.ErrPhase
	}
	return 0, t.game.Claim(seat, atoi(c.Param("plot"), -1))
}

func actBidAt(t *Table, seat int, c *gin.Context) (int, error) {
	if t.game == nil {
		return 0, rules.ErrPhase
	}
	return 0, t.game.SetTarget(seat, atoi(c.Param("price"), 0))
}

// actBidMove is the auction joystick: up, down or release ("hold").
func actBidMove(t *Table, seat int, c *gin.Context) (int, error) {
	g := t.game
	if g == nil || g.Auction == nil {
		return 0, rules.ErrPhase
	}
	switch c.Param("dir") {
	case "up", "u", "1":
		return 0, g.Push(seat, 1)
	case "down", "d", "-1":
		return 0, g.Push(seat, -1)
	default:
		return 0, g.Push(seat, 0)
	}
}

func actDeclare(t *Table, seat int, c *gin.Context) (int, error) {
	if t.game == nil {
		return 0, rules.ErrPhase
	}
	role := rules.RoleNone
	switch strings.ToLower(c.Param("role")) {
	case "b", "buy", "buyer", "1":
		role = rules.RoleBuyer
	case "s", "sell", "seller", "2":
		role = rules.RoleSeller
	}
	return 0, t.game.Declare(seat, role)
}

func actCollude(t *Table, seat int, c *gin.Context) (int, error) {
	if t.game == nil {
		return 0, rules.ErrPhase
	}
	return 0, t.game.Collude(seat)
}

// act wraps an action: lock the table, join/ping, advance, apply, respond.
func (s *Server) act(f action) gin.HandlerFunc {
	return func(c *gin.Context) {
		t := s.Table(c.DefaultQuery("table", "m1"))
		if t == nil {
			s.writeError(c, proto.ErrBadRequest, "unknown table")
			return
		}
		now := s.Now()
		t.mu.Lock()
		before := t.humans()
		wasGame := t.game != nil
		changed := t.update(now)
		seat := t.seatOf(c.Query("player"), now, true)
		var res int
		var err error
		if f != nil {
			if seat < 0 {
				err = rules.ErrSeat
			} else {
				res, err = f(t, seat, c)
			}
		}
		if t.humans() != before || (t.game != nil) != wasGame {
			changed = true
		}
		v := t.view(now, seat)
		t.mu.Unlock()
		if changed {
			s.notifyLobby(t)
		}
		v.Result = res
		v.Poll = pollHint(v)
		if err != nil {
			v.Err, v.ErrText = proto.ErrCode(err), err.Error()
		}
		s.writeView(c, v)
	}
}

func (s *Server) apiView(c *gin.Context) {
	t := s.Table(c.DefaultQuery("table", "m1"))
	if t == nil {
		s.writeError(c, proto.ErrBadRequest, "unknown table")
		return
	}
	t.mu.Lock()
	v := t.view(s.Now(), -1)
	t.mu.Unlock()
	s.writeView(c, v)
}

func (s *Server) apiLeave(c *gin.Context) {
	t := s.Table(c.DefaultQuery("table", "m1"))
	if t == nil {
		c.String(http.StatusOK, "bye")
		return
	}
	t.mu.Lock()
	seat := t.seatOf(c.Query("player"), s.Now(), false)
	t.leave(seat)
	t.mu.Unlock()
	s.notifyLobby(t)
	c.String(http.StatusOK, "bye")
}

// pollHint suggests how often a client should poll, in ms: real-time phases
// need fresh line/cursor positions, other phases change rarely.
func pollHint(v *proto.View) int {
	switch v.Phase {
	case "grant", "landauction", "declare", "trading":
		return 100
	case "develop":
		if v.Turn != nil && v.Turn.Started && !v.Turn.Ended {
			return 250
		}
		return 150
	}
	return 500
}

// TableInfo is one row of /tables (5cardstud-compatible short keys).
type TableInfo struct {
	Table      string `json:"t"`
	Name       string `json:"n"`
	CurPlayers int    `json:"p"`
	MaxPlayers int    `json:"m"`
	Phase      string `json:"ph"`
	Round      int    `json:"r"`
}

func (s *Server) apiTables(c *gin.Context) {
	dev := c.Query("dev") == "1"
	var list []TableInfo
	for _, id := range s.order {
		t := s.Table(id)
		if t.Register == dev {
			continue
		}
		t.mu.Lock()
		ti := TableInfo{Table: t.ID, Name: t.Name, CurPlayers: t.humans(), MaxPlayers: t.MaxHumans, Phase: "lobby"}
		if t.game != nil {
			ti.Phase, ti.Round = t.game.Phase.String(), t.game.Round
		}
		t.mu.Unlock()
		list = append(list, ti)
	}
	switch {
	case c.Query("bin") == "1":
		buf := []byte{byte(len(list))}
		for _, ti := range list {
			buf = append(buf, proto.FixedString(ti.Table, 8)...)
			buf = append(buf, proto.FixedString(ti.Name, 20)...)
			buf = append(buf, proto.FixedString(strconv.Itoa(ti.CurPlayers)+" / "+strconv.Itoa(ti.MaxPlayers), 5)...)
		}
		c.Data(http.StatusOK, "application/octet-stream", buf)
	case c.Query("raw") == "1":
		writeRaw(c, list)
	default:
		writeJSON(c, list)
	}
}

// ---------------------------------------------------------------- output

// viewHash hashes the view excluding fields that change every tick, so
// clients can skip unchanged states with ?hash=.
func viewHash(v *proto.View) uint32 {
	cp := *v
	cp.T, cp.Elapsed, cp.Count = 0, 0, 0
	cp.Poll = 0
	if cp.Turn != nil {
		tc := *cp.Turn
		tc.Elapsed, tc.Left, tc.EndIn = 0, 0, 0
		cp.Turn = &tc
	}
	if cp.Grant != nil {
		gc := *cp.Grant
		gc.Next = 0
		cp.Grant = &gc
	}
	if cp.Auction != nil {
		ac := *cp.Auction
		ac.StageLeft, ac.TickLeft = 0, 0
		cp.Auction = &ac
	}
	b, _ := json.Marshal(&cp)
	h := fnv.New32a()
	h.Write(b)
	return h.Sum32()
}

func (s *Server) writeView(c *gin.Context, v *proto.View) {
	if hq := c.Query("hash"); hq != "" && v.Err == 0 {
		h := viewHash(v)
		c.Header("X-Mule-Hash", strconv.FormatUint(uint64(h), 10))
		if hq == strconv.FormatUint(uint64(h), 10) {
			if c.Query("bin") == "1" {
				hdr := proto.ToBin(v).H
				hdr.MsgType = proto.MsgSame
				c.Data(http.StatusOK, "application/octet-stream", proto.Encode(&hdr, c.Query("be") == "1"))
			} else {
				c.Data(http.StatusOK, "application/json", []byte(`{"same":1,"t":`+strconv.Itoa(v.T)+`,"pe":`+strconv.Itoa(v.Elapsed)+`}`))
			}
			return
		}
	} else if hq != "" {
		c.Header("X-Mule-Hash", strconv.FormatUint(uint64(viewHash(v)), 10))
	}
	switch {
	case c.Query("bin") == "1":
		c.Data(http.StatusOK, "application/octet-stream", proto.Encode(proto.ToBin(v), c.Query("be") == "1"))
	case c.Query("raw") == "1":
		writeRaw(c, v)
	default:
		writeJSON(c, v)
	}
}

func (s *Server) writeError(c *gin.Context, code int, msg string) {
	if c.Query("bin") == "1" {
		hdr := proto.BinHeader{Magic: 'M', Version: proto.Version, MsgType: proto.MsgError, Err: uint8(code), MySeat: -1}
		c.Data(http.StatusOK, "application/octet-stream", proto.Encode(&hdr, c.Query("be") == "1"))
		return
	}
	writeJSON(c, gin.H{"v": proto.Version, "ec": code, "err": msg})
}

func writeJSON(c *gin.Context, v any) {
	b, err := json.Marshal(v)
	if err != nil {
		c.Status(http.StatusInternalServerError)
		return
	}
	c.Data(http.StatusOK, "application/json", b)
}

// writeRaw flattens JSON into key<delim>value<delim> pairs (5cardstud ?raw=1).
func writeRaw(c *gin.Context, v any) {
	delim := "\x00"
	if c.Query("lf") == "1" {
		delim = "\n"
	}
	b, _ := json.Marshal(v)
	c.String(http.StatusOK, proto.Flatten(string(b), delim, c.Query("uc") == "1", c.Query("lc") == "1"))
}

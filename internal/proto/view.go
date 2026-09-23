// Package proto builds the per-player views of a game and serializes them as
// JSON (default), raw key/value pairs (?raw=1) or a fixed binary layout (?bin=1).
// See docs/PROTOCOL.md.
package proto

import "fujinet-multiplayer-mule/internal/rules"

// Version of the protocol (JSON "v" and binary header Version).
const Version = 2

// Error codes, shared by JSON ("ec") and binary (header Err).
const (
	ErrNone = iota
	ErrPhase
	ErrNotTurn
	ErrTimeUp
	ErrPlot
	ErrNoMule
	ErrHaveMule
	ErrNoStock
	ErrMoney
	ErrOutfit
	ErrDone
	ErrTerrain
	ErrSeat
	ErrBadRequest
	ErrTableFull
)

var errCodes = map[error]int{
	rules.ErrPhase: ErrPhase, rules.ErrNotTurn: ErrNotTurn, rules.ErrTimeUp: ErrTimeUp,
	rules.ErrPlot: ErrPlot, rules.ErrNoMule: ErrNoMule, rules.ErrHaveMule: ErrHaveMule,
	rules.ErrNoStock: ErrNoStock, rules.ErrMoney: ErrMoney, rules.ErrOutfit: ErrOutfit,
	rules.ErrDone: ErrDone, rules.ErrTerrain: ErrTerrain, rules.ErrSeat: ErrSeat,
}

// ErrCode maps an engine error to a protocol error code.
func ErrCode(err error) int {
	if err == nil {
		return ErrNone
	}
	if c, ok := errCodes[err]; ok {
		return c
	}
	return ErrBadRequest
}

// LobbySeat is a seat before the game starts.
type LobbySeat struct {
	Name    string
	Species rules.Species
	Human   bool
	Ready   bool
}

// Table is the server-side information needed to build a view.
type Table struct {
	ID        string
	Name      string
	Lobby     [rules.NumSeats]LobbySeat
	Countdown int // BTU until start, -1 none
	Game      *rules.Game
	Connected [rules.NumSeats]bool
}

// View is the JSON state sent to a client. Short keys keep it small for 8-bit parsers.
type View struct {
	V       int       `json:"v"`
	Table   string    `json:"tb"`
	Phase   string    `json:"ph"`
	PhaseID int       `json:"p"`
	Round   int       `json:"r"`
	T       int       `json:"t"`
	Elapsed int       `json:"pe"`
	Count   int       `json:"cd"`
	Me      int       `json:"me"`
	Err     int       `json:"ec"`
	ErrText string    `json:"err,omitempty"`
	Result  int       `json:"res"`
	Hz      int       `json:"hz"`   // clock rate of t/pe and all durations (jiffies per second)
	Poll    int       `json:"poll"` // suggested poll interval, ms
	Players []PlayerV `json:"pl"`
	Map     *MapV     `json:"map,omitempty"`
	Store   *StoreV   `json:"st,omitempty"`
	Grant   *GrantV   `json:"gr,omitempty"`
	Auction *AuctionV `json:"au,omitempty"`
	Turn    *TurnV    `json:"tu,omitempty"`
	Colony  *ColonyV  `json:"co,omitempty"`
	Msgs    []string  `json:"msg,omitempty"`
}

type PlayerV struct {
	Name     string `json:"n"`
	Species  int    `json:"sp"`
	Color    int    `json:"c"`
	AI       bool   `json:"ai"`
	Ready    bool   `json:"rd"`
	Declared bool   `json:"dc"`
	Online   bool   `json:"on"`
	Money    int    `json:"m"`
	Goods    []int  `json:"g"`
	Score    int    `json:"sc"`
	Rank     int    `json:"rk"`
	Prod     []int  `json:"pr"`
	Spoil    []int  `json:"sl"`
}

type MapV struct {
	Terrain   []int  `json:"tr"`
	Mountains []int  `json:"mt"`
	Owner     []int  `json:"ow"`
	Mule      []int  `json:"mu"`
	Crystite  []int  `json:"cr"`
	Prod      []int  `json:"pr"`
	Lost      []bool `json:"ls"`
	ForSale   []bool `json:"fs"`
}

type StoreV struct {
	Stock     []int `json:"s"`
	Price     []int `json:"pr"`
	Buy       []int `json:"b"`
	Sell      []int `json:"x"`
	Mules     int   `json:"mu"`
	MulePrice int   `json:"mp"`
}

type GrantV struct {
	Cursor  int   `json:"cur"` // plot under the cursor, -1 before the sweep, 45 after
	Next    int   `json:"nx"`  // jiffies until the cursor's next step
	Claimed []int `json:"cl"`
	Step    int   `json:"ctu"` // jiffies per cursor step
	Ended   bool  `json:"end"`
}

type LineV struct {
	Role   int  `json:"r"`
	Pos    int  `json:"p"`
	Target int  `json:"t"`
	Units  int  `json:"u"`
	Push   int  `json:"ps"`
	In     bool `json:"in"`
}

type AuctionV struct {
	Land      bool    `json:"land"`
	Good      int     `json:"gd"`
	Plot      int     `json:"pt"`
	Seller    int     `json:"sl"`
	Start     int     `json:"sp"`
	Ask       int     `json:"ak"`
	Stage     int     `json:"sg"`  // 0 show, 1 declare, 2 lead, 3 active, 4 done
	StageLeft int     `json:"sgl"` // jiffies left in a timed stage
	Ticks     int     `json:"tk"`  // time bar: ticks left
	TickLeft  int     `json:"tl"`  // jiffies left in the current tick
	Idle      bool    `json:"idl"` // ticks run at the idle rate
	LineStep  int     `json:"lj"`  // jiffies per line step
	Lo        int     `json:"lo"`
	Hi        int     `json:"hi"`
	StoreBuy  int     `json:"sb"`
	StoreSell int     `json:"ss"`
	Step      int     `json:"stp"`
	Done      bool    `json:"dn"`
	Winner    int     `json:"w"`
	Price     int     `json:"pc"`
	Traded    int     `json:"tr"`
	LastTrade int     `json:"lt"`
	Colluders int     `json:"col"`
	Lines     []LineV `json:"ln"`
}

type WampusV struct {
	Plot  int `json:"p"`
	Mount int `json:"m"`
	From  int `json:"f"`
	To    int `json:"t"`
}

type TurnV struct {
	Seat      int       `json:"s"`
	Started   bool      `json:"st"`
	Ended     bool      `json:"en"`
	Length    int       `json:"len"`
	Elapsed   int       `json:"el"`  // jiffies since the turn began (or since it was offered)
	Left      int       `json:"lf"`  // jiffies of turn time left
	EndIn     int       `json:"ei"`  // after the turn: jiffies until the next turn
	PTU       int       `json:"ptu"` // jiffies per PTU (1 pixel of the time bar)
	AI        bool      `json:"ai"`
	HasMule   bool      `json:"hm"`
	Carrying  int       `json:"cy"`
	Caught    bool      `json:"ct"`
	Event     int       `json:"ev"`
	EventText string    `json:"et,omitempty"`
	Wampus    []WampusV `json:"wp,omitempty"`
	Log       []string  `json:"lg,omitempty"`
}

type ColonyV struct {
	Event      int    `json:"ev"`
	Text       string `json:"tx,omitempty"`
	Plot       int    `json:"pt"`
	Rating     int    `json:"rt"`
	RatingText string `json:"rtx,omitempty"`
	Winner     int    `json:"w"`
	Failed     bool   `json:"f"`
	Total      int    `json:"tot"`
}

func goodsSlice(a [rules.NumGoods]int) []int { return append([]int(nil), a[:]...) }

// BuildView creates the view of tb for seat me (-1 = spectator).
func BuildView(tb *Table, me int) *View {
	v := &View{V: Version, Table: tb.ID, Me: me, Count: tb.Countdown, Hz: rules.JiffiesPerSecond}
	g := tb.Game
	if g == nil {
		v.Phase, v.PhaseID = rules.PhaseLobby.String(), int(rules.PhaseLobby)
		for i, s := range tb.Lobby {
			v.Players = append(v.Players, PlayerV{Name: s.Name, Species: int(s.Species), Color: i,
				AI: !s.Human, Ready: s.Ready, Online: s.Human && tb.Connected[i],
				Goods: []int{0, 0, 0, 0}, Prod: []int{0, 0, 0, 0}, Spoil: []int{0, 0, 0, 0}})
		}
		return v
	}
	v.Phase, v.PhaseID = g.Phase.String(), int(g.Phase)
	v.Round, v.T, v.Elapsed = g.Round, g.T, g.Elapsed()
	for i, p := range g.Players {
		v.Players = append(v.Players, PlayerV{Name: p.Name, Species: int(p.Species), Color: p.Color,
			AI: p.AI, Ready: p.Ready, Declared: p.Declared, Online: p.Human && tb.Connected[i],
			Money: p.Money, Goods: goodsSlice(p.Goods), Score: p.Score, Rank: p.Rank,
			Prod: goodsSlice(p.LastProd), Spoil: goodsSlice(p.LastSpoil)})
	}
	m := &MapV{}
	reveal := g.Phase == rules.PhaseGameOver
	for i, pl := range g.Map {
		m.Terrain = append(m.Terrain, int(pl.Terrain))
		m.Mountains = append(m.Mountains, pl.Mountains)
		m.Owner = append(m.Owner, pl.Owner)
		m.Mule = append(m.Mule, int(pl.Mule))
		cr := -1
		if reveal {
			cr = pl.Crystite
		} else if me >= 0 {
			cr = int(g.Players[me].Known[i])
		}
		m.Crystite = append(m.Crystite, cr)
		m.Prod = append(m.Prod, pl.Prod)
		m.Lost = append(m.Lost, pl.Lost)
		m.ForSale = append(m.ForSale, pl.ForSale)
	}
	v.Map = m
	st := &StoreV{Stock: goodsSlice(g.Store.Stock), Price: goodsSlice(g.Store.Price),
		Mules: g.Store.Mules, MulePrice: g.Store.MulePrice}
	for gd := rules.Good(0); gd < rules.NumGoods; gd++ {
		st.Buy = append(st.Buy, g.Store.BuyPrice(gd))
		st.Sell = append(st.Sell, g.Store.SellPrice(gd))
	}
	v.Store = st
	v.Msgs = g.Messages

	switch g.Phase {
	case rules.PhaseLandGrant:
		// Once the sweep is over the cursor is gone (45), for clients that
		// only have the cursor to go by (the binary state has no end flag).
		cur := g.GrantCur
		if g.GrantEnd >= 0 {
			cur = rules.NumPlots
		}
		v.Grant = &GrantV{Cursor: cur, Next: max(0, g.GrantNext-g.T), Claimed: append([]int(nil), g.Claimed[:]...),
			Step: rules.CTU, Ended: g.GrantEnd >= 0}
	case rules.PhaseLandAuction, rules.PhaseDeclare, rules.PhaseTrading:
		if a := g.Auction; a != nil {
			av := &AuctionV{Land: a.Land, Good: int(a.Good), Plot: a.Plot, Seller: a.Seller, Start: a.Start,
				Ask: a.Ask, Stage: a.Stage, StageLeft: a.StageLeft, Ticks: a.Ticks, TickLeft: a.TickLeft,
				Idle: a.Idle, LineStep: rules.LineStep, Lo: a.Lo, Hi: a.Hi, Step: a.Step,
				StoreBuy: a.StoreBuy, StoreSell: a.StoreSell, Done: a.Done,
				Winner: a.Winner, Price: a.Price, Traded: a.Traded, LastTrade: a.LastTrade,
				Colluders: int(a.Colluding())}
			for _, l := range a.Lines {
				av.Lines = append(av.Lines, LineV{Role: l.Role, Pos: l.Pos, Target: l.Target, Units: l.Units, Push: l.Push, In: l.In})
			}
			v.Auction = av
		}
	case rules.PhaseDevelop:
		if t := g.Turn; t != nil {
			tv := &TurnV{Seat: t.Seat, Started: t.Started, Ended: t.Ended, Length: t.Length,
				PTU: rules.PTUOf(g.Players[t.Seat].Species), HasMule: t.HasMule, Carrying: int(t.Carrying),
				Caught: t.Caught, Event: t.Event, EventText: t.EventText, Log: t.Log, AI: g.Players[t.Seat].AI}
			if t.Started {
				tv.Elapsed = g.T - t.Begin
				tv.Left = max(0, t.Deadline-g.T)
			} else {
				tv.Elapsed = g.T - t.WaitFrom
			}
			if t.Ended {
				tv.Left = 0
				tv.EndIn = max(0, t.EndAt-g.T)
			}
			if me == t.Seat {
				for _, w := range t.Wampus {
					tv.Wampus = append(tv.Wampus, WampusV{w.Plot, w.Mount, w.From, w.To})
				}
			}
			v.Turn = tv
		}
	}
	if g.Phase >= rules.PhaseProduction {
		co := &ColonyV{Event: g.ColonyEvent, Text: g.ColonyText, Plot: g.ColonyPlot, Winner: g.Winner,
			Failed: g.Failed, Total: g.ColonyTotal()}
		if g.Phase == rules.PhaseGameOver {
			co.Rating = g.Rating
			co.RatingText = rules.RatingText[g.Rating]
		}
		v.Colony = co
	}
	return v
}

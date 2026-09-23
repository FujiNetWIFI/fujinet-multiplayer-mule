package rules

import "math/rand/v2"

// Terrain of a plot.
type Terrain uint8

const (
	Plain Terrain = iota
	River
	Mountain
	Town
)

var TerrainNames = []string{"plain", "river", "mountain", "store"}

// Plot is one map square.
type Plot struct {
	Terrain   Terrain
	Mountains int  // 1..3 for Mountain plots
	Crystite  int  // 0..4
	Owner     int  // seat, -1 = none
	Mule      Good // NoGood = no M.U.L.E.
	Prod      int  // units produced last production
	Lost      bool // produced nothing last round (energy/event)
	ForSale   bool // listed by owner at the land office
}

// Player is one seat.
type Player struct {
	Name      string
	Species   Species
	Color     int
	AI        bool
	Human     bool // seat was taken by a human (may now be AI-controlled if dropped)
	Ready     bool
	Money     int
	Goods     [NumGoods]int
	Score     int
	Rank      int            // 1..4
	Known     [NumPlots]int8 // assay results, -1 = unknown
	LastProd  [NumGoods]int
	LastSpoil [NumGoods]int
	Declared  bool
}

// Store holds the colony store stock and prices.
type Store struct {
	Stock     [NumGoods]int
	Price     [NumGoods]int // reference price
	Mules     int
	MulePrice int
}

// BuyPrice is what the store pays for one unit.
func (s *Store) BuyPrice(g Good) int {
	p := s.Price[g]
	switch g {
	case Food, Energy:
		return p - 15
	case Crystite:
		return p &^ 3
	}
	return p
}

// SellPrice is what the store charges for one unit.
func (s *Store) SellPrice(g Good) int {
	switch g {
	case Crystite:
		return s.BuyPrice(g) + 140
	}
	return s.BuyPrice(g) + 35
}

// Line is one participant's line in an auction. Positions are prices; a line
// moves one price step (Auction.Step) every LineStep jiffies toward Target.
type Line struct {
	Role   int // RoleNone, RoleBuyer, RoleSeller
	Pos    int // price in dollars
	Target int
	Home   int  // starting edge (buyers bottom, sellers top)
	In     bool // has entered the auction (moved off the start edge)
	Push   int  // joystick: +1 up, -1 down, 0 released
	Units  int  // units bought(+)/sold(-) this auction
	Floor  int  // seller keeps at least this many units
	Want   int  // buyer stops after this many units (0 = unlimited)
}

const (
	RoleNone = iota
	RoleBuyer
	RoleSeller
)

// Auction stages.
const (
	StageShow    = iota // land: plot for sale shown on the map; goods: status screen
	StageDeclare        // goods: buyers/sellers declare (30 ticks)
	StageLead           // goods: pause before trading
	StageActive         // lines move, timer runs
	StageDone           // result display
)

// Auction is the state of a land or goods auction.
type Auction struct {
	Land      bool
	Good      Good
	Plot      int
	Seller    int // land: seat of seller, -1 colony
	Start     int // land: starting price
	Ask       int // land: seller asking price
	Stage     int
	StageLeft int // jiffies left in a timed stage
	Ticks     int // timer ticks left (the time bar)
	TickLeft  int // jiffies left in the current tick
	Idle      bool
	Lines     [NumSeats]Line
	Lo, Hi    int
	Step      int
	StoreBuy  int
	StoreSell int
	Traded    int
	TradeSum  int
	LastTrade int
	Contact   int           // current trade contact key, -1 none
	ContactN  int           // units traded in this contact
	TradeWait int           // BTU between units in this contact
	NextTrade int           // T of the next unit
	Colluders uint8         // bitmask of colluding seats
	CollUntil int           // Ticks value when collusion ends
	Collude   [NumSeats]int // T of last collude press
	Done      bool
	Winner    int
	Price     int
}

// Wampus appearance during a turn.
type WampusSpot struct {
	Plot  int
	Mount int // which mountain on the plot
	From  int // turn-relative BTU
	To    int
}

// Turn is the active development turn.
type Turn struct {
	Seat      int
	Started   bool
	WaitFrom  int // T when turn became available
	Begin     int // T when turn began
	Deadline  int // T when turn time runs out
	Length    int // BTU
	Ended     bool
	EndAt     int
	Carrying  Good // mule being led; NoGood none
	HasMule   bool
	Wampus    []WampusSpot
	Caught    bool
	Event     int // player event id, 0 none
	EventText string
	Log       []string
	AIUsed    int // PTU used by a computer turn (its turn plays out for this long)
	Pub       bool
}

// Game is the complete state of one M.U.L.E. game.
// grantStop is one plot the land-grant cursor stopped on.
type grantStop struct {
	plot   int // -1 unused
	arrive int // T the cursor reached it
	left   int // T the cursor left it, -1 while it is there
}

type Game struct {
	Seed    uint64
	rng     *rand.Rand
	T       int // BTU clock
	Round   int
	Phase   Phase
	PhaseT  int // T at phase start
	Players [NumSeats]*Player
	Map     [NumPlots]Plot
	Store   Store

	// Land grant
	Claimed   [NumSeats]int        // plot claimed this round, -1 none
	grantWant [NumSeats]int        // AI target plot, -2 = undecided
	GrantCur  int                  // plot under the cursor, -1 before the sweep, NumPlots after
	GrantNext int                  // T of the next cursor step
	GrantEnd  int                  // T the sweep ended, -1 while sweeping
	grantHist [GrantHist]grantStop // the cursor's latest plots, for the latency allowance

	brain Brain

	// Land auctions
	LandQueue        []int // plots to auction (player listings first)
	ColonyLeft       int
	LandPrevAvg      int
	LandLastPrice    int
	LandSum          int
	LandCount        int
	FirstLandSold    bool
	landFirstInRound bool

	// Development
	Order     []int
	OrderIdx  int
	Turn      *Turn
	usedEvent [NumPlayerEvents + 1]bool
	acquired  [NumPlots]int
	acqSeq    int

	// Colony events
	colonyLeft  [NumColonyEvents]int
	ColonyEvent int
	ColonyText  string
	ColonyPlot  int

	// Goods auctions
	GoodIdx int
	Auction *Auction

	Messages []string
	Rating   int
	Failed   bool
	Winner   int
}

// Rng exposes the game's random source to AI code.
func (g *Game) Rng() *rand.Rand { return g.rng }

// PlotRC returns row, col of a plot index.
func PlotRC(p int) (int, int) { return p / MapCols, p % MapCols }

// Neighbors4 returns orthogonal neighbours of p.
func Neighbors4(p int) []int {
	r, c := PlotRC(p)
	var n []int
	if r > 0 {
		n = append(n, p-MapCols)
	}
	if r < MapRows-1 {
		n = append(n, p+MapCols)
	}
	if c > 0 {
		n = append(n, p-1)
	}
	if c < MapCols-1 {
		n = append(n, p+1)
	}
	return n
}

// StoreDistance is the travel distance table from the store. [RE: Store to plot distance]
var StoreDistance = [NumPlots]int{
	4, 4, 4, 4, 5, 4, 4, 4, 4,
	4, 3, 2, 2, 3, 2, 2, 3, 4,
	4, 3, 2, 1, 0, 1, 2, 3, 4,
	4, 3, 2, 2, 3, 2, 2, 3, 4,
	4, 4, 4, 4, 5, 4, 4, 4, 4,
}

// Package rules implements the M.U.L.E. Tournament game rules as a pure,
// deterministic engine. See docs/RULES.md for sources of every constant.
package rules

// Time. The engine clock counts jiffies (NTSC vertical blanks, 60 per second),
// the unit the original game runs on. All values below were measured on the
// Atari 800 original with the tools/extract/capture harness; see docs/TIMING.md.
const (
	JiffiesPerSecond = 60
	BTU              = 4 // jiffies per Base Time Unit (event processor period) [RE, measured]
	PTUDefault       = 7 // BTU per Player Time Unit [RE, measured: 101 PTU turn = 2828 jiffies]
	PTUFlapper       = 9
	PTUHumanoid      = 5
	CTU              = 3 * BTU // land-grant cursor step: 12 jiffies [RE, measured]
	ATU              = 25      // auction timer tick while someone pushes: 25 jiffies [measured, $B327]
	ATUIdle          = ATU / 4 // tick while nobody pushes: 6 jiffies [measured, code at $9D1B]
)

// Map geometry. [RE: Map generation]
const (
	MapRows   = 5
	MapCols   = 9
	NumPlots  = MapRows * MapCols
	StorePlot = 2*MapCols + 4
	RiverCol  = 4
)

const (
	NumSeats  = 4
	NumRounds = 12
	MaxStock  = 255
	MaxMules  = 14 // corral capacity
)

// Good identifies a resource. Order matches store/outfit tables.
type Good int8

const (
	Food Good = iota
	Energy
	Smithore
	Crystite
	NumGoods
	NoGood Good = -1
)

var GoodNames = [NumGoods]string{"food", "energy", "smithore", "crystite"}

// Auction order for goods. [MAN]
var AuctionOrder = [NumGoods]Good{Smithore, Crystite, Food, Energy}

// Outfit prices. [RE: Outfits price]
var OutfitCost = [NumGoods]int{25, 50, 75, 100}

// Store initial state, Tournament. [RE: Round initialization]
var (
	StoreInitStock = [NumGoods]int{8, 8, 8, 0}
	StoreInitPrice = [NumGoods]int{30, 25, 50, 100}
	StoreMinPrice  = [NumGoods]int{30, 25, 20, 50}
	StoreMaxPrice  = 250
	StoreInitMules = 14
	InitMulePrice  = 100
)

// Price step per auction pixel. [RE: Purchase and sale price]
var PriceStep = [NumGoods]int{1, 1, 1, 4}

// Player starting goods, Tournament.
var PlayerInitGoods = [NumGoods]int{4, 2, 0, 0}

// Species. [MAN]
type Species int8

const (
	Mechtron Species = iota
	Gollumer
	Packer
	Bonzoid
	Spheroid
	Flapper
	Leggite
	Humanoid
	NumSpecies
)

var SpeciesNames = [NumSpecies]string{"mechtron", "gollumer", "packer", "bonzoid", "spheroid", "flapper", "leggite", "humanoid"}

// StartMoney returns initial money. [RE: Players initial money]
func StartMoney(s Species, ai bool) int {
	if ai {
		return 1200 // Tournament computer bonus [MAN]
	}
	switch s {
	case Flapper:
		return 1600
	case Humanoid:
		return 600
	}
	return 1000
}

// PTUOf returns the length of one Player Time Unit in jiffies (1 pixel of the turn bar).
func PTUOf(s Species) int {
	switch s {
	case Flapper:
		return PTUFlapper * BTU
	case Humanoid:
		return PTUHumanoid * BTU
	}
	return PTUDefault * BTU
}

// FoodRequired per round (1-based). [RE: Player Food requirement]
func FoodRequired(round int) int {
	switch {
	case round <= 0:
		return 0
	case round <= 3:
		return 3
	case round <= 7:
		return 4
	case round <= NumRounds:
		return 5
	}
	return 0
}

// RoundFactor is round/4+1, used by events, pub and wampus.
func RoundFactor(round int) int { return round/4 + 1 }

// Durations in jiffies unless noted. [measured] = from emulator traces (docs/TIMING.md).
const (
	TurnBaseTime = 10 // PTU
	TurnFoodTime = 91 // PTU

	SummaryAnim     = 410       // status summary drawing before "press buttons" [measured]
	LandingDelay    = 726       // round 1: ship lands, "the ship will be back in 12 months" [measured]
	GrantIntro      = 186       // button press to the first cursor step [measured rounds 1-2]
	GrantClaimPause = 4 * CTU   // cursor pause after a claim [RE, measured 47-48]
	GrantEndPause   = 190       // after the sweep, before the first plot is offered [measured 187-206]
	ForSaleDisplay  = 255 * BTU // plot for sale blinks on the map; skipped when all press [measured]
	GrantLatency    = 36        // [IMPL] network allowance: a human claim may name a plot the cursor left this recently, and takes a computer's claim made this soon after the cursor arrived
	GrantHist       = 4         // [IMPL] cursor plots remembered for GrantLatency (> GrantLatency/CTU)

	LandAuctionTicks  = 70  // [RE, measured $D3]
	DeclareTicks      = 30  // [RE, measured]
	TradeTicks        = 70  // [RE, measured]
	LineStep          = 8   // jiffies per price step of an auction line (2 scanlines) [measured]
	AuctionEndDisplay = 191 // "auction is over" [measured 91+100]
	GoodsStatus       = 561 // per-good usage/spoilage/production screen [measured 127+434]
	DeclareLead       = 57  // pause between declaring and trading [measured]
	TradeFirstBTU     = 12  // first unit after lines meet [measured, $B374]
	TradeMinBTU       = 2   // fastest unit rate [code $912E]
	ColludeTicks      = 10  // [IMPL] not measured

	TurnWaitWampusCap = 255 * BTU // Ta cap for the wampus delay [RE]
	TurnOverDisplay   = 200       // "time has run out" [measured]
	PubDisplay        = 277       // "won $X gambling" [measured 156+121]
	ProductionDisplay = 1421      // production/colony event on the map [measured]

	LobbyCountdown    = 31 * JiffiesPerSecond
	LobbyReady        = 6 * JiffiesPerSecond
	WampusGrace       = 2 * BTU
	LandStep          = 4  // $ per land auction step [RE]
	LandStepsBelow    = 7  // buyers wait 7 steps below the start price [measured y151 vs y137]
	LandStepsAbove    = 35 // highest bid: start + 35 steps [measured y67]
	GoodsStepsBelow   = 10 // buyers start 10 steps below the store buy line [measured y157 vs y137]
	GoodsStepsAbove   = 1  // sellers start 1 step above the store sell line [measured y65 vs y67]
	LandStartFirst    = 160
	LandStartDrop     = 60
	LandStartMin      = 60
	PubCap            = 250
	PlayerEventChance = 275 // per mille
)

// Phase of the game state machine.
type Phase uint8

const (
	PhaseLobby Phase = iota
	PhaseLandGrant
	PhaseLandAuction
	PhaseDevelop
	PhaseProduction
	PhaseDeclare
	PhaseTrading
	PhaseSummary
	PhaseGameOver
)

var PhaseNames = []string{"lobby", "grant", "landauction", "develop", "production", "declare", "trading", "summary", "gameover"}

func (p Phase) String() string { return PhaseNames[p] }

package rules

import (
	"errors"
	"math/rand/v2"
)

// Brain drives computer-controlled seats. Implemented by package ai.
type Brain interface {
	// GrantChoice returns the plot the seat wants during the land grant
	// (must be >= from), or -1.
	GrantChoice(g *Game, seat int, from int) int
	// DevTurn performs a complete development turn using the Game action methods.
	DevTurn(g *Game, seat int)
	// Declare returns RoleBuyer, RoleSeller or RoleNone for the current goods auction.
	Declare(g *Game, seat int, good Good) int
	// AuctionTick lets the computer adjust its line target (land and goods).
	AuctionTick(g *Game, seat int)
}

var (
	ErrPhase    = errors.New("not allowed in this phase")
	ErrNotTurn  = errors.New("not your turn")
	ErrTimeUp   = errors.New("time is up")
	ErrPlot     = errors.New("invalid plot")
	ErrNoMule   = errors.New("no M.U.L.E.")
	ErrHaveMule = errors.New("already leading a M.U.L.E.")
	ErrNoStock  = errors.New("out of stock")
	ErrMoney    = errors.New("not enough money")
	ErrOutfit   = errors.New("M.U.L.E. not outfitted")
	ErrDone     = errors.New("already done")
	ErrTerrain  = errors.New("cannot install that here")
	ErrSeat     = errors.New("invalid seat")
)

// SeatInfo describes a seat when the game starts.
type SeatInfo struct {
	Name    string
	Species Species
	AI      bool
}

// NewGame creates and starts a game. Seats without a human must have AI=true.
func NewGame(seed uint64, seats [NumSeats]SeatInfo, brain Brain) *Game {
	g := &Game{Seed: seed, rng: rand.New(rand.NewPCG(seed, seed^0x4d554c45)), brain: brain}
	g.Map = GenerateMap(g.rng)
	g.Store = Store{Stock: StoreInitStock, Price: StoreInitPrice, Mules: StoreInitMules, MulePrice: InitMulePrice}
	g.colonyLeft = colonyEventCaps
	for i, s := range seats {
		sp := s.Species
		if s.AI {
			sp = Mechtron
		}
		p := &Player{Name: s.Name, Species: sp, Color: i, AI: s.AI, Human: !s.AI,
			Money: StartMoney(sp, s.AI), Goods: PlayerInitGoods}
		for k := range p.Known {
			p.Known[k] = -1
		}
		g.Players[i] = p
	}
	g.Winner, g.ColonyPlot, g.GrantEnd = -1, -1, -1
	g.updateRanks()
	// Status summary #0 is shown before the first round.
	g.setPhase(PhaseSummary)
	return g
}

// SetBrain installs the AI implementation (needed after decoding a saved game).
func (g *Game) SetBrain(b Brain) { g.brain = b }

// SetAI switches a seat between human and computer control (player dropped/rejoined).
func (g *Game) SetAI(seat int, ai bool) {
	g.Players[seat].AI = ai
	if ai && g.Phase == PhaseDevelop && g.Turn != nil && g.Turn.Seat == seat && !g.Turn.Ended {
		if !g.Turn.Started {
			g.beginTurn()
		}
		g.endTurn()
	}
}

func (g *Game) setPhase(p Phase) {
	g.Phase = p
	g.PhaseT = g.T
	for _, pl := range g.Players {
		pl.Ready = false
	}
}

// Elapsed returns jiffies since the phase started.
func (g *Game) Elapsed() int { return g.T - g.PhaseT }

// Advance runs the clock forward to jiffy t.
func (g *Game) Advance(t int) {
	for g.T < t && g.Phase != PhaseGameOver {
		g.T++
		g.tick()
	}
	if g.T < t {
		g.T = t
	}
}

func (g *Game) tick() {
	switch g.Phase {
	case PhaseLandGrant:
		g.tickGrant()
	case PhaseLandAuction, PhaseDeclare, PhaseTrading:
		g.tickAuction()
	case PhaseDevelop:
		g.tickDevelop()
	case PhaseProduction:
		if g.Elapsed() >= ProductionDisplay {
			g.startGoods(0)
		}
	case PhaseSummary:
		// The summary draws for SummaryAnim, then waits for every human's button.
		if g.Elapsed() >= SummaryAnim && g.allReady() {
			g.startRound()
		}
	}
}

// allReady is true when every human-controlled seat pressed its button.
func (g *Game) allReady() bool {
	for _, p := range g.Players {
		if !p.AI && !p.Ready {
			return false
		}
	}
	return true
}

// Continue is a human's button press on a waiting screen.
func (g *Game) Continue(seat int) {
	g.Players[seat].Ready = true
}

func (g *Game) msg(s string) {
	g.Messages = append(g.Messages, s)
	if len(g.Messages) > 8 {
		g.Messages = g.Messages[len(g.Messages)-8:]
	}
}

// ---------------------------------------------------------------- rounds

func (g *Game) startRound() {
	if g.Round >= NumRounds || g.checkFailure() {
		g.endGame()
		return
	}
	g.Round++
	g.updateRanks()
	for i := range g.Claimed {
		g.Claimed[i] = -1
		g.grantWant[i] = -2
	}
	g.setPhase(PhaseLandGrant)
	g.GrantCur, g.GrantEnd = -1, -1
	for i := range g.grantHist {
		g.grantHist[i] = grantStop{plot: -1}
	}
	g.GrantNext = g.T + GrantIntro
	if g.Round == 1 {
		g.GrantNext += LandingDelay
	}
}

func (g *Game) checkFailure() bool {
	if g.Round == 0 {
		return false
	}
	for _, gd := range []Good{Food, Energy} {
		have := g.Store.Stock[gd]
		for i, p := range g.Players {
			have += p.Goods[gd]
			have += countMule(g, i, gd)
		}
		if have == 0 {
			g.Failed = true
			return true
		}
	}
	return false
}

func (g *Game) endGame() {
	g.updateRanks()
	g.Rating = ColonyRating(g.ColonyTotal())
	if g.Failed {
		g.Rating = 0
	}
	g.Winner = g.ByRank()[0]
	g.setPhase(PhaseGameOver)
}

// ---------------------------------------------------------------- land grant

// grantable reports whether the cursor stops on plot p (the store and owned
// plots are skipped without using time). [measured]
func (g *Game) grantable(p int) bool {
	return p != StorePlot && g.Map[p].Owner < 0
}

func (g *Game) tickGrant() {
	if g.GrantEnd >= 0 {
		if g.T-g.GrantEnd >= GrantEndPause {
			g.startLandAuctions()
		}
		return
	}
	if g.T < g.GrantNext {
		return
	}
	// Step the cursor.
	next := g.GrantCur + 1
	for next < NumPlots && !g.grantable(next) {
		next++
	}
	g.moveCursor(next)
	g.GrantNext = g.T + CTU
	unclaimed := false
	for seat := range g.Players {
		if g.Claimed[seat] < 0 {
			unclaimed = true
		}
	}
	if next >= NumPlots || !unclaimed {
		g.GrantEnd = g.T
		return
	}
	// Computers press their button as soon as the cursor reaches their plot. [RE]
	// Several simultaneous claims go to the worst ranked player. (A human who
	// pressed on the same plot can still take it: see Claim.)
	var want []int
	for seat, p := range g.Players {
		if !p.AI || g.Claimed[seat] >= 0 {
			continue
		}
		w := g.grantWant[seat]
		if w == -2 || (w >= 0 && (w < next || !g.grantable(w))) {
			w = g.brain.GrantChoice(g, seat, next)
			g.grantWant[seat] = w
		}
		if w == next {
			want = append(want, seat)
		}
	}
	if len(want) > 0 {
		best := want[0]
		for _, s := range want[1:] {
			if g.Players[s].Rank > g.Players[best].Rank {
				best = s
			}
		}
		g.claim(best, next)
	}
}

// moveCursor puts the grant cursor on plot p and records the stop.
func (g *Game) moveCursor(p int) {
	h := &g.grantHist
	if h[0].plot >= 0 && h[0].left < 0 {
		h[0].left = g.T
	}
	copy(h[1:], h[:len(h)-1])
	h[0] = grantStop{plot: p, arrive: g.T, left: -1}
	g.GrantCur = p
}

// grantStopAt returns the cursor's latest stop on plot p within the latency
// allowance, or nil.
func (g *Game) grantStopAt(p int) *grantStop {
	for i := range g.grantHist {
		st := &g.grantHist[i]
		if st.plot == p && (st.left < 0 || g.T-st.left <= GrantLatency) {
			return st
		}
	}
	return nil
}

func (g *Game) claim(seat, plot int) {
	g.setOwner(plot, seat)
	g.Claimed[seat] = plot
	g.GrantNext = g.T + GrantClaimPause
	for s := range g.grantWant {
		if g.grantWant[s] == plot {
			g.grantWant[s] = -2
		}
	}
}

// Claim is a human's land-grant button press naming the plot it saw under the
// cursor. The human's view and button reach the server late, so the claim is
// judged as of the press [IMPL]: it may name a plot the cursor left up to
// GrantLatency ago, and it takes the plot from a computer that claimed it up to
// GrantLatency after the cursor arrived (a computer presses on arrival, which
// no human over a network could beat). The cursor goes back to the plot and
// pauses there.
func (g *Game) Claim(seat, plot int) error {
	if g.Phase != PhaseLandGrant || g.GrantEnd >= 0 {
		return ErrPhase
	}
	if g.Claimed[seat] >= 0 {
		return ErrDone
	}
	if plot < 0 || plot >= NumPlots || plot == StorePlot {
		return ErrPlot
	}
	st := g.grantStopAt(plot)
	if st == nil {
		return ErrPlot
	}
	if o := g.Map[plot].Owner; o >= 0 {
		if !g.Players[o].AI || g.Claimed[o] != plot || g.T-st.arrive > GrantLatency {
			return ErrPlot
		}
		g.Claimed[o] = -1
		g.grantWant[o] = -2
	}
	if plot != g.GrantCur {
		g.moveCursor(plot)
	}
	g.claim(seat, plot)
	return nil
}

// ---------------------------------------------------------------- land auctions

func (g *Game) startLandAuctions() {
	g.LandQueue = g.LandQueue[:0]
	for i := range g.Map {
		if g.Map[i].ForSale && g.Map[i].Owner >= 0 {
			g.LandQueue = append(g.LandQueue, i)
		}
	}
	g.ColonyLeft = clamp(Normal(g.rng, 1), 0, len(freePlots(g)))
	g.LandSum, g.LandCount = 0, 0
	g.landFirstInRound = true
	g.nextLandAuction()
}

func (g *Game) landStartPrice() int {
	var p int
	switch {
	case !g.FirstLandSold:
		p = LandStartFirst
	case g.landFirstInRound:
		p = g.LandPrevAvg - LandStartDrop
	default:
		p = g.LandLastPrice - LandStartDrop
	}
	p = max(p, LandStartMin)
	return p - p%LandStep
}

func (g *Game) nextLandAuction() {
	plot, seller := -1, -1
	if len(g.LandQueue) > 0 {
		plot = g.LandQueue[0]
		g.LandQueue = g.LandQueue[1:]
		seller = g.Map[plot].Owner
	} else if g.ColonyLeft > 0 {
		f := freePlots(g)
		if len(f) > 0 {
			plot = f[g.rng.IntN(len(f))]
		}
		g.ColonyLeft--
	}
	if plot < 0 {
		if g.LandCount > 0 {
			g.LandPrevAvg = g.LandSum / g.LandCount
		}
		g.startDevelop()
		return
	}
	start := g.landStartPrice()
	a := &Auction{Land: true, Good: NoGood, Plot: plot, Seller: seller, Crit: -1, Start: start, Step: LandStep,
		Stage: StageShow, StageLeft: ForSaleDisplay, Ticks: LandAuctionTicks, Winner: -1, Contact: -1}
	a.Lo = max(0, start-LandStepsBelow*LandStep)
	a.Hi = start + LandStepsAbove*LandStep
	a.Ask = a.Hi
	for s := range a.Lines {
		a.Collude[s] = -1000
		l := &a.Lines[s]
		if s == seller {
			l.Role, l.Home = RoleSeller, a.Hi
		} else {
			l.Role, l.Home = RoleBuyer, a.Lo
		}
		l.Pos, l.Target = l.Home, l.Home
	}
	g.Auction = a
	g.setPhase(PhaseLandAuction)
}

func (g *Game) finishLandAuction() {
	a := g.Auction
	a.Done = true
	a.Stage, a.StageLeft = StageDone, AuctionEndDisplay
	if a.Winner < 0 && a.Seller < 0 {
		best := g.bestLine(RoleBuyer, true)
		if best >= 0 && a.Lines[best].Pos >= a.Start {
			a.Winner, a.Price = best, a.Lines[best].Pos
		}
	}
	if a.Winner >= 0 {
		w := g.Players[a.Winner]
		w.Money -= a.Price
		if a.Seller >= 0 {
			g.Players[a.Seller].Money += a.Price
		}
		g.Map[a.Plot].ForSale = false
		g.setOwner(a.Plot, a.Winner)
		g.LandLastPrice = a.Price
		g.FirstLandSold = true
	} else {
		g.Map[a.Plot].ForSale = false
		if a.Seller < 0 {
			g.LandLastPrice = a.Start/2 + 52
			g.ColonyLeft = 0
		}
	}
	g.LandSum += g.LandLastPrice
	g.LandCount++
	g.landFirstInRound = false
}

// ---------------------------------------------------------------- development

func (g *Game) startDevelop() {
	g.Auction = nil
	g.updateRanks()
	g.Order = g.ByRank()
	if g.Store.Mules <= 7 {
		for i, j := 0, len(g.Order)-1; i < j; i, j = i+1, j-1 {
			g.Order[i], g.Order[j] = g.Order[j], g.Order[i]
		}
	}
	g.OrderIdx = 0
	g.setPhase(PhaseDevelop)
	g.startTurn()
}

func (g *Game) startTurn() {
	seat := g.Order[g.OrderIdx]
	g.Turn = &Turn{Seat: seat, WaitFrom: g.T, Carrying: NoGood}
	if g.Players[seat].AI {
		// A computer turn is computed at once and shown only as its result
		// display: clients don't animate its walk, so waiting out the time
		// it used would look like a hang. [IMPL]
		g.beginTurn()
		t := g.Turn
		g.brain.DevTurn(g, seat)
		pause := TurnOverDisplay
		if t.Pub {
			pause = PubDisplay
		}
		t.Ended = true
		t.EndAt = t.Begin + pause
	}
}

// TurnLengthPTU returns the development time for seat. [RE: Available time for turn]
func (g *Game) TurnLengthPTU(seat int) int {
	need := FoodRequired(g.Round)
	have := g.Players[seat].Goods[Food]
	if have >= need {
		return TurnBaseTime + TurnFoodTime
	}
	return TurnBaseTime + TurnFoodTime*have/need
}

func (g *Game) beginTurn() {
	t := g.Turn
	seat := t.Seat
	t.Started, t.Begin = true, g.T
	t.Event, t.EventText = g.rollPlayerEvent(seat)
	ptu := PTUOf(g.Players[seat].Species)
	t.Length = g.TurnLengthPTU(seat) * ptu
	t.Deadline = g.T + t.Length
	// Ta: time waited before pressing, capped at 255 BTU, minus 100 BTU if an event is shown. [RE]
	ta := min(TurnWaitWampusCap, g.T-t.WaitFrom) / BTU
	if t.Event != 0 {
		ta = max(0, ta-100)
	}
	t.Wampus = g.wampusSchedule(ta/4*ptu, t.Length, ptu)
}

func (g *Game) wampusSchedule(first, length, ptu int) []WampusSpot {
	type spot struct{ p, m int }
	var spots []spot
	for i, pl := range g.Map {
		for m := 0; m < pl.Mountains; m++ {
			spots = append(spots, spot{i, m})
		}
	}
	if len(spots) == 0 {
		return nil
	}
	var ws []WampusSpot
	cur := spots[g.rng.IntN(len(spots))]
	t := first
	for t < length {
		vis := (1 + g.rng.IntN(4)) * ptu
		ws = append(ws, WampusSpot{Plot: cur.p, Mount: cur.m, From: t, To: t + vis})
		t += vis
		if g.rng.IntN(2) == 0 {
			t += (4 + g.rng.IntN(4)) * ptu
		} else {
			t += 2 * ptu
			cur = spots[g.rng.IntN(len(spots))]
		}
	}
	return ws
}

// endTurn ends a human's turn now; the result display follows.
func (g *Game) endTurn() {
	t := g.Turn
	if t.HasMule {
		t.HasMule = false
		t.Log = append(t.Log, "your M.U.L.E. ran away.")
	}
	pause := TurnOverDisplay
	if t.Pub {
		pause = PubDisplay
	}
	t.Ended, t.EndAt = true, g.T+pause
}

func (g *Game) tickDevelop() {
	t := g.Turn
	if t == nil {
		return
	}
	// A human's turn starts only when the player presses the button. [measured: no timeout]
	if !t.Started {
		return
	}
	if !t.Ended && g.T >= t.Deadline {
		g.endTurn()
	}
	if t.Ended && g.T >= t.EndAt {
		g.OrderIdx++
		if g.OrderIdx >= len(g.Order) {
			g.Turn = nil
			g.startProduction()
			return
		}
		g.startTurn()
	}
}

// activeTurn validates that seat may act now.
func (g *Game) activeTurn(seat int) (*Turn, error) {
	if g.Phase != PhaseDevelop || g.Turn == nil {
		return nil, ErrPhase
	}
	t := g.Turn
	if t.Seat != seat {
		return nil, ErrNotTurn
	}
	if !t.Started || t.Ended {
		return nil, ErrDone
	}
	if g.T >= t.Deadline {
		g.endTurn()
		return nil, ErrTimeUp
	}
	return t, nil
}

// BeginTurn starts a human's turn (the "press button" moment).
func (g *Game) BeginTurn(seat int) error {
	if g.Phase != PhaseDevelop || g.Turn == nil || g.Turn.Seat != seat {
		return ErrNotTurn
	}
	if g.Turn.Started {
		return ErrDone
	}
	g.beginTurn()
	return nil
}

// BuyMule buys a M.U.L.E. from the corral.
func (g *Game) BuyMule(seat int) error {
	t, err := g.activeTurn(seat)
	if err != nil {
		return err
	}
	if t.HasMule {
		return ErrHaveMule
	}
	if g.Store.Mules <= 0 {
		return ErrNoStock
	}
	p := g.Players[seat]
	if p.Money < g.Store.MulePrice {
		return ErrMoney
	}
	p.Money -= g.Store.MulePrice
	g.Store.Mules--
	t.HasMule, t.Carrying = true, NoGood
	return nil
}

// Outfit equips the led M.U.L.E. for good.
func (g *Game) Outfit(seat int, good Good) error {
	t, err := g.activeTurn(seat)
	if err != nil {
		return err
	}
	if good < 0 || good >= NumGoods {
		return ErrOutfit
	}
	if !t.HasMule {
		return ErrNoMule
	}
	if t.Carrying == good {
		return ErrDone
	}
	p := g.Players[seat]
	if p.Money < OutfitCost[good] {
		return ErrMoney
	}
	p.Money -= OutfitCost[good]
	t.Carrying = good
	return nil
}

// Install puts the led M.U.L.E. on plot. Returns true if the M.U.L.E. ran away.
func (g *Game) Install(seat, plot int) (bool, error) {
	t, err := g.activeTurn(seat)
	if err != nil {
		return false, err
	}
	if !t.HasMule {
		return false, ErrNoMule
	}
	if plot < 0 || plot >= NumPlots {
		return false, ErrPlot
	}
	pl := &g.Map[plot]
	if pl.Owner != seat || t.Carrying == NoGood {
		t.HasMule = false
		t.Log = append(t.Log, "your M.U.L.E. ran away.")
		return true, nil
	}
	if !pl.CanInstall(t.Carrying) {
		return false, ErrTerrain
	}
	old := pl.Mule
	pl.Mule = t.Carrying
	if old != NoGood {
		t.Carrying = old
	} else {
		t.HasMule, t.Carrying = false, NoGood
	}
	return false, nil
}

// ReturnMule sells the led M.U.L.E. back to the corral.
func (g *Game) ReturnMule(seat int) error {
	t, err := g.activeTurn(seat)
	if err != nil {
		return err
	}
	if !t.HasMule {
		return ErrNoMule
	}
	g.Players[seat].Money += g.Store.MulePrice
	g.Store.Mules++
	t.HasMule, t.Carrying = false, NoGood
	return nil
}

// Assay returns the crystite level of plot.
func (g *Game) Assay(seat, plot int) (int, error) {
	if _, err := g.activeTurn(seat); err != nil {
		return 0, err
	}
	if plot < 0 || plot >= NumPlots || plot == StorePlot {
		return 0, ErrPlot
	}
	v := g.Map[plot].Crystite
	g.Players[seat].Known[plot] = int8(v)
	return v, nil
}

// ListLand toggles an owned plot for sale at the next land auction.
func (g *Game) ListLand(seat, plot int, sale bool) error {
	if _, err := g.activeTurn(seat); err != nil {
		return err
	}
	if plot < 0 || plot >= NumPlots || g.Map[plot].Owner != seat {
		return ErrPlot
	}
	g.Map[plot].ForSale = sale
	return nil
}

// WampusVisible returns the visible wampus spot at turn-relative BTU rel, or nil.
func (t *Turn) WampusVisible(rel, grace int) *WampusSpot {
	for i := range t.Wampus {
		w := &t.Wampus[i]
		if rel >= w.From && rel < w.To+grace {
			return w
		}
	}
	return nil
}

// CatchWampus claims the wampus bounty. Returns the reward.
func (g *Game) CatchWampus(seat int) (int, error) {
	t, err := g.activeTurn(seat)
	if err != nil {
		return 0, err
	}
	if t.Caught || t.HasMule {
		return 0, ErrDone
	}
	if t.WampusVisible(g.T-t.Begin, WampusGrace) == nil {
		return 0, ErrPlot
	}
	t.Caught = true
	r := 100 * RoundFactor(g.Round)
	g.Players[seat].Money += r
	return r, nil
}

// Pub ends the turn with gambling winnings. [RE: Gambling at the Pub]
func (g *Game) Pub(seat int) (int, error) {
	t, err := g.activeTurn(seat)
	if err != nil {
		return 0, err
	}
	ptu := PTUOf(g.Players[seat].Species)
	left := (t.Deadline - g.T) / ptu
	win := 50 * RoundFactor(g.Round)
	if left > 0 {
		win += g.rng.IntN(2 * left)
	}
	win = min(win, PubCap)
	g.Players[seat].Money += win
	t.Log = append(t.Log, "pub")
	t.Pub = true
	g.endTurn()
	return win, nil
}

// EndTurn ends the development turn early.
func (g *Game) EndTurn(seat int) error {
	if _, err := g.activeTurn(seat); err != nil {
		return err
	}
	g.endTurn()
	return nil
}

// ---------------------------------------------------------------- production

func (g *Game) startProduction() {
	g.computeProduction()
	g.rollColonyEvent()
	g.applyProduction()
	g.updateRanks()
	g.setPhase(PhaseProduction)
}

// ---------------------------------------------------------------- goods

// Critical returns the amount of good a seat needs to keep for next round.
func (g *Game) Critical(seat int, gd Good) int {
	switch gd {
	case Food:
		return FoodRequired(g.Round + 1)
	case Energy:
		return g.EnergyNeed(seat)
	}
	return 0
}

func (g *Game) startGoods(idx int) {
	for ; idx < int(NumGoods); idx++ {
		gd := AuctionOrder[idx]
		g.updatePrice(gd)
		any := g.Store.Stock[gd] > 0
		for _, p := range g.Players {
			if p.Goods[gd] > 0 {
				any = true
			}
		}
		if any {
			break
		}
		if gd == Crystite {
			g.Store.Stock[Crystite] = 0
		}
	}
	g.GoodIdx = idx
	if idx >= int(NumGoods) {
		g.Auction = nil
		g.buildMules()
		g.updateRanks()
		g.setPhase(PhaseSummary)
		return
	}
	gd := AuctionOrder[idx]
	st := PriceStep[gd]
	a := &Auction{Good: gd, Plot: -1, Seller: -1, Winner: -1, Contact: -1, Crit: -1, Step: st,
		Stage: StageShow, StageLeft: GoodsStatus, Ticks: DeclareTicks}
	a.StoreBuy, a.StoreSell = g.Store.BuyPrice(gd), g.Store.SellPrice(gd)
	// The screen spans the store's buy and sell lines (35 steps apart), with
	// buyers starting 10 steps below and sellers 1 step above. [measured]
	a.Lo = max(0, a.StoreBuy-GoodsStepsBelow*st)
	a.Hi = a.StoreSell + GoodsStepsAbove*st
	for s, p := range g.Players {
		a.Collude[s] = -1000
		p.Declared = false
		role := RoleBuyer
		if p.Goods[gd] > g.Critical(s, gd) {
			role = RoleSeller
		}
		a.Lines[s].Role = role
	}
	g.Auction = a
	g.setPhase(PhaseDeclare)
}

// Declare sets a seat's role for the upcoming goods auction.
func (g *Game) Declare(seat, role int) error {
	if g.Phase != PhaseDeclare || g.Auction == nil || g.Auction.Stage > StageDeclare {
		return ErrPhase
	}
	if role < RoleNone || role > RoleSeller {
		return ErrSeat
	}
	g.Auction.Lines[seat].Role = role
	g.Players[seat].Declared = true
	return nil
}

func (g *Game) finishGoodsAuction() {
	a := g.Auction
	a.Done = true
	a.Stage, a.StageLeft = StageDone, AuctionEndDisplay
	gd := a.Good
	if a.Traded > 0 {
		g.Store.Price[gd] = clamp(a.TradeSum/a.Traded, StoreMinPrice[gd], StoreMaxPrice)
	}
	if gd == Crystite {
		g.Store.Stock[Crystite] = 0
	}
}

// ---------------------------------------------------------------- auctions (shared)

// SetTarget sets a seat's desired auction line position.
func (g *Game) SetTarget(seat, price int) error {
	a := g.Auction
	if a == nil || a.Stage != StageActive || (g.Phase != PhaseLandAuction && g.Phase != PhaseTrading) {
		return ErrPhase
	}
	l := &a.Lines[seat]
	if l.Role == RoleNone {
		return ErrSeat
	}
	l.Target = clamp(price, a.Lo, g.lineTop(seat))
	return nil
}

// lineTop is the highest price a seat's line may reach. A land bid has no
// top but the bidder's money: the scale scrolls up with it. [Atari]
func (g *Game) lineTop(seat int) int {
	a := g.Auction
	if a.Land && a.Lines[seat].Role == RoleBuyer {
		m := g.Players[seat].Money
		return max(a.Lo, m-m%LandStep)
	}
	return a.Hi
}

// Push is a joystick push up (+1), down (-1) or release (0) in an auction.
// Releasing in a land auction lets a bid drift back down. [measured]
func (g *Game) Push(seat, dir int) error { return g.PushSeen(seat, dir, -1) }

// PushSeen is Push with the Traded count the player last saw (-1 unknown).
// A push that arrives before the lines are released (the plot-for-sale
// display, or the lead into trading) is held and applied the moment they
// are, so a human starts moving on the same jiffy as the computers. [IMPL]
func (g *Game) PushSeen(seat, dir, seen int) error {
	a := g.Auction
	if a == nil {
		return ErrPhase
	}
	l := &a.Lines[seat]
	if a.Stage == StageLead || (a.Land && a.Stage == StageShow) {
		l.Push = dir
		return nil
	}
	if g.Phase == PhaseDeclare {
		switch {
		case dir > 0:
			return g.Declare(seat, RoleSeller)
		case dir < 0:
			return g.Declare(seat, RoleBuyer)
		}
		return nil
	}
	l.Push = dir
	if dir != 0 {
		l.Dir = dir
	}
	if dir != 0 && !a.Land && a.Stage == StageActive {
		g.breakContact(seat, dir, seen)
	}
	return g.applyPush(seat)
}

// HoldAt is a stick release that names the price the player saw their line
// stop at. The client walks its line ahead of the server between polls, so
// the server's line is behind it: a released goods line carries on to where
// the player let go, up to HoldLead steps past the server's position in the
// way it was pushed, rather than jumping back. A land bid still drifts
// down. [IMPL]
func (g *Game) HoldAt(seat, price, seen int) error {
	if err := g.PushSeen(seat, 0, seen); err != nil {
		return err
	}
	a := g.Auction
	l := &a.Lines[seat]
	if a.Land || a.Stage != StageActive || a.Crit == seat || l.Dir == 0 {
		return nil
	}
	lead := l.Pos + l.Dir*HoldLead*a.Step
	price = clamp(price, min(l.Pos, lead), max(l.Pos, lead))
	// Not past where the line can go, so it comes to rest there (the
	// client keeps its own line until the server's is at rest).
	if l.Role == RoleBuyer {
		price = min(price, max(l.Pos, g.buyerCeiling(seat)))
	} else {
		price = max(price, min(l.Pos, g.sellerFloor(seat)))
	}
	return g.SetTarget(seat, price)
}

// applyPush sets a line's target from its stick.
func (g *Game) applyPush(seat int) error {
	a := g.Auction
	l := &a.Lines[seat]
	// A seller stopped at the critical level, or sent back after running
	// dry, stays at the edge. One at the critical level from the start may
	// step in once, to be told.
	if !a.Land && l.Role == RoleSeller && (a.Crit == seat || (a.Stage == StageActive && l.In && !g.canSell(seat))) {
		return nil
	}
	switch {
	case l.Push > 0:
		return g.SetTarget(seat, g.lineTop(seat))
	case l.Push < 0, a.Land && l.Role == RoleBuyer:
		return g.SetTarget(seat, a.Lo)
	}
	return g.SetTarget(seat, l.Pos)
}

// applyHeldPushes starts the humans' lines held before the auction began.
func (g *Game) applyHeldPushes() {
	for s, p := range g.Players {
		if !p.AI && g.Auction.Lines[s].Push != 0 {
			g.applyPush(s)
		}
	}
}

// breakContact: a trader pushing away from the line they are trading with
// leaves it at once, rather than at the next line step, so no further unit
// trades. Units that traded after the count the player had seen, within
// TradeUndoWindow, are given back: the player was reacting to an older
// screen (poll and network latency). [IMPL]
func (g *Game) breakContact(seat, dir, seen int) {
	a := g.Auction
	if a.Contact < 0 {
		return
	}
	buyer, seller := a.Contact/8-1, a.Contact%8-1
	if !(seat == buyer && dir < 0) && !(seat == seller && dir > 0) {
		return
	}
	if seen >= 0 && a.Traded < MaxStock {
		for i := len(a.Recent) - 1; i >= 0; i-- {
			r := a.Recent[i]
			if r.N <= seen || g.T-r.T > TradeUndoWindow {
				break
			}
			g.undoTrade(r)
			a.Recent = a.Recent[:i]
		}
	}
	l := &a.Lines[seat]
	l.Pos = clamp(l.Pos+dir*a.Step, a.Lo, a.Hi)
	a.Contact = -1
	a.Recent = a.Recent[:0]
}

// undoTrade gives back one unit traded in the current contact.
func (g *Game) undoTrade(r TradeRec) {
	a := g.Auction
	gd := a.Good
	if r.Buyer >= 0 {
		g.Players[r.Buyer].Money += r.Price
		g.Players[r.Buyer].Goods[gd]--
		a.Lines[r.Buyer].Units--
	} else {
		g.Store.Stock[gd]--
	}
	if r.Seller >= 0 {
		g.Players[r.Seller].Money -= r.Price
		g.Players[r.Seller].Goods[gd]++
		a.Lines[r.Seller].Units++
	} else {
		g.Store.Stock[gd]++
	}
	a.Traded--
	a.TradeSum -= r.Price
	a.ContactN--
}

// Collude registers a collusion press.
func (g *Game) Collude(seat int) error {
	a := g.Auction
	if g.Phase != PhaseTrading || a == nil || a.Stage != StageActive {
		return ErrPhase
	}
	a.Collude[seat] = g.T
	var mask uint8
	n := 0
	for s := range a.Collude {
		if g.T-a.Collude[s] < ATU {
			mask |= 1 << s
			n++
		}
	}
	if n >= 2 {
		a.Colluders = mask
		a.CollUntil = a.Ticks - ColludeTicks
	}
	return nil
}

func (a *Auction) colluding() bool { return a.Colluders != 0 && a.Ticks > a.CollUntil }

// Colluding returns the bitmask of seats currently colluding (0 = none).
func (a *Auction) Colluding() uint8 {
	if a.colluding() {
		return a.Colluders
	}
	return 0
}

func (a *Auction) inCollusion(seat int) bool {
	return !a.colluding() || a.Colluders&(1<<seat) != 0
}

// canBuy: seat is an active buyer that can pay price.
func (g *Game) canBuy(seat, price int) bool {
	a := g.Auction
	l := &a.Lines[seat]
	if l.Role != RoleBuyer || !a.inCollusion(seat) {
		return false
	}
	if l.Want > 0 && l.Units >= l.Want {
		return false
	}
	return g.Players[seat].Money >= price
}

func (g *Game) canSell(seat int) bool {
	a := g.Auction
	l := &a.Lines[seat]
	if l.Role != RoleSeller || !a.inCollusion(seat) {
		return false
	}
	if a.Land {
		return true
	}
	return g.Players[seat].Goods[a.Good] > max(l.Floor, g.Critical(seat, a.Good))
}

// critical stops a seller who has reached the critical level of the good:
// "seller at critical level!" is shown, the line waits CritPause and then
// goes back to its edge. Only goods with a critical level (Food, Energy)
// stop this way. [MAN]
func (g *Game) critical(seat int) {
	a := g.Auction
	l := &a.Lines[seat]
	a.Crit, a.CritLeft = seat, CritShow
	a.Contact = -1
	l.Push, l.Target = 0, l.Pos
}

// critTick runs "seller at critical level".
func (g *Game) critTick() {
	a := g.Auction
	if a.Crit < 0 {
		return
	}
	a.CritLeft--
	if a.CritLeft == CritShow-CritPause {
		l := &a.Lines[a.Crit]
		l.Pos, l.Target = l.Home, l.Home
	}
	if a.CritLeft <= 0 {
		a.Crit = -1
	}
}

// bestLine returns the highest buyer or lowest seller (ties: worst rank).
func (g *Game) bestLine(role int, land bool) int {
	a := g.Auction
	best := -1
	for s := range a.Lines {
		l := &a.Lines[s]
		ok := false
		if role == RoleBuyer {
			ok = g.canBuy(s, l.Pos)
			if land {
				ok = l.Role == RoleBuyer && g.Players[s].Money >= l.Pos
			}
		} else {
			ok = g.canSell(s)
		}
		if !ok {
			continue
		}
		if best < 0 {
			best = s
			continue
		}
		bl := &a.Lines[best]
		better := l.Pos > bl.Pos
		if role == RoleSeller {
			better = l.Pos < bl.Pos
		}
		if better || (l.Pos == bl.Pos && g.Players[s].Rank > g.Players[best].Rank) {
			best = s
		}
	}
	return best
}

func step(pos, target, st int) int {
	switch {
	case pos < target:
		return min(pos+st, target)
	case pos > target:
		return max(pos-st, target)
	}
	return pos
}

// tickAuction runs one jiffy of a land or goods auction.
func (g *Game) tickAuction() {
	a := g.Auction
	switch a.Stage {
	case StageShow:
		a.StageLeft--
		// The plot-for-sale display ends early when every human presses. [measured]
		if a.StageLeft <= 0 || (a.Land && g.allReady()) {
			if a.Land {
				a.Stage = StageActive
				a.TickLeft = ATU
				g.applyHeldPushes()
			} else {
				a.Stage = StageDeclare
				a.TickLeft = ATUIdle
				for s, p := range g.Players {
					if p.AI {
						a.Lines[s].Role = g.brain.Declare(g, s, a.Good)
						p.Declared = true
					}
				}
			}
		}
		return
	case StageDeclare:
		// 30 ticks at the idle rate (nobody moves a line while declaring). [measured 180 jiffies]
		a.TickLeft--
		if a.TickLeft <= 0 {
			a.Ticks--
			a.TickLeft = ATUIdle
			if a.Ticks <= 0 {
				a.Stage, a.StageLeft = StageLead, DeclareLead
			}
		}
		return
	case StageLead:
		a.StageLeft--
		if a.StageLeft <= 0 {
			a.Stage, a.Ticks, a.TickLeft = StageActive, TradeTicks, ATU
			for s := range a.Lines {
				l := &a.Lines[s]
				switch l.Role {
				case RoleBuyer:
					l.Home = a.Lo
				case RoleSeller:
					l.Home = a.Hi
				}
				l.Pos, l.Target = l.Home, l.Home
			}
			g.setPhase(PhaseTrading)
			g.applyHeldPushes()
		}
		return
	case StageDone:
		a.StageLeft--
		if a.StageLeft <= 0 {
			if a.Land {
				g.nextLandAuction()
			} else {
				g.startGoods(g.GoodIdx + 1)
			}
		}
		return
	}

	for s, p := range g.Players {
		if p.AI {
			g.brain.AuctionTick(g, s)
		}
	}
	if !a.Land {
		g.critTick()
	}
	// Lines step every LineStep jiffies. [measured: 2 scanlines per 8 jiffies]
	if g.T%LineStep == 0 {
		g.moveLines()
	}
	contact := false
	if a.Land {
		if a.Seller >= 0 {
			a.Ask = a.Lines[a.Seller].Pos
			b := g.bestLine(RoleBuyer, true)
			if b >= 0 && a.Lines[b].Pos >= a.Ask {
				a.Winner, a.Price = b, a.Ask
				g.finishLandAuction()
				return
			}
		}
	} else {
		contact = g.trade()
	}
	// The time bar: one tick per ATU while any line in the auction is being
	// pushed, ATU/4 while nobody pushes; stopped while a trade is in contact. [code $9CE6]
	if !contact {
		a.TickLeft--
		if a.TickLeft <= 0 {
			a.Ticks--
			a.Idle = !g.anyPushing()
			a.TickLeft = ATU
			if a.Idle {
				a.TickLeft = ATUIdle
			}
		}
	}
	if a.Ticks <= 0 {
		if a.Land {
			g.finishLandAuction()
		} else {
			g.finishGoodsAuction()
		}
	}
}

// anyPushing reports whether a participating line is being pushed.
func (g *Game) anyPushing() bool {
	a := g.Auction
	for s := range a.Lines {
		l := &a.Lines[s]
		if l.Role == RoleNone || !l.In {
			continue
		}
		// Humans push with the stick; computers push while heading for a target.
		if l.Push != 0 || (g.Players[s].AI && l.Target != l.Pos) {
			return true
		}
	}
	return false
}

func (g *Game) moveLines() {
	a := g.Auction
	for s := range a.Lines {
		l := &a.Lines[s]
		if l.Role == RoleNone || a.Crit == s {
			continue
		}
		np := clamp(step(l.Pos, l.Target, a.Step), a.Lo, g.lineTop(s))
		if a.Land {
			if l.Role == RoleSeller {
				np = max(np, a.Start)
			}
		} else if l.Role == RoleBuyer {
			np = min(np, g.buyerCeiling(s))
		} else {
			np = max(np, g.sellerFloor(s))
		}
		l.Pos = np
		if a.Land {
			l.In = l.Role == RoleSeller || l.Pos >= a.Start
		} else if l.Pos != l.Home && !l.In {
			l.In = true
			if l.Role == RoleSeller && g.Critical(s, a.Good) > 0 && !g.canSell(s) {
				if a.Crit < 0 {
					g.critical(s)
				} else {
					// Someone else is being told: this one just stays out.
					l.Pos, l.Target = l.Home, l.Home
				}
			}
		}
	}
}

// buyerCeiling is the highest price a buyer line may reach.
func (g *Game) buyerCeiling(seat int) int {
	a := g.Auction
	c := a.Hi
	if !a.colluding() && g.Store.Stock[a.Good] > 0 {
		c = a.StoreSell
	}
	for s := range a.Lines {
		if s != seat && g.canSell(s) && a.Lines[s].Pos < c {
			c = a.Lines[s].Pos
		}
	}
	return c
}

// sellerFloor is the lowest price a seller line may reach.
func (g *Game) sellerFloor(seat int) int {
	a := g.Auction
	f := a.Lo
	if !a.colluding() {
		f = a.StoreBuy
	}
	for s := range a.Lines {
		if s != seat && g.canBuy(s, a.Lines[s].Pos) && a.Lines[s].Pos > f {
			f = a.Lines[s].Pos
		}
	}
	return f
}

// trade finds the current contact (buyer meets seller, buyer at the store's
// sell line, or seller at the store's buy line) and trades one unit when its
// delay runs out. Units in one contact come after 12, 12, 10, 8, 6, 4, 2, 2...
// BTU. [measured, code $912E] Returns whether lines are in contact.
func (g *Game) trade() bool {
	a := g.Auction
	gd := a.Good
	b := g.bestLine(RoleBuyer, false)
	s := g.bestLine(RoleSeller, false)
	price, buyer, seller := 0, -1, -1
	switch {
	case b >= 0 && s >= 0 && a.Lines[b].Pos >= a.Lines[s].Pos:
		price, buyer, seller = a.Lines[s].Pos, b, s
	case b >= 0 && !a.colluding() && g.Store.Stock[gd] > 0 && a.Lines[b].Pos >= a.StoreSell:
		price, buyer = a.StoreSell, b
	case s >= 0 && !a.colluding() && a.Lines[s].Pos <= a.StoreBuy:
		price, seller = a.StoreBuy, s
	default:
		a.Contact = -1
		return false
	}
	key := (buyer+1)*8 + seller + 1
	if key != a.Contact {
		a.Contact, a.ContactN, a.TradeWait = key, 0, TradeFirstBTU
		a.Recent = a.Recent[:0]
		a.NextTrade = g.T + TradeFirstBTU*BTU
		return true
	}
	if g.T < a.NextTrade {
		return true
	}
	if buyer >= 0 {
		g.Players[buyer].Money -= price
		g.Players[buyer].Goods[gd] = min(MaxStock, g.Players[buyer].Goods[gd]+1)
		a.Lines[buyer].Units++
	} else {
		g.Store.Stock[gd]++
	}
	if seller >= 0 {
		g.Players[seller].Money += price
		g.Players[seller].Goods[gd]--
		a.Lines[seller].Units--
	} else {
		g.Store.Stock[gd]--
	}
	a.Traded++
	a.TradeSum += price
	a.LastTrade = price
	a.ContactN++
	if len(a.Recent) >= 16 {
		a.Recent = append(a.Recent[:0], a.Recent[1:]...)
	}
	a.Recent = append(a.Recent, TradeRec{T: g.T, Buyer: buyer, Seller: seller, Price: price, N: a.Traded})
	if a.ContactN >= 2 {
		a.TradeWait = max(TradeMinBTU, a.TradeWait-2)
	}
	a.NextTrade = g.T + a.TradeWait*BTU
	// A line that can no longer trade goes back to its starting edge. [measured]
	for _, x := range []int{buyer, seller} {
		if x < 0 {
			continue
		}
		l := &a.Lines[x]
		if l.Role == RoleSeller && !g.canSell(x) && a.Crit < 0 && l.Floor < g.Critical(x, gd) &&
			g.Players[x].Goods[gd] == g.Critical(x, gd) {
			g.critical(x)
			continue
		}
		if (l.Role == RoleBuyer && !g.canBuy(x, price)) || (l.Role == RoleSeller && !g.canSell(x)) {
			l.Pos, l.Target = l.Home, l.Home
			a.Contact = -1
		}
	}
	return true
}

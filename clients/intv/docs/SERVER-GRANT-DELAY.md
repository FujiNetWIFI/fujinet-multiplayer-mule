# Proposal: a reaction delay for computer players in the land grant

> **Superseded (2026-09-23).** The 8-jiffy delay was too short for a FujiNet round trip, and holding the cursor made our cursor run ahead and snap back. The server now judges a human claim as of the press instead: `Claim` accepts a plot the cursor left up to `GrantLatency` (36) jiffies ago, and takes the plot from a computer that claimed it within `GrantLatency` of the cursor arriving. Computers claim on arrival again. See `Claim` in `internal/rules/game.go`.

Proposed by the Intellivision client session (2026-09-23), for the session that owns the server (`internal/rules`, `internal/ai`). Nothing in the server has been changed.

## Summary

- **The problem.** In the land grant a computer always gets a plot it wants before any human. It claims in the same server tick that the cursor lands on the plot, so no human claim can ever get there first.
- **The proposal.** Delay each computer's claim by a few jiffies after the cursor arrives, and let a human claim made in that window win.
- **What it touches.** `tickGrant`, `claim` and `Claim` in `internal/rules/game.go`, one new constant, and a new test.

## What happens now

In `tickGrant` (`internal/rules/game.go`, about line 203):

- **Computers claim on arrival.** Each step moves the cursor, and in the same call every computer whose `grantWant` is the new plot claims it through `g.claim(best, next)`. The comment calls this `[RE]`: "Computers press their button as soon as the cursor reaches their plot".
- **What a human's claim needs.** `Claim` accepts a plot only if it is `GrantCur`, or `GrantPrev` within `GrantLatency` (24 jiffies), and only if it is still `grantable`.
- **Why the computer always wins.** A human's press happens after they see the cursor on the plot, so it always arrives after that step's tick. By then the plot is owned, `grantable` is false, and the claim fails with `ErrPlot`.

**Other details:**

- **The computers aim for the same plots.** `GrantChoice` (`internal/ai/ai.go`) has each computer aim for the best plot left, by best use, distance from the store and row, plus 0–3 of randomness. Those are the plots a human wants too.
- **The pause looks like lag.** After every claim, `GrantClaimPause` (48 jiffies) holds the cursor. Clients learn about it only from the next state, so to a human the cursor seems to stutter.

## Measured on the Intellivision client

The client's own timing bugs are fixed:
- **Claims:** it now claims the plot that was under the cursor when the button went down, even if the main loop is busy with a network request at that moment.
- **Cursor:** it runs at most one plot ahead of the server's cursor.
- **Early claims:** it holds a claim that arrived too early and re-sends it when the cursor gets there.

Against a private server (table `ai3`, one human and three computers), the automated player pressed as the cursor reached a target plot:
- **Plot free:** every claim for a plot no computer wanted was won.
- **Plot wanted by a computer:** every claim was lost. For example, in round 1 the press was made with the target plot on screen. The claim reached the server before its cursor got there and was held, then re-sent when the cursor arrived. But seat 3 had already claimed the plot in the tick the cursor landed on it.

Players read this as "the computer always wins". In practice they can only take plots the computers don't want at that moment.

## Whether the Atari really does this

The current behaviour is marked `[RE]`. Worth checking on the Atari before changing it:

- **Test:** in round 1, with one human and three computers, have the human press on the frame the cursor reaches a plot that a computer then takes.
- **Tool:** `tools/extract/capture/cap.c`. Its `trace` can log the frame on which the plot's owner byte changes.
- **If the Atari also gives the computer every tie:** the delay below is still worth having as an option for network play. A human claim spends its reaction time *plus* a network round trip. The fastest possible human claim takes about 10–20 frames (FujiNet, polling every 12 frames). On an Atari at the console it would take about 12.

## Proposed change

A new constant in `internal/rules/constants.go`:

```go
AIGrantDelay = 8 // [IMPL] jiffies a computer waits after the cursor lands before it claims;
                 // a human claim in that window wins. Less than CTU, so the cursor never
                 // moves on first.
```

In `tickGrant`, split the computers' claim out of the step:

```go
// State on Game: the computer about to claim, and when.
//   grantAISeat int  (-1 none)
//   grantAIDue  int  (jiffy)

func (g *Game) tickGrant() {
	if g.GrantEnd >= 0 { ...unchanged... }

	// A computer's claim that has waited out its reaction time.
	if g.grantAISeat >= 0 && g.T >= g.grantAIDue {
		s, p := g.grantAISeat, g.GrantCur
		g.grantAISeat = -1
		if g.Claimed[s] < 0 && g.grantable(p) {
			g.claim(s, p)
		}
	}

	if g.T < g.GrantNext {
		return
	}
	// Step the cursor ...unchanged up to choosing `best`...

	if len(want) > 0 {
		// was: g.claim(best, next)
		g.grantAISeat, g.grantAIDue = best, g.T+AIGrantDelay
	}
}
```

Supporting changes:

- **The cursor waits for the computer.** `CTU` is 12 and the delay is 8, so the claim is decided before the next step. Guard it anyway: at the start of the step block, `if g.grantAISeat >= 0 { return }`.
- **A human claim cancels the computer's.** In `claim` (or in `Claim` before calling it), a human taking the plot clears the pending computer claim:

  ```go
  if g.grantAISeat >= 0 && plot == g.GrantCur {
  	g.grantAISeat = -1
  }
  ```

  The human's plot is then no longer `grantable`, so the computer re-picks through `grantWant` (claim already resets `grantWant` for that plot to -2) at the next step.
- **Reset the pending claim** (`grantAISeat = -1`) wherever the grant starts, next to `g.GrantCur, g.GrantPrev, g.GrantEnd = -1, -1, -1`.
- **Rank rule.** When several computers want the plot, `best` is still the worst-ranked, as now. A human who presses within the delay beats all of them.
- **Protocol.** Nothing new needs sending. States still show the plot as free until the computer claims it, and `GRANT_NEXT` is unchanged.

## Tuning

- **Default of 8 jiffies (133 ms).** It's short enough that computers still look instant to a person watching. It's long enough for a human who presses as the cursor arrives, having anticipated it, to win on a local server.
- **Network play.** On a slow network even 8 won't be enough. A per-table setting (for example `?aidelay=`) or a larger value for tables with human players would help.
- **Keeping the Atari rule.** Setting it to 0 gives today's behaviour, which suits a table that wants the Atari rule exactly.

## Test

A case in `internal/rules/rules_test.go`:

1. Set up a grant with a human and a computer whose `GrantChoice` is plot P.
2. Step to the tick where the cursor reaches P.
3. Call `Claim(human, P)` at `T + AIGrantDelay - 1`.
4. Check that the human owns P, and that the computer claims another plot at a later step.

A second case: no human claim, and the computer owns P at `T + AIGrantDelay`.

## Two other server items from the same session

- **Land bids stop at the opening price + 35 steps.** `a.Hi` clamps land buyers in `SetTarget` and `moveLines`. The Atari lets bidding go on until the bidder's money runs out (checked on atari800), and scrolls the price scale. The fix is to cap land buyers at `Money - Money%LandStep` instead of `a.Hi`, and keep `a.Hi` for the seller's line.
- **A finished table didn't restart.** After a game over on table `ai3`, the human was ready (`rd: true`) but the lobby countdown stayed at `cd: -1` and the game never began. A fresh server started normally.

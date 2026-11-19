---
id: LI-02
title: "Game clocks and Lichess speeds"
type: task
status: open
priority: medium
---

# LI-02 Game clocks and Lichess speeds

<!-- migrated from TASKS.md:258 (Lichess Improvement Backlog) -->

- [ ] LI-02 Game clocks and Lichess speeds [Medium] — plan: [`docs/LI-02-ClockPlan.md`](docs/LI-02-ClockPlan.md). A Lichess-like clock in **every mode** (Lichess games, two players, vs Robot): `MM:SS`, tenths under 10 s, red when low, a low-time sound, the flag loses (a draw if the other side can't mate); for Lichess from the stream's `wtime`/`btime`, plus the correspondence deadline if the Board API gives it. The challenge card names the modes (Rapid, Classical) with the player's rating, and an optional Lichess username makes Blitz possible (a direct challenge). The Board API allows only Rapid, Classical and Correspondence online, Blitz only in a challenge to a named user (checked 2026-11-02), so Bullet and UltraBullet are for local and robot games only. Doesn't depend on LI-01. Steps: 0 plan (#118); 1 `GameClock` + `CanMate` with tests (#119); 2 clocks in local and robot games (#120); 3 the Lichess clock (#121); 4 the challenge card (modes, rating, Blitz by username, #122); 5 docs and hand tests (done, branch `li02Docs_041126`). **Coded; the hand tests are open:** [`docs/LI-02-ManualTests.md`](docs/LI-02-ManualTests.md).

## Resolution

### 2026-11-03 — LI-02 plan: game clocks and Lichess speeds (branch: li02Plan_031126)

#### What was done
- Plan [`docs/LI-02-ClockPlan.md`](docs/LI-02-ClockPlan.md), with the owner's choices: a Lichess-like clock in every mode (Lichess, two players, vs Robot); red when low, tenths under 10 s, a low-time sound, the correspondence deadline if the Board API gives it; Blitz online through a challenge by Lichess username. `TASKS.md`'s LI-02 rewritten (Bullet/UltraBullet: local only; no dependency on LI-01).

#### Next
- Step 1: `GameClock` (fake-clock tests) and ChessCore's `CanMate`.

### 2026-11-03 — LI-02 step 1: GameClock and CanMate (branch: gameClock_031126)

#### What was done
- **`GameClock`** (`gameclock.h/.cpp`, new, not used by the app yet): the two clocks of a game, built like `MoveGate` with a time source tests can replace. `moveMade()` for local games, `setSnapshot()` for Lichess's times or a saved game, `setTurn()` after a takeback, `pause()`/`resume()`. It runs only after both sides have moved; the increment goes to a mover whose clock ran. `format()`: `03:00`, `1:30:00`, `00:09.4`, `3 days`. `flagged()` only for local games. `lowTime()` once below Lichess's line (an eighth of the start, 10 s to 60 s), again only above 1.5 times the line and after 20 s. The timer wakes only when the shown time changes.
- **`Board::CanMate(side)`** (ChessCore): the rule of Lichess's `InsufficientMatingMaterial` (checked against scalachess): false for a lone king, K+N against a king and nothing but queens, and bishops on one square colour unless the other side has a pawn, a knight or a bishop on the other colour. It decides whether a flag loses or draws.
- **Review of #119**: a Lichess clock at zero no longer wakes its timer every millisecond; a move or takeback after the time ran out no longer cancels the flag (the timer can be late); the low-time sound only while that side's clock runs, so never during a paused Continue; K+N against queens only is a draw.
- **Tests**: `GameClockTest` (15), `MatingMaterialTests` (8). Release builds; ChessTests + ChessQtTests **265 pass**. No UI change, so no screenshots.

#### Next
- Step 2: clocks in local and robot games.

### 2026-11-04 — LI-02 step 2: clocks in local and robot games (branch: localClocks_031126)

#### What was done
- **The picker**: `TimeControlPicker.qml` (new) in both setup dialogs: Lichess's quick-pairing tiles, 1+0 to 30+20, and No clock (the default). The last choice is kept in `AppSettings`.
- **The clock on the cards**: `PlayerCard` shows it at the right, lit while it runs, red when low, the same width at any time. `ChessConnector.Clock` is the `GameClock`; `startComputerGame` and `startLocalGame` take `limitSec` and `incSec`, and a rematch keeps them.
- **The moves**: `localMoveOnBoard()` switches the clock after the player's and the robot's move. A takeback vs the robot keeps the time spent, takes off the increments of the moves taken back and runs the side to move's clock. Review doesn't stop it.
- **A flag**: `onClockFlagged()`: "<side> ran out of time" with the flagged king, or "Draw: <side> ran out of time" when the other side can't mate. It ends the game for good: no takeback (the Undo button is disabled), as on Lichess. The robot's answer on its way is dropped.
- **Stalemate (new for local games)**: `Game` only reports a mate, so a stalemated side would have lost on time. `localMoveOnBoard()` now ends the game as "Draw by stalemate" (`Game::IsStalemate()`), with or without a clock. Before, a stalemate just left the game sitting there.
- **Continue**: the autosave keeps the time control and both times (`GameSaveInfo`). The clock is paused until the page has been open for a second (`pageOpened`).
- **The robot**: `GameClock::moveBudgetMs()` caps levels 3 to 5 at a thirtieth of its time plus half the increment, at least 0.1 s, never more than half of what's left (`findMove`, `FindMove`). Levels 1 and 2 search by depth and are quick anyway.
- **Low-time sound**: `sounds/lowtime.wav`, drawn by `scripts/make_sounds.py`.
- **Checked against Lichess's source** (`clockView.ts`, `clockCtrl.ts`), as the plan asked: hours are padded (`01:30:00`, was `1:30:00`), and below a minute's start the low line is a fifth of it, at least 2 s. Both fixed. The rest matched. Not copied: hundredths on a stopped clock under a second.
- **Scenes**: `clock`, `clock-low`, `clock-flag`, `clock-promotion`, `clock-continue`, `stalemate`, `players-many`; `computer` and `players` show the picker. Time stands still in screenshot mode; `setScreenshotClock()` sets what a scene shows.
- **Tests**: `GameClockTest` (`MoveBudget`, the line, `timeUp`), `FindMove_StaysWithinTheClock_Test`, the autosave's clock fields. Release builds; ChessTests + ChessQtTests **273 pass**. All scenes light and dark: only `computer` and `players` changed, baselines accepted.
- **Real time, once**: a throwaway build without the freeze showed the clock ticking, tenths, a flag by the real timer and Continue resuming. Not committed.
- **Review of #120**:
  - A flag no longer outlives its game: `stop()` forgets it, so a next game without a clock keeps its Undo.
  - Review could flag the robot for free, since the gate held its answer while its clock ran. Now the answer brings the board back to the live position, as a Lichess opponent's move does, and entering a review plays a held answer (`showHeldMoves`). Checked with the real engine in a throwaway scene.
  - A flag during the piece choice: the pawn goes back and the picker closes (`clock-promotion`). The move used to complete after the result.
  - Move and Undo over and over added increments for ever. Lichess gives the time back on a takeback (`Rewind.scala`), but there the opponent must agree; here that would be free thinking time, so only the increments go (`moveTakenBack`).
  - Continue then Review in the first second froze the clock: it now starts from `pageOpened()`, not from the board settling.
  - An engine failure pauses the clock instead of giving the player a win on time.
  - `Game::IsStalemate()` replaces the connector's own check (the rule belongs in `Chess::`); `endGameWithLoser()` and `dropRobotAnswer()` replace the copies.
  - The two-player dialog scrolls when many opponents would push Start out of a small window (`players-many`). The clock pill grows for a longer time.

#### Not checked yet
- Nobody has heard `lowtime.wav`. A whole robot game with a clock hasn't been played (step 5's hand tests).
- The robot can still flag: the gate's 0.4 s between the player's move and its answer counts on its clock, which matters only in a very long game without increment, and levels 1 and 2 aren't capped.
- `settings`, `about` and `restore` sometimes differ by a line: the Settings page's engine lookup races the shot (not from this work).

#### Next
- Step 3: the Lichess clock.

### 2026-11-04 — LI-02 step 3: the Lichess clock (branch: lichessClock_041126)

#### What was done
- **The stream**: `LichessClient` now reads the clock: `clockStarted(limitMs, incrementMs, correspondence)` after `gameStarted`, `clockUpdated(whiteMs, blackMs, whiteToMove, running)` after the moves of every gameFull and gameState. A real-time clock runs from the second move, and the state that ends the game stops it with the final times.
- **Our move** switches the clock at once (`moveOnBoard()`, was `localMoveOnBoard()`), and Lichess's next state brings the exact times. A state without our sent move yet is older than our board, so it's skipped.
- **A refused move**: the list is replaced as before, and the clock goes back to the side to move without its increment, because no state follows. Any other list comes with Lichess's exact times.
- **Low time**: our own side only. Rejoining with little time left, the sound waits until the page has opened (`GamePage`); checked in a throwaway run.
- **Correspondence**: the plan's question is answered from Lichess's source (`BotJsonView.scala`): `wtime` and `btime` are the time left for the move, so `secondsLeft` isn't needed. A game with no clock at all sends the largest int, which is ignored. `GameClock::startCorrespondence()` runs from the start, is never low and makes no sound, as `corresClockView.ts`.
- **Correspondence format differs from the plan**: Lichess shows `2 days 5 hours`, `One day`, `05:30` (hours and minutes), `45:10`, without tenths. `formatCorrespondence()` follows it; `format()` no longer knows days.
- **Scenes**: `clock-online` (the opponent low, our clock lit), `clock-correspondence`.
- **Tests**: 6 in `GameClockTest`, 8 in `LichessClientTest`. Release builds; ChessTests + ChessQtTests **287 pass**. All other scenes unchanged.
- **Review of #121**: the clock's own estimate now runs only after a refused move. On every list change it could take increments off times that were already exact, and play a false low-time sound that then stayed silent at the real crossing. The rejoin sound waits for the page instead of being dropped for the rest of the game. A correspondence clock no longer shows `72:00:00` for a moment.

#### Not checked yet
- Nothing has run against the real Lichess: step 5's hand tests. Open there: does Lichess end a flagged game by itself (the Board API has no flag call; until then the clock stays at `00:00.0`), and how far the shown time drifts from Lichess's.
- Our move switching the clock has no test: `ChessConnector` has none, and a scene can't make a move without the network.
- `players` failed once in a full run (a shift of about a pixel), then passed 6 of 6.
- Left from the review, on purpose:
  - A move POST lost without an answer (a timeout) leaves our board and clock waiting for an echo that never comes. The board is out of sync too, so that needs its own fix (a resync), not a clock one.
  - The stream closing mid-game: there's no reconnect yet, and the clock goes on counting. Lichess's time does go on, so that's true as far as anyone can know.
  - After a refused move the round trip is charged to the opponent until the next state: under a second normally, and refusals are rare.
  - The clock switches when Lichess's state arrives, up to 0.4 s before the opponent's move has slid in. Holding it back would show us more time than we have.

#### Next
- Step 4: the challenge card.

### 2026-11-04 — LI-02 step 4: the challenge card (branch: challengeCard_041126)

#### What was done
- **The picker on the card**: `TimeControlPicker` gets `online` (no Bullet, no No clock) and `blitzAllowed`. Blitz tiles are dim until a username is entered, with a line saying why. Real time is now the card's default, not Unlimited, so the tiles show first.
- **The rating**: "Your Rapid rating: 1688" for the chosen mode, also Correspondence; nothing for Unlimited or a pool without a rating.
- **By name**: an optional "Lichess username" field. `LichessClient::challengePlayer()` sends a direct challenge through the rematch's `challengeUser()` and its wait for the game. The Create button then says "Challenge <name>", and stays off for a Blitz tile with no name.
- **The rematch's code assumed every direct challenge was a rematch**: the opponent's name came from the last game, so a new challenge would have named the wrong player, and the dialog, the declined message and the errors all said "rematch". `_directOpponentName` holds who it was sent to, and `_pendingRematch` is now true only in a game, as for a link.
- **Usernames**: `isUsername()` (Lichess's `[a-zA-Z0-9_-]{2,30}`) before the name goes into the URL. The test caught `$` letting a trailing newline through: `anchoredPattern`.
- **Scenes**: `online-blitz`, `online-challenge`; `online` and `online-empty` show the new card.
- **Tests**: 3 in `LichessClientTest`. Release builds; ChessTests + ChessQtTests **290 pass**.
- **Review of #122**:
  - A challenge by name with days per move or unlimited no longer needs the app open: Lichess keeps those two weeks (only a clock's `timeControl.l` is swept after 20 s), so we don't hold a request. `directChallengeLeft`: a message, and the game comes up under Your games. A correspondence rematch goes the same way.
  - `open` and `ai` aren't usernames: they're other calls next to `/api/challenge/:user`, and `open` would have created an open challenge.
  - A late answer to a cancel no longer closes a newer challenge. A challenge by name no longer deletes a saved link. The card can't stay busy after a decline.
  - The arrow keys skip dim tiles. The screenshot hooks left the page: the scene finds its items by `objectName`.

#### Not checked yet
- Nothing has run against the real Lichess (step 5): a challenge by name, Blitz, declined, no such player, days per move by name. `challengePlayer()` past the name check has no test: it needs the network.
- The challenges are casual, as links and rematches always were here: `rated=false`. The card shows a rating, but these games don't change it.

#### Next
- Step 5: docs and hand tests.

### 2026-11-04 — LI-02 step 5: docs and hand tests (branch: li02Docs_041126)

#### What was done
- **`docs/LI-02-ManualTests.md`**: 15 scenarios, 7 on this computer and 8 on the real Lichess.
- **Scene `clock-flag-draw`**: the draw on a flag against a lone king, by a real 32-ply game, so nobody has to play one by hand. The last path of step 2 that had never run.
- **Docs**: `CLAUDE.md` (`GameClock`, `TimeControlPicker`, the tests, the challenge), the `run-chess-app` skill (the new scenes, the settings flake, the clock in screenshot mode).

#### LI-02 is coded (#118–#122); open
- The owner's hand tests. Two questions only they can answer: does Lichess end a flagged game by itself (the Board API has no flag call), and how far the shown time drifts.
- A move lost on the way (a POST that times out and never reached Lichess) leaves the board and the clock waiting for an echo: a board resync, its own task if the owner wants it.

#### Next
- The owner's choice: the hand tests, then LI-01, BL or the next item in `TASKS.md`.

---
id: BL-18
title: "continuing a Lichess game shows the start position first"
type: bug
status: open
priority: medium
---

# BL-18 continuing a Lichess game shows the start position first

<!-- migrated from TASKS.md:94 (In Progress / Backlog) -->

- [ ] BL-18 **Bug**: continuing a Lichess game shows the start position first [Medium] — **Fixed (branch `bugs/continueBoardFlash_101126`): `gameStarted` carries the moves, played before the page opens; scene `rejoin`; left: the owner's check with a real game.** the owner (2026-11-10): opening an unfinished Lichess game (Continue on Home or in the Continue list, Continue on the Online page) shows the board in the start position, and 50–500 ms later the game's real position. **Expected**: the game's position shows at once, without the board being built twice (no start position, no slides or sounds for the moves already played). Likely cause (not confirmed): on the stream's first `gameFull`, `LichessClient` emits `gameStarted` first, so `ChessConnector::onLichessGameStarted` → `startOnlineGame` sets up the start position and `route()` opens the game page; only then come the moves already played (`opponentMoveReceived`), and they go through the `MoveGate`, which holds them until the page has settled (`setStageReady`), then shows them as a resync. To do: give the game its moves before the page opens (e.g. `gameStarted` with the moves, or `startOnlineGame` playing them before `gameInfoChanged`), not through the gate; check the same for a new challenge accepted mid-game and for a rejoin by link; the clock and premove still right; a screenshot scene that opens a rejoined game with moves (its first frame must show the position), and a check of the app log's frame trace that the board isn't reset twice.

## Resolution

### 2026-11-10 — BL-18: continuing a Lichess game showed the start position first (branch: bugs/continueBoardFlash_101126)

#### What was done
- **The cause** (as suspected in `TASKS.md`): on the stream's first `gameFull`, `LichessClient` emitted `gameStarted`, so `ChessConnector::startOnlineGame` set up the start position and the game page opened; the moves played before then came after it (`opponentMoveReceived`) and waited in the `MoveGate` until the page had settled, then showed as a resync.
- **The fix**: `gameStarted` carries the game's moves so far (`QStringList moves`, empty for a new game), and they no longer come again as `opponentMoveReceived`. `ChessConnector::playJoinedMoves` puts them on the board in `startOnlineGame`, before `setGameMode` opens the page: the first frame shows the game's position, without slides, move sounds or "Check" for them (the page tells a check in the position it opens on, as for a resumed robot game). The clock is off while they're played (it starts after, from Lichess's times). The same path serves Continue (Home, the list, the Online page), Join by link, an accepted challenge and a rematch (those two start with no moves).
- **Tests**: the stream tests expect the moves with the start (`GameStream_GameFull_MovesComeWithTheStart_Test` replaces the resync-in-one-signal test; the clock tests: `started e2e4 e7e5`, then the clock, then the times). ChessQtTests **222 pass**.
- **Screenshots**: new scene `rejoin` (in `all`): a game joined with ten moves, grabbed as soon as the page is up (no `--delay` for it): the position after 5… a6, not the start position. The other Lichess scenes (fed through the same start) still match.
- **The owner's `/code-review medium` of #146: 9 findings, all fixed** (same branch):
  - A joined game that ended in mate was recorded and told as the previous game's (vs Robot, "Robot wins by checkmate"): the game's mode and opponent are set before its moves are played (`setGameMode()`, which opens the page, comes after them). Its mate's sound and result aren't announced before the page (`checkMateNotify` / `checkMateResult` quiet while `_resuming`).
  - On a game page that was open already, the board never became idle again, so a check (or a waiting result) in the joined position wasn't told: the joined moves make the gate busy, then idle (`_gate->moveShown()`), and `GamePage` takes a new game's position as the one it "opened" on (`onNewGameStarted`).
  - A joined move that can't be played here left the board out of step with Lichess: the game isn't opened then ("Couldn't continue the game: Lichess sent a move Chess++ can't play"; `endGame()` stops the stream).
  - A new game whose opponent moved before our stream opened showed that move without a slide, a sound or the premove: only a game with two moves or more counts as joined; zero or one move comes through the gate as the opponent's, as before.
  - One move loop: `playMoves(moves, announce)` (quiet for joined moves) instead of `playJoinedMoves`.
  - The `rejoin` scene is ready with the first frame drawn after the join (`frameSwapped`), and its delay of 0 is the script's (an explicit `-Delay` still wins), not a special case in `main.cpp`.
  - A stream test waited for one event too many (it sat out its 5 s timeout and sometimes caught the stream's silence warning): fixed. ChessQtTests **222 pass**; the scenes `rejoin`, `takeback`, `premove-played`, `opponent-left-claim`, `draw-offer`, `clock-online` checked one by one.

#### Next
- The owner's check with a real Lichess game: Continue an unfinished game with moves (from Home, the list, the Online page): no start position first, no move sounds.

---
id: BL-09
title: "Engine move off the GUI thread's game"
type: task
status: done
priority: medium
---

# BL-09 Engine move off the GUI thread's game

<!-- migrated from TASKS.md:88 (In Progress / Backlog) -->

- [x] BL-09 Engine move off the GUI thread's game [Medium]: `ChessEnginePlayer::DoMove` runs on the engine thread and calls `_game->DoMove()`, while the GUI thread reads the same `Game` (captured pieces, `MaterialBalance`, `LastMoveFrom/To`, review replays). Clicking a move or pressing ← during the robot's turn can read the move history while it reallocates. Fix: the engine thread only finds the move (`EngineWorker` returns the UCI string); `ChessConnector` plays it on the GUI thread. Found in the P4 code review (#81). → done: `ChessEnginePlayer::FindMove(history)` gets a copy of the moves, `ChessConnector::onEngineMoveFound` plays the move; a request number drops answers for an old game or position.

## Resolution

### 2026-10-29 — BL-09 engine move on the GUI thread done (branch: engineThreadMove_291026)

#### What was done
- **The engine thread no longer touches the app's `Game`.** Before, `ChessEnginePlayer::DoMove` ran on the engine thread and called `_game->DoMove()` while the GUI thread read the same game (captured pieces, material, last move, review). Now:
  - `ChessEnginePlayer::FindMove(history)` gets a copy of the move list and returns the engine's move in UCI. For FEN it replays the moves on its own `Game`. `DebugDump(history)` works the same way.
  - `EngineWorker::findMove(request, history)` runs on the engine thread and answers `moveFound(request, move)` or `moveError(request, …)`.
  - `ChessConnector::askEngineMove()` (one place for the robot's turn: after the player's move, a new game with the robot as White, a takeback, Continue) sends a copy of the moves. `onEngineMoveFound()` plays the move on the GUI thread. A move that can't be played goes the error path, with the FEN in the log.
  - A request number (`_engineRequest`) drops an answer for an old game or position: `stopEngineThread()` bumps it. `_gameOver` is a plain `bool` again: only the GUI thread uses it.
- **Tests:** `ChessQtTests/ChessEnginePlayerTest.cpp` (6 tests, real Stockfish like `UciConnectorTest`): mate in one found in StartPos, PureFen and FenWindow (12 plies, so a FEN plus moves); the dump; the worker returns the request number; an unresolved promotion is an error with the moves in the dump. All 157 tests pass.
- **Screenshot scene `robot`** (not in `all`: needs Stockfish): a level 5 game where the player is Black. The robot's first answer is dropped (`endGame()` while it thinks), then Continue: the robot mates with Qh5#, and the shot waits for the engine. All 42 baseline scenes still match.

#### Where to continue
Next backlog candidates: BL-05 (stub engine, Quick tests: the `robot` scene would then run without Stockfish), BL-06 (engine choice), BL-04 (dragging pieces). The manual checks for BL-02 (P1–P6) are still open in TASKS.

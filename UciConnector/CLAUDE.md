# UciConnector/

Talks to an external UCI engine (Stockfish) through `QProcess`: a static library, Qt Core only. The robot's player
on top of it, `ChessEnginePlayer`, and its thread, `EngineWorker`, are in `ChessGui/ChessGui/`.

- API: `Init()` (also the restart after the engine exited: Stockfish 18+ quits on an illegal move in
  `position … moves`), `NewGame()`, `GetEngineMove(StartPosMoveRequest)` (`Fen` empty → `position startpos`,
  then `Moves`), `GetCandidateMoves` (MultiPV, full strength to a depth: the robot's levels pick one with `PickWeakMove`; the
  analysis), `SetDifficulty(elo)` (clamped to
  1320–3190), `PickWeakMove`, `Kill()` (from another thread, through the process's own Win32 handle),
  `DebugState()` (for bug reports, Release too).
- `SetChess960(on)` (LI-01A): `UCI_Chess960`, sent only when it changes something; it throws for an engine
  without the option when `on`.
- The engine (`EngineCandidates()`): `./engines/stockfish.exe` beside the app, else one on `PATH`; a candidate
  that fails to start falls through to the next. `CHESS_ENGINE_PATH` names the only one instead: the tests'
  FakeUci, never Stockfish then.

## Invariants
- A Chess960 game's request is always its start FEN and every move since, whatever the `PositionMode`
  (`ChessEnginePlayer::MakeRequest(history, plies, mode, start)`; `ChessEnginePlayer`, `EngineWorker` and the
  analysis get the game's `StartPosition`).
- The tests of this library are `ChessQtTests/UciConnectorTest.cpp` (they need Stockfish).

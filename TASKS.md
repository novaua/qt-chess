# Tasks

## Branch: feLichessRematch_281026 — Lichess Rematch

Plan: [`docs/LichessRematchPlan.md`](docs/LichessRematchPlan.md)

### In Progress

- [x] 1. `LichessClient`: `RematchInfo`, `parseRematchInfo()`, keep last game's settings from `gameFull` + unit tests (4 tests in `LichessStaticTests`)
- [x] 2. `LichessClient`: `createRematch()` (direct challenge with `keepAliveStream` to a registered opponent / open challenge for anonymous), `waitForGameStart(…, registerAsPlayer)`, `directChallengeSent` / `challengeDeclined` signals
- [x] 3. `ChessConnector`: track online games in C++ (`_lastGameOnline`), `CanRematch` property, `rematch()` routing
- [x] 4. QML: Rematch button for online games, `ChallengeDialog` direct-challenge mode, `ChessGame.qml` handlers (decline banner, cancel on leaving the game)
- [x] 4b. Online game end handling (found while testing: opponent resigned, app showed nothing)
  - Result card with Rematch on the board when the side panels are hidden (default portrait window)
  - Board notification ("Black resigned", "… ran out of time", "… left the game") + red flag on the losing king (`FlaggedKing`)
  - Board frozen after an online game ends (`_gameOver`); reason texts for `timeout` / `noStart` / `stalemate`
  - `opponentGone` banner with countdown and **Claim victory** (`LichessClient::claimVictory`)
  - `gameFull` of an already finished game ends it (`state.status`)
- [x] 4c. Red flag + result for local game ends too
  - Checkmate flags the mated king (all modes); resign vs Robot always flags the human (also while the robot thinks)
  - Local resign sets `GameResult`, so Rematch / result card appear; flag hidden while reviewing earlier moves
  - Undo after mate / resign vs Robot resumes the game (clears flag, result, `_gameOver`)
  - Winner name fixed when the player plays Black
- [ ] 5. Manual test against Lichess (scenarios in plan §5) — Debug + Release build ✅, app starts without QML errors ✅, live Lichess scenarios pending

### Done

- [x] Release 0.1.0 – 0.1.2 published (installer via `release.yml`)
- [x] CI/CD build pipeline (MSBuild + windeployqt + signing + Inno Setup) — working since #33/#34
- [x] Computer levels rework + levels help screen (#68)
  - Levels 1–2: weak-move picker over MultiPV candidates (`PickWeakMove`), levels 3–5: `UCI_Elo` 1320/2000/3190
  - Level table with name/Elo/hint in `EngineLevel.h`; `LevelsHelpDialog.qml`; robot avatar tooltip
- [x] Promotion dialog upside-down when playing Black (#69)
- [x] User profiles — `UserManager`, per-user saves, login screen (feUserAndMultyplayer_140526)

### Backlog

- [ ] **chess.com account** — save/restore user chess.com credentials and data when connected
- [ ] Flaky `ChessQtTests` run: test host crashes intermittently around the engine-kill tests (`Init_AfterEngineKilled…`, `DebugState_AfterEngineDied…`); also on `develop`

---

## Network Multiplayer Game

- [ ] Re-implement network game support
  - Skeleton code preserved in `networkplayer.h` / `networkplayer.cpp` (excluded from build)
  - QML dialogs preserved in `SelectPlayerDialog.qml` / `NetworkPlayers.qml` (excluded from qml.qrc)
  - `ChessConnector` had `PlayersName` Q_PROPERTY wired up for player discovery — restore when implementing
  - Protocol skeleton uses UDP multicast (`239.255.43.32:33211`) for player discovery; extend for actual move exchange
  - Re-add to build: `networkplayer.cpp`, `networkplayer.h` (QtMoc), restore QML files to `qml.qrc`
  - Re-add `#include "networkplayer.h"` and `NetworkPlayerAptr _netPlayer` to `ChessConnector`
  - Re-add "Network Player" button to `ChessGame.qml` toolbar and state visibility rules

---

## How to use this file

Mark tasks `[x]` when done. Add new tasks under the appropriate section. Move completed sections to the bottom under **Done**.

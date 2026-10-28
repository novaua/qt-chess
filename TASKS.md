# Tasks

## In Progress

- [ ] Lichess rematch: live test with two Lichess accounts — direct rematch accept / decline / cancel, `opponentGone` + Claim victory (scenarios in [`docs/LichessRematchPlan.md`](docs/LichessRematchPlan.md) §5)

### Done

- [x] Lichess rematch + game end handling (#70) — plan: [`docs/LichessRematchPlan.md`](docs/LichessRematchPlan.md)
  - `LichessClient::createRematch()`: direct challenge (`keepAliveStream`) to a registered opponent, open challenge for anonymous; `parseRematchInfo()` + 4 unit tests
  - `ChessConnector::CanRematch`, Rematch button for online games, `ChallengeDialog` direct-challenge mode
  - Game end visible everywhere: `GameResultCard.qml` (portrait), board notification, red flag on the losing king (`FlaggedKing`), board frozen
  - `OpponentGoneBanner.qml` + Claim victory; `gameFull` of a finished game ends it
  - Local fixes: resign vs Robot, undo after mate, stats once per game, no autosave of finished games, `#` only for mate
  - Live check 2026-10-28: resign from the app and from an anonymous Lichess opponent ✅
- [x] Intermittent `ChessQtTests` test host crash (#71) — leaked `UciConnector` in `MainInit_IsGoood_Test` outlived the `QCoreApplication`; `.runsettings` git-ignored
- [x] Promotion dialog upside-down when playing Black (#69)
- [x] Computer levels rework + levels help screen (#68)
  - Levels 1–2: weak-move picker over MultiPV candidates (`PickWeakMove`), levels 3–5: `UCI_Elo` 1320/2000/3190
  - Level table with name/Elo/hint in `EngineLevel.h`; `LevelsHelpDialog.qml`; robot avatar tooltip
- [x] Release 0.1.0 – 0.1.2 published (installer via `release.yml`)
- [x] CI/CD build pipeline (MSBuild + windeployqt + signing + Inno Setup) — working since #33/#34
- [x] User profiles — `UserManager`, per-user saves, login screen (feUserAndMultyplayer_140526)

### Backlog

- [ ] **chess.com account** — save/restore user chess.com credentials and data when connected

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

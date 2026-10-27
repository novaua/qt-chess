# Tasks

## Branch: feRelease010_271026 — Release 0.1.0

### In Progress

- [ ] Merge release-prep PR to `develop` (CI green)
- [ ] Tag `v0.1.0` on `develop` and push; verify GitHub Release + `ChessSetup-0.1.0.exe`
- [ ] Smoke-test installer on a clean machine

### Done

- [x] CI/CD build pipeline (MSBuild + windeployqt + signing + Inno Setup) — working since #33/#34
- [x] Release notes `docs/release-notes/v0.1.0.md`, wired into `release.yml`

### Backlog

- [ ] **User profiles** — per-user config (display name, settings, stats); `AppConfig` is designed to extend
- [ ] **chess.com account** — save/restore user chess.com credentials and data when connected

- [x] Engine Difficulty setting — implemented
  - `UciConnector::SetDifficulty(int)` sets "Skill Level" via `setoption`; `SetOption` now updates `_opt` so `GetOption` reflects it
  - `ChessEnginePlayer(game, difficulty)` calls `SetDifficulty` after `Init()`
  - `startNewGameWithComputer(int difficulty)` stores `_lastDifficulty`; `continueGame()` reuses it
  - Two new unit tests in `UciConnectorTest.cpp`: boundary verification via `GetOption`, move test at difficulty 1

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

## Done

<!-- Move completed task blocks here -->

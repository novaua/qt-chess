# Tasks

## Branch: feMakeInstaller_0430a — CMake Installer Pipeline

### In Progress

- [ ] Fix CMake build so `cmake --build build --config Release` produces `ChessGui.exe`
  - CMakeLists.txt added (commit e463983) but build is still failing (commits 360efa3–9ee0a48)
  - Check that all four VS projects (ChessCore, UciConnector, ChessConnector, ChessGui) are wired in CMakeLists.txt
  - Verify Qt6 find_package paths and windeployqt invocation

- [ ] Validate GitHub Actions workflow (`.github/workflows/release.yml`)
  - Workflow builds with CMake + Ninja on `windows-latest`
  - Runs `windeployqt` then packages with Inno Setup (`installer/ChessSetup.iss`)
  - Test by opening a PR against `develop` — artifact should appear under the run

- [ ] Create / verify Inno Setup script (`installer/ChessSetup.iss`)
  - Script must pick up `deploy/` folder produced by the workflow
  - Output file should match `installer/Output/ChessSetup-*.exe`

### Backlog

- [ ] Merge `feMakeInstaller_0430a` → `develop` once CI passes green
- [ ] Tag a release (`v*`) to trigger the GitHub Release publish step
- [ ] Engine Difficulty setting
  - Slider UI is already in the start menu (`difficultySlider.value`, range 0–20)
  - Wire up: pass difficulty to Stockfish via `setoption name Skill Level value N` UCI command before each move
  - Add `void SetDifficulty(int level)` to `UciConnector`; call from `ChessEnginePlayer::DoMove()`
  - `ChessConnector::startNewGameWithComputer()` should accept difficulty parameter from QML slider value

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

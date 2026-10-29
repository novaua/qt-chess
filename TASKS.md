# Tasks

## In Progress

- [ ] **BL-02 / BL-03 App modernisation**: Windows 11 Fluent look, clear user flows, local profiles with a linked Lichess account. Plan: [`docs/AppModernizationPlan.md`](docs/AppModernizationPlan.md), mockups linked there. **Next: P3** (see `PROGRESS.md`).
  - [x] Review of all screens, plan, design kit (`pics/icons/*.svg`, [`docs/design/ThemeTokens.md`](docs/design/ThemeTokens.md)), mockups (#77)
  - [x] Decide the QML access rule (plan §9 Q1): game flows through `ChessConnector`, services stay separate typed singletons (CLAUDE.md "Key Conventions")
  - [x] P1 Foundation (`fluentFoundation_281026`): FluentWinUI3, `Theme.qml`, System/Light/Dark, icons in qrc, `AppDialog`; removed Qt5Compat effects and `NetworkPlayers.qml` (closes BL-01)
    - [ ] Manual check with a connected Lichess account: Lichess dialog (connected), Challenge dialog, draw/takeback banners
  - [x] P2 Shell and navigation (`shellNavigation_291026`): `StackView` pages, command bar, account menu, Home, setup dialogs, InfoBar; services as typed singletons; game flows into `ChessConnector` (`startComputerGame`, `startLocalGame`, `switchUser`, Lichess game start/end)
    - [ ] Manual check in a real window: shortcuts (Esc, F, Ctrl+S/Z, ←/→, F1, Ctrl+,), Enter/Esc in the setup dialogs, Switch user confirmation during a game
    - [ ] Manual check with Lichess: challenge → game page, game end text, draw/takeback InfoBars, opponent gone + Claim victory, rematch
  - [ ] P3 Users and Lichess: welcome, profile picker, Profile page (ratings, Reconnect, OAuth Cancel: closes BL-08), Guest, statistics, `QSaveFile`, backups, export/import
  - [x] P4 Game chrome (`gameChrome_291026`): player cards, captured strip with +N (`MaterialBalance`), side panel or bottom sheet, one result card (Rematch / New game / Review moves), move list (click a move, piece images, `reviewTo`), Copy / Export PGN; `Game::FormatHistorySan` (real mate detection), `FormatPgn`, `MaterialBalance` in ChessCore with 13 tests; `-Offscreen` screenshots for a locked screen
    - [x] Light-mode after shots in a real window (`docs/design/after-p4/`: light and dark)
    - [ ] Manual check: Export PGN… save dialog and the file, Copy PGN, Esc closes the move sheet, Review moves / New game on the result card (vs Robot, two players)
  - [ ] P5 Settings and About pages
  - [ ] P6 Polish: keyboard and accessibility, high contrast, motion, app icon, screenshot baselines (with BL-05)

Manual checks, no code (do any time):

- [ ] Installer: install the Microsoft VC++ runtime when missing (branch `fixVcRedistInstall_281026`)
  - [x] `vc_redist.x64.exe` no longer copied into `{app}`; unpacked to `{tmp}` and run (`/install /quiet /norestart`) only if the installed runtime (registry `VC\Runtimes\x64`) is missing or older than the bundled one; exit codes 0/1638 OK, 3010 → restart offered, others → warning with download link
  - [x] `Build-Installer.ps1` sets `VCINSTALLDIR` so local builds bundle it too; the build fails if it's missing
  - [ ] Install on a PC without the runtime (or older) and check the `/LOG` setup log line `VC++ runtime …: exit code`
- [x] Installer: Stockfish from the newest **stable** release, single download (#72)
  - [x] `Resolve-Stockfish.ps1` pins newest stable + previous stable (fallback) with GitHub's SHA-256 at build time (`Build-Installer.ps1`, `release.yml`); no hard-coded release in `ChessSetup.iss`, the build fails if it can't resolve (review: option A)
  - [x] Per candidate: download (checksum) → unzip → UCI check (`Prepare-Stockfish.ps1`, `uci` → `uciok`, version logged); fallback only if a step fails
  - [x] `{app}\engines\stockfish-install.log` (+ Inno `/LOG`); `Test-StockfishDownload.ps1` mirrors the flow — normal and fallback paths verified
  - [ ] Run the built installer once (normal install) and check `engines\stockfish-install.log`
- [ ] Lichess rematch: live test with two Lichess accounts — direct rematch accept / decline / cancel, `opponentGone` + Claim victory (scenarios in [`docs/LichessRematchPlan.md`](docs/LichessRematchPlan.md) §5)

### Done

- [x] Screenshot mode + `run-chess-app` skill (#76): `--screenshot`/`--scene`/`--data-dir`, `scripts\Take-Screenshot.ps1`. The stub UCI engine moved to BL-05.
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

- [x] BL-01 Remove obsolete QT QML components, used for chess glow for example [Easy] → done in BL-02 P1
- [ ] BL-02 Review and modernize all game screens [Hard] → In Progress (plan done)
- [ ] BL-03 Remove user handling relay on adopting LiChess more, need more reserch here before or togather with BL-02 [Medium] → In Progress, BL-02 P3 (decided: local profiles + linked Lichess account)
- [ ] BL-04 Improve game UI making pice move and dragged by hands, use latest QLM techniques [Medium]
- [ ] BL-05 UI test suite for local pre-PR sanity (Qt Quick Test + screenshot diff, optional pywinauto smoke / CI job) — plan: [`docs/UiTestSuitePlan.md`](docs/UiTestSuitePlan.md) [Hard]
- [ ] BL-06 Implement engine selection. Currently we bundle only with Stockfish. Implement setting to select test and make default another UCI engine.
- [ ] BL-07 Polish LiChess workflows add full suport inlcuding caht and diffrent way pof playing.
- [ ] BL-09 Engine move off the GUI thread's game [Medium]: `ChessEnginePlayer::DoMove` runs on the engine thread and calls `_game->DoMove()`, while the GUI thread reads the same `Game` (captured pieces, `MaterialBalance`, `LastMoveFrom/To`, review replays). Clicking a move or pressing ← during the robot's turn can read the move history while it reallocates. Fix: the engine thread only finds the move (`EngineWorker` returns the UCI string); `ChessConnector` plays it on the GUI thread. Found in the P4 code review (#81).
- [ ] BL-08 When authorizing using LiChess, 'Waiting for browser' message is shown, but the browser is not opened or the link lost or user dont wasn to. Add cancel option. → part of BL-02 P3

---



## Network Multiplayer Game

- [ ] Re-implement network game support with local games without server
     - Consider if it's worth doing at all
     - 	
---

## How to use this file

Mark tasks `[x]` when done. Add new tasks under the appropriate section. Move completed sections to the bottom under **Done**.

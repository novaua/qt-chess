# Tasks

## In Progress

- [ ] Installer: install the Microsoft VC++ runtime when missing (branch `fixVcRedistInstall_281026`)
  - [x] `vc_redist.x64.exe` no longer copied into `{app}`; unpacked to `{tmp}` and run (`/install /quiet /norestart`) only if the installed runtime (registry `VC\Runtimes\x64`) is missing or older than the bundled one; exit codes 0/1638 OK, 3010 → restart offered, others → warning with download link
  - [x] `Build-Installer.ps1` sets `VCINSTALLDIR` so local builds bundle it too; the build fails if it's missing
  - [ ] Install on a PC without the runtime (or older) and check the `/LOG` setup log line `VC++ runtime …: exit code`
- [x] Installer: Stockfish from the newest **stable** release, single download (#72)
  - [x] `Resolve-Stockfish.ps1` pins newest stable + previous stable (fallback) with GitHub's SHA-256 at build time (`Build-Installer.ps1`, `release.yml`); no hard-coded release in `ChessSetup.iss`, the build fails if it can't resolve (review: option A)
  - [x] Per candidate: download (checksum) → unzip → UCI check (`Prepare-Stockfish.ps1`, `uci` → `uciok`, version logged); fallback only if a step fails
  - [x] `{app}\engines\stockfish-install.log` (+ Inno `/LOG`); `Test-StockfishDownload.ps1` mirrors the flow — normal and fallback paths verified
  - [ ] Run the built installer once (normal install) and check `engines\stockfish-install.log`
- [ ] Screenshot mode + `run-chess-app` skill — step 1 of BL-05 (branch `screenshotHook_281026`)
  - [x] `ChessGui.exe --screenshot <png> --scene <name>` (+ `--moves`, `--black`, `--dark`, `--size`, `--delay`), works with `-platform offscreen`; `--data-dir` for a throwaway profile
  - [x] `scripts\Take-Screenshot.ps1` (all scenes, optional build / windowed); `.claude/skills/run-chess-app`
  - [ ] Stub UCI engine for a single-player scene (later, with BL-05)
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

- [ ] BL-01 Remove obsolete QT QML components, used for chess glow for example [Easy]
- [ ] BL-02 Review and modernize all game screens [Hard]
- [ ] BL-03 Remove user handling relay on adopting LiChess more, need more reserch here before or togather with BL-02 [Medium]
- [ ] BL-04 Improve game UI making pice move and grag by hand, ulse latest QLM techniques [Medium]
- [ ] BL-05 UI test suite for local pre-PR sanity (Qt Quick Test + screenshot diff, optional pywinauto smoke / CI job) — plan: [`docs/UiTestSuitePlan.md`](docs/UiTestSuitePlan.md) [Hard]
- [ ] 
---

## Network Multiplayer Game

- [ ] Re-implement network game support with local games without server
     - Consider if it's worht doing at all	
---

## How to use this file

Mark tasks `[x]` when done. Add new tasks under the appropriate section. Move completed sections to the bottom under **Done**.

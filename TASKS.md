# Tasks

## In Progress

- [ ] **BL-02 / BL-03 App modernisation**: Windows 11 Fluent look, clear user flows, local profiles with a linked Lichess account. Plan: [`docs/AppModernizationPlan.md`](docs/AppModernizationPlan.md), mockups linked there. **All phases coded; manual checks left** (see `PROGRESS.md`).
  - [x] Review of all screens, plan, design kit (`pics/icons/*.svg`, [`docs/design/ThemeTokens.md`](docs/design/ThemeTokens.md)), mockups (#77)
  - [x] Decide the QML access rule (plan §9 Q1): game flows through `ChessConnector`, services stay separate typed singletons (CLAUDE.md "Key Conventions")
  - [x] P1 Foundation (`fluentFoundation_281026`): FluentWinUI3, `Theme.qml`, System/Light/Dark, icons in qrc, `AppDialog`; removed Qt5Compat effects and `NetworkPlayers.qml` (closes BL-01)
    - [ ] Manual check with a connected Lichess account: Lichess dialog (connected), Challenge dialog, draw/takeback banners
  - [x] P2 Shell and navigation (`shellNavigation_291026`): `StackView` pages, command bar, account menu, Home, setup dialogs, InfoBar; services as typed singletons; game flows into `ChessConnector` (`startComputerGame`, `startLocalGame`, `switchUser`, Lichess game start/end)
    - [ ] Manual check in a real window: shortcuts (Esc, F, Ctrl+S/Z, ←/→, F1, Ctrl+,), Enter/Esc in the setup dialogs, Switch user confirmation during a game
    - [ ] Manual check with Lichess: challenge → game page, game end text, draw/takeback InfoBars, opponent gone + Claim victory, rematch
  - [x] P3 Users and Lichess (`usersLichess_291026`): Welcome page, centred profile picker (Lichess badge, last played, export/delete per card), Profile page (name and avatar, Lichess card with ratings, Reconnect after a 401, Cancel while waiting for the browser: closes BL-08 and BL-03; statistics per opponent with draws; export / delete), import with a conflict dialog, restore from backup; `QSaveFile` for `users.json` and `config.json`, 3 rolling backups; Lichess sign-in moved from `ChessConnector` into `UserManager`; 16 new tests in `ChessQtTests`
    - [ ] Manual check with Lichess: Continue with Lichess on the Welcome page (a new profile named after the account), Link / Cancel / API token on the Profile page, ratings shown, revoke the token on lichess.org → Reconnect
    - [ ] Manual check in a real window: export a profile, import it on another Windows user (Reconnect Lichess), import conflict (as new / replace), delete a profile, a damaged `users.json` → Restore
  - [x] P4 Game chrome (`gameChrome_291026`): player cards, captured strip with +N (`MaterialBalance`), side panel or bottom sheet, one result card (Rematch / New game / Review moves), move list (click a move, piece images, `reviewTo`), Copy / Export PGN; `Game::FormatHistorySan` (real mate detection), `FormatPgn`, `MaterialBalance` in ChessCore with 13 tests; `-Offscreen` screenshots for a locked screen
    - [x] Light-mode after shots in a real window (`docs/design/after-p4/`: light and dark)
    - [x] Manual check: Copy PGN, Export PGN… (dialog and file), PGN date of a continued game (#83)
    - [ ] Manual check: Esc closes the move sheet, Review moves / New game on the result card (vs Robot, two players)
  - [x] P5 Settings and About (`settingsAbout_291026`): `SettingsPage` replaces `SettingsDialog` (Appearance: theme, coordinates around the board or on the squares, reduce animations · Sound: effects switch + volume 1–5 · Gameplay: confirm resign, legal moves, last move · Computer: engine name and path, levels · Data: back up now, restore a chosen backup, export / import, data folder · Advanced: FEN or moves, log folder · About: version, build, licences, links, shortcuts); `AppSettings` singleton (same keys; old "volume 0" and `darkMode` migrated); `AppInfo` service; `UserManager.backups` / `backupNow` / `restoreBackup` with 3 tests; board colours fixed (a1 is dark)
    - [ ] Manual check in a real window: each switch takes effect in a game (legal-move dots, last move, resign without confirmation, Reduce animations when flipping), the volume preview sound, Back up now / Restore… / Open data folder / Open log folder, the About links
  - [x] P6 Polish (`polish_291026`): Ctrl+N / Ctrl+O; Enter on Home cards; avatar and opponent pickers as one Tab stop with arrow keys and a `FocusRing`; high contrast (`Theme.highContrast`: system colours, solid borders, `--high-contrast` screenshots); page transitions (fade and rise, off with Reduce animations); Reduce animations also follows Windows' animation effects; new app icon and installer images (`scripts/make_icons.py`, `WizardStyle=modern`); screenshot baselines (`Compare-Screenshots.ps1`, BL-05 step 4)
    - [ ] Manual check in a real window: Tab through Home, the profile picker (card → More), Create profile and Two players (arrow keys, focus ring), Enter on a Home card, Ctrl+N / Ctrl+O, the page transitions
    - [ ] Manual check: a Windows contrast theme (Settings → Accessibility → Contrast themes) and 200 % scaling; Windows animation effects off → Settings shows Reduce animations on and disabled
    - [ ] Manual check: run the built installer once and look at the new wizard images and icons

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
- [x] BL-03 Remove user handling relay on adopting LiChess more, need more research here before or together with BL-02 [Medium] → done in BL-02 P3 (local profiles + linked Lichess account)
- [ ] BL-04 Improve game UI making piece move and dragged by hands, use latest QLM techniques [Medium] → In Progress — plan: [`docs/BL-04-PieceMovePlan.md`](docs/BL-04-PieceMovePlan.md) (pieces with an identity that slide, drag and drop, one board geometry). Steps 1–4 done: `Game::UndoMove` takes back castling / en passant / promotion correctly; `Chess::BoardDiff`; the `BoardPieces` model as `ChessConnector.Pieces` (+ `SelectedSquare`, `MovableColor`, `movePiece`); the QML board on `Pieces` with one geometry and one `TapHandler`; slides and fades (step 5); dragging and the `drag` scene (steps 6–7). Left: manual check of dragging (plan §5 rows 2, 3, 7, 11–14)
- [ ] BL-05 UI test suite for local pre-PR sanity (Qt Quick Test + screenshot diff, optional pywinauto smoke / CI job) — plan: [`docs/UiTestSuitePlan.md`](docs/UiTestSuitePlan.md) [Hard]
- [ ] BL-06 Implement engine selection. Currently we bundle only with Stockfish. Implement setting to select test and make default another UCI engine.
- [ ] BL-07 Polish LiChess workflows add full support including chat and different way pof playing.
- [x] BL-09 Engine move off the GUI thread's game [Medium]: `ChessEnginePlayer::DoMove` runs on the engine thread and calls `_game->DoMove()`, while the GUI thread reads the same `Game` (captured pieces, `MaterialBalance`, `LastMoveFrom/To`, review replays). Clicking a move or pressing ← during the robot's turn can read the move history while it reallocates. Fix: the engine thread only finds the move (`EngineWorker` returns the UCI string); `ChessConnector` plays it on the GUI thread. Found in the P4 code review (#81). → done: `ChessEnginePlayer::FindMove(history)` gets a copy of the moves, `ChessConnector::onEngineMoveFound` plays the move; a request number drops answers for an old game or position.
- [x] BL-08 When authorizing using LiChess, 'Waiting for browser' message is shown, but the browser is not opened or the link lost or user dont wasn to. Add cancel option. → done in BL-02 P3 (Cancel on the Welcome and Profile pages)
- [ ] BL-10 Lichess challenges as notifications while signed in [Hard]: today the app hears Lichess only during its own flows (waiting for a game, and since #89 a rematch offer while the finished game is on screen). A challenge sent to the linked account from lichess.org, or a rematch offered after we left the game, never shows up. Lichess sends both on `/api/stream/event` (`challenge`, `challengeCanceled`, `challengeDeclined`, `gameStart`; a rematch is a `challenge` with `rematchOf`), only to a stream open at that moment. To do: keep one event stream open whenever a Lichess account is linked (reconnect with backoff, stop on sign-out / 401, share it with `waitForGameStart` and the rematch watch); keep a list of incoming challenges (challenger, rating, time control, variant, rated, colour) and show them anywhere in the app (badge or InfoBar on Home and the game page, and a list); Accept (needs `compat.board`, else "open on lichess.org") and Decline with a reason; decide what Accept does during a running game; optionally a Windows notification when the window is minimised; parse tests and a screenshot scene for the notification. See `LichessClient::isRematchOffer` / `watchForRematch` (#89).
- [ ] BL-11 Profile the app and fix the top inefficiencies [Medium]: opening the game page takes up to a second, and we don't know what redraws during a game. To do: measure first, in Release with the deployed Qt: the time from the click (Home card, Continue, a setup dialog's Start) to the first frame of `GamePage` (log timestamps or a `QElapsedTimer` in `ChessConnector`), and a QML profile (Qt Creator's QML Profiler / `qmlprofiler`) of opening the game page and of a few moves, a drag, a takeback and the review (←/→). Check the scene graph with `QSG_VISUALIZE=changes|batches|overdraw` and `QSG_RENDER_TIMING=1` / `qt.scenegraph.time.*`: an idle board should not redraw, and a move should repaint only the squares and pieces it changes. Look for: bindings that re-evaluate on every move or on every `ChessConnector` signal (one wide `...Changed` signal that refreshes the whole board, a model reset in place of a row change), `Repeater`s that recreate their 64 delegates, page and dialog items created up front instead of on demand (`Loader`/`asynchronous`), images loaded or scaled at full size (`sourceSize`), effects/`layer.enabled`, timers or animations that keep running, and slow C++ calls from QML (move list, captured pieces, material, legal moves). Write the numbers before and after in `PROGRESS.md`; fix the biggest few; keep the 42 screenshot scenes matching.
- [ ] BL-12 Smaller Qt deployment — PR #91, **frozen** until the owner picks it up again (more testing and evaluation first) [Medium]: ship only the Qt files the app uses (`installer\Deploy-Qt.ps1`; plugins and QML in `deploy\qt\`; no translations, `opengl32sw`, D3D12 compilers, FFmpeg; `QT_MEDIA_BACKEND=windows`; `QGuiApplication` instead of `QApplication`, no Qt Widgets). Installed size 146 → 67 MB, installer 50.8 → 36 MB. Branch `pruneQtDeploy_301026`: on GitHub still on the old base; a local copy rebased onto `develop` after #93 (e616f08, built and tested, not pushed). To resume: rebase onto the then-current `develop` (expect `PROGRESS.md` / `TASKS.md` conflicts; skip the "spell fix" commit, already in `develop`), Rebuild ChessGui (the Widgets module goes away), then install over the last release and check that the old plugin folders are gone, sounds play, and every page opens.
- [ ] BL-13 Game page layout for wide screens [Medium], **needs a plan first**: on a monitor much wider than it is high, the player cards (avatar, name, captured pieces, +N) should move to a column on the left of the board (opponent at the top, player at the bottom), so the board can take all of the available height. Today, in a wide window (`GamePage.qml`: `area.wide`, width >= 960 or landscape), only the moves panel goes beside the board (on the right); the two cards stay above and below it, so the board loses their height (`area.boardSize`). Keep the current layout when the window is taller than wide (e.g. snapped to half the screen) and in narrow windows (move sheet). The plan should cover: when to switch (aspect ratio, minimum height), the left column's width and what it shows (captured pieces vertical or wrapped, the active-side marker), the moves panel on the right, and screenshot scenes at e.g. `-Size 1920x1080` and `1400x900` with baselines. The owner will add more features to this task, e.g. other ways to show the move history.
- [ ] BL-14 One thing at a time on the board [Medium] — plan: [`docs/BL-14-MovePacingPlan.md`](docs/BL-14-MovePacingPlan.md). Follow-up of BL-04, from the owner's testing: the robot's answer (level 1) slides together with the player's move; playing Black, the robot moves while the board is still turning; sounds and messages follow the game, not the board; slides sometimes stop for a few ms. Plan: a `MoveGate` that holds moves from outside (robot, Lichess opponent) until the board is idle (page settled, not turning, last slide done, a short pause), so game, board and sounds stay in step; the player's own move stays instant. Step 1 done (branch `boardFreezes_301026`): frame trace, `NavigationLayer` and the move list without rebuilds — numbers in `PROGRESS.md`. Step 2 done (branch `moveGate_301026`): `MoveGate` with 12 tests (fake clock) — holds outside moves until the board is idle, one batch for a resync, `isIdle()` for results without a move. Next: step 3, the robot through the gate. Open: think during the animation and show after (recommended), or ask the engine only after it

---



## Network Multiplayer Game

- [ ] Re-implement network game support with local games without server
     - Consider if it's worth doing at all
     - 	
---

## How to use this file

Mark tasks `[x]` when done. Add new tasks under the appropriate section. Move completed sections to the bottom under **Done**.

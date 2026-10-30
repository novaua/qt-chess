# Progress Log

Use this file to record where work stopped and what to do next. Newest entry at the top.

---

## 2026-10-30 — BL-04 step 4: the QML board on `Pieces` (branch: boardQml_301026)

### What was done
- **One board geometry** (`ChessBoard.qml`): `squaresSize`, `squareSize`, `squareX(square)`, `squareY(square)` (a1 at the bottom left, in the board's own coordinates) and `squareAt(x, y)`. `ChessBoxes`, `ChessFigures` and `NavigationLayer` place their items with them; the three X-mirrored `Grid`s and the per-item Y-mirror/180° counter-rotations are gone. The board still turns for Black with `rotation`; a piece turns back with `rotation: chessBoard.angle` (same animation as before).
- **`ChessFigures.qml`**: a `Repeater` over `ChessConnector.Pieces`, one item per piece at its square (`taken` ones hidden until step 5 fades them). Before, every move removed and re-inserted `ListModel` rows, rebuilding square delegates (64 on page creation and on every review step).
- **Clicks**: one `TapHandler` on the pieces layer → `figureSelected(squareAt(...))`, instead of 64 `MouseArea`s. The selected piece's tint is in `ChessBoxes` (`SelectedSquare`), above the other highlights.
- **Removed**: `EmptyBoardModel.qml`, `ChessConnector::boardChanged`, `refreshBoard()`, the QML-only `selectedIndex`. `emitBoardState()` (review) now only syncs `Pieces` and clears the dots.
- **Screenshot scene `select`** (in `all`): 1. e4 e5 with the g1 knight selected — tint and dots; `-Black` shows the turned board.
- Debug and Release build; ChessTests 111, ChessQtTests 85 pass; all 44 scenes (light, dark) and `robot` match the baselines, no QML warnings. The old `robot` baseline was still from Qt 6.10 (`all` doesn't take it): renewed, only its text antialiasing differed.

### Where to continue
- **Manual check (needs a person, screenshot mode ignores the mouse):** click-select-click moves vs the robot as White and as Black, in a two-player game, after Flip board; clicking an opponent's piece or an empty square; promotion; review ← →; takeback.
- Step 5 (plan §2, §3.4): slides (`Behavior on x/y`, `Theme.slow`), captured pieces fading out (`taken`), resets without slides, all off with Reduce animations.

## 2026-10-30 — BL-04 step 3: the pieces model (branch: boardPieces_301026, from pieceDrag_301026 / #92)

### What was done
- **`BoardPieces`** (`ChessGui/boardpieces.h/.cpp`, a `QAbstractListModel`): one row per piece, roles `pieceId`, `square`, `piece` ("N"/"n"), `taken`. `sync(board)` compares with the last shown position (`BoardDiff`): a moved or promoted piece keeps its row and id (`dataChanged`), a captured one is marked `taken` (its row stays until the next reset, so the fade isn't cut when the robot answers at once), a piece put back by a takeback is a new row. More than 4 changes (new game, review jump) → model reset with new ids.
- **`ChessConnector`:**
  - `Pieces` (CONSTANT). The board listener and `emitBoardState()` (review) post one queued `sync` per change (`schedulePiecesSync()`), so castling's four squares, a two-move takeback or a Lichess batch become one comparison. It syncs the **shown** position: `shownGame()` (a saved game's replay, a reviewed replay, or the game), now also used by `checkedKing()` and `materialBalance()`.
  - `canPlayerMove()`: the checks `figureSelected` had (replay, review, robot thinking, promotion choice, game over, the Lichess opponent's turn), now shared. `MovableColor` (0 none, 1 White, 2 Black) on it, notified by the signals its inputs change with.
  - `SelectedSquare`: the clicked piece of the side to move, or -1 (today's tint is QML-only). `clearPossibleMoves()` resets it with the dots, in the 4 places that cleared the dots.
  - `movePiece(from, to)`: a drop. Plays it if legal (`GetAllowedMoves(from)`), else returns false; clears the dots either way.
  - `boardChanged` / `refreshBoard()` stay until step 4 moves the QML board to `Pieces`.
- **Tests:** `ChessQtTests/BoardPiecesTest.cpp`, 9 tests with `QAbstractItemModelTester` (warnings collected, expected none): every piece on the first sync, nothing on the same position, id kept on a move, capture marks taken, takeback of a capture, castling (2 rows), promotion keeps the pawn's id, a big change resets, role names. ChessQtTests now uses Qt Test (`testlib`).
- Debug and Release build; ChessTests 111, ChessQtTests 85 pass; all 43 screenshot scenes match the baselines (no QML change yet).

### Where to continue
- Step 4 (plan §3.4): one board geometry, `ChessFigures` on `Pieces`, a board `TapHandler`, the selection tint in `ChessBoxes`.
- Locally, after changing a project's `QtModules` (here `testlib`), Rebuild that project once: Qt's build cache keeps the old module list.

## 2026-10-30 — BL-04 plan, steps 1–2: takeback fix and BoardDiff (branch: pieceDrag_301026)

### What was done
- **Plan:** `docs/BL-04-PieceMovePlan.md` (pieces with an identity that slide, drag and drop, one board geometry), linked from BL-04 in TASKS.
- **Step 1, `Game::UndoMove` fixed.** It only moved the piece back and put a captured piece on the target square. After a takeback the castled rook stayed on f1/d1 (d8/f8), a pawn taken en passant came back on the target square instead of beside it, and a promoted piece went back to the 7th rank as itself. Takeback vs the robot (`ChessConnector::movePrev`) and stepping back in a loaded game (`HistoryPlayer::MoveBack`) use it; review (←/→) replays and wasn't affected. All changed squares are reported to the board listeners. En passant is recognised by the move before being a double step over the target square (`Game` records en passant as a capture on the target square). `ChessTests/UndoTests.cpp`: 14 tests (each castling, en passant both colours, promotion with and without capture, the player's promotion choice, two moves back), comparing FENs and the reported squares; 11 failed before the fix.
- **Step 2, `Chess::BoardDiff(before, after)`** (`ChessCore/BoardDiff.h/.cpp`; a class after review, first a free function `MatchPieces`): which piece went where between two positions: `Moved()` (`PieceTravel`: from, to, piece before/after), `Removed()`, `Added()`. Equal pieces pair nearest first; a pawn leaving the 7th (2nd) rank and a piece arriving beside it on the last rank is a promotion, reversed a takeback of one, on the same square the player's choice. `ChessTests/BoardDiffTests.cpp`: 11 tests, each case both ways.
- Debug and Release build; ChessTests 111, ChessQtTests 76 pass. No UI change yet.
- **Robot trace in Debug builds** (`ENGINE_TRACE` in `engineworker.h`, nothing in Release): the app log gets a line when the robot is asked (request number), when the worker starts and answers, when the answer is played or dropped as stale, and when the engine thread stops. For a flaky hang seen once in manual testing: playing Black vs a weak level, the board stayed on "Robot is thinking" although `uci_debug_log.txt` shows Stockfish answered in 40 ms; not reproduced since (8 runs of the `robot` scene, and the user's second try). If it happens again in a Debug build, the log shows where the answer stopped.

### Where to continue
- BL-04 step 3: the `BoardPieces` model and the `ChessConnector` wiring (plan §3.2–3.3), on a new branch from `develop` once this PR is in.
- Locally: after switching from #91's branch, ChessGui needs a Rebuild (its Qt build cache still has Widgets dropped).

## 2026-10-30 — Moved to Qt 6.12.0 (branch: qt612Upgrade_301026)

### What was done
- **Every Qt project now builds against `6.12.0_msvc2022_64`**: ChessGui, LiChessConnector, UciConnector, ChessQtTests. ChessCore was already plain C++.
- **ChessTests no longer uses Qt.** It was still a Qt VS Tools project (`QtVS_v304`, `core;network`, the QtMsBuild imports, include paths to UciConnector and ChessGui), but its tests use only ChessCore. Now it's a plain `Win32Proj`: its DLL depends only on the test framework and the C++ runtime, and its 86 tests pass with no Qt on `PATH`. CI (`release.yml`, `tests.yml`) installs 6.12.0 with only `qtmultimedia`: `qt5compat` and `qtshadertools` aren't used since BL-02 P1.
- **Modules:** the app needs Qt Multimedia (the `SoundEffect`s in `ChessGame.qml` and `SettingsPage.qml`). It's an add-on that the 6.12.0 install didn't have at first; with it added, nothing else is missing. Also not installed, and not used: Qt5Compat, Charts, Quick3D, ShaderTools, Quick Timeline.
- **Qt 6.11 change: `DialogButtonBox` sets `highlighted` on its buttons** and highlights only its new `defaultButton`. A `highlighted: true` on a footer button was overwritten to `false`, so every dialog's primary button lost its accent colour. The 8 dialogs with a primary button now set `defaultButton: primaryButton` on the box (see `AppDialog.qml`).
- **Checked:** Debug and Release build clean; 86 + 71 tests pass; all 42 screenshot scenes render. Compared with the 6.10 baselines, the rest of the differences are 6.12's semibold Fluent dialog titles, text anti-aliasing, and the Qt version and build date on About.

### Where to continue
- **CI installs aqtinstall from a pinned `master` commit** (`aqtsource:` in `release.yml` and `tests.yml`): 3.3.0, its latest release, can't find Qt 6.11+ for Windows (one repository folder per compiler since 6.11; fix miurahr/aqtinstall#1000 is merged but unreleased). Drop the `aqtsource` line once an aqtinstall release after 3.3.0 is out.
- Accept the new look as the baselines: `scripts\Compare-Screenshots.ps1 -Update` (local).
- Locally, put `D:\Tools\Qt\6.12.0\msvc2022_64\bin` on `PATH` in place of 6.10.2's (the app takes Qt's DLLs from there). You can also make 6.12.0 the default in Qt VS Tools.

## 2026-10-29 — BL-09 engine move on the GUI thread done (branch: engineThreadMove_291026)

### What was done
- **The engine thread no longer touches the app's `Game`.** Before, `ChessEnginePlayer::DoMove` ran on the engine thread and called `_game->DoMove()` while the GUI thread read the same game (captured pieces, material, last move, review). Now:
  - `ChessEnginePlayer::FindMove(history)` gets a copy of the move list and returns the engine's move in UCI. For FEN it replays the moves on its own `Game`. `DebugDump(history)` works the same way.
  - `EngineWorker::findMove(request, history)` runs on the engine thread and answers `moveFound(request, move)` or `moveError(request, …)`.
  - `ChessConnector::askEngineMove()` (one place for the robot's turn: after the player's move, a new game with the robot as White, a takeback, Continue) sends a copy of the moves. `onEngineMoveFound()` plays the move on the GUI thread. A move that can't be played goes the error path, with the FEN in the log.
  - A request number (`_engineRequest`) drops an answer for an old game or position: `stopEngineThread()` bumps it. `_gameOver` is a plain `bool` again: only the GUI thread uses it.
- **Tests:** `ChessQtTests/ChessEnginePlayerTest.cpp` (6 tests, real Stockfish like `UciConnectorTest`): mate in one found in StartPos, PureFen and FenWindow (12 plies, so a FEN plus moves); the dump; the worker returns the request number; an unresolved promotion is an error with the moves in the dump. All 157 tests pass.
- **Screenshot scene `robot`** (not in `all`: needs Stockfish): a level 5 game where the player is Black. The robot's first answer is dropped (`endGame()` while it thinks), then Continue: the robot mates with Qh5#, and the shot waits for the engine. All 42 baseline scenes still match.

### Where to continue
Next backlog candidates: BL-05 (stub engine, Quick tests: the `robot` scene would then run without Stockfish), BL-06 (engine choice), BL-04 (dragging pieces). The manual checks for BL-02 (P1–P6) are still open in TASKS.

## 2026-10-29 — BL-02 P6 Polish done (branch: polish_291026); BL-02 is coded, manual checks left

### What was done
- **Keyboard:** Ctrl+N (Home: new game vs the computer; a finished game: the result card's New game, so a running game is never dropped) and Ctrl+O (Home: open the saved game, or say there is none); both on the cards' tooltips and in About's shortcut list. Home cards take Enter as well as Space. The avatar picker (Create profile, Profile) and the opponent picker (Two players) are one Tab stop each, like radio buttons: arrow keys pick, and the new `FocusRing.qml` outlines the chosen one. A profile card's More button is hidden with opacity instead of `visible`, so Tab still reaches it.
- **Accessibility:** every icon-only button already had a tooltip and `Accessible.name` (checked); the pickers now have list roles and names. No OK/Yes/No buttons are left.
- **High contrast:** `Theme.highContrast` follows a Windows contrast theme (`styleHints.accessibility.contrastPreference`, Qt 6.10). The Fluent controls already switch to the system colours; our roles now do too (`SystemPalette`): window, windowText, disabled text, highlight as accent; no selected fill, no scrim, no Home photo; hovered cards get an accent border. The board keeps its colours. `--high-contrast` / `Take-Screenshot.ps1 -HighContrast` forces our part (the Fluent controls can't be forced).
- **Motion:** page transitions in the `StackView` (fade in and rise 24 px over `Theme.normal`, fade out over `fast`) replace the Basic style's 400 ms slide, and switch off with Reduce animations. `Theme.reduceMotion` is also on when Windows' animation effects are off (`AppInfo.windowsAnimationsOff`, `SPI_GETCLIENTAREAANIMATION`); Settings then shows the switch on and disabled.
- **App icon and installer:** a new icon (the board's white knight on a rounded tile in the board's green) in `chess.ico` (16–256 px), `pics/app-icon.png` (window, Welcome, About; replaces `chess_icon.svg`), and installer images `installer/art/wizard-*.bmp` for 100–250 % DPI with `WizardStyle=modern`. `scripts/make_icons.py` (Pillow) redraws them all. `Build-Installer.ps1 -SkipBuild` builds the installer with them.
- **Screenshot baselines (BL-05 step 4):** `scripts/Compare-Screenshots.ps1` compares `test-results\screens` with local baselines (`-Update` accepts), with a tolerance, and writes a diff image per failure. Screenshot mode turns animations off (`Theme.forceReduceMotion`, not saved), so two runs of all 42 scenes (light and dark) match; a swapped picture fails with a diff.
- Release and Debug build OK; all 151 tests pass; all 42 scenes OK, no QML warnings. The `run-chess-app` skill, `ThemeTokens.md` (§1, §6, §11), the plan's §7 and `UiTestSuitePlan.md` are updated.

### Code review (medium, same branch)
9 findings: 8 are fixed (one of them partly), 1 isn't changed.
- **Pages:** Reduce animations makes the page transitions instant, not disabled. Before, a disabled `popEnter` didn't undo the `pushExit` fade, so Home came back blank after Settings if the switch changed in between. `popEnter` also resets `y` after a push that was cut short.
- **High contrast:** the shown move in the move list gets an accent border. It had no highlight at all, since high contrast has no selected fill.
- **Arrow keys:**
  - In the avatar and opponent pickers, an arrow at an edge leaves the selection alone. Before, Up in the first row jumped to the first avatar.
  - An arrow that changes nothing no longer saves the profile. Each step that does change it still saves, as a click does.
- **Profile picker:** on a touchscreen, a tap on the hidden More button now selects the profile instead of opening the menu.
- **Ctrl+O** just calls `loadGame()`, which says itself when there is no saved game, so there's one message, not two.
- **Screenshots:**
  - `Take-Screenshot.ps1` deletes the old picture before each scene, so a scene that fails can't pass with last run's PNG.
  - `Compare-Screenshots.ps1` fails a baseline that has no new screenshot (`MISSING`), and writes a diff image only for failures.
  - The `restore` scene's backups get fixed times (9:00 and 9:30), so all 42 scenes now match exactly.
- Not changed: the opponent picker has no Up/Down. It is a `Flow` with a varying number per row, and Left/Right reach every opponent. The two step functions differ (a grid and a list), so they stay separate.

### Not checked live
- Keyboard in a real window: Tab order on Home, the profile picker, Create profile and Two players; the focus ring; Ctrl+N / Ctrl+O; the page transitions. Screenshots can't show focus (the window never takes it).
- A real Windows contrast theme, 200 % scaling, Windows animation effects off, and the installer's wizard pages. Listed in TASKS under P6.

### Where to continue
BL-02 is coded. What's left are the manual checks in TASKS (P1–P6). Next backlog candidates: BL-09 (engine thread vs GUI thread, a real bug), BL-05 (stub engine, Quick tests), BL-06 (engine choice), BL-04 (dragging pieces).

---

## 2026-10-29 — BL-02 P5 Settings and About done (branch: settingsAbout_291026); next: P6

### What was done
- **Settings page** (`SettingsPage.qml`, pushed over Home or the game like Profile; Back, Esc, Ctrl+,) replaces `SettingsDialog`. One `SectionCard` per group, one `SettingRow` per setting:
  - Appearance: theme; **board coordinates** around the board or on the squares (the squares then fill the board item: `Theme.boardSquares`); **Reduce animations** (`Theme.reduceMotion`: no board turn, piece fade, legal-move dot pop, check glow or turn pulse, sheet slide).
  - Sound: an on/off switch and a volume of 1–5 (muting is the switch only), with a preview sound.
  - Gameplay: **confirm before resigning** (`root.resign()`), **show legal moves**, **highlight the last move**.
  - Computer: the engine's name and path from a short `uci` run (`AppInfo.probeEngine()`, e.g. "Stockfish 19"), or "not found" with what to do; Show levels. BL-06 adds "Change…" there.
  - Data: Back up now, Restore… (`RestoreBackupDialog`: pick a backup; the one equal to `users.json` is disabled; off during a game), Export… / Import…, Open data folder.
  - Advanced: send the game to the engine as a position (FEN) or the moves (`ChessConnector.UseFen`, `config.json`), Open log folder.
  - About: icon, version and build (`Built Oct 29, 2026 · Qt 6.10.2`), GitHub and issue links, licences (Chess++ GPLv3, Qt LGPLv3 + source, Stockfish GPLv3 + source), keyboard shortcuts.
- **`AppSettings.qml`** singleton: the QtCore `Settings` that was `appSettings` in `ChessGame.qml`, with the same keys, so nothing is lost. On load, an unset `theme` comes from the old `darkMode`, and the old "volume 0 = muted" becomes sound off at `lastSoundVolume`. `Theme.scheme` binds to it directly (the `Binding` is gone). Note: `Settings` stores *every* property, so derived values are functions (`effectsVolume()`).
- **`AppInfo`** (C++ service singleton in `ChessPlusPlus`): version from `APP_VERSION_*` (now also defined for C++ in `Directory.Build.props`), build date, Qt version, data and log folders, engine probe. `UciConnector::EngineCandidates()` is the engine lookup that `Init()` and the probe share.
- **`UserManager`**: `backups` (number, date, profile names, `current`), `backupNow()`, `restoreBackup(n)`. A restore first backs up the current profiles, so it can be undone, and keeps the active profile logged in when the backup has it. Saved games aren't in backups (the Data card says so). 3 new tests in `ChessQtTests`; all 149 pass (ChessTests + ChessQtTests).
- **Board colours fixed** (not P5, found while testing the coordinates on the squares): the squares were mirrored, with a1 and h8 light and the queens on the wrong colour. Now a1 is dark (`ChessBoxes.qml`).
- **Scenes:** `about` (Settings scrolled to About) and `restore` (two seeded backups, the dialog open) are new; `settings` shows the page. All 21 scenes OK in light and dark, no QML warnings. The `run-chess-app` skill explains how to run a scene with other settings (an ini in `--data-dir`). After shots: `docs/design/after-p5/`. Debug and Release build OK.

### Follow-ups on the same branch
- **Move list colours** (user report): since P4 every piece was drawn in the text colour, so White's and Black's moves looked alike. Pieces have their own colour again, with a soft glow of the other colour (black glows white, white glows black: `MultiEffect`, `Theme.pieceGlowOnBlack/OnWhite`) so both show on either panel.
- **Code review** (medium): 10 findings, 6 fixed.
  - The engine probe tries every candidate, like `UciConnector::Init()`.
  - `backupsChanged` fires on every save, so "the same as now" stays right; it compares the profiles, not who is logged in.
  - A restore keeps, for profiles that exist now, the saved-game info (the files aren't restored) and the Lichess sign-in (a removed one doesn't come back).
  - The piece fade and dot pop start once per piece, so switching Reduce animations doesn't replay them.
  - The `APP_VERSION_*` C++ defines are for ChessGui only.
  - Not changed: the coordinates are drawn over the pieces on purpose (as on Lichess); a glow per visible move is cheap (`ListView` only creates the visible rows); `backups` reads 4 small files, only while Settings is open.
  - 2 more tests; all 151 pass.

### Not checked live
- Clicking through the page in a real window: each switch in a game (legal-move dots need a selected piece, resign without confirmation, Reduce animations while flipping), the volume preview sound, Back up now / Restore… / the folder buttons / the About links. Listed in TASKS under P5.
- The engine line when Stockfish is missing (here it is on the PATH).

### Where to continue
P6 (Polish), on a new branch from `origin/develop` once this PR is merged: keyboard and accessibility pass, high contrast, motion tokens (`Theme.reduceMotion` exists now; StackView page transitions don't follow it yet), app icon and installer visuals, screenshot baselines (with BL-05). The plan's Ctrl+N and Ctrl+O shortcuts aren't there yet.

---

## 2026-10-29 — BL-02 P3 Users and Lichess done (branch: usersLichess_291026); next: P5

### What was done
- **Pages:** `WelcomePage` on first run: Continue with Lichess (OAuth, then a profile named after the account with a random avatar, or the profile already linked to it), Create local profile, Import a profile. `ProfilePickerPage` replaces `UserLoginScreen`. Its cards are centred and show the avatar, the Lichess username and when the profile last played, and each card has a menu with Export and Delete. With 2 or more profiles the app now starts here; with one, straight to Home. `ProfilePage` (a full page pushed over Home or the game; Back or Esc pops it) replaces the profile dialog: name and avatar apply right away; a Lichess card (ratings for bullet to correspondence, "?" = provisional; Open on lichess.org; Disconnect with confirmation; Advanced: API token); statistics; Export / Delete (Delete is off during a game). `CreateUserDialog` only creates now.
- **Lichess (BL-08, BL-03):** the sign-in moved from `ChessConnector::loginWithLichess` into `UserManager` (`linkLichess`, `linkLichessToken`, `createUserWithLichess`, `cancelLichessLink`), so the token never passes through QML. `LichessAuth::cancel()` closes the local listener and aborts the token exchange: **Cancel** while "Waiting for the browser…". `LichessClient::refreshAccount()` reads the ratings from `/api/account` (on login, on a new token, 3 s after an online game). Any call made with our token that gets a 401 emits `authExpired`; the profile then shows "Lichess sign-in expired · Reconnect", Home's Play online card says so, and the client drops the token.
- **Statistics per opponent**, with draws: the robot by level, each profile, Guest, Lichess. `ChessConnector::recordResult()` runs on mate, resignation and a Lichess game end, once per game. A two-player game between profiles counts for both (checked: the `result` scene gives Tester a loss vs Bob and Bob a win vs Tester). `GameSaveInfo.opponentUserId` keeps the opponent of a continued game. Version 1 totals are migrated: robot games become "level not recorded", the rest "Two players, before results were kept". Plan §9 Q2 is decided: a Guest game counts for the profile.
- **Data safety:** `users.json` and `config.json` are written with `QSaveFile`. On start a readable `users.json` goes to `backups/users-1.json`, and 3 are kept (an unchanged file isn't copied again). If `users.json` doesn't parse, nothing is loaded and an InfoBar offers **Restore** (the newest good backup). The damaged file is kept as `users.damaged-<time>.json`, also when the user starts over instead.
- **Export / import:** `<name>.chessprofile` (JSON, format version 1) holds the profile, statistics, ratings and both saves (base64), and never the token or a pending challenge. Importing adds the profile; on the same id or name, `ImportConflictDialog` offers Import as new ("Bob (2)") or Replace. Replacing the profile that is playing is refused. An imported Lichess link asks to reconnect.
- **Tests:** `ChessQtTests` has 16 new tests: `UserManagerTest.cpp` (14: restart, start page, per-opponent stats, v1 migration, delete, backup rotation, unchanged file, damaged file + restore, damaged file kept, export without token, import on another PC, conflict as new / replace, replace refused, bad files, file name) and `parseRatings` ×2. `UserManager.cpp`, `AppConfig.cpp` and `AvatarProvider.cpp` are compiled into the test DLL. All 143 tests pass (ChessTests + ChessQtTests).
- **Scenes:** `welcome`, `profiles` (`login` is its old name), `profile`, `profile-local`, `profile-expired`, `import`; `create-user` is now the first-run dialog. Their profiles come from `seedScreenshotProfiles()` in `main.cpp` (placeholder token, no network in screenshot mode). All 19 scenes OK, no QML warnings. After shots: `docs/design/after-p3/` (light and dark).

### Code review fixes (same branch)
A medium `/code-review` found 10 issues; 8 were real and are fixed, 2 can't happen (Delete is off during a game; the autosave runs only when leaving a game, not per move).
- A damaged `users.json` is **copied** to `users.damaged-<time>[-n].json`. If that fails, nothing overwrites it. Restore is only offered for a backup that has a `users` array.
- A login cancels a pending Lichess sign-in, so a late browser sign-in can't switch profiles. A profile made by Continue with Lichess gets "Name (2)" if the name is taken, like an import.
- The autosave keeps the robot's level (`computerLevel`): Continue plays at that level and the statistics use it, even if another profile changed `lastLevel`. It also keeps `resultRecorded`, so a mate that was undone and then played again after Continue doesn't count twice.
- Import reads the file once. A 401 is reported once per reply.
- `ChessQtTests`: 2 more tests (backup without users, autosave level and counted flag). All 145 pass, and all 19 scenes are OK.

### Not checked live
- Continue of a game vs the robot at a level other than `lastLevel` (needs Stockfish; the saved fields are unit-tested).
- Anything that needs lichess.org: OAuth from Welcome and Profile, Cancel, the API token, ratings, a real 401 (revoke the token on lichess.org). Listed in TASKS under P3.
- Export / import through the real file dialogs, delete, and Restore from a damaged file in a real window (the logic is unit-tested).

### Where to continue
P5 (Settings and About pages), on a new branch from `origin/develop` once this PR is merged. Settings → Data can reuse `exportProfile` / `inspectImport` / `importProfile` and `restoreFromBackup`; "Back up now" needs a new `UserManager` call.

## 2026-10-29 — Check glow on the king (branch: checkHighlight_291026)

- The king in check glows red-yellow and pulses gently, so a missed "Check" InfoBar doesn't leave the player wondering why most moves are refused. `ChessConnector::CheckedKing` gives the square for the position on the board (live, reviewed, or a saved game's replay); `Game::IsInCheck()` (side to move) is new in ChessCore with a test. The glow is a `QtQuick.Shapes` radial gradient in `ChessBoxes.qml` (`Theme.checkGlowCenter` / `checkGlowEdge` replace the unused `checkSquare`).
- Checked: `game` with `e2e4 f7f6 d1h5` (light; dark and flipped with Bb5+), a mate (the red flagged king on top), and a reviewed check that is blocked live. No QML warnings. ChessTests 86/86.
- Independent of P3 (#84): branched from `develop`.

---

## 2026-10-29 — PGN date kept in saves (branch: pgnGameDate_291026)

- Hand check by the user: Copy PGN and Export PGN work, but a continued game exported `[Date "????.??.??"]`.
- `GameSaveInfo.startDate` (`startDate` in users.json) is written with the autosave and the saved game, and read back by Continue and Open saved game. Saves made before this still give `????.??.??`. Checked: the `continue` scene's autosave holds `"startDate": "2026-10-29"`. `ChessTests` 85/85.

---

## 2026-10-29 — P4 code review fixes (branch: p4ReviewFixes_291026); next: P3

A `/code-review` of #81 found 10 issues. I checked all of them: 9 were real, and 1 (the overlapping review properties) was mostly style. This branch fixes 9, including the style one; #2 became backlog item BL-09.
- **Rematch after closing the result card:** the moves panel now shows Rematch and New game, and the sheet handle shows a Result button that reopens the card. The new `ended` scene shows this.
- **Review moves** on the result card goes to the start position (`reviewFirst()`); in a narrow window it also opens the sheet.
- **Move list:** after a single move, only that move is formatted. `Game::IsCheckMate()` supplies the mate, and `UndoMove` now clears it. The full replay runs only after undo, load or several moves at once. In `makeMove`, an exception now only covers `DoMove`.
- **Captured pieces:** they come from `HistoryMove::CapturedPiece()` in ChessCore. A `Game` records en passant as a normal capture, so the old GUI special case never ran.
- **PGN:** the date is the day the game started. A saved or continued game gets `????.??.??` and no date in the file name. The suggested export name is URL-encoded, so `#` and `%` in names work. Export rejects URLs that aren't local files.
- Esc no longer needs two presses after the window grows wide with the sheet open. `capturedChanged` fires once per update. `PlyCount` replaced `ReviewIndex`, which QML no longer used.
- `ChessTests`: 85/85, 5 new (`CapturedPiece` ×3, `IsCheckMate` set by mate and cleared by undo). All 15 scenes OK in a real window.
- **Not fixed here, now BL-09:** the engine thread changes `_game` while the GUI thread reads it.

---

## 2026-10-29 — BL-02 P4 Game chrome done (branch: gameChrome_291026); next: P3

### What was done
- **Player cards** (`PlayerCard.qml`) above and below the board: avatar, name (`ChessConnector.PlayerName` / `OpponentName`), the robot's level and Elo, the turn (accent bar and avatar pulse), and a **captured strip** (`CapturedStrip.qml`). The strip groups pieces by type (P N B R Q), overlaps pieces of a type, and shows "+N" (`MaterialBalance`, counted from the board, so promotions count). Captures and material follow the reviewed position. When the board is flipped, the cards swap.
- **Moves:** a side panel (`MovesPanel` + `MoveList`) at width ≥ 960 or aspect > 1.15; otherwise a **bottom sheet** whose handle shows "Moves 12. Nf3", the status, and Back to live while reviewing. Esc closes the sheet first. The move list has two columns and a move-number column, piece images in the text colour, and tabular figures. **Click a move** to review it (`reviewTo(ply)`; `ShownPly` is highlighted). While reviewing, a slim InfoBar "Viewing 5. Nf3 · Back to live" sits at the bottom of the panel. The blinking button and "Reviewing" are gone.
- **Result:** one `ResultCard` over the dimmed board in every layout, with Rematch, New game (the setup dialog of the same mode; online goes Home → Lichess dialog) and Review moves. `GameResultDialog`, `GameResultCard`, `MoveHistoryPanel` and `CapturedPanel` were deleted. Their statistics moved to a "Statistics" line in the profile dialog until P3's Profile page.
- **PGN:** Copy PGN and Export PGN… are in the panel and under More. Export uses a `FileDialog` that suggests "White vs Black yyyy-MM-dd.pgn" and writes with `QSaveFile`; an InfoBar confirms. The tags are Event, Site (the lichess.org URL online), Date, Round, White, Black and Result.
- **ChessCore:** `Game::FormatHistorySan()` replays the game (in `Game`, so `San` stays free of `Game.h`) and marks '#' from the real mate event, no longer from the result string. `FormatPgn()` escapes tag values and wraps at 80 columns. `PieceValue()`/`MaterialBalance()` are in `Board.h`. `ChessTests` has 80 tests, 13 new (history SAN, PGN, material). En passant now counts in the captured strip.
- **Scenes:** `review`, `result` and `moves` are new; `game` and `continue` work as before. `Take-Screenshot.ps1 -Offscreen` works when the screen is locked, dark only (see Notes). Debug and Release build OK; no QML warnings in the scene logs.

### Notes
- **Locked screen:** while Windows is locked, a real window renders no frames. Animations freeze (half-closed dialogs, empty squares) and the mate scene times out, on `develop` too. Use `-Offscreen` there: Fluent is always dark offscreen, so `Theme.dark` follows it. Dialog backgrounds don't render offscreen. After unlocking, all scenes (including the mate) worked in a real window. `docs/design/after-p4/` has the light shots (real window) and the dark ones (offscreen).
- Board coordinates are still outside the squares (the inside/outside setting is P5).

### Where to continue
P3 (Users and Lichess), on a new branch from `origin/develop` once this PR is merged.

---

## 2026-10-29 — BL-02 P2 Shell and navigation done (branch: shellNavigation_291026); next: P3 or P4

### What was done
- **Pages:** `ChessGame.qml` (986 → 491 lines) is the window: a `StackView` holding `UserLoginScreen`, `HomePage` or `GamePage`. `route()` picks the page from `UserManager.isLoggedIn` and `ChessConnector.InGame`. All dialogs live in the window and are opened with `open()`; pages emit signals for them. The `_show*` flags and `screen_1…4` states are gone.
- **Home:** an account button (avatar and name; menu with Profile and Switch user), Settings, a **Continue game** card (`ChessConnector.ContinueDescription`, e.g. "vs Bob"), and cards for Play vs computer, Two players, Play online (dimmed until Lichess is linked; then it opens Profile) and Open saved game (always enabled; it explains when there is nothing to open). The Exit button is gone.
- **Setup dialogs:** `ComputerGameDialog` (White/Black/Random and level; its info button swaps to the levels help and back, so dialogs never stack) and `TwoPlayerDialog` (side, and the opponent picked by profile **id** or Guest). Both use the new shared `ChoiceRow`.
- **Game command bar:** Home, title (`ChessConnector.GameTitle`: "vs Robot · Level 3", "Alice vs Bob", "Lichess · vs X", "Saved game · …"), Prev/Next (replay), Undo/Takeback, **Flip board** (new), Save, Offer draw, Resign (with `ConfirmDialog`), More (Computer levels, Settings, Profile, Switch user). Undo, Draw and Resign are no longer duplicated in the move panel.
- **InfoBar** replaces `Notificator` and the three banners: one message bar (Check, Game saved, errors, rematch declined) plus bars bound to `ChessConnector.PendingOffer` (draw or takeback) and `OpponentGone`/`ClaimWinSeconds`. Castling is no longer announced.
- **Game flows in C++:** `startComputerGame(level, side)`, `startLocalGame(white, opponentUserId)`, `switchUser()` (autosave, end the game, log out) and `refreshBoard()` (a new board view fills itself in). Lichess game start and end are handled in `ChessConnector`, which sets the result text, flags the king and sets the opponent's avatar. Game-end text is now `ResultText`, with names: "Bob wins by checkmate", "Alice resigned". `GameSaveInfo.opponentName` is stored for Continue and the title. The avatar is set per mode (the `newGameStarted → randomize` connection in `main.cpp` is gone), so a rematch keeps the opponent's avatar.
- **Typed singletons:** `qmlRegisterSingletonInstance("ChessPlusPlus", …)` for `ChessConnector`, `UserManager`, `LichessClient` and `AvatarProvider`, parented to the app so they outlive the engine. No `setContextProperty` is left.
- **Shortcuts** (off while a dialog is open): Esc Home, F flip, Ctrl+S, Ctrl+Z, ←/→/Home/End review, F1 levels, Ctrl+, settings, Alt+S move input. Minimum window size 640×560; the window title shows the game.
- **Scenes:** `home`, `continue`, `game` (Tester vs Bob), `computer`; `menu` and `board` still work as aliases. After shots: `docs/design/after-p2/`. Debug and Release builds OK; `ChessTests` 67/67; no QML warnings in the scene logs.

### Not checked live
- Keyboard shortcuts and Enter/Esc in a real window. Driving the window with SendKeys didn't reach the Qt controls, so the skill now says to add a scene instead.
- Lichess flows (challenge, game start and end, draw/takeback InfoBars, opponent gone) need a connected account. Add them to the Lichess live test in TASKS.

### Where to continue
P3 (Users and Lichess) and P4 (Game chrome) can run in parallel; each needs a new branch from `origin/develop` once this PR is merged. P3 replaces `UserLoginScreen` with the Welcome page and profile picker, and moves `CreateUserDialog`'s Lichess part to a Profile page. P4 replaces `CapturedPanel`, `GameResultDialog`/`GameResultCard` and `MoveHistoryPanel`.

---

## 2026-10-29 — BL-02 P1 Foundation done (branch: fluentFoundation_281026); next: P2

### What was done
- **Style:** `QQuickStyle::setStyle("FluentWinUI3")` in `main.cpp`. The palette block, inline `Button`/`ModeButton` and every `background:`/`contentItem:` override are gone. `Button.qml`, `ActionButton.qml`, `CustomSlider.qml`, `NetworkPlayers.qml` and `menu_icon.svg` were deleted.
- **`qml/Theme.qml`** singleton (`qml/qmldir`): colour roles, board, type ramp, spacing and motion as in `docs/design/ThemeTokens.md`; §8 there lists the differences. No `Window.window.color` and no hex colour outside `Theme.qml`.
- **System / Light / Dark:** `appSettings.theme` (Settings → "App theme"). It sets `Application.styleHints.colorScheme`; until it is set, the old `darkMode` decides. "System" follows Windows; checked live.
- **Icons:** `pics/icons/*.svg` are in `qml.qrc` under `qrc:/icons/` (`Theme.icon("undo")`). New `IconButton.qml` (icon-only `ToolButton` with tooltip and `Accessible.name`) is used in the command bar and the move panel.
- **`AppDialog.qml`:** a modal Fluent `Dialog` on the overlay with the `Theme.scrim`. Enter runs `accept()` and Esc runs `reject()`; footers are `DialogButtonBox`es. Settings, Profile/Create profile, Levels, Lichess, Challenge, Select player and Apply moves use it. Shown with `visible: _showX; onClosed: _showX = false`.
- **Board:** the `Glow` is replaced by an accent tint on the selected piece's square, cleared after a move. Legal-move dots use `Theme.legalMoveDot`, and coordinates use `textSecondary`. The photo shows only behind the start menu and login, faint.
- **Screenshots:** `-platform offscreen` can't force light/dark, and Fluent falls back to its dark palette there. Screenshot mode now uses a real window that is transparent, frameless, ignores input and has no taskbar button, at 100% scale. `Take-Screenshot.ps1` dropped `-Windowed`. New scenes: `online`, `players`. After shots: `docs/design/after-p1/`.
- Keyboard checked live: Enter creates a profile, F1 opens levels, Esc closes the dialog. `windeployqt` deploys the FluentWinUI3 plugin and its assets. Debug + Release build OK; `ChessTests` 67/67.

### Where to continue: P2 Shell and navigation (new branch from `origin/develop`)
Spec: plan §6 and §8 (P2 row). `StackView` pages, command bar, account menu, Home, InfoBar replaces `Notificator` and the banners, `_show*` flags removed, services as typed singletons, game flows into `ChessConnector`.

### Notes
- Not seen in a scene yet: the connected Lichess dialog, the Challenge dialog, the draw/takeback banners and the two resign confirmations. Check them during the Lichess live test (TASKS).
- `windeployqt --qmldir ChessGui\ChessGui` locally still lists Qt5Compat, but only because stale Qt VS Tools caches (`ChessGui\ChessGui\x64\*\qt\qmake\*\import.qml`) import it. CI starts clean.
- The board's own animations (elastic flip, InCirc piece rotation) are unchanged; motion tokens come in P6.

---

## 2026-10-28 — App modernisation planned (BL-02/BL-03); next: Phase 1

### What was done
- **#76 Screenshot mode:** `ChessGui.exe --screenshot <png> --scene <name>` (with `--moves`, `--black`, `--dark`, `--size`, `--delay`) on a throwaway profile; `--data-dir <dir>` for interactive runs. `scripts\Take-Screenshot.ps1 -Scene all [-Dark] [-Windowed] [-Build]` writes to `test-results\screens\`. The `run-chess-app` skill explains it.
- **#77 Modernisation plan:** `docs/AppModernizationPlan.md` holds the screen-by-screen review, target flows, roadmap P1–P6 and the mockup link (§10).
  - Design kit: 38 icons in `ChessGui/ChessGui/pics/icons/` (not in `qml.qrc` yet), `docs/design/ThemeTokens.md`, and "before" screenshots in `docs/design/before/`.
  - Decisions: Windows 11 Fluent (`FluentWinUI3` style, system theme and accent); local profiles with a linked Lichess account; profile export/import plus backups.
  - QML ↔ C++ rule (CLAUDE.md "Key Conventions"): game flows go through `ChessConnector`, one call per action; `UserManager` and `LichessClient` stay separate services and become typed singletons in P2.
- `TASKS.md`: BL-02/BL-03 is at the top of *In Progress*, with one sub-item per phase.

### Where to continue: P1 Foundation (new branch from `origin/develop`, e.g. `fluentFoundation_<ddmmyy>`)
Plan first (`/plan`), then implement. Spec: plan §6 and §8, `docs/design/ThemeTokens.md`.
1. `main.cpp`: `QQuickStyle::setStyle("FluentWinUI3")`. Remove the per-file `background:`/`contentItem:` overrides that fight the style: inline `Button` and `ModeButton` in `ChessGame.qml`, `Button.qml`, `ActionButton.qml`, `CustomSlider.qml`, the primary-button copies in the dialogs. Prefer plain Fluent controls (`highlighted: true` for the primary action).
2. `qml/Theme.qml` singleton (`qmldir` + `qml.qrc`). It replaces the 16 `Window.window.color` dark-mode blocks. System/Light/Dark sets `Application.styleHints.colorScheme`; migrate `appSettings.darkMode`.
3. Icons: add `pics/icons/*.svg` to `qml.qrc` and use them via `icon.source` + `icon.color`. This replaces the `ColorOverlay` menu icon and glyph labels ("⚑", "↩", "|<<").
4. `AppDialog` base: modal, with a scrim and Enter/Esc. Move the existing dialogs onto it (Settings, Profile, Levels, Lichess, Challenge, Select player, Apply moves).
5. Remove `Qt5Compat.GraphicalEffects` (`ColorOverlay` in `ChessGame.qml`; `Glow` in `ChessFigures.qml`, which becomes an accent square tint) and the unused `NetworkPlayers.qml`. That closes BL-01.
6. Keep every screenshot scene working (`setUpScreenshotScene()` in `ChessGame.qml`). Before/after shots, light and dark, go in the PR.

Out of scope for P1: the page split and navigation (P2), and piece moving/drag (BL-04).

### Notes
- After P1, offscreen screenshots should match windowed ones, because Qt5Compat effects are what rendered blank offscreen.
- Login cards are left-aligned (`UserLoginScreen.qml` `Flow`); this is fixed in P3, not P1.
- Manual checks still open (no code): VC++ runtime install, Stockfish install log, Lichess rematch live test. They're listed under *In Progress* in `TASKS.md`.

---

## 2026-10-28 — Lichess Rematch (branch: feLichessRematch_281026)

### What was done
- Merged earlier today: computer levels rework + levels help (#68), promotion dialog rotation fix (#69).
- Analysed why Rematch doesn't work vs Lichess: the button is hidden for online games and the Board API has no rematch endpoint. Plan written to `docs/LichessRematchPlan.md`: Rematch creates a new challenge (direct to a registered opponent, open link for anonymous).
- Implemented steps 1–4 (see `TASKS.md`): `LichessClient::createRematch()` / `parseRematchInfo()`, `ChessConnector::CanRematch`, Rematch button for online games, `ChallengeDialog` direct mode. Direct challenges use `keepAliveStream=true` (otherwise they expire after 20 s).
- Debug + Release build OK; `LichessStaticTests` 21/21 (4 new); app starts without QML errors.
- Testing showed an opponent's resignation was received (`status:"resign"`) but nothing was shown: result and Rematch lived only in the landscape side panels. Added step 4b (result card, notification, red flag on the losing king, frozen board, `opponentGone` + Claim victory).
- Note for testers: resigning on lichess.org needs the flag **and** the ✓ confirm within ~3 s; otherwise nothing reaches the server.

- Code review fixes (step 4d) and PR #70 opened. Live check OK for resign from the app and from an anonymous Lichess opponent.

- Merged: #70 (rematch + game end) and #71 (test host crash fix). `ChessQtTests` engine tests now run reliably (10/10).

### Where to continue
1. Play a game against a second Lichess account and run the manual scenarios in plan §5 (accept, decline, cancel, anonymous link, vs Robot unchanged, restart, leave while pending).
2. Watch the log for `LichessClient: challenge stream line:`. It confirms the keep-alive challenge JSON and the final `{"done":...}` shape this code expects.
3. If a live scenario fails, fix it on a new branch from `develop`.

### Notes
- If the opponent clicks Rematch in their browser, the Board API client probably never sees it; the app must send the challenge.

---

## 2026-10-27 — Release 0.1.0 preparation (branch: feRelease010_271026)

### What was done
- CI/CD re-checked. It is MSBuild-based, not CMake: `tests.yml` (Debug build + `ChessTests` on every push/PR) and `release.yml` (Release build, windeployqt, signing, Inno Setup installer). Last runs on `develop` and the PR installer build (2026-06-07) were green.
- `release.yml`: release step now uses `docs/release-notes/<tag>.md` as the body (generated notes appended) and fails if no installer is found.
- `installer/ChessSetup.iss`: `AppPublisherURL` corrected to `novaua/qt-chess`.
- Added `docs/release-notes/v0.1.0.md`; README links to the first release.

### Where to continue
1. Merge this branch to `develop` once CI is green.
2. Tag `v0.1.0` on `develop` and push the tag; the Build Installer workflow publishes the GitHub Release with `ChessSetup-0.1.0.exe`.
3. Check the release page, then install the EXE on a clean machine and run it.
4. The release-notes link to the EXE is `.../releases/download/v0.1.0/ChessSetup-0.1.0.exe`; confirm it resolves.

### Notes
- The old CMake pipeline (feMakeInstaller_0430a) was replaced by the MSBuild workflow; there is no `CMakeLists.txt`.
- The app does not display its version anywhere; the version only lives in the tag and installer name.

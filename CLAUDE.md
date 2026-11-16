# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Qt-chess is a chess application with a Qt QML frontend and C++ backend, supporting local two-player, UCI engine (e.g., Stockfish), and network multiplayer modes.

## Build System

**Primary build**: Visual Studio 2022 solution — `ChessCore.sln`

**Qt GUI project** also has a `.pro` file for Qt Creator: `ChessGui/ChessGui/ChessGui.pro`

**Dependencies**: Qt 6.12+ only (with the Qt Multimedia add-on) — no third-party C++ libraries and no package manager (vcpkg was removed once Boost was replaced by Qt).

**Build configurations**: Debug|x64 and Release|x64 are the relevant targets.

## Running Tests

Tests use **Microsoft CppUnitTest Framework** (built into Visual Studio).

- Open `ChessCore.sln` in Visual Studio
- Build the `ChessTests` and `ChessQtTests` projects
- Run via **Test Explorer** (Test → Test Explorer) or:

```
vstest.console.exe x64\Debug\ChessTests.dll
vstest.console.exe x64\Debug\ChessQtTests.dll
```

- `ChessTests/` — chess logic (`BasicTest.cpp`, `CheckTests.cpp`, `FenGenTests.cpp`, `SanTest.cpp`, `UndoTests.cpp`, `BoardDiffTests.cpp`, `MatingMaterialTests.cpp`, `PremoveTests.cpp`, `FakeEngineTests.cpp`). Plain C++, no Qt.
- `ChessQtTests/` — the Qt parts: `UciConnectorTest.cpp` (UCI protocol), `ChessEnginePlayerTest.cpp`, `LichessClientTest.cpp`, `BoardPiecesTest.cpp`,
  `MoveListModelTest.cpp`, `UserManagerTest.cpp`, `GameClockTest.cpp`, `ClaimWinTest.cpp`, `GameLibraryTest.cpp` (the games library, BL-19), `GamePgnTest.cpp` (a kept game's PGN), `GameAnalysisTest.cpp` (the engine's analysis of a kept game, BL-19 step 6; FakeUci, built with the tests, and one test on Stockfish). Needs Qt's `bin` on `PATH`; the engine tests need Stockfish.
  `LichessClientTest.cpp`'s `LichessReconnectTests`, `LichessStreamTest.cpp` and `LichessChallengeTest.cpp` play against `FakeLichess` (`FakeLichess.h`), a `QTcpServer` on 127.0.0.1 (no Lichess).
- `ChessGuiTests/` — the UI tests (BL-05, `docs/UiTestSuitePlan.md`): **Qt Quick Test**, a console exe, not in Test Explorer; GitHub runs them on every push (`tests.yml`, Debug; a failure uploads `test-results\ui` as `ui-test-results`). `qml/tst_*.qml` (first run, two players, the board, promotion, the robot, profiles, a restart, Lichess, the games library, the analysis) play the app's own window (`ChessGame.qml`) with real mouse events, offscreen (also with the screen locked); `qml/AppDriver.qml` has the helpers (`clickMove`, `dragPiece`, `pieceAt`, `waitPiece`, `waitBoardIdle`, `waitMessage`, `startTwoPlayerGame`, `leaveToHome`, `scrollIntoView`, …), items are found by `objectName`. One test file per process on a fresh profile (`TestSetup`, the app's `AppServices`; a file with `// ui-test-runs: 2` runs twice on one data folder, `TestRun.run`: a restart); the robot is FakeUci (`app.startComputerGame(line, thinkMs)`), no Stockfish needed; Lichess is `FakeLichess` (`fakelichessserver.h`, a local server the test plays the other side on; `TestRun.linkLichess`), never lichess.org; `TestRun.addFinishedGames(n)` fills the games library without playing. It builds every ChessGui `.cpp` except `main.cpp`: **a new ChessGui source goes into `ChessGuiTests.vcxproj` too** (the link fails without it). Run:

```
scripts\Run-UiTests.ps1 -Build              # every test file; -Name tst_twoPlayers -Function test_2_newGameAndDrag for one
scripts\Run-UiTests.ps1 -Build -Screens     # the pre-PR check: also every screenshot, compared with the baselines
scripts\Run-UiTests.ps1 -Name tst_twoPlayers -Visible   # watch it in a real window, paced; a failure waits until you close it
```
A failed test function leaves a picture of the window: `test-results\ui\<TestCase>-<function>.png` (listed with the failure).

## Seeing the UI

`scripts\Take-Screenshot.ps1 -Scene all` saves screenshots of the app's views (invisible window) to
`test-results\screens\` on a throwaway profile. `scripts\Compare-Screenshots.ps1` diffs them against
local baselines (`-Update` accepts the current look). Details: the `run-chess-app` skill
(`.claude/skills/run-chess-app/SKILL.md`).

The installer's smoke test (BL-05 step 6): `installer\Test-Installer.ps1`, run by `release.yml` on every PR and
release after the installer is built: installs it silently, runs the installed app on five screenshot scenes
with Qt off `PATH` (only installed files count), checks exit codes, pictures and logs, uninstalls. Locally only
`-AppDir <folder>` (it refuses to install where Chess++ is installed).

App icon and installer images: `python scripts\make_icons.py` (Pillow) redraws `chess.ico`,
`pics/app-icon.png` and `installer/art/*.bmp`.

## Architecture

Four projects in the solution:

```
ChessGui (QML app)
  └─ ChessConnector (C++ ↔ QML bridge)
       └─ Game (game state, event dispatch)
            ├─ Board (8×8 state, piece tracking)
            ├─ MoveGeneration (legal move calculation)
            ├─ Check (check/checkmate detection)
            └─ ChessEnginePlayer
                 └─ UciConnector (QProcess → external engine)
```

### ChessCore (static library)

All chess logic, isolated in the `Chess::` namespace. Key types:

- `Board` — 64-square state; owns piece positions
- `Game` — orchestrates turns, validates moves, fires events to listeners (observer pattern via `EventBase`/`PawnPromotionEvent`); `GetPremoveTargets` for premoves (LI-05)
- `Move` / `HistoryMove` — move representation (from/to squares, capture flag, promotion)
- `MoveGeneration` — generates pseudo-legal and legal moves including castling and en passant
- `ChessEnginePlayer` — wraps `UciConnector` to produce computer moves
- `Serializer` — the binary save format of versions before BL-19 (`Game::Save` / `Load`), read only to migrate the two old save slots; `Game::LoadMoves` replays a UCI move list into the loaded history (a kept game's replay)
- `BoardDiff` (`BoardDiff.h`) — which piece went where between two positions (moved, removed, added), for the board's piece animations

### UciConnector (static library)

Spawns and communicates with external UCI engines via `QProcess`. Main API: `Init()`, `NewGame()`, `GetEngineMove()`.
The engine is `./engines/stockfish.exe`, else one on `PATH`; `CHESS_ENGINE_PATH` names the only one instead (the tests' FakeUci).

### FakeUci (console exe, tests only)

A stand-in UCI engine on `ChessCore`, no Qt (`FakeUci/fakeengine.h`, BL-05 step 3): answers what the app asks of Stockfish
at once and the same way every time: the move of the game in `FAKEUCI_MOVES` at that ply when legal there (a promotion without its piece: a queen), else the first legal
move by its UCI text (said on stderr); its one line's score is `mate 1` when the move mates, else the material after it (the analysis tests); `FAKEUCI_DELAY_MS` before each answer; `moves` (not UCI) lists its legal moves. The UI tests
(`TestSetup`) and the robot screenshot scenes (`Take-Screenshot.ps1`) run it; its tests are `ChessTests/FakeEngineTests.cpp`.

### ChessGui (Qt QML application)

- **`chessconnector.h/cpp`** — the C++/QML bridge for the chess game; exposes `Q_PROPERTY` bindings and slots that QML calls. Premove (LI-05, vs the robot and on Lichess, not two players): during the opponent's turn `figureSelected` / `movePiece` set `PremoveFrom`/`PremoveTo` instead of moving (`PremoveDots`: the dots are premove targets); it is played right after the opponent's move if legal then, else dropped silently; `cancelPremove()` (right click, Esc). `acceptChallenge(id)` (BL-10): accepts a Lichess challenge; the game on the board stays until the new one starts (a local game is then kept in the library), so a refused Accept loses nothing; not during a running real-time Lichess game (`CanAcceptChallenge`). BL-17: `continueOnlyGame()` (Home's Continue: the only unfinished game, else false and the list opens), `continueOnlineGame(id)` (rejoin a Lichess game: `StartingOnlineGame` and `StartingGameId` until it starts, `gameStartFailed`; `AcceptingChallengeId`: the challenge `acceptChallenge` is accepting, one state for the bar and the Online page, BL-16; dropped by `cancelOnlineStart()` or another profile). BL-19: `continueGame(id)` / `openGame(id)` for a kept game (see `gamelibrary`); leaving a local game never asks, it is kept as it is. Play from here (BL-19 step 4): `PlayFromHere` (only while reviewing: the position shown of a replay, or of a local game stepped back: `{ gameId, ply, move, whiteToMove }`, `{}` when none or not playable; step 4b) and `startComputerGameFrom` / `startLocalGameFrom(gameId, ply, …)`: a new game with that game's first moves (a fork: `forkOfId`, `forkPly`; those moves aren't taken back and the clock doesn't count them); the review bar's **Play from here** button (`MovesPanel`, the sheet handle in a narrow window) → its menu (vs computer… / Two players…) → the setup dialogs' `openFrom(position)`
- **`boardpieces.h/cpp`** — `BoardPieces`, the pieces model behind `ChessConnector.Pieces`: one row per piece that keeps its id while it moves (`BoardDiff`), so the board can slide it
- **`movelistmodel.h/cpp`** — `MoveListModel`, the move list behind `ChessConnector.MoveHistory`: one row per move number, and a move changes only its row instead of rebuilding the list
- **`movegate.h/cpp`** — `MoveGate`, which holds the moves from outside (the robot's answer, the Lichess opponent's) and releases them one at a time when the board is idle (a Lichess resync of more than two: all at once; in a real-time Lichess game the opponent's moves don't wait for the board, only for the page: `setHoldOnline`, LI-06); `ChessConnector` owns one, `GamePage` reports the board settled and the slide time
- **`gameclock.h/cpp`** — `GameClock` (`ChessConnector.Clock`), the two clocks of a game, as on lichess.org: `ChessConnector` drives it with `moveMade()` in a local game, Lichess's times come with `setSnapshot()` and never flag here, and a correspondence game shows the time left for the move (LI-02). `Board::CanMate` decides whether a flag loses or draws
- **`gamelibrary.h/cpp`** — `GameRecord` and `GameLibrary` (BL-19, QML singleton `GameLibrary`): every local game (vs the robot, two players) kept from its first move in `users/<id>/games.json`, finished or left, newest first. The rating vs the robot (step 5): `LocalRating` (Elo from 1200, provisional under 10 games) stored as it goes in `games.json`: `put` rates a finished robot game without takeback or fork once (`rated`, `ratingBefore`, `ratingChange` on the record), gives the change back when a takeback resumes it; removing games never changes it (`rating`, `ratedGames`, `ratingProvisional`; Home's header, the Profile page, the cards, the PGN's player Elo) (`games`, `unfinishedGames`, `count`, `unfinishedCount`, `finishedCount`, `remove(id)`); the last `keepCount` finished games are kept (Settings → Data "Games kept", `AppSettings.gamesKept`, default 100; a smaller number asks first: `ChessGame.setGamesKept`), unfinished ones never removed on their own; follows the active profile; the old `chess.autosave` / `chess.save` of a profile are migrated once (one that can't be read is kept as `.damaged`; a Lichess autosave isn't migrated). `put` / `remove` write `games.json` once the current event is done (`flush()` writes now; the destructor and a profile change do); a `games.json` that is there but can't be opened is never written over (read again at the next write, its games merged). "Games kept" is every profile's: `finishedBeyond(keep)` counts them all for the confirm, `setKeepCount` trims every profile's file. `GameRecord::play` / `playOn` play kept moves (false at one that can't be played, a move from an empty square too); `continueGame` / `openGame` refuse such a record (`keptGameFailed`). `ChessConnector` writes the record with every move, takeback, result and when the game is left (`updateRecord`, `keepGameOnLeave`), `continueGame(id)` resumes an unfinished one (the robot at its level, or two players; the clock as left) and `openGame(id)` replays any (mode `"replay"`). Lichess games have no record (the owner's choice)
- **`gameanalysis.h/cpp`**, **`gameanalyzer.h/cpp`** — BL-19 step 6: `GameAnalysis`, the engine's analysis of a kept game (per position the eval from White's view and the best move; stored on the record, `GameRecord::analysis`; win chances, judgements at 5 / 10 / 15 points and accuracy as lichess.org, worked out when read); `GameAnalyzer` runs it on a thread with a full-strength engine of its own (`AnalysisWorker`), kept only if the record's moves are still those analysed (`GameLibrary::setAnalysis`; `put` drops a stale one). `ChessConnector.analyseGame(id)` / `cancelAnalysis()`, `Analysing`, `AnalysisDone` / `AnalysisTotal`, `AnalysisDepth` (Settings → Computer "Analysis depth"). Step 7, the board's game: `CanAnalyse` (a kept game that is over, or a replay: never one going on), `AnalysingThisGame`, `Analysis` (per side accuracy and `?!` `?` `??` counts), `ShownAnalysis` (the shown position's eval, the move's mark and the engine's best); `MoveListModel`'s `wMark` / `bMark`; QML: `AnalysisSummary` (the moves panel), `EvalBar`, Analyse on the result card and in More
- **`gamepgn.h/cpp`** — `GamePgn` (BL-19 step 3): a game as PGN, the seven standard tags plus the robot's Elo and `TimeControl`; `ChessConnector::pgn(gameId)` / `pgnFileName(gameId)` / `exportPgn(file, gameId)` write the game on the board (`""`) or a kept game (a Games card's Export PGN) with it
- **`claimwin.h/cpp`** — `ClaimWin`, the Lichess opponent left: Lichess's countdown to the claim and "only on the opponent's turn" (Lichess refuses it on the player's), behind `ChessConnector.OpponentGone`, `ClaimWinSeconds` and `CanClaimVictory`; `claimVictory()` posts only then
- **`frametrace.h/cpp`** — `FrameTrace`, frame timing in the app log, off unless `QT_LOGGING_RULES="chess.frames.debug=true"` (BL-14); `GameStartTiming`, always on (`chess.timing`): a `[Timing]` line from the action that starts a game to its first frame, with its steps (`ChessConnector` marks them, `GamePage` calls `gameShown`), and the engine stop's wait (BL-11)
- **`layoutgate.h/cpp`** — `LayoutGate` (QML type in `ChessPlusPlus`): `open` from the first frame's polish pass, once a page's layout has settled; costly items behind it (`Loader { active: gate.open }`) are made once at their real size and are still in that frame (BL-11: the board's layers, the moves panel)
- **`sceneinput.h/cpp`** — `SceneInput`, screenshot mode only (QML singleton): mouse clicks and drags sent to the window, for scenes that play as a user does (`play`, BL-11)
- **`appservices.h/cpp`** — `AppServices`: the five QML services, how they work together (the Lichess token, account, ongoing games) and their QML registration; `main.cpp` and the UI tests' `TestSetup` both use it (BL-05)
- **`UserManager`**, **`LichessClient`** (LiChessConnector project; `ongoingGames`: the unfinished Lichess games, LI-04; a game stream lost mid-game (closed, or 20 s without even a keep-alive line) reconnects on its own (`LichessStream`: one ndjson GET with the silence watchdog, the 1 → 15 s backoff, 4xx stops it; BL-10 step 1) and its `gameFull` is a resync, as is a move POST without an answer: `gameConnectionChanged`, `ChessConnector.Reconnecting`, LI-06; `setBaseUrl` / `setStreamTiming` for the fake-server tests; the account's event stream, open while there is a token (`startEventStream`, from `AppServices`; one per token, shared by the challenge wait and the rematch watch), gives `incomingChallenges` / `challengeReceived` / `declineChallenge` / `challengeAcceptFailed`; `rejoinGame` has its own `rejoiningGameId` / `rejoinFailed`, not `networkError`, BL-17; `gameStarted` carries the game's moves so far, which `ChessConnector::playJoinedMoves` puts on the board before the game page opens, BL-18 (`rematchOfferId`: the one the rematch bar shows) and refreshes `ongoingGames` on games started or ended elsewhere, BL-10), **`AvatarProvider`**, **`AppInfo`** (version, data/log folders, the engine's name) — app services without chess logic, also visible to QML (see Key Conventions)
- QML files in `ChessGui/ChessGui/qml/`:
  - `ChessGame.qml` — the window: a `StackView` with one page (`WelcomePage` on first run,
    `ProfilePickerPage`, `HomePage`, `GamePage`; `route()` picks it from `UserManager` and
    `ChessConnector.InGame`; before it builds the game page it shows "Starting the game…" and waits until that is drawn: `ChessConnector.waitForStartFrame`, BL-16), with `ProfilePage` or `SettingsPage` pushed over Home or the game, `OnlinePage` and `ContinueGamesPage` over Home; the
    InfoBars (under the command bar; on the game page's player column between its cards, `GamePage.barsArea`, BL-13; also the BL-10 challenge bar over every page: Accept / Open on lichess.org, Decline, Hide; a sound and the taskbar flash for a new one) and every dialog. Pages ask for dialogs through signals.
  - `HomePage.qml` (account menu, Continue: the only unfinished game directly, two or more: `ContinueGamesPage`, BL-17 (`ChessGame.continueGames()`); action cards, among them **Games** (Ctrl+O): `GamesPage`), `ContinueGamesPage.qml` (every unfinished game: the kept local games first as `GameCard`s (`GameLibrary.unfinishedGames`, BL-19), then the Lichess ones as `OngoingGameCard`s; BL-17), `GamesPage.qml` (the games library, objectName `library`: every kept game as a `GameCard`, Open (a replay) or Continue, Export PGN, Delete; the filters All · vs Robot · Two players · Unfinished; `games.js`: the words for a kept game, shared with Home and the Continue list), `OnlinePage.qml` (pushed by "Play online": the challenges to the player with Accept and Decline… with Lichess's reasons (BL-10), a new challenge (a link, or to a named Lichess player, the only way to play Blitz), join by link, and last the unfinished Lichess games as `OngoingGameCard`s with Continue and `OngoingGamesRefresh`; LI-04; `lichess.js`: Lichess's words for a game, shared with the Continue list), `GamePage.qml` (command bar, board between
    two `PlayerCard`s with a `CapturedStrip` and "+N" (BL-13: in a wide window where that gives a bigger board, the cards go `stacked` in a column left of the board, `area.side`), the moves (`MovesPanel` + `MoveList`) beside
    the board or in a bottom sheet, `ResultCard` over the board)
  - `ChessBoard.qml` — the board: one geometry for all layers (`squareX/Y`, `squareAt`, a1 bottom left; turned for Black by `rotation`), the layers made behind a `LayoutGate` (BL-11), the click `TapHandler`, the drag state (`dragFrom`, `dragPoint`, `dropSquare`) and the dragged piece under the pointer, coordinates (only the shown set; the edge labels' size fitted once), promotion dialog
  - `ChessFigures.qml` — the pieces, one item per row of `ChessConnector.Pieces` on its square; it slides when the square changes, a captured one fades out; a `DragHandler` per piece of the side to move, or of the player during the opponent's turn (a premove) (drop → `ChessConnector.movePiece`)
  - `ChessBoxes.qml` — the squares and their highlights (last move, premove, check, flagged king, selected piece, drop square: one item each, moved to its square); `NavigationLayer.qml` — the legal-move dots (premove targets in red)
  - Shared: `Theme.qml` and `AppSettings.qml` (singletons; `AppSettings` is the QtCore `Settings` with the
    app's own options), `SettingRow`, `AppDialog`, `ConfirmDialog`, `CommandBar` (`messageArea`: the item over which short messages show), `CommandBarMessage` (a message over the command bar, taking no room: "Check", "Game saved", an offer with Accept / Decline, the lost connection, the opponent left (Claim victory: `ChessConnector.CanClaimVictory`; closable); a short message takes the state's place for its time (an error until closed), but an offer or Claim victory is hidden at most 4 s ("Check" not at all) and takes the place back at once when it comes; text without buttons is cut short to fit (all of it on hover); only what doesn't fit falls back to the InfoBar; `ChessGame.notify()` for every short message, BL-13 step 3), `MessagesFlyout` (the recent messages: the last 20 of the session, an open offer still answerable; "Recent messages" in the game's More menu and Home's account menu, Ctrl+M, a dot for new ones: `NewMessagesDot`, `RecentMessagesMenuItem`; cleared when the profile changes), `InfoBar`, `IconButton`, `HomeCard`, `GameCard` (a game: a `BoardPreview`, who, whose turn or the result, Continue / Open, an optional Delete; `OngoingGameCard`: one of `LichessClient.ongoingGames`), `BoardPreview`, `ChoiceRow`, `SectionCard`, `AvatarPicker`, `TimeControlPicker` (the clock tiles of the setup dialogs and the challenge card), `FocusRing` (keyboard focus outline for custom-drawn items), `BusyButton` (a button whose action waits for Lichess: `busy`, a ring in the icon's place and `busyText`; set `idleText` / `idleIcon`; BL-16) and `BusyRing` (spinning, or a still clock with Reduce animations)
  - Tooling: `ScreenshotScene` (the `--screenshot` scenes; loaded only in that mode, keep debug/test helpers there, not in the pages)
  - Setup dialogs: `ComputerGameDialog`, `TwoPlayerDialog`, `ChallengeDialog` (a Lichess challenge waiting for the friend or a named player), `LeaveOnlineDialog` (Leave / Resign / Stay when leaving or closing during a running Lichess game); profiles: `CreateUserDialog`, `ImportConflictDialog`, `RestoreBackupDialog`
- `UserManager` keeps `users.json` (atomic `QSaveFile` writes, `backups/users-1..3.json`), statistics
  per opponent, profile export/import (`.chessprofile`, with the profile's `games.json`, never the Lichess token) and the Lichess
  sign-in (`LichessAuth`), so the token never reaches QML. Its tests are in `ChessQtTests/UserManagerTest.cpp`.
- Image assets live in `ChessGui/ChessGui/pics/`
- Resources bundled via `qml.qrc`

### ChessTests (test DLL)

Builds as a DLL consumed by the VS test runner. Tests exercise `Game`, `Board`, `MoveGeneration` and SAN/FEN directly.
It links only `ChessCore` and does **not** use Qt; tests that need Qt (UCI engine, Lichess, users) go in `ChessQtTests`.

## Key Conventions

- Smart pointers (`std::shared_ptr`) throughout; avoid raw owning pointers
- Chess logic stays in the `Chess::` namespace; GUI code stays in ChessGui
- QML ↔ C++ boundary:
  1. Anything that reads or changes the game — including flows that combine the game with users or Lichess (start a local or online game, game end, rematch, switch user during a game) — goes through `ChessConnector`, as **one call per user action**. QML never strings game steps together itself, and never sees `Chess::` types (`Game`, `Board`, engine).
  2. `UserManager` and `LichessClient` stay separate services. QML may call them directly for non-game work: profiles, login, Lichess account and challenge setup, ratings. Don't forward their API through `ChessConnector`.
  3. Expose QML-visible services as typed singletons (`qmlRegisterSingletonInstance`), not context properties: `ChessConnector`, `UserManager`, `LichessClient`, `AvatarProvider`, `AppInfo` and `GameLibrary` in module `ChessPlusPlus` (`AppServices`: made, wired and registered there, for `main.cpp` and the UI tests alike). A QML file that uses them has `import ChessPlusPlus`.
- `PieceColor` is an `enum class` (Light/Dark); use scoped references (`PieceColor::Light`)

## Resuming Work After a Session Break

At the start of a new session, read these two files to get full context:

1. `PROGRESS.md` — latest status, what's broken, and numbered next steps
2. `TASKS.md` — full task checklist; find first unchecked item under "In Progress"

Then confirm the current branch with `git branch` and `git log --oneline -5` before making any changes.

## Branching Workflow

- Start every task on a **new branch from the latest `origin/develop`** (`git fetch` first). Name it `features/<camelCaseTask>_<DDMMYY>` for a feature or task, `bugs/<camelCaseTask>_<DDMMYY>` for a bug fix, `docs/<camelCaseTask>_<DDMMYY>` for a change to docs only (a plan, `TASKS.md`, the README), e.g. `features/removeVcpkg_281026`, `bugs/infoBarsCover_101126`, `docs/bl13Plan_091126`.
  - `DDMMYY` is the day the branch is created: two digits each for day, month and year (10 Nov 2026 → `101126`). Take it from the local clock (`Get-Date -Format ddMMyy`), not from memory: a session can run past midnight.
  - Branches before 2026-11-10 have no `features/` / `bugs/` prefix (e.g. `infoBarsColumn_091126`, PR #137); don't rename them.
- Never continue a new task on the branch of a previous task.
- When a feature/task is done (built, tested, committed), push the branch and open a GitHub PR into `develop` with the `gh` CLI (`gh pr create --base develop`) — no need to ask first. Report the PR link.
- Exception: if the task depends on changes that are not merged into `develop` yet, stop and ask the user to either merge them first (then wait and branch from the updated `develop`) or explicitly approve branching from the unmerged branch.

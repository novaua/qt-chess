# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Qt-chess is a chess application with a Qt QML frontend and C++ backend, supporting local two-player, UCI engine (e.g., Stockfish), and network multiplayer modes. Games are standard chess or Chess960 (LI-01A): local ones, and on Lichess (stage 2: a Lichess Chess960 game starts from its `initialFen`, other variants are refused; the Online page's challenge offers it).

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

- `ChessTests/` — chess logic (`BasicTest.cpp`, `CheckTests.cpp`, `FenGenTests.cpp`, `SanTest.cpp`, `UndoTests.cpp`, `BoardDiffTests.cpp`, `MatingMaterialTests.cpp`, `PremoveTests.cpp`, `FakeEngineTests.cpp`, `Chess960Tests.cpp` (LI-01A: the start positions, castling from any set-up, and 60 random games checked ply by ply against python-chess: `Chess960Reference.h`, made by `python scripts\make_chess960_reference.py`)). Plain C++, no Qt.
- `ChessQtTests/` — the Qt parts: `UciConnectorTest.cpp` (UCI protocol), `ChessEnginePlayerTest.cpp`, `LichessClientTest.cpp`, `BoardPiecesTest.cpp`,
  `MoveListModelTest.cpp`, `UserManagerTest.cpp`, `GameClockTest.cpp`, `ClaimWinTest.cpp`, `GameLibraryTest.cpp` (the games library, BL-19; its signed files, BL-20), `GameSignatureTest.cpp` (BL-20: the signature, key 1 pinned, a game file read back), `GamePgnTest.cpp` (a kept game's PGN), `GameAnalysisTest.cpp` (the engine's analysis of a kept game, BL-19 step 6; FakeUci, built with the tests, and one test on Stockfish). Needs Qt's `bin` on `PATH`; the engine tests need Stockfish.
  `LichessClientTest.cpp`'s `LichessReconnectTests`, `LichessStreamTest.cpp`, `LichessChallengeTest.cpp` and `LichessSeekTest.cpp` (LI-09's quick pairing seek, `FakeSeekLichess`) play against `FakeLichess` (`FakeLichess.h`), a `QTcpServer` on 127.0.0.1 (no Lichess).
- `ChessGuiTests/` — the UI tests (BL-05, `docs/UiTestSuitePlan.md`): **Qt Quick Test**, a console exe, not in Test Explorer; GitHub runs them on every push (`tests.yml`, Debug; a failure uploads `test-results\ui` as `ui-test-results`). `qml/tst_*.qml` (first run, two players, the board, promotion, the robot, profiles, a restart, Lichess, the games library, the analysis, Chess960, Chess960 on Lichess, no engine) play the app's own window (`ChessGame.qml`) with real mouse events, offscreen (also with the screen locked); `qml/AppDriver.qml` has the helpers (`clickMove`, `dragPiece`, `pieceAt`, `waitPiece`, `waitBoardIdle`, `waitMessage`, `startTwoPlayerGame`, `leaveToHome`, `scrollIntoView`, …), items are found by `objectName`. One test file per process on a fresh profile (`TestSetup`, the app's `AppServices`; a file with `// ui-test-runs: 2` runs twice on one data folder, `TestRun.run`: a restart; `// ui-test-engine: none` runs it without an engine, `tst_noEngine`); the robot is FakeUci (`app.startComputerGame(line, thinkMs)`), no Stockfish needed; Lichess is `FakeLichess` (`fakelichessserver.h`, a local server the test plays the other side on; a challenge or ongoing game can be another variant, with a start FEN; `TestRun.linkLichess`), never lichess.org; `TestRun.addFinishedGames(n)` fills the games library without playing. It builds every ChessGui `.cpp` except `main.cpp`: **a new ChessGui source goes into `ChessGuiTests.vcxproj` too** (the link fails without it). Run:

```
scripts\Run-UiTests.ps1 -Build              # every test file (Release, the default: without -Build an old Release exe runs; -Configuration Debug for the Debug build); -Name tst_twoPlayers -Function test_2_newGameAndDrag for one
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
- `MoveGeneration` — generates pseudo-legal and legal moves including castling and en passant; `Play` plays a validated move on the board (castling with its rook, en passant), `HasCastlingRight`, `CastlingPath` (a side's castling squares when nothing stands in the way: the legal moves and the premove targets share it), `IsCastling`
- `StartPosition` (`StartPosition.h`, LI-01A) — the set-up a game starts from: standard, or Chess960 #0–959 (Scharnagl, as lichess.org numbers them; #518 is the standard one), `Random()`, `FromFen` (a start position only), `Fen()`. `Game::Restart(whiteFirst, start)` (standard when none is given), `GetStartPosition()`; `LoadMoves`, `EndGame` and `FormatHistorySan(history, start)` keep it. Castling for any set-up: in a Chess960 game it is written as the king onto its own rook (`f1g1`, `b1a1`; Stockfish's `UCI_Chess960` and Lichess), a standard game keeps `e1g1`; `Board::DoCastling` moves both pieces in one step (they may swap, or one stay), `HistoryMove::Castling()` gives the squares, a castling is never a capture; FEN castling is X-FEN (`KQkq`, a file letter only when ambiguous)
- `ChessEnginePlayer` — wraps `UciConnector` to produce computer moves
- `Serializer` — the binary save format of versions before BL-19 (`Game::Save` / `Load`), read only to migrate the two old save slots; `Game::LoadMoves` replays a UCI move list into the loaded history (a kept game's replay)
- `BoardDiff` (`BoardDiff.h`) — which piece went where between two positions (moved, removed, added), for the board's piece animations

### UciConnector (static library)

Spawns and communicates with external UCI engines via `QProcess`. Main API: `Init()`, `NewGame()`, `GetEngineMove()`; `SetChess960(on)` (LI-01A: `UCI_Chess960`, sent only when it changes something). A Chess960 game's requests are always its start FEN and every move, whatever the `PositionMode` (`ChessEnginePlayer::MakeRequest(history, plies, mode, start)`; `ChessEnginePlayer`, `EngineWorker` and the analysis get the game's `StartPosition`).
The engine is `./engines/stockfish.exe`, else one on `PATH`; `CHESS_ENGINE_PATH` names the only one instead (the tests' FakeUci).

### FakeUci (console exe, tests only)

A stand-in UCI engine on `ChessCore`, no Qt (`FakeUci/fakeengine.h`, BL-05 step 3): answers what the app asks of Stockfish
at once and the same way every time: the move of the game in `FAKEUCI_MOVES` at that ply when legal there (a promotion without its piece: a queen), else the first legal
move by its UCI text (said on stderr); its one line's score is `mate 1` when the move mates, else the material after it (the analysis tests); Chess960 as Stockfish: `setoption name UCI_Chess960 value true`, then `position fen` of a Chess960 start and its moves; `FAKEUCI_DELAY_MS` before each answer; `moves` (not UCI) lists its legal moves. The UI tests
(`TestSetup`) and the robot screenshot scenes (`Take-Screenshot.ps1`) run it; its tests are `ChessTests/FakeEngineTests.cpp`.

### ChessGui (Qt QML application)

- **`chessconnector.h/cpp`** — the C++/QML bridge for the chess game; exposes `Q_PROPERTY` bindings and slots that QML calls. Premove (LI-05, vs the robot and on Lichess, not two players): during the opponent's turn `figureSelected` / `movePiece` set `PremoveFrom`/`PremoveTo` instead of moving (`PremoveDots`: the dots are premove targets; `CaptureSquares`: which of them take a piece, BL-21); it is played right after the opponent's move if legal then, else dropped silently; `cancelPremove()` (right click, Esc). `acceptChallenge(id)` (BL-10): accepts a Lichess challenge; the game on the board stays until the new one starts (a local game is then kept in the library), so a refused Accept loses nothing; not during a running real-time Lichess game (`CanAcceptChallenge`). BL-17: `continueOnlyGame()` (Home's Continue: the only unfinished game, else false and the list opens; on the last Lichess list while it refreshes, the list only before the first one came: `LichessClient::ongoingGamesLoaded`, BL-24), `continueOnlineGame(id)` (rejoin a Lichess game: `StartingOnlineGame` and `StartingGameId` until it starts, `gameStartFailed`; `AcceptingChallengeId`: the challenge `acceptChallenge` is accepting, one state for the bar and the Online page, BL-16; dropped by `cancelOnlineStart()` or another profile). `rematch()`: a Lichess rematch always asks for the other colour; a robot or two-player one swaps the player's side when `RematchSwapsColours` (Settings → Gameplay, `AppSettings.rematchSwapsColours`, on by default; #193), for that game only (`applyPlayerSide`, not saved): the setup dialogs offer the side last chosen in them (`AppSettings.computerSide`, `twoPlayerSide`). BL-19: `continueGame(id)` / `openGame(id)` for a kept game (see `gamelibrary`); leaving a local game never asks, it is kept as it is. Play from here (BL-19 step 4): `PlayFromHere` (only while reviewing: the position shown of a replay, or of a local game stepped back once it is over (never a game going on): `{ gameId, ply, move, whiteToMove }`, `{}` when none or not playable; step 4b) and `startComputerGameFrom` / `startLocalGameFrom(gameId, ply, …)`: a new game with that game's first moves (a fork: `forkOfId`, `forkPly`; those moves aren't taken back and the clock doesn't count them); the review bar's **Play from here** button (`MovesPanel`, the sheet handle in a narrow window) → its menu (vs computer… / Two players…) → the setup dialogs' `openFrom(position)`
- **`boardpieces.h/cpp`** — `BoardPieces`, the pieces model behind `ChessConnector.Pieces`: one row per piece that keeps its id while it moves (`BoardDiff`), so the board can slide it
- **`movelistmodel.h/cpp`** — `MoveListModel`, the move list behind `ChessConnector.MoveHistory`: one row per move number, and a move changes only its row instead of rebuilding the list
- **`movegate.h/cpp`** — `MoveGate`, which holds the moves from outside (the robot's answer, the Lichess opponent's) and releases them one at a time when the board is idle (a Lichess resync of more than two: all at once; in a real-time Lichess game the opponent's moves don't wait for the board, only for the page: `setHoldOnline`, LI-06); `ChessConnector` owns one, `GamePage` reports the board settled and the slide time
- **`gameclock.h/cpp`** — `GameClock` (`ChessConnector.Clock`), the two clocks of a game, as on lichess.org: `ChessConnector` drives it with `moveMade()` in a local game, Lichess's times come with `setSnapshot()` and never flag here, and a correspondence game shows the time left for the move (LI-02). `Board::CanMate` decides whether a flag loses or draws
- **`gamelibrary.h/cpp`** — `GameRecord` and `GameLibrary` (BL-19, QML singleton `GameLibrary`): every local game (vs the robot, two players) kept from its first move, finished or left, newest first; BL-20: one signed file per game, `users/<id>/games/<gameId>.json` (`{"record", "sig"}`: `GameSignature`, HMAC-SHA256 with one app-wide key and a key number, `gamesignature.h/cpp`), the rating in `users/<id>/rating.json`; a game file that fails its check (edited, damaged, another game's id) is moved to `games/rejected/`, never loaded (`setAsideCount`, `gamesSetAside`: "N kept games couldn't be verified and were set aside."); the `games.json` of BL-19 is migrated once (trusted, signed; kept as `games.json.migrated`), never beside files of BL-20 (`hasSignedFiles`: a `rating.json`, a game file, `games.json.migrated`, `games/rejected/`; it is set aside then). The rating vs the robot (step 5): `LocalRating` (Elo from 1200, provisional under 10 games) stored as it goes in `rating.json` (BL-20 step 2: signed; one that fails its check, or is missing while rated games are kept, is set aside in `games/rejected/` and restored from the newest rated game: `ratingBefore + ratingChange` and its `ratedGames`, else 1200: `ratingRestored`, `ratingRestoredOnLoad`, "Your rating couldn't be verified and was restored…"; one behind the rated games that continue it exactly (`continued`: `ratingBefore` the rating so far, one more rated game each) catches up silently; a rated game copied from another profile doesn't move it): `put` rates a finished robot game without takeback or fork once (`rated`, `ratingBefore`, `ratingChange` on the record), gives the change back when a takeback resumes it; removing games never changes it (`rating`, `ratedGames`, `ratingProvisional`; Home's header, the Profile page, the cards, the PGN's player Elo) (`games`, `unfinishedGames`, `count`, `unfinishedCount`, `finishedCount`, `remove(id)`); the last `keepCount` finished games are kept (Settings → Data "Games kept", `AppSettings.gamesKept`, default 100; a smaller number asks first: `ChessGame.setGamesKept`), unfinished ones never removed on their own; follows the active profile; the old `chess.autosave` / `chess.save` of a profile are migrated once (one that can't be read is kept as `.damaged`; a Lichess autosave isn't migrated). `put` / `remove` write only that game's file once the current event is done (`flush()` writes now; the destructor and a profile change do); a BL-19 `games.json` that is there but can't be opened is never written over (migrated at a later write, its games merged; the rating waits for it). "Games kept" is every profile's: `finishedBeyond(keep)` counts them all for the confirm, `setKeepCount` trims every profile's file. `GameRecord::play` / `playOn` play kept moves (false at one that can't be played, a move from an empty square too); `continueGame` / `openGame` refuse such a record (`keptGameFailed`). `ChessConnector` writes the record with every move, takeback, result and when the game is left (`updateRecord`, `keepGameOnLeave`), `continueGame(id)` resumes an unfinished one (the robot at its level, or two players; the clock as left) and `openGame(id)` replays any (mode `"replay"`). Lichess games have no record (the owner's choice). LI-01A: a Chess960 game's record has `startFen` (`variant: "chess960"` in the JSON; `chess960()`, `startPosition()`, `replay(plies)` plays its moves from it; QML: `chess960`, `startPosition` number), is never rated; its PGN has `Variant`, `SetUp`, `FEN`. `ChessConnector`: `startComputerGame` / `startLocalGame(…, chess960)` (`StandardGame`, `RandomChess960` or a number), `Chess960`, `StartPositionNumber` (`GameTitle` ends "· Chess960 #N", the cards' details begin with it); rematch, continue, replay and Play from here keep the start; castling input as lichess.org (`moveTarget`: the king onto its rook, or onto its castling square when no move of its own goes there; also a premove); the review and replay positions are replayed from the start (`shownGame`)
- **`gameanalysis.h/cpp`**, **`gameanalyzer.h/cpp`** — BL-19 step 6: `GameAnalysis`, the engine's analysis of a kept game (per position the eval from White's view and the best move; stored on the record, `GameRecord::analysis`; win chances, judgements at 5 / 10 / 15 points and accuracy as lichess.org, worked out when read); `GameAnalyzer` runs it on a thread with a full-strength engine of its own (`AnalysisWorker`), kept only if the record's moves are still those analysed (`GameLibrary::setAnalysis`; `put` drops a stale one). `ChessConnector.analyseGame(id)` / `cancelAnalysis()`, `Analysing`, `AnalysisDone` / `AnalysisTotal`, `AnalysisDepth` (Settings → Computer "Analysis depth"). Step 7, the board's game: `CanAnalyse` (a kept game that is over, or a replay: never one going on), `AnalysingThisGame`, `Analysis` (per side accuracy and `?!` `?` `??` counts), `ShownAnalysis` (the shown position's eval, the move's mark and the engine's best); `MoveListModel`'s `wMark` / `bMark`; QML: `AnalysisSummary` (the moves panel; "Best was …" in green), `EvalBar`, `MoveArrows` (on a judged move the board shows the move played in its mark's colour and the engine's best in green: `ShownAnalysis.playedFrom/To`, `bestFrom/To`), Analyse on the result card and in More
- **`gamepgn.h/cpp`** — `GamePgn` (BL-19 step 3): a game as PGN, the seven standard tags plus the robot's Elo and `TimeControl`; `ChessConnector::pgn(gameId)` / `pgnFileName(gameId)` / `exportPgn(file, gameId)` write the game on the board (`""`) or a kept game (a Games card's Export PGN) with it
- **`claimwin.h/cpp`** — `ClaimWin`, the Lichess opponent left: Lichess's countdown to the claim and "only on the opponent's turn" (Lichess refuses it on the player's), behind `ChessConnector.OpponentGone`, `ClaimWinSeconds` and `CanClaimVictory`; `claimVictory()` posts only then
- **`frametrace.h/cpp`** — `FrameTrace`, frame timing in the app log, off unless `QT_LOGGING_RULES="chess.frames.debug=true"` (BL-14); `GameStartTiming`, always on (`chess.timing`): a `[Timing]` line from the action that starts a game to its first frame, with its steps (`ChessConnector` marks them, `GamePage` calls `gameShown`), and the engine stop's wait (BL-11)
- **`layoutgate.h/cpp`** — `LayoutGate` (QML type in `ChessPlusPlus`): `open` from the first frame's polish pass, once a page's layout has settled; costly items behind it (`Loader { active: gate.open }`) are made once at their real size and are still in that frame (BL-11: the board's layers, the moves panel)
- **`sceneinput.h/cpp`** — `SceneInput`, screenshot mode only (QML singleton): mouse clicks and drags sent to the window, for scenes that play as a user does (`play`, BL-11)
- **`appservices.h/cpp`** — `AppServices`: the five QML services, how they work together (the Lichess token, account, ongoing games) and their QML registration; `main.cpp` and the UI tests' `TestSetup` both use it (BL-05)
- **`UserManager`**, **`LichessClient`** (LiChessConnector project; LI-01A stage 2: `playableVariant` (standard, chess960: `canPlayHere`), `gameStarted` ends with the game's `variant` and start FEN (`""` for `startpos`; `ChessConnector::onLichessGameStarted` starts from it or refuses the game with `gameStartFailed`), `variantName` (Lichess's names, QML too), `challengePlayer(…, variant)`, a Chess960 rematch sends the game's `fen` (the same position), the `chess960` rating pool; `ongoingGames`: the unfinished Lichess games, LI-04; a game stream lost mid-game (closed, or 20 s without even a keep-alive line) reconnects on its own (`LichessStream`: one ndjson GET with the silence watchdog, the 1 → 15 s backoff, 4xx stops it; BL-10 step 1) and its `gameFull` is a resync, as is a move POST without an answer: `gameConnectionChanged`, `ChessConnector.Reconnecting`, LI-06; `setBaseUrl` / `setStreamTiming` for the fake-server tests; the account's event stream, open while there is a token (`startEventStream`, from `AppServices`; one per token, shared by the challenge wait and the rematch watch), gives `incomingChallenges` / `challengeReceived` / `declineChallenge` / `challengeAcceptFailed`; `rejoinGame` has its own `rejoiningGameId` / `rejoinFailed`, not `networkError`, BL-17; `gameStarted` carries the game's moves so far, which `ChessConnector::playJoinedMoves` puts on the board before the game page opens, BL-18 (`rematchOfferId`: the one the rematch bar shows) and refreshes `ongoingGames` on games started or ended elsewhere, BL-10), **`AvatarProvider`**, **`AppInfo`** (version, data/log folders, the engine's name; the engine looked for at start, `AppServices::start()`, and again while it is missing (Settings, each game page's moves panel: a first start slower than the probe's 5 s isn't final); `engineFound` only once one answered: the Analyse buttons need it, BL-25; `copyToClipboard(text, message)`: QML's clipboard, `copied(message)` becomes the window's short message, LI-10) — app services without chess logic, also visible to QML (see Key Conventions)
- QML files in `ChessGui/ChessGui/qml/`:
  - `ChessGame.qml` — the window: a `StackView` with one page (`WelcomePage` on first run,
    `ProfilePickerPage`, `HomePage`, `GamePage`; `route()` picks it from `UserManager` and
    `ChessConnector.InGame`; before it builds the game page it shows "Starting the game…" and waits until that is drawn: `ChessConnector.waitForStartFrame`, BL-16), with `ProfilePage` or `SettingsPage` pushed over Home or the game, `OnlinePage` and `ContinueGamesPage` over Home; the
    InfoBars (under the command bar; on the game page's player column between its cards, `GamePage.barsArea`, BL-13; also the BL-10 challenge bar over every page: Accept / Open on lichess.org, Decline, Hide; a sound and the taskbar flash for a new one) and every dialog. Pages ask for dialogs through signals.
  - `HomePage.qml` (account menu, Continue: the only unfinished game directly, two or more: `ContinueGamesPage`, BL-17 (`ChessGame.continueGames()`); action cards, among them **Games** (Ctrl+O): `GamesPage`), `ContinueGamesPage.qml` (every unfinished game: the kept local games first as `GameCard`s (`GameLibrary.unfinishedGames`, BL-19), then the Lichess ones as `OngoingGameCard`s; BL-17), `GamesPage.qml` (the games library, objectName `library`: every kept game as a `GameCard`, Open (a replay) or Continue, Export PGN, Delete; the filters All · vs Robot · Two players · Unfinished; `games.js`: the words for a kept game, shared with Home and the Continue list), `OnlinePage.qml` (pushed by "Play online" (`onlineCard`): the challenges to the player with Accept and Decline… with Lichess's reasons (BL-10), a new challenge (a link, or to a named Lichess player, the only way to play Blitz; its Variant row, `AppSettings.onlineChess960`, apart from the local dialogs', and the Chess960 rating when chosen: LI-01A stage 2), join by link, and last the unfinished Lichess games as `OngoingGameCard`s with Continue and `OngoingGamesRefresh`; LI-04; `lichess.js`: Lichess's words for a game, shared with the Continue list), `GamePage.qml` (command bar, board between
    two `PlayerCard`s with a `CapturedStrip` and "+N" (BL-13: in a wide window where that gives a bigger board, the cards go `stacked` in a column left of the board, `area.side`), the moves (`MovesPanel` + `MoveList`) beside
    the board or in a bottom sheet, `ResultCard` over the board)
  - `ChessBoard.qml` — the board: one geometry for all layers (`squareX/Y`, `squareAt`, a1 bottom left; turned for Black by `rotation`), the layers made behind a `LayoutGate` (BL-11), the click `TapHandler`, the drag state (`dragFrom`, `dragPoint`, `dropSquare`) and the dragged piece under the pointer, coordinates (only the shown set; the edge labels' size fitted once), promotion dialog
  - `ChessFigures.qml` — the pieces, one item per row of `ChessConnector.Pieces` on its square; it slides when the square changes, a captured one fades out; a `DragHandler` per piece of the side to move, or of the player during the opponent's turn (a premove) (drop → `ChessConnector.movePiece`)
  - `MoveArrows.qml` — the analysis's two arrows on a judged move (played, best), over the pieces
  - `ChessBoxes.qml` — the squares and their highlights (last move, premove, check, flagged king, selected piece, drop square: one item each, moved to its square); `NavigationLayer.qml` — the legal-move dots (premove targets in red); BL-21: a target that takes a piece (`ChessConnector.CaptureSquares`: the other side's piece there, or en passant) shows its four corners in the dot's colour instead, as lichess.org (a premove's in the premove dot's), from a second instance under the pieces (`kind: "captures"`, objectName `captureLayer`; the corners a clipped Rectangle ring, made once per square; `ChessBoard` reads the targets once for both layers)
  - Shared: `Theme.qml` and `AppSettings.qml` (singletons; `AppSettings` is the QtCore `Settings` with the
    app's own options), `SettingRow`, `AppDialog`, `ConfirmDialog`, `CommandBar` (`messageArea`: the item over which short messages show), `CommandBarMessage` (a message over the command bar, taking no room: "Check", "Game saved", an offer with Accept / Decline, the lost connection, the opponent left (Claim victory: `ChessConnector.CanClaimVictory`; closable); a short message takes the state's place for its time (an error until closed), but an offer or Claim victory is hidden at most 4 s ("Check" not at all) and takes the place back at once when it comes; text without buttons is cut short to fit (all of it on hover); only what doesn't fit falls back to the InfoBar; `ChessGame.notify()` for every short message, BL-13 step 3), `MessagesFlyout` (the recent messages: the last 20 of the session, an open offer still answerable; "Recent messages" in the game's More menu and Home's account menu, Ctrl+M, a dot for new ones: `NewMessagesDot`, `RecentMessagesMenuItem`; cleared when the profile changes), `InfoBar`, `IconButton`, `HomeCard`, `GameCard` (a game: a `BoardPreview`, who, whose turn or the result, Continue / Open, an optional Delete; `OngoingGameCard`: one of `LichessClient.ongoingGames`; a right click opens its menu: Continue (a game playable here), Open on lichess.org, Copy link (`AppInfo.copyToClipboard`, "Link copied"), LI-10), `BoardPreview`, `ChoiceRow`, `SectionCard`, `AvatarPicker`, `TimeControlPicker` (the clock tiles of the setup dialogs and the challenge card), `VariantPicker` (the setup dialogs' Variant row, Standard / Chess960, `AppSettings.chess960`; none in Play from here; LI-01A; `online`: the Online page's, `AppSettings.onlineChess960`), `FocusRing` (keyboard focus outline for custom-drawn items), `BusyButton` (a button whose action waits for Lichess: `busy`, a ring in the icon's place and `busyText`; set `idleText` / `idleIcon`; BL-16) and `BusyRing` (spinning, or a still clock with Reduce animations)
  - Tooling: `ScreenshotScene` (the `--screenshot` scenes; loaded only in that mode, keep debug/test helpers there, not in the pages)
  - Setup dialogs: `ComputerGameDialog`, `TwoPlayerDialog` (both with the Variant row, LI-01A), `ChallengeDialog` (a Lichess challenge waiting for the friend or a named player), `LeaveOnlineDialog` (Leave / Resign / Stay when leaving or closing during a running Lichess game); profiles: `CreateUserDialog`, `ImportConflictDialog`, `RestoreBackupDialog`
- `UserManager` keeps `users.json` (atomic `QSaveFile` writes, `backups/users-1..3.json`), statistics
  per opponent (BL-20 step 2: each profile's signed, `statsSig`, `users.json` version 3 (`kUsersVersion`; an older file is trusted once and written as version 3 at once, never when a profile already has signed files); ones that fail their check are restored from the newest backup whose statistics verify, else rebuilt from the kept games of every profile: `takeRestoredStatistics`, `statisticsRestored`), profile export/import (`.chessprofile`, export format 2: with the profile's signed game files (`games/<id>.json`, the only names written under `games/` on import) and `rating.json`; never the Lichess token. BL-20 step 3: an import is checked before it is written (`GameLibrary::checkImport`): a game file that fails its check goes to `games/rejected/`, a failing `rating.json` too (the rating restored from the newest verified rated game), an export's `games.json` (from before BL-20) comes in as signed files without their rating fields and no rating (unrated), statistics that don't verify are rebuilt from the imported games; `lastImport()` says what was found, in the import's message) and the Lichess
  sign-in (`LichessAuth`), so the token never reaches QML (`lichessToken()`, C++ only: a token this Windows account can't decrypt, e.g. a data folder copied from another PC, marks the link expired: "reconnect"; written and `lichessChanged` once the call that asked is done). The app's files are written with `AppConfig::writeFileAtomically` (QSaveFile). Its tests are in `ChessQtTests/UserManagerTest.cpp`.
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
- `-platform offscreen` (the UI tests, `-Offscreen` screenshots) renders with Qt Quick's software renderer, which has no shader effects: an item with `layer.effect` (`MultiEffect`) isn't drawn at all there. Enable such a layer only with `GraphicsInfo.api !== GraphicsInfo.Software`, so the item is drawn plain (the move list's and the captured strip's piece glow, BL-27)

## Tasks and Bugs (`.tasks/`)

Every task and bug is one Markdown file, `<ID>-<short-kebab-title>.md` (e.g. `LI-10-copy-a-games-link-from-its.md`):

- `.tasks/open/` — the queue: open and in-progress tickets.
- `.tasks/archive/` — done tickets, each with its history under `## Resolution`. `_unmatched-progress.md` holds old log entries that match no ticket.
- IDs: `BL-` the app (bugs included: BL-23 and BL-24 were bugs), `LI-` playing on Lichess, `CE-` the chess rules in `ChessCore`; a variant of a ticket gets a letter (`LI-01A`, `LI-01B`). A new ticket takes the next free number of its prefix, across both folders (`ls .tasks/open .tasks/archive`). `TSK-` / `BUG-` are only the old items that had no ID; don't make new ones.
- Frontmatter, then `# <ID> <title>` and the ticket's text:
  ```yaml
  ---
  id: LI-10
  title: "Copy a game's link from its card"
  type: task        # task | bug
  status: open      # open | in_progress | done
  priority: medium  # high | medium | low
  ---
  ```
- A ticket's plan and hand tests stay in `docs/` (`docs/<ID>-…Plan.md`, `docs/<ID>-ManualTests.md`), linked from the ticket.

Saving tokens:

- **Never** read all of `.tasks/open/`, and **never** read `.tasks/archive/` unless asked to look into a past ticket.
- To find work: `ls .tasks/open`, or Grep `.tasks/open` for `status: in_progress` / `priority: high` (file names only), then read **only** the ticket being worked on.
- A bug or follow-up found on the way: create its file in `.tasks/open/` at once (with what was found: cause if known, where, how to see it) instead of writing it into `PROGRESS.md` or another ticket.

Working a ticket:

- Starting it: `status: in_progress`. A ticket done in steps (one PR each, like BL-19 or LI-01A) stays in `open/`; each step's PR appends a short `### <YYYY-MM-DD> — <step> (branch: <branch>)` entry to its `## Resolution`: done, checked, what is left.
- Closing it (built, tested, and the owner's hand tests passed where the ticket has them):
  1. Append a short `## Resolution` entry to the ticket: the cause (a bug), the fix, the tests run and their result.
  2. Set `status: done` in its frontmatter.
  3. `git mv .tasks/open/<file>.md .tasks/archive/<file>.md`, in the same PR.

## Session Handover (`PROGRESS.md`)

At the start of a session read `PROGRESS.md`, then confirm the branch with `git branch` and `git log --oneline -5` before changing anything.

`PROGRESS.md` is a handover note, **under 15 lines**, not a diary: update it in every PR by overwriting it (the history is in the tickets' `## Resolution`; on a merge conflict keep the newer note). Only:

- **Last completed:** ticket ID, branch, one line on what was done.
- **Current focus:** the active ticket ID and where it stands, or `None`.
- **Next up:** the ticket from `.tasks/open/` to take next.
- **Active gotchas:** blockers and passing environment notes only. Lasting rules go into this file (or the `run-chess-app` skill for screenshots), not there.

## Branching Workflow

- Start every task on a **new branch from the latest `origin/develop`** (`git fetch` first). Name it `features/<camelCaseTask>_<DDMMYY>` for a feature or task, `bugs/<camelCaseTask>_<DDMMYY>` for a bug fix, `docs/<camelCaseTask>_<DDMMYY>` for a change to docs only (a plan, a ticket in `.tasks/`, the README), e.g. `features/removeVcpkg_281026`, `bugs/infoBarsCover_101126`, `docs/bl13Plan_091126`.
  - `DDMMYY` is the day the branch is created: two digits each for day, month and year (10 Nov 2026 → `101126`). Take it from the local clock (`Get-Date -Format ddMMyy`), not from memory: a session can run past midnight.
  - Branches before 2026-11-10 have no `features/` / `bugs/` prefix (e.g. `infoBarsColumn_091126`, PR #137); don't rename them.
- Never continue a new task on the branch of a previous task.
- When a feature/task is done (built, tested, committed), push the branch and open a GitHub PR into `develop` with the `gh` CLI (`gh pr create --base develop`) — no need to ask first. Report the PR link.
- Exception: if the task depends on changes that are not merged into `develop` yet, stop and ask the user to either merge them first (then wait and branch from the updated `develop`) or explicitly approve branching from the unmerged branch.

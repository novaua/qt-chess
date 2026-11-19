---
id: LI-01A
title: "Chess960"
type: task
status: open
priority: medium
---

# LI-01A Chess960

<!-- migrated from TASKS.md:235 (Lichess Improvement Backlog) -->

- [ ] LI-01A Chess960 (Fischer Random) [Hard] — plan: [`docs/LI-01A-Chess960Plan.md`](docs/LI-01A-Chess960Plan.md) (the owner split LI-01 on 2026-11-17: A Chess960, B Crazyhouse; rules: <https://lichess.org/variant/chess960>). **Stage 1 done (steps 1–5, #175–#179; the owner's hand tests [`LI-01A-ManualTests.md`](docs/LI-01A-ManualTests.md) all passed on 2026-11-18); stage 2 (Lichess) coded (#182, #183; plan [`LI-01A-Chess960LichessPlan.md`](docs/LI-01A-Chess960LichessPlan.md)); its hand tests are the owner's ([`LI-01A-LichessManualTests.md`](docs/LI-01A-LichessManualTests.md)).**
  - [x] **Stage 1, local**: vs the robot and two players, kept in the games library. The owner's choices: castling as on Lichess (the king onto its rook, or onto its destination when that isn't an ordinary king move); 960 games aren't rated; a random position "Chess960 #N", Rematch keeps it, New game draws a new one. Steps (plan §4):
    - [x] 1 ChessCore rules: `StartPosition`, `Board::DoCastling`, castling for any set-up (king onto own rook in 960, `e1g1` kept in standard), SAN/FEN (branch `features/chess960Rules_171126`; checked against python-chess)
    - [x] 2 The engine: `UCI_Chess960`, the start FEN in every request, the analysis, FakeUci (branch `features/chess960Engine_171126`)
    - [x] 3 The games library and `ChessConnector`: `variant` / `startFen` on the record, PGN tags, start / rematch / continue / replay / fork, the castling input (branch `features/chess960Library_171126`; `tst_chess960`)
    - [x] 4 The UI: the setup dialogs' Variant row (`VariantPicker`, remembered; New game offers the game's own), "Chess960 #N" in the game's title and on the cards, scenes `chess960`, `chess960-select`, `chess960-castled`, `computer-960`, `tst_chess960` +3 (branch `features/chess960Ui_171126`)
    - [x] 5 Docs and hand tests: [`docs/LI-01A-ManualTests.md`](docs/LI-01A-ManualTests.md) (8 scenarios), CLAUDE.md, the run-chess-app skill's scenes (branch `docs/chess960ManualTests_181126`)
    - [x] The owner runs `LI-01A-ManualTests.md` (Stockfish at levels 1 and 5, castlings, takeback, Continue, not rated, replay, Analyse, the PGN on lichess.org): all 8 passed on 2026-11-18
  - [ ] **Stage 2, on Lichess** (plan: [`docs/LI-01A-Chess960LichessPlan.md`](docs/LI-01A-Chess960LichessPlan.md); the owner's choice: a 960 Rematch keeps the position). Steps (plan §4):
    - [x] 1 C++: `playableVariant` (standard, chess960), `gameStarted` with the variant and the start FEN, `startOnlineGame` from it, other variants refused, `challengePlayer`'s variant, the rematch's `fen`, the `chess960` rating, `variantName`; ChessQtTests (branch `features/chess960LichessGames_181126`)
    - [x] 2 UI: the Online page's Variant row (`AppSettings.onlineChess960`, the Chess960 rating line), Lichess's variant names, the Profile's Chess960 pool, `FakeLichessServer` variants, scenes, `tst_lichess960` (branch `features/chess960LichessUi_181126`)
    - [x] 3 Docs and hand tests on lichess.org: [`docs/LI-01A-LichessManualTests.md`](docs/LI-01A-LichessManualTests.md) (9 scenarios, two Lichess accounts), `TASKS.md`, `PROGRESS.md` (branch `docs/chess960LichessManualTests_181126`)
    - [ ] The owner runs `LI-01A-LichessManualTests.md` (a 960 challenge both ways, castlings, premove, Continue, rematch's position, correspondence, PGN, the rating, Crazyhouse refused)

## Resolution

### 2026-11-17 — LI-01A Chess960 plan (branch: docs/li01aChess960Plan_171126)

The owner split LI-01: **LI-01A Chess960** (local first: vs the robot and two players; then Lichess as stage 2) and **LI-01B Crazyhouse** (later). Plan: `docs/LI-01A-Chess960Plan.md`. The owner's choices: castling as on Lichess (king onto its rook, or onto its destination when that isn't an ordinary king move); 960 games not rated; a random position "Chess960 #N", Rematch keeps it.

#### Next
- Stage 1 step 1: the rules in ChessCore (`StartPosition`, `Board::DoCastling`, castling for any set-up; 960 castling as king onto own rook, standard keeps `e1g1`), `ChessTests/Chess960Tests.cpp`.

### 2026-11-17 — LI-01A step 1: Chess960 rules in ChessCore (branch: features/chess960Rules_171126)

#### Done
- **`StartPosition`** (`ChessCore/StartPosition.h/cpp`): standard or Chess960 #0–959 (Scharnagl, as lichess.org; #518 standard), `Random()`, `FromFen` (start positions only), `Fen()`, king and castling-rook files.
- **Castling for any set-up**, one rule (plan §3.3): king and rook unmoved (no move from or to the rook's square), every square they pass or land on free but for themselves, the king's squares not attacked; the move's form is the game's (Chess960: the king onto its own rook, `f1g1`; standard keeps `e1g1`). Castling may come on the first move (#3: king f1, rook g1).
- **`Board::DoCastling`**: both pieces in one step (swap, or one stays); `MoveGeneration::Play` plays every move (castling, en passant) for `Game`, FakeUci and the check simulation, which now moves the rook too (a rook that shielded its king, #379). `Game::UndoMove` takes castling back in one step.
- `HistoryMove`: a castling onto its own rook isn't a capture; `Castling()` gives the squares. SAN by the rook that castled (and castling now gets `+` / `#`). FEN castling X-FEN. Premove targets: the rook's square in Chess960 and the king's square. `IsEverMoved` (unused) removed.
- `Game::Restart(whiteFirst, start)`, `GetStartPosition()`; `LoadMoves`, `ResumeFromLoad`, `EndGame` keep the start; `FormatHistorySan(history, start)`. Everything else still starts standard (steps 2–3 pass the start on).
- **Tests** `ChessTests/Chess960Tests.cpp` (19): numbering, all 960 valid and read back from their FEN, FromFen, the named cases (swap, king stays, O-O-O from b1 and its blocked rook square, an attacked square, a moved rook, the shielding rook, the form decided by the game, LoadMoves, Restart, premove targets, BoardDiff). **60 random games checked ply by ply against python-chess** (`scripts/make_chess960_reference.py` → `Chess960Reference.h`: legal-move counts, final FEN, castling SAN, then every takeback): 1920 plies, 88 castlings, all agree.

#### Checked
- Debug: ChessTests 163/163, ChessQtTests 264/264; UI tests 10 files, 0 failed. Release: ChessTests 163/163.

#### Next
- Step 2, the engine (plan §3.6): `UCI_Chess960`, the start FEN in every request, the analysis, FakeUci.

### 2026-11-17 — LI-01A step 2: the engine plays Chess960 (branch: features/chess960Engine_171126)

#### Done
- `UciConnector::SetChess960(on)`: `UCI_Chess960`, sent only when it changes something; an engine without the option can't play a Chess960 game (an error).
- `ChessEnginePlayer` / `EngineWorker` take the game's `StartPosition` (`ChessConnector::startEngineThread` passes `_game->GetStartPosition()`); a Chess960 one sets the option when the engine starts. `MakeRequest(history, plies, mode, start)`: a Chess960 game is always its start FEN and every move (no FEN window: the engine knows the castling rooks from the start; plan §3.6 updated). The debug dump names the position.
- The analysis (`AnalysisWorker`) sets the option and asks from the start; `GameAnalyzer::start` passes the replayed game's start (standard until step 3 gives the record one).
- FakeUci: lists `UCI_Chess960`, `setoption` turns it on, `position fen` of a Chess960 start (or `startpos` as #518) is then a Chess960 game; castling from the history.
- Tests: Stockfish's root moves equal ours in Chess960 #3 (castling as the king onto its rook), `SetChess960` sends nothing when off; `MakeRequest` (start FEN in every mode), the robot at levels 1 and 5 plays a legal move in a 960 game; FakeUci +2; the analysis worker tells FakeUci the start.

#### Checked
- Debug: ChessTests 165/165, ChessQtTests 269/269; UI tests 10 files, 0 failed.

#### Next
- Step 3 (plan §3.7): `GameRecord` `variant` / `startFen`, PGN tags, `ChessConnector` start / rematch / continue / replay / fork with the start position, the castling input.

### 2026-11-17 — LI-01A step 3: Chess960 games kept, continued, replayed; castling input (branch: features/chess960Library_171126)

#### Done
- **`GameRecord`**: `startFen` (JSON `variant: "chess960"`, `startFen`; a record without them is standard), `chess960()`, `startPosition()`, `setStartPosition()`, `replay(plies)`; `play(moves, start)`; never rated (`rateable()`); QML `chess960`, `startPosition`.
- **`GamePgn`**: `Variant "Chess960"`, `SetUp "1"`, `FEN` (lichess.org's tags); the board's PGN and a kept game's alike.
- **`ChessConnector`**: `startComputerGame` / `startLocalGame(…, chess960)` (`StandardGame` default, `RandomChess960`, or a number); `Chess960`, `StartPositionNumber` (`startPositionChanged`); `restartGame(start)` everywhere the board restarts; Rematch keeps the position; continue, replay (`openGame`) and Play from here (`StartFrom.start`) keep the record's; Home resets to standard; the SAN, the analysis's SAN and **the review / replay positions** (`shownGame`, which replayed from the standard start: stepping back in a 960 game was wrong) from the start.
- **Castling input** as lichess.org: with the king selected, a click or drop on its own castling rook castles; a dot on the rook and on the king's castling square when no move of its own goes there (#518 as 960: e1 → g1); a click there castles (`moveTarget`); premove too (click the king, then its rook).
- Tests: `GameLibraryTest` (a 960 record kept, read, replayed; not rated), `GamePgnTest` (the tags); **`tst_chess960.qml`** (8): castle by drop and by click on the rook, Rematch keeps #3, kept and continued, its PGN, replay and Play from here, #518 castling on the king's square and ← review, vs the robot with a premoved O-O, the robot's O-O and Undo of both.
- Found: `Run-UiTests.ps1` runs Release by default; without `-Build` it ran a Release exe from before step 1, so the local UI runs of steps 1–2 tested old code (CI built and ran them fresh: green). CLAUDE.md notes it.

#### Checked
- Debug: ChessTests 165/165, ChessQtTests 271/271; UI tests 11 files, 0 failed (Debug and Release, built fresh).

#### Next
- Step 4, the UI (plan §3.8): the setup dialogs' Variant row, "Chess960 #N" on the game page, the cards, screenshot scenes.

### 2026-11-17 — LI-01A step 4: the Chess960 UI (branch: features/chess960Ui_171126)

#### Done
- **`VariantPicker.qml`** (new): the Variant row of `ComputerGameDialog` and `TwoPlayerDialog`, Standard / Chess960 (a random one of the 960), with a line on what Chess960 is and how to castle ("Not rated." vs the robot). The choice is remembered (`AppSettings.chess960`); the result card's **New game** (and Ctrl+N in a game) offers the game's own variant; no row in Play from here (the fork keeps its start). `TwoPlayerDialog.openNew(chess960)` like the computer dialog's.
- **"Chess960 #N"** in `ChessConnector.GameTitle` (the command bar, the window title, the messages' source) for every mode; `restartGame` also emits `gameInfoChanged` when the position changes.
- **Cards**: `games.js` `details()` begins with "Chess960 #N" (the Games page, the Continue list).
- **Scenes**: `chess960` (#3's start), `chess960-select` (the king selected: a dot on its rook), `chess960-castled` (the f1/g1 swap), `computer-960` (the dialog with Chess960 chosen); sized `chess960@1400x900`, `chess960-castled@640x560`. `games` / `games-filter` / `profile` keep a 7th game, an unfinished #3 vs Bob (the Games pictures change).
- **`tst_chess960`** +3 (11): Two players with Chess960 chosen in the dialog (a random number, the title, remembered, New game offers it again), vs computer with Standard chosen again, no Variant row in Play from here.

#### Checked
- Debug: UI tests 11 files, 0 failed (`tst_chess960` 11/11). Screenshots of the new scenes looked at.

#### Next
- Step 5 (plan §4): `LI-01A-ManualTests.md` (Stockfish 960 at levels 1 and 5, both castlings, Takeback, Continue, replay, Analyse, the PGN pasted into lichess.org's analysis board), CLAUDE.md, `TASKS.md`.

### 2026-11-18 — LI-01A step 5: docs and the hand tests (branch: docs/chess960ManualTests_181126)

#### Done
- **`docs/LI-01A-ManualTests.md`** (new, 8 scenarios, for the owner): two players at a random position with both castlings (and the rare ones: the king staying, king and rook swapping) and the forbidden ones; Stockfish at level 1 (the weak picker) and 5, a premoved castling; Undo, Home, restart and Continue; Rematch / New game / the remembered choice; not rated; replay, Play from here, Analyse (the best-move arrow of a castling); the exported PGN imported on lichess.org/paste; the engine setting "Send the game to the engine as".
- **CLAUDE.md**: the overview (local games standard or Chess960), the setup dialogs' Variant row, `GameTitle`'s "Chess960 #N". **run-chess-app skill**: the scenes `chess960`, `chess960-select`, `chess960-castled`, `computer-960`; `games` has seven games.
- `TASKS.md`, the plan's status: stage 1 coded; its hand tests open.

#### Next
- The owner runs `LI-01A-ManualTests.md`.
- LI-01A stage 2 (Chess960 on Lichess) needs its plan (plan §6), or LI-01B Crazyhouse's plan, as the owner chooses.

### 2026-11-18 — LI-01A stage 1: the hand tests passed (branch: docs/li01aHandTestResults_181126)

#### Done
- The owner ran `docs/LI-01A-ManualTests.md`: all 8 scenarios passed (both castlings, Stockfish at levels 1 and 5, takeback / Continue, Rematch / New game, not rated, replay / Play from here / Analyse, the PGN imported on lichess.org, the engine setting). Results written in the file; `TASKS.md` and the plan: stage 1 done.

#### Next
- LI-01A stage 2 (Chess960 on Lichess) needs its plan (plan §6), or LI-01B Crazyhouse's plan, as the owner chooses.

### 2026-11-18 — LI-01A stage 2 plan: Chess960 on Lichess (branch: docs/chess960LichessPlan_181126)

#### Done
- **`docs/LI-01A-Chess960LichessPlan.md`**: what blocks Chess960 on Lichess now (`playableHere`, `parseChallenge`, the challenge form, `startOnlineGame`'s standard start, `gameFull`'s `initialFen` not passed on, the rating pools); the design (one `playableVariant` rule; `gameStarted` with the variant and start FEN; other variants refused with a message instead of a board out of step; the Online page's Variant row; Lichess's variant names; the Chess960 rating); 3 steps; tests (ChessQtTests, `tst_lichess960`, hand tests on lichess.org); risks.
- Checked in lila's source: the Board API limits games by the clock only (`Game.isBoardCompatible`), and the challenge forms take `variant` and `fen` (a 960 start, casual only).
- The owner's choice: a Chess960 Rematch keeps the position (the challenge sends the start FEN).
- `TASKS.md`: stage 2's steps; the stage 1 plan's §6 links the new plan.

#### Next
- Step 1 (`features/chess960LichessGames_…`): the C++ side and its ChessQtTests.

### 2026-11-18 — LI-01A stage 2 step 1: Chess960 Lichess games (branch: features/chess960LichessGames_181126)

#### Done
- **`LichessClient`**: `playableVariant` (standard, chess960) decides `canPlayHere` of the ongoing games and of the challenges (with `compat.board`); `variantOf` / `startFenOf` of a `gameFull`; `gameStarted` ends with `variant` and `startFen` (`""` for `startpos`); `variantName` (Lichess's names, `Q_INVOKABLE` static for step 2's QML); `challengePlayer(…, variant = "standard")`; `RematchInfo.startFen` (a Chess960 game's), sent as `fen` by the rematch (named or open challenge): the same position, the owner's choice; `parseRatings` reads `chess960`.
- **`ChessConnector::onLichessGameStarted`**: standard (no start FEN) or Chess960 (`StartPosition::FromFen`) starts from that position (`startOnlineGame(…, start, …)`); any other variant, or a FEN that isn't a 960 start, is refused before it replaces the board's game: `gameStartFailed` ("it is Crazyhouse, and Chess++ plays standard chess and Chess960. Open it on lichess.org."), the stream stopped.
- **ChessQtTests** +5 tests, 4 extended: the ongoing games' and challenges' variants, `variantName`, the ratings, `parseRematchInfo`'s start FEN, `gameStarted`'s variant and start FEN; `LichessChess960ChallengeTests`: the named challenge's variant, the 960 rematch's `fen` (named and open), none for standard.
- CLAUDE.md, the plan (§3.4: `variantName` in C++, used by step 2's QML).

#### Checked
- Debug: ChessQtTests 279/279; UI tests 11 files, 0 failed.

#### Next
- Step 2 (`features/chess960LichessUi_…`): the Online page's Variant row, Lichess's variant names in the cards and rows, the Profile's Chess960 pool, `FakeLichessServer` variants, scenes, `tst_lichess960` (also the connector's start from a 960 `gameFull` and the refusal, untested in C++ until then).

### 2026-11-18 — LI-01A stage 2 step 2: the Chess960 Lichess UI (branch: features/chess960LichessUi_181126)

#### Done
- **Online page**: a Variant row under the time control (`VariantPicker` with `online`: "…one of 960 start positions, picked by Lichess…"; remembered as `AppSettings.onlineChess960`, apart from the local dialogs' `chess960`); the challenge (named or a link) asks for `chess960`; with Chess960 chosen the rating line is "Your Chess960 rating" (one pool for every speed). `onlineCard` (Home) and `challengeButton` named for the tests.
- **Names**: `LichessClient` adds `variantName` to the ongoing games' and challenges' rows; the cards (`lichess.js`), the challenge rows and the challenge bar (now with the variant: "10+5 · Chess960 · Casual") show "Chess960", not the key.
- **Profile**: the Chess960 rating tile.
- **`FakeLichessServer`**: `challenge(…, variant, startFen)`, `addOngoingGame(…, variant, startFen)`; `gameFull` / `/api/account/playing` send them; the account has a Chess960 rating (1480?).
- **`tst_lichess960`** (4 tests): a 960 challenge accepted at #3 (the title, the board), a premoved O-O posted as `f8g8`, the opponent's `f1g1` from the stream; an ongoing 960 game continued from Home, castled by clicks (`f1g1` posted); a Crazyhouse challenge without Accept, a Crazyhouse game rejoined anyway refused (`gameStartFailed`, Home stays); the Online page's Variant row: the Chess960 rating, `variant=chess960` for a named challenge and a link, the choice remembered apart. The 960 rematch's `fen` stays covered by ChessQtTests (FakeLichess has no outgoing challenges).
- **Scenes**: `online-960` (new); `online-challenges` / `challenge`: SlowPlayer's challenge is Chess960; `profile`: the Chess960 tile. Their baselines change.
- CLAUDE.md, the run-chess-app skill.

#### Checked
- Debug: ChessQtTests all passed; UI tests 12 files, 0 failed; the scenes `online-960`, `online-challenges`, `profile`, `challenge` looked at.

#### Next
- Step 3 (`docs/chess960LichessManualTests_…`): `LI-01A-LichessManualTests.md` on real lichess.org, docs.

### 2026-11-18 — LI-01A stage 2 step 3: the hand tests on lichess.org (branch: docs/chess960LichessManualTests_181126)

#### Done
- **`docs/LI-01A-LichessManualTests.md`** (new, 9 scenarios, two Lichess accounts as BL-10's): a Chess960 challenge from lichess.org accepted (the same set-up and #N, both castlings both ways); a premoved castling; Leave, Continue, restart; Rematch keeps the position, and the opponent's rematch offer; a Chess960 challenge from the app (named and a link; the choice kept apart from the local dialogs'); correspondence; the PGN pasted at lichess.org/paste; the Chess960 rating; Crazyhouse refused (the bar, a joined link's message).
- `TASKS.md`, the plan's status: stage 2 coded, its hand tests open. CLAUDE.md already describes stage 2 (#182, #183).

#### Next
- The owner runs `LI-01A-LichessManualTests.md`; LI-01A is done when they pass.
- Then LI-01B Crazyhouse's plan, or the next item in `TASKS.md`, as the owner chooses.

### 2026-11-18 — LI-01A code review fixes (branch: bugs/li01aReviewFixes_181126)

A `/code-review medium` of all LI-01A code: 4 bugs, 5 cleanups, all fixed.

#### Done
- **A refused Lichess game touched the game on the board** (bugs 1, 4): after `ChessConnector` refused a game in `gameStarted` (Crazyhouse…), `handleGameStreamLine` still started its clock, sent its times and its end: a local game's clock jumped to Lichess's, its result could be set. Now `LichessClient` stops reading once the receiver stopped the stream in `gameStarted` (`_stops`, also no rematch of it), and `onLichessClockStarted` / `onLichessGameEnded` / `clockUpdated` ignore Lichess when no Lichess game is on the board.
- **A Chess960 game sent as `startpos`** (bug 2) was refused ("it is Chess960, and Chess++ plays … Chess960"): now Chess960 #518. A bad start FEN of a playable variant has its own message.
- **A Chess960 record with an unreadable `startFen`** (bug 3) became a standard game and lost its variant at the next write: now it stays Chess960 (written back as read), `replay()` fails, so continue / open refuse it; the card says "Chess960" without a number.
- Cleanups: `GameRecord::start` (the start position read once, not per card per list rebuild); `MoveGeneration::CastlingPath` shared by the legal moves and `GetPremoveTargets`; `ChessConnector.StandardGame` / `RandomChess960` as a QML enum (no -1 / -2 in QML); the screenshot seed's fork by a `fork` field, not `&kept[5]`; `ChessConnector::moveTargets` gives both the dots and the click / drop target (a click on the king's own square no longer castles when it stays put).
- Tests: ChessQtTests +2 (the refused game's stream; the unreadable record); `tst_lichess960` +2 (a 960 game at `startpos`; a refused game during a local game with a clock).

#### Checked
- Debug: ChessTests 165/165, ChessQtTests 281/281, UI tests 12 files, 0 failed; the scenes `chess960-select` and `games` as before.

#### Next
- The owner's hand tests: `LI-01A-LichessManualTests.md`.

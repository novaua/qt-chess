---
id: BL-19
title: "Games library: keep every local game, continue, fork, rate, analyse"
type: task
status: done
priority: medium
---

# BL-19 Games library: keep every local game, continue, fork, rate, analyse

<!-- migrated from TASKS.md:27 (In Progress) -->

- [ ] **BL-19 Games library: keep every local game, continue, fork, rate, analyse** [Hard] — plan: [`docs/BL-19-GamesLibraryPlan.md`](docs/BL-19-GamesLibraryPlan.md) (the owner's review of the save and game-log capabilities and decisions, 2026-11-13: every local game kept automatically, a cap by count, Play from here, a local rating and engine analysis in scope; logging Lichess games left out). Steps: 1–2 done (#162): `GameRecord` / `GameLibrary` (`users/<id>/games.json`, the old `chess.save` / `chess.autosave` migrated once, a damaged file kept aside, the last 100 finished games kept, 13 tests), `ChessConnector` writes the record with every move, takeback, result and leave, `continueGame(id)` (vs the robot or two players, the clock as left) and `openGame(id)` (a replay); the Save button, "Open saved game", the autosave slot and the "Leave without saving?" dialog are gone; Home's **Games** card and `GamesPage` (Open / Continue / Delete), the Continue list with every unfinished local game; the UI tests adjusted. Step 3 done (#163): the Games page's filters (All · vs Robot · Two players · Unfinished), Export PGN per card, the PGN's Event / robot Elo / TimeControl tags (`GamePgn`), Settings → Data "Games kept" (asks before deleting), scenes `games`, `games-filter`, `games-empty`, `tst_games.qml` (7 tests), `GamePgnTest.cpp` (5). Hand tests (plan §5) 1–8 passed (the owner, 2026-11-16); 9 ("Games kept" with 51 finished games) open. Step 4 done (#165): Play from here vs the robot or two players from any position of a kept game (the More menu, the setup dialogs' "from here" mode, `startComputerGameFrom` / `startLocalGameFrom`, a fork record, "from move N" on its card), scene `play-from-here`, `tst_games` 6–7; typed-in moves (Alt+S) are kept now. Review fixes of #162/#163/#165 (10, branch `bugs/bl19ReviewFixes_161126`). Step 4b (the owner's follow-up to #165, #167): Play from here only while reviewing (a replay, or a robot / two-player game stepped back) at a playable position, as a **Play from here** button in the review bar (the moves panel's bar beside Back to live, and the sheet handle in a narrow window; a replay gets the bar too) that opens vs computer… / Two players…; the More menu's two entries removed. Step 5 (branch `features/localRating_161126`): the rating vs the robot, **stored as it goes** (the owner's choice, 2026-11-16): `LocalRating` in `games.json`, each rated game's change on its record (`GameLibrary::put`), a takeback after the result gives it back, removing games never changes it; Home's header, the Profile page's Statistics card, "You won · +27" on the cards, the PGN's player Elo; GameLibraryTest +5, GamePgnTest +1, `tst_games` 9 (#168). Step 6 (branch `features/analysisEngine_161126`): the analysis engine, no UI yet: `GameAnalysis` (evals and best moves per position, Lichess's win chances, judgements at 5 / 10 / 15 points and game accuracy), `GameAnalyzer` (a thread and a full-strength engine per analysis, progress, cancel; kept on the record only for the moves analysed, dropped when they change), `ChessConnector.analyseGame` / `cancelAnalysis` / `Analysing…`, Settings → Computer "Analysis depth", FakeUci's scores; `GameAnalysisTest.cpp` (12), FakeEngineTests +2 (#169). Step 7 (branch `features/analysisUi_161126`): the analysis UI: Analyse on the result card, in More and in the moves panel (only a game that is over, or a replay: no hints during a game), the progress with Cancel, the summary (accuracy and `?!` `?` `??` per side, the shown move's "Blunder · Best was Bb4", its eval), the marks in the move list, an eval bar beside the board; scene `analysis`, `tst_analysis.qml` (4), MoveListModelTest +1 (#170). Review fixes of #167–#170 (10, branch `bugs/bl19AnalysisReviewFixes_161126`): an aborted robot game (before both sides moved) isn't rated, as on lichess.org (the owner's choice). Step 8 (branch `docs/bl19ManualTests_161126`): [`docs/BL-19-ManualTests.md`](docs/BL-19-ManualTests.md) (9 scenarios: "Games kept" with 51 games (plan §5 test 9, a script makes them), Play from here, the rating, the analysis with Stockfish, depth / Cancel / leaving, no engine, not during a game, export / import, dark and high contrast), the `analysis` scene in the `run-chess-app` skill. Open: the owner's hand tests; the task is done when they pass. Follow-ups of the review (the owner, 2026-11-17, branch `features/reviewBestMove_171126`): a judged move shows two arrows on the board, the move played in its mark's colour and the engine's best in green ("Best was …" green too; `MoveArrows`, `tst_analysis` 6); Play from here no longer forks a game going on (a robot / two-player game stepped back: only once it is over; replays as before; `tst_games` 6). Follow-up (not in scope): log the Lichess games played here.

## Resolution

### 2026-11-16 — BL-19 steps 1–2: the games library's backend and first UI (branch: features/gamesLibraryBackend_131126)

Plan: `docs/BL-19-GamesLibraryPlan.md` (the owner's decisions of 2026-11-13).

#### Done
- **`GameRecord` / `GameLibrary`** (`gamelibrary.h/cpp`, QML singleton): every local game (vs the robot, two players) kept from its first move in `users/<id>/games.json`, newest first; follows the active profile; the last 100 finished games kept, unfinished ones never removed on their own; a damaged file is kept aside; the old `chess.save` / `chess.autosave` migrated once, then cleared. `ChessQtTests/GameLibraryTest.cpp` (13 tests); export/import carries `games.json`.
- **`ChessConnector`** writes the record with every move, takeback, result and when the game is left; `continueGame(id)` (the robot at its level or two players, the clock as left), `openGame(id)` (a replay). The Save button, "Open saved game", the autosave slot and the "Leave without saving?" dialog (#161) are gone: leaving a local game never asks.
- **QML**: Home's **Games** card (Ctrl+O) → `GamesPage` (Open / Continue / Delete, `games.js`); the Continue list shows every unfinished local game before the Lichess ones; `GameCard` for a kept game.
- UI tests adjusted (`tst_twoPlayers` test 3 now: leaving keeps the game, Continue has it; `tst_board`, `tst_profiles`, `tst_restart`, `tst_robot`, `AppDriver`).

#### Checked
- Debug build clean; ChessTests 142/142, ChessQtTests 234/234; UI tests 8 files, 0 failed (120 s).
- Screenshots `-Scene all` (Debug) against the baselines: the changes are the expected ones (Home's Games card for "Open saved game", the cards' new subtitles, About's build date). Settings' shortcut list still said "Ctrl+O Open the saved game" / "Ctrl+S Save the game": now "Your games (Home)" / "Export the game as PGN".
- **Not this branch:** the Lichess scenes at 640x560 (`opponent-left-turn`, `messages`, `draw-offer`) time out waiting for `boardIdle`, on `develop` too (checked in a worktree, Debug); `draw-offer@640x560` passed once in the full run, so it is timing.

#### Next
- BL-19 step 3 on its own branch: library UI polish (filters, Export PGN per card, PGN tags, Settings "Games kept", scenes `games*` + baselines, `tst_games.qml`). Then the owner's hand tests (plan §5).
- The Lichess scenes at 640x560 that never see the board idle (above): find why, separately.

### 2026-11-16 — BL-19 step 3: the games library's UI polish (branch: features/gamesLibraryUi_161126)

#### Done
- **Games page**: filters All · vs Robot · Two players · Unfinished (`ChoiceRow`, objectName `gamesFilter`; "No unfinished games." when a filter shows none); **Export PGN** on every card (`GameCard.exportable`) → the window's file dialog for that game (`ChessGame.exportPgn(gameId)`).
- **`GamePgn`** (`gamepgn.h/cpp`): one writer for the game on the board and a kept game. New tags: Event ("Casual game vs the robot, level 3 (Club Player)", "Casual two-player game"), the robot's `WhiteElo`/`BlackElo`, `TimeControl` ("300+3", "-"; none for Lichess). `ChessConnector::pgn/pgnFileName/exportPgn` take an optional game id; a replay is written from its record. `GameRecord::setMoves` (moves + fen) for records made outside a game.
- **Settings → Data "Games kept"** 50/100/200/500 (`AppSettings.gamesKept`, bound to `GameLibrary.keepCount`); a smaller number that would delete games asks first (`gamesKeptConfirm`: "Keep only 50 finished games?", Delete N games). `GameLibrary.finishedCount`. "saved games" → "games" in Settings, the profile export and import texts.
- **Scenes** `games` (six kept games of Tester on fixed days, seeded in main.cpp; also `@640x560`, `@1400x900`), `games-filter`, `games-empty`; the stale `saved` / `leave-local` names gone from main.cpp's list.
- **Tests**: `tst_games.qml` (three games played, the filters, the PGN of both kinds and the exported file, Delete, Games kept with Cancel / Delete / more); `GamePgnTest.cpp` (5). Test helpers: `TestRun.addFinishedGames(n)`, `fileUrl`, `readTextFile`; `AppDriver.scrollIntoView(name)`; `ConfirmDialog`'s Cancel is `cancelButton`; `ChoiceRow` buttons are `<objectName>_<index>`.

#### Checked
- Debug: ChessTests 142/142, ChessQtTests 239/239; UI tests 9 files, 0 failed (124 s). Scenes `games*`, `about` (the new row) looked at.

#### Learned
- A move from an empty square in `Game::LoadMoves` hits a Debug assert (abort, a crash dialog on the desktop) instead of the `ChessException` an illegal move throws: a unit test fed it one and crashed the test host twice. The test uses an illegal move from an occupied square now; a damaged `games.json` record with such a move would still abort a Debug build (`openGame`, Export PGN).
- Git Bash's `sed -i` turns CRLF files into LF: edit with Python (or the Edit tool) instead.
- The UI tests can't see the app's `AppSettings` (a qrc singleton): check `GameLibrary.keepCount`, which follows it.

**Hand tests (the owner, 2026-11-16):** plan §5 rows 1–8 passed; row 9 ("Games kept" 50 with 51 finished games) still open.

#### Next
- Hand test 9, then step 4 Play from here.

### 2026-11-16 — BL-19 step 4: Play from here (branch: features/playFromHere_161126)

#### Done
- **More menu** of the game page: "Play from here vs computer…", "Play from here, two players…" (not online; enabled when `ChessConnector.playFromHere()` has a position: of a kept game, after a move, not mate/stalemate). They open the setup dialogs with `openFrom(position)`: title "Play from here …", "From the position after 1… e5: White to move.", the side to move preselected.
- **`ChessConnector`**: `playFromHere()`, `startComputerGameFrom` / `startLocalGameFrom(gameId, ply, …)` (`_startFrom` → `applyStartFrom()` after the restart: the moves played quietly, `_forkOfId`, `_forkPly`); the robot answers when it is to move; the record only at the fork's own first move; a takeback stops at `_forkPly`; the clock counts `clockPly()`. `GameRecord::play(moves)` shared by `setMoves`, `GamePgn` and the fork check.
- **Cards**: "from move N" for a fork (`games.js`). **Scenes**: `play-from-here`; `games` has a fork (14 Nov, from move 3).
- **Fixes found on the way**: `applyMoves` (Alt+S, typed-in moves) now writes the record (it never did since step 2); a kept game without avatar names (made outside a game, an old save) keeps the current avatars instead of loading `avatars/.png`; the More menu is 280 wide (the new entries were cut).
- **Tests**: `tst_games` test 6 (fork vs the robot from a reviewed two-player game: dialog, moves, record, takeback limit), test 7 (fork for two players from a replay; not kept without a move; none after a mate).

#### Checked
- Debug: ChessTests 142/142, ChessQtTests 239/239; UI tests 9 files, 0 failed (133 s). Scenes `play-from-here`, `more-menu`, `games` looked at.

#### Learned
- In Qt 6 QML, `const g = GameLibrary.games[0]` is a reference that reads the list property again on access: after the list changes, `g` is the new first element. Copy what you need (`.id`).

#### Next
- Hand test 9 (step 3), and Play from here by hand; then step 5, the rating.

### 2026-11-16 — BL-19 review fixes (branch: bugs/bl19ReviewFixes_161126)

A `/code-review medium` of #162, #163 and #165 found 10 problems; all fixed:
1. **Migration**: an old autosave of a Lichess game (`isSinglePlayer` false) isn't migrated (those versions never offered it); it used to become an unfinished two-player game in Continue.
2. **"Games kept" is every profile's**: the confirm counts every profile's games beyond it (`GameLibrary::finishedBeyond`), and `setKeepCount` trims the other profiles' files at once (they were cut later, without a word). Text: "N finished games are deleted for good: the oldest beyond 50 of each profile on this computer."
3. **A `games.json` that can't be opened now** (a lock) was taken as missing and the next move wrote over it with one game: now it is never written over; the next write reads it again and merges (`_unreadable`).
4. **Continue of a damaged record** went on from the moves before the bad one and saved only those: `continueGame` / `openGame` check the moves first (`GameRecord::play`) and refuse with a message (`keptGameFailed`).
5. **An old save that can't be read** was deleted: kept as `.damaged`.
6. The Continue list's "your turn in N" counted two-player games; now as the cards: robot games on the player's move and Lichess games.
7. **Writes**: `put` / `remove` write once the current event is done (a move and its result: one write); `flush()`, the destructor and a profile change write at once.
8. The opponent profile's lookup: `UserManager::nameOf` / `avatarOf` (new) in `startLocalGame` and `continueGame`.
9. `GameRecord::play` replays once (`Restart` + `playOn`), and `playOn` is the connector's `replayMoves` too; a move from an empty square is refused instead of asserting (the Debug abort found in step 3).
10. `openGame` calls `beginGame`: the previous game's fork, takebacks and Lichess rematch don't carry into a replay.

Tests: GameLibraryTest +4 (a file that can't be read, old saves that can't be migrated, keepCount over profiles, `play` / `playOn`), 3 adjusted for the deferred write; `tst_games` test 8 (a damaged game: Continue and Open refuse, the record unchanged; `TestRun.addDamagedGame`). Debug: ChessTests 142/142, ChessQtTests 243/243, UI tests 9 files 0 failed.

### 2026-11-16 — BL-19 step 4b: the Play from here button (branch: features/playFromHereButton_161126, #167)

The owner's follow-up to #165: Play from here only while reviewing, as its own button (placement and the More menu: the owner's choice, 2026-11-16).

#### Done
- **`ChessConnector.PlayFromHere`** (a property now, NOTIFY `reviewStateChanged`): only in a replay or a robot / two-player game stepped back (`ReviewMode`); the live position no longer counts.
- **The review bar's Play from here button** → a menu (vs computer… / Two players…) → the setup dialogs' `openFrom` as before: in the moves panel's bar (`MovesPanel`, objectName `playFromHereButton`, beside Back to live; the buttons go under the text) and the narrow window's sheet handle (`playFromHereHandleButton`; the sheet widens to fit its buttons). A replay shows the bar too ("Move 9 of 20", no Back to live). One menu for both (`GamePage.openFromHereMenu`, made on the first click; above the button near the window's bottom).
- **The More menu's two entries removed** (and its 280 width).
- **Scenes** `replay`, `play-from-here-menu` (also `@640x560`), `review@640x560`.
- **Tests**: `tst_games` 6 (not at the live position, not in the More menu; the button's menu), 7 (the button in a replay; gone after the mate, back a move earlier).

#### Next
- Step 5, the rating (#168).

### 2026-11-16 — BL-19 step 5: the rating vs the robot (branch: features/localRating_161126)

The owner chose **stored as it goes** over the plan's computed rating (2026-11-16): games leave the library (the cap, Delete), and a computed rating would move with them.

#### Done
- **`LocalRating`** (`gamelibrary.h`): Elo from 1200 against the robot's `DisplayElo()`, K 40 for the first 30 rated games then 20, provisional under 10; in `games.json` as `"rating": { value, games }` (only once a game is rated). `readFile` / `writeFile` carry it (`ProfileFile`), so trimming another profile's file keeps its rating; the "can't be read now" merge adds what was rated meanwhile.
- **`GameLibrary::put` rates** a record that is rateable (`GameRecord::rateable()`: vs the robot, finished with a result, no fork, no takeback) and not rated yet: `rated`, `ratingBefore`, `ratingChange` on the record. A rated record put back unfinished (a takeback resumed it) gives its change back (exact: only the game on the board can be). Migrated old saves aren't rated. Properties `rating`, `ratedGames`, `ratingProvisional`.
- **UI**: Home's account button "Rating 1227?" (`homeRating`, once a game is rated); the Profile page's Statistics card "Rating vs the robot" (the number, provisional N of 10, or how to get one); a Games card "You won · +27" (`games.js`: `ratingChangeText`, `resultWithRating`, `ratingText`).
- **PGN**: the player's rating before a rated game as `WhiteElo` / `BlackElo` (`GamePgn::setPlayerElo`).
- **Scenes**: `profile` gets the `games` scene's kept games (two rated robot games: 1227?).
- **Tests**: GameLibraryTest +5 (Elo, rated once and from the file, games not rated, a takeback gives it back, removing keeps it), GamePgnTest +1; `tst_games` 9 (a won robot game: Home's header and the card; a takeback after the mate gives the change back, mated again: not rated).

#### Next
- Step 6, the analysis engine.

#### Checked
- Debug: ChessTests 142/142, ChessQtTests 249/249; UI tests 9 files, 0 failed (144 s). Scenes `games`, `profile` looked at.

### 2026-11-16 — BL-19 step 6: the analysis engine (branch: features/analysisEngine_161126)

#### Done
- **`GameAnalysis`** (`gameanalysis.h/cpp`): per position 0…n the eval (White's view; mate in N ±(100000 − N)) and the engine's best move; stored on the record as `{ version, depth, evals, best }`. Win chance (Lichess's curve, ±1000 cp ceiling), the mover's drop, judgements at **5 / 10 / 15 points** (Lichess's 0.1 / 0.2 / 0.3 of its −1…1 winning chances; the plan's 10 / 20 / 30 was a misreading, corrected there), lila's game accuracy (weighted + harmonic mean).
- **`GameAnalyzer` / `AnalysisWorker`** (`gameanalyzer.h/cpp`): a thread and a full-strength `UciConnector` per analysis, positions asked as the robot asks (`ChessEnginePlayer::MakeRequest`, factored out of `FindMove`); the final mate / stalemate isn't asked; progress; cancel (kills the engine, keeps nothing). Kept only when the record still has the analysed moves (`GameLibrary::setAnalysis`); a change meanwhile fails it at once. `GameLibrary::put` drops a stale analysis (ply count differs).
- **`ChessConnector`**: `analyseGame(id)` (`""` = the game on the board), `cancelAnalysis()`, `Analysing`, `AnalysisGameId`, `AnalysisDone`/`AnalysisTotal`, `AnalysisDepth`, `analysisFinished`/`analysisFailed`. No UI uses them yet (step 7).
- **Settings → Computer "Analysis depth"**: Fast (10) / Normal (14) / Deep (18), `AppSettings.analysisDepth`.
- **FakeUci** scores its one line: `mate 1` when its move mates, else the material after it.
- **Tests**: `GameAnalysisTest.cpp` 12 (numbers; FakeUci on the scholar's mate; Stockfish; cancel; a change meanwhile; nothing to analyse; the record's side), FakeEngineTests +2. ChessQtTests now builds FakeUci (not linked).

#### Checked
- Debug: ChessTests 144/144, ChessQtTests 261/261; UI tests 9 files, 0 failed (151 s). Scene `settings` looked at (its baseline differs: the new row).

#### Next
- Step 7, the analysis UI (plan §3.6): Analyse on the result card and in More, progress, the eval bar, `?!` `?` `??` in the move list, the summary, "Best was …".

### 2026-11-16 — BL-19 step 7: the analysis UI (branch: features/analysisUi_161126)

#### Done
- **`ChessConnector`**: the board game's analysis (`refreshBoardAnalysis`, on its moves, result, record and the analysis's state): `CanAnalyse` (a kept game that is over, or a replay; not one going on: no hints), `AnalysingThisGame`, `Analysis` (accuracy and counts per side), `ShownAnalysis` (the shown position's eval text and White's win chance; the move's mark, judgement and the engine's best in SAN).
- **`MoveListModel`**: `wMark` / `bMark` (`setMarks`); the move list shows `?!` `?` `??` in Lichess's colours (`Theme.inaccuracy` / `mistake` / `blunder`, `Theme.markColor`).
- **QML**: Analyse on the result card (2×2 with Rematch, New game, Review moves), in More ("Analyse game" / "Analyse again"), in the moves panel; `AnalysisSummary.qml` (progress with Cancel, then the summary and "Blunder · Best was Bb4"); `EvalBar.qml` left of the squares; the review bar and the sheet handle add the mark; an `analyse` icon; analysisFailed → a critical message.
- **Scene** `analysis` (main.cpp: a kept game with a made-up analysis; also `@1400x900`, `@640x560`).
- **Tests**: `tst_analysis.qml` 4 (result card → progress → marks, summary, eval bar, review; cancel; a takeback drops it; a kept game opened), MoveListModelTest +1.

#### Checked
- Debug: ChessTests 144/144, ChessQtTests 262/262; UI tests 10 files, 0 failed (171 s). Scenes `analysis` (three sizes), `result`, `ended@1400x900` looked at; their baselines differ (new).

#### Next
- Step 8: docs and hand tests (plan §5): CLAUDE.md, the `run-chess-app` scene list, `BL-19-ManualTests.md`.

### 2026-11-16 — BL-19 step 8: docs and hand tests (branch: docs/bl19ManualTests_161126)

#### Done
- **`docs/BL-19-ManualTests.md`**: 9 scenarios for the owner, on a throwaway data folder (`--data-dir`): "Games kept" with 51 finished games (plan §5's open test 9; a PowerShell script adds 50 finished games to `games.json` as text, checked: the app reads all of them), Play from here with Stockfish, the rating, analysing a real game (and how long it takes), depth / Cancel / leaving / closing, no engine, not during a game, export and import, dark and high contrast.
- The plan's §5 points there; the `run-chess-app` skill lists the `analysis` scene. CLAUDE.md was kept up to date by steps 6–7.

#### Next
- The owner runs `BL-19-ManualTests.md`; BL-19 is done when they pass. Follow-up (not in scope): log the Lichess games played here.

### 2026-11-16 — BL-19 review fixes of #167–#170 (branch: bugs/bl19AnalysisReviewFixes_161126)

A `/code-review medium` of #167–#170 found 10 problems; all fixed (the owner: "fix them all, Lichess rule for aborted games"):
1. **`AnalysingThisGame`** followed only the analyzer: after a rematch / new game the old game's "Analysing… N of M" and Cancel stayed. Now worked out in `refreshBoardAnalysis` with the board's record (NOTIFY `boardAnalysisChanged`).
2. **Deleting the analysed game or switching profile** showed a critical "couldn't analyse" message: now it stops quietly (a takeback meanwhile too).
3. The failure message doubled its sentence: one sentence now ("The chess engine couldn't analyse the game.").
4. **Another game's Analyse** stopped the running analysis without a word: `GameAnalyzer::interrupted` → "The other game's analysis stopped: one game is analysed at a time."
5. **`games.json` that can't be read**: the rating showed and rated from 1200. Now a game that ends then waits (`_rateLater`) and is rated from the real rating once the file is read; the merge takes the file's rating.
6. **The analysis matched by ply count only**: it now keeps a key of its moves (`GameAnalysis::movesKey`, `"moves"` in the JSON); other moves of the same length (Alt+S) drop it.
7. The moves panel's **Analyse game** didn't close the result card: it goes through `GamePage.analyse()` as the card's.
8. **Aborted games**: a robot game resigned before both sides moved isn't rated (as lichess.org; `GameRecord::rateable`, the owner's choice).
9. The summary and the best moves' SAN are worked out once per analysis (`_analysisSummary`, `_bestSan`), not on every read / review step.
10. The Profile page's "N of 10" reads `GameLibrary.provisionalGames`.

Tests: GameLibraryTest +1 (a game ended while the file can't be read), rating tests with both sides' moves and an aborted game; GameAnalysisTest +1 (another game interrupts), the change-meanwhile test now quiet (and a delete), the same-length other moves; `tst_analysis` +1 (the panel's Analyse closes the card; a rematch shows no old progress, the old analysis is kept). Debug: ChessTests 144/144, ChessQtTests 264/264; UI tests 10 files, 0 failed.

### 2026-11-17 — BL-19 follow-ups: best-move arrows, no fork of a game going on (branch: features/reviewBestMove_171126)

The owner's two small asks on the game review:
1. **The best move on the board**: on a judged move (?!, ?, ??) whose engine move was another, two arrows: the move played in its mark's colour (lichess.org's blue / orange / red) and the engine's best in green, both from the position before it (`MoveArrows.qml`, over the pieces, turns with the board). `ShownAnalysis` gives `playedFrom/To`, `bestFrom/To` (squares, -1 without). "Best was Bb4" in the summary is green to match. Arrows only where the text says "Best was" (judged moves), as lichess.org's review.
2. **Play from here not from a game going on**: a robot / two-player game stepped back offers it only once it is over (`playFromHere()` checks the result; `PlayFromHereChanged` follows the review and the result). Replays (also of an unfinished kept game) as before. Scenes `play-from-here` and `play-from-here-menu` now use the replay.

Tests: `tst_analysis` +1 (`test_6_bestMoveArrows`), `tst_games` test 6 (none during the game, offered after resigning, still reviewing). Release: UI tests 10 files, 0 failed. Looked at: scenes `analysis` (narrow, wide 1400x900, Black), `play-from-here-menu`.

### 2026-11-19 — BL-19 hand tests run by Claude (branch: docs/bl19HandTestRun_191126)

#### Done
- All 9 scenarios of `docs/BL-19-ManualTests.md` run as a throwaway Qt Quick Test file: the app's window offscreen, real mouse clicks (`AppDriver`), **Stockfish 19**, real time and files, throwaway data folders; four runs (a restart, no engine, a second data folder for the import). Results in each scenario's "Result:" line.
- Passed: 1 ("Games kept" 50 of 51, also after a restart), 2 (Play from here vs Stockfish), 3 (the rating: a loss, Ctrl+Z gives it back, delete, an aborted game), 4 (analysis of a 24-move game: 2.6 s at depth 14; "Best was e3" also Stockfish's own first choice at depth 18, checked apart with python-chess), 5 (Deep 16 s, Cancel, on in the background, the app closed mid-analysis in 0.19 s with no stockfish.exe left), 7, 8 (export, import into another data folder), 9.1 (dark).
- Found: **BL-25** (scenario 6: Analyse game is enabled without an engine until Settings has been opened; pressing it fails with a message), **BL-26** ("More (1 new messages)").
- The doc fixed where it no longer matched the app: scenario 1's script (since BL-20 games are signed files a script can't add: `TestRun.addFinishedGames`), the newest card (the unfinished 1. e4), scenario 2's step 1 (an unfinished game's card offers only Continue: resign it first; the replay opens at the start).

#### Checked
- Looked at, and not app bugs: a picture of a reviewed move taken under 0.5 s after the click still showed the last position (the board was redrawn by 0.5 s; both positions checked against python-chess); the move list's piece figures are blank in the offscreen window only (their `layer.effect`; the screenshot scenes show them).

#### Left for the owner
- 6: the disabled Analyse game's tooltip on hover (hover doesn't reach items offscreen); 9.2: Windows' own contrast theme; how it looks and feels in a real window. Whether BL-19 is done with these and BL-25 open is the owner's call.

### 2026-11-20 — the owner's hand tests; BL-19 done (branch: docs/bl19HandTestsDone_201126)

The owner ran what was left of `docs/BL-19-ManualTests.md` in a real window (`develop` at #206, Release, Stockfish, a throwaway data folder):
- Step 1 (a full game vs the robot, then Analyse): the marks (?! blue, ? orange, ?? light red), the played and best arrows, Play from here, Back to live, Rematch / New game: OK. Scenario 3 at a glance: 1187, provisional (1 of 10 rated games).
- 6 (no engine): OK, the tooltip too (a disabled Button does get hover here). BL-25's fix holds: Analyse disabled before Settings was opened.
- 9.2 (Windows' contrast theme): OK. Found on the way: **BL-27** (a black captured piece nearly vanishes on a dark card).

All 9 scenarios pass; BL-19 is done. Follow-up, not in scope: log the Lichess games played here.

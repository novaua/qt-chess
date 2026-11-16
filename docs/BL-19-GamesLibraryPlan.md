# BL-19 Games Library — keep every local game, continue, fork, rate, analyse

| | |
| :---- | :---- |
| **Date** | 2026-11-13 |
| **Status** | Steps 1–7 merged (#162, #163, #165, #167–#170); step 8 (docs, hand tests) on `docs/bl19ManualTests_161126`; hand tests 1–8 passed (2026-11-16); the rest in [`BL-19-ManualTests.md`](BL-19-ManualTests.md) |
| **Branch** | each step on its own branch from `develop` |
| **Task** | BL-19 in `TASKS.md` (the owner's review of the save and game-log capabilities, 2026-11-13) |

## 1. What there was (reviewed 2026-11-13)

Two save slots per profile, both under `users/<id>/`:

| Slot | Written | Format | Opened as |
|---|---|---|---|
| `chess.save` — "Save game" (Ctrl+S, a toolbar button) | only when pressed; the next Save overwrote it | `Game::Save`: a header, a count, then raw `HistoryMove` structs (not portable, no metadata) | **replay only** (`loadGame`: mode `"replay"`); it could never be continued |
| `chess.autosave` — the robot's game | on leave / close / switch user, **robot games only**; deleted when continued or finished | the same binary file | `continueGame`: the engine at the saved level, the clock restored |

The metadata lived apart from the moves, in `users.json` as two `GameSaveInfo` per user. Two-player games were
never kept unless Save was pressed, so leaving one asked "Leave without saving?". The statistics are counters per
opponent (`UserManager::recordResult`): no per-game record, no dates. The PGN had seven tags, no `TimeControl`,
no Elo. Nothing about a finished game was kept: no log, nothing to rate or analyse.

## 2. The owner's decisions (2026-11-13)

1. **Every local game is kept automatically** (vs the robot and two players, finished or left), from its first
   move. The Save button, "Open saved game" and the "Leave without saving?" dialog go; Export PGN stays (Ctrl+S).
2. **Retention: a cap by count, no time limit.** The last N finished games per profile (Settings → Data, default
   100); the oldest finished game goes when a new one comes. Unfinished games are never removed on their own.
3. **In scope:** the library and continuing any unfinished local game; **Play from here** (a new game from any
   position of a kept game, vs the robot at a chosen level or two players); a **local rating**; **engine
   analysis** of a kept game. **Out of scope:** logging Lichess games played here (a follow-up; the ongoing
   Lichess games stay Lichess's and the statistics count them as before).

## 3. Design

### 3.1 The record and the file

`users/<id>/games.json` — `{ "version": 1, "games": [ … ] }`, written whole with `QSaveFile` on every change
(a record is small: a crash loses at most the move being made). One file per profile keeps export/import,
delete-profile and backups simple (`kProfileFiles` in `usermanager.cpp`).

A `GameRecord` (`gamelibrary.h`): `id` (uuid), `kind` ("computer" | "local"), `finished`, `startedAt`,
`finishedAt`, `playerPlaysWhite`, `opponentName`, `opponentUserId` ("" = Guest), `level` (the robot's),
the two avatars, the clock (`clockLimitSec`, `clockIncSec`, `whiteMs`, `blackMs` as left), `moves` (UCI, as
Lichess), `fen` (the last position, for the card's board), `result` ("1-0" | "0-1" | "1/2-1/2" | ""),
`resultText`, `resultRecorded`, `undoCount` (a game with a takeback isn't rated), `forkOfId` + `forkPly`
(Play from here), `analysis` (step 6; absent until made). The player's own name isn't stored: it is the profile's.

### 3.2 `GameLibrary` (`ChessGui/ChessGui/gamelibrary.h/cpp`)

The active profile's records, newest first; follows `UserManager::activeUserChanged`; `put` (add or replace,
then write and trim), `remove`, `find`, `unfinished()`; `keepCount` (the cap; `AppSettings.gamesKept` binds it,
step 3). **Migration:** on the first load of a profile without a `games.json`, its `chess.autosave` and
`chess.save` (read with the old `Game::Load`, kept for this alone) and their `GameSaveInfo`s become records: the
autosave an unfinished robot game; the saved game finished when it ended in mate or stalemate, else unfinished
(it can go on now). The old files are then removed and the infos cleared. A damaged file is kept aside as
`games.json.damaged` and the library starts empty.

The sixth typed QML singleton, `GameLibrary` (`AppServices`): `games`, `unfinishedGames` (QVariantLists, as
`LichessClient.ongoingGames`), `count`, `unfinishedCount`, `keepCount`, `remove(id)`, `game(id)`. Browsing and
Delete are non-game work, so QML calls it directly; anything that puts a game on the board goes through
`ChessConnector` (the Key Conventions).

### 3.3 `ChessConnector`

- **Writes the record** (`updateRecord()`): at the first move and every move (`onHumanMoveDone`,
  `playRobotMove`), a takeback (`movePrev`: `undoCount`), the result (`setResult`: finished, `finishedAt`) and
  when the game is left (`keepGameOnLeave()` in `endGame`, the destructor and when a Lichess challenge replaces
  the game; a promotion still being chosen doesn't stand). Not for Lichess games or a replay.
- **Gone:** `saveGame`, `loadGame`, `CanLoad`, `leaveLosesGame`, `CanContinue`, `ContinueGame`,
  `ContinueDescription`, the autosave slot. `UserManager`'s `GameSaveInfo` stays for the migration only.
- **`continueGame(id)`**: an unfinished record goes on — vs the robot at the record's level, or two players with
  the opponent's profile when it still exists (else Guest under the name it had), the clock as left (paused until
  `pageOpened`). **`openGame(id)`**: any record as a replay (`Game::LoadMoves` → `HistoryPlayer`, mode
  `"replay"`; the PGN gets the record's result). `continueOnlyGame()` counts the local unfinished games with the
  Lichess ones.
- Later: `startComputerGameFrom(gameId, ply, …)` / `startLocalGameFrom(…)` (step 4), the PGN's `TimeControl`,
  `WhiteElo`/`BlackElo` and `Event` tags (step 3), `pgnOf(id)` for a card's Export PGN.

### 3.4 QML

- **Home:** the "Open saved game" card is **Games** (Ctrl+O): "12 games · 1 unfinished · replay or continue",
  dimmed with "Your games are kept here as you play them". The Continue card and list count every unfinished
  local game (one `GameCard` each: the board as left, "Robot · Level 3 · Club Player" or the other player,
  whose turn) with the Lichess ones. `games.js`: the words for a kept game (title, details, result, turn).
- **`GamesPage.qml`** (pushed over Home like the Online page, objectName `library`): every kept game as a
  `GameCard`, newest first: Open (finished: a replay) or Continue (unfinished), Delete (a confirm). Step 3 adds
  the filters (All · vs Robot · Two players · Unfinished), Export PGN per card, Settings → Data "Games kept".
- **The game page:** no Save button; Ctrl+S exports the PGN. **The window:** no "Leave without saving?" and no
  accept-challenge confirm (a local game is kept); Switch user's confirm says the game is kept.
- Screenshot scenes: `saved` and `leave-local` are gone (their baselines too); `continue*` and `robot` continue
  the kept game by id. New in step 3: `games` (six kept games on fixed days, main.cpp; also `games@640x560`
  and `games@1400x900` in place of a `games-narrow`), `games-filter` (Unfinished), `games-empty`.
- **Step 3 as built:** `GamePgn` (`gamepgn.h/cpp`) writes both the game on the board and a kept game: Event
  "Casual game vs the robot, level 3 (Club Player)" / "Casual two-player game", the robot's `WhiteElo`/`BlackElo`
  (`EngineLevel::DisplayElo`; the player's comes with step 5), `TimeControl` "300+3" or "-" (none for a Lichess
  game). `ChessConnector::pgn(gameId)`, `pgnFileName(gameId)`, `exportPgn(file, gameId)`. Settings → Data
  "Games kept" 50 / 100 / 200 / 500 (`AppSettings.gamesKept` → `GameLibrary.keepCount`); a smaller number that
  deletes games asks first ("Keep only 50 finished games?", Delete N games).

### 3.4.1 Play from here (step 4, as built)

- The game page's More menu: **Play from here vs computer…** and **Play from here, two players…** (not in a Lichess
  game), enabled when `ChessConnector.playFromHere()` gives a position: the one shown (a replay's, or the local
  game's, live or reviewed) of a kept game, after at least one move, not mate or stalemate.
- They open the usual setup dialogs (`openFrom(position)`): the title says "Play from here", a line says where
  ("From the position after 1… e5: White to move."), the player takes the side to move by default.
- `startComputerGameFrom` / `startLocalGameFrom(gameId, ply, …)`: the game restarts, the kept game's first `ply`
  moves are played on it quietly, then it goes on (the robot answers if it is to move). The fork's record is made
  at its first move of its own (left before one: not kept); `forkOfId`, `forkPly`. A takeback vs the robot stops at
  `forkPly` ("No more moves"); the clock counts only the fork's own moves (it runs after both sides moved).
- The card says "from move N". Scenes: `play-from-here`; `games` has a fork. Tests: `tst_games` 6–7.

### 3.4.2 The Play from here button (step 4b, the owner's follow-up to #165, 2026-11-16)

- Only **while reviewing** (a replay, or a robot / two-player game stepped back: `ReviewMode`) and only at a playable
  position (after a move, not mate or stalemate): `ChessConnector.PlayFromHere` (a property now, `{}` otherwise; the
  live position of a game no longer counts).
- A **Play from here** button in the review bar, where the eye is while stepping: the moves panel's bar ("Viewing
  2… Nc6", beside Back to live; a replay shows the bar too, with "Move 2 of 4" and no Back to live) and, in a narrow
  window, the sheet handle (the sheet widens to fit its buttons). It opens a small menu: vs computer… / Two players…
  → the setup dialogs' `openFrom(position)` as before.
- The More menu's two entries are gone (the owner's choice: the button is the one way in).

### 3.5 Rating (step 5)

**Stored as it goes** (the owner's choice, 2026-11-16, over the first plan's rating computed from the kept
games: games leave the library by the cap and by Delete, and a computed rating would move with them, e.g.
deleting a lost game would raise it). `LocalRating` (`gamelibrary.h`): the value and the number of rated games,
in `games.json` (`"rating": { "value", "games" }`, written once a game is rated; exported with the profile).
`GameLibrary::put` rates a record when it is rateable (kind computer, finished with a result, no fork,
`undoCount == 0`) and not rated yet: `ratingBefore`, `ratingChange` and `rated` on the record, the rating moved.
A rated record put back unfinished (a takeback after the result resumed the game) gives its change back; with
the takeback the game is never rated again. Removing games (the cap, Delete) never changes the rating; old
saves migrated from before BL-19 aren't rated. The robot's rating is `EngineLevel::DisplayElo()` (levels 1–2
are human estimates; they count). Standard Elo: start 1200, `E = 1/(1+10^((Ropp−R)/400))`,
`R' = R + K·(S−E)` (the change rounded), K = 40 for the first 30 rated games, 20 after; provisional under 10
games ("1227?"). Shown in Home's header ("Rating 1227?"), on the Profile page's Statistics card ("Rating vs the
robot", provisional: N of 10), on a Games card ("You won · +27"); the PGN tags the player's rating before a
rated game (`WhiteElo` / `BlackElo`).

### 3.6 Engine analysis (steps 6–7)

`GameAnalyzer` (+ a worker on its own thread, as `EngineWorker`): one `UciConnector` at full strength,
`GetCandidateMoves(position, 1, depth)` for every position 0…n (depth 14 default; Settings → Computer
"Analysis depth" 10/14/18): `evals[i]` from White's view and `best[i]`. Per played move the win-chance drop
(Lichess's formula, `winChance = 50 + 50·(2/(1+e^(−0.00368208·cp)) − 1)`): inaccuracy ≥ 10 %, mistake ≥ 20 %,
blunder ≥ 30 %; accuracy per side (`103.1668·e^(−0.04354·drop) − 3.1669`). Progress and cancel; stored in the
record (once; "Analyse again" replaces it). FakeUci answers MultiPV searches for the tests. UI: "Analyse" on the
result card and in More; a `BusyButton` "Analysing… 23/68"; an eval bar beside the board; `?!` `?` `??` in the
move list (`MoveListModel` roles); a summary row (accuracy, counts per side); "Best was Nf3" for the reviewed
ply. Without an engine the button says so.

**Step 6 as built** (the engine, no UI yet):
- `GameAnalysis` (`gameanalysis.h/cpp`): `evals` (one per position 0…n, White's view; a mate in N ±(100000 − N),
  mated on the board ±100000) and `best` (UCI, "" where the game is over), `depth`. Stored as is on the record
  (`{ version 1, depth, evals, best }`); win chances, judgements and accuracy are worked out when read. The
  thresholds are Lichess's: its 0.1 / 0.2 / 0.3 are on winning chances from −1 to 1, so **5 / 10 / 15 points** of
  the 0–100 win chance (not 10 / 20 / 30 as first written here); a score reads as at most ±1000 cp. Accuracy is
  lila's game accuracy: the mean of the volatility-weighted mean and the harmonic mean of the moves' accuracies
  (each `103.1668·e^(−0.04354·drop) − 3.1669 + 1`, 0–100).
- `GameAnalyzer` + `AnalysisWorker` (`gameanalyzer.h/cpp`): a thread and a fresh full-strength `UciConnector` per
  analysis (no idle engine left), `GetCandidateMoves(position, 1, depth)` per position, asked as the robot is
  (`ChessEnginePlayer::MakeRequest`, the FEN window with Settings' UseFen); a mate or stalemate at the end isn't
  asked. Progress per position; `cancel()` kills the engine and keeps nothing; never waits on the GUI thread (the
  destructor waits at most 3 s). The result goes on the record only if its moves are still those analysed
  (`GameLibrary::setAnalysis`); a change of the moves meanwhile (or another profile, or the game deleted) fails
  it at once. `GameLibrary::put` drops an analysis whose ply count isn't the record's (a takeback, the game went on).
- `ChessConnector`: `analyseGame(id)` ("" = the game on the board, once kept), `cancelAnalysis()`, `Analysing`,
  `AnalysisGameId`, `AnalysisDone` / `AnalysisTotal`, `AnalysisDepth`, `analysisFinished` / `analysisFailed`.
- Settings → Computer **Analysis depth**: Fast (10) / Normal (14) / Deep (18) (`AppSettings.analysisDepth`).
- FakeUci scores its line: `mate 1` when its move mates, else the material after it (deterministic; still one
  line at any MultiPV, so the robot's weak levels stay deterministic in the UI tests).
- Tests: `GameAnalysisTest.cpp` (6 on the numbers, 6 on the analyzer: FakeUci on the scholar's mate, Stockfish,
  cancel, a change meanwhile, nothing to analyse, the record's side), FakeEngineTests +2.

**Step 7 as built** (the UI):
- **Only a game that is over, or a replay** can be analysed (`ChessConnector.CanAnalyse`): the analysis of a game
  going on would hint the moves (and vs the robot, a rated game). A takeback after the mate drops the analysis
  (the moves changed) and the game can't be analysed until it is over again.
- **Analyse**: on the result card (four buttons: two rows of two), in the More menu ("Analyse game", "Analyse
  again" once there is one; not in a Lichess game), and in the moves panel ("Analyse game" over the review
  buttons, for a finished game or a replay). Without an engine they are disabled (the panel's says why). It closes
  the result card and opens the sheet in a narrow window: the progress is in the moves panel ("Analysing… 3 of 8",
  Cancel; a cancel keeps the analysis there was).
- **The summary** (`AnalysisSummary.qml`, the moves panel): "Analysis" and the shown position's eval ("+2.5",
  "#3", "1-0"), a row per side: the player, accuracy %, counts of `?!` `?` `??` in their colours; under them the
  judged move on the board: "Blunder · Best was Bb4" (the engine's best when another move).
- **The move list**: `?!` `?` `??` after the move (Lichess's blue, orange, red: `Theme.inaccuracy`, `mistake`,
  `blunder`; `MoveListModel` roles `wMark` / `bMark`); the review bar and the sheet handle say it too
  ("Viewing 10… Bxc2??").
- **The eval bar** (`EvalBar.qml`) left of the squares: White's share is White's win chance, from White's side of
  the board (turns with it); the eval on hover.
- `ChessConnector`: `CanAnalyse`, `AnalysingThisGame`, `Analysis` ({ depth, white, black }), `ShownAnalysis`
  ({ evalText, whitePercent, mark, judgement, best }); failures say "Couldn't analyse the game. …" (a critical
  message).
- Scenes: `analysis` (a made-up analysis of a kept game, at 10… Bxc2??; also `@1400x900`, `@640x560`); `result` and
  `ended` show Analyse. Tests: `tst_analysis.qml` (4: the result card, progress, marks, eval, review; cancel; a
  takeback drops it; a kept game opened), MoveListModelTest +1.

## 4. Steps

| Step | What | Branch |
|---|---|---|
| 1–2 | This plan; `GameRecord`, `GameLibrary`, migration, the cap, `GameLibraryTest.cpp`; `ChessConnector` writes records, `continueGame(id)`, `openGame(id)`; the old slots removed; `GamesPage` (Open / Continue / Delete), Home's Games card, the Continue list with two-player games, no Save button or leave dialog; the UI tests adjusted | `features/gamesLibraryBackend_131126` ✅ |
| 3 | Library UI polish: filters, Export PGN per card, the PGN tags, Settings "Games kept", scenes `games*` + baselines, `tst_games.qml` | `features/gamesLibraryUi_161126` ✅ (#163) |
| 4 | Play from here: the `…From` starts, the dialogs' `fromPly`, More menu entries, `forkOf` on the card, scene, tests | `features/playFromHere_161126` ✅ |
| 4b | The Play from here button (§3.4.2): review only, the review bar's button and its menu, the More menu entries removed, scenes, tests | `features/playFromHereButton_161126` ✅ (#167) |
| 5 | Rating: `LocalRating` stored as it goes (`GameLibrary::put`), Profile page + Home header, card changes, the PGN's player Elo, tests | `features/localRating_161126` ✅ (#168) |
| 6 | Analysis engine: `GameAnalyzer` + worker, FakeUci MultiPV, storage, unit tests, Settings depth | `features/analysisEngine_161126` ✅ (#169) |
| 7 | Analysis UI: eval bar, judgements, summary, Analyse buttons, scenes, `tst_analysis.qml` | `features/analysisUi_161126` ✅ (#170) |
| 8 | Docs and hand tests: CLAUDE.md, the `run-chess-app` scene list, `BL-19-ManualTests.md`, PROGRESS.md | `docs/bl19ManualTests_161126` |

## 5. Hand tests (the owner, after step 3)

Hand test 9 and the checks of steps 4–7 (Play from here, the rating, the analysis with Stockfish) moved to
[`BL-19-ManualTests.md`](BL-19-ManualTests.md) (scenario 1 is hand test 9).

| # | Check | Expected | Result |
|---|---|---|---|
| 1 | Two players: a few moves, Esc | Home at once, no question; Continue shows the game; Games lists it as unfinished | ✅ 2026-11-16 |
| 2 | Continue it, finish it (mate or resign) | The result card; Games shows it finished with the result; Continue no longer offers it | ✅ 2026-11-16 |
| 3 | vs the robot: a few moves, close the app | Next start: Continue offers it; the robot's level and the clock as left | ✅ 2026-11-16 |
| 4 | A profile with the old `chess.save` and `chess.autosave` (a build before BL-19) | Both in Games after the update, the autosave unfinished and continuable, the old files gone | ✅ 2026-11-16 |
| 5 | Open a finished game | A replay from the start; ‹ › step through; the PGN's Result is right (More → Copy PGN) | ✅ 2026-11-16 |
| 6 | Delete a game | Gone after the confirm; the others stay | ✅ 2026-11-16 |
| 7 | Switch user during a two-player game | "Switch user?" says the game is kept; the other profile doesn't see it; back, Continue has it | ✅ 2026-11-16 |
| 8 | Export a profile, import it as new on another Windows user | Its games come along | ✅ 2026-11-16 |
| 9 | Settings → Data "Games kept" 50 with 51 finished games (step 3) | The oldest finished one goes; unfinished ones stay | → `BL-19-ManualTests.md` §1 |

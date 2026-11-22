---
paths:
  - "ChessGui/ChessGui/*.{h,cpp}"
---

# ChessGui's C++, class by class

The folder's map is `ChessGui/ChessGui/CLAUDE.md`; the QML in `qml/CLAUDE.md`. A ticket ID in brackets says
where the full story is.

## The game: `ChessConnector` (`chessconnector.h/cpp`)
The C++/QML bridge for the chess game: `Q_PROPERTY` bindings and slots that QML calls, **one call per user
action**; it owns the `Game`, the robot, the `MoveGate`, the `GameClock` and the game's record.
- **Starting:** `startComputerGame` / `startLocalGame(…, chess960)` (`StandardGame`, `RandomChess960` or a
  number); `Chess960`, `StartPositionNumber` (`GameTitle` ends "· Chess960 #N", the cards' details begin with
  it). Rematch, continue, replay and Play from here keep the start. `waitForStartFrame`: the window draws
  "Starting the game…" before it builds the game page.
- **Moves:** castling input as lichess.org (`moveTarget`: the king onto its rook, or onto its castling square
  when no move of its own goes there; also a premove). The review and replay positions are replayed from the
  start (`shownGame`).
- **Premove** (LI-05; vs the robot and on Lichess, not two players): during the opponent's turn `figureSelected` /
  `movePiece` set `PremoveFrom` / `PremoveTo` instead of moving (`PremoveDots`: the dots are premove targets;
  `CaptureSquares`: which targets take a piece); played right after the opponent's move if legal then, else
  dropped silently; `cancelPremove()` (right click, Esc).
- **Kept games** (see `GameLibrary`): the record is written with every move, takeback, result and when the game
  is left (`updateRecord`, `keepGameOnLeave`); leaving a local game never asks. `continueGame(id)` resumes an
  unfinished one (the robot at its level, or two players; the clock as left), `openGame(id)` replays any (mode
  `"replay"`); both refuse a record that can't be played (`keptGameFailed`).
- **Play from here** (BL-19 step 4): `PlayFromHere` is `{ gameId, ply, move, whiteToMove }` only while reviewing
  (the shown position of a replay, or of a local game stepped back once it is over; never a game going on), else
  `{}`. `startComputerGameFrom` / `startLocalGameFrom(gameId, ply, …)`: a new game with that game's first moves, a
  fork (`forkOfId`, `forkPly`): those moves aren't taken back and the clock doesn't count them.
- **Rematch:** `rematch()`: on Lichess always the other colour; vs the robot or two players it swaps the player's
  side when `RematchSwapsColours` (`AppSettings.rematchSwapsColours`, on by default; #193), for that game only
  (`applyPlayerSide`, not saved). The setup dialogs offer the side last chosen in them (`AppSettings.computerSide`,
  `twoPlayerSide`).
- **A Lichess game's players** (LI-07, LI-11): both cards show the Lichess players. `PlayerName` is the Lichess
  name (`shownPlayerName()`; the result and the messages use it too), `PlayerTitle` / `OpponentTitle` ("GM",
  "BOT"), `OpponentDetail` the rating ("1523?" provisional), `PlayerDetail` the profile's name (when not the same)
  and the rating; `playerName()` stays the profile's (records, statistics, PGN of kept games). The opponent's
  avatar is their flair (`AvatarProvider::setOpponentOnline`), else the standard `online-player.png` (a pawn
  before a globe, `scripts/make_icons.py avatar`; never in the picker's `avatarNames()`; `opponentFallbackUrl`
  under a flair that loads or fails); the player's own flair, `PlayerFlair`, shows small after the name.
- **Starting on Lichess:** `onLichessGameStarted` starts from the game's variant and start FEN or refuses it
  (`gameStartFailed`); `playJoinedMoves` puts a joined game's moves on the board before the page opens.
  `acceptChallenge(id)` (BL-10): the game on the board stays until the new one starts (a local game is then kept),
  so a refused Accept loses nothing; not during a running real-time Lichess game (`CanAcceptChallenge`);
  `AcceptingChallengeId`: the challenge being accepted, one state for the bar and the Online page.
  `continueOnlineGame(id)` rejoins (`StartingOnlineGame`, `StartingGameId` until it starts, `gameStartFailed`).
  `cancelOnlineStart()` or another profile drops both. `continueOnlyGame()` (Home's Continue): the only
  unfinished game, else false and the list opens; it decides on the last Lichess list while that refreshes, and
  waits for the list only before the first one came (`LichessClient::ongoingGamesLoaded`). `Reconnecting`;
  `switchUser` (also ends a seek).
- **The opponent left:** `ClaimWin` (`claimwin.h/cpp`): Lichess's countdown and "only on the opponent's turn"
  (Lichess refuses it on the player's) behind `OpponentGone`, `ClaimWinSeconds`, `CanClaimVictory`;
  `claimVictory()` posts only then.
- **Analysis:** `analyseGame(id)` / `cancelAnalysis()`, `Analysing`, `AnalysisDone` / `AnalysisTotal`,
  `AnalysisDepth` (Settings → Computer). The board's game: `CanAnalyse` (a kept game that is over, or a replay:
  never one going on), `AnalysingThisGame`, `Analysis` (per side accuracy and `?!` `?` `??` counts),
  `ShownAnalysis` (the shown position's eval, the move's mark, the engine's best: `ShownAnalysis.playedFrom/To`,
  `bestFrom/To`).
- **PGN:** `pgn(gameId)` / `pgnFileName(gameId)` / `exportPgn(file, gameId)`: the game on the board (`""`) or a
  kept one, through `GamePgn`.
- **Debugging the robot:** `robotState()` (logged by a scene that times out, with what it waited for; BL-28).

## Models and helpers of the game
- `BoardPieces` (`ChessConnector.Pieces`): one row per piece that keeps its id while it moves (`BoardDiff`), so
  the board can slide it.
- `MoveListModel` (`ChessConnector.MoveHistory`): one row per move number; a move changes only its row. The
  analysis's `wMark` / `bMark`.
- `MoveGate`: holds the moves from outside (the robot's answer, the Lichess opponent's) and releases them one at a
  time when the board is idle; a Lichess resync of more than two: all at once; in a real-time Lichess game the
  opponent's moves wait only for the page, not the board (`setHoldOnline`, LI-06). `GamePage` reports the board
  settled and the slide time.
- `GameClock` (`ChessConnector.Clock`): the two clocks, as on lichess.org. A local game: `moveMade()`. Lichess's
  times come with `setSnapshot()` and never flag here; a correspondence game shows the time left for the move
  (LI-02). `Board::CanMate` decides whether a flag loses or draws.

## The robot
- `ChessEnginePlayer` wraps `UciConnector` to produce computer moves (`PositionMode`, `MakeRequest`);
  `EngineLevel.h`: levels 1–2 pick among the best moves at full strength (`PickWeakMove`), 3–5 use `UCI_Elo`.
  `EngineWorker` runs it on its own thread.
- The robot's path in the app log: `ENGINE_TRACE` (`engineworker.h`, category `chess.engine`: asked, answered,
  held by the gate, played, the board settled), on in Debug builds and in `--screenshot` mode, else
  `QT_LOGGING_RULES="chess.engine.debug=true"`.

## Kept games, the rating, the analysis
- `GameRecord` and `GameLibrary` (QML singleton `GameLibrary`; BL-19, BL-20): every local game (vs the robot, two
  players) kept from its first move, finished or left, newest first; Lichess games have no record (the owner's
  choice). It follows the active profile. `games`, `unfinishedGames`, `count`, `unfinishedCount`,
  `finishedCount`, `remove(id)`.
  - **Files:** one signed file per game, `users/<id>/games/<gameId>.json` (`{"record", "sig"}`); `put` / `remove`
    write only that file once the current event is done (`flush()` writes now; the destructor and a profile
    change do).
  - **Signature:** `GameSignature` (`gamesignature.h/cpp`): HMAC-SHA256 with one app-wide key and a key number.
    A game file that fails its check (edited, damaged, another game's id) is moved to `games/rejected/`
    (`setAsideCount`, `gamesSetAside`: "N kept games couldn't be verified and were set aside.").
  - **Keeping:** the last `keepCount` finished games (Settings → Data "Games kept", `AppSettings.gamesKept`,
    default 100; a smaller number asks first: `ChessGame.setGamesKept`); it is every profile's:
    `finishedBeyond(keep)` counts them all for the confirm, `setKeepCount` trims every profile's.
  - **Playing kept moves:** `GameRecord::play` / `playOn` (false at a move that can't be played, a move from an
    empty square too).
  - **Chess960:** the record has `startFen` (`variant: "chess960"` in the JSON; `chess960()`, `startPosition()`,
    `replay(plies)` plays its moves from it; QML: `chess960`, `startPosition` number); its PGN has `Variant`,
    `SetUp`, `FEN`.
  - **Migrations, once:** a profile's old `chess.autosave` / `chess.save` (one that can't be read is kept as
    `.damaged`; a Lichess autosave isn't migrated); BL-19's `games.json` (trusted and signed, kept as
    `games.json.migrated`), never beside files of BL-20 (`hasSignedFiles`: a `rating.json`, a game file,
    `games.json.migrated`, `games/rejected/`; it is set aside then). A `games.json` that is there but can't be
    opened is never written over (migrated at a later write, its games merged; the rating waits for it).
- **The rating vs the robot:** `LocalRating` (Elo from 1200, provisional under 10 games), stored as it goes in
  `users/<id>/rating.json` (signed). `put` rates a finished robot game without takeback or fork once (`rated`,
  `ratingBefore`, `ratingChange` on the record) and gives the change back when a takeback resumes it. `rating`,
  `ratedGames`, `ratingProvisional` (Home's header, the Profile page, the cards, the PGN's player Elo).
  - A `rating.json` that fails its check, or is missing while rated games are kept, is set aside in
    `games/rejected/` and restored from the newest rated game (`ratingBefore + ratingChange` and its
    `ratedGames`, else 1200): `ratingRestored`, `ratingRestoredOnLoad`, "Your rating couldn't be verified and was
    restored…".
  - One behind the rated games that continue it exactly (`continued`: `ratingBefore` the rating so far, one more
    rated game each) catches up silently.
- **The analysis:** `GameAnalysis` (`gameanalysis.h/cpp`): per position the eval from White's view and the best
  move, stored on the record (`GameRecord::analysis`); win chances, judgements at 5 / 10 / 15 points and accuracy
  as lichess.org, worked out when read. `GameAnalyzer` (`gameanalyzer.h/cpp`) runs it on a thread with a
  full-strength engine of its own (`AnalysisWorker`); kept only if the record's moves are still those analysed
  (`GameLibrary::setAnalysis`; `put` drops a stale one).
- `GamePgn` (`gamepgn.h/cpp`): a game as PGN, the seven standard tags plus the robot's Elo and `TimeControl`.

## Profiles and app services
- `UserManager`: `users.json` (`backups/users-1..3.json`), version 3 (`kUsersVersion`); statistics per opponent,
  each profile's signed (`statsSig`). An older file is trusted once and written as version 3 at once, never when a
  profile already has signed files. Statistics that fail their check are restored from the newest backup whose
  statistics verify, else rebuilt from the kept games of every profile (`takeRestoredStatistics`,
  `statisticsRestored`). Its tests: `ChessQtTests/UserManagerTest.cpp`.
  - **Export / import** (`.chessprofile`, export format 2): the profile's signed game files (`games/<id>.json`)
    and `rating.json`. An import is checked before it is written (`GameLibrary::checkImport`): a failing game
    file goes to `games/rejected/`, a failing `rating.json` too (restored from the newest verified rated game);
    an old export's `games.json` comes in as signed files without their rating fields (unrated); statistics that
    don't verify are rebuilt from the imported games; `lastImport()` says what was found.
  - **The import's swap** (BL-29): written into `users/<id>.importing` (`AppConfig::writeFileUnflushed`) and
    swapped in by rename, the old folder kept as `<id>.replaced` until `users.json` is saved; `load()` recovers an
    interrupted swap (`recoverInterruptedImports`). An export fails on a game file it can't read
    (`AppConfig::failWritesForTest` for the tests). `[Timing] profile import/export` lines in the log.
  - **Lichess sign-in** through `LichessAuth`; the token is kept here: `lichessToken()`, C++ only. A token this
    Windows account can't decrypt (a data folder copied from another PC) marks the link expired: "reconnect".
    Written and `lichessChanged` once the call that asked is done.
- `AvatarProvider`: the avatars (`avatarNames()`), the opponent's flair.
- `AppInfo`: version, data and log folders, the engine's name. The engine is looked for at start
  (`AppServices::start()`) and again while it is missing (Settings, each game page's moves panel: a first start
  slower than the probe's 5 s isn't final); `engineFound` only once one answered (the Analyse buttons need it).
  `copyToClipboard(text, message)`: `copied(message)` becomes the window's short message.
- `AppConfig`: the data paths; `writeFileAtomically` (QSaveFile) for every file the app writes.
- `AppServices` (`appservices.h/cpp`): the QML services, how they work together (the Lichess token, account,
  ongoing games) and their registration; `main.cpp` and the UI tests' `TestSetup` both use it.

## Infrastructure
- `AppLog` (`applog.h/cpp`): `<data>/logs/ChessPlusPlus_AppDebug.log` (a screenshot scene's beside its picture,
  truncated); every thread writes it under one mutex. It rolls over at 5 MB (at start and while running) to
  `.1.log` … `.9.log`, the oldest dropped; when it can't be moved (another instance has it open) it is appended
  to and tried again later. Lichess's empty keep-alive lines aren't logged.
- `FrameTrace` (`frametrace.h/cpp`): frame timing, off unless `QT_LOGGING_RULES="chess.frames.debug=true"`.
  `GameStartTiming`, always on (`chess.timing`): a `[Timing]` line from the action that starts a game to its
  first frame, with its steps (`ChessConnector` marks them, `GamePage` calls `gameShown`), and the engine stop's
  wait.
- `LayoutGate` (`layoutgate.h/cpp`, QML type): `open` from the first frame's polish pass, once a page's layout has
  settled; costly items behind it (`Loader { active: gate.open }`) are made once at their real size and are still
  in that frame.
- `SceneInput` (`sceneinput.h/cpp`, QML singleton, screenshot mode only): mouse clicks and drags sent to the
  window, for scenes that play as a user does (`play`).

## Invariants
- A game file or `rating.json` that fails its check is never loaded: it goes to `games/rejected/`.
- On import, only `games/<id>.json` names are written under `games/`.
- Exports never carry the Lichess token; it never reaches QML.
- A Chess960 game is never rated; nor is a game with a takeback or a fork; removing games never changes the rating.
- A rated game copied from another profile doesn't move the rating.
- Unfinished kept games are never removed on their own.
- Lichess games have no record here (LI-14 will change this: finished Lichess games kept locally, the owner's
  decision).
- The analysis never runs on a game going on.
- A premove that isn't legal when its turn comes is dropped silently.

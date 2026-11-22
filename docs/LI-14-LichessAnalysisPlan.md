# LI-14 — Analyse a Lichess game: plan

The owner's decision (2026-11-22): **finished Lichess games are kept locally**, in the games library, and
analysed there as local games are (option (b) of the ticket). This plan says how. Line numbers are from
`develop` at 1e4cdfa.

## What is there today

- The analysis (`GameAnalyzer`, BL-19 steps 6–7) works on a **record** in `GameLibrary`: `analyseGame(id)`
  analyses the record, `GameLibrary::setAnalysis` stores it on the record, and `refreshBoardAnalysis()` shows it
  on the board (the marks, `EvalBar`, `MoveArrows`, `AnalysisSummary`).
- A Lichess game has no record. `ChessConnector::updateRecord()` returns at once for an online game
  (`chessconnector.cpp:2392`), and `refreshBoardAnalysis()` doesn't count one as kept (`:1390`). So `CanAnalyse` is
  false, and the result card's **Analyse** (`analyseResultButton`) and the More menu's are hidden.
- The QML needs no change for Analyse itself. The result card, the More menu and the moves panel follow
  `ChessConnector.CanAnalyse`, `AnalysingThisGame` and `Analysis`.
- What we know about a Lichess game when it ends: `gameFull` (players with id, name, title, rating,
  provisional; `rated`, `speed`, `clock` or `daysPerTurn`, `variant`, `initialFen`, `createdAt`), the moves, and
  the result (`onLichessGameEnded`, or a mate found on our board). The players' rating changes aren't in the
  Board API's stream.

## 1. The record of a Lichess game

A finished Lichess game gets a `GameRecord` like a local game, with:

- **`kind: "lichess"`**, beside `"computer"` and `"local"`.
- **`id: "lichess-<gameId>"`.** The Lichess id is 8 letters and digits, so the id names a file
  (`GameLibrary::isFileId`). The same game is never kept twice: a game joined again after it ended, or LI-15's
  list, finds its record by this id.
- **The usual fields:** `playerPlaysWhite`, `opponentName` (the Lichess name), `startedAt` (`createdAt`),
  `finishedAt`, `moves`, `fen`, `result`, `resultText`, `startFen` (Chess960), `clockLimitSec` / `clockIncSec`.
  `resultRecorded: true`, because the statistics have it (`recordResult` counts it under "Lichess").
- **A new `lichess` object** in the JSON, read and written by a small class `LichessGameInfo` (OO style:
  `fromJson`, `toJson`, getters):
  - `gameId`, `rated`, `speed` ("rapid", "classical", "correspondence", …), `daysPerTurn`;
  - `white` and `black`: the players **as `gameFull` gives them** (`id`, `name`, `title`, `rating`,
    `provisional`, `aiLevel`), so `LichessPlayer::fromJson` reads them back as it is;
  - the opponent's flair URL, when it came (LI-11).
- **Never:**
  - rated here: `rateable()` is only for `"computer"`, so nothing changes there;
  - continued: `continueGame` refuses a finished record already;
  - Played from here (the owner's decision, in the ticket): `PlayFromHere` gets one more condition.
- **The local rating's fields** (`rated`, `ratingBefore`, `ratingChange`) stay the local rating's. Lichess's
  `rated` lives in the `lichess` object, so the two never mix.

**Where it shows up beyond the board:**

- **Statistics rebuilt from kept games** (`UserManager`, when the statistics fail their check): a `"lichess"`
  record counts under the `lichess` key. Today it would count as a Guest game (`UserManager.cpp:673`).
- **PGN of the record** (`GamePgn::ofRecord`):
  - `Site https://lichess.org/<id>`;
  - the Lichess names (not the profile's);
  - `WhiteElo` / `BlackElo` from the ratings, `TimeControl`, and Chess960's `Variant` / `SetUp` / `FEN`.
- **"Games kept":** Lichess games count among the finished games of the cap (`keepCount`), with no cap of their
  own. Settings' line ("Every game you play here is kept in Games") stays true.
- **Export / import:** game files are game files, so the format doesn't change. An older Chess++ reading a
  `"lichess"` record shows it as a two-player game. That is acceptable for a downgrade.

## 2. When it is written

**When the result comes, on the board.** This is the one place that sees every finished Lichess game:

- `onLichessGameEnded` (resign, time, draw, …);
- a mate found on our board (`endGameWithLoser` → `setResult`).

`updateRecord()` writes it: an online game passes only once it has a result. Not written:

- **an aborted game** (`*`): Lichess doesn't count it either;
- **a game left before its end:** Leave during a running game, or a resignation without an answer within 5 s
  (`_leaveAfterResign`). Its result never reaches us. LI-15's list can bring it later;
- **a game the player never had on the board here:** played on lichess.org, or a correspondence game that ended
  meanwhile. That is LI-15.

A game joined after it ended (`continueOnlineGame`: Lichess sends `gameFull` with the result) is written then,
under the same id: no duplicate.

**Why not only when it's analysed:** the owner wants it to survive a restart and show on the Games page, also
without Analyse. One write at the end is also the simplest place.

## 3. Analyse on the board

- `refreshBoardAnalysis()`: an online game with a record (`_recordId`) and a result counts as kept. `CanAnalyse`
  then turns on by itself once the game is over, never while it is played (the invariant "no hints"). This
  brings back the result card's **Analyse** and More's **Analyse**.
- `analyseGame("")` analyses `_recordId`, as today. The analysis is stored on the record and shown as for a
  local game: progress, marks, accuracy, eval bar, best-move arrows.
- **Rematch, or the next game:** `beginGame` drops `_recordId` as today, so the new game starts without one.
- **Chess960:** the record has `startFen`, and `GameAnalyzer` replays from it (`replay()`), as for a kept
  Chess960 game.

## 4. The Games page and opening a kept Lichess game

- **Filter:** a **Lichess** filter between Two players and Unfinished.
- **The card's words** (`games.js`):
  - title: the opponent's name with their title ("GM Hikaru");
  - details: `Rapid 10+5 · Rated · you play White · 22 Nov 2026 · 34 moves` (Chess960 first, as now);
  - result: "You won" / "You lost" / "Draw", as a robot game's, since the player's side is known.
- **The card's actions:**
  - **Open:** a replay, as for any kept game;
  - Export PGN and Delete, as now;
  - **Open on lichess.org** and **Copy link** in its right-click menu, as `OngoingGameCard`'s (LI-10);
  - no Continue, no Play from here.
- **Opening it** (`openGame`): the replay shows the Lichess players as the live game did (LI-07): names,
  titles, ratings, the opponent's flair or the standard Lichess avatar. Today `shownPlayerName()` and the
  title, detail and flair getters ask `isOnlineGame()`. They will ask "a Lichess game on the board, live or
  replayed" (one helper), fed by the record's `lichess` object.
- **Home's header and the Profile page** are about the robot rating: unchanged.

## 5. Tests

- **`ChessQtTests`:**
  - `GameLibraryTest`: a `"lichess"` record through JSON and its signed file (`LichessGameInfo` in and out); it
    isn't rated; it counts in the cap.
  - `UserManagerTest`: statistics rebuilt from a `"lichess"` record count under Lichess.
  - `GamePgnTest` (or where the PGN is tested): Site, names, Elo, TimeControl, Chess960.
- **`tst_lichess`:**
  - the game of test 8 (the opponent resigns) is kept: `GameLibrary` has `lichess-game0001`, finished, kind
    lichess;
  - its result card shows **Analyse**; FakeUci analyses it; the marks show on the move list;
  - **Analyse** isn't there during the game;
  - an aborted game isn't kept.
- **`tst_lichess960`:** a Chess960 Lichess game is kept with its `startFen`, and analysed.
- **`tst_games`:** the Lichess filter, the card's words, Open replays it with the Lichess names, no Continue.
- **`tst_restart`:** a kept Lichess game is still there after the restart. Could be in the same test file run
  twice.
- **New scenes** (none shows a Lichess game's end today):
  - `lichess-result`: a Lichess game just ended, its result card with Rematch and Analyse (step 2);
  - `games-lichess`: the Games page with the Lichess filter and a kept Lichess game's card (step 3).

## Steps (one PR each)

| Step | What | Tests |
|---|---|---|
| 1 | `kind: "lichess"`, `LichessGameInfo`, the id `lichess-<id>`, statistics rebuilt, PGN of the record, no Play from here | ChessQtTests |
| 2 | Kept when the result comes; Analyse on the board (`refreshBoardAnalysis`) | `tst_lichess`, `tst_lichess960`, new scene `lichess-result` |
| 3 | The Games page: Lichess filter, card words and menu; `openGame` shows the Lichess players | `tst_games`, `tst_restart`, new scene `games-lichess` |

Step 2 is what the owner asked for after the Rapid games: Analyse at the end of a Lichess game. Step 3 makes the
kept games findable later.

## Related tickets

- **LI-15** (recent games from the server): a game already kept here opens its record (`lichess-<id>`). One that
  isn't is replayed from the server's data. Whether Analyse on such a game keeps it (as a record, the same
  shape) is LI-15's to decide; this plan makes it possible.
- **LI-16** (the end of a Lichess game): its **Analyse** is step 2's.
- **The invariant** "Lichess games have no record here" (`.claude/rules/chessgui-cpp.md`) becomes: "Only
  finished Lichess games have a record (`kind: "lichess"`, id `lichess-<id>`), written when the result comes
  here; never rated, continued or Played from here."

## For the owner to decide

1. **Aborted games:** not kept (recommended), as Lichess doesn't count them.
2. **The cap:** Lichess games under the same "Games kept" number (recommended), or a cap of their own.
3. **A game left before it ended** isn't kept: its result never reaches us. LI-15 lists it later. Fine, or
   should a left game ask Lichess for its result when it ends?
4. **Lichess's own analysis:** not part of LI-14. Our engine analyses, as for local games. A link "Open the
   analysis on lichess.org" (`https://lichess.org/<id>/analysis`) in a kept Lichess game's card menu and the
   More menu would be cheap to add in step 3. Wanted?
5. **The players' rating changes** aren't in the Board API's stream, so the record keeps the ratings from
   before the game. Enough, or should step 3 fetch the change (`GET /game/export/<id>`) when the card shows?

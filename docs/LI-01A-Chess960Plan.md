# LI-01A Chess960 — local first (vs the robot and two players), then on Lichess

| | |
| :---- | :---- |
| **Date** | 2026-11-17 |
| **Status** | Stage 1: step 1 (the rules) merged (#175); step 2 (the engine) merged (#176); step 3 (the library, the connector, castling input, `tst_chess960`) merged (#177); step 4 (the UI) on `features/chess960Ui_171126`; step 5 not started |
| **Branch** | each step on its own branch from `develop` |
| **Task** | LI-01A in `TASKS.md` (LI-01 split by the owner, 2026-11-17: A Chess960, B Crazyhouse) |
| **Rules** | <https://lichess.org/variant/chess960> |

## 1. What there is (reviewed 2026-11-17)

- **Only the standard start.** `Board::Initialize()` lays out RNBQKBNR. Every replay starts there:
  - `Game::Restart` / `LoadMoves` / `ResumeFromLoad` / `FormatHistorySan`;
  - `ChessEnginePlayer::Replay` (the engine's FEN windows, the analysis);
  - `GameRecord::play` / `playOn` (the games library, its previews, PGN and SAN).
- **Castling is hard-coded** for a king on e1/e8 and rooks on a/h, in six places:
  - `MoveGeneration::GenerateAdvancedMoves`: rook offsets −4/+3, the king's two steps;
  - `MoveGeneration::AddComplementalMove`: "the king moves two squares" adds the rook's move;
  - `Game::UndoMove`: the rook goes back by the same offsets;
  - `Game::CanCastleKingside` / `CanCastleQueenside`: e/h and e/a, for the FEN;
  - `Game::GetPremoveTargets`: home e1/e8;
  - `FormatMoveSan`: O-O by the king's direction.
- **Castling is checked as a king-only move.** `ExcludeCheckMoves` plays a castling as the king's move alone on
  its test board (the rook stays). In standard chess that is harmless; in 960 it is not (§3.4).
- **The engine is asked from the standard start:** `position startpos moves …`, or a FEN window
  (`PositionMode`), and `UCI_Chess960` is never set.
- **Lichess refuses non-standard games here**: `playableHere` (`LichessClient.cpp`) and the challenge form
  (`OnlinePage.qml`: "Standard only … LI-01"). Nothing is broken in the meantime.

## 2. The owner's decisions (2026-11-17)

1. **LI-01 is split.** LI-01A is Chess960, LI-01B Crazyhouse (later, a plan of its own).
2. **Chess960 in two stages.** First local play: vs the robot and two players, kept in the games library
   (continue, replay, Play from here, analyse, PGN). Then the same on Lichess (stage 2, §6).
3. **Castling as on Lichess.** The king dropped or clicked on its own castling rook always castles. The king's
   destination (g/c file) also gets a dot when that square isn't an ordinary king move.
4. **Not rated.** A 960 game vs the robot never changes the local rating (BL-19 step 5). It is kept, continued,
   replayed and analysed like any other game. A 960 rating can come later.
5. **Positions.** A new game gets one of the 960 at random, shown as "Chess960 #N" (Scharnagl numbering, the
   one Lichess shows; #518 is the standard set-up). **Rematch** replays the same set-up with the colours
   swapped; **New game** draws a new one.

## 3. Design

### 3.1 `Chess::StartPosition`

A new class in ChessCore (`StartPosition.h/cpp`; classes with const accessors, as the project's C++ style asks):

| Member | |
|---|---|
| `static Standard()` | #518, RNBQKBNR |
| `static Chess960(int n)` | 0–959, Scharnagl's numbering (bishops, queen, knights from `n`; then R K R on the three squares left) |
| `static Random()` | one of the 960 |
| `static FromFen(fen)` | a start position only: the back rank mirrored, eight pawns on each side, White to move, all four castlings. Empty when it isn't one. FakeUci needs it now, Lichess's `initialFen` in stage 2 |
| `Number()`, `IsChess960()` | `IsChess960()` is a property of the game, not of the set-up: #518 played as Chess960 still castles as 960 (Lichess does the same) |
| `BackRank()` | the eight pieces, a–h |
| `KingFile()`, `RookFile(bool kingside)` | the castling squares |
| `Fen()` | the start FEN, castling field `KQkq` (X-FEN) |

No general FEN input is needed. A fork (Play from here), a continued game and a replay all play their moves from
the start position.

### 3.2 The castling move

- **In a 960 game, castling is "king onto own rook"**: `e1h1`, `b1a1`, `g1h1`. This is what Stockfish with
  `UCI_Chess960` and the Lichess Board API use. It is never ambiguous, also when the king doesn't move
  (king g1, rook h1: O-O moves only the rook).
- **A standard game keeps `e1g1`.** `games.json`, the tests, the analysis keys (`movesKey`) and every existing
  record stay as they are.
- One generator serves both games. Standard is the case king on e, rooks on a and h; the move it emits has the
  game's form (king's destination or the rook's square).

### 3.3 The rule

Castling on one side is legal when:
- the king and that side's castling rook have never moved (`MoveGeneration::IsEverMoved` already works from any
  square);
- every square between the king's start and its destination, and between the rook's start and its destination,
  is empty, apart from the king and that rook;
- the king isn't in check, and no square it crosses or lands on is attacked;
- after it, the king is on g1 and the rook on f1 (O-O), or the king on c1 and the rook on d1 (O-O-O); the same on
  the 8th rank for Black.

`GameState` carries the start position, so the generator knows the rook files.

### 3.4 Playing it in one step

- A new **`Board::DoCastling(kingFrom, rookFrom, kingTo, rookTo)`** lifts both pieces and places both. It takes
  one `_previousBoard` snapshot (so `BeforeLastMove()` is the position before the castling), and handles the king
  and rook swapping squares (king f1, rook g1) and either piece staying put.
- `Game::DoMove`, `Game::UndoMove` and `ExcludeCheckMoves`' test board all use it. The test board then sees the
  rook move too. Example: king b1, rook a1, a black rook on a8. After O-O-O the king is on c1 and the a-file
  opens, which is fine. But with the white rook on b1 between the king on c1 and an attacker on a1, after O-O-O
  (king stays on c1, rook to d1) the king is in check from a1. Only a full simulation catches that.
- `AddComplementalMove` keeps only en passant. Its "the king moves two squares" branch would misfire on a 960
  king-onto-rook move that is two squares long (king c1, rook a1).

### 3.5 History, SAN, FEN, the board

- **`HistoryMove`**:
  - `IsCapturingMove()` is false when `To.Piece` is the mover's own colour (the rook);
  - `IsCastlingMove()` knows both forms;
  - a helper gives the king's and the rook's destination squares, for the undo, SAN and the board.
- **SAN**: O-O or O-O-O by which rook castled, not by the direction the king went (it may not move at all).
- **FEN**: the castling field is `KQkq` (X-FEN), which Stockfish and Lichess both read. A rook's file letter
  (`HAha`, Shredder style) is used only when the castling rook of one side is ambiguous (a second rook on the
  same side of the king on the back rank).
- **The board's animation**: `BoardDiff` matches pieces by type, so the king and rook slide to their squares, the
  swap included. Two `BoardDiffTests` check it; no QML change is expected.
- **Premove targets** (`GetPremoveTargets`): the castling rook's square and the king's destination.

### 3.6 The engine

- `UciConnector` sends `setoption name UCI_Chess960 value true|false` at each new game, when the engine lists
  the option (Stockfish does).
- A 960 game always asks `position fen <start FEN> moves …` (`StartPosMoveRequest.Fen` = the start FEN), in
  every `PositionMode` (done so in step 2, instead of FEN windows with X-FEN): the engine knows the castling
  rooks from the start, never from a mid-game FEN's castling field, and FakeUci needs only the start.
- `ChessEnginePlayer::MakeRequest` / `Replay` take the start position; the analysis (`GameAnalyzer`,
  `AnalysisWorker`) passes it on. The weak-move picker (levels 1–2, `GetCandidateMoves`) is unchanged.
- FakeUci learns the `UCI_Chess960` option and `position fen` of a 960 start (`StartPosition::FromFen`); its
  first-legal-move picker castles in the king-onto-rook form.

### 3.7 The games library and ChessConnector

- **`GameRecord`**:
  - two new fields, `variant` (`"standard"` / `"chess960"`) and `startFen` (960 only). A record without them is
    standard, so old files load as they are;
  - `play` / `playOn` / `setMoves` replay from the start position;
  - `rateable()` is false for 960 (§2.4).
- **`GamePgn`**: `[Variant "Chess960"]`, `[SetUp "1"]`, `[FEN "<start>"]`. Lichess's analysis board and other
  programs read these.
- **`ChessConnector`**:
  - `startComputerGame` and `startLocalGame` gain `chess960Number`: −2 is standard (the default), −1 is a
    random 960, 0–959 a given one (the tests and screenshot scenes);
  - Rematch keeps the start position;
  - `continueGame`, `openGame` and `start*From` (Play from here) take the record's start position;
  - two new properties, `Chess960` and `StartPositionNumber`.
- **Castling input** (`figureSelected`, `movePiece`, premove):
  - with the king selected, a click or drop on its own castling rook castles, instead of selecting the rook;
  - the dots go on that rook, and on the king's destination when that isn't an ordinary king move;
  - a click on the destination plays the king-onto-rook move.

### 3.8 The UI

- `ComputerGameDialog` and `TwoPlayerDialog`: a **Variant** row, Standard / Chess960 (`ChoiceRow`), remembered in
  `AppSettings`. It is hidden in Play from here, which keeps the game's start.
- Game page: "Chess960 #N" with the players (done in step 4 as part of `ChessConnector.GameTitle`: the command
  bar and the window title). The result card's **New game** reopens the dialog with Chess960 chosen (a new
  position); **Rematch** keeps the position.
- `GameCard` / `games.js`: "Chess960" in a card's description, on the Games page and in the Continue list.
- The board preview (`BoardPreview`) reads the record's `fen`: no change.

## 4. Steps

Each step is one branch and one PR from the latest `origin/develop`.

| Step | Branch | What |
|---|---|---|
| 1 | `features/chess960Rules_…` | ChessCore: `StartPosition`, `Board::Initialize(start)`, `Board::DoCastling`, the generalised castling (§3.3–3.5), `Game::Restart(whiteFirst, start)` / `GetStartPosition()`, `FormatHistorySan(history, start)`. No UI |
| 2 | `features/chess960Engine_…` | §3.6: `UCI_Chess960`, the start FEN in every request, the analysis, FakeUci |
| 3 | `features/chess960Library_…` | §3.7: `GameRecord`, `GamePgn`, `ChessConnector` (start, rematch, continue, replay, fork, the castling input) |
| 4 | `features/chess960Ui_…` | §3.8: the dialogs' Variant row, the page's "Chess960 #N", the cards; screenshot scenes; `tst_chess960` |
| 5 | `docs/chess960ManualTests_…` | CLAUDE.md, `LI-01A-ManualTests.md`, `TASKS.md`, `PROGRESS.md` |

## 5. Tests

**`ChessTests/Chess960Tests.cpp`** (new, step 1):
- numbering: #518 is RNBQKBNR, #0 BBQNNRKR, #959 RKRNNQBB. For all 960: two bishops on opposite colours, the
  king between the rooks, all different.
- castling:
  - king b1 / rook a1, O-O-O: king c1, rook d1;
  - king g1 / rook h1, O-O: only the rook moves (to f1);
  - king f1 / rook g1, O-O: the two swap;
  - blocked by a piece on the rook's path, not on the king's;
  - the king crosses an attacked square; the king is in check;
  - the rook has moved and come back;
  - the shielding rook (§3.4);
  - undo restores the position and the castling rights.
- FEN castling field, SAN (O-O / O-O-O), UCI strings, `LoadMoves` replay of a 960 game.
- `BoardDiffTests` +2 (the swap, the king staying put). Every existing ChessTests test passes unchanged.

**ChessQtTests** (steps 2–3):
- `UciConnectorTest`: Stockfish in 960, its move (castling included) is legal in our `Game`;
- `FakeEngineTests` +2 (a 960 start, castling);
- `GameAnalysisTest`: a 960 game analysed with FakeUci;
- `GameLibraryTest`: the record round trip, not rated, replay;
- `GamePgnTest`: the 960 tags;
- `ChessEnginePlayerTest`: the request's start FEN.

**UI** (step 4):
- scenes `chess960` (a fixed #) and `chess960-castled` (after the f1/g1 swap), the dialogs' new row (their
  baselines updated);
- `tst_chess960.qml`: a two-player 960 game at a fixed number, castling by dropping the king on the rook and by
  clicking the destination, a takeback; vs the robot (FakeUci) until it castles; the game in the library,
  replayed.

**By hand** (step 5, `LI-01A-ManualTests.md`): Stockfish 960 at levels 1 and 5, both castlings; Takeback; leave
and Continue; replay; Analyse; Export PGN, then paste it into lichess.org's analysis board to confirm the moves
and the castlings read the same.

## 6. Stage 2: Chess960 on Lichess (a later plan)

The rules work of stage 1 is all Lichess needs. What stays:
- start a Lichess game from `gameFull.initialFen` (`StartPosition::FromFen`); the stream's castling moves already
  have the king-onto-rook form;
- `playableHere` and `canPlayHere` allow `chess960`; the challenge form offers it again (`OnlinePage.qml`);
- incoming 960 challenges (BL-10) can be accepted;
- the `chess960` rating pool in `parseRatings` and on the Profile page;
- rejoin (LI-04) of a 960 game.

## 7. Risks

- **Six hard-coded castling places** (§1): all go through one generator and `DoCastling`. The existing tests
  (castling, undo, SAN, FEN, premove, BoardDiff) guard standard chess.
- **`IsCapturingMove()` sees the king-onto-rook move's own rook**: fixed in `HistoryMove` (§3.5), so the
  captured pieces, sounds and SAN never take it for a capture.
- **Old records and analyses**: unchanged, because a standard game keeps `e1g1` and a record without `variant`
  is standard.

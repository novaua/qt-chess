# BL-04 Pieces That Move — Implementation Plan

| | |
| :---- | :---- |
| **Date** | 2026-10-30 |
| **Status** | Steps 1–8 done (`Game::UndoMove` fix, `BoardDiff`, `BoardPieces` + `ChessConnector`, the QML board on `Pieces`, slides and fades, dragging, the `drag` scene); manual check of dragging open |
| **Branch** | `pieceDrag_301026` (from `develop`) |
| **Task** | BL-04 in `TASKS.md` |

## 1. Problem

Pieces don't move on the board; they disappear and reappear.

- `ChessFigures.qml` is a `Grid` of 64 square delegates over a `ListModel` (`EmptyBoardModel`). A move
  reaches it as `ChessConnector::boardChanged(square, piece)`, one signal per changed square (2 for a
  move, 4 for castling, 64 for `refreshBoard()` and every review step). Each signal **removes and
  re-inserts** the square's row, so its delegate is destroyed and created again, and fades in over
  450 ms. A knight going g1–f3 is "g1 fades out, f3 fades in".
- There is no notion of a piece with an identity ("the g1 knight is now on f3"), so there is nothing
  to slide, and nothing to drag.
- Input is click-select-click: a `MouseArea` on each of the 64 squares calls
  `ChessConnector.figureSelected(square)` — the first click shows the legal-move dots, the second
  moves. Dragging a piece does nothing.
- The board is turned for Black by three stacked `Grid`s, each mirrored with an X-axis `Rotation`,
  with a counter-rotation on every delegate (`rotation: 180 + chessBoard.angle` plus a Y-axis
  `Rotation`). It works, but square ↔ pixel mapping is hard to follow, which matters for dropping.

Found while reading the code for this plan:

- **`Game::UndoMove()` only undoes the main move.** It moves the piece on `To` back to `From` and puts
  back a piece captured on `To`. It does not move the castling rook back, does not restore a pawn
  taken en passant (that pawn is never pushed on `_captured`: `IsCapturingMove()` is false for en
  passant), and leaves the promoted piece on the 7th rank instead of a pawn. It is used by takeback
  against the robot (`ChessConnector::movePrev`) and by stepping back in a loaded game
  (`HistoryPlayer::MoveBack`). Review (←/→ in a live game) is not affected: it replays the moves.
  No test covers it. Takeback is one of the moves that will slide, so this is fixed first (step 1).
- Each `boardChanged` emission triggers a `ListModel` remove + insert, i.e. a delegate (Item, Image,
  Text, MouseArea, Rotation, PropertyAnimation) built from scratch: 64 of them when `GamePage` is
  created (`refreshBoard()`) and on every review step. This is likely part of the slow game page
  load in BL-11; the piece model below removes it.

## 2. Goals

| | |
|---|---|
| **Slide** | A piece glides from square to square (`Theme.slow`, 250 ms, `OutCubic`) for every single move, whoever made it: the player (click or drag), the robot, a Lichess opponent, a takeback, one review step (← / →), a step in a loaded game. Castling slides king and rook; en passant and captures fade the taken piece out; promotion slides the pawn and turns it into the new piece. |
| **No slide** | A position that changes a lot at once — new game, Continue, load, review jump (move list click, Home/End), Lichess resync of many moves — appears at once (short cross-fade). |
| **Drag** | Press a piece of the side to move and drag it: it follows the pointer above the other pieces, the legal-move dots show, the square under it is outlined; dropping on a legal square plays the move, anywhere else (illegal square, off the board, Esc) slides it back. Works with mouse and touch. |
| **Click** | Click-select-click stays exactly as today; a press without a drag is a click. |
| **Rules** | No dragging when the player can't move: robot thinking, Lichess opponent's turn, review, loaded game, promotion choice open, game over. The rules stay in `ChessConnector` (`figureSelected` already has them). |
| **Reduce animations** | `Theme.reduceMotion` (setting, Windows, screenshot mode): no slides or fades; dragging still works. |

Out of scope: premoves, arrows/marking squares, keyboard play on the board, piece sets, sounds
(they stay where they are), the Lichess takeback resync (see §7).

## 3. Design

### 3.1 ChessCore: which piece went where — `BoardDiff`

A class in `Chess::` (`BoardDiff.h`), no Qt, tested in `ChessTests`. The constructor compares the two positions; the result is read-only:

```cpp
class PieceTravel {      // one piece from a square to another; ChangesType(): a promotion or its takeback
public:
    BoardPosition From() const; BoardPosition To() const;
    const Piece& Before() const; const Piece& After() const; bool ChangesType() const;
};
class BoardDiff {
public:
    BoardDiff(const Board& before, const Board& after);
    const std::vector<PieceTravel>& Moved() const;     // includes promotions (and their takeback)
    const std::vector<PositionPiece>& Removed() const; // captured, en passant
    const std::vector<PositionPiece>& Added() const;   // a captured piece put back by a takeback
    size_t Size() const;
};
```

1. Squares with the same piece before and after are left alone.
2. Every piece that **vanished** from a square is matched with a piece of the same kind and colour
   that **appeared** elsewhere, nearest first. One move gives one pair; castling two.
3. A pawn that vanished from the 7th rank (2nd for Black) and a piece of its colour that appeared on
   the 8th (1st) one file away at most → a promotion. The reverse (takeback) → a piece turning back
   into a pawn. The same square (the promotion choice after a dropped pawn) → only the type changes.
4. What is left: vanished → `Removed` (fade out), appeared → `Added` (fade in).

Comparing two positions, not moves, means it needs no hint and works the same for every path that
changes the board (play, engine, Lichess, takeback, review, loaded game). The caller decides "too
much changed": more than 4 entries in total → no slide, the position is shown as new.

### 3.2 ChessGui: the pieces model — `BoardPieces`

`BoardPieces : QAbstractListModel` (`boardpieces.h/cpp` in ChessGui, compiled into `ChessQtTests`
like `UserManager`), one row per piece on the board:

| Role | |
|---|---|
| `pieceId` | stable for the piece's life on the board |
| `square` | 0 = a1 … 63 = h8 |
| `piece` | `"N"`, `"n"`, … (as `boardChanged` sends today) |
| `taken` | true while a captured piece fades out |

- `sync(const Board& shown)` compares `BoardDiff(_lastShown, shown)`: `Moved` → `dataChanged` on
  `square` (and `piece`); `Removed` → `taken = true`; `Added` → new rows. Too much changed →
  `beginResetModel()` with fresh ids. Rows marked `taken` stay until the next reset (at most one
  per capture), so a capture still fades when the robot answers within milliseconds.
- `ChessConnector` exposes it as `Q_PROPERTY(QAbstractItemModel* Pieces CONSTANT)` (instead of
  `boardChanged` + `refreshBoard()`, which go away).

**One sync per change.** `ChessConnector`'s board listener (`_game->RegisterBoardChanged`) no longer
emits per square: it marks the board dirty and posts one queued `syncPieces()` (if not already
posted). So castling's four `Place` calls, `ResumeFromLoad()`'s replay, an `applyMoves()` batch, a
takeback of two moves all end in one comparison of the before/after positions. `emitBoardState()`
(review) calls the same `syncPieces()`. `syncPieces()` takes the **shown** position: the reviewed
one while reviewing, the live game otherwise — the same choice `checkedKing()` makes; that choice
moves into one helper used by both.

### 3.3 ChessConnector: input

- `Q_INVOKABLE void figureSelected(int square)` — unchanged (click).
- `Q_INVOKABLE bool movePiece(int from, int to)` — a drop. Same checks as `figureSelected`, then
  `makeMove(from, to)` if `to` is a legal target of `from`; returns false (the piece slides back)
  otherwise. Clears the dots either way. One call per user action.
- `Q_PROPERTY(int SelectedSquare …)` — the selected piece's square or -1, so the selection tint
  moves to `ChessBoxes` (today a QML-only `selectedIndex` on the piece delegate).
- `Q_PROPERTY(int MovableColor …)` — 0 none, 1 White, 2 Black: whose pieces can be pressed now.
  Computed from the same conditions as `figureSelected` (one helper `canPlayerMove()` used by both),
  notified on move count, engine thinking, review, promotion, game over, online game changes.

### 3.4 QML

**One geometry.** `ChessBoard.qml` gets `squareSize` and `function squareX(sq)` / `squareY(sq)`: a1
at the bottom left in the board's own coordinates (`x = file * size`, `y = (7 - rank) * size`), and
`squareAt(x, y)`. The board keeps its turn for Black (`rotation: angle` with the existing animation);
pieces, dots and labels inside counter-rotate (`rotation: -chessBoard.angle`) to stay upright. The
X/Y-axis `Rotation` transforms on `ChessBoxes`, `ChessFigures`, `NavigationLayer` and their delegates
go away; `ChessBoxes` and `NavigationLayer` keep their `Repeater { model: 64 }` but place delegates
with `squareX/Y` instead of a mirrored `Grid`.

**`ChessFigures.qml`** — an `Item` with `Repeater { model: ChessConnector.Pieces }`:

```qml
delegate: Item {
    required property int pieceId; required property int square
    required property string piece; required property bool taken
    x: chessBoard.squareX(square) + dragOffset.x
    y: chessBoard.squareY(square) + dragOffset.y
    z: dragHandler.active ? 10 : 1
    opacity: taken ? 0 : 1
    Behavior on x { enabled: slides; NumberAnimation { duration: Theme.slow; easing.type: Easing.OutCubic } }
    Behavior on y { … }                 // the same
    Behavior on opacity { enabled: !Theme.reduceMotion; NumberAnimation { duration: Theme.normal } }
    Image { … source from piece, sourceSize set to the square size … }
    Text { "⚑" when square === ChessConnector.FlaggedKing }
    DragHandler { id: dragHandler; target: null; enabled: owns(piece) … }
}
```

- `slides` = `!Theme.reduceMotion && !dragHandler.active && !justDropped`. A dropped piece is
  already over its square, so it doesn't slide there again; a refused drop sets `dragOffset` back to
  0 with the Behavior on, so it slides home.
- New rows (a model reset, a piece put back) fade in over `Theme.fast`; a reset cross-fades the
  board instead of 32 separate fades.
- `DragHandler` with `target: null` (the piece's `x/y` stay bound to its square; `dragOffset` follows
  `translation`), `dragThreshold` default, so a press-and-release is still a click.
  - `onActiveChanged` true → `ChessConnector.figureSelected(square)` if it isn't selected yet (dots
    show), cursor `Qt.ClosedHandCursor`.
  - false → `ChessConnector.movePiece(square, chessBoard.squareAt(centre))`; false → slide back.
  - Esc while dragging, or the pointer leaving the window → cancel (slide back).
- `HoverHandler` on movable pieces: `Qt.OpenHandCursor`.

**Clicks:** one `TapHandler` on the board (in `ChessBoard.qml`, under the pieces) →
`ChessConnector.figureSelected(squareAt(point))` — replaces the 64 `MouseArea`s, so empty squares
(targets) are clickable as before.

**`ChessBoxes.qml`** also shows the selection tint (`SelectedSquare`) and, while dragging, an
outline on the square under the piece (`chessBoard.dropSquare`).

**Promotion:** unchanged. A pawn dropped on the last rank stands there while
`PawnPromotionOptions` is open; the choice turns it into the piece in place (§3.1, rule 3).

**Removed:** `EmptyBoardModel.qml`, `ChessConnector::boardChanged`, `refreshBoard()`, the
`chessfigureSelected` signal, the per-delegate fade/rotation animations.

## 4. Implementation steps

1. **`Game::UndoMove` fix** (ChessCore): castling puts the rook back, en passant puts the pawn back
   (pushed on `_captured` in `DoMove`), promotion turns back into a pawn; all changed squares are
   notified. Tests in `ChessTests` (`UndoTests.cpp`): undo castling both sides and colours, en
   passant, promotion with and without capture, then redo; position and FEN equal before and after.
2. **`BoardDiff`** (ChessCore) + tests: quiet move, capture, castling ×4, en passant, promotion
   (straight, capturing, choice in place), each reversed (takeback), two moves at once (Lichess
   batch), unchanged position, new game (too much changed).
3. **`BoardPieces` model** + `ChessConnector` wiring (queued `syncPieces()`, shown position helper,
   `Pieces`, `SelectedSquare`, `MovableColor`, `movePiece`, `canPlayerMove()`); tests in
   `ChessQtTests/BoardPiecesTest.cpp` (with `QAbstractItemModelTester`): ids stay on a move,
   dataChanged rows, reset on a jump, taken rows dropped on the next sync.
4. **QML geometry and the new `ChessFigures`**: one coordinate system, pieces from `Pieces`,
   board `TapHandler`, selection tint in `ChessBoxes`; click play works as before. All 42 screenshot
   scenes match (light/dark, White/Black, coordinates inside/outside) before going on.
5. **Slides and fades** (§2), all on `Theme.reduceMotion`.
6. **Dragging** (§3.4): handler, drop, slide back, Esc, cursors, drop-square outline, promotion by drop.
   As built: the dragged piece is drawn by `ChessBoard` above the move dots (`dragFrom`, `dragPiece`,
   `dragPoint`; the delegate stays on its square, faded), not moved by a `dragOffset` in the delegate,
   which would have stayed under the dots layer. On a drop the delegate takes the drop point as its
   slide offset, so a played drop only settles into the square and a refused one slides back.
7. **Screenshot scene `drag`**: a piece held over a legal square, dots and outline showing
   (`ScreenshotScene` sets the drag state; the scene stays in `all`). Update the `run-chess-app`
   skill's scene list.
8. **Docs:** `CLAUDE.md` (QML file list: `ChessFigures`, `BoardPieces`), `PROGRESS.md`, `TASKS.md`.

Steps 1–2 are plain C++ and can go in first on their own if wanted.

## 5. Testing

**Unit tests:** steps 1–3 above (`ChessTests`, `ChessQtTests`).

**Screenshots:** all 42 scenes unchanged (screenshot mode has animations off), plus `drag`.

**Manual** (Release, Reduce animations off, then on):

| # | Scenario | Expected |
|---|---|---|
| 1 | Click e2, click e4 | Dots, then the pawn slides; last-move squares as today |
| 2 | Drag Ng1 to f3 | Knight follows the pointer above other pieces, dots and outline show, drop plays Nf3, no second slide |
| 3 | Drag to an illegal square / off the board / press Esc | Piece slides back, nothing played, dots cleared |
| 4 | Robot's reply, Lichess opponent's move | Slide in |
| 5 | O-O by drag, by click, by the robot | King and rook slide together |
| 6 | En passant, a capture | Taker slides, taken piece fades out |
| 7 | Promotion by drag and by click; robot promotes | Pawn on the last rank, choice dialog, pawn turns into the piece; robot's pawn slides and turns |
| 8 | Takeback after castling, en passant and promotion (vs robot) | Pieces slide back, the castled rook and taken pawn return, the queen is a pawn again |
| 9 | ← / → in review, in a loaded game | One move slides per step |
| 10 | Click a move far back in the list, Home/End, new game, Continue | Position appears without slides |
| 11 | Try to drag: the robot's pieces, while it thinks, during review, after mate, opponent's turn online | Doesn't move; no hand cursor |
| 12 | Playing Black (board turned), then Flip board | Drag and drop land on the right squares both ways |
| 13 | Touch screen (if available) | Drag works with a finger |
| 14 | Reduce animations on | Moves jump, drag still works |

## 6. Effect on BL-11

Opening the game page and each review step no longer rebuild 64 delegates (one model reset builds
≤ 32 small pieces once); a move changes 1–3 rows instead of re-creating 2–4 delegates. Measure the
game page load before and after (BL-11) to see how much of the "up to a second" this was.

## 7. Risks and open questions

- **Queued sync timing.** The pieces update one event-loop pass after the move, other properties
  (last move, check) right away. Within one frame normally; if the highlight visibly leads the piece,
  `lastMoveChanged` can be emitted from `syncPieces()` too.
- **Drag on a turned board.** The pieces live inside the board turned by 180° for Black. Check in
  which coordinates `DragHandler`'s translation and centroid come on a turned parent, map with
  `mapFromItem`/`mapToItem` where needed, and test (manual #12).
- **"Too much changed" = 4.** A Lichess resync of two moves (4 entries with a capture) still slides;
  more appears at once. Adjust after trying it.
- **Lichess takeback** isn't applied to our board at all today (`LichessClient` only ever appends
  moves from `gameState`, it never notices the list got shorter). Not part of BL-04; worth its own
  task.
- **`HistoryPlayer` steps back with `UndoMove`** — fixed by step 1; until then loaded games with
  castling, en passant or promotion show wrong positions when stepping back (today as well).

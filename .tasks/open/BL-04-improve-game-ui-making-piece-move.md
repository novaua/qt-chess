---
id: BL-04
title: "Improve game UI making piece move and dragged by hands, use latest QLM techniques"
type: task
status: open
priority: medium
---

# BL-04 Improve game UI making piece move and dragged by hands, use latest QLM techniques

<!-- migrated from TASKS.md:84 (In Progress / Backlog) -->

- [ ] BL-04 Improve game UI making piece move and dragged by hands, use latest QLM techniques [Medium] → In Progress — plan: [`docs/BL-04-PieceMovePlan.md`](docs/BL-04-PieceMovePlan.md) (pieces with an identity that slide, drag and drop, one board geometry). Steps 1–4 done: `Game::UndoMove` takes back castling / en passant / promotion correctly; `Chess::BoardDiff`; the `BoardPieces` model as `ChessConnector.Pieces` (+ `SelectedSquare`, `MovableColor`, `movePiece`); the QML board on `Pieces` with one geometry and one `TapHandler`; slides and fades (step 5); dragging and the `drag` scene (steps 6–7). **Hand test by the owner (2026-11-12, plan §5 rows 2, 3, 7, 11–14):** OK: a slow drag with dots and the drop outline (A.1), an illegal drop goes back (A.2), Esc during a drag (A.4), no drag or hand cursor for the side not to move (A.5) or after mate (A.8), promotion by drag and by clicks (A.6–7), dragging on a flipped board both ways (B.9–10), the robot's promotion with FakeUci (D.13), Reduce animations (E.15). Found and fixed (branch `bugs/bl04HandTestFixes_121126`): a piece dragged off the board went under the bottom player card and the moves panel (A.3; it is now in the window's overlay, over everything); leaving a two-player game lost its moves without a word (A.4; now "Leave without saving?" with Leave / Save and leave / Stay on Home, Esc and closing the window, when there are moves not saved since; Switch user's confirm no longer says a two-player game is saved). **Rechecked by the owner (2026-11-13): A.3, A.4 and C.11–12 (vs the robot: busy cursor, premove by drag, no drag in review) OK.** Left: E.14 (touch: no touch screen)

## Resolution

### 2026-10-30 — BL-04 plan, steps 1–2: takeback fix and BoardDiff (branch: pieceDrag_301026)

#### What was done
- **Plan:** `docs/BL-04-PieceMovePlan.md` (pieces with an identity that slide, drag and drop, one board geometry), linked from BL-04 in TASKS.
- **Step 1, `Game::UndoMove` fixed.** It only moved the piece back and put a captured piece on the target square. After a takeback the castled rook stayed on f1/d1 (d8/f8), a pawn taken en passant came back on the target square instead of beside it, and a promoted piece went back to the 7th rank as itself. Takeback vs the robot (`ChessConnector::movePrev`) and stepping back in a loaded game (`HistoryPlayer::MoveBack`) use it; review (←/→) replays and wasn't affected. All changed squares are reported to the board listeners. En passant is recognised by the move before being a double step over the target square (`Game` records en passant as a capture on the target square). `ChessTests/UndoTests.cpp`: 14 tests (each castling, en passant both colours, promotion with and without capture, the player's promotion choice, two moves back), comparing FENs and the reported squares; 11 failed before the fix.
- **Step 2, `Chess::BoardDiff(before, after)`** (`ChessCore/BoardDiff.h/.cpp`; a class after review, first a free function `MatchPieces`): which piece went where between two positions: `Moved()` (`PieceTravel`: from, to, piece before/after), `Removed()`, `Added()`. Equal pieces pair nearest first; a pawn leaving the 7th (2nd) rank and a piece arriving beside it on the last rank is a promotion, reversed a takeback of one, on the same square the player's choice. `ChessTests/BoardDiffTests.cpp`: 11 tests, each case both ways.
- Debug and Release build; ChessTests 111, ChessQtTests 76 pass. No UI change yet.
- **Robot trace in Debug builds** (`ENGINE_TRACE` in `engineworker.h`, nothing in Release): the app log gets a line when the robot is asked (request number), when the worker starts and answers, when the answer is played or dropped as stale, and when the engine thread stops. For a flaky hang seen once in manual testing: playing Black vs a weak level, the board stayed on "Robot is thinking" although `uci_debug_log.txt` shows Stockfish answered in 40 ms; not reproduced since (8 runs of the `robot` scene, and the user's second try). If it happens again in a Debug build, the log shows where the answer stopped.

#### Where to continue
- BL-04 step 3: the `BoardPieces` model and the `ChessConnector` wiring (plan §3.2–3.3), on a new branch from `develop` once this PR is in.
- Locally: after switching from #91's branch, ChessGui needs a Rebuild (its Qt build cache still has Widgets dropped).

### 2026-10-30 — BL-04 step 3: the pieces model (branch: boardPieces_301026, from pieceDrag_301026 / #92)

#### What was done
- **`BoardPieces`** (`ChessGui/boardpieces.h/.cpp`, a `QAbstractListModel`): one row per piece, roles `pieceId`, `square`, `piece` ("N"/"n"), `taken`. `sync(board)` compares with the last shown position (`BoardDiff`): a moved or promoted piece keeps its row and id (`dataChanged`), a captured one is marked `taken` (its row stays until the next reset, so the fade isn't cut when the robot answers at once), a piece put back by a takeback is a new row. More than 4 changes (new game, review jump) → model reset with new ids.
- **`ChessConnector`:**
  - `Pieces` (CONSTANT). The board listener and `emitBoardState()` (review) post one queued `sync` per change (`schedulePiecesSync()`), so castling's four squares, a two-move takeback or a Lichess batch become one comparison. It syncs the **shown** position: `shownGame()` (a saved game's replay, a reviewed replay, or the game), now also used by `checkedKing()` and `materialBalance()`.
  - `canPlayerMove()`: the checks `figureSelected` had (replay, review, robot thinking, promotion choice, game over, the Lichess opponent's turn), now shared. `MovableColor` (0 none, 1 White, 2 Black) on it, notified by the signals its inputs change with.
  - `SelectedSquare`: the clicked piece of the side to move, or -1 (today's tint is QML-only). `clearPossibleMoves()` resets it with the dots, in the 4 places that cleared the dots.
  - `movePiece(from, to)`: a drop. Plays it if legal (`GetAllowedMoves(from)`), else returns false; clears the dots either way.
  - `boardChanged` / `refreshBoard()` stay until step 4 moves the QML board to `Pieces`.
- **Tests:** `ChessQtTests/BoardPiecesTest.cpp`, 9 tests with `QAbstractItemModelTester` (warnings collected, expected none): every piece on the first sync, nothing on the same position, id kept on a move, capture marks taken, takeback of a capture, castling (2 rows), promotion keeps the pawn's id, a big change resets, role names. ChessQtTests now uses Qt Test (`testlib`).
- Debug and Release build; ChessTests 111, ChessQtTests 85 pass; all 43 screenshot scenes match the baselines (no QML change yet).

#### Where to continue
- Step 4 (plan §3.4): one board geometry, `ChessFigures` on `Pieces`, a board `TapHandler`, the selection tint in `ChessBoxes`.
- Locally, after changing a project's `QtModules` (here `testlib`), Rebuild that project once: Qt's build cache keeps the old module list.

### 2026-10-30 — BL-04 step 4: the QML board on `Pieces` (branch: boardQml_301026)

#### What was done
- **One board geometry** (`ChessBoard.qml`): `squaresSize`, `squareSize`, `squareX(square)`, `squareY(square)` (a1 at the bottom left, in the board's own coordinates) and `squareAt(x, y)`. `ChessBoxes`, `ChessFigures` and `NavigationLayer` place their items with them; the three X-mirrored `Grid`s and the per-item Y-mirror/180° counter-rotations are gone. The board still turns for Black with `rotation`; a piece turns back with `rotation: chessBoard.angle` (same animation as before).
- **`ChessFigures.qml`**: a `Repeater` over `ChessConnector.Pieces`, one item per piece at its square (`taken` ones hidden until step 5 fades them). Before, every move removed and re-inserted `ListModel` rows, rebuilding square delegates (64 on page creation and on every review step).
- **Clicks**: one `TapHandler` on the pieces layer → `figureSelected(squareAt(...))`, instead of 64 `MouseArea`s. The selected piece's tint is in `ChessBoxes` (`SelectedSquare`), above the other highlights.
- **Removed**: `EmptyBoardModel.qml`, `ChessConnector::boardChanged`, `refreshBoard()`, the QML-only `selectedIndex`. `emitBoardState()` (review) now only syncs `Pieces` and clears the dots.
- **Screenshot scene `select`** (in `all`): 1. e4 e5 with the g1 knight selected — tint and dots; `-Black` shows the turned board.
- Debug and Release build; ChessTests 111, ChessQtTests 85 pass; all 44 scenes (light, dark) and `robot` match the baselines, no QML warnings. The old `robot` baseline was still from Qt 6.10 (`all` doesn't take it): renewed, only its text antialiasing differed.

#### Where to continue
- **Manual check (needs a person, screenshot mode ignores the mouse):** click-select-click moves vs the robot as White and as Black, in a two-player game, after Flip board; clicking an opponent's piece or an empty square; promotion; review ← →; takeback.
- Step 5 (plan §2, §3.4): slides (`Behavior on x/y`, `Theme.slow`), captured pieces fading out (`taken`), resets without slides, all off with Reduce animations.

### 2026-10-30 — BL-04 step 5: slides and fades (branch: pieceSlides_301026)

#### What was done
- **Pieces slide** (`ChessFigures.qml`): when a piece's `square` changes it slides there (`Theme.slow`, `OutCubic`), whoever moved it — the player, the robot, a Lichess opponent, a takeback, one review step. The slide is an offset in squares (`slideFile`/`slideRank`, animated to 0), not a `Behavior on x/y`, so resizing the window or turning the board doesn't slide anything, and a slide that starts while another is under way starts from where the piece is drawn.
- **Captures fade out** (`taken`, `Theme.normal`), under the piece that slides onto the square; **new pieces fade in** (a model reset — new game, review jump, Continue — or a piece put back by a takeback). This replaces the old 450 ms fade-in per piece.
- **Promotion:** the pawn slides and turns into the new piece when it arrives (`shownPiece`); the player's choice after the pawn's move turns it in place. A queen taken back turns into a pawn once on the 7th rank.
- Reduce animations (and screenshot mode): pieces jump and appear at once.
- Release and Debug build; all 46 scenes (light, dark, `select -Black`) match the baselines, no QML warnings. A temporary scene with animations on (not committed) showed e4xd5 mid-slide: the pawn over d5's fading pawn at 60 ms, settled by 1 s.

#### Where to continue
- Manual check by the owner (animations on): moves, castling and en passant slide and fade as planned.
- Step 6 (plan §3.4): dragging — `DragHandler` (`target: null`) with a pixel `dragOffset` next to the slide offset, `movePiece`, slide back on a refused drop, Esc, cursors, the drop-square outline.

### 2026-10-30 — BL-04 steps 6–7: dragging pieces (branch: dragPieces_301026)

#### What was done
- **Drag and drop** (`ChessFigures.qml`): each piece has a `DragHandler` (`target: null`), enabled for the side to move when the player may move (`ChessConnector.MovableColor`). Starting a drag selects the piece (dots); the piece stays faded on its square and `ChessBoard` draws it under the pointer, a little lifted, above the dots. The square under it is outlined (`ChessBoxes`, `Theme.dropSquare` = accent). A drop calls `ChessConnector.movePiece(from, to)`: played → the piece only settles from the drop point into the square (the drop point becomes its slide offset), refused (illegal square, off the board) → slides back, dots cleared. Dropped on its own square → stays selected. A press without a drag is still a click (`TapHandler`).
- **Esc** while dragging puts the piece back (`ChessBoard`'s Shortcut, `dragCancelled`); the window's Esc (leave the game) waits while a piece is held (`GamePage.pieceDragging`).
- Cursors: open hand over a movable piece, closed hand while dragging. The pointer is mapped from scene coordinates into the board's, so it is right on the turned board.
- **Screenshot scene `drag`** (in `all`): 1. e4 e5, the g1 knight held over f3; `-Black` too. `main.cpp` knows the name.
- Tested with a temporary hook (not committed) that sent mouse events to the window: Nf3 by drag; g1→g4 refused (back, no dots); Nf6 as Black on the turned board; press-release = click (selected, dots); a7xb8 by drop (pawn on b8, promotion dialog); Esc mid-drag (back, no move, still on the game page). Synthetic events need timestamps, else each move looks like a new press.
- Release and Debug build; all 49 shots (46 scenes, `select`/`drag -Black`) match, `drag` baselines new; no QML warnings.

#### Where to continue
- **Manual check:** plan §5 rows 2, 3, 7, 11–14 — dragging with the mouse vs the robot (White and Black, after Flip board), refused drops, Esc, promotion by drop, no dragging while the robot thinks / in review / after mate / opponent's turn online, touch if available, Reduce animations on. Then tick BL-04.
- Plan §7 risk still open: the last-move highlight comes one event-loop pass before the piece (not noticed so far).

### 2026-11-12 — BL-04 hand test and its two fixes (branch: bugs/bl04HandTestFixes_121126)

The owner ran BL-04's hand test (plan §5 rows 2, 3, 7, 11–14; results in `TASKS.md`). Two findings, both fixed:
- **A piece dragged off the board** went under the bottom player card and the moves panel (over the top card only because it comes earlier in `GamePage`). The dragged piece (`ChessBoard.qml`) is now in the window's overlay, mapped from the squares' coordinates, so it is over everything, the command bar too.
- **Leaving a two-player game lost it without a word** (only the robot's games are autosaved). `ChessConnector::leaveLosesGame()`: an unfinished two-player game with moves not saved since (Save game). Home, Esc and closing the window then ask "Leave / Quit without saving?" with Leave, Save and leave, Stay (`leaveLocalConfirm`; `ConfirmDialog` got an optional second action). Switch user's confirm said "The game is saved" for a two-player game too; it now says such games aren't kept.
- `tst_twoPlayers` test 3 (Esc and Stay, Save and leave, no question without moves or after Save game); `AppDriver.leaveToHome()` answers Leave when asked.
- Scenes `drag-off` (the knight over the bottom card) and `leave-local` (the dialog). The `drag` scenes match their baselines.

#### Learned
- In the overlay the dragged piece is mapped from the squares' coordinates on each frame of a drag (`FrameAnimation`): a mapping is no binding dependency, and the `drag` scene starts its drag before the page's layout (and, for Black, the board's turn) has settled.
- A hidden button in a `DialogButtonBox` keeps its slot: `ConfirmDialog` removes its second action's button when it has none.
- Restoring a file with `Copy-Item` keeps the backup's older time, and MSBuild then skips it (qrc): touch it before building.

**Code review (`/code-review` of #161, 10 findings), fixed:**
- `Game::Save` returns false when the file couldn't be written (it never checked); `ChessConnector::saveGame()` then says "Couldn't save the game" and the moves still count as unsaved, and Save and leave stays in the game. Unit test `Save_UnwritablePath_ReturnsFalse_Test`.
- The dialog's default (Enter) is **Save and leave** / Save and quit; Leave / Quit is the second action. With a saved game already there it says the save replaces it.
- Switch user and Accept a challenge use the same rule (`leaveLosesGame()`): no question for a two-player game without unsaved moves.
- The dragged piece is in the overlay only during a drag (an overlay with a child item stays visible), below a dialog (`z: -1`); `remap()` reads `dragFrom`, not the derived `dragging`; `ConfirmDialog` no longer destroys the removed button twice; the redundant `_gameResult` check is gone.
- Kept: the `FrameAnimation` (only during a drag, and the game page already draws every frame for the avatar pulse), and no question for a finished two-player game (nothing to continue, its result counted, as for the robot).

**Rechecked by the owner (2026-11-13):** A.3, A.4, C.11–12 OK.

#### Next
- BL-04: only E.14 (touch) is open, for when a touch screen is at hand.

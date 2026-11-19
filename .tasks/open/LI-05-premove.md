---
id: LI-05
title: "Premove"
type: task
status: open
priority: medium
---

# LI-05 Premove

<!-- migrated from TASKS.md:282 (Lichess Improvement Backlog) -->

- [ ] LI-05 Premove [Medium] (owner's manual check, bug 3). **Coded (branch `premove_091126`), vs the robot and on Lichess (not two players, owner's choice 2026-11-09): click or drag during the opponent's turn, red squares and red dots, a new premove replaces the old one; right click, Esc or a click elsewhere cancels it; played after the opponent's move has slid in, dropped silently when illegal (as on Lichess); a pawn to the last rank becomes a queen. `Game::GetPremoveTargets` (8 tests), scenes `premove`, `premove-played`, `premove-dropped`, `premove-robot`. Left: the owner's check in a real window and on Lichess (`PROGRESS.md`).** Fixed 2026-11-12 (BL-05 step 3, found by `tst_robot`): against the robot a premove couldn't be made with the mouse at all, a click blocker over the board took every click and drag while it thought. While it's the opponent's turn our pieces were frozen (`canPlayerMove()` follows Lichess's turn). On lichess.org a premove is a move made during the opponent's turn: it's kept in the browser and sent the moment it becomes our turn. The Board API has nothing for it (`/move` is refused when it isn't our turn), so it's ours to build:
  - allow one pending move (drag or click) in our colour during the opponent's turn, checked only for piece movement (it may become illegal), and shown highlighted (Lichess: red squares);
  - cancel it with a click on an empty square or a right click;
  - when the opponent's move is shown (MoveGate release), play the premove at once with no gate pause if it's legal, otherwise drop it silently;
  - maybe later: a queue of several premoves, and premoves vs the robot (the same mechanism, `Source::Robot`).

## Resolution

### 2026-11-09 — LI-05: premove (branch: premove_091126)

#### What was done
- **The owner's choices**: premove vs the robot and on Lichess, not in a two-player game (both sides sit at the board); an illegal premove is dropped silently, as on Lichess.
- **`Game::GetPremoveTargets(square)`** (ChessCore): where a piece may go once the other side has made one move, by its shape only. A line stops at our own piece (it may be taken: a recapture) and passes one of theirs (it may move away), not two. A pawn steps unless our own piece is in front, and has both captures. The king can castle while it and that rook haven't moved and none of our pieces stands between. Never onto our own king. 8 tests (`PremoveTests.cpp`).
- **`ChessConnector`**:
  - `canPremove()`: a robot or Lichess game, the opponent's turn, not reviewing, not finished. `MovableColor` is then the player's colour, so the pieces can be picked up as usual.
  - `figureSelected` / `movePiece` set the premove instead of moving: our piece shows its premove targets (`PremoveDots`), a dot or a drop on one sets `PremoveFrom`/`PremoveTo`. A new premove replaces the old one. A click on another square, a drop elsewhere, a right click or Esc (`cancelPremove()`) cancels it. Picking another piece keeps the pending one until a new target is chosen.
  - When the robot's or the opponent's move has been played (`playRobotMove`, `playOnlineMoves`), the premove waits for that move's slide (`slideMs`, 0 with Reduce animations), not for the gate's pause, then is played like the player's move if it is legal then (`playPremove`). A pawn to the last rank becomes a queen (Lichess's "auto-queen when premoving"). A selected piece without a premove gets its legal-move dots when the turn comes.
  - Cancelled by: the game's end, a new / continued / loaded game, leaving, a takeback vs the robot, a Lichess move list replaced. While reviewing it stays but isn't shown.
- **The board**: the two squares in red (`Theme.premove`), the premove dots in dark red (`Theme.premoveDot`); a right-click `TapHandler`; Esc in the window's Escape shortcut (move sheet first, then the premove, then leave).
- **`LichessClient::postMove`** does nothing in screenshot mode (`setOffline`), so a scene can play our move.
- **Scenes** (`main.cpp` feeds a Lichess game, no network): `premove` (Bf1-c4 waiting, the d2 pawn's premove dots), `premove-played` (Black's Nc6, then Bc4), `premove-dropped` (e4-e5 waiting, gone after Nc6), all in `all` with baselines. `premove-robot` (not in `all`, Stockfish): 1. e4, Nf3 premoved and played after the robot's answer; checked once, no baseline.
- Release builds; ChessTests + ChessQtTests **298 pass**. Screenshots: 82 compared, 0 failed; 6 new premove baselines (light and dark).

#### Code review (the owner's `/code-review medium` of #124, same branch)
9 findings: 7 fixed, 1 in part, 1 left for the owner.
- **Fixed:**
  - **A premove outlived a move made by hand**: in the moment it waits for the opponent's slide the player may already move. That move now clears it (`makeMove`), or it would have been played a move later in another position.
  - **A click on our own piece that is a dot set a premove onto it** (e.g. Nb1 selected, then a click on the d2 pawn gave Nb1-d2). A click on our own piece now always selects it; a recapture premove is made by dragging.
  - **The robot's move that can't be played** left the premove on the board: cancelled.
  - **The review dropped the premove** although the property only hides it: it is now kept, and `reviewLast()` plays it (if it is our turn) once the board is back at the live position.
  - **Esc left the game while premove dots showed**: it clears them first, like the right click.
  - **The promotion rule** moved to ChessCore: `Game::IsPromotion(from, to)` (1 test).
  - **The player's colour** in one helper, `isPlayerPiece()`. `GetPremoveTargets` and the castling checks are `const`.
- **In part:** "`GetPremoveTargets` repeats the piece geometry of `MoveGeneration`". It's `const` now, but stays separate: its lines follow other rules (through one of theirs, onto our own pieces), which `GenerateBasicMoves` can't express without a flag through all its paths.
- **Left for the owner:** "the premove waits `slideMs` and that costs clock time". The open question above.
- Release builds; ChessTests + ChessQtTests **299 pass**; screenshots 88 compared, 0 failed; `premove-robot` checked again.

#### Not checked yet (the owner, in a real window)
- vs the robot: premove by click and by drag, change it, cancel it by right click, Esc and a click on an empty square; a premove that becomes illegal (e.g. the robot takes the piece) disappears; a recapture premove (onto our own piece the robot takes); a premove promotion becomes a queen; castling as a premove.
- On Lichess: the same against a person, and that the premoved move reaches Lichess (the opponent sees it at once).
- The premove waits for the opponent's slide (about a quarter of a second): on a fast clock that time counts. Lichess plays it at once; tell me if it should not wait.

#### Next
- The owner's checks above; then the LI-02 and LI-04 hand tests, or the next task.

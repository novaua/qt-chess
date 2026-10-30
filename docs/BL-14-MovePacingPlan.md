# BL-14 One Thing at a Time on the Board — Implementation Plan

| | |
| :---- | :---- |
| **Date** | 2026-10-30 |
| **Status** | Plan |
| **Branch** | `movePacingPlan_301026` (plan); each step on its own branch from `develop` |
| **Task** | BL-14 in `TASKS.md`; follow-up of BL-04 (`docs/BL-04-PieceMovePlan.md`) |

## 1. Problem

Since BL-04 pieces slide (250 ms) and fade. The board shows what the game did, but nothing decides
*when* the next thing may happen, so things overlap:

1. **The robot moves together with the player.** `onHumanMoveDone()` asks the engine at once
   (`askEngineMove()`), and `onEngineMoveFound()` plays the answer the moment it arrives. At level 1
   that is a few milliseconds: both pieces slide at the same time.
2. **The robot moves while the board turns.** Playing Black, the robot's first move arrives while
   the board is still turning to the player. PR #95 fixed the same kind of problem for one case only
   (the check sound on Continue: `GamePage`'s `checkShownTimer`, at least 1 s after the page opened
   and not while `ChessBoard.turning`).
3. **Sounds and messages follow the game, not the board.** `ChessGame.qml` plays the move sound on
   `moveCountChanged`, the check sound and bar on `checkNotify`, mate on `checkMateNotify`; the
   result card waits a fixed 1600 ms (`resultTimer`). They are only in step with the board when
   nothing else is going on.
4. **Short freezes in a slide.** A piece sometimes stops for a few milliseconds. Not measured yet.
   Suspect: `NavigationLayer.qml` is a `Repeater` over `ChessConnector.PossibleMoves`, a
   `QStringList`; every `PossibleMovesChanged` (each click, and `clearPossibleMoves()` after every
   move) destroys and creates all 64 dot delegates (Item, Rectangle, PropertyAnimation) — in the same
   frame the slide starts. The robot's answer, the move list update and the sound start then too,
   all on the GUI thread, which also runs the QML animations.

## 2. Goals

| | |
|---|---|
| **One thing at a time** | A move that comes from outside — the robot, the Lichess opponent — is shown only when the board is idle: the page has settled, the board isn't turning, the last slide has finished, plus a short pause. Then the next one. |
| **The player is never kept waiting** | The player's own move (click or drop) is played at once, as today. |
| **The game and the board agree** | A held move is not played on `Game` until it is shown, so the last-move squares, check glow, move list, captured pieces, clock of moves and sounds stay in step without each being delayed on its own. |
| **Sounds and messages on the beat** | The move sound, check (sound and bar), mate and the result card come with the move they belong to, after it is shown. |
| **Smooth** | No visible stop in a slide (measured: no GUI-thread frame over ~20 ms during a move). |
| **No new waits in tests and screenshots** | Screenshot mode and Reduce animations: no slide time; the `robot` scene still finishes (`EngineThinking` stays true until the move is shown). |

Out of scope: premoves, clocks/time control, Lichess takeback (still not applied to our board,
see BL-04 §7), engine selection (BL-06).

## 3. Design

### 3.1 The move gate — `MoveGate` (ChessGui, C++)

A small `QObject` in ChessGui (`movegate.h/.cpp`, compiled into `ChessQtTests` like `BoardPieces`),
owned by `ChessConnector`. It holds the moves that came from outside and lets one through when the
board is idle.

```cpp
class MoveGate : public QObject {
    Q_OBJECT
public:
    struct Timing { int slideMs = 250; int pauseMs = 150; int robotMinMs = 400; };
    void setTiming(Timing);              // from QML: Theme.slow, 0 with Reduce animations
    void setStageReady(bool);            // the page has settled and the board isn't turning
    void moveShown(bool byRobot = false);// any move was played: the board is busy for slideMs + pauseMs
    void push(const QString& uciMove, Source);   // Robot (with its request number) or Online
    void clear();                        // new game, takeback, leaving the game, review
    bool holding() const;                // a move waits
signals:
    void release(const QString& uciMove, Source); // play it now
    void idle();                          // nothing waits and the board has settled
};
```

- **When is the board idle?** `stageReady && now >= busyUntil`. `moveShown()` sets
  `busyUntil = now + slideMs + pauseMs`. For the robot also at least `robotMinMs` after the player's
  move was shown, so a level-1 answer doesn't look instant (one value; tune by trying).
- **One at a time.** `release()` hands out one move; `ChessConnector` plays it, which calls
  `moveShown()` again, so the next one waits for that slide.
- **Many at once.** More than 2 moves waiting (a Lichess resync after a reconnect) → all are released
  together; `BoardPieces` resets (no slides), one sound.
- **Clock.** A single-shot `QTimer` to `busyUntil`; time comes from an injectable clock
  (`std::function<qint64()>`, `QElapsedTimer` by default), so tests drive it without waiting.
- **Stage ready** = the page has been open for at least 1 s (the rule from PR #95) and
  `ChessBoard.turning` is false. `GamePage` reports it (`ChessConnector.setBoardSettled(bool)`, one
  call); leaving the page or turning the board (Flip, new game as Black) sets it false again.

### 3.2 ChessConnector

- `onHumanMoveDone()` → `_gate->moveShown()`; the engine is still asked at once (see §3.4).
- `onEngineMoveFound(request, move)`: the stale check stays; then `_gate->push(move, Robot)` instead
  of `_game->DoMove()`. `EngineThinking` stays true until it is released and played, so
  `MovableColor` stays 0 and the player can't move in between. The release plays it with today's
  code (moved into `playRobotMove()`).
- Lichess: `movesReceived` → `applyMoves()` today. Moves of the opponent go through the gate
  (`push(move, Online)`); our own moves echoed back by Lichess are already on the board and are
  skipped as today. The online `canPlayerMove()` check (`IsWhiteMove()` vs our colour) already keeps
  the player from moving while the opponent's move waits.
- `startComputerGame`, `startLocalGame`, `startOnlineGame`, `continueGame`, `movePrev` (takeback),
  `endGame`, entering review → `_gate->clear()`. The request number already drops a stale robot
  answer; `clear()` also drops one already in the gate.
- `applyMoves()` from the Alt+S dialog and the screenshot scenes stays direct (not gated).

### 3.3 Sounds, check, mate, result

Because the held move isn't played until it is shown, today's signals already come at the right
time once the gate is in: `moveCountChanged`, `checkNotify`, `checkMateNotify`, `checkMateResult`
are emitted when the move is played. What changes:

- **#95's special case goes**: `GamePage.checkShownTimer` (1 s, then polling `turning`) becomes
  "the gate reports `idle` for the first time on this page" → `checkShown()` if in check and no
  move was made meanwhile.
- **Result card**: `resultTimer` (1600 ms after the result) starts on the next `idle` after the
  result instead (the mating move has finished its slide), plus a short beat (~600 ms) so the
  final position is seen first.
- **Sound after the slide or with it?** Today the sound starts with the move. Keep that (it is the
  "click" of the piece being picked up and set down); the gate only makes sure two moves don't
  sound at once. Revisit after trying.

### 3.4 When is the engine asked? (owner's decision)

| Option | |
|---|---|
| **A. Think during the animation, show after** (recommended) | Asked at once, as today. Its answer waits in the gate. No thinking time is lost at the high levels (Stockfish may think seconds), and the low levels still wait for the slide and `robotMinMs`. |
| B. Ask only when the board is idle | Simpler to explain ("the robot starts after the move is shown"), but the slide time is added to every robot move at every level. |

The plan builds A; switching to B is one line in `onHumanMoveDone()` (ask on the gate's `idle`).

### 3.5 Freezes

**Measure first** (Release, animations on):

- A frame-time trace in Debug builds, like `ENGINE_TRACE`: connect to `QQuickWindow::frameSwapped`,
  log every frame gap over 20 ms with the last `ChessConnector` signal before it
  (`PossibleMovesChanged`, `moveCountChanged`, a sync of `Pieces`, the robot's answer).
- `QSG_RENDER_TIMING=1` for the render side, the QML profiler (`qmlprofiler`) for a few moves.

**Fix what it shows.** Expected first:

- `NavigationLayer.qml`: `Repeater { model: 64 }` with
  `visible: ChessConnector.PossibleMoves[index] !== " "` (or a small `QAbstractListModel` with
  `dataChanged`), so no delegates are rebuilt; the pop-in runs on `visible` turning true.
- `BoardPieces` sync and the move list update in the same event-loop pass as the move: fine if
  cheap; if not, the move list (`appendMoveToHistory()`) can follow one pass later.
- If GUI-thread work still stalls a slide: run the slide on the render thread (`XAnimator` /
  `YAnimator` on an inner item of the piece). Caveat: an Animator's value is only written back at
  the end, so a slide that starts while another runs needs its own start point kept in QML. Only if
  the measurements say so.

This is the move part of BL-11 (profiling); the numbers go into `PROGRESS.md` as BL-11 asks.

## 4. Implementation steps

1. **Measure and fix the freezes** (§3.5): frame trace (Debug), numbers before/after,
   `NavigationLayer` without delegate rebuilds. Screenshots unchanged.
2. **`MoveGate`** + tests in `ChessQtTests/MoveGateTest.cpp` (fake clock): a robot answer before the
   slide ends waits until `slideMs + pauseMs` and `robotMinMs`; stage not ready holds it; `clear()`
   drops it; two Lichess moves come out one after the other; more than 2 at once together; Reduce
   animations (`slideMs = 0`) still keeps the pause; `idle` after the last one.
3. **The robot through the gate** (§3.2): `onEngineMoveFound` → `push`, `playRobotMove()`,
   `EngineThinking` until shown, `clear()` on new game / takeback / leave / review; `GamePage`
   reports stage ready (1 s and not turning). Fixes problems 1 and 2.
4. **Check, mate and result on the gate** (§3.3): `checkShownTimer` and `resultTimer` replaced by
   `idle`. Fixes problem 3; the memory rule "sounds after animations" is then one mechanism.
5. **Lichess opponent's moves through the gate**, resync of many moves at once.
6. **Render-thread slides** — only if step 1's numbers after the fixes still show stalls.
7. **Docs:** `CLAUDE.md` (MoveGate in the architecture list), `PROGRESS.md`, `TASKS.md`.

Steps 1 and 2 don't depend on each other; 3 needs 2; 4 and 5 need 3.

## 5. Testing

**Unit tests:** step 2 (`MoveGateTest`, fake clock); existing `ChessQtTests` (engine player, UCI)
still pass.

**Screenshots:** all scenes unchanged; `robot` still ends with Qh5# (it waits for `EngineThinking`,
which now ends when the move is shown).

**Manual** (Release, animations on, then Reduce animations on):

| # | Scenario | Expected |
|---|---|---|
| 1 | Level 1 as White, quick moves | Player's piece slides, then the robot's; never both at once |
| 2 | New game as Black (board turns) | The board turns first, then the robot's first move slides |
| 3 | Flip board while the robot thinks | Its move waits until the board has turned |
| 4 | Level 8+ (slow robot) | No extra wait compared to today |
| 5 | Robot gives check / mates | Its piece slides, then check sound and bar / mate sound; the result card after the slide |
| 6 | Continue a game in check (as Black) | As after #95: board turns, then the check sound and bar once |
| 7 | Takeback or New game while the robot's answer waits | The answer is dropped; nothing moves afterwards |
| 8 | Lichess: opponent answers instantly (premove) | Our slide, then theirs |
| 9 | Lichess reconnect with several moves | Position appears at once, one sound |
| 10 | Click pieces and play quickly for a minute | No visible stops in the slides |

## 6. Risks and open questions

- **Holding the robot's move changes what "thinking" means** for the UI: the "Robot is thinking"
  bar and the wait cursor stay until the move is shown (up to ~0.5 s longer at low levels). Fine,
  arguably right.
- **Autosave on close** (`canAutoSave()`) while a robot answer waits in the gate: the answer isn't
  in the game yet, so it isn't saved; on Continue the engine is asked again. Acceptable.
- **Lichess clocks:** holding the opponent's move for ~0.4 s doesn't cost the player time on
  Lichess (our clock starts when Lichess says so), but the shown position is that much later. Keep
  the Online pause short (slide only, no `robotMinMs`).
- **Stage ready from QML** is one more C++ ↔ QML link; keep it a single call
  (`setBoardSettled(bool)`) driven by `GamePage`, not several properties.

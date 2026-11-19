---
id: BL-14
title: "One thing at a time on the board"
type: task
status: open
priority: medium
---

# BL-14 One thing at a time on the board

<!-- migrated from TASKS.md:97 (In Progress / Backlog) -->

- [ ] BL-14 One thing at a time on the board [Medium] — plan: [`docs/BL-14-MovePacingPlan.md`](docs/BL-14-MovePacingPlan.md). Follow-up of BL-04, from the owner's testing: the robot's answer (level 1) slides together with the player's move; playing Black, the robot moves while the board is still turning; sounds and messages follow the game, not the board; slides sometimes stop for a few ms. Plan: a `MoveGate` that holds moves from outside (robot, Lichess opponent) until the board is idle (page settled, not turning, last slide done, a short pause), so game, board and sounds stay in step; the player's own move stays instant. Step 1 done (branch `boardFreezes_301026`): frame trace, `NavigationLayer` and the move list without rebuilds — numbers in `PROGRESS.md`. Step 2 done (branch `moveGate_301026`): `MoveGate` with 12 tests (fake clock) — holds outside moves until the board is idle, one batch for a resync, `isIdle()` for results without a move. Step 3 done (branch `robotThroughGate_301026`): the robot's answer goes through the gate and `EngineThinking` lasts until it is shown; `GamePage` reports the board settled (1 s, not turning) and the slide time; takeback, review, new game and leave drop a held answer. Step 4 done (branch `resultOnIdle_311026`): the result card comes a beat after the gate is idle (`isBoardIdle()` / `boardIdle`) instead of 1600 ms from the result; a result without a move starts the beat at once. Step 5 done (branch `lichessThroughGate_021126`): the Lichess opponent's moves go through the gate (a resync of more than two at once is shown anew, without slides, with one sound); a game end that arrives with the opponent's last move is shown after that move, and the stream now emits that move (the mating move used to be lost). Next: manual checks with a real Lichess game; step 6 only if the step-1 numbers still show stalls. Open: think during the animation and show after (recommended), or ask the engine only after it

## Resolution

### 2026-10-30 — BL-14 plan: one thing at a time on the board (branch: movePacingPlan_301026)

#### What was done
- The owner tried BL-04 (#96, #98 merged; slides, castling, en passant fine) and found: the level-1 robot's answer slides together with the player's move; as Black the robot moves while the board turns; sounds follow the game, not the board; slides sometimes stop for a few ms.
- **Plan:** `docs/BL-14-MovePacingPlan.md`, task BL-14 in TASKS. A `MoveGate` (ChessGui) holds moves from outside — the robot's answer, the Lichess opponent's move — and plays one only when the board is idle: page open ≥ 1 s and not turning (PR #95's rule, now general), last slide done, a short pause (and ≥ ~400 ms after the player's move for the robot). A held move isn't played on `Game` until shown, so last move, check, move list and sounds stay in step by themselves; `EngineThinking` lasts until the move is shown. #95's `checkShownTimer` and the 1600 ms `resultTimer` become "the gate is idle". Freezes: measure first (frame trace in Debug); suspect `NavigationLayer`'s `Repeater` over the `PossibleMoves` string list (64 delegates rebuilt on every click and after every move).
- Checked while planning: Lichess echoes of our own moves are skipped (`LichessClient::postMove` appends to `_lastMovesList`); a resync on joining comes as one batch.

#### Where to continue
- **Owner's decision** (plan §3.4): the robot thinks during the animation and its move is shown after (recommended, planned), or it is asked only once the board is idle.
- Step 1 (measure the freezes, `NavigationLayer` without rebuilds) and step 2 (`MoveGate` + tests) can start in either order, each on a new branch from `develop`.
- BL-04: the manual check of dragging is still open.

### 2026-10-30 — BL-14 step 1: the freezes measured and fixed (branch: boardFreezes_301026)

#### What was done
- **Frame trace** (`frametrace.h/.cpp`, off unless `QT_LOGGING_RULES="chess.frames.debug=true"`, Release too): the app log gets gaps between two frames the board drew over 25 ms ("slow frame"; over 500 ms "no frame for" — the window resting, or a long stall), a 16 ms GUI-thread heartbeat's late ticks ("GUI thread busy", however long — this also catches the page-open stalls), and how long the board's own work took — the move dots, the pieces sync, the signals after a move — with log timestamps to line them up. `FrameTrace::watch(window)` in `main.cpp`.
- **`slides` screenshot scene** (not in `all`): animations on (every other scene forces them off), selects the g1 knight and plays Nf3, Nf6, Bc4, Nc6 0.4–1 s apart. It makes the numbers below reproducible: `ChessGui.exe --screenshot out.png --scene slides --delay 5200` with `QT_LOGGING_RULES="chess.frames.debug=true"`, then read `out.png.log`.
- **`NavigationLayer.qml` without rebuilds:** the 64 dots stay for the page's life; a selection only shows and hides them. Before, the `Repeater` was bound to `PossibleMoves`, so every click and every move (dots cleared) destroyed and created all 64 delegates, and each new delegate started its 450 ms pop-in **even when its dot was hidden**. Now the pop-in starts from `onVisibleChanged`, only for a dot that appears, and hiding a dot stops it.
- **`MoveListModel`** (`movelistmodel.h/.cpp`, `QAbstractListModel`, 12 tests in `ChessQtTests/MoveListModelTest.cpp`): the move list is `ChessConnector.MoveHistory` as a model; a move adds a row (White) or fills Black's half of the last one, a takeback removes or clears that row, never a reset; only the new plies are copied. Before, the whole `QVariantList` was rebuilt on every move and `MoveList.qml` read `modelData`. `ChessConnector.ShownMove` (e.g. "12. Nf3", "12… exd5") replaces the QML computation from the list; Copy/Export PGN use `PlyCount` instead of `MoveHistory.length`.

#### Measurements (Release, the `slides` scene above, animations on; 3 runs of the old code, 2 of the new)
| | before | after |
|---|---|---|
| Selecting a piece (`PossibleMovesChanged` work) | 3.5–4.3 ms | 0.5–0.8 ms |
| The move right after a selection (dots cleared) | 3.5–4.2 ms | 0.4–1.1 ms |
| Later moves, dots already hidden | 0.2–0.4 ms | 0.4–0.8 ms |
| `select` scene in screenshot mode (no animations) | 2.9 ms | 0.3 ms |

What the owner sees as "the slide stops for a few ms" is the thread that runs the animation doing this rebuild in the same frame the slide starts; it is ~4 ms, a quarter of a 60 Hz frame, plus the hidden pop-ins that keep animating. Frame gaps over 25 ms during the slides were **not** reproducible: one run of the old code showed 21 frames at 32–34 ms stretched over ~3 s, but later runs of the same build showed none, so that was not the delegate rebuilds alone. The trace now reports frame gaps and GUI-thread stalls, so if the owner still sees stops, the log says which side it is. (Checked with the code review: the original frame trace silently dropped gaps of 500 ms and more, and had no GUI-thread measure; both are fixed.)

- Debug and Release build; ChessTests 111, ChessQtTests 97 pass; all 49 screenshot scenes match the baselines (light, dark, `select`/`drag -Black`), `robot` scene OK, no QML warnings.

#### Code review (medium, same branch)
12 findings, 8 fixed: the frame trace's 500 ms blind spot and its missing GUI-thread measure (now the heartbeat), the pop-in that kept running when its dot hid, test assertions that counted signals but not their rows/roles (now they check both, and a new test covers a shorter game with other moves), the redundant full copy of the move history on every move, the robot's move trace that double-counted its inner scopes and claimed a rejected move was played, and the test inventory in `CLAUDE.md`. The rest, not changed: the model keeps its own copy of the moves (a model owns its data; only `ChessConnector` calls `show()`), SAN piece letters are inherent to SAN, `ShownMove` and the cells' accessible names are built separately, the test scaffolding repeats `BoardPiecesTest`'s, and `GamePage`'s `shownMove` alias is a plain pass-through.

#### Where to continue
- **BL-14 step 2 (`MoveGate`) and step 3 (the robot through the gate)** — the next branch from `develop`; the plan is `docs/BL-14-MovePacingPlan.md`. The owner's decision on §3.4 is still open (recommended: think during the animation, show after).
- The frame trace and the `slides` scene stay for step 6: if the owner still sees stops after the gate is in, they will say whether GUI-thread work or the render side is left.
- BL-04's manual drag check is still open.

### 2026-10-30 — BL-14 step 2: `MoveGate` and its tests (branch: moveGate_301026)

#### What was done
- **`MoveGate`** (`ChessGui/movegate.h/.cpp`; not wired into `ChessConnector` yet — that is step 3): holds the moves that come from outside and releases them one at a time only when the board is idle.
  - Idle = `setStageReady(true)` (the page has settled and the board isn't turning) and `now >= busyUntil`. `moveShown(byRobot)` sets `busyUntil = now + slideMs + pauseMs`; a robot move also waits `robotMinMs` after the player's move, an Online move does not.
  - More than 2 waiting at once (a Lichess resync) are released together as one `releaseBatch`, so the receiver can show the position at once; otherwise one at a time.
  - `clear()` drops a waiting move and the pacing of the old board; `idle()` fires once each time the gate becomes idle — when the page settles and after each shown move's slide (step 4 uses it for the check sound and the result card). `isIdle()` answers the same question on demand.
  - Timing comes from QML later: `slideMs = Theme.slow`, `0` with Reduce animations; `pauseMs` stays, and `setTiming()` shortens a window already under way. A single-shot `QTimer` drives it; the clock is injectable and `tick()` is public so the tests drive time by hand.
- **12 tests** (`ChessQtTests/MoveGateTest.cpp`, fake clock): a robot answer waits for slide + pause + `robotMinMs`; an Online move ignores `robotMinMs`; stage not ready holds; `clear()` drops, and resets the pacing; two Online moves come out one after the other; more than two at once as one batch; `slideMs = 0` still keeps the pause; shortening the timing under way; a release without `moveShown()` still settles; `idle()` once, after the last released move, and `isIdle()`.
- `ChessQtTests/TestSupport.h`: the `ToString(QString)` specialization the test DLL needs, shared by its five test files instead of copied into each.
- Debug and Release build; ChessTests 111, ChessQtTests 109 pass; the `home` screenshot still matches (no UI change).

#### Code review (medium, same branch)
11 findings, 7 fixed:
- `idle()` is edge-triggered, so a result without a move (resignation, draw, timeout) would never get one; added `isIdle()` for step 4 to ask.
- A resync batch was N ordinary releases, which a receiver couldn't tell from N moves (a sound and a slide each); now one `releaseBatch`.
- A released move that the receiver applies as a position reset (no `moveShown()`) left no busy window, so `idle()` never came; the gate now marks the board busy before it releases.
- `clear()` kept the old board's pacing into the new game; it resets it now.
- `setTiming()` claimed to shorten a running slide without doing so; it recomputes the window now.
- The `schedule()` copy of `tick()`'s pacing rules is gone.
- The `ToString` copy-paste is one shared header.
Not changed: `setClock()`/`tick()` stay public as the documented test seam; non-move animations (the takeback/undo slide, the first paint, review jumps) still need the connector to call `moveShown()` — listed for step 3; a Robot answer's release also sets a busy window optimistically, which the connector's `moveShown()` simply confirms.

#### Where to continue
- **Step 3 (new branch from `develop`)**: the robot through the gate — `onEngineMoveFound` does the stale check then `push`; the release plays the move (`playRobotMove()`), `EngineThinking` stays true until it is shown, `clear()` on new game / takeback / leaving / review; `GamePage` reports stage ready (at least 1 s after opening and not turning, with `Theme.slow` as the slide time).
  - **Move the `_resignAfterEngineMove` consume block with the play tail into `playRobotMove()`**: it only works today because `onEngineMoveFound` clears `_engineThinking` before it; left behind, a resignation while the answer waits would be re-deferred and never happen.
  - **A takeback while the robot's answer waits in the gate**: `movePrev()` returns at once when `_engineThinking` is set, so Undo would silently do nothing and the held answer would still slide in. Before that guard, step 3 must `_gate->clear()`, bump `_engineRequest`, clear `_engineThinking` and `emit engineThinkingChanged()`, then let the takeback run — the dropped answer never reaches `onEngineMoveFound()`, so nothing else would reset the thinking state.
  - **Call `moveShown()` after a takeback** (and any other non-move board animation, e.g. the new game's first paint), so the engine's answer waits for the undo slide.
  - Fixes the robot moving together with the player and while the board turns.
- Then step 4 (check, mate and result on `idle`; `isIdle()` covers a result without a move), step 5 (the Lichess opponent's moves, and `releaseBatch` for the resync).
- The owner's §3.4 decision is still open (recommended A: think during the animation, show the answer after).

### 2026-10-30 — BL-14 step 3: the robot through the gate (branch: robotThroughGate_301026)

#### What was done
- **The robot's answer is held, not played on arrival.** `onEngineMoveFound` pushes it into the gate as `Source::Robot`; `EngineThinking` stays true until the gate releases it, so the "Robot is thinking…" bar, the wait cursor and `MovableColor` stay until the move is shown. `playRobotMove()` is the old play tail moved out of the handler (play the move, history, counters); it consumes `_resignAfterEngineMove` too, so a resignation pressed meanwhile happens after the answer (the trap noted in step 2).
- **The board settles** (`GamePage.qml`): `ChessConnector.setBoardSettled(bool)` after the page has been open for 1 s and the board isn't turning (`ChessBoard.turning`); a Flip or a new game as Black holds the answer until the turn ends. `setMoveTiming(Theme.reduceMotion ? 0 : Theme.slow)` on opening and whenever Reduce animations changes.
- **The player's own move** calls `moveShown()` in `onHumanMoveDone()`: the robot's answer waits for its slide + pause and at least `robotMinMs`. The player's move itself stays instant.
- **Clearing:** `beginGame()` (every new, continued and online game), `stopEngineThread()` (also when no engine is running, so every path that stops the robot), `endGame()`, `loadGame()`. A **takeback** drops the held or still-coming answer, bumps `_engineRequest` and clears the thinking state (otherwise `movePrev()`'s `_engineThinking` guard would make Undo a silent no-op); the undo slide is marked with `moveShown()`, so a re-asked answer waits for it. A **live review** drops the waiting answer; `reviewLast()` asks the engine again.
- **Verified** with temporary scripted scenes (not committed):
  - Continue as Black: Qh5# was held from its arrival until the page settled, then played (469 ms later);
  - level 1, the player plays e4, with `robotMinMs` temporarily raised to 1500: the answer arrived after 751 ms and was held until exactly 1499 ms after the player's move;
  - a takeback while that answer was held: it was never played, the thinking state cleared, and no new engine request was made.
  - Continue in check (Qxf7+ and the robot to move): the answer was held to the settle and played; the first `boardIdle` came after it (count changed), so no check was announced — the check sound/bar now follow the gate (fixing the 1 s race below).
- Debug and Release build; ChessTests 111, ChessQtTests 109 pass; all 49 screenshot scenes match, the `robot` scene (Continue, Qh5#) included.

#### Code review (medium, same branch)
5 findings, all fixed:
- **Undo while the robot's first answer was pending**: the thinking block dropped the answer, then `count == 0` returned before re-asking, so the robot never moved — and `canPlayerMove()` let the human move its pieces. Now a count-0 Undo re-asks when the robot is to move, and, as a safety net, a computer game only lets the player move its own colour.
- **The deferred resignation (`_resignAfterEngineMove`) survived a takeback**: the next answer would resign on its own. A takeback now cancels it.
- **Review jumps**: dropping the answer on review and re-asking wasted the running search and could show the answer during the jump. New design: a live review reports the board as not settled (`_boardSettled` excludes `ReviewMode`), so the answer simply stays held — no drop, no re-ask; `reviewLast()` marks the jump with `moveShown()` so the answer waits for it.
- **The resumed-check announcement raced the held answer**: `checkShownTimer` and the settle timer fired in the same tick, announcing a check that the answer parried in the same pass. The check is now announced on the gate's first `boardIdle` (new `ChessConnector.boardIdle`), which is silent while an answer waits. This is part of step 4's §3.3, pulled in here because step 3 introduced the race.
- **Stacked searches after review round-trips**: gone with the no-drop review design.
Not changed: a takeback during a search still lets that search finish before the re-ask — the engine cannot be cancelled mid-search without more work in `EngineWorker`; the answer is simply dropped (request number) and a new search queued.

#### Where to continue
- **Step 4 (new branch from `develop`)**: the result card on the gate's `idle` (replace the 1600 ms `resultTimer`; use `isIdle()` for a result that comes without a move — resignation, draw, timeout). The check sound/bar already follow `boardIdle` (done here); mate and the result card are left.
- **Step 5**: the Lichess opponent's moves through the gate (`push(..., Source::Online)`, `releaseBatch` for a resync, `moveShown` after applying them).
- **Manual check by the owner** (Release, animations on): level-1 quick moves (never both pieces sliding at once); a new game as Black (board turns first, then the robot's move); Flip board while it thinks; the robot gives check / mates; Continue in check; takeback / New game while its answer waits; **Undo before the robot's first move** (as Black; the robot must still move); review ←/→ while it thinks, then Back to live; Reduce animations on.
- The owner's §3.4 decision is still open (currently A: think during the animation, show the answer after).

### 2026-10-31 — BL-14 step 4: the result card on the gate's idle (branch: resultOnIdle_311026)

#### What was done
- **The result card no longer waits a fixed 1600 ms from the result.** On a result, `GamePage` now waits for the board to settle — the move gate's idle: the mating move's slide and pause are over, or, for a resignation / draw / timeout (no move), the board was already still — and then shows the card after a **600 ms beat**, so the final position is seen first.
  - `ChessConnector.isBoardIdle()` (Q_INVOKABLE, `MoveGate::isIdle()`) lets the page check the state when the result arrives: a result that comes with a move is checked on the next pass (`Qt.callLater`), after the move has marked the board busy; otherwise the next `boardIdle` shows it.
  - Undo after mate/resign still clears the pending result and hides the card.
- `ScreenshotScene`: a finished game's ready delay is 2000 ms (the card now comes after the settle + beat).
- Verified with the Debug build **offscreen** (the screen was locked): `result`, `ended` and `robot` logs show result → page settle (`openTimer` at 1 s) → `boardIdle` → card 600 ms later → screenshot; the `result` and `robot` frames were inspected (card, mate, flagged king correct). ChessTests 111, ChessQtTests 110 pass in Debug and Release.
- **Real-window comparison done after the unlock:** `Take-Screenshot.ps1 -Scene all`, `-Scene all -Dark` and `-Scene robot` on this branch, then `Compare-Screenshots.ps1`: **49 compared, 0 failed** (`result`/`result-dark` show the card over the settled board, `robot` too). The `about` pair failed at 0.09 % first — only the build timestamp of the fresh build (`Version 0.1.0 Build Oct 31, 2026, 4:13:12`) — and both were re-accepted with `-Update -Name 'about*'` (local baselines). The `robot` scene passed in a real window this run.
- **Rebased onto BL-15 (#104)** after its merge: Debug and Release build; ChessTests 111 + ChessQtTests 113 (224) pass in both; the `robot` scene — it ends the game while the engine thinks, the BL-15 shutdown path — passed **3/3** real-window runs, and `robot`, `result`, `result-dark` match the baselines.

#### Code review (medium, same branch)
3 findings, all fixed:
- **A result that arrives while reviewing stayed pending**: the gate is deliberately not-ready during review, so neither the one-shot `isBoardIdle()` probe nor a `boardIdle` edge could show the card (the old 1600 ms timer ignored review). Now a pending result shows when it arrives during review (`_resultIdleCheck` treats review as "no live animation"), and `onReviewStateChanged` shows it if the user enters review after the result. Verified with a temporary scene (resign while reviewing): the card shows over the reviewed position.
- **MoveGate lost its idle edge when retiming** (`setTiming` after an idle edge, e.g. Reduce animations switched off within a pause): `updateBusy()` moved the window into the future while `_wasIdle` stayed true, so the next idle was suppressed. `tick()` now clears `_wasIdle` whenever the board is busy again; new test `RetimingAfterIdleEmitsTheNextIdle`.
- **ScreenshotScene guessed the card timing** (2000 ms): it now polls `_resultShown` instead, so it grabs only once the card is actually up.

#### Where to continue
- **Step 5**: the Lichess opponent's moves through the gate (`push(..., Source::Online)`, `releaseBatch` for a resync, `moveShown` after applying them).
- Manual checks for step 3 are still open (see the step-3 entry below).

### 2026-11-02 — BL-14 step 5: the Lichess opponent through the gate (branch: lichessThroughGate_021126)

#### What was done
- **The opponent's moves wait for the board.** `LichessClient::opponentMoveReceived` now goes to `ChessConnector::onLichessOpponentMoves()`, which pushes each move into the gate as `Source::Online` (no `robotMinMs`: only the slide and pause). `playOnlineMoves()` plays a released move (or a batch), updates the history and counters, and marks the board busy (`moveShown(true)`). Lichess's own turn rule (`canPlayerMove()`) keeps the player from moving while the move waits.
- **Resync** (`gameFull` on (re)joining a game with moves): more than two moves at once come out as one `releaseBatch`. The pieces are shown anew without slides (`BoardPieces::reset`, now public, via `_piecesAnew`), and there is one move sound; only a check in the final position is announced.
- **Game end while the opponent's last move waits** (e.g. a resync before the page has settled): the end is applied at once (game over, statistics, the flag), and the result card waits for the board as for any result (step 4). See the code review below; the first version held the end back.
- **The opponent's mating move was lost** (found while doing this): `LichessClient` returned on a `gameState` with a final status before emitting its new moves. Lichess sends the mate in that same state, so our board never showed it. The stream handler now emits the new moves first, then `gameEnded`. The per-line handling is `handleGameStreamLine()` (public, for tests).
- **Reviewing when the opponent moves**: the gate shows nothing while reviewing, but the player's clock runs on Lichess, so the board goes back to the live position (`reviewLast()`) and then shows the move.
- Plain `applyMoves()` (Alt+S dialog, screenshot scenes) stays direct.
- **Tests:** `LichessClientTest` +3 (mate move before the end, resync in one signal, only new moves), `BoardPiecesTest` +1 (`reset` without slides). Debug and Release build; ChessTests 111, ChessQtTests 117 pass in both.
- **Verified end to end** with a temporary hook (not committed) that fed stream lines into `LichessClient` in screenshot mode: (1) a resync of `f2f3 e7e5 g2g4` plus the opponent's `d8h4` mate and `status: mate` arriving before the page settled — the end waited, all four moves appeared at once after the settle, mate, result card; (2) the mate arriving after the resync was shown — the three moves at once, then Qh4# alone, then Lichess's end. Both screenshots: Qh4#, the flagged king, "Friend wins by checkmate".
- Screenshots: all 49 match. The 6 profile shots differed only in the "since <date>" text (today's date); their local baselines were re-accepted.

#### Code review (PR #106 comments, same branch)
8 findings; fixed:
- **A resync on a settled page let the first move through alone** (each move was pushed separately, and an idle gate released the first at once). `MoveGate::push(QStringList, Source)` queues them all before the gate looks; test `ResyncPushedTogetherIntoAnIdleGateIsOneBatch`.
- **A held game end kept the game "running"** (rematch offers dropped; Resign / Offer draw still posted to an ended game), and **a held end stayed hidden while reviewing**. The held end is gone: Lichess's end is applied at once, the card already waits for the board. This also removes the two loose strings and the second winner-to-score mapping (cleanup finding).
- **Re-entering the review while the opponent's move waits** hid it until the review ended. Entering a review of an online game (`reviewFirst/Prev/To`) now plays the held moves first (`MoveGate::releaseAll()`, test `ReleaseAllLetsTheHeldMovesThroughAtOnce`); the board shows an earlier position anyway.
- **`_piecesAnew` stayed set if a resync's first move failed**: now set only when more than one move was played.
- **A draw offer / takeback request showed before the move it came with**: `afterHeldMoves()` runs it once the held moves are shown.
- **`playOnlineMoves` copied `applyMoves`' loop**: both use `playMoves()` now. Side effect: `applyMoves()` (screenshot scenes, Alt+S) also announces only a check in the final position. The `review` and `moves` baselines showed a stale "Check" bar from …Qa5+ earlier in the scripted game (the final position is not check); they were re-accepted without it.
- **Found while verifying (a step-3 bug): `GamePage` reported a stale "settled"** to the gate. `onReviewStateChanged` / `onTurningChanged` called `_reportSettled()` before the `_boardSettled` binding had been updated, so entering a review reported "settled" and leaving it reported "not settled" — a move held then waited forever (the robot's answer after Back to live too). Now `on_BoardSettledChanged` reports it, on the next pass (`Qt.callLater`) because the gate may play a held move at once, which re-enters the binding (QML warned of a binding loop).
- Not fixed here: **an accepted Lichess takeback** (shorter move list) is not undone on our board — the existing gap from BL-04 §7; now **LI-03** in the Lichess backlog (`TASKS.md`, PR #107).
- Verified with the temporary stream hook (not committed), five scenarios: mate held during the settle (result at once, card after the move), mate after the resync, a resync on a settled page (0 → 4 moves in one step), a draw offer with a held move (offer after the moves), review while a move is held, then the opponent's move + resignation during the review (back to live, move, card). No QML warnings. ChessTests 111, ChessQtTests 119 pass in Debug and Release; 49 screenshots match, `robot` included.

#### Where to continue
- **Manual checks by the owner with a real Lichess game** (plan §5 rows 8 and 9): the opponent answers instantly (our slide, then theirs); reconnect with several moves (the position at once, one sound); the opponent mates us (the move slides, then the card); review while the opponent moves.
- Steps 3–5 manual checks are still open (see the step-3 entry).
- Step 6 (render-thread slides) only if the step-1 numbers still show stalls.

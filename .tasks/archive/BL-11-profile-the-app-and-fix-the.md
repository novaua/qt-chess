---
id: BL-11
title: "Profile the app and fix the top inefficiencies"
type: task
status: done
priority: medium
---

# BL-11 Profile the app and fix the top inefficiencies

<!-- migrated from TASKS.md:91 (In Progress / Backlog) -->

- [x] BL-11 Profile the app and fix the top inefficiencies [Medium]: **Done (step 3, branch `features/moveProfile_121126`): scene `play` (clicks and a drag as mouse events, a takeback, the review); Release has no stall in play; Debug: the redundant signals behind each move and review step removed (a move's QML ~100 → ~70 ms, a selection's dots 8.5 → 2.3 ms, the review steps without the dots' 3.5 ms).** **Step 1 done (branch `features/profiling_111126`): a `[Timing]` line in the app log for every game start, scene `timing`, a QML profile; fixed the flag glyph (361 ms of the first game page) and Home's photo decode (64 ms per return to Home); numbers in `PROGRESS.md`. Found for BL-16: leaving a robot game while its first answer is starting freezes the window 0.8 s in `stopEngineThread()` (up to 3 s by design; fixed in BL-16). **Step 2 done (branch `features/gamePageSpeed_121126`): the game page builds in 70–110 ms instead of 153–198 (Release; Debug about 0.6–0.8 s to the first frame instead of 2 s): the board's layers made once at their real size (`LayoutGate`), one item per square highlight, the edge labels fitted once, the More menu and the hidden moves panel on demand, the piece images kept decoded; the scene graph checks: a move changes only its piece, and an idle game page draws 60 frames a second only for the active player's avatar pulse (117 ms of CPU per second; kept, the owner likes it).** opening the game page takes up to a second, and we don't know what redraws during a game. To do: measure first, in Release with the deployed Qt: the time from the click (Home card, Continue, a setup dialog's Start) to the first frame of `GamePage` (log timestamps or a `QElapsedTimer` in `ChessConnector`), and a QML profile (Qt Creator's QML Profiler / `qmlprofiler`) of opening the game page and of a few moves, a drag, a takeback and the review (←/→). Check the scene graph with `QSG_VISUALIZE=changes|batches|overdraw` and `QSG_RENDER_TIMING=1` / `qt.scenegraph.time.*`: an idle board should not redraw, and a move should repaint only the squares and pieces it changes. Look for: bindings that re-evaluate on every move or on every `ChessConnector` signal (one wide `...Changed` signal that refreshes the whole board, a model reset in place of a row change), `Repeater`s that recreate their 64 delegates, page and dialog items created up front instead of on demand (`Loader`/`asynchronous`), images loaded or scaled at full size (`sourceSize`), effects/`layer.enabled`, timers or animations that keep running, and slow C++ calls from QML (move list, captured pieces, material, legal moves). Write the numbers before and after in `PROGRESS.md`; fix the biggest few; keep the 42 screenshot scenes matching.

## Resolution

### 2026-11-11 — BL-11: profiling, the top two fixes (branch: features/profiling_111126)

#### Measuring
- **A timing line in the app log, always** (category `chess.timing`, `GameStartTiming` in `frametrace.h`): from the action that starts a game (Start vs Robot / two players, Continue, a Lichess rejoin, an accepted challenge) to the game page's first frame, with its steps, e.g. `[Timing] vs Robot (level 3): 177 ms to the first frame (save 14, stop engine 0, setup 2, start engine 0, page 141, first frame 20)`. `ChessConnector` marks the steps, `GamePage` reports itself (`gameShown`). Also `[Timing] engine stop: the GUI thread waited N ms` when `stopEngineThread()` waits over 20 ms. (BL-16 asked for this line.)
- **Scene `timing`** (not in `all`): from Home, 1.5 s apart, as clicks in a running app: vs Robot, leave, Continue, leave, two players, leave, vs Robot as Black and leave while the engine thinks. `Take-Screenshot.ps1 -KeepLog` keeps a scene's log also when it succeeds; with `QT_LOGGING_RULES="chess.frames.debug=true"` the log has `FrameTrace`'s "GUI thread busy" lines too.
- **QML profile**: `qmlprofiler` on a Release build with QML debugging switched on for the run only (not committed: it lets the app open a debug port), summed per file and line.

#### Found and fixed
1. **The flag on a resigned or flagged king** (`ChessFigures.qml`): a `Text` "⚑" in each of the 64 piece items, hidden but created. The glyph isn't in the UI font, and the font fallback search took about 6 ms per item: **361 ms of the first game page's 514 ms** (once per session; later pages had it cached). Now a `Loader`, made only on the flagged king's square: the same look.
2. **Home's photo** (`ChessBackground.jpg`, 3840 × 2160): decoded at full size each time Home appeared (33 MB, too big to stay in the image cache): **64 ms of every return to Home**. Now one image in the window behind the pages (`ChessGame.qml`), shown under Welcome, the profile picker and Home (their backgrounds are transparent): decoded once, at full quality and any scaling; the pictures are the same to the pixel.

#### Numbers (Release, this PC, the `timing` scene, two runs each)
| | before | after |
|---|---|---|
| The session's first game start, to its first frame | 507–645 ms (page 466–604) | 177–218 ms (page 141–172) |
| Later game starts | 131–171 ms | 131–176 ms |
| Leaving a game (GUI thread busy) | 86–264 ms | 47–95 ms |
| Engine stop while it thinks (GUI thread waits) | 42–74 ms | 37–49 ms |
| Engine stop while its first answer is starting (GUI thread waits) | — | **755–829 ms** (see below) |

A scene at app start (cold, the window's first frames too): two players 670 → 366 ms, vs Robot 775 → 438 ms, a Lichess rejoin 691 → 240 ms.

#### Left (for BL-16 and later)
- **The likely "up to 3 s"**: leaving a robot game right after the player's move, while the robot's first answer is starting, the GUI thread waits **755–829 ms** in `stopEngineThread()` (`_engineThread->wait(3000)`: the engine thread is in its UCI start, not in its event loop). The window is frozen meanwhile, also when a new game starts (it stops the old engine first). Its limit is 3 s; a slower PC or a Debug build comes closer to it. For BL-16: stop the engine off the GUI thread (let the old thread finish on its own, detached, as when it doesn't stop in time), so a click never waits for it. The timing line and `[Timing] engine stop` are in every log now.
- **The code review of #147** (10 findings, all fixed): a start that fails or is dropped (a refused Accept, a failed rejoin, Back, a declined rematch, `endGame()`) cancels its timing, and one older than a minute no longer counts; a rematch is timed; Continue's engine start is its own step; on a game page that was open already the page isn't a step (it was recorded before the setup); "first frame" is the first frame synchronized after the page is up (not one already being drawn), and a window that never draws ends the timing after 5 s; `GamePage` reports once it is in the window; the photo (above); the `timing` scene plays only the player's move before leaving (the scripted reply could race the robot's own).
- **A game page still takes 110–170 ms to create** (warm). Of its 81 ms of QML: the board's square-size binding (`ChessBoard.qml:14`, 25 ms: the board's width and height settle in four steps, each moving every piece) and the 64 piece `Image`s (14 ms). Worth it only with BL-16's busy state.
- **Not done**: the scene graph checks (`QSG_VISUALIZE`): idle boards and per-move repaints; the move, drag, takeback and review profile.

### 2026-11-12 — BL-11 step 2: the game page faster to build, the scene graph checks (branch: features/gamePageSpeed_121126)

#### Measuring
- The `timing` scene (Release and Debug), and a QML profile of it (`qmlprofiler` on the Debug build, which has QML debugging on), summed per file and line over each game start (the route() handler that builds the page and the 1.3 s after it). The profile lists each range twice; the self times are right.
- **Where the Debug build's 1.8 s went** (per game start, self time with the profiler on): the board's square-size binding **~535 ms** (`ChessBoard.qml:14`: the page passes through stand-in sizes while it is built, 200 px and then the real one, and each moved every square, piece, dot and label), the 64 squares with 6 highlight items each (a check-glow `Shape` among them) ~200 ms, the 32 edge coordinates with `Text.Fit` (each searches its own font size) ~135 ms, the More menu's items ~95 ms, the piece images ~78 ms (the 12 PNGs decoded again on every start), the second moves panel (both the side panel's and the sheet's were made).

#### Fixed
1. **The board's layers are made once, at the board's real size** (`LayoutGate`, new, `layoutgate.h/cpp`, a QML type in `ChessPlusPlus`): an item that opens in the first frame's polish pass, once the page's layout has settled; `ChessBoard`'s layers (squares, pieces, coordinates, dots, the dragged piece) are behind a `Loader` on it. Items made in the polish pass are polished in the same pass, so the first frame still has the whole board (the `rejoin` scene grabs that frame). The stand-in size couldn't be avoided in the bindings: `boardSize` is evaluated before `freeH` has caught up (`boardSize 200 … freeH 0`, then 736), and a `RowLayout` around the board passed the 200 too.
2. **The square highlights are one item each** (`ChessBoxes.qml`: last move ×2, premove ×2, check glow, flagged king, selected, drop square), moved to their square, in the order they had on a square; the 64 squares are plain rectangles.
3. **Only the shown coordinates are made** (around the board or on the squares), and the edge labels' size is fitted once (one hidden `Text.Fit`, its `fontInfo.pixelSize` for all 32; checked at 12 sizes: the same size and layout as each label fitting itself).
4. **The More menu is made on its first opening** (`GamePage.openMoreMenu()`; scene `more-menu`, new), and **only the shown moves panel** (side panel or sheet), behind the `LayoutGate` too.
5. **The piece images stay decoded for the session** (`ChessGame.qml`: 12 hidden images, made after the window's first frame): Qt's image cache keeps about 2 MB of images no item shows, less than the pieces' 4.3 MB, so each game page decoded them again (the first of each kind 4–7 ms in Debug, the repeats ~1 ms).

#### Numbers (this PC, the `timing` scene: the session's first start (vs Robot), Continue, two players, vs Robot again)
| | before | after |
|---|---|---|
| Release, to the first frame | 222–238, 203–220, 171–172, 217–231 ms | 124–136, 102–108, 82–93, 113–124 ms |
| Release, page + first frame | 153–198 ms | 70–110 ms |
| Debug, to the first frame | ~2000 ms (continue 1949, two players 2023) | 602–773 ms |

The `[Timing]` line's steps moved: most of the build is now in `first frame` (the layers are made in its polish), `page` is 14–29 ms in Release. Settings (52 ms) and the Online page (22 ms) unchanged. All 198 screenshots (light and dark) match the baselines from `develop`.

#### Scene graph
- **During a move** (`QSG_VISUALIZE=changes`, grabbed 100 ms into a slide): only the sliding piece changes. **Batches**: the 64 squares are one batch; each piece is its own (its own texture: 32 small draws). **Overdraw**: nothing unusual.
- **An idle game page draws 60 frames a second**: the active player's avatar pulse (`PlayerCard.qml`, an infinite animation) is the only cause; with it off, an idle board draws nothing (checked with `qt.scenegraph.time.renderloop`). It costs **117 ms of CPU per second** (Release). As `OpacityAnimator`s it was no lighter (the GUI thread still synced 30 times a second; 128–133 ms), so **the owner kept it as it is** (2026-11-12: "I like this effect"). The check glow pulses the same way while a king is in check.

#### Code review (9 findings, no definite bug)
- **Fixed**: each moves panel is kept once made (a window resized across the wide/narrow size rebuilt it, and lost its scroll); the edge coordinates' Loader fills the squares, so they no longer repeat the squares' geometry in an item of their own; a redundant `anchors.fill` gone.
- **Not changed, and why**:
  - The gate opening before a layout settles: nothing lays the board out in the polish pass. Its size comes from bindings, which have settled before the first polish (a probe saw only the two sizes during the build, both before the gate opened).
  - The piece images decoded synchronously after the first frame (~10 ms in Release, 50–80 ms in Debug): asynchronous, a game page made while they load (the scenes start one at once) would wait for them and miss its pieces in the first frame.
  - The layers not made while the window is minimized: they come with its first frame; nothing outside the board uses them.
  - The More menu's width on its first opening: the `more-menu` scene opens it the first time and it sits right.
  - The rank labels fitted in the files' box: a rank digit is less than half its box wide at that size; checked at 12 sizes.
  - Making the sheet's panel only when the sheet opens: that opening would then build it during the sheet's slide.

#### Left
- The QML profile of a move, a drag, a takeback and the review (←/→): not done (the frame trace of BL-14 covers the slides).
- Smaller still, if wanted: the 64 move dots each have a wrapper item and an animation (~20 ms per start in Debug; keeping their whole-pixel centring without the wrapper needs Qt's own rounding), the pieces' 14 objects each.

### 2026-11-12 — BL-11 step 3: the move, drag, takeback and review profile (branch: features/moveProfile_121126)

#### Measuring
- **Scene `play`** (new, not in `all`, needs Stockfish): a game vs the robot (level 1) played as a user plays it, with animations on: Ng1–f3 by two clicks, the robot's answer, e2–e4 by a mouse drag, a takeback (Ctrl+Z's), then the review (←, ←, →, →, Home, End). The clicks and the drag are mouse events sent to the window (`SceneInput`, new, `sceneinput.h/cpp`, registered only in the screenshot mode), so they go through the board's `TapHandler` and `DragHandler`. `[Play]` lines in the log mark the steps; with `QT_LOGGING_RULES="chess.frames.debug=true"` the frame trace is between them.
- **Release: nothing to fix.** No slow frame and no busy GUI thread from the first click to the end, before or after the fixes below; the board's own work is 0.1–4 ms per action.
- **Debug, before**: a move 26–47 ms of GUI thread with a frame or two dropped (28–69 ms); the takeback and each review step 17–23 ms of QML, three of the seven over 25 ms of GUI thread; a selection's dots 8.5 ms. The QML profile per step showed redundant work: the 32 pieces' `movable` binding 192 times per move (`MovableColor` passed on seven signals of a move as its own, changed or not), the 64 dots' colour and visibility on every review step with no dots shown (clearing them signalled anyway, 3.5 ms in C++ each time), and the click's own call 14.3 ms.

#### Fixed
1. **`MovableColor` is signalled once for all the signals of one move** (`ChessConnector::scheduleMovableColorChanged`, queued like the pieces' sync): the same seven signals as before, one `movableColorChanged` after them. Always sent, not only when the value differs (the code review: some of what it reads changes without a signal, so an "unchanged" could hide a change that a piece made in between has seen).
4. **A click that changes neither the dots nor the selection doesn't signal** (`figureSelected`: an empty square with nothing selected).
2. **Clearing the move dots does nothing when there are none** (`clearPossibleMoves`): no `PossibleMovesChanged`, so the 64 dots aren't evaluated again.
3. **The dots' colour once for the layer** (`NavigationLayer.dotColor`), not in each of the 64.

#### Numbers (Debug, the `play` scene)
| | before | after |
|---|---|---|
| A move with the robot's answer, QML | ~100 ms | ~70 ms |
| The click's own call (move, dots, signals) | 14.3 ms | 3.9 ms |
| A selection's dots | 8.5 ms | 2.3 ms |
| The takeback and the 6 review steps: GUI thread over 25 ms | 3 of 7 (28–30 ms) | 1 of 7 (Home, 26 ms) |
| Clearing the dots on each of those steps (C++) | 3.5 ms | none |

A move is still 40–48 ms of GUI thread in Debug. What is left is the move itself: the slide's frames, the new move-list row (its figurine with a `MultiEffect` glow, BL-02), the move sound, and the Fluent style recolouring the review buttons' icons when they turn on.

#### Checks
- All screenshots (light and dark) match; `premove-robot` still plays the premove after the robot's answer.
- After the code review's fixes (the screen was locked, so offscreen): the same 12 game scenes (select, drag, the three premove scenes, check, review, …) before and after are the same to the pixel.

#### Code review (8 findings)
- **Fixed**: the `play` steps wait for what they need (the board's layers, the robot's answer shown and its slide done: `isBoardIdle()`) instead of fixed times, so a slow Debug build or a cold engine can't play e4 as a premove or start before the page; the `MovableColor` signal is coalesced instead of cached (above); `figureSelected` signals only on a change; a click's release comes 50 ms after its press.
- **Not changed**: QML never writes `PossibleMoves` (the early return also needs no selection); one flag instead of the 64-entry check would have to be kept right in more places; the synthetic events' timestamps can't mix with real input, which the screenshot window doesn't take (`WindowTransparentForInput`).
- **Found on the way**: with the screen locked, a scene's QML `Timer`s never fire (no frames, no animation timer), and the scene times out without a message. Use `-Offscreen` then, as for the screenshots.

#### Left (not BL-11)
- Each move-list row has its own `MultiEffect` layer for the figurine's glow: fine now, but a long game has one per move. A pre-drawn glow image would do the same with less, if it ever shows up in a profile.
- The avatar pulse keeps an idle game page drawing every frame (BL-11 step 2; kept, the owner's call).

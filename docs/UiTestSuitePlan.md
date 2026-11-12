# UI Test Suite — Plan (BL-05)

| | |
| :---- | :---- |
| **Date** | 2026-10-28, refreshed 2026-11-12 (after BL-11) |
| **Status** | Layer 2 (screenshots and their diff) done; Layer 1: step 2 done (branch `features/uiTestsAppServices_121126`): `AppServices`, `ChessGuiTests` with scenarios 1–3, `Run-UiTests.ps1`; step 3 done too (branch `features/fakeUciEngine_121126`): FakeUci, the robot test, the robot scenes on it; step 4 in three parts: 4a (the board) done (branch `features/uiTestsBoard_121126`), next 4b (settings, profiles), 4c (Lichess). The owner chose the recommended answers to §10 (2026-11-12) |
| **Branch** | one per step from `develop`, e.g. `features/uiTestsAppServices_<DDMMYY>` |

## 1. Problem

What is tested today:

- `ChessTests` (CppUnitTest, no Qt): the chess logic.
- `ChessQtTests` (CppUnitTest, Qt): the C++ behind the UI without QML: `BoardPieces`, `MoveListModel`,
  `MoveGate`, `GameClock`, `ClaimWin`, `UserManager`, the UCI connector, Lichess against `FakeLichess`.
- **Screenshot scenes** (`--screenshot`, about 100 scenes, `Take-Screenshot.ps1`) and their diff against
  local baselines (`Compare-Screenshots.ps1`, 202 shots light and dark). `SceneInput` (BL-11) sends real
  mouse clicks and drags through the board's handlers, in the `play` scene.

What is missing: nothing **checks behaviour through the QML**. A scene sets a state and takes a picture;
it asserts nothing. If a click stops reaching `ChessConnector`, a binding breaks, a dialog no longer
opens or the move list misses a move, we only notice if a picture changes and someone reads the diff,
or by hand. `ChessConnector` itself (2200 lines, the game flows) has no tests at all, since it needs the
QML around it.

Goal: a **local pre-PR suite** that plays the app as a user does and checks the result, runs headless
(also with the screen locked), in about a minute; later the same suite in GitHub's `tests.yml`.

## 2. Approach: three layers

| Layer | Tool | Runs | Purpose | State |
|---|---|---|---|---|
| 1. Behaviour | **Qt Quick Test** (in Qt: `Qt6QuickTest`, `qmltestrunner`) | `ChessGuiTests.exe`, offscreen | click, drag, check state and items | **to do** |
| 2. Visual | screenshot scenes + golden-image diff | `ChessGui.exe --screenshot`, real transparent window | layout and look | done |
| 3. Smoke | pywinauto (UI Automation) | the installed app on the desktop | packaging and deployment | optional |

**Why Qt Quick Test, not CppUnitTest:** one `QGuiApplication` per process and a QML engine don't sit well
inside the `vstest` host next to the other tests; Qt Quick Test is made for this (`TestCase`, `mouseDrag`,
`tryCompare`, `findChild`, one engine per test file) and its tests read like the QML they test. The cost:
`ChessGuiTests` is an exe, not in Test Explorer; `Run-UiTests.ps1` runs it (§8).

**Why not UI Automation as the main suite:** it takes over the real mouse and keyboard, needs an unlocked
visible desktop, can't run headless, and the board still needs computed coordinates. Kept for a few
optional smoke tests (§6).

## 3. Layer 1 — the `ChessGuiTests` project

- **A console exe** in `ChessCore.sln`, Qt modules `core gui qml quick quickcontrols2 multimedia
  network quicktest`, references `ChessCore`, `UciConnector`, `LiChessConnector`. It builds the
  ChessGui sources **except `main.cpp`**, plus `qml.qrc`, the way `ChessQtTests` already builds some of
  them (§10 Q1).
- **`main` is `QUICK_TEST_MAIN_WITH_SETUP(ChessGuiTests, TestSetup)`**; the tests are
  `ChessGuiTests/qml/tst_*.qml`.
- **`AppServices` (new, in ChessGui)** takes the app's wiring out of `main.cpp`: it creates
  `ChessConnector`, `AvatarProvider`, `UserManager`, `LichessClient` and `AppInfo`, connects them (the
  token to the client, account, ongoing games, `isOnlineGameChanged`, `gameEnded`) and registers the
  QML singletons and `LayoutGate`. `main.cpp` and the tests both use it, so the tests run the app's own
  wiring, not a copy. The screenshot-only parts (`seedScreenshotProfiles`, the `play*Scene` functions,
  the grab) stay in `main.cpp`. No behaviour change: all screenshot scenes must still match.
- **`TestSetup`** (test project): `applicationAvailable()` sets the style (`FluentWinUI3`), the
  organisation names and a fresh data folder (`QTemporaryDir` → `AppConfig::setAppDataDir`, `QSettings`
  ini path), as `--data-dir` does; `qmlEngineAvailable()` makes the `AppServices`, with Lichess
  offline (`setOffline(true)`).
- **One process per test file**: `Run-UiTests.ps1` runs `ChessGuiTests.exe -input tst_x.qml` per file,
  so each file starts on a new profile with new services (`qmlRegisterSingletonInstance` is global; a
  second engine in the same process would see the first one's state). The test functions of one file
  share the app and run in order; each file's `init()` brings it back to Home.
- **The app in a test:** a `TestCase` creates `qrc:/qml/ChessGame.qml` (`createTemporaryObject`), waits
  for `windowShown` and for the first page. Reduce animations on (`Theme.forceReduceMotion`, as in the
  scenes), except in a test about slides or dragging.
- **Helpers** (`ChessGuiTests/qml/AppDriver.qml`, used by every file): `page()` (the StackView's top,
  by `objectName`), `createProfile(name, avatar)`, `startTwoPlayerGame()`, `squarePoint("e2")` (from
  `ChessBoard`'s `squareX/squareY`, right for a turned board too), `clickSquare`, `dragPiece(from, to)`
  (`mousePress` / `mouseMove` / `mouseRelease`), `waitBoardIdle()` (`ChessConnector.isBoardIdle()`).
- **What a test checks:** `ChessConnector`'s properties (`MoveCount`, `Pieces`, `SelectedSquare`,
  `PossibleMoves`, the result), and the items a user sees (visible, text, where a piece is). No pixels:
  that is Layer 2.
- **`objectName`s** are added as tests need them, camelCase, unique on their page. Today there are about 20
  (the pages: `home`, `game`, `settings`, …; `startNotice`, `challengeBar`, `robotGameCard`, …).
- **Offscreen** (`QT_QPA_PLATFORM=offscreen`): runs with the screen locked and in CI. Fluent's colours are
  always dark there, which a behaviour test doesn't care about. Dialog backgrounds don't render
  offscreen; their items still exist and take clicks.
- **The QML ↔ C++ rule** stays: QML talks to the same five singletons; nothing test-only in the pages.
  A test-only hook, if one is ever needed, goes on `ChessConnector` next to the screenshot ones
  (`setScreenshotClock`, …) (§10 Q2).

## 4. Stub engine

Games vs the robot need Stockfish today: its answers vary and take time, and `tests.yml` has none. A
**`FakeUci`** console project (links `ChessCore`) answers `uci`, `isready`, `ucinewgame`, `position`,
`go` with `bestmove`: the next move of a script if one is given (`FAKEUCI_MOVES="e7e5 b8c6"`), else the
first legal move in a fixed order, after an optional delay (`FAKEUCI_DELAY_MS`). `UciConnector` looks in
`./engines/` then on `PATH`; an override (`CHESS_ENGINE_PATH`) points the app at the stub.

Also useful outside Layer 1: the `robot`, `premove-robot` and `play` scenes, and `ChessEnginePlayerTest`,
could run without Stockfish and with the same moves every time (the plan's old step 1 "Open" item).

## 5. Layer 2 — screenshots (done)

`--screenshot` scenes, `Take-Screenshot.ps1`, `Compare-Screenshots.ps1`, `SceneInput`: see the
`run-chess-app` skill. Local only (fonts and scaling differ per PC). Layer 1 doesn't replace it; the
pre-PR run does both (§8).

## 6. Layer 3 — pywinauto smoke tests (optional, unchanged)

3–5 tests against the built Release exe or the installed app: it starts, Home's cards are there, a
two-player game, one move, a clean exit. Needs `Accessible.name` on the controls (screen readers gain
too). Python is test tooling only. Before releases, not before every PR.

## 7. Determinism

| Source | Fix |
|---|---|
| The real profile (`users.json`, settings, saves) | a fresh data folder per test file (`TestSetup`), as `--data-dir` |
| Stockfish's answers and timing | the stub engine (§4) |
| Random avatars | profiles made by the test with a fixed avatar; the opponent's as in the scenes |
| Lichess | offline (`setOffline`); later `FakeLichess` (already in `ChessQtTests`) via `setBaseUrl` |
| Animations | Reduce animations on; `tryCompare` / `waitBoardIdle()` where they're on |
| The clock | games without a clock unless the test is about the clock |
| Window size | fixed by the test (`window.width/height`), as `--size` |

## 8. Running

- **`scripts\Run-UiTests.ps1`**: builds `ChessGuiTests` (and `FakeUci`), runs each `tst_*.qml` in its
  own process offscreen with Qt's `bin` on `PATH`, writes `test-results\ui\<file>.txt` (and JUnit XML,
  `-o file,junitxml`), prints a summary; exit code = failed files. `-Name tst_game*` for some.
  `-Screens` also takes all screenshots, light and dark, and compares them: the full pre-PR check.
- **GitHub** (`tests.yml`): Layer 1 after the CppUnit tests, offscreen, with the stub engine; never the
  screenshots (§10 Q3).
- **Docs:** CLAUDE.md "Running Tests", and a section in the `run-chess-app` skill.

## 9. Scenarios

First batch (step 2):
1. **First run:** Welcome → create a profile → Home with its cards.
2. **Two players, a drag:** e2→e4 dragged: the pawn on e4, last-move highlight on e2 and e4, "1. e4"
   in the move list, Black to move; the black pieces now draggable, the white ones not.
3. **Fool's mate by clicks:** f3, e5, g4, Qh4#: the result card (Black wins), the flagged king,
   Rematch / New game.

Later (step 4):
4. A drag to an illegal square: the piece goes back, nothing played.
5. Promotion: the dialog, a knight chosen (White and Black).
6. Takeback, and the review: ←, →, Home, End, a click on a move.
7. vs the stub robot: it answers; a premove is played after its answer; leaving while it thinks.
8. Save, back to Home, Continue: the same position. Two unfinished games: the Continue list.
9. A setting survives a restart (a second process on the same data folder).
10. Profiles: create, switch during a game (the confirmation), the picker with two profiles.
11. Messages: "Check" over the command bar, the recent messages.
12. Lichess against `FakeLichess`: a challenge on the bar, Accept → the game page with its moves.

## 10. Questions for the owner

**Answered 2026-11-12: the recommended option for all four** (Q1 a, Q2 yes, Q3 a, Q4 a).

- **Q1 How `ChessGuiTests` gets the ChessGui code.** (a) **Recommended:** list the ChessGui sources in
  the test project, as `ChessQtTests` does today; a new ChessGui `.cpp` must then go into both (the
  test build fails to link if it's forgotten, so it can't slip). (b) A ChessGui static library used by
  `ChessGui.exe` and both test projects: one list, but a bigger change to the build, the `.pro` file and
  the qrc (`Q_INIT_RESOURCE`), and it touches the release build.
- **Q2 The test setup outside ChessGui** (the old plan's open question). **Recommended: yes**, since
  `TestSetup` only calls `AppServices`, the same code `main.cpp` runs; the rule (game flows through
  `ChessConnector`, services as typed singletons) holds for the tests as for the app.
- **Q3 Layer 1 on GitHub.** (a) **Recommended:** on every push, as the CppUnit tests, if the run stays
  under ~2 minutes (offscreen, stub engine). (b) Only by hand (`workflow_dispatch`) or a PR label.
- **Q4 The stub engine.** (a) **Recommended:** its own step after the first scenarios (step 3): the first
  batch needs no robot. (b) In step 2, so the first batch has a game vs the robot too.

## 11. Steps

1. **Foundation — done:** `--data-dir`, `--screenshot` with ~100 scenes, `Take-Screenshot.ps1`,
   `Compare-Screenshots.ps1` (old step 4, BL-02 P6), `SceneInput` (BL-11).
2. **`AppServices` out of `main.cpp`** (all scenes still match), **the `ChessGuiTests` project** with
   `TestSetup`, `AppDriver`, scenarios 1–3, **`Run-UiTests.ps1`**; docs.
   - Done (`features/uiTestsAppServices_121126`): `tst_firstRun.qml` (Welcome → a profile → Home) and
     `tst_twoPlayers.qml` (Fool's mate by clicks → the result card; New game → e2–e4 dragged: the pawn
     on e4, the last move, "e4" in the move list, Black to move), 8 test functions in ~6 s.
   - Learned on the way: the window is made with `createObject` and destroyed in `cleanupTestCase`
     (`createTemporaryObject` would destroy it after each test function); a `ListView`'s rows are found
     only by the visual search, so `AppDriver.find` starts at the window's root item; a dialog still
     fading out takes the clicks with its modal overlay (`waitClosed`); keys from a test go to the
     test's own window, so a `TextField` gets its text set; a single test function on the command
     line is `<TestCase name>::<function>` (the runner adds the name); the qmake module is `qmltest`; `waitForRendering` on a page that is already drawn waits out its whole timeout (5 s), so it isn't used.
3. **The stub engine** and `CHESS_ENGINE_PATH`; scenario 7; the robot scenes on the stub.
   - Done (`features/fakeUciEngine_121126`): `FakeUci` (console exe on `ChessCore`, no Qt; 10 tests in
     `ChessTests`), `UciConnector` takes `CHESS_ENGINE_PATH`, `TestSetup` sets it and `TestRun.setEngine`
     scripts the robot; `tst_robot.qml`: it answers, a premove made with clicks while it thinks is played
     after its answer, Home while it thinks (at once, its answer dropped), Continue. `robot` and
     `premove-robot` are now the same every time and in `all`; `-Stockfish` for the real engine.
   - **Found by the robot test:** a `MouseArea` over the board while the robot thought (from BL-02 P2) took
     every click and drag, so a premove against the robot could only be made by the scenes (which call
     `figureSelected`), never with the mouse. Removed: `ChessConnector` already plays nothing else then.
   - Learned: `MoveGeneration` makes castling and en passant only after a first move, so FakeUci puts one
     into its history after a FEN; a slow engine outlives the test by its "thinking" and holds the
     results file it inherited (the runner retries reading it).
4. **The other scenarios** (4–6, 8–12), `objectName`s as needed. In three parts:
   - **4a, the board — done** (`features/uiTestsBoard_121126`): `tst_board.qml` (a drop on an illegal
     square goes back, a black piece can't be dragged on White's move; "Check" over the command bar and
     the king marked; the review from the move list and its ‹ › ⏭ buttons; Save, then "Open saved game",
     which is a replay from the start: stepped through to the saved position), `tst_promotion.qml`
     (White and Black each choose a knight; nobody moves while the piece is chosen), and in
     `tst_robot.qml` the takeback (two moves on the player's turn, one while it thinks, its answer never
     played; a two-player game has no Undo button). 26 test functions in all.
   - Learned: `MovableColor` reaches QML queued, so a press right after a change (e.g. `applyMoves`)
     found the pieces not draggable: `waitBoardIdle()` and the input helpers now let queued signals
     through first. The board is never idle in review (the gate holds), hence `waitPiece`. In a Debug
     build the board's layers come a frame after the page (`LayoutGate`): `pieceAt` says "?" until then.
   - 4b: a setting after a restart (a second process on the same data folder), profiles (create, switch
     during a game, the picker). 4c: Lichess against `FakeLichess` (the challenge bar, Accept, the game
     with its moves; scenario 12) and messages from Lichess.
5. **GitHub job** (Q3).
6. Optional: pywinauto smoke tests.

# ChessGuiTests/

The UI tests (BL-05, `docs/UiTestSuitePlan.md`): **Qt Quick Test**, a console exe, not in Test Explorer.
`qml/tst_*.qml` play the app's own window (`ChessGame.qml`) with real mouse events, offscreen (also with the
screen locked), and check its state and items; items are found by `objectName`. It builds every ChessGui `.cpp`
except `main.cpp`: **a new ChessGui source goes into `ChessGuiTests.vcxproj` too** (the link fails without it).

## Running
```powershell
.\scripts\Run-UiTests.ps1 -Build                                   # build ChessGuiTests (Release), run every file
.\scripts\Run-UiTests.ps1 -Name tst_twoPlayers -Function test_2_newGameAndDrag   # one function
.\scripts\Run-UiTests.ps1 -Build -Screens                          # + all screenshots and the comparison: the pre-PR check
.\scripts\Run-UiTests.ps1 -Name tst_twoPlayers -Visible            # the owner watches: a real window, 500 ms per action (-StepDelay)
```
- Release is the default: without `-Build` an old Release exe runs; `-Configuration Debug` for the Debug build.
- Results in `test-results\ui\<file>.txt` / `.xml`, failures printed; exit code = failed files. A QML
  `TypeError` or `ReferenceError` in a file's log fails it too, even with every test passed (BL-38). A failed test
  function leaves `test-results\ui\<TestCase>-<function>.png`, the window at the failure (offscreen: dark),
  listed under the failure: Read it.
- `-Visible` puts windows on the owner's screen and, after a failure, waits until they close the window: for the
  owner, not for an agent's runs.
- GitHub runs them on every push (`tests.yml`, Debug, ~95 s); a failed run uploads `test-results\ui` as the
  artifact `ui-test-results` (logs and pictures): `gh run download <run id> -n ui-test-results`.

## How a run is set up
- One test file per process on a fresh profile (`TestSetup`, `testsetup.h/cpp`, which uses the app's
  `AppServices`): a temporary data folder, or `CHESS_UI_TEST_DATA_DIR` (`Run-UiTests.ps1` sets it, so a
  restart's second run finds the first's data); `main.cpp` sets the app's style and names.
- `// ui-test-runs: 2` in a file runs it twice on one data folder: a restart (`tst_restart.qml`); each function
  skips itself in the other run (`TestRun.run`).
- `// ui-test-engine: none` runs it without an engine (`tst_noEngine`).
- `TestRun` (`TestSetup` as a QML singleton, module `ChessGuiTests`, with `FakeLichess`): `linkLichess`,
  `addFinishedGames(n)` (fills the games library without playing), `addLichessGame` (a kept Lichess game,
  LI-14), `addDamagedGame`, `addEditedGame`,
  `editRatingFile`, `setEngine`, `readTextFile`, `clipboardText`, `filesIn`, `qmlCachePath`.
- The tests' compiled QML is in a cache folder of their own (`AppConfig::useOwnQmlCache()`, BL-39), never the
  app's: a run can't rewrite the QML of an app running beside it (`tst_firstRun` test 3).
- **The robot is FakeUci** (`FakeUci/CLAUDE.md`), no Stockfish: `app.startComputerGame("e2e4 e7e5 …", thinkMs)`
  gives it the game to follow and how long it thinks (`tst_robot.qml`). An empty `CHESS_ENGINE_PATH` counts as
  none (BL-37: a shell can hold it empty, and the app would then find Stockfish).
- **Lichess is `FakeLichess`** (QML singleton, `fakelichessserver.h`), a local server the test plays the other
  side on: `TestRun.linkLichess(FakeLichess.username)` links the profile; `challenge` (a challenge or ongoing game
  can be another variant, with a start FEN), `play`, `offerDraw`, `endGame`, `dropGameStreams`,
  `addOngoingGame`, `setPlayer` (a player's title and rating in gameFull), `pairSeek`, `refuseSeeks`, `refuseGameActions`; it checks
  what the app asked (`received("POST /api/…")`, `bodyOf`, `moves(gameId)`). See `tst_lichess.qml`.
  What it doesn't serve (the player's own challenges) a test gives as `LichessClient`'s answer, calling its
  signal (`LichessClient.challengeDeclined("Friend")`, `tst_lichess` test 16).

## Writing a test
- A `TestCase` in `qml/tst_<area>.qml` with an `AppDriver` (`app.start()` in `initTestCase`, `app.stop()` in
  `cleanupTestCase`); functions `test_1_…`, `test_2_…` run in name order and go on from each other; ten or more:
  `test_01_…`, or `test_10` runs second.
- `qml/AppDriver.qml` has the helpers: `clickMove`, `dragPiece`, `pieceAt`, `waitPiece`, `waitBoardIdle`,
  `waitMessage`, `startTwoPlayerGame`, `leaveToHome`, `scrollIntoView`, `chooseFromMenu`, `clickIn`, `find`,
  `waitForPage`, `createFirstProfile`, `pickProfile`, ….
- Give the items it needs an `objectName`.
- **Waiting right:**
  - `waitBoardIdle()` after moves. It also lets queued signals reach QML: `MovableColor` is sent queued, and a
    press before it lands finds the pieces not draggable yet.
  - In review or a replay the board is never "idle" (the gate holds outside moves): wait for what is shown,
    `waitPiece("e4", "P")` (`pieceAt` is "?" until the board's layers exist). White's pieces in capitals,
    Black's in small letters.
  - The moves panel (`review_next`, `analysisSummary`, the list) is made only after the game page's first frame,
    late in Debug (GitHub's build): wait until `app.find(…)` has the item before `click`, which looks it up once.
  - Menus: `chooseFromMenu(button, item)` waits for the menu to be open (a click during its enter animation
    misses). A button every dialog has (`acceptButton`): `clickIn(dialog, button)`.
  - `click` and `clickIn` wait for the window's layouts first (`laidOut()`): a button just shown in a `Flow` (a
    bar's actions) is placed only on the next polish.
- Keys can't be typed into the app's window from a test (they go to the test's own): set a `TextField`'s text
  instead.

## Offscreen
- Hover doesn't reach items.
- No shader effects (the software renderer): an item with `layer.effect` isn't drawn (`qml/CLAUDE.md`'s
  `MultiEffect` rule); dialogs' backgrounds aren't drawn.
- The Fluent style is always dark.

## Invariants
- The tests never reach lichess.org: Lichess is always `FakeLichess`.
- Never Stockfish: the robot is FakeUci (or no engine at all).
- Each file starts on a fresh profile; the owner's own data is never touched.

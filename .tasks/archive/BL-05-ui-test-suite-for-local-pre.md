---
id: BL-05
title: "UI test suite for local pre-PR sanity"
type: task
status: done
priority: medium
---

# BL-05 UI test suite for local pre-PR sanity

<!-- migrated from TASKS.md:85 (In Progress / Backlog) -->

- [x] BL-05 UI test suite for local pre-PR sanity (Qt Quick Test + screenshot diff, optional pywinauto smoke / CI job) — plan: [`docs/UiTestSuitePlan.md`](docs/UiTestSuitePlan.md) [Hard]. **Plan refreshed (2026-11-12, branch `docs/bl05Plan_121126`): Layer 2 (screenshots, diff, `SceneInput`) done; next: `AppServices` out of `main.cpp` and a `ChessGuiTests` project (Qt Quick Test, offscreen, one process per test file) with three scenarios, then a stub UCI engine. The owner chose the recommended Q1–Q4 (2026-11-12). Step 2 done (branch `features/uiTestsAppServices_121126`): `AppServices` (the services' wiring, out of `main.cpp`), the `ChessGuiTests` project (Qt Quick Test, offscreen) with `tst_firstRun` and `tst_twoPlayers` (8 functions: first run, Fool's mate by clicks, a dragged e4), `scripts\Run-UiTests.ps1` (`-Screens`: the pre-PR check). Step 3 done (branch `features/fakeUciEngine_121126`): FakeUci, the stand-in UCI engine (`CHESS_ENGINE_PATH`), `tst_robot` (answer, premove by clicks, leave while it thinks, Continue), the `robot` / `premove-robot` scenes on it and in `all`; fixed on the way: a premove against the robot couldn't be made with the mouse (a click blocker over the board). Step 4a done (branch `features/uiTestsBoard_121126`): `tst_board` (illegal drop, Check, review, saved game replayed), `tst_promotion` (both colours), the takeback in `tst_robot`; 26 test functions. Step 4b done (branch `features/uiTestsSettingsProfiles_121126`): `tst_restart` (runs twice on one data folder: a setting and an autosaved game kept), `tst_profiles` (add, switch during a game, each profile's own game); 38 test functions. Step 4c done (branch `features/uiTestsLichess_121126`): `FakeLichess` (a local Lichess for every test file, the test plays the other side), `tst_lichess` (a challenge declined and one accepted, moves both ways, a draw offer, the connection lost and back, the opponent resigns, an unfinished game continued with its moves, Leave / Resign); 50 test functions. Step 5 done (branch `features/uiTestsCi_121126`): the UI tests in `tests.yml` on every push (Debug, offscreen, ~95 s; the results uploaded on a failure). Step 6 done as the installer's smoke test (branch `features/installerSmokeTest_121126`, instead of pywinauto): every PR and release installs the installer on GitHub, runs the installed app on five screenshot scenes with only the installed files, and uninstalls it.**

## Resolution

### 2026-11-12 — BL-05 plan refreshed (branch: docs/bl05Plan_121126)

- `docs/UiTestSuitePlan.md` brought up to date: Layer 2 (screenshot scenes, `Compare-Screenshots.ps1`, `SceneInput`) is done; the old plan's context properties, "main menu" and "Load" are gone from the app.
- Layer 1 designed against today's code: `AppServices` (new) takes the services and their wiring out of `main.cpp`, so the app and the tests run the same code; `ChessGuiTests` (Qt Quick Test, `QUICK_TEST_MAIN_WITH_SETUP`) builds the ChessGui sources except `main.cpp`, runs offscreen, one process per test file (fresh data folder and singletons); `AppDriver` helpers (`squarePoint`, `dragPiece`, `waitBoardIdle`, …); `scripts\Run-UiTests.ps1`. A stub engine (`FakeUci`, links `ChessCore`) and `CHESS_ENGINE_PATH` come next.
- **The owner's answers (§10):** the recommended ones: the test project lists the ChessGui sources (as `ChessQtTests`); the test setup outside ChessGui calling `AppServices` is fine; the GitHub job on every push (if under ~2 min); the stub engine as its own step 3.
- **Next:** step 2 on a new `features/` branch once this PR is merged.

### 2026-11-12 — BL-05 step 2: the first UI tests (branch: features/uiTestsAppServices_121126)

#### Done
- **`AppServices`** (`appservices.h/cpp`, new in ChessGui): makes the five QML services, wires them (the Lichess token, account, ongoing games, `isOnlineGameChanged`, `gameEnded`) and registers them; `main.cpp` keeps only the options, the log and the screenshot scenes. No behaviour change: all screenshots, light and dark, match (202 compared; `about` / `about-dark` differ only by the build date).
- **`ChessGuiTests`** (new project, console exe, Qt Quick Test): builds the ChessGui sources except `main.cpp`, and `qml.qrc`; `TestSetup` gives each run a fresh data folder and the app's own `AppServices` (Lichess offline); one test file per process (a second file in the same run stops with a message). `qml/AppDriver.qml`: `start`/`stop`, `find`/`item`/`click`, `waitForPage`, `waitClosed`, `squarePoint`, `clickSquare`, `clickMove`, `dragPiece`, `pieceAt` (by where the piece is drawn), `waitBoardIdle`, `createFirstProfile`, `startTwoPlayerGame`.
- **Tests**: `tst_firstRun.qml` (Welcome → Create local profile → Home) and `tst_twoPlayers.qml` (Fool's mate by clicks: the queen on h4, White's king in check, "… checkmate", the result card; New game from it, then e2–e4 dragged: the pawn on e4 and e2 empty, last move e2/e4, Black to move, "e4" as the move list's first move). 8 functions, ~6 s offscreen (Release; Debug ~13 s). A copy with wrong expectations fails where it should.
- **`objectName`s** for them: `pages` (the StackView), `createUserDialog`, `nameField`, `createProfileButton`, `createLocalProfile`, `twoPlayersCard`, `twoPlayerDialog`, `startGameButton`, `resultCard`, `rematchButton`, `newGameButton`, `reviewButton`, `ply<N>` (the move list's cells).
- **`scripts\Run-UiTests.ps1`**: `-Build`, `-Name`, `-Function`, `-Timeout`; results in `test-results\ui\` (txt and JUnit XML); the failures printed; `-Screens` adds all screenshots and the comparison.
- Docs: CLAUDE.md (Running Tests, `AppServices`), the `run-chess-app` skill, the plan (step 2 and what was learned).
- **Seeing a failure** (the owner's ask): a failed test function leaves `test-results\ui\<TestCase>-<function>.png` (the window at that moment; the runner lists it under the failure). `Run-UiTests.ps1 -Visible`: a real window on the screen, 500 ms after each click or drag (`-StepDelay`), and after a failure the window stays with a red note until it is closed. The options reach the tests through `TestSetup` (QML singleton `TestRun`, from `CHESS_UI_TEST_*` set by the runner). Watched, the window is fitted to the screen and centred (it opened at 0,0 with its title bar off the screen: the owner's report), with Windows' scaling as in the app. Checked: the picture of a forced failure; the pause after it (offscreen, still waiting at 25 s); the owner watched a run.
- **5 s per page wait removed:** `waitForPage` called `waitForRendering`, which on a page already drawn waits its whole 5 s timeout for a frame that doesn't come (without failing): `tst_twoPlayers` 9.6–14.6 s → 4.6 s.

#### Next
- BL-05 step 3: the stub engine (`FakeUci`, `CHESS_ENGINE_PATH`) and a game vs the robot (scenario 7). Then step 4 (more scenarios) and step 5 (the GitHub job).

### 2026-11-12 — BL-05 step 3: the stub engine and the robot test (branch: features/fakeUciEngine_121126)

#### Done
- **FakeUci** (new project, console exe on `ChessCore`, no Qt): answers what the app asks of Stockfish (`uci` with the options `UCI_LimitStrength`, `UCI_Elo`, `MultiPV`; `isready`, `ucinewgame`, `setoption`, `position startpos|fen … moves …`, `go` with one `info … multipv 1 … pv` line, `quit`), at once and the same way every time: at ply N the move N of `FAKEUCI_MOVES` (a whole game in UCI) when legal there, else the first legal move by its UCI text; `FAKEUCI_DELAY_MS` before each answer. A FEN's castling field and en passant square are honoured; a promotion names its piece. 10 tests in `ChessTests/FakeEngineTests.cpp` (140 in all pass).
- **`UciConnector::EngineCandidates`**: `CHESS_ENGINE_PATH` names the only engine (also what Settings / About probe).
- **UI tests**: `TestSetup` points the app at `FakeUci.exe` next to the tests; `TestRun.setEngine(moves, delayMs)`, `app.startComputerGame(line, thinkMs)`. `tst_robot.qml` (6 functions): 1. e4 dragged, the robot thinks and answers e5; 2. Nf3, Bc4 premoved with clicks while it thinks (red squares), its Nc6, then the premove at once; Home while it thinks: at once, its answer dropped, the game kept; Continue: the moves back and its answer Nf6. All 3 files, 14 functions, ~16 s.
- **Screenshots**: `robot`, `premove-robot` and `play` run against FakeUci (`$engineScenes`: `robot` mates with Qh5#, `premove-robot` ends 1. e4 e5 2. Nf3 Nc6), so they are the same every time; `robot` and `premove-robot` joined `all` (new baselines). `-Stockfish` for the real engine.

#### Fixed (found by the robot test)
- **A premove against the robot couldn't be made with the mouse.** `GamePage` had a `MouseArea` over the board while the robot thought (since BL-02 P2: "no clicks on the board") that took every click and drag; LI-05 added premoves vs the robot later, and its scenes premove with `figureSelected` directly, so it never showed. Removed: `ChessConnector` already refuses other moves then (`canPlayerMove`) and turns a click or drop into a premove (`canPremove`). The wait cursor over the board went with it.

#### Notes
- `MoveGeneration` makes castling and en passant only after a first move (an empty history is the start position): after a FEN, FakeUci puts the other side's double step (when the FEN names an en passant square) or an empty move into its history.
- A thinking engine outlives the test process for its delay and holds the results file it inherited: `Run-UiTests.ps1` retries reading it.

#### Next
- BL-05 step 4: the other scenarios (illegal drop, promotion, takeback and review, save and Continue, a setting after a restart, profiles, messages, Lichess against `FakeLichess`). Then step 5, the GitHub job (now possible without Stockfish).

### 2026-11-12 — BL-05 step 4a: the board's UI tests (branch: features/uiTestsBoard_121126)

#### Done
- **`tst_board.qml`** (two players): e2 dropped on e5 goes back, nothing played; a black pawn can't be dragged on White's move; 1. e4 f6 2. Qh5+: "Check" over the command bar and Black's king marked; the review: 1. e4 from the move list (the position at move 1, no check shown), ‹ to the start, ›, ⏭ back to the game (the check again, Black to move); Save ("Game saved"), Home, "Open saved game": a replay at move 0 of 3, stepped with › to the saved position, the check at the end.
- **`tst_promotion.qml`**: 1. a4 … 5. bxa8 and 5... gxh1, the pawn dropped on the last rank, the promotion box, a knight chosen: a white knight on a8, a black one on h1; the move counts at the drop and nobody can move while the piece is chosen (`MovableColor` 0).
- **`tst_robot.qml`** + 2: Undo on the player's turn takes back its answer and the player's move; Undo while it thinks takes back the player's move only, and its answer never comes. (A two-player game has no Undo button.)
- All: 5 files, 26 functions, ~28 s in Release (three runs alike), ~52 s in Debug.
- `objectName`s: `undoButton`, `saveButton`, `prevButton`, `nextButton`, `review_first|prev|next|last` (the moves panel), `shortMessage`, `openSavedCard`, `promotionOptions`, `promoteQ|R|B|N`. Driver: `waitMessage`, `leaveToHome`, `waitPiece`, `settle`.

#### Learned
- **A press right after a change came before QML knew of it**: `MovableColor` is sent queued (BL-11), and the test's next press came in the same turn of the event loop, so the pieces weren't draggable yet (C++ already said they were). Only a test can be that fast; `waitBoardIdle()` and the input helpers now let queued signals through first. The first board test's illegal drop would have passed either way: the robot and promotion tests showed it.
- In review or a replay the board is never "idle" (the gate holds outside moves), so those checks wait for what is shown (`waitPiece`). In a Debug build the board's layers come a frame after the page (`LayoutGate`), so `pieceAt` is "?" until then.
- "Open saved game" is a replay from the start, not the game resumed.

#### Next
- BL-05 step 4b: a setting after a restart, profiles. Then 4c: Lichess against `FakeLichess`. Then step 5, the GitHub job.

### 2026-11-12 — BL-05 step 4b: a restart and profiles (branch: features/uiTestsSettingsProfiles_121126)

#### Done
- **A test file that runs twice** (a restart): the line `// ui-test-runs: 2` makes `Run-UiTests.ps1` run it twice on one data folder (made in %TEMP%, deleted after; results `<file>-run1.txt`, `-run2.txt`); `TestSetup` takes the folder from `CHESS_UI_TEST_DATA_DIR` and the run from `CHESS_UI_TEST_RUN` (`TestRun.run`).
- **`tst_restart.qml`**: run 1 creates Tester, turns "Highlight last move" off in Settings and plays 1. e4 e5 vs the robot, then the window closes with the game on (the app autosaves it: `~ChessConnector`); run 2 starts on Home as Tester, the setting off and its switch shown off, Continue brings the game back with the last move known but not marked. Run 2 alone fails (Welcome, no game).
- **`tst_profiles.qml`**: Switch user with one profile (the picker), Alice added with Add profile; Alice plays 1. e4 e5 vs the robot, Switch user from the game's More menu asks first ("Switch user?"), then the picker with both cards; Tester's Home has no game to continue; back to Alice, her game continues.
- All: 7 files, 38 functions, ~40 s in Release (three runs alike).
- `objectName`s: `settingsButton`, `accountButton`, `switchUserItem` (Home's and the game's menu), `moreButton`, `backButton` (Settings), `highlightLastMoveSwitch`, `switchUserConfirm`, `acceptButton` (every ConfirmDialog), `profileCard_<name>`, `addProfileButton`, `lastMoveFromMark` / `lastMoveToMark`. Driver: `clickIn`, `chooseFromMenu`, `addProfile`, `switchUserFromHome`, `pickProfile`, `lastMoveMarked`.

#### Learned
- A menu item clicked while the menu is still opening (its enter animation) is missed: `chooseFromMenu` waits for `menu.opened`.

#### Next
- BL-05 step 4c: Lichess against `FakeLichess` (the challenge bar, Accept, the game with its moves). Then step 5, the GitHub job.

### 2026-11-12 — BL-05 step 4c: Lichess against FakeLichess (branch: features/uiTestsLichess_121126)

#### Done
- **`FakeLichessServer`** (`ChessGuiTests/fakelichessserver.h/.cpp`, QML singleton `FakeLichess` in `ChessGuiTests`), built on `ChessQtTests/FakeLichess.h`: lichess.org on 127.0.0.1 as far as the app uses it: `/api/account`, `/api/account/playing`, the event stream (challenges, resent when it opens; `gameStart`, `gameFinish`), accept / decline, the game stream (`gameFull`, `gameState`), move, draw yes/no, resign; a keep-alive line every second. The test plays the other side: `challenge`, `play`, `offerDraw`, `endGame`, `dropGameStreams(id, downMs)` (cut, then 503 for a while), `addOngoingGame`; and checks `received("POST /api/…")`, `bodyOf`, `moves(id)`, `status(id)`.
- **`TestSetup`**: every test file's Lichess is the fake (not offline any more; without a token nothing is asked), short stream timings (retry after 0.2 s); `TestRun.linkLichess(username)` links the profile as the sign-in does (the OAuth page is lichess.org's own, not tested).
- **`tst_lichess.qml`** (12 functions): linked (rating, ongoing games, event stream); a challenge declined from the bar (`reason=generic`); one accepted from Home: the game page, the board turned for Black, the opponent's e4 in, our dragged e5 posted; a draw offer over the command bar, declined; a challenge during the real-time game (Decline only, "Finish your game first", Hide keeps it on Lichess); the stream cut: "Reconnecting to Lichess…", then "Reconnected to Lichess", a move still posted; the opponent resigns: the result card, Home without a question; an unfinished game continued from Home with its moves on the board, a move posted; Home during it: Leave the game? → Resign posted.
- All: 8 files, 50 functions, ~50 s in Release; `tst_lichess` five runs alike.
- `objectName`s: `challengeAcceptButton`, `challengeDeclineButton`, `challengeHideButton`, `statusMessage`, `offerAcceptButton`, `offerDeclineButton`, `acceptChallengeConfirm`, `leaveOnlineDialog`, `resignButton` (LeaveOnlineDialog).

#### Learned
- A bar's buttons (in a `Flow`) are placed on the next polish after it shows: a click before that missed Decline. `AppDriver.click` / `clickIn` now wait for the window's polish (`laidOut()`).
- Qt Quick Test runs the functions in name order: `test_10_…` runs before `test_2_…`; two digits from ten on.

#### Next
- BL-05 step 5: the GitHub job (Layer 1 in `tests.yml`, offscreen, FakeUci and FakeLichess: no Stockfish, no Lichess).

### 2026-11-12 — BL-05 step 5: the UI tests on GitHub (branch: features/uiTestsCi_121126)

#### Done
- **`tests.yml`**: a "Run UI tests" step after the CppUnit tests: `Run-UiTests.ps1 -Configuration Debug` on the Debug build the job already makes, offscreen; FakeUci and FakeLichess, so no Stockfish and no lichess.org. On a failure, "Upload UI test results" keeps `test-results\ui` (logs, the failed functions' pictures) as the artifact `ui-test-results`. `release.yml` calls `tests.yml`, so a release waits for them too.
- First run green: 8 files, 50 functions, 93 s on `windows-latest` (the whole job ~8 min, most of it Qt's install and the build). Locally in Debug 113 s.

#### Not checked
- The upload on a failure: the step only runs when something fails, and no run has failed yet.

#### Next
- BL-05 is done except the optional pywinauto smoke tests (step 6). The LI-04 / LI-02 / LI-05 hand tests on real Lichess are still open (`TASKS.md`).

### 2026-11-12 — BL-05 review fixes (branch: bugs/bl05ReviewFixes_121126)

A `/code-review medium` of all BL-05 commits found 8 problems, none a crash in the app; all fixed:
- **`Take-Screenshot.ps1`** left `CHESS_ENGINE_PATH` / `FAKEUCI_*` set in the shell (the app then played FakeUci), or removed the shell's own: the values are saved and put back (`finally`), and the non-robot scenes get the shell's own. Without `FakeUci.exe` the robot scenes are skipped with a warning instead of stopping `-Scene all`.
- **`tst_robot`** test 3 could flake on a slow machine: the robot thinks 3 s (was 1.5), and "Home before it answered" is timed from when it began to think (test 2), not from the click.
- **The UI tests left a profile folder in `%TEMP%`** for every robot game (306 of them here): `TestSetup` went before the app, so the autosave on the way out wrote into a folder already deleted. It is now made before the `QApplication`. (Only a test file stopped by the timeout still leaves one.)
- **The busy cursor** over the board while the robot thinks is back as `Qt.BusyCursor` on a `HoverHandler` (it takes no input: premoves still work; over a piece that can be premoved its open hand shows).
- **`Run-UiTests.ps1`**: a test file stopped by the timeout is ended with its whole tree (`taskkill /T`), so a FakeUci it started doesn't hold the results file.
- **FakeLichess**: abort ends the game (`aborted`), claim-victory too (`timeout`, the player wins).
- **FakeUci**: a scripted promotion without its piece ("a7a8") is a queen; a scripted move it can't play is said on stderr (new test `Script_PromotionWithoutItsPiece_AQueen`).
- `tst_lichess.qml` listed in `ChessGuiTests.vcxproj`.

Checked: ChessTests 141/141; UI tests all 8 files in Release (58 s), `tst_robot` and `tst_profiles` in Debug; no new `%TEMP%` folder after a run; a screenshot run keeps the shell's `CHESS_ENGINE_PATH`; FakeUci renamed: `robot` skipped, `home` taken; `-Timeout 5`: no FakeUci left.

### 2026-11-12 — BL-05 step 6: the installer's smoke test (branch: features/installerSmokeTest_121126)

#### Done
- **`installer\Test-Installer.ps1`** (owner's choice over pywinauto, every PR): the installer silently into a temp folder (no desktop icon, no Stockfish); the installed `ChessGui.exe` on `home`, `game`, `settings`, `online`, `robot` (vs FakeUci to mate) in screenshot mode with Qt off `PATH` and Qt's plugin/QML paths unset; each scene: exit 0, a picture that isn't blank, no warning in its log; the TLS backend present; the uninstaller: folder and Add/Remove Programs entry gone. "DLL not found" dialogs off (`SetErrorMode`); a scene that didn't run skips the rest. Refuses to install where Chess++ is installed; `-AppDir` tests a folder.
- **`release.yml`**: "Smoke test the installer" after the installer is built, before its upload and the release; "Upload installer test results" on a failure.
- Checked locally on a `windeployqt` folder: healthy passes; without `multimedia\` the log's warning fails it; without the Fluent style module exit 3; without `Qt6Network.dll` `0xC0000135` at once, no dialog.

#### Learned
- On GitHub the installed app's real window works (no offscreen plugin is installed). The runner has no sound device ("playback of this format is not supported on the selected audio device": ignored) and no engine (every scene gets FakeUci, as if Stockfish were installed).

#### Next
- BL-05 is done. Open from before: the LI-04 / LI-02 / LI-05 hand tests on real Lichess (`TASKS.md`).

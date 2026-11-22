# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Qt-chess is a chess application with a Qt QML frontend and C++ backend, supporting local two-player, UCI engine (e.g., Stockfish), and network multiplayer modes. Games are standard chess or Chess960 (LI-01A): local ones, and on Lichess (stage 2: a Lichess Chess960 game starts from its `initialFen`, other variants are refused; the Online page's challenge offers it).

## Where to read more

Each project folder has its own `CLAUDE.md`, loaded when a file in that folder is read. **Before changing code in
a folder, Read its `CLAUDE.md`** (Grep and Glob don't load it).

| File | What it covers |
|---|---|
| `ChessCore/CLAUDE.md` | the chess rules: key types, castling and FEN for any set-up, invariants |
| `ChessTests/CLAUDE.md` | the rules' tests, the Chess960 reference games |
| `UciConnector/CLAUDE.md` | the UCI engine: API, Chess960 requests, which engine runs |
| `FakeUci/CLAUDE.md` | the tests' stand-in engine |
| `LiChessConnector/CLAUDE.md` | Lichess: `LichessClient` by area (variants, players, streams, challenges, seeks), `LichessAuth`, invariants |
| `ChessQtTests/CLAUDE.md` | the Qt parts' tests: what they need, `FakeLichess` |
| `ChessGui/ChessGui/CLAUDE.md` | the app's folder: its C++ by area, assets, which lists a new file goes into |
| `.claude/rules/chessgui-cpp.md` | ChessGui's C++ class by class, invariants (loads with a `.h` / `.cpp` there) |
| `ChessGui/ChessGui/qml/CLAUDE.md` | the QML: rules, the window and pages, the board's layers, shared components, dialogs |
| `installer/CLAUDE.md` | the installer, Stockfish's download, the smoke test, the icons |
| `.claude/rules/cpp-style.md` | C++ style (loads with any `.h` / `.cpp`) |
| `.claude/rules/tasks.md` | the ticket format and working a ticket (loads with a ticket) |
| `.claude/skills/run-chess-app/` | running the app, screenshots and their scenes |

ChessGuiTests is still described under Running Tests (BL-32 moves it next).

## Build System

**Primary build**: Visual Studio 2022 solution — `ChessCore.sln`

**Qt GUI project** also has a `.pro` file for Qt Creator: `ChessGui/ChessGui/ChessGui.pro`

**Dependencies**: Qt 6.12+ only (with the Qt Multimedia and Qt Image Formats add-ons; Image Formats for the WebP of Lichess flairs, LI-11) — no third-party C++ libraries and no package manager (vcpkg was removed once Boost was replaced by Qt).

**Build configurations**: Debug|x64 and Release|x64 are the relevant targets.

## Running Tests

Tests use **Microsoft CppUnitTest Framework** (built into Visual Studio).

- Open `ChessCore.sln` in Visual Studio
- Build the `ChessTests` and `ChessQtTests` projects
- Run via **Test Explorer** (Test → Test Explorer) or:

```
vstest.console.exe x64\Debug\ChessTests.dll
vstest.console.exe x64\Debug\ChessQtTests.dll
```

- `ChessTests/` — chess logic, plain C++, no Qt (`ChessTests/CLAUDE.md`).
- `ChessQtTests/` — the Qt parts (UciConnector, Lichess, ChessGui's C++): Qt's `bin` on `PATH`, some tests need Stockfish; not run by GitHub (`ChessQtTests/CLAUDE.md`).
- `ChessGuiTests/` — the UI tests (BL-05, `docs/UiTestSuitePlan.md`): **Qt Quick Test**, a console exe, not in Test Explorer; GitHub runs them on every push (`tests.yml`, Debug; a failure uploads `test-results\ui` as `ui-test-results`). `qml/tst_*.qml` (first run, two players, the board, promotion, the robot, profiles, a restart, Lichess, the games library, the analysis, Chess960, Chess960 on Lichess, no engine) play the app's own window (`ChessGame.qml`) with real mouse events, offscreen (also with the screen locked); `qml/AppDriver.qml` has the helpers (`clickMove`, `dragPiece`, `pieceAt`, `waitPiece`, `waitBoardIdle`, `waitMessage`, `startTwoPlayerGame`, `leaveToHome`, `scrollIntoView`, …), items are found by `objectName`. One test file per process on a fresh profile (`TestSetup`, the app's `AppServices`; a file with `// ui-test-runs: 2` runs twice on one data folder, `TestRun.run`: a restart; `// ui-test-engine: none` runs it without an engine, `tst_noEngine`); the robot is FakeUci (`app.startComputerGame(line, thinkMs)`), no Stockfish needed; Lichess is `FakeLichess` (`fakelichessserver.h`, a local server the test plays the other side on; a challenge or ongoing game can be another variant, with a start FEN; `setPlayer` gives a player a title and rating in gameFull; `TestRun.linkLichess`), never lichess.org; `TestRun.addFinishedGames(n)` fills the games library without playing. It builds every ChessGui `.cpp` except `main.cpp`: **a new ChessGui source goes into `ChessGuiTests.vcxproj` too** (the link fails without it). Run:

```
scripts\Run-UiTests.ps1 -Build              # every test file (Release, the default: without -Build an old Release exe runs; -Configuration Debug for the Debug build); -Name tst_twoPlayers -Function test_2_newGameAndDrag for one
scripts\Run-UiTests.ps1 -Build -Screens     # the pre-PR check: also every screenshot, compared with the baselines
scripts\Run-UiTests.ps1 -Name tst_twoPlayers -Visible   # watch it in a real window, paced; a failure waits until you close it
```
A failed test function leaves a picture of the window: `test-results\ui\<TestCase>-<function>.png` (listed with the failure).

## Seeing the UI

`scripts\Take-Screenshot.ps1 -Scene all` saves screenshots of the app's views (invisible window) to
`test-results\screens\` on a throwaway profile. `scripts\Compare-Screenshots.ps1` diffs them against
local baselines (`-Update` accepts the current look). Details: the `run-chess-app` skill
(`.claude/skills/run-chess-app/SKILL.md`).

The installer (`release.yml`, Stockfish's download, the smoke test `Test-Installer.ps1`) and the app icon
(`scripts\make_icons.py`): `installer/CLAUDE.md`.

## Architecture

Eight projects in `ChessCore.sln`:

| Project | Kind | What |
|---|---|---|
| `ChessCore` | static lib, no Qt | the chess rules, `Chess::` |
| `UciConnector` | static lib, Qt Core | a UCI engine (Stockfish) through `QProcess` |
| `LiChessConnector` | DLL, Qt Core + Network | Lichess: `LichessClient`, `LichessStream`, `LichessAuth` |
| `ChessGui` (`ChessGui/ChessGui/`) | the app, Qt Quick | the QML UI and its C++: `ChessConnector`, models, profiles, the games library, the robot |
| `FakeUci` | console exe, no Qt | the tests' stand-in engine |
| `ChessTests` | test DLL, no Qt | ChessCore's tests |
| `ChessQtTests` | test DLL, Qt | the Qt parts: UciConnector, Lichess, and ChessGui's C++ (built from its sources) |
| `ChessGuiTests` | console exe, Qt Quick Test | the UI tests (every ChessGui `.cpp` but `main.cpp`) |

```
ChessGui: QML pages → ChessConnector (the game), UserManager, GameLibrary, LichessClient, …
  ├─ ChessCore: Game → Board, MoveGeneration, GameChecks, StartPosition
  ├─ UciConnector ← ChessEnginePlayer / EngineWorker (the robot), GameAnalyzer (the analysis)
  └─ LiChessConnector: LichessClient → LichessStream, LichessAuth
FakeUci, ChessTests → ChessCore;   ChessQtTests, ChessGuiTests → all four above (+ FakeUci)
```

## Key Conventions

- C++ style (OO, `std::shared_ptr`, `Chess::` without Qt, `PieceColors::Light`, no formatters): `.claude/rules/cpp-style.md`.
- QML ↔ C++ boundary:
  1. Anything that reads or changes the game — including flows that combine the game with users or Lichess (start a local or online game, game end, rematch, switch user during a game) — goes through `ChessConnector`, as **one call per user action**. QML never strings game steps together itself, and never sees `Chess::` types (`Game`, `Board`, engine).
  2. `UserManager` and `LichessClient` stay separate services. QML may call them directly for non-game work: profiles, login, Lichess account and challenge setup, ratings. Don't forward their API through `ChessConnector`.
  3. Expose QML-visible services as typed singletons (`qmlRegisterSingletonInstance`), not context properties: `ChessConnector`, `UserManager`, `LichessClient`, `AvatarProvider`, `AppInfo` and `GameLibrary` in module `ChessPlusPlus` (`AppServices`: made, wired and registered there, for `main.cpp` and the UI tests alike). A QML file that uses them has `import ChessPlusPlus`.
- The Lichess token never reaches QML and is never exported (`UserManager::lichessToken()` is C++ only).

## Tasks and Bugs (`.tasks/`)

Every task and bug is one Markdown file. Its format (file name, frontmatter, the next free number, plan and
hand tests in `docs/`) and working a ticket (start, step entries, closing) are in `.claude/rules/tasks.md`: it
loads when a ticket is read or edited; Read it before writing a new ticket.

- `.tasks/open/` — the queue: open and in-progress tickets.
- `.tasks/archive/` — done tickets, each with its history under `## Resolution`. `_unmatched-progress.md` holds old log entries that match no ticket.
- IDs: `BL-` the app (bugs included: BL-23 and BL-24 were bugs), `LI-` playing on Lichess, `CE-` the chess rules in `ChessCore`; a variant of a ticket gets a letter (`LI-01A`, `LI-01B`). `TSK-` / `BUG-` are only the old items that had no ID; don't make new ones.

Saving tokens:

- **Never** read all of `.tasks/open/`, and **never** read `.tasks/archive/` unless asked to look into a past ticket.
- To find work: `ls .tasks/open`, or Grep `.tasks/open` for `status: in_progress` / `priority: high` (file names only), then read **only** the ticket being worked on.
- A bug or follow-up found on the way: create its file in `.tasks/open/` at once (with what was found: cause if known, where, how to see it) instead of writing it into `PROGRESS.md` or another ticket.

## Session Handover (`PROGRESS.md`)

At the start of a session read `PROGRESS.md`, then confirm the branch with `git branch` and `git log --oneline -5` before changing anything.

`PROGRESS.md` is a handover note, **under 15 lines**, not a diary: update it in every PR by overwriting it (the history is in the tickets' `## Resolution`; on a merge conflict keep the newer note). Only:

- **Last completed:** ticket ID, branch, one line on what was done.
- **Current focus:** the active ticket ID and where it stands, or `None`.
- **Next up:** the ticket from `.tasks/open/` to take next.
- **Active gotchas:** blockers and passing environment notes only. Lasting rules go into this file (or the `run-chess-app` skill for screenshots), not there.

## Branching Workflow

- Start every task on a **new branch from the latest `origin/develop`** (`git fetch` first). Name it `features/<camelCaseTask>_<DDMMYY>` for a feature or task, `bugs/<camelCaseTask>_<DDMMYY>` for a bug fix, `docs/<camelCaseTask>_<DDMMYY>` for a change to docs only (a plan, a ticket in `.tasks/`, the README), e.g. `features/removeVcpkg_281026`, `bugs/infoBarsCover_101126`, `docs/bl13Plan_091126`.
  - `DDMMYY` is the day the branch is created: two digits each for day, month and year (10 Nov 2026 → `101126`). Take it from the local clock (`Get-Date -Format ddMMyy`), not from memory: a session can run past midnight.
  - Branches before 2026-11-10 have no `features/` / `bugs/` prefix (e.g. `infoBarsColumn_091126`, PR #137); don't rename them.
- Never continue a new task on the branch of a previous task.
- When a feature/task is done (built, tested, committed), push the branch and open a GitHub PR into `develop` with the `gh` CLI (`gh pr create --base develop`) — no need to ask first. Report the PR link.
- Exception: if the task depends on changes that are not merged into `develop` yet, stop and ask the user to either merge them first (then wait and branch from the updated `develop`) or explicitly approve branching from the unmerged branch.

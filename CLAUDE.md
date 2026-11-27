# CLAUDE.md

## Project Overview

Chess++ (qt-chess): a chess app, a Qt QML frontend on a C++ backend. Two players on one board, vs a UCI engine
(Stockfish), and on Lichess; standard chess or Chess960 (LI-01A; a Lichess Chess960 game starts from its
`initialFen`, other variants are refused).

## Where to read more

Each project folder has its own `CLAUDE.md`, loaded when a file in that folder is read. **Before changing code in
a folder, Read its `CLAUDE.md`** (Grep and Glob don't load it).

| File | What it covers |
|---|---|
| `ChessCore/CLAUDE.md` · `ChessTests/CLAUDE.md` | the chess rules (castling and FEN for any set-up, invariants); their tests |
| `UciConnector/CLAUDE.md` · `FakeUci/CLAUDE.md` | the UCI engine (API, Chess960 requests, which engine runs); the tests' stand-in |
| `OnlineCore/CLAUDE.md` · `LiChessConnector/CLAUDE.md` | online play: `OnlineClient` by area, `OnlineServer`; Lichess's own (`LichessServer`, `LichessAuth`) |
| `ChessQtTests/CLAUDE.md` | the Qt parts' tests: what they need, `FakeLichess` |
| `ChessGui/ChessGui/CLAUDE.md` | the app's folder: its C++ by area, assets, which lists a new file goes into |
| `.claude/rules/chessgui-cpp.md` | ChessGui's C++ class by class, invariants (loads with a `.h` / `.cpp` there) |
| `ChessGui/ChessGui/qml/CLAUDE.md` | the QML: rules, the window and pages, the board's layers, components, dialogs |
| `ChessGuiTests/CLAUDE.md` | the UI tests: running, set-up, FakeUci and `FakeLichess`, writing a test |
| `installer/CLAUDE.md` | the installer (`release.yml`), Stockfish's download, `Test-Installer.ps1`, `scripts\make_icons.py` |
| `.claude/rules/cpp-style.md` · `.claude/rules/tasks.md` | C++ style (any `.h` / `.cpp`); the ticket format (a ticket) |
| skills `ticket` · `run-chess-app` | a ticket's steps; running the app, screenshots (`scenes.md`: every scene), baselines |

## Build and tests

- Visual Studio 2022: `ChessCore.sln`, Debug|x64 and Release|x64; Qt Creator: `ChessGui/ChessGui/ChessGui.pro`.
- Qt 6.12+ only (with the Qt Multimedia and Qt Image Formats add-ons; Image Formats for the WebP of Lichess
  flairs), no third-party C++ libraries, no package manager.
- Unit tests: Microsoft CppUnitTest (Test Explorer, or `vstest.console.exe`). `ChessTests/`: chess logic, no Qt.
  `ChessQtTests/`: the Qt parts; Qt's `bin` on `PATH`, some tests need Stockfish; not run by GitHub.
- UI tests (`ChessGuiTests/`): Qt Quick Test, the app's window played with the mouse, offscreen, FakeUci and
  `FakeLichess` (never lichess.org); GitHub runs them on every push.

```
vstest.console.exe x64\Debug\ChessTests.dll
vstest.console.exe x64\Debug\ChessQtTests.dll
scripts\Run-UiTests.ps1 -Build              # every UI test file (Release; -Configuration Debug for the Debug build)
scripts\Run-UiTests.ps1 -Build -Screens     # the pre-PR check: also every screenshot, compared with the baselines
scripts\Take-Screenshot.ps1 -Scene all      # screenshots of the app's views (invisible window), throwaway profile
```

## Architecture

| Project | Kind | What |
|---|---|---|
| `ChessCore` | static lib, no Qt | the chess rules, `Chess::` |
| `UciConnector` | static lib, Qt Core | a UCI engine (Stockfish) through `QProcess` |
| `OnlineCore` | DLL, Qt Core + Network | online play, the Board API: `OnlineClient`, `OnlineStream`, `OnlineServer`, `OnlineAuth` |
| `LiChessConnector` | DLL, on `OnlineCore` | Lichess's own: `LichessServer`, `LichessAuth` (LI-18) |
| `ChessGui` (`ChessGui/ChessGui/`) | the app, Qt Quick | the QML UI and its C++: `ChessConnector`, models, profiles, the games library, the robot |
| `FakeUci` | console exe, no Qt | the tests' stand-in engine |
| `ChessTests` · `ChessQtTests` | test DLLs | ChessCore's tests; the Qt parts' (ChessGui's C++ built from its sources) |
| `ChessGuiTests` | console exe, Qt Quick Test | the UI tests (every ChessGui `.cpp` but `main.cpp`) |

```
ChessGui: QML pages → ChessConnector (the game), UserManager, GameLibrary, LichessClient, …
  ├─ ChessCore: Game → Board, MoveGeneration, GameChecks, StartPosition
  ├─ UciConnector ← ChessEnginePlayer / EngineWorker (the robot), GameAnalyzer (the analysis)
  └─ OnlineCore: OnlineClient → OnlineStream, OnlineServer ← LiChessConnector: LichessServer, LichessAuth
FakeUci, ChessTests → ChessCore;   ChessQtTests, ChessGuiTests → all five above (+ FakeUci)
```


## Key Conventions

- C++ style (OO, `std::shared_ptr`, `Chess::` without Qt, `PieceColors::Light`, no formatters): `.claude/rules/cpp-style.md`.
- QML ↔ C++ boundary:
  1. Anything that reads or changes the game — including flows that combine the game with users or Lichess (start a local or online game, game end, rematch, switch user during a game) — goes through `ChessConnector`, as **one call per user action**. QML never strings game steps together itself, and never sees `Chess::` types (`Game`, `Board`, engine).
  2. `UserManager` and `LichessClient` stay separate services. QML may call them directly for non-game work: profiles, login, Lichess account and challenge setup, ratings. Don't forward their API through `ChessConnector`.
  3. Expose QML-visible services as typed singletons (`qmlRegisterSingletonInstance`), not context properties: `ChessConnector`, `UserManager`, `LichessClient`, `AvatarProvider`, `AppInfo` and `GameLibrary` in module `ChessPlusPlus` (`AppServices`: made, wired and registered there, for `main.cpp` and the UI tests alike). A QML file that uses them has `import ChessPlusPlus`.
- The Lichess token never reaches QML and is never exported (`UserManager::lichessToken()` is C++ only).
- **A new ChessGui source goes into `ChessGuiTests.vcxproj` too** (the link fails without it); the other lists it goes into: `ChessGui/ChessGui/CLAUDE.md`.

## Tasks and Bugs (`.tasks/`)

Every task and bug is one Markdown file: the format in `.claude/rules/tasks.md` (it loads with a ticket; Read it
before writing a new one), the steps (a new ticket, starting, step entries, closing) in the `ticket` skill.

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
- **Active gotchas:** blockers and passing environment notes only. Lasting rules go where they apply, not there: the `CLAUDE.md` of the folder they are about, a `.claude/rules/` file for a kind of file, a skill for a procedure; this file only for what every task needs.

## Branching Workflow

- Start every task on a **new branch from the latest `origin/develop`** (`git fetch` first). Name it `features/<camelCaseTask>_<DDMMYY>` for a feature or task, `bugs/<camelCaseTask>_<DDMMYY>` for a bug fix, `docs/<camelCaseTask>_<DDMMYY>` for a change to docs only (a plan, a ticket in `.tasks/`, the README), e.g. `features/removeVcpkg_281026`, `bugs/infoBarsCover_101126`, `docs/bl13Plan_091126`.
  - `DDMMYY` is the day the branch is created: two digits each for day, month and year (10 Nov 2026 → `101126`). Take it from the local clock (`Get-Date -Format ddMMyy`), not from memory: a session can run past midnight.
  - Branches before 2026-11-10 have no `features/` / `bugs/` prefix (e.g. `infoBarsColumn_091126`, PR #137); don't rename them.
- Never continue a new task on the branch of a previous task.
- When a feature/task is done (built, tested, committed), push the branch and open a GitHub PR into `develop` with the `gh` CLI (`gh pr create --base develop`) — no need to ask first. Report the PR link.
- Exception: if the task depends on changes that are not merged into `develop` yet, stop and ask the user to either merge them first (then wait and branch from the updated `develop`) or explicitly approve branching from the unmerged branch.

## Writing these docs

- A class is described in its folder's `CLAUDE.md` (ChessGui's C++: `.claude/rules/chessgui-cpp.md`) by what it
  is and its invariants, never its ticket history: that is in the tickets' `## Resolution` and `docs/`.
- A rule that must always hold goes into that file's `### Invariants` / `## Invariants` list.
- This file stays under 120 lines, with only what every task needs; no `@` imports in it (they load eagerly).
- A new project folder gets its `CLAUDE.md` and a row in "Where to read more".

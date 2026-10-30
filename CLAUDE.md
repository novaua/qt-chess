# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Qt-chess is a chess application with a Qt QML frontend and C++ backend, supporting local two-player, UCI engine (e.g., Stockfish), and network multiplayer modes.

## Build System

**Primary build**: Visual Studio 2022 solution — `ChessCore.sln`

**Qt GUI project** also has a `.pro` file for Qt Creator: `ChessGui/ChessGui/ChessGui.pro`

**Dependencies**: Qt 6.12+ only (with the Qt Multimedia add-on) — no third-party C++ libraries and no package manager (vcpkg was removed once Boost was replaced by Qt).

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

- `ChessTests/` — chess logic (`BasicTest.cpp`, `CheckTests.cpp`, `FenGenTests.cpp`, `SanTest.cpp`). Plain C++, no Qt.
- `ChessQtTests/` — the Qt parts: `UciConnectorTest.cpp` (UCI protocol), `ChessEnginePlayerTest.cpp`, `LichessClientTest.cpp`,
  `UserManagerTest.cpp`. Needs Qt's `bin` on `PATH`; the engine tests need Stockfish.

## Seeing the UI

`scripts\Take-Screenshot.ps1 -Scene all` saves screenshots of the app's views (invisible window) to
`test-results\screens\` on a throwaway profile. `scripts\Compare-Screenshots.ps1` diffs them against
local baselines (`-Update` accepts the current look). Details: the `run-chess-app` skill
(`.claude/skills/run-chess-app/SKILL.md`).

App icon and installer images: `python scripts\make_icons.py` (Pillow) redraws `chess.ico`,
`pics/app-icon.png` and `installer/art/*.bmp`.

## Architecture

Four projects in the solution:

```
ChessGui (QML app)
  └─ ChessConnector (C++ ↔ QML bridge)
       └─ Game (game state, event dispatch)
            ├─ Board (8×8 state, piece tracking)
            ├─ MoveGeneration (legal move calculation)
            ├─ Check (check/checkmate detection)
            └─ ChessEnginePlayer
                 └─ UciConnector (QProcess → external engine)
```

### ChessCore (static library)

All chess logic, isolated in the `Chess::` namespace. Key types:

- `Board` — 64-square state; owns piece positions
- `Game` — orchestrates turns, validates moves, fires events to listeners (observer pattern via `EventBase`/`PawnPromotionEvent`)
- `Move` / `HistoryMove` — move representation (from/to squares, capture flag, promotion)
- `MoveGeneration` — generates pseudo-legal and legal moves including castling and en passant
- `ChessEnginePlayer` — wraps `UciConnector` to produce computer moves
- `Serializer` — binary save/load of game history

### UciConnector (static library)

Spawns and communicates with external UCI engines via `QProcess`. Main API: `Init()`, `NewGame()`, `GetEngineMove()`.

### ChessGui (Qt QML application)

- **`chessconnector.h/cpp`** — the C++/QML bridge for the chess game; exposes `Q_PROPERTY` bindings and slots that QML calls
- **`UserManager`**, **`LichessClient`** (LiChessConnector project), **`AvatarProvider`**, **`AppInfo`** (version, data/log folders, the engine's name) — app services without chess logic, also visible to QML (see Key Conventions)
- QML files in `ChessGui/ChessGui/qml/`:
  - `ChessGame.qml` — the window: a `StackView` with one page (`WelcomePage` on first run,
    `ProfilePickerPage`, `HomePage`, `GamePage`; `route()` picks it from `UserManager` and
    `ChessConnector.InGame`), with `ProfilePage` or `SettingsPage` pushed over Home or the game; the
    InfoBars and every dialog. Pages ask for dialogs through signals.
  - `HomePage.qml` (account menu, Continue, action cards), `GamePage.qml` (command bar, board between
    two `PlayerCard`s with a `CapturedStrip` and "+N", the moves (`MovesPanel` + `MoveList`) beside
    the board or in a bottom sheet, `ResultCard` over the board)
  - `ChessBoard.qml` — board rendering
  - `ChessFigures.qml` — piece rendering and drag/drop animation
  - `ChessBoxes.qml` — square highlights (legal moves, last move)
  - Shared: `Theme.qml` and `AppSettings.qml` (singletons; `AppSettings` is the QtCore `Settings` with the
    app's own options), `SettingRow`, `AppDialog`, `ConfirmDialog`, `CommandBar`, `InfoBar`, `IconButton`, `HomeCard`, `ChoiceRow`, `SectionCard`, `AvatarPicker`, `FocusRing` (keyboard focus outline for custom-drawn items)
  - Tooling: `ScreenshotScene` (the `--screenshot` scenes; loaded only in that mode, keep debug/test helpers there, not in the pages)
  - Setup dialogs: `ComputerGameDialog`, `TwoPlayerDialog`, `LichessGameDialog`; profiles: `CreateUserDialog`, `ImportConflictDialog`, `RestoreBackupDialog`
- `UserManager` keeps `users.json` (atomic `QSaveFile` writes, `backups/users-1..3.json`), statistics
  per opponent, profile export/import (`.chessprofile`, never the Lichess token) and the Lichess
  sign-in (`LichessAuth`), so the token never reaches QML. Its tests are in `ChessQtTests/UserManagerTest.cpp`.
- Image assets live in `ChessGui/ChessGui/pics/`
- Resources bundled via `qml.qrc`

### ChessTests (test DLL)

Builds as a DLL consumed by the VS test runner. Tests exercise `Game`, `Board`, `MoveGeneration` and SAN/FEN directly.
It links only `ChessCore` and does **not** use Qt; tests that need Qt (UCI engine, Lichess, users) go in `ChessQtTests`.

## Key Conventions

- Smart pointers (`std::shared_ptr`) throughout; avoid raw owning pointers
- Chess logic stays in the `Chess::` namespace; GUI code stays in ChessGui
- QML ↔ C++ boundary:
  1. Anything that reads or changes the game — including flows that combine the game with users or Lichess (start a local or online game, game end, rematch, switch user during a game) — goes through `ChessConnector`, as **one call per user action**. QML never strings game steps together itself, and never sees `Chess::` types (`Game`, `Board`, engine).
  2. `UserManager` and `LichessClient` stay separate services. QML may call them directly for non-game work: profiles, login, Lichess account and challenge setup, ratings. Don't forward their API through `ChessConnector`.
  3. Expose QML-visible services as typed singletons (`qmlRegisterSingletonInstance`), not context properties: `ChessConnector`, `UserManager`, `LichessClient`, `AvatarProvider` and `AppInfo` in module `ChessPlusPlus` (`main.cpp`). A QML file that uses them has `import ChessPlusPlus`.
- `PieceColor` is an `enum class` (Light/Dark); use scoped references (`PieceColor::Light`)

## Resuming Work After a Session Break

At the start of a new session, read these two files to get full context:

1. `PROGRESS.md` — latest status, what's broken, and numbered next steps
2. `TASKS.md` — full task checklist; find first unchecked item under "In Progress"

Then confirm the current branch with `git branch` and `git log --oneline -5` before making any changes.

## Branching Workflow

- Start every task on a **new branch from the latest `origin/develop`** (`git fetch` first). Name it `<camelCaseTask>_<ddmmyy>`, e.g. `removeVcpkg_281026`.
- Never continue a new task on the branch of a previous task.
- When a feature/task is done (built, tested, committed), push the branch and open a GitHub PR into `develop` with the `gh` CLI (`gh pr create --base develop`) — no need to ask first. Report the PR link.
- Exception: if the task depends on changes that are not merged into `develop` yet, stop and ask the user to either merge them first (then wait and branch from the updated `develop`) or explicitly approve branching from the unmerged branch.

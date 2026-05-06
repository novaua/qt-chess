# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Qt-chess is a chess application with a Qt QML frontend and C++ backend, supporting local two-player, UCI engine (e.g., Stockfish), and network multiplayer modes.

## Build System

**Primary build**: Visual Studio 2022 solution — `ChessCore.sln`

**Qt GUI project** also has a `.pro` file for Qt Creator: `ChessGui/ChessGui/ChessGui.pro`

**Dependencies** are managed via vcpkg (manifest mode):
- `vcpkg.json` lists: boost-algorithm, boost-format, boost-regex, boost-process, boost-asio
- Run `vcpkg install` in the repo root to restore dependencies

**Build configurations**: Debug|x64 and Release|x64 are the relevant targets.

## Running Tests

Tests use **Microsoft CppUnitTest Framework** (built into Visual Studio).

- Open `ChessCore.sln` in Visual Studio
- Build the `ChessTests` project
- Run via **Test Explorer** (Test → Test Explorer) or:

```
vstest.console.exe x64\Debug\ChessTests.dll
```

Test files are in `ChessTests/` — `BasicTest.cpp` (board logic) and `UciConnectorTest.cpp` (UCI protocol).

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
                 └─ UciConnector (Boost.Process/ASIO → external engine)
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

Spawns and communicates with external UCI engines via `boost::process` and `boost::asio`. Main API: `Init()`, `NewGame()`, `GetEngineMove()`.

### ChessGui (Qt QML application)

- **`chessconnector.h/cpp`** — the sole C++/QML bridge; exposes `Q_PROPERTY` bindings and slots that QML calls
- QML files in `ChessGui/ChessGui/qml/`:
  - `ChessGame.qml` — top-level window, game state machine
  - `ChessBoard.qml` — board rendering
  - `ChessFigures.qml` — piece rendering and drag/drop animation
  - `ChessBoxes.qml` — square highlights (legal moves, last move)
  - `PawnPromotionOptions.qml`, `SelectPlayerDialog.qml`, `NavigationLayer.qml`
- Image assets live in `ChessGui/ChessGui/pics/`
- Resources bundled via `qml.qrc`

### ChessTests (test DLL)

Builds as a DLL consumed by the VS test runner. Tests exercise `Game`, `Board`, `MoveGeneration`, and `UciConnector` directly — no QML involved.

## Key Conventions

- Smart pointers (`std::shared_ptr`) throughout; avoid raw owning pointers
- Chess logic stays in the `Chess::` namespace; GUI code stays in ChessGui
- QML ↔ C++ communication goes exclusively through `ChessConnector` — don't add direct `Q_OBJECT` wrappers elsewhere
- `PieceColor` is an `enum class` (Light/Dark); use scoped references (`PieceColor::Light`)

## Resuming Work After a Session Break

At the start of a new session, read these two files to get full context:

1. `PROGRESS.md` — latest status, what's broken, and numbered next steps
2. `TASKS.md` — full task checklist; find first unchecked item under "In Progress"

Then confirm the current branch with `git branch` and `git log --oneline -5` before making any changes.

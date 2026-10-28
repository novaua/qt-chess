# UI Test Suite — Plan

| | |
| :---- | :---- |
| **Date** | 2026-10-28 |
| **Status** | Step 1 partly done (screenshot mode, `--data-dir`); rest is backlog BL-05 in `TASKS.md` |
| **Branch** | new branch from `develop`, e.g. `uiTestFoundation_<ddmmyy>` |

## 1. Problem

Only the chess logic and the UCI connector have tests (`ChessTests`, CppUnitTest). Nothing checks
the QML screens, so UI regressions are found by hand. We want:

- a **local sanity suite** to run before opening a PR (the main goal; it may be heavy),
- optionally a **GitHub job** for the lighter part,
- a reliable way for Claude Code to **see the app's views** (screenshots it can read) while
  working on UI changes.

Claude Code can already build the app, launch it and screenshot its window from PowerShell, but it
cannot reliably click or drag inside it; coordinate-based input is fragile.

## 2. Approach: three layers

| Layer | Tool | Runs | Purpose |
|---|---|---|---|
| 1. Behaviour | **Qt Quick Test** (part of Qt) | in-process, `QT_QPA_PLATFORM=offscreen` | Main suite: click, drag pieces, check state. Local + CI |
| 2. Visual | Screenshot hook + golden-image diff | in-process, offscreen | Catch layout/look regressions. **Local only** |
| 3. Smoke | **pywinauto** (Windows UI Automation) | real exe on the desktop | Catch packaging/deploy breakage. Before releases only |

### Why not UI Automation (pywinauto/FlaUI) as the main suite

It drives the real mouse and keyboard: the PC is unusable while it runs, it needs an unlocked
visible desktop, it cannot run headless, and chess-board drags still need computed coordinates.
It is kept for a handful of black-box smoke tests that in-process tests cannot cover (missing Qt
DLL, broken qrc, missing VC++ runtime).

## 3. Layer 1 — Qt Quick Test project

- New project `ChessGuiTests` in `ChessCore.sln` (Qt-only, no third-party libraries).
- Uses `QUICK_TEST_MAIN_WITH_SETUP`; the setup object's `qmlEngineAvailable()` registers the real
  `chessConnector`, `userManager`, `avatarProvider`, `lichessClient` context properties the same
  way `main.cpp` does, then tests load `ChessGame.qml`.
- Tests are QML `TestCase`s: find items by `objectName`, `mouseClick` / `mouseDrag` (piece moves),
  `tryCompare` (wait for animations), `grabImage` (screenshots).
- Prerequisite: add `objectName`s to buttons, dialogs and board squares — today the QML tree has
  only 4 `objectName`/`Accessible` uses.

## 4. Layer 2 — Screenshot hook and visual diff

- Debug/test switch in the app, e.g. `--screenshot out.png --scene board [--position <fen|save>]`:
  open a view, take `QQuickWindow::grabWindow()`, save, exit. Also usable by Claude Code to look at
  any view on demand.
- Quick tests save named states (menu, board after 1.e4, promotion dialog, mate result card) to
  `test-results/screens/`.
- Compare against local baselines with a small tolerance; on failure write a diff image.
- Local only: font rendering and DPI differ between machines, so baselines would be flaky in CI.

## 5. Layer 3 — pywinauto smoke tests (optional)

- 3–5 tests against the built Release exe or the installed app: app starts, main-menu buttons
  exist, start a two-player game, make one move, exit cleanly.
- Requires `Accessible.name` on controls (also improves screen-reader support).
- Python is test tooling only; no C++ dependency is added.

## 6. Determinism (required for all layers)

| Source of flakiness | Fix |
|---|---|
| Tests touching the real profile (`users.json`, `config.json`, saves under `AppLocalDataLocation`, see `AppConfig.cpp`) | `QStandardPaths::setTestModeEnabled(true)` or a `--data-dir` switch |
| Stockfish timing and move choice | Stub UCI engine with scripted replies (path configurable in test mode) |
| Random avatars (`AvatarProvider::randomize` on every new game) | Seed / test flag |
| Lichess / network | Fake client or skip |
| Animations | `tryCompare` waits, or shorter durations in test mode |
| DPI / window size (screenshots) | `QT_SCALE_FACTOR=1`, fixed window size |

Test-only switches live in `ChessConnector` or `main.cpp`, keeping the rule that QML ↔ C++ goes
through `ChessConnector`. The test project's own setup `QObject` sits outside ChessGui — confirm
this is acceptable under that rule before starting.

## 7. First scenarios

1. Main menu renders; **Load** disabled when there are no saves.
2. Two players: drag e2→e4 — piece moves, last-move highlight, move appears in the history panel.
3. Illegal drag snaps back.
4. Promotion dialog appears; choosing a knight gives a knight (both colours).
5. Fool's mate → result dialog / card and red flag on the losing king.
6. Save → Continue / Load restores the position.
7. Move-history navigation back and forward.
8. Single player vs the stub engine: engine replies.
9. A Settings toggle survives a restart.
10. Create user / log in.

## 8. Running

- **Local:** `scripts\ui-tests.ps1` — build Release, run CppUnit tests + Quick tests + visual diff
  (+ smoke with a switch); write `test-results\` with screenshots and diffs. Optional, opt-in git
  pre-push hook, since the run is heavy.
- **GitHub:** Layer 1 only, in `tests.yml`, on `workflow_dispatch` or a PR label rather than every
  push; screenshots uploaded as artifacts, never diffed.

## 9. Steps

1. Test foundation in the app: isolated data dir, avatar seed, stub-engine path, screenshot switch.
   - Done (`screenshotHook_281026`): `--data-dir`, `--screenshot` with scenes `create-user`, `login`,
     `menu`, `board` (+ `--moves`, `--black`, `--dark`, `--size`), `settings`, `levels`, `profile`;
     `scripts\Take-Screenshot.ps1`; `run-chess-app` skill. The board scene fixes the opponent avatar
     instead of seeding the random one.
   - Open: stub UCI engine (for a single-player scene), Lichess scenes.
   - Offscreen rendering leaves Qt5Compat shader effects (`ColorOverlay`, e.g. the ☰ icon) blank —
     visual baselines must be taken windowed, or those effects replaced (see BL-01).
2. `objectName`s + `ChessGuiTests` project with scenarios 1, 2 and 5.
3. `scripts\ui-tests.ps1` local runner.
4. Golden-image baselines and diff.
5. Remaining scenarios.
6. Optional: pywinauto smoke tests; manual CI job.

---
id: BL-32
title: "Split CLAUDE.md per project"
type: task
status: done
priority: medium
---

# BL-32 Split CLAUDE.md per project

The owner (2026-11-22): CLAUDE.md "got too fat and not every task requires all of the info". For example, the ChessGui section lists every file with its methods. Wanted: a short root CLAUDE.md, per-project descriptions it points to, and Claude Code's newer features used so agents load only what a task needs.

Found: the root `CLAUDE.md` is 207 lines and 40.7k characters, about 10k tokens loaded into every session and every subagent. The ChessGui section is 59% of it, much of it ticket history, and some of it is out of date ("Four projects": there are 8).

**Plan:** [`docs/BL-32-ClaudeMdSplitPlan.md`](../../docs/BL-32-ClaudeMdSplitPlan.md), for the owner's approval. It sets out:
- a root under 120 lines;
- a `CLAUDE.md` per project folder, loaded only when that folder's files are read;
- path-scoped rules (`.claude/rules/` with `paths:`) for the ChessGui C++ catalogue, C++ style and the ticket format;
- skills for procedures (`ticket`; `run-chess-app` split);
- a mechanical check that nothing is lost.

It's done in 7 docs PRs, a pilot first. Saving: about 6–7k tokens for most tasks and about 7.7k per subagent.

Also found (§7 of the plan): `.claude/settings.local.json` is tracked in git, and `ChessCore/ReadMe.txt` is wizard boilerplate.

## Resolution

### 2026-11-22 — step 0, the pilot (branch: docs/bl32Pilot_221126)
- Done: `installer/CLAUDE.md` (CI, `Build-Installer.ps1`, Stockfish's download, the smoke test, `make_icons.py`) and `.claude/rules/tasks.md` (`paths: .tasks/**/*.md`: the ticket format and working a ticket); their lines out of the root, which points to both (root 207 → 185 lines). "Saving tokens", the folders and the IDs stay in the root word for word.
- Coverage: every `` `code` `` span of the old root is in the new files; two are reworded (`installer/art/*.bmp` → `art\wizard-*.bmp`, `installer\Test-Installer.ps1` → `Test-Installer.ps1` in its folder's file).
- Checked in fresh headless sessions (`claude -p`, asked to list the instruction files in context): at start only the root loads; reading an `installer/` file loads `installer/CLAUDE.md`; reading a ticket loads the rule; **writing** a new ticket loads it too, but only with the Write's result (after the file is written), so the root keeps "Read it before writing a new ticket"; reading a `qml/` file **does** load a `ChessGui/ChessGui/CLAUDE.md` (a temporary probe file). So, as §3 of the plan expects, the ChessGui C++ catalogue goes into the rule `chessgui-cpp.md`, and `ChessGui/ChessGui/CLAUDE.md` stays a short map.
- Left: steps 1–6.

### 2026-11-22 — step 1, Core (branch: docs/bl32Core_221126)
- Done: `ChessCore/`, `ChessTests/`, `UciConnector/` and `FakeUci/CLAUDE.md`; `.claude/rules/cpp-style.md` (`paths: **/*.{h,cpp,hpp}`: OO style from PR #92, `shared_ptr`, `Chess::` without Qt, `PieceColors`, `writeFileAtomically`, no formatters, the vcxproj rule); the root's "Where to read more" table; Architecture fixed: the 8 projects and a corrected dependency tree (`ChessEnginePlayer` / `EngineWorker` and `ENGINE_TRACE` moved to the ChessGui section, where they live). Root 185 → 179 lines.
- Fixed on the way: the root said `PieceColor` (Light/Dark); the type is `PieceColors` (`Empty`, `Light`, `Dark`). `GetCandidateMoves` serves the robot's levels too, not only the analysis.
- Coverage: every `` `code` `` span of the old root is in the new files but six reworded ones: `BoardDiff.h`, `StartPosition.h` (the classes, named as their files), `Game::Save`, `Game::LoadMoves` (under `Game`), `GetEngineMove()` (with its request type), `PieceColor::Light` (corrected).
- Checked in fresh `claude -p` sessions: reading a file in each of the four folders (`.h`, `.cpp`, `.hpp`) loads the root, that folder's `CLAUDE.md` and `cpp-style.md`.
- Left: steps 2–6.

### 2026-11-22 — step 2, Lichess (branch: docs/bl32Lichess_221126)
- Done: `LiChessConnector/CLAUDE.md` (the three files; `LichessClient` by area: variants, players and flairs, the game stream and reconnect, joining, ongoing games, the event stream, quick pairing, screenshot mode; an Invariants list) and `ChessQtTests/CLAUDE.md` (what the DLL links and compiles, what a run needs, the one `QCoreApplication`, `TestSupport.h`, one file per class, `FakeLichess`). Root: the ChessQtTests bullet and the LichessClient text become pointers; Key Conventions gets "The Lichess token never reaches QML and is never exported"; two rows in "Where to read more". Root 179 → 180 lines, 37.0k → 33.9k characters.
- Found on the way, now written down: GitHub doesn't run ChessQtTests (`tests.yml`: ChessTests and the UI tests only); `LichessNetworkTests` calls the real lichess.org; the root's list missed `AppLogTest.cpp`, `MoveGateTest.cpp` and `TestSupport.h`.
- Coverage: every `` `code` `` span of the old root is in the new files.
- Checked in fresh `claude -p` sessions: reading `LiChessConnector/LichessClient.h` or `ChessQtTests/FakeLichess.h` loads the root, that folder's `CLAUDE.md` and `cpp-style.md`.
- Left: steps 3–6.

### 2026-11-22 — step 3, ChessGui (branch: docs/bl32ChessGui_221126)
- Done: `ChessGui/ChessGui/CLAUDE.md` (a map: the C++ by area, assets, which lists a new file goes into), `.claude/rules/chessgui-cpp.md` (`paths: ChessGui/ChessGui/*.{h,cpp}`: the C++ class by class, by area, with an Invariants list), `ChessGui/ChessGui/qml/CLAUDE.md` (QML rules, the window and pages, the board, shared components, dialogs, tooling). The root's ChessGui section and its offscreen line are gone; three rows in "Where to read more". Root 180 → 142 lines, 33.9k → 11.6k characters. The rule is 15.1k characters (about 3.7k tokens, the plan said 2.6k): the identifiers are kept, the ticket history cut.
- Newly described: `EngineLevel.h`, `AppConfig`, `MarkerRow` / `MarkerColumn`, `PawnPromotionOptions`, `LevelsHelpDialog`, `ApplyMovesDialog`, `sounds/`, the four lists a new file goes into (`ChessGui.pro` is kept in sync). Found: `networkplayer.h/cpp` are in no project: BL-33.
- Coverage: every `` `code` `` span of the old root is in the new files, but 12 reworded: file names without `.h/cpp` (`boardpieces`, `movegate`, …), `ChessConnector.`/`::` names under the ChessConnector heading, `ChessGui/ChessGui/pics/` as `pics/` in that folder's file.
- Checked in fresh `claude -p` sessions: reading a `qml/` file loads the root, the folder map and `qml/CLAUDE.md` (not the C++ rule); reading `chessconnector.h` loads the root, the map, `chessgui-cpp.md` and `cpp-style.md`; reading `qml.qrc` loads the root and the map.
- Left: the owner's review of the Invariants lists (step 3's check); steps 4–6.

- 2026-11-22: the owner reviewed step 3's Invariants lists (#231): good as they are.

### 2026-11-22 — step 4, UI tests and the skill (branch: docs/bl32UiTests_221126)
- Done: `ChessGuiTests/CLAUDE.md` (running, how a run is set up: `TestSetup` / `TestRun`, `// ui-test-runs`, `// ui-test-engine`, FakeUci, `FakeLichess`; writing a test; waiting right; offscreen; Invariants), from the root's ChessGuiTests bullet and the skill's UI-test section. `run-chess-app` split: the scene catalogue in `scenes.md` beside `SKILL.md`, read only when a scene is chosen (`SKILL.md` 22.4k → 9.0k characters; `scenes.md` 11.0k). Root: the ChessGuiTests bullet a pointer (the `Run-UiTests.ps1` commands stay), the vcxproj rule in Key Conventions, one row more in "Where to read more". Root 142 lines, 11.6k → 10.5k characters. `PROGRESS.md`'s offscreen gotcha dropped (in `ChessGuiTests/CLAUDE.md` now).
- Coverage: every `` `code` `` span of the old root and the old skill is in the new files, but five reworded: `TestRun.addFinishedGames(n)`, `app.startComputerGame(line, thinkMs)` (with a real line), `ChessGuiTests.exe` and the `ChessGuiTests\qml\…` paths (as `qml/…` in the folder's file).
- Checked in a fresh `claude -p` session: reading `ChessGuiTests/qml/tst_board.qml` loads the root and `ChessGuiTests/CLAUDE.md`.
- Left: steps 5–6.

### 2026-11-22 — step 5, tickets (branch: docs/bl32Tickets_221126)
- Done: the `ticket` skill (`.claude/skills/ticket/SKILL.md`): a new ticket (the prefix, the next free number by a PowerShell one-liner, the file, one found on the way), starting one, each PR's step entry and `PROGRESS.md`, closing (hand tests: ask the owner, never assume). The format stays in one place, `.claude/rules/tasks.md`; the skill points to it. Pointers updated: `.tasks/README.md`, the root's Tasks section and "Where to read more", the rule; the root's Session Handover now says where lasting rules go (a folder's `CLAUDE.md`, a rule, a skill; the root only for what every task needs) instead of "this file or the run-chess-app skill".
- Checked: the number command gives BL-34 and LI-17 (BL-33, LI-16 are the last); a fresh `claude -p` session lists the `ticket` skill with its description.
- Left: step 6, the final trim.

### 2026-11-22 — step 6, the final trim; closed (branch: docs/bl32FinalTrim_221126)
- Done: the root rewritten to 118 lines, 9.5k characters (from 207 lines, 40.7k: about 10k → 2.4k tokens in every session and subagent): the overview in 3 lines, "Where to read more" in 11 rows, Build and tests merged with the commands only, Architecture's table shortened, the new "Writing these docs" (a class by what it is and its invariants, never its history; Invariants lists; the root under 120 lines, only what every task needs, no `@` imports; a new folder gets its `CLAUDE.md` and a row). Kept word for word: QML ↔ C++ rules 1–3, the token and vcxproj rules, the folders, IDs and "Saving tokens", Session Handover, Branching Workflow.
- The full check against the files before step 0 (`4b02473`): of the old root's 597 `` `code` `` spans and the old skill's 238, none dropped; 24 reworded (listed in each step's PR).
- Three fresh `claude -p` sessions (Sonnet), read-only: **a QML change** (a new component beside `BusyRing.qml`): it named `qml.qrc`, `ChessGui.vcxproj`'s `<None>` list and `ChessGui.pro`'s `DISTFILES`, and the QML rules (Theme, objectName, the `MultiEffect` guard, `ScreenshotScene`); **a Lichess change** (a new event-stream event): the test in ChessQtTests against `FakeLichess`, never lichess.org, run locally as GitHub doesn't; loaded the root, `LiChessConnector/`, `ChessQtTests/CLAUDE.md`, `cpp-style.md`; **a ticket** (a bug found): it used the `ticket` skill and gave BL-34, the right next number.

## Resolution summary
The root `CLAUDE.md` holds only what every task needs (118 lines); a `CLAUDE.md` in each project folder (ChessCore, ChessTests, UciConnector, FakeUci, LiChessConnector, ChessQtTests, ChessGui/ChessGui, its qml/, ChessGuiTests, installer) loads when a file there is read; path-scoped rules (`cpp-style.md`, `chessgui-cpp.md`, `tasks.md`) load with their files; the skills `ticket` and `run-chess-app` (its scenes in `scenes.md`) hold the procedures. PRs #228–#233 and this one. Found on the way: BL-33 (unbuilt `networkplayer`), `PieceColors` (the root said `PieceColor`), GitHub not running ChessQtTests (now written down).

### 2026-11-22 — follow-up: the new files checked against the code (branch: docs/claudeMdFactCheck_221126)
- Mechanical: every identifier in the new `CLAUDE.md` files, rules and skills grepped in the sources: one missing, `playJoinedMoves` (stale already in the old root): it is `ChessConnector::startOnlineGame(…, joinedMoves)`. The scene catalogue against `Take-Screenshot.ps1`: the six LI-09 scenes (`quick-pairing`, `-waiting`, `-indicator`, `seek-dialog`, `-blitz`, `-days`) were never listed; added.
- Claims corrected: `MoveGeneration` is implemented mostly in `Move.cpp`; `IsChess960()`, not the number, decides castling (#518 as Chess960); `canPlayHere` is false also for speeds the Board API can't play; another variant is listed unplayable and refused at start by `ChessConnector`; `LichessNetworkTests` is the one test that reaches lichess.org; `ChessConnector` owns the Game, `EngineWorker`, `MoveGate`, `GameClock`, `GameAnalyzer` and writes the record; the flag's win-or-draw is `ChessConnector` asking `Board::CanMate`; rated games are `GameRecord::rateable()`'s (a result, two plies, a valid level, no takeback or fork, never Chess960); "no analysis of the game going on" is `CanAnalyse`, not the analyzer; not every dialog and InfoBar is `ChessGame.qml`'s; PR installers are signed too, the release body falls back to GitHub's notes; `writeFileAtomically` is for the app's own files (not the log, not QtCore `Settings`); the UI tests' `CHESS_UI_TEST_DATA_DIR`.
- README: the UI tests' command added.

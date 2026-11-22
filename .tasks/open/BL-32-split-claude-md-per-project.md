---
id: BL-32
title: "Split CLAUDE.md per project"
type: task
status: in_progress
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

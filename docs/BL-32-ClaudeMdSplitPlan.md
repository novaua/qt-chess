# BL-32 Split CLAUDE.md per Project — Plan

| | |
| :---- | :---- |
| **Date** | 2026-11-22 |
| **Status** | Plan, for the owner's approval |
| **Branch** | `docs/claudeMdSplitPlan_221126` (plan); each step on its own `docs/…` branch from `develop` |
| **Task** | [`BL-32`](../.tasks/open/BL-32-split-claude-md-per-project.md) |

## 1. Problem

The root `CLAUDE.md` is loaded into **every** Claude Code session, and again into every subagent. It is
207 lines, 40.7k characters, about **10k tokens**, over Claude Code's recommended 200 lines.

Most tasks need only a slice of it:
- **The ChessGui section is 59%** (about 5.9k tokens): nearly every C++ and QML file, with methods,
  properties and ticket history.
- **Much of the text is history** that already lives in the tickets' `## Resolution` and in `docs/*Plan.md`:
  migrations, "step 4b", "BL-05 step 6", "(vcpkg was removed…)".
- **Some of it is out of date:**
  - "Four projects in the solution": there are 8.
  - `ChessEnginePlayer` is listed under ChessCore but lives in ChessGui.
  - The ChessQtTests list misses `AppLogTest`, `MoveGateTest` and `TestSupport.h`.

## 2. How Claude Code loads instructions (checked in its docs, 2026-11-22)

| Mechanism | When it loads | Fit |
|---|---|---|
| Root `CLAUDE.md` | At start, every session and every subagent | Only what every task needs; under 200 lines (aim ~120) |
| `<folder>/CLAUDE.md` | When Claude reads or edits a file in that folder; then it stays | Per-project descriptions |
| `.claude/rules/*.md` with `paths:` globs | When a matching file is read or edited; without `paths:` they load at start | Rules for one kind of file (QML, C++, tickets) |
| Skills `.claude/skills/*/SKILL.md` | Name and description only, until used; supporting files only when the skill points to them | Procedures: tickets, running the app, shipping a PR |
| `@path` imports | **Eagerly**, with the file that imports them | Save nothing in the root; avoid there |
| A `README.md` in a folder | Never by itself | Docs for people; a plain path mention costs nothing until read |

Docs: `code.claude.com/docs/en/memory.md`, `skills.md`, `sub-agents.md`, `best-practices.md`. The rule of
thumb from the best-practices guide: keep only what Claude can't learn by reading the code (commands,
decisions, traps). A file-by-file API list fails that test.

## 3. Target structure

### Root `CLAUDE.md`: under 120 lines, about 2.3k tokens
1. Project overview, in 2 lines.
2. **Where to read more** (new): a table of folder → its `CLAUDE.md` → what it covers, and the line
   "Before changing code in a folder, Read its CLAUDE.md (Grep/Glob don't load it)".
3. Build, and running tests: the commands only.
4. Seeing the UI: points to the `run-chess-app` skill.
5. Architecture: the 8 projects, one line each, and the dependency tree (corrected).
6. **Key Conventions**: the QML ↔ C++ rules 1–3 **unchanged and numbered as now** (tickets cite "rule 1").
   Two lines are lifted into it from deeper text:
   - "The Lichess token never reaches QML and is never exported."
   - "A new ChessGui `.cpp` goes into `ChessGuiTests.vcxproj` too."
7. Tasks and Bugs: folders, IDs, and "Saving tokens" **word for word**. The rest is in the `ticket` skill.
8. Session handover (`PROGRESS.md`) and branching: kept; "lasting rules go into their folder's CLAUDE.md".
9. **Writing these docs** (new):
   - a class is described in its folder's file by what it is and its invariants, never its ticket history;
   - the root stays under 120 lines;
   - no `@` imports in the root.

### Per-folder `CLAUDE.md` (loaded on demand)
| File | Content | ~Tokens |
|---|---|---:|
| `ChessCore/CLAUDE.md` | No Qt, `Chess::`; key types; Chess960 castling notation and X-FEN; invariants | 500 |
| `ChessTests/CLAUDE.md` | No Qt; the Chess960 reference and its script | 150 |
| `UciConnector/CLAUDE.md` | API, `SetChess960`, Chess960 requests (start FEN + every move), engine lookup | 200 |
| `FakeUci/CLAUDE.md` | What it answers, `FAKEUCI_MOVES` / `_DELAY_MS`, who runs it | 200 |
| `LiChessConnector/CLAUDE.md` | LichessClient: variants, players and flairs, streams and reconnect, events and challenges, rejoin, seek; LichessAuth; invariants; pointers to `docs/LI-*` | 700 |
| `ChessQtTests/CLAUDE.md` | One test file per class; Qt on PATH and Stockfish; the `FakeLichess` pattern | 350 |
| `ChessGui/ChessGui/CLAUDE.md` | Map of the folder, `AppServices` and the singletons, where the C++ details are | 300 |
| `ChessGui/ChessGui/qml/CLAUDE.md` | The window, the pages, the board's layers, shared components, dialogs: one line each; QML rules (objectName, `MultiEffect` guard, `ScreenshotScene` only, dialogs through signals) | 1,300 |
| `ChessGuiTests/CLAUDE.md` | AppDriver, TestSetup/TestRun, `// ui-test-runs`, FakeLichess in QML, writing a test (moved out of the skill) | 900 |
| `installer/CLAUDE.md` | `Test-Installer.ps1`, `release.yml`, icons, Stockfish scripts | 200 |

### Path-scoped rules (`.claude/rules/`, each with `paths:`)
| Rule | `paths:` | Content |
|---|---|---|
| `chessgui-cpp.md` | `ChessGui/ChessGui/*.{h,cpp}` | The C++ catalogue by area (game bridge, models, library and analysis, profiles, infrastructure) with an **Invariants** list, about 2.6k tokens instead of 4.4k. As a rule, it doesn't load for a QML-only task: a `ChessGui/ChessGui/CLAUDE.md` would, since `qml/` is under that folder. |
| `cpp-style.md` | `**/*.{h,cpp,hpp}` | Smart pointers, `Chess::` stays in ChessCore, `PieceColor::Light`, OO style (PR #92), `writeFileAtomically`, no formatters |
| `tasks.md` | `.tasks/**/*.md` | Frontmatter template, naming, step entry, close checklist |

### Skills
- **`ticket`** (new): new ticket (next free number), start, step entry, close.
- **`run-chess-app`** (split):
  - The scene catalogue moves to a `scenes.md` beside `SKILL.md`, read only when a scene is chosen. Using
    the skill then costs about 2.2k tokens instead of 5.6k.
  - Its UI-test section moves to `ChessGuiTests/CLAUDE.md`.
- **`ship-pr`** (optional, started by hand only): build, `Run-UiTests.ps1 -Build -Screens`, ticket
  resolution, `PROGRESS.md`, push, `gh pr create`.

### No per-folder README
A nested `CLAUDE.md` is plain Markdown that GitHub shows too. A README + CLAUDE.md pair would only
duplicate it. The root `README.md` stays for people (download, build, installer).

## 4. Saving

| Task | Loaded after the split | ~Tokens (now 10k) |
|---|---|---:|
| Ticket or docs | root + `tasks.md` | 2.7k |
| Chess rules | root + ChessCore + ChessTests + cpp-style | 3.2k |
| Lichess client | root + LiChessConnector + ChessQtTests + cpp-style | 3.4k |
| QML-only UI | root + ChessGui map + qml + ChessGuiTests | 4.8k |
| QML + ChessConnector | + chessgui-cpp + cpp-style | 7.5k |
| Everything | all files, loaded as they're needed | ~9.5k |

Every subagent also starts from the small root: about 7.7k tokens less per subagent.

## 5. Not losing anything

- **Kept in the root word for word:**
  - never read all of `.tasks/open/` and never `.tasks/archive/` unless asked;
  - QML ↔ C++ rules 1–3 and "one call per user action";
  - the token rule;
  - the branching rules (from `origin/develop`, naming, `Get-Date`, open the PR without asking, stop on
    unmerged dependencies);
  - `PROGRESS.md` under 15 lines;
  - the vcxproj rule.
- **Invariants hidden in history lines** get an explicit `### Invariants` list in their new file, for example:
  - rejected game files are never loaded;
  - only `games/<id>.json` names are written on import;
  - exports never carry the token;
  - Chess960 games are never rated;
  - a Chess960 engine request is always its start FEN and every move;
  - debug helpers live only in `ScreenshotScene`;
  - UI tests never reach lichess.org;
  - finished Lichess games are kept locally (LI-14's new decision).
- **Coverage check, mechanical:** every one of the 586 `` `code` `` spans in today's file is found in a new
  file, or listed in the PR as "dropped as history" with the ticket or doc that holds it.
- **Mapping:** each step's PR ticks off the line ranges it moves. The full table is in the review this plan
  comes from, summarised in §3.
- **Pointers to update:**
  - `.tasks/README.md` ("the rules are in CLAUDE.md");
  - `PROGRESS.md` ("lasting rules go into this file or the run-chess-app skill");
  - its duplicated offscreen gotcha.
- **Checked in a fresh session:** `/memory` and `/context` show which files loaded after reading a file in
  each area.

## 6. Steps (one docs PR each, a `### step` entry in BL-32)

0. **Pilot:**
   - Do: `installer/CLAUDE.md` and `.claude/rules/tasks.md`, with their lines out of the root.
   - Check in a fresh session: both load when their files are read; whether reading a `qml/` file loads
     `ChessGui/ChessGui/CLAUDE.md`. That decides between the rule and the folder file for the C++ catalogue.
1. **Core:**
   - Do: the `ChessCore`, `ChessTests`, `UciConnector` and `FakeUci` CLAUDE.md files, `cpp-style.md`, the root's
     "Where to read more" table, and the architecture fixed.
2. **Lichess:**
   - Do: `LiChessConnector/CLAUDE.md` and `ChessQtTests/CLAUDE.md`.
3. **ChessGui:**
   - Do: `ChessGui/ChessGui/CLAUDE.md`, `chessgui-cpp.md`, `qml/CLAUDE.md`; history cut.
   - Check: the owner reviews the Invariants lists.
4. **UI tests and skill:**
   - Do: `ChessGuiTests/CLAUDE.md`; `run-chess-app` split into `SKILL.md` and `scenes.md`.
5. **Tickets:**
   - Do: the `ticket` skill; the pointers updated.
6. **Final trim:**
   - Do: the root under 120 lines, "Writing these docs".
   - Check: the full identifier coverage, and three fresh-session checks (QML change, Lichess change, ticket).

## 7. Found on the way, for the owner
- **`.claude/settings.local.json` is tracked in git.** It's normally per user: permissions and local
  settings. To decide: untrack it (`git rm --cached`, add it to `.gitignore`) or keep it shared.
- **`.claude/agents/qt-android-analyzer.md` is untracked.** Keep it locally, or commit it.
- **`ChessCore/ReadMe.txt`** is Visual Studio wizard boilerplate: delete it in its own PR.
- **The owner's auto-memory notes** repeat some CLAUDE.md text ("Qt not on agent PATH", "UI tests run
  Release by default"). Trim them once the new files hold that text.

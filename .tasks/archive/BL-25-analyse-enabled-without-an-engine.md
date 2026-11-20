---
id: BL-25
title: "Analyse game is enabled without an engine until Settings has been opened"
type: bug
status: done
priority: medium
---

# BL-25 Analyse game is enabled without an engine until Settings has been opened

Found in Claude's run of `docs/BL-19-ManualTests.md` scenario 6 (2026-11-19) [Easy].

- **What happens:** with no engine (`CHESS_ENGINE_PATH` pointing to a file that isn't there, or no Stockfish installed), a finished game's **Analyse game** (the moves panel) is enabled in a session where the Settings page hasn't been opened yet. Pressing it fails: "The chess engine couldn't analyse the game." (log: `Analysis of <id> failed: UciConnector: failed to start stockfish`). Once Settings has been opened, the button is disabled, as scenario 6 expects. More → Analyse wasn't checked before Settings.
- **Cause:** `AppInfo.engineState` is "checking" until `AppInfo::probeEngine()` is done, and only `SettingsPage.qml` (`Component.onCompleted`) calls it. `AnalysisSummary.qml`'s `engineFound` is `AppInfo.engineState !== "missing"`, so "checking" counts as found.
- **Fix (to choose):**
  - probe once at start (`AppServices`, after the first frame), so the state is known before any game page; or
  - call `AppInfo.probeEngine()` from wherever the state is shown (the moves panel, More), and treat "checking" as not yet available.
  - Check the More menu's Analyse too (`GamePage.qml`, `analyseMenuItem`) and the vs computer card, if they read the same state.
- **Tests:** a UI test with `CHESS_ENGINE_PATH` set to a missing file: open a finished game straight from the Games page, Analyse game disabled (no Settings first).

## Resolution

### 2026-11-20 — BL-25: the engine looked for at start (branch: bugs/analyseNeedsEngine_191126)

- **Cause:** as found: `AppInfo.engineState` stayed "checking" until the Settings page called `probeEngine()`, and the three Analyse controls (the moves panel's button, More → Analyse, the result card's) took anything but "missing" as an engine.
- **Fix:** `AppServices::start()` asks for `AppInfo::probeEngine()` once the event loop runs (the app and the UI tests alike; Settings' own call does nothing then). `AppInfo.engineFound` (an engine answered) is what the three controls read now, so "checking" doesn't count either.
- **Tests:** `tst_noEngine.qml` (new, 3 functions), run without an engine by a new runner line `// ui-test-engine: none` (`Run-UiTests.ps1` points `CHESS_ENGINE_PATH` at a file that isn't there): looked for at start without Settings, a finished game's result card has no Analyse, the panel's and More's are disabled, also for the kept game opened from the Games page. It failed on the old build ("checking" after 10 s), passes now; `tst_analysis` still passes (FakeUci found at start). UI tests (Release): all files, 0 failed.

### 2026-11-20 — the code review of #204 (branch: bugs/reviewFixes203and204_201126)

- **Not final when missing:** a first start slower than the probe's 5 s (an antivirus scan of a new stockfish.exe) turned the analysis off for the session. `AppInfo::probeEngine()` now looks again while the state is "missing" (Settings, each game page's moves panel; the state stays "missing" meanwhile, one probe at a time; the warning logged once). `tst_noEngine` test 4: FakeUci appears later, the next game page finds it, Analyse is on.
- **While it looks:** the disabled Analyse's tip says "Looking for the chess engine…", not that it is missing; the screenshot scenes wait for the look (the result card's Analyse is there or not, never by timing).
- `Run-UiTests.ps1` puts `CHESS_ENGINE_PATH` back in a `finally` (Ctrl+C, an error); `tst_noEngine` says it needs the runner; Settings reads `engineFound`.
- **Not changed:** the engine check stays in QML beside `CanAnalyse` (`ChessConnector` would need `AppInfo` for it); the look at start stays (a lazy one brings back the result card's 3 → 4 buttons).

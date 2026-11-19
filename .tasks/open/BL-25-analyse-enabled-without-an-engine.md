---
id: BL-25
title: "Analyse game is enabled without an engine until Settings has been opened"
type: bug
status: open
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

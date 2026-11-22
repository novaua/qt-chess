# Active Handover

- **Last completed:** BL-37 (`bugs/emptyEngineVars_221126`): an empty `CHESS_ENGINE_PATH` counts as none (`TestSetup`, `Run-UiTests.ps1`), and the scripts remove a variable instead of leaving it empty: the UI tests no longer play Stockfish after a screenshot run.
- **Current focus:** BL-35 (split `ChessGame.qml`, plan `docs/BL-35-ChessGameSplitPlan.md`): step 6 next ("Games kept" → SettingsPage), the last. BL-28 waits for a next occurrence in CI.
- **Next up:** BL-35 step 6; BL-36 after BL-35; BL-38 (small); LI-14 (plan first); LI-09's remaining hand tests (scenarios 1, 3–9); LI-12; BL-33.
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`; `starting`, `timing`, `levels`, the message, challenge and profile scenes renewed from `develop` on 2026-11-22); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

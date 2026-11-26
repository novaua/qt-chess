# Active Handover

- **Last completed:** BL-36 (`features/bl36LeaveFlow_261126`): the leave, resign, switch-user and quit flow moved into `LeaveFlow.qml`; `ChessConnector.endGame()` now withdraws a waiting rematch challenge itself (root rule 1). BL-36 closed.
- **Current focus:** LI-16: its plan (`docs/LI-16-NewOpponentPlan.md`, branch `docs/li16Plan_261126`) waits for the owner's review; then step 1. BL-28 waits for a next occurrence in CI.
- **Next up:** LI-16 step 1 once its plan is approved; LI-15 (recent Lichess games from the server); LI-09's remaining hand tests (scenarios 1, 3–9); LI-12; BL-33.
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`; `starting`, `timing`, `levels`, the message, challenge and profile scenes renewed from `develop` on 2026-11-22); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`, `lichess-result`, `games-lichess`; `games-empty`'s line changed (LI-14). `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

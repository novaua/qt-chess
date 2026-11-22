# Active Handover

- **Last completed:** BL-38 (`bugs/figuresCallLater_221126`): ChessFigures' delayed call checks that its delegate is still there; `Run-UiTests.ps1` now fails a file whose log has a QML TypeError or ReferenceError.
- **Current focus:** LI-14 (analyse a Lichess game): plan `docs/LI-14-LichessAnalysisPlan.md` (`docs/li14Plan_221126`) waits for the owner's approval and answers to its five questions; then step 1. BL-28 waits for a next occurrence in CI.
- **Next up:** LI-14 step 1 once approved; BL-36 (the leave flow out of `ChessGame.qml`); LI-09's remaining hand tests (scenarios 1, 3–9); LI-12; BL-33.
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`; `starting`, `timing`, `levels`, the message, challenge and profile scenes renewed from `develop` on 2026-11-22); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

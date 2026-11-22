# Active Handover

- **Last completed:** LI-14 step 2 (`features/li14KeepAndAnalyse_231126`): a finished Lichess game is kept when its result comes (`keepLichessGame`), and Analyse is on its result card and in More.
- **Current focus:** LI-14 (plan `docs/LI-14-LichessAnalysisPlan.md`): step 3 next (the Games page's Lichess filter, card words and menu, "Open the analysis on lichess.org"; replaying a kept Lichess game with its players). BL-28 waits for a next occurrence in CI.
- **Next up:** LI-14 step 3; BL-36 (the leave flow out of `ChessGame.qml`); LI-09's remaining hand tests (scenarios 1, 3–9); LI-12; BL-33.
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`; `starting`, `timing`, `levels`, the message, challenge and profile scenes renewed from `develop` on 2026-11-22); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`, `lichess-result`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

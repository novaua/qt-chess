# Active Handover

- **Last completed:** BL-40 (`bugs/staleCheckSound_221126`): a check's sound and "Check" only if the board still shows it when it settles (found by the review of BL-35's PRs, with three comment fixes and sounds still loading played again).
- **Current focus:** None. BL-28 waits for a next occurrence in CI.
- **Next up:** BL-36 (the leave flow out of `ChessGame.qml`); BL-38 (small); LI-14 (plan first); LI-09's remaining hand tests (scenarios 1, 3–9); LI-12; BL-33.
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`; `starting`, `timing`, `levels`, the message, challenge and profile scenes renewed from `develop` on 2026-11-22); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

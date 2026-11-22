# Active Handover

- **Last completed:** BL-34 (`bugs/checkmateSoundAfterMove_221126`): the checkmate and win sounds wait for the board (`boardIdle`), so the mating move is seen before it is heard. Before it: the `CLAUDE.md` fact check (#235).
- **Current focus:** BL-35 (split `ChessGame.qml`): the plan `docs/BL-35-ChessGameSplitPlan.md` waits for the owner's approval; step 1 (`GameSounds.qml`) next. BL-28 waits for a next occurrence in CI.
- **Next up:** BL-35 step 1 once the plan is approved; LI-14 (plan first); LI-09's remaining hand tests (scenarios 1, 3–9); LI-12; BL-33.
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

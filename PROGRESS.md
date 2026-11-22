# Active Handover

- **Last completed:** BL-32 step 4, UI tests and the skill (`docs/bl32UiTests_221126`): `ChessGuiTests/CLAUDE.md`; `run-chess-app` split into `SKILL.md` and `scenes.md`. Before it: step 3, ChessGui (#231; its Invariants reviewed by the owner).
- **Current focus:** BL-32: step 5 (the `ticket` skill; the pointers in `.tasks/README.md` and this file) next (`docs/BL-32-ClaudeMdSplitPlan.md`). BL-28 waits for a next occurrence in CI.
- **Next up:** BL-32 steps 5–6; then LI-14 (plan first); LI-09's remaining hand tests (scenarios 1, 3–9); LI-12; BL-33.
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

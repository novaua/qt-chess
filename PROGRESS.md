# Active Handover

- **Last completed:** tickets LI-14 (analyse a Lichess game, high; finished Lichess games kept locally, the owner's decision), LI-15 (recent Lichess games from the server), LI-16 (the end of a Lichess game: Rematch, New opponent, Analyse), `docs/lichessGamesTickets_221126` (2026-11-22). Merged before it: BL-32's plan (#226, approved by merging), housekeeping (#227), LI-11 (#224).
- **Current focus:** BL-32, split CLAUDE.md per project: step 0 (pilot) next, in a fresh session (`docs/BL-32-ClaudeMdSplitPlan.md`). BL-28 waits for a next occurrence in CI.
- **Next up:** BL-32 steps 0–6; then LI-14 (plan first); LI-09's remaining hand tests (scenarios 1, 3–9); LI-12.
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - UI tests offscreen: hover doesn't reach items; no shader effects (the software renderer: pieces drawn without their glow; dialogs' backgrounds not drawn). `// ui-test-engine: none` runs a test file without an engine.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

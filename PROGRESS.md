# Active Handover

- **Last completed:** BL-27, `bugs/capturedStripGlow_201126`: the captured strip's pieces glow as the move list's (a black piece on a dark card); shader effects only off the software renderer, so offscreen the figures draw plain.
- **Current focus:** None.
- **Next up:** the owner's choice: hand tests (LI-01A on Lichess, LI-02, LI-05, LI-06, BL-10, TSK-02, TSK-04), or a plan for one of LI-09 (suggested), LI-08, BL-22, CE-01, LI-01B; LI-07 needs the owner's choice first.
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - UI tests offscreen: hover doesn't reach items; no shader effects (the software renderer: pieces drawn without their glow). `// ui-test-engine: none` runs a test file without an engine.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

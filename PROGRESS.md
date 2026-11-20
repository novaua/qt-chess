# Active Handover

- **Last completed:** BL-19 closed, `docs/bl19HandTestsDone_201126`: the owner's hand tests 6 (no engine, the tooltip) and 9.2 (Windows' contrast theme) passed in a real window; BL-27 found (a black captured piece on a dark card).
- **Current focus:** None.
- **Next up:** BL-27 (small, low: the captured strip's glow). Then the owner's choice: hand tests (LI-01A on Lichess, LI-02, LI-05, LI-06, BL-10, TSK-02, TSK-04), or a plan for one of LI-09 (suggested), LI-08, BL-22, CE-01, LI-01B; LI-07 needs the owner's choice first.
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - UI tests offscreen: hover doesn't reach items, and the move list's piece figures (a `layer.effect`) don't draw. `// ui-test-engine: none` runs a test file without an engine.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`.

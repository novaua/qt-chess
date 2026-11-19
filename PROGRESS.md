# Active Handover

- **Last completed:** BL-25, `bugs/analyseNeedsEngine_191126`: the engine is looked for at start, and Analyse is enabled only once one answered (it was enabled without one until Settings had been opened).
- **Current focus:** None. BL-19 is `in_progress`: the owner decides whether it is done (left for them: scenario 6's tooltip on hover, 9.2 Windows' contrast theme).
- **Next up:** BL-26 or LI-10 (both Easy).
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - UI tests offscreen: hover doesn't reach items, and the move list's piece figures (a `layer.effect`) don't draw. `// ui-test-engine: none` runs a test file without an engine.
  - Local screenshot baselines are stale (e.g. `settings`, `about`: rows added since); new scenes without one: `capture`, `capture-ep`, `premove-capture`.

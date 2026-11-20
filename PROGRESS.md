# Active Handover

- **Last completed:** the code review fixes of #203 (a pawn's straight premove: no corners; the corners made once per square) and #204 (a missing engine looked for again), `bugs/reviewFixes203and204_201126`. Before it: BL-26 and LI-10 (#205).
- **Current focus:** None. BL-19 is `in_progress`: the owner decides whether it is done (left for them: scenario 6's tooltip on hover, 9.2 Windows' contrast theme).
- **Next up:** no small ticket left. The owner's choice: hand tests (LI-01A on Lichess, LI-02, LI-05, LI-06, BL-10, …), or a plan for one of LI-07 (needs the owner's choice first), LI-08, LI-09, BL-22, LI-01B, CE-01.
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - UI tests offscreen: hover doesn't reach items, and the move list's piece figures (a `layer.effect`) don't draw. `// ui-test-engine: none` runs a test file without an engine.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`.

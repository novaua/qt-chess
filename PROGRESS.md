# Active Handover

- **Last completed:** BL-32's plan (`docs/claudeMdSplitPlan_221126`): split CLAUDE.md (10k tokens in every session) into a short root, per-folder CLAUDE.md files, path-scoped rules and skills; `docs/BL-32-ClaudeMdSplitPlan.md`, for the owner's approval (2026-11-22). Before it: LI-14–16 tickets (#225), LI-11 (#224).
- **Current focus:** BL-32 waits for the owner's approval of its plan. BL-28 waits for a next occurrence in CI.
- **Next up:** BL-32 step 0 (pilot) once approved; LI-14 (plan first); LI-09's remaining hand tests; LI-12.
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - UI tests offscreen: hover doesn't reach items; no shader effects (the software renderer: pieces drawn without their glow; dialogs' backgrounds not drawn). `// ui-test-engine: none` runs a test file without an engine.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

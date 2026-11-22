# Active Handover

- **Last completed:** BL-32 step 0, the pilot (`docs/bl32Pilot_221126`): `installer/CLAUDE.md` and `.claude/rules/tasks.md`, their lines out of the root; checked that both load on demand and that a `qml/` read loads `ChessGui/ChessGui/CLAUDE.md` (so the C++ catalogue goes into a rule). Before it: LI-14, LI-15, LI-16 tickets (#225), BL-32's plan (#226).
- **Current focus:** BL-32, split CLAUDE.md per project: step 1 (Core) next (`docs/BL-32-ClaudeMdSplitPlan.md`). BL-28 waits for a next occurrence in CI.
- **Next up:** BL-32 steps 1–6; then LI-14 (plan first); LI-09's remaining hand tests (scenarios 1, 3–9); LI-12.
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - UI tests offscreen: hover doesn't reach items; no shader effects (the software renderer: pieces drawn without their glow; dialogs' backgrounds not drawn). `// ui-test-engine: none` runs a test file without an engine.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

# Active Handover

- **Last completed:** LI-09 step 5, `docs/li09ManualTests_201126`: `docs/LI-09-ManualTests.md` (ten scenarios with two Lichess accounts) and CLAUDE.md's seek API (2026-11-20).
- **Current focus:** LI-09 Quick pairing: all five steps done; waits for the owner's hand tests (`docs/LI-09-ManualTests.md`) before it is closed.
- **Next up:** after the hand tests, close LI-09 (Resolution, `status: done`, `git mv` to `archive/`); then the next ticket from `.tasks/open/` (ask the owner).
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - UI tests offscreen: hover doesn't reach items; no shader effects (the software renderer: pieces drawn without their glow; dialogs' backgrounds not drawn). `// ui-test-engine: none` runs a test file without an engine.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

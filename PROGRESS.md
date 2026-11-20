# Active Handover

- **Last completed:** BL-28 step 1, `bugs/robotSceneEngineNoAnswer_211126`: the robot's path in every scene's log (`chess.engine`, on in `--screenshot` mode) and a timed-out scene names what it waited for; cause not found (40 local runs passed) (2026-11-21).
- **Current focus:** BL-28 waits for a next occurrence in CI (keep its `installer-test-results` artifact; read the scene log's `[Engine trace]` and `Screenshot:` lines). LI-09 waits for the owner's hand tests (`docs/LI-09-ManualTests.md`).
- **Next up:** after the hand tests, close LI-09 (Resolution, `status: done`, `git mv` to `archive/`); then the next ticket from `.tasks/open/` (ask the owner).
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - UI tests offscreen: hover doesn't reach items; no shader effects (the software renderer: pieces drawn without their glow; dialogs' backgrounds not drawn). `// ui-test-engine: none` runs a test file without an engine.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

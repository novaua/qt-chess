# Active Handover

- **Last completed:** LI-07 step 1, `features/lichessIdentityInGame_211126`: a Lichess game's cards show the Lichess players (title, name, rating; the player's profile name in the detail line) (2026-11-21).
- **Current focus:** LI-07 step 2, the flair (gameFull has none: `/api/account`, `/api/user/{name}`). BL-28 waits for a next occurrence in CI (keep its `installer-test-results` artifact). LI-09 waits for the owner's hand tests (`docs/LI-09-ManualTests.md`).
- **Next up:** LI-07 step 2; close LI-09 after its hand tests.
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - UI tests offscreen: hover doesn't reach items; no shader effects (the software renderer: pieces drawn without their glow; dialogs' backgrounds not drawn). `// ui-test-engine: none` runs a test file without an engine.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

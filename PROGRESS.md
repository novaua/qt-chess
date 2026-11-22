# Active Handover

- **Last completed:** LI-11, `features/lichessFlair_221126`: a Lichess opponent's flair is their avatar, else the new pawn-and-globe `online-player.png`; the player's flair after their name; Qt Image Formats (WebP) in CI and the installer (2026-11-22). #223 fixes `tst_lichess960` after #222.
- **Current focus:** None. BL-28 waits for a next occurrence in CI.
- **Next up:** LI-09's remaining hand tests (scenarios 1, 3–9); LI-12 (Lichess requests without a timeout).
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - UI tests offscreen: hover doesn't reach items; no shader effects (the software renderer: pieces drawn without their glow; dialogs' backgrounds not drawn). `// ui-test-engine: none` runs a test file without an engine.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

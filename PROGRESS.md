# Active Handover

- **Last completed:** BL-29, `bugs/safeProfileImport_211126`: a profile import is staged and swapped in, so a failed Replace no longer loses the old profile; 500 games import in 0.67 s (was 3.3 s) (2026-11-21).
- **Current focus:** None. BL-28 waits for a next occurrence in CI (keep its `installer-test-results` artifact). LI-09 waits for the owner's hand tests (`docs/LI-09-ManualTests.md`).
- **Next up:** close LI-09 after its hand tests; then LI-12 (Lichess requests without a timeout).
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - UI tests offscreen: hover doesn't reach items; no shader effects (the software renderer: pieces drawn without their glow; dialogs' backgrounds not drawn). `// ui-test-engine: none` runs a test file without an engine.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

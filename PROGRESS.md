# Active Handover

- **Last completed:** the async review of file and network code (`docs/asyncReviewTickets_211126`): no broad move to async APIs needed (the Lichess code is already asynchronous); five tickets filed: LI-12, LI-13, BL-29, BL-30, BL-31 (2026-11-21).
- **Current focus:** None. BL-28 waits for a next occurrence in CI (keep its `installer-test-results` artifact). LI-09 waits for the owner's hand tests (`docs/LI-09-ManualTests.md`).
- **Next up:** close LI-09 after its hand tests; then BL-29 (a Replace import can lose the old profile) or LI-12 (Lichess requests without a timeout).
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - UI tests offscreen: hover doesn't reach items; no shader effects (the software renderer: pieces drawn without their glow; dialogs' backgrounds not drawn). `// ui-test-engine: none` runs a test file without an engine.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

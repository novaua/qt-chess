# Active Handover

- **Last completed:** LI-09 follow-up, `features/quickPairingNoRating_221126`: the owner's first hand tests written into `docs/LI-09-ManualTests.md`; the Quick pairing tiles without the rating; fast games on our own service noted in TSK-13 (2026-11-22). BL-29 (PR #221): a profile import can't lose the old profile.
- **Current focus:** LI-11, the flair and a standard avatar for Lichess players (Qt Image Formats to add): plan next. BL-28 waits for a next occurrence in CI.
- **Next up:** LI-11; LI-09's remaining hand tests (scenarios 1, 3–9); LI-12 (Lichess requests without a timeout).
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - UI tests offscreen: hover doesn't reach items; no shader effects (the software renderer: pieces drawn without their glow; dialogs' backgrounds not drawn). `// ui-test-engine: none` runs a test file without an engine.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

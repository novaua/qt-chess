# Active Handover

- **Last completed:** LI-07 closed (`docs/closeLi07_211126`): step 1 (PR #216) shows the Lichess players on the cards; the flair (WebP only on lichess.org, Qt has no WebP plugin here) moved to LI-11 (2026-11-21). PR #217: LiChessConnector on C++20.
- **Current focus:** None. BL-28 waits for a next occurrence in CI (keep its `installer-test-results` artifact). LI-09 waits for the owner's hand tests (`docs/LI-09-ManualTests.md`).
- **Next up:** close LI-09 after its hand tests; then the next ticket from `.tasks/open/` (ask the owner).
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - UI tests offscreen: hover doesn't reach items; no shader effects (the software renderer: pieces drawn without their glow; dialogs' backgrounds not drawn). `// ui-test-engine: none` runs a test file without an engine.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

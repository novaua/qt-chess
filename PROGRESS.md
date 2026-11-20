# Active Handover

- **Last completed:** LI-09 step 2, `features/li09QuickPairingCard_201126`: the Online page's Quick pairing card (`QuickPairingGrid`, Rated / Casual, its Variant row), `tst_lichess` test_13 (2026-11-20).
- **Current focus:** LI-09 Quick pairing: steps 1–2 done; 3–5 left (`docs/LI-09-QuickPairingPlan.md` §5).
- **Next up:** LI-09 step 3 (the seek goes on beyond the Online page: `SeekIndicator` in `CommandBar`, the message on leaving; today leaving the page cancels it), on its own branch from `develop`.
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - UI tests offscreen: hover doesn't reach items; no shader effects (the software renderer: pieces drawn without their glow). `// ui-test-engine: none` runs a test file without an engine.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

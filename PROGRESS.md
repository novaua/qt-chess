# Active Handover

- **Last completed:** LI-09 step 4, `features/li09SeekDialog_201126`: Quick pairing's Custom tile opens `SeekDialog` (real time on lichess.org's steps, or correspondence; Seek off below Rapid; a correspondence seek's message with Open on lichess.org), `tst_lichess` test_15, scenes `seek-dialog*` (2026-11-20).
- **Current focus:** LI-09 Quick pairing: steps 1–4 done; 5 left (`docs/LI-09-QuickPairingPlan.md` §5).
- **Next up:** LI-09 step 5 (CLAUDE.md, `docs/LI-09-ManualTests.md` with two Lichess accounts), on its own branch from `develop`.
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - UI tests offscreen: hover doesn't reach items; no shader effects (the software renderer: pieces drawn without their glow; dialogs' backgrounds not drawn). `// ui-test-engine: none` runs a test file without an engine.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

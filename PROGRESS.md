# Active Handover

- **Last completed:** LI-09 step 1, `features/li09QuickPairing_201126`: `LichessClient::startSeek` / `cancelSeek`, the `seek` state, the fallback; `LichessSeekTest.cpp` against `FakeSeekLichess` (2026-11-20).
- **Current focus:** LI-09 Quick pairing: step 1 (C++) done; steps 2–5 left (`docs/LI-09-QuickPairingPlan.md` §5).
- **Next up:** LI-09 step 2 (the Online page's Quick pairing card, `QuickPairingGrid`, `fakelichessserver`'s seek route, `tst_lichess`), on its own branch from `develop`.
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - UI tests offscreen: hover doesn't reach items; no shader effects (the software renderer: pieces drawn without their glow). `// ui-test-engine: none` runs a test file without an engine.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

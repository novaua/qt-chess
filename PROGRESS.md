# Active Handover

- **Last completed:** BL-35 step 5 (`features/bl35ProfileFiles_221126`): `ProfileFiles.qml` (export, import, delete) and `DataCheckNotices.qml`; `tst_profiles` test 5 (delete the active profile); no behaviour change.
- **Current focus:** BL-35 (split `ChessGame.qml`, plan `docs/BL-35-ChessGameSplitPlan.md`): step 6 next ("Games kept" → SettingsPage), the last. BL-28 waits for a next occurrence in CI.
- **Next up:** BL-35 step 6; BL-36 after BL-35; BL-37, BL-38 (small); LI-14 (plan first); LI-09's remaining hand tests (scenarios 1, 3–9); LI-12; BL-33.
- **Active gotchas:**
  - The robot's UI tests fail together while the PC is busy (the app with Stockfish beside them): BL-37; run them again on a quiet PC.
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`; `starting`, `timing`, `levels`, the message, challenge and profile scenes renewed from `develop` on 2026-11-22); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

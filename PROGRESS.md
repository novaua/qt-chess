# Active Handover

- **Last completed:** LI-17 (`bugs/withdrawnChallengeResumed_261126`): a withdrawn challenge is dropped at once (`cancelChallenge`), so the Online page no longer resumes it; it was what failed `test_20_newOpponent` on `develop`. LI-17 closed.
- **Current focus:** LI-12 (PR #259, `bugs/li12LichessTimeout_261126`): done, its CI red only from LI-17; merge after this. LI-16: its hand tests (`docs/LI-16-ManualTests.md`) wait for the owner. BL-28 waits for a next occurrence in CI.
- **Next up:** LI-15 (a plan first). The owner's hand tests: LI-16, LI-09 (1, 3–9).
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`; `starting`, `timing`, `levels`, the message, challenge and profile scenes renewed from `develop` on 2026-11-22); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`, `lichess-result`, `lichess-new-opponent`, `games-lichess`; `games-empty`'s line changed (LI-14). `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

# Active Handover

- **Last completed:** LI-12 (`bugs/li12LichessTimeout_261126`): a 30 s inactivity timeout on every Lichess request; a timed-out one fails as "Lichess didn't answer" and its busy state clears. LI-12 closed.
- **Current focus:** LI-16: coded; its hand tests on lichess.org (`docs/LI-16-ManualTests.md`) wait for the owner, then it closes. BL-28 waits for a next occurrence in CI.
- **Next up:** LI-15 (a plan first). The owner's hand tests: LI-16, LI-09 (1, 3–9).
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`; `starting`, `timing`, `levels`, the message, challenge and profile scenes renewed from `develop` on 2026-11-22); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`, `lichess-result`, `lichess-new-opponent`, `games-lichess`; `games-empty`'s line changed (LI-14). `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

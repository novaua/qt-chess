# Active Handover

- **Last completed:** BL-33 (`features/bl33RemoveNetworkPlayer_261126`): the unbuilt `networkplayer.h/cpp` sketch removed. BL-33 closed.
- **Current focus:** LI-16: coded; its hand tests on lichess.org (`docs/LI-16-ManualTests.md`) wait for the owner, then it closes. BL-28 waits for a next occurrence in CI.
- **Next up:** LI-15 (recent Lichess games from the server: a plan first); LI-09's remaining hand tests (scenarios 1, 3–9); LI-12.
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`; `starting`, `timing`, `levels`, the message, challenge and profile scenes renewed from `develop` on 2026-11-22); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`, `lichess-result`, `lichess-new-opponent`, `games-lichess`; `games-empty`'s line changed (LI-14). `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).

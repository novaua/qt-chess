# Active Handover

- **Last completed:** BL-33 (`features/bl33RemoveNetworkPlayer_261126`): the unbuilt `networkplayer.h/cpp` sketch removed. BL-33 closed.
- **Current focus:** LI-16: coded; its hand tests on lichess.org (`docs/LI-16-ManualTests.md`) wait for the owner, then it closes. BL-28 waits for a next occurrence in CI.
- **Next up:** **LI-12** (the owner, 2026-11-26: the next session starts it): a 30 s inactivity timeout on every Lichess request (`LichessClient`, `LichessAuth`); a timed-out game stream fails as one ("Lichess didn't answer") instead of looking stopped by us, so the busy states clear; ChessQtTests against a silent FakeLichess route. The ticket has the fix and the test; no plan needed. Then LI-15 (a plan first); the owner's hand tests: LI-16, LI-09 (1, 3–9).
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`; `starting`, `timing`, `levels`, the message, challenge and profile scenes renewed from `develop` on 2026-11-22); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`, `lichess-result`, `lichess-new-opponent`, `games-lichess`; `games-empty`'s line changed (LI-14). `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).
